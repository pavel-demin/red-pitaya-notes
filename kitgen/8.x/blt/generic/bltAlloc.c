#include "bltInt.h"

#ifndef linux
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif /* HAVE_MALLOC_H */
#endif

/*
 * Blt_MallocProcPtr, Blt_FreeProcPtr --
 *
 *	These global variables allow you to override the default
 *	memory allocation/deallocation routines, simply by setting the
 *	pointers to your own C functions.  By default, we try to use
 *	the same memory allocation scheme that Tcl is using: generally
 *	that's Tcl_Alloc and Tcl_Free.
 */

EXTERN Blt_MallocProc TclpAlloc;
EXTERN Blt_FreeProc TclpFree;
EXTERN Blt_ReallocProc TclpRealloc;

Blt_MallocProc *Blt_MallocProcPtr = TclpAlloc;
Blt_FreeProc *Blt_FreeProcPtr = TclpFree;
Blt_ReallocProc *Blt_ReallocProcPtr = TclpRealloc;

void *
Blt_Calloc(nElems, sizeOfElem)
    unsigned int nElems; 
    size_t sizeOfElem;
{
    char *ptr;
    size_t size;

    size = nElems * sizeOfElem;
    ptr = Blt_Malloc(size);
    if (ptr != NULL) {
	memset(ptr, 0, size);
    }
    return ptr;
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_Strdup --
 *
 *      Create a copy of the string from heap storage.
 *
 * Results:
 *      Returns a pointer to the need string copy.
 *
 *----------------------------------------------------------------------
 */
char *
Blt_Strdup(string)
    CONST char *string;
{
    size_t size;
    char *ptr;

    size = strlen(string) + 1;
    ptr = Blt_Malloc(size * sizeof(char));
    if (ptr != NULL) {
	strcpy(ptr, string);
    }
    return ptr;
}

