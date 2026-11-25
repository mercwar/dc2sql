#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifndef BUILDING_JET_DLL
#define BUILDING_JET_DLL
#endif


#include "dc2sql_diag.h"

#define MAX_DIAG_ENTRIES 256
#define MAX_CAT_LEN      64
#define MAX_MSG_LEN      512

typedef struct {
    DWORD timestamp;
    char  category[MAX_CAT_LEN];
    char  message[MAX_MSG_LEN];
} DiagEntry;

static volatile LONG    g_diag_enabled = 0;
static DiagEntry        g_diag_buffer[MAX_DIAG_ENTRIES];
static volatile LONG    g_diag_head = 0;
static volatile LONG    g_diag_tail = 0;
static CRITICAL_SECTION g_diag_lock;
static INIT_ONCE        g_diag_init_once = INIT_ONCE_STATIC_INIT;

static BOOL CALLBACK InitDiagLockCallback(PINIT_ONCE initOnce, PVOID param, PVOID* context) {
    (void)initOnce;
    (void)param;
    (void)context;
    InitializeCriticalSectionAndSpinCount(&g_diag_lock, 4000);
    return TRUE;
}

static void EnsureDiagLock(void) {
    InitOnceExecuteOnce(&g_diag_init_once, InitDiagLockCallback, NULL, NULL);
}

DIAG_API void __stdcall diag_set_enabled(bool enabled) {
    InterlockedExchange(&g_diag_enabled, enabled ? 1 : 0);
}

DIAG_API bool __stdcall diag_is_enabled(void) {
    return (InterlockedCompareExchange(&g_diag_enabled, 0, 0) != 0);
}

DIAG_API void __stdcall diag_log_event(const char* category, const char* message) {
    if (!diag_is_enabled() || !message) return;

    EnsureDiagLock();
    EnterCriticalSection(&g_diag_lock);

    LONG head = g_diag_head;
    LONG next_head = (head + 1) % MAX_DIAG_ENTRIES;

    /* Drop oldest entry if ring buffer is full */
    if (next_head == g_diag_tail) {
        g_diag_tail = (g_diag_tail + 1) % MAX_DIAG_ENTRIES;
    }

    g_diag_buffer[head].timestamp = GetTickCount();
    strncpy_s(g_diag_buffer[head].category, sizeof(g_diag_buffer[head].category), category ? category : "SYSTEM", _TRUNCATE);
    strncpy_s(g_diag_buffer[head].message, sizeof(g_diag_buffer[head].message), message, _TRUNCATE);

    g_diag_head = next_head;

    LeaveCriticalSection(&g_diag_lock);
}

DIAG_API void __stdcall diag_log_session_event(const char* session_name, const char* category, const char* message) {
    if (!diag_is_enabled() || !message) return;

    char formattedMsg[MAX_MSG_LEN];
    snprintf(formattedMsg, sizeof(formattedMsg), "[SID:%s] %s",
             (session_name && session_name[0] != '\0') ? session_name : "default",
             message);

    diag_log_event(category, formattedMsg);
}

DIAG_API int __stdcall diag_get_buffered_count(void) {
    EnsureDiagLock();
    EnterCriticalSection(&g_diag_lock);

    LONG head = g_diag_head;
    LONG tail = g_diag_tail;
    int count = (head >= tail) ? (int)(head - tail) : (int)(MAX_DIAG_ENTRIES - tail + head);

    LeaveCriticalSection(&g_diag_lock);
    return count;
}

DIAG_API bool __stdcall diag_flush_reports(char* out_buffer, size_t max_size) {
    if (!out_buffer || max_size == 0) return false;

    EnsureDiagLock();
    EnterCriticalSection(&g_diag_lock);

    size_t offset = 0;
    int hdr = snprintf(out_buffer + offset, max_size - offset, "\r\n--- [DC2SQL / MS-JET DIAGNOSTIC TELEMETRY] ---\r\n");
    if (hdr > 0 && (size_t)hdr < max_size) {
        offset += (size_t)hdr;
    }

    LONG tail = g_diag_tail;
    LONG head = g_diag_head;

    if (tail == head && offset < max_size - 32) {
        int emptyMsg = snprintf(out_buffer + offset, max_size - offset, "(No buffered diagnostic events)\r\n");
        if (emptyMsg > 0) offset += (size_t)emptyMsg;
    }

    while (tail != head && offset < max_size - 128) {
        DiagEntry* entry = &g_diag_buffer[tail];
        int written = snprintf(out_buffer + offset, max_size - offset,
                               "[%lu ms] [%s] %s\r\n",
                               entry->timestamp, entry->category, entry->message);
        if (written > 0) {
            offset += (size_t)written;
        }
        tail = (tail + 1) % MAX_DIAG_ENTRIES;
    }

    g_diag_tail = head;

    LeaveCriticalSection(&g_diag_lock);
    return true;
}

DIAG_API void __stdcall diag_clear_buffer(void) {
    EnsureDiagLock();
    EnterCriticalSection(&g_diag_lock);
    g_diag_head = 0;
    g_diag_tail = 0;
    LeaveCriticalSection(&g_diag_lock);
}
