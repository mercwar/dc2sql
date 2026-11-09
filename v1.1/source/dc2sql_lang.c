#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef bool (__stdcall *JetRowCallback)(int col_count, const char** col_names, const char** col_values, void* user_data);

typedef struct {
    int row_counter;
    int lang_type;       // 0=Text, 1=Python, 2=JS/TS, 3=C, 4=C++, 5=Java, 6=PHP, 7=JSON, 8=CSV, 9=INI, 10=XML, 11=VBS
    bool enable_ack;     // -ack switch toggle
    double duration_sec; // Query performance metric
    bool first_item;
} LangContext;

// Safe value stringifier with type detection
static void PrintSafeValue(const char* val, int lang) {
    if (!val) {
        if (lang == 5) printf("null"); // Java
        else printf("NULL");
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

// ============================================================================
// OFFICIAL MS-JET ACKNOWLEDGMENT HEADER / FOOTER GENERATOR
// ============================================================================
void PrintHeaderAcknowledgment(int lang_type, bool enable_ack, double elapsed_sec) {
    if (!enable_ack) return;

    char timeStr[64];
    sprintf_s(timeStr, sizeof(timeStr), "query finished in %.4f seconds", elapsed_sec);

    switch (lang_type) {
        case 1: // Python
            printf("    {\n");
            printf("        \"msjet\": {\n");
            printf("            \"status\": \"ok\",\n");
            printf("            \"task_result\": \"success\",\n");
            printf("            \"telemetry\": \"%s\"\n", timeStr);
            printf("        },\n");
            printf("        \"data\": [\n");
            break;
        case 2: // JavaScript / JSON (7)
        case 7:
            printf("  {\n");
            printf("    \"msjet\": {\n");
            printf("      \"status\": \"ok\",\n");
            printf("      \"task_result\": \"success\",\n");
            printf("      \"telemetry\": \"%s\"\n", timeStr);
            printf("    },\n");
            printf("    \"data\": [\n");
            break;
        case 9: // INI
            printf("[msjet]\nstatus=ok\ntask_result=success\ntelemetry=%s\n\n", timeStr);
            break;
        case 10: // XML
            printf("<msjet_response>\n");
            printf("  <msjet>\n");
            printf("    <status>ok</status>\n");
            printf("    <task_result>success</task_result>\n");
            printf("    <telemetry>%s</telemetry>\n", timeStr);
            printf("  </msjet>\n");
            printf("  <data>\n");
            break;
        case 11: // VBScript
            printf("' {msjet:status:ok, task_results:success, %s}\n\n", timeStr);
            break;
        default:
            printf("# {msjet:status:ok, task_results:success, %s}\n\n", timeStr);
            break;
    }
}

void PrintFooterAcknowledgment(int lang_type, bool enable_ack) {
    if (!enable_ack) return;
    if (lang_type == 1 || lang_type == 2 || lang_type == 7) {
        printf("\n    ]\n  }");
    } else if (lang_type == 10) {
        printf("  </data>\n</msjet_response>");
    }
}

// ============================================================================
// SERIALIZATION ROW CALLBACKS
// ============================================================================
bool __stdcall PythonRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data) {
    LangContext* ctx = (LangContext*)user_data;
    if (ctx->first_item) { ctx->first_item = false; } else { printf(",\n"); }
    printf("        {\n            \"row\": %d,\n", ctx->row_counter++);
    for (int i = 0; i < col_count; i++) {
        printf("            \"%s\": ", col_names[i]);
        PrintSafeValue(col_values[i], 1);
        printf("%s\n", (i < col_count - 1) ? "," : "");
    }
    printf("        }");
    return true;
}

bool __stdcall JavascriptRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data) {
    LangContext* ctx = (LangContext*)user_data;
    if (ctx->first_item) { ctx->first_item = false; } else { printf(",\n"); }
    printf("    {\n      \"row\": %d,\n", ctx->row_counter++);
    for (int i = 0; i < col_count; i++) {
        printf("      \"%s\": ", col_names[i]);
        PrintSafeValue(col_values[i], 2);
        printf("%s\n", (i < col_count - 1) ? "," : "");
    }
    printf("    }");
    return true;
}

bool __stdcall CStructRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data) {
    LangContext* ctx = (LangContext*)user_data;
    int rowNum = ctx ? ctx->row_counter++ : 1;
    printf("/* Row %d */\n{\n", rowNum);
    for (int i = 0; i < col_count; i++) {
        printf("    .%s = ", col_names[i]);
        PrintSafeValue(col_values[i], 3);
        printf("%s\n", (i < col_count - 1) ? "," : "");
    }
    printf("};\n\n");
    return true;
}

bool __stdcall CppObjectRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data) {
    LangContext* ctx = (LangContext*)user_data;
    int rowNum = ctx ? ctx->row_counter++ : 1;
    printf("JetRecord record_%d = {\n", rowNum);
    for (int i = 0; i < col_count; i++) {
        printf("    {\"%s\", ", col_names[i]);
        PrintSafeValue(col_values[i], 4);
        printf("}%s\n", (i < col_count - 1) ? "," : "");
    }
    printf("};\n\n");
    return true;
}

bool __stdcall JavaObjectRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data) {
    LangContext* ctx = (LangContext*)user_data;
    int rowNum = ctx ? ctx->row_counter++ : 1;
    printf("new JetRecord(%d, new HashMap<String, Object>() {{\n", rowNum);
    for (int i = 0; i < col_count; i++) {
        printf("    put(\"%s\", ", col_names[i]);
        PrintSafeValue(col_values[i], 5);
        printf(");\n");
    }
    printf("}});\n\n");
    return true;
}

bool __stdcall PhpArrayRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data) {
    LangContext* ctx = (LangContext*)user_data;
    int rowNum = ctx ? ctx->row_counter++ : 1;
    printf("[\n    'row' => %d,\n", rowNum);
    for (int i = 0; i < col_count; i++) {
        printf("    '%s' => ", col_names[i]);
        PrintSafeValue(col_values[i], 6);
        printf("%s\n", (i < col_count - 1) ? "," : "");
    }
    printf("],\n");
    return true;
}

bool __stdcall CsvRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data) {
    LangContext* ctx = (LangContext*)user_data;
    if (ctx->row_counter == 1) {
        for (int i = 0; i < col_count; i++) {
            printf("%s%s", col_names[i], (i < col_count - 1) ? "," : "\n");
        }
    }
    ctx->row_counter++;
    for (int i = 0; i < col_count; i++) {
        printf("\"%s\"%s", col_values[i] ? col_values[i] : "", (i < col_count - 1) ? "," : "\n");
    }
    return true;
}

bool __stdcall IniRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data) {
    LangContext* ctx = (LangContext*)user_data;
    int rowNum = ctx ? ctx->row_counter++ : 1;
    printf("[Row_%d]\n", rowNum);
    for (int i = 0; i < col_count; i++) {
        printf("%s=%s\n", col_names[i], col_values[i] ? col_values[i] : "");
    }
    printf("\n");
    return true;
}

bool __stdcall XmlRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data) {
    LangContext* ctx = (LangContext*)user_data;
    int rowNum = ctx ? ctx->row_counter++ : 1;
    printf("    <Record id=\"%d\">\n", rowNum);
    for (int i = 0; i < col_count; i++) {
        printf("        <%s>%s</%s>\n", col_names[i], col_values[i] ? col_values[i] : "", col_names[i]);
    }
    printf("    </Record>\n");
    return true;
}

bool __stdcall VbsRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data) {
    LangContext* ctx = (LangContext*)user_data;
    int rowNum = ctx ? ctx->row_counter++ : 1;
    printf("Set dict_%d = CreateObject(\"Scripting.Dictionary\")\n", rowNum);
    for (int i = 0; i < col_count; i++) {
        printf("dict_%d.Add \"%s\", ", rowNum, col_names[i]);
        PrintSafeValue(col_values[i], 11);
        printf("\n");
    }
    printf("\n");
    return true;
}

// ============================================================================
// FACTORY SELECTOR
// ============================================================================
JetRowCallback GetLangFormatterCallback(const char* lang_style, bool enable_ack, LangContext* out_ctx) {
    out_ctx->row_counter = 1;
    out_ctx->enable_ack = enable_ack;
    out_ctx->first_item = true;

    if (!lang_style || lang_style[0] == '\0') {
        out_ctx->lang_type = 0;
        return NULL;
    }

    if (_stricmp(lang_style, "python") == 0 || _stricmp(lang_style, "py") == 0) {
        out_ctx->lang_type = 1;
        return PythonRowCallback;
    } else if (_stricmp(lang_style, "javascript") == 0 || _stricmp(lang_style, "js") == 0 || _stricmp(lang_style, "ts") == 0) {
        out_ctx->lang_type = 2;
        return JavascriptRowCallback;
    } else if (_stricmp(lang_style, "c") == 0) {
        out_ctx->lang_type = 3;
        return CStructRowCallback;
    } else if (_stricmp(lang_style, "c++") == 0 || _stricmp(lang_style, "cpp") == 0) {
        out_ctx->lang_type = 4;
        return CppObjectRowCallback;
    } else if (_stricmp(lang_style, "java") == 0) {
        out_ctx->lang_type = 5;
        return JavaObjectRowCallback;
    } else if (_stricmp(lang_style, "php") == 0) {
        out_ctx->lang_type = 6;
        return PhpArrayRowCallback;
    } else if (_stricmp(lang_style, "json") == 0) {
        out_ctx->lang_type = 7;
        return JavascriptRowCallback;
    } else if (_stricmp(lang_style, "csv") == 0) {
        out_ctx->lang_type = 8;
        return CsvRowCallback;
    } else if (_stricmp(lang_style, "ini") == 0) {
        out_ctx->lang_type = 9;
        return IniRowCallback;
    } else if (_stricmp(lang_style, "xml") == 0) {
        out_ctx->lang_type = 10;
        return XmlRowCallback;
    } else if (_stricmp(lang_style, "vbs") == 0) {
        out_ctx->lang_type = 11;
        return VbsRowCallback;
    }

    out_ctx->lang_type = 0;
    return NULL;
}