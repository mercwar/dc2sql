#include "Tracker.h"
#include <stdlib.h>
#include <string.h>

VOID*** MW_WINS = NULL;

// Safely allocate or free workspace memory blocks via handle indices
FG_Handle _FG_Let(FG_Handle hndl, SIZE_T size) {
    if (!MW_WINS) return 0;

    UINT32 high = FG_HIGH(hndl);
    UINT32 low  = FG_LOW(hndl);

    if (high >= 256 || low >= 256) return 0;

    // Free existing memory block if present
    if (MW_WINS[high] && MW_WINS[high][low]) {
        free(MW_WINS[high][low]);
        MW_WINS[high][low] = NULL;
    }

    // A size allocation of 0 indicates an explicit command to sweep and free resources safely
    if (size == 0) {
        return hndl;
    }

    if (!MW_WINS[high]) return 0;

    VOID *mem = malloc(size);
    if (!mem) return 0;

    memset(mem, 0, size);
    MW_WINS[high][low] = mem;

    return hndl;
}

// Retrieve a pointer to the tracked workspace instance
VOID* _FG_Get(FG_Handle handle) {
    if (!MW_WINS) return NULL;
    
    UINT32 h = FG_HIGH(handle);
    UINT32 l = FG_LOW(handle);
    
    if (h >= 256 || l >= 256) return NULL;
    if (!MW_WINS[h]) return NULL;
    
    return MW_WINS[h][l];
}

// Copy data into an existing tracked workspace instance memory block
VOID _FG_Set(FG_Handle handle, VOID *src, SIZE_T size) {
    VOID *dest = _FG_Get(handle);
    if (dest && src && size > 0) {
        memcpy(dest, src, size);
    }
}

// Clean up tracker matrix tables on engine termination
void ShutdownTrackerEngine(void) {
    if (!MW_WINS) return;

    for (int i = 0; i < 256; i++) {
        if (MW_WINS[i]) {
            for (int j = 0; j < 256; j++) {
                if (MW_WINS[i][j]) {
                    free(MW_WINS[i][j]);
                    MW_WINS[i][j] = NULL;
                }
            }
            free(MW_WINS[i]);
            MW_WINS[i] = NULL;
        }
    }
    free(MW_WINS);
    MW_WINS = NULL;
}