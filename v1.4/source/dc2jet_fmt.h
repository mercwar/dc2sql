#ifndef DC2JET_FMT_H
#define DC2JET_FMT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JetContext JetContext;
typedef uint64_t JetSessionHandle;
typedef bool (__stdcall *JetRowCallback)(int col_count, const char** col_names, const char** col_values, void* user_data);

typedef struct {
    int    row_counter;
    int    lang_type;
    bool   enable_ack;
    double duration_sec;
    bool   first_item;
} LangContext;

void PrintHeaderAcknowledgment(int lang_type, bool enable_ack, double elapsed_sec);
void PrintFooterAcknowledgment(int lang_type, bool enable_ack);

bool __stdcall PythonRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data);
bool __stdcall JavascriptRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data);
bool __stdcall CStructRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data);
bool __stdcall CppObjectRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data);
bool __stdcall JavaObjectRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data);
bool __stdcall PhpArrayRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data);
bool __stdcall CsvRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data);
bool __stdcall IniRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data);
bool __stdcall XmlRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data);
bool __stdcall VbsRowCallback(int col_count, const char** col_names, const char** col_values, void* user_data);

JetRowCallback GetLangFormatterCallback(const char* lang_style, bool enable_ack, LangContext* out_ctx);

#ifdef __cplusplus
}
#endif

#endif /* DC2JET_FMT_H */