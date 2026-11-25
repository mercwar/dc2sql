#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "dc2jet_fmt.h"

typedef JetContext*      (__stdcall *PFN_jet_initialize)(const char*, const char*, const char*);
typedef bool             (__stdcall *PFN_jet_execute_action)(JetContext*, const char*);
typedef bool             (__stdcall *PFN_jet_execute_select)(JetContext*, const char*, JetRowCallback, void*);
typedef void             (__stdcall *PFN_jet_shutdown)(JetContext*);

typedef JetSessionHandle (__stdcall *PFN_jet_open_session)(const char*, const char*, const char*, const char*);
typedef JetContext*      (__stdcall *PFN_jet_get_session)(JetSessionHandle);
typedef bool             (__stdcall *PFN_jet_close_session)(JetSessionHandle);
typedef int              (__stdcall *PFN_jet_get_active_session_count)(void);

typedef void             (__stdcall *PFN_diag_set_enabled)(bool);
typedef bool             (__stdcall *PFN_diag_is_enabled)(void);
typedef int              (__stdcall *PFN_diag_get_buffered_count)(void);
typedef bool             (__stdcall *PFN_diag_flush_reports)(char*, size_t);
typedef void             (__stdcall *PFN_diag_clear_buffer)(void);

typedef struct {
    HMODULE                          hDll;
    PFN_jet_initialize               jet_initialize;
    PFN_jet_execute_action           jet_execute_action;
    PFN_jet_execute_select           jet_execute_select;
    PFN_jet_shutdown                 jet_shutdown;
    PFN_jet_open_session             jet_open_session;
    PFN_jet_get_session              jet_get_session;
    PFN_jet_close_session            jet_close_session;
    PFN_jet_get_active_session_count jet_get_active_session_count;
    PFN_diag_set_enabled             diag_set_enabled;
    PFN_diag_is_enabled              diag_is_enabled;
    PFN_diag_get_buffered_count      diag_get_buffered_count;
    PFN_diag_flush_reports           diag_flush_reports;
    PFN_diag_clear_buffer            diag_clear_buffer;
} SqlDllApi;

static volatile BOOL g_KeepSessionAlive = TRUE;

static BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType) {
    if (ctrlType == CTRL_C_EVENT || ctrlType == CTRL_BREAK_EVENT || ctrlType == CTRL_CLOSE_EVENT) {
        g_KeepSessionAlive = FALSE;
        return TRUE;
    }
    return FALSE;
}

static bool __stdcall DefaultRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data) {
    int* rowNum = (int*)user_data;
    printf("[Row %d]\n", (*rowNum)++);
    for (int i = 0; i < col_count; i++) {
        printf("  %-18s = %s\n", col_names[i], col_values[i] ? col_values[i] : "NULL");
    }
    printf("\n");
    return true;
}

static HMODULE LoadLocalDll(const char* explicitPath) {
    if (explicitPath && explicitPath[0] != '\0') {
        return LoadLibraryA(explicitPath);
    }

    const char* candidates[2] = { "dc2sql.dll", "dc2jet.dll" };
    char exeDir[MAX_PATH] = {0};

    if (GetModuleFileNameA(NULL, exeDir, MAX_PATH) > 0) {
        char* slash = strrchr(exeDir, '\\');
        if (slash) {
            *(slash + 1) = '\0';
            for (int i = 0; i < 2; i++) {
                char full[MAX_PATH] = {0};
                snprintf(full, sizeof(full), "%s%s", exeDir, candidates[i]);
                HMODULE h = LoadLibraryA(full);
                if (h) return h;
            }
        }
    }

    for (int i = 0; i < 2; i++) {
        HMODULE h = LoadLibraryA(candidates[i]);
        if (h) return h;
    }
    return NULL;
}

static bool BindSqlApi(SqlDllApi* api, const char* explicitDll) {
    memset(api, 0, sizeof(SqlDllApi));
    api->hDll = LoadLocalDll(explicitDll);
    if (!api->hDll) return false;

    api->jet_initialize               = (PFN_jet_initialize)              GetProcAddress(api->hDll, "jet_initialize");
    api->jet_execute_action           = (PFN_jet_execute_action)          GetProcAddress(api->hDll, "jet_execute_action");
    api->jet_execute_select           = (PFN_jet_execute_select)          GetProcAddress(api->hDll, "jet_execute_select");
    api->jet_shutdown                 = (PFN_jet_shutdown)                GetProcAddress(api->hDll, "jet_shutdown");

    api->jet_open_session             = (PFN_jet_open_session)            GetProcAddress(api->hDll, "jet_open_session");
    api->jet_get_session              = (PFN_jet_get_session)             GetProcAddress(api->hDll, "jet_get_session");
    api->jet_close_session            = (PFN_jet_close_session)           GetProcAddress(api->hDll, "jet_close_session");
    api->jet_get_active_session_count = (PFN_jet_get_active_session_count)GetProcAddress(api->hDll, "jet_get_active_session_count");

    api->diag_set_enabled             = (PFN_diag_set_enabled)            GetProcAddress(api->hDll, "diag_set_enabled");
    api->diag_is_enabled              = (PFN_diag_is_enabled)             GetProcAddress(api->hDll, "diag_is_enabled");
    api->diag_get_buffered_count      = (PFN_diag_get_buffered_count)     GetProcAddress(api->hDll, "diag_get_buffered_count");
    api->diag_flush_reports           = (PFN_diag_flush_reports)          GetProcAddress(api->hDll, "diag_flush_reports");
    api->diag_clear_buffer            = (PFN_diag_clear_buffer)           GetProcAddress(api->hDll, "diag_clear_buffer");

    return (api->jet_initialize && api->jet_execute_action && api->jet_execute_select && api->jet_shutdown);
}

static bool IsSelectQuery(const char* sql) {
    if (!sql) return false;
    while (*sql == ' ' || *sql == '\t' || *sql == '\r' || *sql == '\n') sql++;
    return (_strnicmp(sql, "SELECT", 6) == 0 &&
           (sql[6] == ' ' || sql[6] == '\t' || sql[6] == '\r' || sql[6] == '\n' || sql[6] == '*'));
}

static void FlushDiagnostics(const SqlDllApi* api, bool enabled) {
    if (!enabled || !api || !api->diag_flush_reports) return;
    if (api->diag_get_buffered_count && api->diag_get_buffered_count() == 0) return;

    char diagBuf[8192] = {0};
    if (api->diag_flush_reports(diagBuf, sizeof(diagBuf))) {
        printf("%s", diagBuf);
        fflush(stdout);
    }
}

static int RunSingleStatement(const SqlDllApi* api, JetContext* ctx, const char* sqlQuery, const char* langStyle, bool hasLang, bool enableAck) {
    const char* q = sqlQuery;
    while (*q == ' ' || *q == '\t' || *q == '\r' || *q == '\n') q++;
    if (*q == '\0') return 0;

    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);

    LangContext langCtx = {0};
    int defaultCounter = 1;
    JetRowCallback activeCb = DefaultRowCallback;
    void* cbUserData = &defaultCounter;

    if (hasLang) {
        JetRowCallback parsedCb = GetLangFormatterCallback(langStyle[0] != '\0' ? langStyle : "json", enableAck, &langCtx);
        if (parsedCb) {
            activeCb   = parsedCb;
            cbUserData = &langCtx;
        }
    }

    bool wrapJsonArray = (langCtx.lang_type == 1 || langCtx.lang_type == 2 || langCtx.lang_type == 7) && !enableAck;
    bool ok = false;

    if (IsSelectQuery(q)) {
        if (enableAck) {
            PrintHeaderAcknowledgment(langCtx.lang_type, true, 0.0);
        } else if (wrapJsonArray) {
            printf("[\n");
        }

        ok = api->jet_execute_select(ctx, q, activeCb, cbUserData);

        if (enableAck) {
            PrintFooterAcknowledgment(langCtx.lang_type, true);
        } else if (wrapJsonArray) {
            printf("\n]\n");
        }
    } else {
        ok = api->jet_execute_action(ctx, q);
        QueryPerformanceCounter(&t1);
        double elapsed = (double)(t1.QuadPart - t0.QuadPart) / (double)freq.QuadPart;

        if (ok) {
            if (enableAck) {
                PrintHeaderAcknowledgment(langCtx.lang_type, true, elapsed);
                PrintFooterAcknowledgment(langCtx.lang_type, true);
            } else {
                printf("[OK] Query executed successfully (%.4f sec).\n", elapsed);
            }
        }
    }

    fflush(stdout);
    return ok ? 0 : 1;
}

static void TrimLine(char* line) {
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n' || line[len - 1] == ' ' || line[len - 1] == '\t')) {
        line[--len] = '\0';
    }
}

static void PrintUsage(void) {
    printf("Usage: dc2sql.exe [options]\n");
    printf("  --db <file>        Database file path (.accdb or .mdb)\n");
    printf("  --dir <folder>     Base directory for database file\n");
    printf("  --pass <passkey>   Database password\n");
    printf("  --sql <query>      SQL statement to execute (CREATE, INSERT, SELECT, etc.)\n");
    printf("  --session <name>   Named session ID (optional)\n");
    printf("  --dll <path>       Explicit path to dc2sql.dll / dc2jet.dll\n");
    printf("  --lang <format>    Output format (json, csv, xml, etc.)\n");
    printf("  --ack              Enable structured acknowledgment headers/footers\n");
    printf("  --diag             Print internal DC2SQL/Jet diagnostic telemetry\n");
    printf("  -track             Keep session open & tracked in Tracker.dll until Ctrl+C\n");
    printf("  --hold             Alias for -track\n");
    printf("  --help             Show this help message\n");
}
int main(int argc, char** argv) {
    const char* dbFile      = NULL;
    const char* baseDir     = NULL;
    const char* passkey     = NULL;
    const char* sqlQuery    = NULL;
    const char* sessionName = NULL;
    const char* explicitDll = NULL;
    const char* langStyle   = "json";
    bool        hasLang     = false;
    bool        enableAck   = false;
    bool        enableDiag  = false;
    bool        trackMode   = false;

    const char* positional[8] = {0};
    int posCount = 0;

    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (!arg || arg[0] == '\0') continue;

        const char* flag = arg;
        if (flag[0] == '/' || flag[0] == '-') flag++;
        if (flag[0] == '-') flag++;

        if ((arg[0] == '-' || arg[0] == '/') &&
            (_stricmp(flag, "db") == 0 || _stricmp(flag, "d") == 0 || _stricmp(flag, "file") == 0 || _stricmp(flag, "database") == 0) && i + 1 < argc) {
            dbFile = argv[++i];
        } else if ((arg[0] == '-' || arg[0] == '/') &&
            (_stricmp(flag, "dir") == 0 || _stricmp(flag, "b") == 0 || _stricmp(flag, "base") == 0 || _stricmp(flag, "basedir") == 0) && i + 1 < argc) {
            baseDir = argv[++i];
        } else if ((arg[0] == '-' || arg[0] == '/') &&
            (_stricmp(flag, "pass") == 0 || _stricmp(flag, "p") == 0 || _stricmp(flag, "pwd") == 0 || _stricmp(flag, "passkey") == 0) && i + 1 < argc) {
            passkey = argv[++i];
        } else if ((arg[0] == '-' || arg[0] == '/') &&
            (_stricmp(flag, "sql") == 0 || _stricmp(flag, "q") == 0 || _stricmp(flag, "query") == 0 || _stricmp(flag, "cmd") == 0 || _stricmp(flag, "exec") == 0) && i + 1 < argc) {
            sqlQuery = argv[++i];
        } else if ((arg[0] == '-' || arg[0] == '/') &&
            (_stricmp(flag, "session") == 0 || _stricmp(flag, "s") == 0) && i + 1 < argc) {
            sessionName = argv[++i];
        } else if ((arg[0] == '-' || arg[0] == '/') &&
            _stricmp(flag, "dll") == 0 && i + 1 < argc) {
            explicitDll = argv[++i];
        } else if ((arg[0] == '-' || arg[0] == '/') &&
            (_stricmp(flag, "lang") == 0 || _stricmp(flag, "l") == 0 || _stricmp(flag, "fmt") == 0 || _stricmp(flag, "format") == 0) && i + 1 < argc) {
            langStyle = argv[++i];
            hasLang = true;
        } else if ((arg[0] == '-' || arg[0] == '/') && _stricmp(flag, "ack") == 0) {
            enableAck = true;
        } else if ((arg[0] == '-' || arg[0] == '/') && (_stricmp(flag, "diag") == 0 || _stricmp(flag, "verbose") == 0 || _stricmp(flag, "v") == 0)) {
            enableDiag = true;
        } else if ((arg[0] == '-' || arg[0] == '/') && (_stricmp(flag, "track") == 0 || _stricmp(flag, "hold") == 0)) {
            trackMode = true;
        } else if ((arg[0] == '-' || arg[0] == '/') && (_stricmp(flag, "help") == 0 || _stricmp(flag, "h") == 0 || _stricmp(flag, "?") == 0)) {
            PrintUsage();
            return 0;
        } else if (posCount < 8) {
            positional[posCount++] = arg;
        }
    }

    if (!dbFile && posCount >= 1)   dbFile   = positional[0];
    if (!sqlQuery && posCount >= 2) sqlQuery = positional[1];
    if (!hasLang && posCount >= 3) {
        langStyle = positional[2];
        hasLang = true;
    }

    if (!dbFile || dbFile[0] == '\0') {
        fprintf(stderr, "[ERROR] Missing required database file parameter.\n");
        PrintUsage();
        return 1;
    }

    SqlDllApi api;
    if (!BindSqlApi(&api, explicitDll)) {
        fprintf(stderr, "[ERROR] Failed to load or bind dc2sql.dll / dc2jet.dll.\n");
        return 2;
    }

    if (api.diag_set_enabled) {
        api.diag_set_enabled(enableDiag);
    }

    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    int exitCode = 0;

    if ((trackMode || sessionName) && api.jet_open_session && api.jet_get_session && api.jet_close_session) {
        const char* activeName = (sessionName && sessionName[0] != '\0') ? sessionName : "tracked_session";

        JetSessionHandle sid = api.jet_open_session(activeName, baseDir, dbFile, passkey);
        if (!sid) {
            fprintf(stderr, "[ERROR] Workspace session login failed for database: %s\n", dbFile);
            FlushDiagnostics(&api, enableDiag);
            FreeLibrary(api.hDll);
            return 3;
        }

        JetContext* ctx = api.jet_get_session(sid);
        if (!ctx) {
            fprintf(stderr, "[ERROR] Failed to retrieve tracked JetContext for session: %s\n", activeName);
            api.jet_close_session(sid);
            FlushDiagnostics(&api, enableDiag);
            FreeLibrary(api.hDll);
            return 3;
        }

        if (sqlQuery && sqlQuery[0] != '\0') {
            exitCode = RunSingleStatement(&api, ctx, sqlQuery, langStyle, hasLang, enableAck);
        }

        if (trackMode) {
            int activeCount = api.jet_get_active_session_count ? api.jet_get_active_session_count() : 1;
            printf("[TRACK] Session '%s' (Handle: 0x%llX) active in Tracker.dll (%d active). Press Ctrl+C to close...\n",
                   activeName,
                   (unsigned long long)sid,
                   activeCount);
            fflush(stdout);
            FlushDiagnostics(&api, enableDiag);

            while (g_KeepSessionAlive) {
                Sleep(100);
            }

            printf("\n[TRACK] Releasing tracked session '%s'...\n", activeName);
        }

        api.jet_close_session(sid);
    } else {
        JetContext* ctx = api.jet_initialize(baseDir, dbFile, passkey);
        if (!ctx) {
            fprintf(stderr, "[ERROR] Workspace session login failed for database: %s\n", dbFile);
            FlushDiagnostics(&api, enableDiag);
            FreeLibrary(api.hDll);
            return 3;
        }

        if (sqlQuery && sqlQuery[0] != '\0') {
            exitCode = RunSingleStatement(&api, ctx, sqlQuery, langStyle, hasLang, enableAck);
        }

        api.jet_shutdown(ctx);
    }

    FlushDiagnostics(&api, enableDiag);
    FreeLibrary(api.hDll);
    return exitCode;
}
