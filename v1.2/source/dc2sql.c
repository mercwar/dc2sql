#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifndef BUILDING_JET_DLL
#define BUILDING_JET_DLL
#endif
#include "dc2sql.h"
#include "dc2sql_diag.h"

#define TRACKER_DLL_1 "Tracker.dll"
#define TRACKER_DLL_2 "dc_tracker.dll"
#define TRACKER_DLL_3 "dc_track.dll"

enum {
    FG_NS_USER   = 0,
    FG_NS_GDI    = 1,
    FG_NS_KERNEL = 2,
    FG_NS_MODULE = 3,
    FG_NS_REG    = 4,
    FG_NS_ID64   = 5
};

typedef int    (*PFN_InitTrackerEngine)(void);
typedef void   (*PFN_ShutdownTrackerEngine)(void);
typedef UINT64 (*PFN_FG_LetEx)(int ns, UINT64 id, SIZE_T size);
typedef VOID*  (*PFN_FG_GetEx)(int ns, UINT64 id);
typedef VOID   (*PFN_FG_SetEx)(int ns, UINT64 id, VOID* src, SIZE_T size);

typedef struct {
    HMODULE                   hDll;
    PFN_InitTrackerEngine     InitEngine;
    PFN_ShutdownTrackerEngine ShutdownEngine;
    PFN_FG_LetEx              LetEx;
    PFN_FG_GetEx              GetEx;
    PFN_FG_SetEx              SetEx;
} TrackerDynApi;

static TrackerDynApi   g_Tracker         = {0};
static volatile LONG64 g_NextSqlHandleId = 0x5000;

typedef struct {
    JetSessionHandle jet_session;
    char             session_id[64];
    char             db_path[MAX_PATH];
    DWORD            last_active_tick;
    BOOL             is_connected;
} DC2SQL_SESSION;

typedef struct {
    char*  buf;
    size_t cap;
    size_t len;
    int    rows;
} RowOutBuf;

static bool __stdcall AppendRowToBuf(int col_count, const char** col_names, const char** col_values, void* user_data) {
    RowOutBuf* out = (RowOutBuf*)user_data;
    if (!out || !out->buf || out->len + 4 >= out->cap) return false;

    if (out->rows == 0) {
        for (int i = 0; i < col_count; i++) {
            int n = snprintf(out->buf + out->len, out->cap - out->len, "%s%s",
                             i ? "\t" : "", col_names[i] ? col_names[i] : "");
            if (n > 0) out->len += (size_t)n;
            if (out->len + 4 >= out->cap) return false;
        }
        int nl = snprintf(out->buf + out->len, out->cap - out->len, "\r\n");
        if (nl > 0) out->len += (size_t)nl;
    }

    for (int i = 0; i < col_count; i++) {
        int n = snprintf(out->buf + out->len, out->cap - out->len, "%s%s",
                         i ? "\t" : "", col_values[i] ? col_values[i] : "NULL");
        if (n > 0) out->len += (size_t)n;
        if (out->len + 4 >= out->cap) return false;
    }
    int nl = snprintf(out->buf + out->len, out->cap - out->len, "\r\n");
    if (nl > 0) out->len += (size_t)nl;

    out->rows++;
    return true;
}

DC2SQL_API SQLRETURN __stdcall DC2SQL_InitializeEngine(void) {
    if (g_Tracker.hDll) return SQL_SUCCESS;

    const char* dllNames[3] = { TRACKER_DLL_1, TRACKER_DLL_2, TRACKER_DLL_3 };
    char dir[MAX_PATH] = {0};
    HMODULE hSelf = NULL;

    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&DC2SQL_InitializeEngine,
        &hSelf
    );

    if (hSelf && GetModuleFileNameA(hSelf, dir, MAX_PATH) > 0) {
        char* slash = strrchr(dir, '\\');
        if (slash) {
            *(slash + 1) = '\0';
            for (int i = 0; i < 3 && !g_Tracker.hDll; i++) {
                char candidate[MAX_PATH] = {0};
                snprintf(candidate, sizeof(candidate), "%s%s", dir, dllNames[i]);
                g_Tracker.hDll = LoadLibraryA(candidate);
            }
        }
    }

    for (int i = 0; i < 3 && !g_Tracker.hDll; i++) {
        g_Tracker.hDll = LoadLibraryA(dllNames[i]);
    }

    if (!g_Tracker.hDll) {
        diag_log_event("DC2SQL", "CRITICAL: Failed to load Tracker.dll.");
        return SQL_ERROR;
    }

    g_Tracker.InitEngine     = (PFN_InitTrackerEngine)    GetProcAddress(g_Tracker.hDll, "InitTrackerEngine");
    g_Tracker.ShutdownEngine = (PFN_ShutdownTrackerEngine)GetProcAddress(g_Tracker.hDll, "ShutdownTrackerEngine");
    g_Tracker.LetEx          = (PFN_FG_LetEx)             GetProcAddress(g_Tracker.hDll, "_FG_LetEx");
    g_Tracker.GetEx          = (PFN_FG_GetEx)             GetProcAddress(g_Tracker.hDll, "_FG_GetEx");
    g_Tracker.SetEx          = (PFN_FG_SetEx)             GetProcAddress(g_Tracker.hDll, "_FG_SetEx");

    if (!g_Tracker.LetEx || !g_Tracker.GetEx || !g_Tracker.SetEx) {
        diag_log_event("DC2SQL", "CRITICAL: Missing _FG_LetEx / _FG_GetEx / _FG_SetEx exports in Tracker.dll.");
        FreeLibrary(g_Tracker.hDll);
        memset(&g_Tracker, 0, sizeof(g_Tracker));
        return SQL_ERROR;
    }

    if (g_Tracker.InitEngine && !g_Tracker.InitEngine()) {
        diag_log_event("DC2SQL", "CRITICAL: InitTrackerEngine() failed.");
        FreeLibrary(g_Tracker.hDll);
        memset(&g_Tracker, 0, sizeof(g_Tracker));
        return SQL_ERROR;
    }

    diag_log_event("DC2SQL", "Engine initialized and bound to Tracker.dll.");
    return SQL_SUCCESS;
}

DC2SQL_API FG_Handle __stdcall DC2SQL_OpenSessionEx(const char* sessionId, const char* baseDir, const char* dbPath, const char* passkey) {
    if (!dbPath || dbPath[0] == '\0') return 0;
    if (!g_Tracker.hDll && DC2SQL_InitializeEngine() != SQL_SUCCESS) return 0;

    const char* name = (sessionId && sessionId[0] != '\0') ? sessionId : "default_session";

    JetSessionHandle jetSess = jet_open_session(name, baseDir, dbPath, passkey);
    if (!jetSess) {
        diag_log_event("DC2SQL", "Failed to open underlying Jet session.");
        return 0;
    }

    UINT64 id = (UINT64)InterlockedIncrement64(&g_NextSqlHandleId);
    if (g_Tracker.LetEx(FG_NS_ID64, id, sizeof(DC2SQL_SESSION)) == 0) {
        jet_close_session(jetSess);
        return 0;
    }

    DC2SQL_SESSION sess;
    memset(&sess, 0, sizeof(sess));
    sess.jet_session      = jetSess;
    sess.last_active_tick = GetTickCount();
    sess.is_connected     = TRUE;
    strncpy_s(sess.session_id, sizeof(sess.session_id), name, _TRUNCATE);
    strncpy_s(sess.db_path, sizeof(sess.db_path), dbPath, _TRUNCATE);

    g_Tracker.SetEx(FG_NS_ID64, id, &sess, sizeof(DC2SQL_SESSION));
    diag_log_event("DC2SQL", "Session opened and registered in Tracker.dll.");
    return (FG_Handle)id;
}

DC2SQL_API FG_Handle __stdcall DC2SQL_OpenSession(const char* sessionId, const char* dbPath) {
    return DC2SQL_OpenSessionEx(sessionId, NULL, dbPath, NULL);
}

DC2SQL_API SQLRETURN __stdcall DC2SQL_ExecuteQuery(FG_Handle handle, const char* sqlQuery, char* outputBuffer, size_t bufferSize) {
    if (!handle || !sqlQuery || !g_Tracker.GetEx) return SQL_INVALID_HANDLE;

    DC2SQL_SESSION* sess = (DC2SQL_SESSION*)g_Tracker.GetEx(FG_NS_ID64, (UINT64)handle);
    if (!sess || !sess->is_connected || !sess->jet_session) return SQL_ERROR;

    JetContext* ctx = jet_get_session(sess->jet_session);
    if (!ctx) return SQL_ERROR;

    const char* q = sqlQuery;
    while (*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n') q++;

    if (_strnicmp(q, "SELECT", 6) == 0 && (q[6] == ' ' || q[6] == '\t' || q[6] == '\r' || q[6] == '\n' || q[6] == '*')) {
        if (outputBuffer && bufferSize > 0) outputBuffer[0] = '\0';
        RowOutBuf out = { outputBuffer, bufferSize, 0, 0 };
        if (!jet_execute_select(ctx, q, AppendRowToBuf, &out)) return SQL_ERROR;
    } else {
        if (!jet_execute_action(ctx, q)) return SQL_ERROR;
        if (outputBuffer && bufferSize > 0) {
            snprintf(outputBuffer, bufferSize, "OK");
        }
    }

    sess->last_active_tick = GetTickCount();
    return SQL_SUCCESS;
}

DC2SQL_API void __stdcall DC2SQL_CloseSession(FG_Handle handle) {
    if (!handle || !g_Tracker.GetEx || !g_Tracker.LetEx) return;

    DC2SQL_SESSION* sess = (DC2SQL_SESSION*)g_Tracker.GetEx(FG_NS_ID64, (UINT64)handle);
    if (!sess) return;

    if (sess->jet_session) {
        jet_close_session(sess->jet_session);
        sess->jet_session = 0;
    }
    sess->is_connected = FALSE;

    g_Tracker.LetEx(FG_NS_ID64, (UINT64)handle, 0);
    diag_log_event("DC2SQL", "Session closed and removed from Tracker.dll.");
}

DC2SQL_API int __stdcall DC2SQL_GetActiveSessionCount(void) {
    return jet_get_active_session_count();
}

DC2SQL_API bool __stdcall DC2SQL_WaitForSessions(DWORD pollIntervalMs, DWORD timeoutMs) {
    DWORD step = (pollIntervalMs > 0) ? pollIntervalMs : 50;
    DWORD startTick = GetTickCount();

    while (jet_get_active_session_count() > 0) {
        if (timeoutMs != INFINITE && (GetTickCount() - startTick) >= timeoutMs) {
            return false;
        }
        Sleep(step);
    }
    return true;
}

DC2SQL_API void __stdcall DC2SQL_ShutdownEngine(void) {
    diag_log_event("DC2SQL", "Shutting down DC2SQL engine...");

    if (g_Tracker.hDll) {
        if (g_Tracker.ShutdownEngine) {
            g_Tracker.ShutdownEngine();
        }
        FreeLibrary(g_Tracker.hDll);
        memset(&g_Tracker, 0, sizeof(g_Tracker));
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    (void)hinstDLL;
    (void)lpvReserved;
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            if (lpvReserved == NULL) {
                DC2SQL_ShutdownEngine();
            }
            break;
    }
    return TRUE;
}
