#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ole2.h>
#include <initguid.h>
#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "dc2jet.h"
#include "dc2sql_diag.h"

/* =========================================================================
 * DYNAMIC TRACKER DLL LOADER (Replaces Tracker.h / MW_WINS)
 * ========================================================================= */
typedef enum _FG_NAMESPACE {
    FG_NS_USER   = 0,
    FG_NS_GDI    = 1,
    FG_NS_KERNEL = 2,
    FG_NS_MODULE = 3,
    FG_NS_REG    = 4,
    FG_NS_ID64   = 5,
    FG_NS_COUNT  = 6
} FG_NAMESPACE;

typedef int    (*PFN_InitTrackerEngine)(void);
typedef void   (*PFN_ShutdownTrackerEngine)(void);
typedef UINT64 (*PFN_FG_LetEx)(FG_NAMESPACE ns, UINT64 id, SIZE_T size);
typedef VOID*  (*PFN_FG_GetEx)(FG_NAMESPACE ns, UINT64 id);
typedef VOID   (*PFN_FG_SetEx)(FG_NAMESPACE ns, UINT64 id, VOID* src, SIZE_T size);

static struct {
    HMODULE                   hDll;
    PFN_InitTrackerEngine     InitTrackerEngine;
    PFN_ShutdownTrackerEngine ShutdownTrackerEngine;
    PFN_FG_LetEx              LetEx;
    PFN_FG_GetEx              GetEx;
    PFN_FG_SetEx              SetEx;
} g_Trk = {0};

struct JetContext {
    SQLHENV  henv;
    SQLHDBC  hdbc;
    SQLHSTMT hstmt;
    char     db_path[MAX_PATH];
    char     passkey[128];
    DWORD    last_active_tick;
    BOOL     is_connected;
};

static void EnsureTrackerInitialized(void) {
    if (g_Trk.hDll) return;

    char dir[MAX_PATH] = {0};
    HMODULE hSelf = NULL;
    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&EnsureTrackerInitialized,
        &hSelf
    );

    if (hSelf && GetModuleFileNameA(hSelf, dir, MAX_PATH) > 0) {
        char* slash = strrchr(dir, '\\');
        if (slash) {
            *(slash + 1) = '\0';
            char candidate[MAX_PATH] = {0};
            snprintf(candidate, sizeof(candidate), "%sTracker.dll", dir);
            g_Trk.hDll = LoadLibraryA(candidate);
            if (!g_Trk.hDll) {
                snprintf(candidate, sizeof(candidate), "%sdc_tracker.dll", dir);
                g_Trk.hDll = LoadLibraryA(candidate);
            }
        }
    }

    if (!g_Trk.hDll) g_Trk.hDll = LoadLibraryA("Tracker.dll");
    if (!g_Trk.hDll) g_Trk.hDll = LoadLibraryA("dc_tracker.dll");
    if (!g_Trk.hDll) return;

    g_Trk.InitTrackerEngine     = (PFN_InitTrackerEngine)    GetProcAddress(g_Trk.hDll, "InitTrackerEngine");
    g_Trk.ShutdownTrackerEngine = (PFN_ShutdownTrackerEngine)GetProcAddress(g_Trk.hDll, "ShutdownTrackerEngine");
    g_Trk.LetEx                 = (PFN_FG_LetEx)             GetProcAddress(g_Trk.hDll, "_FG_LetEx");
    g_Trk.GetEx                 = (PFN_FG_GetEx)             GetProcAddress(g_Trk.hDll, "_FG_GetEx");
    g_Trk.SetEx                 = (PFN_FG_SetEx)             GetProcAddress(g_Trk.hDll, "_FG_SetEx");

    if (!g_Trk.InitTrackerEngine || !g_Trk.LetEx || !g_Trk.GetEx || !g_Trk.SetEx) {
        FreeLibrary(g_Trk.hDll);
        memset(&g_Trk, 0, sizeof(g_Trk));
        return;
    }

    g_Trk.InitTrackerEngine();
}

static BOOL IsAccdbFile(const char* path) {
    if (!path) return FALSE;
    const char* ext = strrchr(path, '.');
    return (ext && _stricmp(ext, ".accdb") == 0);
}

static BOOL EnsureDatabaseExistsDAO(const char* db_path, const char* passkey) {
    DWORD attr = GetFileAttributesA(db_path);
    if (attr != INVALID_FILE_ATTRIBUTES && attr != FILE_ATTRIBUTE_DIRECTORY) {
        diag_log_event("DAO", "Exclusive Jet database container active.");
        return TRUE;
    }

    diag_log_event("DAO", "Initializing COM Apartment for Jet/ACE provisioning...");
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    BOOL created = FALSE;
    BOOL isAccdb = IsAccdbFile(db_path);

    CLSID clsid;
    if (isAccdb) {
        hr = CLSIDFromProgID(L"DAO.DBEngine.120", &clsid);
        if (FAILED(hr)) hr = CLSIDFromProgID(L"DAO.DBEngine.36", &clsid);
    } else {
        hr = CLSIDFromProgID(L"DAO.DBEngine.36", &clsid);
        if (FAILED(hr)) hr = CLSIDFromProgID(L"DAO.DBEngine.120", &clsid);
    }

    if (SUCCEEDED(hr)) {
        IDispatch* pDBEngine = NULL;
        hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IDispatch, (void**)&pDBEngine);
        if (SUCCEEDED(hr) && pDBEngine) {
            IDispatch* pWorkspace = NULL;
            VARIANTARG vWsArg[2];
            VARIANT vWsRes;
            DISPPARAMS dpWs = { vWsArg, NULL, 2, 0 };

            VariantInit(&vWsArg[1]);
            vWsArg[1].vt = VT_BSTR;
            vWsArg[1].bstrVal = SysAllocString(L"#DefaultUser#");

            VariantInit(&vWsArg[0]);
            vWsArg[0].vt = VT_BSTR;
            vWsArg[0].bstrVal = SysAllocString(L"Admin");

            VariantInit(&vWsRes);
            DISPID dispidCreateWs;
            OLECHAR* szCreateWs = L"CreateWorkspace";

            if (SUCCEEDED(pDBEngine->lpVtbl->GetIDsOfNames(pDBEngine, &IID_NULL, &szCreateWs, 1, LOCALE_USER_DEFAULT, &dispidCreateWs))) {
                if (SUCCEEDED(pDBEngine->lpVtbl->Invoke(pDBEngine, dispidCreateWs, &IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dpWs, &vWsRes, NULL, NULL))) {
                    if (vWsRes.vt == VT_DISPATCH) {
                        pWorkspace = vWsRes.pdispVal;
                    }
                }
            }
            SysFreeString(vWsArg[1].bstrVal);
            SysFreeString(vWsArg[0].bstrVal);

            IDispatch* pTargetWorkspace = pWorkspace ? pWorkspace : pDBEngine;

            VARIANTARG vArg[3];
            VARIANT vRes;
            DISPPARAMS dp = { vArg, NULL, 3, 0 };

            WCHAR wPath[MAX_PATH];
            MultiByteToWideChar(CP_ACP, 0, db_path, -1, wPath, MAX_PATH);

            WCHAR wLocale[256];
            if (passkey && passkey[0] != '\0') {
                WCHAR wPass[128];
                MultiByteToWideChar(CP_ACP, 0, passkey, -1, wPass, 128);
                swprintf_s(wLocale, 256, L";LANGID=0x0409;CP=1252;COUNTRY=0;PWD=%s", wPass);
            } else {
                wcscpy_s(wLocale, 256, L";LANGID=0x0409;CP=1252;COUNTRY=0");
            }

            VariantInit(&vArg[2]);
            vArg[2].vt = VT_BSTR;
            vArg[2].bstrVal = SysAllocString(wPath);

            VariantInit(&vArg[1]);
            vArg[1].vt = VT_BSTR;
            vArg[1].bstrVal = SysAllocString(wLocale);

            VariantInit(&vArg[0]);
            vArg[0].vt = VT_I4;
            vArg[0].lVal = isAccdb ? 128 : 64; /* 128 = dbVersion120 (.accdb), 64 = dbVersion40 (.mdb) */

            VariantInit(&vRes);
            DISPID dispid;
            OLECHAR* szMember = L"CreateDatabase";
            hr = pTargetWorkspace->lpVtbl->GetIDsOfNames(pTargetWorkspace, &IID_NULL, &szMember, 1, LOCALE_USER_DEFAULT, &dispid);

            if (SUCCEEDED(hr)) {
                hr = pTargetWorkspace->lpVtbl->Invoke(pTargetWorkspace, dispid, &IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dp, &vRes, NULL, NULL);
                if (SUCCEEDED(hr)) {
                    if (vRes.vt == VT_DISPATCH && vRes.pdispVal) {
                        DISPID dispidClose;
                        OLECHAR* szClose = L"Close";
                        if (SUCCEEDED(vRes.pdispVal->lpVtbl->GetIDsOfNames(vRes.pdispVal, &IID_NULL, &szClose, 1, LOCALE_USER_DEFAULT, &dispidClose))) {
                            DISPPARAMS dpClose = { NULL, NULL, 0, 0 };
                            vRes.pdispVal->lpVtbl->Invoke(vRes.pdispVal, dispidClose, &IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dpClose, NULL, NULL, NULL);
                        }
                        vRes.pdispVal->lpVtbl->Release(vRes.pdispVal);
                        diag_log_event("DAO", isAccdb ? "ACE 12.0 container created cleanly." : "Jet 4.0 container created cleanly.");
                    }
                    created = TRUE;
                }
            }

            SysFreeString(vArg[2].bstrVal);
            SysFreeString(vArg[1].bstrVal);

            if (pWorkspace) pWorkspace->lpVtbl->Release(pWorkspace);
            pDBEngine->lpVtbl->Release(pDBEngine);
        }
    }

    if (SUCCEEDED(hr) || hr == S_FALSE) {
        CoUninitialize();
    }

    Sleep(50);
    return created;
}

static void BootstrapJetDatabaseSchema(SQLHDBC hdbc) {
    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    if (SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt) != SQL_SUCCESS) return;

    const char* createTableSQL =
        "CREATE TABLE SystemNodes ("
        "   NodeID COUNTER PRIMARY KEY, "
        "   NodeVersion TEXT(64), "
        "   Operator TEXT(64), "
        "   Details MEMO"
        ")";

    SQLExecDirectA(hstmt, (SQLCHAR*)createTableSQL, SQL_NTS);
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
}

JET_API JetContext* __stdcall jet_initialize(const char* base_dir, const char* db_filename, const char* passkey) {
    EnsureTrackerInitialized();

    JetContext* ctx = (JetContext*)malloc(sizeof(JetContext));
    if (!ctx) return NULL;
    memset(ctx, 0, sizeof(JetContext));

    if (g_Trk.LetEx && g_Trk.SetEx) {
        g_Trk.LetEx(FG_NS_MODULE, (UINT64)(uintptr_t)ctx, sizeof(JetContext));
        g_Trk.SetEx(FG_NS_MODULE, (UINT64)(uintptr_t)ctx, ctx, sizeof(JetContext));
    }

    char rawPath[MAX_PATH] = {0};
    if (base_dir && base_dir[0] != '\0') {
        snprintf(rawPath, sizeof(rawPath), "%s\\%s", base_dir, db_filename);
    } else {
        snprintf(rawPath, sizeof(rawPath), "%s", db_filename);
    }

    if (GetFullPathNameA(rawPath, MAX_PATH, ctx->db_path, NULL) == 0) {
        strncpy_s(ctx->db_path, sizeof(ctx->db_path), rawPath, _TRUNCATE);
    }

    if (passkey && passkey[0] != '\0') {
        strncpy_s(ctx->passkey, sizeof(ctx->passkey), passkey, _TRUNCATE);
    }

    HKEY hOdbcKey = NULL;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\ODBC\\ODBC.INI", 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hOdbcKey, NULL) == ERROR_SUCCESS) {
        RegCloseKey(hOdbcKey);
    }

    diag_log_event("JET", "Ensuring database container exists...");
    EnsureDatabaseExistsDAO(ctx->db_path, ctx->passkey);

    ctx->last_active_tick = GetTickCount();

    if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HENV, &ctx->henv) != SQL_SUCCESS) {
        diag_log_event("JET", "CRITICAL: Failed ODBC Environment handle allocation.");
        free(ctx);
        return NULL;
    }
    if (g_Trk.LetEx && g_Trk.SetEx) {
        g_Trk.LetEx(FG_NS_KERNEL, (UINT64)(uintptr_t)ctx->henv, sizeof(SQLHENV));
        g_Trk.SetEx(FG_NS_KERNEL, (UINT64)(uintptr_t)ctx->henv, &ctx->henv, sizeof(SQLHENV));
    }

    SQLSetEnvAttr(ctx->henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

    if (SQLAllocHandle(SQL_HANDLE_DBC, ctx->henv, &ctx->hdbc) != SQL_SUCCESS) {
        SQLFreeHandle(SQL_HANDLE_ENV, ctx->henv);
        free(ctx);
        return NULL;
    }
    if (g_Trk.LetEx && g_Trk.SetEx) {
        g_Trk.LetEx(FG_NS_KERNEL, (UINT64)(uintptr_t)ctx->hdbc, sizeof(SQLHDBC));
        g_Trk.SetEx(FG_NS_KERNEL, (UINT64)(uintptr_t)ctx->hdbc, &ctx->hdbc, sizeof(SQLHDBC));
    }

    const char* driver = IsAccdbFile(ctx->db_path)
        ? "Microsoft Access Driver (*.mdb, *.accdb)"
        : "Microsoft Access Driver (*.mdb)";

    char connStr[512];
    if (ctx->passkey[0] != '\0') {
        snprintf(connStr, sizeof(connStr), "Driver={%s};DBQ=%s;Pwd=%s;", driver, ctx->db_path, ctx->passkey);
    } else {
        snprintf(connStr, sizeof(connStr), "Driver={%s};DBQ=%s;", driver, ctx->db_path);
    }

    SQLCHAR outConn[1024];
    SQLSMALLINT outLen = 0;
    SQLRETURN ret = SQLDriverConnectA(ctx->hdbc, NULL, (SQLCHAR*)connStr, SQL_NTS, outConn, sizeof(outConn), &outLen, SQL_DRIVER_NOPROMPT);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        SQLCHAR sqlState[6] = {0};
        SQLCHAR errMsg[512] = {0};
        SQLINTEGER nativeErr = 0;
        SQLSMALLINT errLen = 0;
        if (SQLGetDiagRecA(SQL_HANDLE_DBC, ctx->hdbc, 1, sqlState, &nativeErr, errMsg, sizeof(errMsg), &errLen) == SQL_SUCCESS) {
            char diagBuf[640];
            snprintf(diagBuf, sizeof(diagBuf), "SQLDriverConnectA failed [%s | %ld]: %s", sqlState, (long)nativeErr, errMsg);
            diag_log_event("ODBC", diagBuf);
        } else {
            diag_log_event("JET", "CRITICAL: SQLDriverConnectA failed.");
        }
        SQLFreeHandle(SQL_HANDLE_DBC, ctx->hdbc);
        SQLFreeHandle(SQL_HANDLE_ENV, ctx->henv);
        free(ctx);
        return NULL;
    }

    ctx->is_connected = TRUE;
    if (g_Trk.SetEx) {
        g_Trk.SetEx(FG_NS_MODULE, (UINT64)(uintptr_t)ctx, ctx, sizeof(JetContext));
    }

    diag_log_event("JET", "ODBC connection established successfully.");
    diag_log_event("JET", connStr);

    BootstrapJetDatabaseSchema(ctx->hdbc);
    return ctx;
}


JET_API bool __stdcall jet_execute_action(JetContext* ctx, const char* sql_command) {
    if (!ctx || !ctx->is_connected || !sql_command) return false;
    diag_log_event("ACTION", sql_command);

    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    if (SQLAllocHandle(SQL_HANDLE_STMT, ctx->hdbc, &hstmt) != SQL_SUCCESS) return false;

    SQLRETURN ret = SQLExecDirectA(hstmt, (SQLCHAR*)sql_command, SQL_NTS);
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    return (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO);
}

JET_API bool __stdcall jet_execute_select(JetContext* ctx, const char* sql_command, JetRowCallback callback, void* user_data) {
    if (!ctx || !ctx->is_connected || !sql_command || !callback) return false;
    diag_log_event("SELECT", sql_command);

    SQLHSTMT hstmt = SQL_NULL_HSTMT;
    if (SQLAllocHandle(SQL_HANDLE_STMT, ctx->hdbc, &hstmt) != SQL_SUCCESS) return false;

    SQLRETURN ret = SQLExecDirectA(hstmt, (SQLCHAR*)sql_command, SQL_NTS);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        return false;
    }

    SQLSMALLINT num_cols = 0;
    SQLNumResultCols(hstmt, &num_cols);
    if (num_cols <= 0) {
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        return true;
    }

    const char** col_names = (const char**)malloc((size_t)num_cols * sizeof(char*));
    char** col_vals_buf = (char**)malloc((size_t)num_cols * sizeof(char*));

    for (SQLSMALLINT i = 0; i < num_cols; i++) {
        char nameBuf[256] = {0};
        SQLDescribeColA(hstmt, i + 1, (SQLCHAR*)nameBuf, sizeof(nameBuf), NULL, NULL, NULL, NULL, NULL);
        col_names[i] = _strdup(nameBuf[0] != '\0' ? nameBuf : "Col");
        col_vals_buf[i] = (char*)malloc(2048);
        memset(col_vals_buf[i], 0, 2048);
    }

    bool success = true;
    while (SQLFetch(hstmt) == SQL_SUCCESS) {
        for (SQLSMALLINT i = 0; i < num_cols; i++) {
            SQLLEN indicator = 0;
            memset(col_vals_buf[i], 0, 2048);
            SQLGetData(hstmt, i + 1, SQL_C_CHAR, col_vals_buf[i], 2048, &indicator);
            if (indicator == SQL_NULL_DATA) {
                col_vals_buf[i][0] = '\0';
            }
        }
        if (!callback(num_cols, col_names, (const char**)col_vals_buf, user_data)) {
            success = false;
            break;
        }
    }

    for (SQLSMALLINT i = 0; i < num_cols; i++) {
        free((void*)col_names[i]);
        free(col_vals_buf[i]);
    }
    free(col_names);
    free(col_vals_buf);

    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    diag_log_event("SELECT", "Query completed and rows streamed successfully.");
    return success;
}
JET_API void __stdcall jet_shutdown(JetContext* ctx) {
    if (!ctx) return;
    diag_log_event("JET", "Shutting down Jet context...");

    if (g_Trk.LetEx) {
        g_Trk.LetEx(FG_NS_KERNEL, (UINT64)(uintptr_t)ctx->hdbc, 0);
        g_Trk.LetEx(FG_NS_KERNEL, (UINT64)(uintptr_t)ctx->henv, 0);
        g_Trk.LetEx(FG_NS_MODULE, (UINT64)(uintptr_t)ctx, 0);
    }

    if (ctx->is_connected) {
        SQLDisconnect(ctx->hdbc);
    }
    if (ctx->hdbc) SQLFreeHandle(SQL_HANDLE_DBC, ctx->hdbc);
    if (ctx->henv) SQLFreeHandle(SQL_HANDLE_ENV, ctx->henv);
    free(ctx);
}


/* =========================================================================
 * SESSION MANAGEMENT & UTILITIES (Backed by Tracker.dll FG_NS_ID64)
 * ========================================================================= */
static volatile LONG64 g_NextJetSessionId  = 0x1000;
static volatile LONG   g_ActiveJetSessions = 0;

JET_API bool __stdcall jet_route_path(const char* prefix, const char* input_name, const char* suffix, char* out_buffer, size_t max_size) {
    if (!out_buffer || max_size == 0) return false;
    out_buffer[0] = '\0';

    const char* pfx = prefix ? prefix : "";
    const char* inp = input_name ? input_name : "";
    const char* sfx = suffix ? suffix : "";

    size_t pfxLen = strlen(pfx);
    bool needSlash = (pfxLen > 0 && pfx[pfxLen - 1] != '\\' && pfx[pfxLen - 1] != '/' && inp[0] != '\0');

    int written = snprintf(out_buffer, max_size, "%s%s%s%s",
                           pfx,
                           needSlash ? "\\" : "",
                           inp,
                           sfx);

    return (written >= 0 && (size_t)written < max_size);
}

JET_API JetSessionHandle __stdcall jet_open_session(const char* session_name, const char* base_dir, const char* db_filename, const char* passkey) {
    (void)session_name;
    if (!db_filename || db_filename[0] == '\0') return 0;
    EnsureTrackerInitialized();
    if (!g_Trk.LetEx || !g_Trk.SetEx) return 0;

    JetContext* ctx = jet_initialize(base_dir, db_filename, passkey);
    if (!ctx) return 0;

    UINT64 sid = (UINT64)InterlockedIncrement64(&g_NextJetSessionId);
    if (g_Trk.LetEx(FG_NS_ID64, sid, sizeof(JetContext*)) == 0) {
        jet_shutdown(ctx);
        return 0;
    }

    g_Trk.SetEx(FG_NS_ID64, sid, &ctx, sizeof(JetContext*));
    InterlockedIncrement(&g_ActiveJetSessions);
    diag_log_event("JET", "Session registered in Tracker.dll.");
    return (JetSessionHandle)sid;
}

JET_API JetContext* __stdcall jet_get_session(JetSessionHandle session_handle) {
    if (!session_handle || !g_Trk.GetEx) return NULL;
    JetContext** pCtx = (JetContext**)g_Trk.GetEx(FG_NS_ID64, (UINT64)session_handle);
    if (!pCtx) return NULL;
    return *pCtx;
}

JET_API bool __stdcall jet_close_session(JetSessionHandle session_handle) {
    if (!session_handle || !g_Trk.GetEx || !g_Trk.LetEx) return false;

    JetContext** pCtx = (JetContext**)g_Trk.GetEx(FG_NS_ID64, (UINT64)session_handle);
    if (!pCtx) return false;

    JetContext* ctx = *pCtx;
    if (ctx) {
        jet_shutdown(ctx);
    }

    g_Trk.LetEx(FG_NS_ID64, (UINT64)session_handle, 0);
    if (g_ActiveJetSessions > 0) {
        InterlockedDecrement(&g_ActiveJetSessions);
    }
    diag_log_event("JET", "Session closed and freed from Tracker.dll.");
    return true;
}

JET_API int __stdcall jet_get_active_session_count(void) {
    return (int)g_ActiveJetSessions;
}

JET_API void CALLBACK RunJetTest(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow) {
    (void)hwnd;
    (void)hinst;
    (void)nCmdShow;

    const char* dbName = (lpszCmdLine && lpszCmdLine[0] != '\0') ? lpszCmdLine : "dc2jet_test.accdb";
    JetSessionHandle sid = jet_open_session("rundll_test", NULL, dbName, NULL);
    if (sid) {
        JetContext* ctx = jet_get_session(sid);
        if (ctx) {
            jet_execute_action(ctx, "INSERT INTO SystemNodes (NodeVersion, Operator, Details) VALUES ('1.0', 'RunJetTest', 'Verified via Tracker.dll')");
        }
        jet_close_session(sid);
    }
}
