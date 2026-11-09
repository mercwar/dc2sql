#ifndef TRACKER_H
#define TRACKER_H

#include <windows.h>

typedef UINT_PTR FG_Handle;

#define FG_HIGH(h) ((UINT32)(((h) >> 16) & 0xFFFF))
#define FG_LOW(h)  ((UINT32)((h) & 0xFFFF))

// Crucial: Declares MW_WINS as an extern so dc2jet.c shares it with Tracker.c
extern VOID*** MW_WINS;

#ifdef __cplusplus
extern "C" {
#endif

FG_Handle _FG_Let(FG_Handle hndl, SIZE_T size);
VOID*     _FG_Get(FG_Handle handle);
VOID      _FG_Set(FG_Handle handle, VOID *src, SIZE_T size);
void      ShutdownTrackerEngine(void);

#ifdef __cplusplus
}
#endif

#endif // TRACKER_H