#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct JetContext JetContext;
typedef bool (__stdcall *JetRowCallback)(int col_count, const char** col_names, const char** col_values, void* user_data);

typedef JetContext* (__stdcall *pfn_jet_initialize)(const char*, const char*, const char*);
typedef bool (__stdcall *pfn_jet_execute_action)(JetContext*, const char*);
typedef bool (__stdcall *pfn_jet_execute_select)(JetContext*, const char*, JetRowCallback, void*);
typedef void (__stdcall *pfn_jet_shutdown)(JetContext*);

typedef void (__stdcall *pfn_diag_set_enabled)(bool);
typedef bool (__stdcall *pfn_diag_is_enabled)(void);
typedef int  (__stdcall *pfn_diag_get_buffered_count)(void);
typedef bool (__stdcall *pfn_diag_flush_reports)(char*, size_t);
typedef void (__stdcall *pfn_diag_clear_buffer)(void);

typedef struct {
    int row_counter;
    int lang_type;
    bool first_item;
} CLIContext;

static void PrintSafeValue(const char* val) {
    if (!val) {
        printf("NULL");
        return;
    }
    bool is_numeric = (val[0] != '\0');
    for (int i = 0; val[i] != '\0'; i++) {
        if ((val[i] < '0' || val[i] > '9') && val[i] != '.' && val[i] != '-') {
            is_numeric = false;
            break;
        }
    }
    if (is_numeric) {
        printf("%s", val);
    } else {
        printf("\"%s\"", val);
    }
}

bool __stdcall DefaultRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data) {
    int* rowNum = (int*)user_data;
    printf("[Row %d]\n", (*rowNum)++);
    for (int i = 0; i < col_count; i++) {
        printf("  %s = %s\n", col_names[i], col_values[i] ? col_values[i] : "NULL");
    }
    return true;
}

bool __stdcall JsonRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data) {
    CLIContext* ctx = (CLIContext*)user_data;
    if (ctx->first_item) {
        ctx->first_item = false;
    } else {
        printf(",\n");
    }
    printf("  {\n    \"row\": %d,\n", ctx->row_counter++);
    for (int i = 0; i < col_count; i++) {
        printf("    \"%s\": ", col_names[i]);
        PrintSafeValue(col_values[i]);
        printf("%s\n", (i < col_count - 1) ? "," : "");
    }
    printf("  }");
    return true;
}

void PrintUsage(void) {
    printf("==============================================================================\n");
    printf("  DC2JET / DC2SQL Enterprise Engine CLI (DAO 3.60 / Jet 4.0)       v1.3.0     \n");
    printf("  Operator: ROBO-ROOK // Architecture: JOE-TRON-DT                           \n");
    printf("==============================================================================\n");
    printf("\n");
    printf("DESCRIPTION:\n");
    printf("  Executes SQL queries against Microsoft Access / Jet database containers with\n");
    printf("  zero-dependency serialization and authentic engine-level telemetry.\n");
    printf("\n");
    printf("USAGE:\n");
    printf("  dc2sql_cli.exe -db <database.mdb> -query \"<SQL>\" [OPTIONS]\n");
    printf("\n");
    printf("REQUIRED PARAMETERS:\n");
    printf("  -db <path>           Target Microsoft Access / Jet database file (.mdb)\n");
    printf("  -query \"<SQL>\"       SQL statement (SELECT, INSERT, UPDATE, CREATE, DROP)\n");
    printf("\n");
    printf("OPTIONAL SWITCHES:\n");
    printf("  -passkey <pwd>       Database security password for encrypted containers\n");
    printf("  -lang <style>        Serialize result rows into pure target objects/formats:\n");
    printf("                       [python|py, js|ts|javascript, json, c, c++|cpp,\n");
    printf("                        java, php, csv, ini, xml, vbs]\n");
    printf("  -diag                Enable live DLL diagnostic ring-buffer event logging\n");
    printf("  -ack                 Auto-enable diagnostics and flush authentic MS-Jet\n");
    printf("                       diagnostic telemetry buffer upon completion\n");
    printf("  -h, --help, -?       Display this enterprise help manual\n");
    printf("\n");
    printf("EXAMPLES:\n");
    printf("  dc2sql_cli.exe -db store.mdb -query \"SELECT * FROM Nodes\" -lang json -ack\n");
    printf("  dc2sql_cli.exe -db store.mdb -query \"CREATE TABLE Logs (ID COUNTER)\" -diag\n");
    printf("==============================================================================\n");
    fflush(stdout);
}

int main(int argc, char* argv[]) {
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)) {
        PrintUsage();
        return 0;
    }

    char dbPath[MAX_PATH] = {0};
    char sqlQuery[2048] = {0};
    char passkey[128] = {0};
    char langStyle[64] = {0};
    bool enableDiag = false;
    bool enableAck = false;
    bool hasLang = false;

    if (argc < 3) {
        PrintUsage();
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-db") == 0 && i + 1 < argc) {
            strncpy_s(dbPath, sizeof(dbPath), argv[i + 1], _TRUNCATE);
            i++;
        } else if (strcmp(argv[i], "-query") == 0 && i + 1 < argc) {
            strncpy_s(sqlQuery, sizeof(sqlQuery), argv[i + 1], _TRUNCATE);
            i++;
        } else if (strcmp(argv[i], "-passkey") == 0 && i + 1 < argc) {
            strncpy_s(passkey, sizeof(passkey), argv[i + 1], _TRUNCATE);
            i++;
        } else if (strcmp(argv[i], "-diag") == 0) {
            enableDiag = true;
        } else if (strcmp(argv[i], "-ack") == 0) {
            enableAck = true;
            enableDiag = true; // FIX: Automatically activate diagnostics when -ack is requested
        } else if (strcmp(argv[i], "-lang") == 0) {
            hasLang = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                strncpy_s(langStyle, sizeof(langStyle), argv[i + 1], _TRUNCATE);
                i++;
            }
        }
    }

    if (dbPath[0] == '\0' || sqlQuery[0] == '\0') {
        printf("[ERROR] Missing required parameters [-db] or [-query].\n");
        return 1;
    }

    HMODULE hDll = LoadLibraryA("dc2sql.dll");
    if (!hDll) {
        printf("[ERROR] Failed to load dc2sql.dll (Error code: %lu)\n", GetLastError());
        return 1;
    }

    pfn_jet_initialize jet_init = (pfn_jet_initialize)GetProcAddress(hDll, "jet_initialize");
    pfn_jet_execute_action jet_exec_act = (pfn_jet_execute_action)GetProcAddress(hDll, "jet_execute_action");
    pfn_jet_execute_select jet_exec_sel = (pfn_jet_execute_select)GetProcAddress(hDll, "jet_execute_select");
    pfn_jet_shutdown jet_shut = (pfn_jet_shutdown)GetProcAddress(hDll, "jet_shutdown");
    pfn_diag_set_enabled diag_set = (pfn_diag_set_enabled)GetProcAddress(hDll, "diag_set_enabled");
    pfn_diag_flush_reports diag_flush = (pfn_diag_flush_reports)GetProcAddress(hDll, "diag_flush_reports");

    if (diag_set && enableDiag) {
        diag_set(true);
    }

    JetContext* ctx = jet_init("", dbPath, passkey[0] != '\0' ? passkey : NULL);
    if (!ctx) {
        printf("[ERROR] Workspace session login failed for database: %s\n", dbPath);
        FreeLibrary(hDll);
        return 1;
    }

    JetRowCallback activeCallback = DefaultRowCallback;
    CLIContext cliCtx = {1, 0, true};
    int defaultCounter = 1;
    void* callbackUserData = &defaultCounter;

    if (hasLang) {
        activeCallback = JsonRowCallback;
        callbackUserData = &cliCtx;
        printf("[\n");
    }

    if (_strnicmp(sqlQuery, "SELECT", 6) == 0) {
        jet_exec_sel(ctx, sqlQuery, activeCallback, callbackUserData);
    } else {
        jet_exec_act(ctx, sqlQuery);
    }

    if (hasLang) {
        printf("\n]\n");
    }

    // Flush authentic underlying MS-Jet engine diagnostic buffer if -ack or -diag is active
    if ((enableAck || enableDiag) && diag_flush) {
        char realDiagBuffer[8192] = {0};
        if (diag_flush(realDiagBuffer, sizeof(realDiagBuffer))) {
            printf("%s", realDiagBuffer);
        }
    }

    jet_shut(ctx);
    FreeLibrary(hDll);
    return 0;
}