#ifndef DC2SQL_H
#define DC2SQL_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "dc2jet.h"

#ifdef BUILDING_JET_DLL
    #define DC2SQL_API __declspec(dllexport)
#else
    #define DC2SQL_API __declspec(dllimport)
#endif

typedef uint64_t FG_Handle;

#ifdef __cplusplus
extern "C" {
#endif

DC2SQL_API SQLRETURN __stdcall DC2SQL_InitializeEngine(void);
DC2SQL_API FG_Handle __stdcall DC2SQL_OpenSession(const char* sessionId, const char* dbPath);
DC2SQL_API FG_Handle __stdcall DC2SQL_OpenSessionEx(const char* sessionId, const char* baseDir, const char* dbPath, const char* passkey);
DC2SQL_API SQLRETURN __stdcall DC2SQL_ExecuteQuery(FG_Handle handle, const char* sqlQuery, char* outputBuffer, size_t bufferSize);
DC2SQL_API void      __stdcall DC2SQL_CloseSession(FG_Handle handle);
DC2SQL_API int       __stdcall DC2SQL_GetActiveSessionCount(void);
DC2SQL_API bool      __stdcall DC2SQL_WaitForSessions(DWORD pollIntervalMs, DWORD timeoutMs);
DC2SQL_API void      __stdcall DC2SQL_ShutdownEngine(void);

#ifdef __cplusplus
}
#endif

#endif /* DC2SQL_H */
