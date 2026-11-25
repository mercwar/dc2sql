#ifndef DC2JET_H
#define DC2JET_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef BUILDING_JET_DLL
    #define JET_API __declspec(dllexport)
#else
    #define JET_API __declspec(dllimport)
#endif

typedef struct JetContext JetContext;
typedef uint64_t JetSessionHandle;

typedef bool (__stdcall *JetRowCallback)(int col_count, const char** col_names, const char** col_values, void* user_data);

#ifdef __cplusplus
extern "C" {
#endif

JET_API JetContext*      __stdcall jet_initialize(const char* base_dir, const char* db_filename, const char* passkey);
JET_API bool             __stdcall jet_route_path(const char* prefix, const char* input_name, const char* suffix, char* out_buffer, size_t max_size);
JET_API bool             __stdcall jet_execute_action(JetContext* ctx, const char* sql_command);
JET_API bool             __stdcall jet_execute_select(JetContext* ctx, const char* sql_command, JetRowCallback callback, void* user_data);
JET_API void             __stdcall jet_shutdown(JetContext* ctx);

JET_API JetSessionHandle __stdcall jet_open_session(const char* session_name, const char* base_dir, const char* db_filename, const char* passkey);
JET_API JetContext*      __stdcall jet_get_session(JetSessionHandle session_handle);
JET_API bool             __stdcall jet_close_session(JetSessionHandle session_handle);
JET_API int              __stdcall jet_get_active_session_count(void);

JET_API void CALLBACK RunJetTest(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow);

#ifdef __cplusplus
}
#endif

#endif /* DC2JET_H */
