/*
 * bltSwitch.c --
 *
 *	This module implements command/argument switch parsing 
 *	procedures for the BLT toolkit.
 *
 * Copyright 1991-1998 Lucent Technologies, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and warranty
 * disclaimer appear in supporting documentation, and that the names
 * of Lucent Technologies any of their entities not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 *
 * Lucent Technologies disclaims all warranties with regard to this
 * software, including all implied warranties of merchantability and
 * fitness.  In no event shall Lucent Technologies be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether in
 * an action of contract, negligence or other tortuous action, arising
 * out of or in connection with the use or performance of this
 * software.
 */

#include "bltInt.h"
#if defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "bltSwitch.h"

static Blt_SwitchSpec *	GetCachedSwitchSpecs _ANSI_ARGS_((Tcl_Interp *interp,
    const Blt_SwitchSpec *staticSpecs));
    static void		DeleteSpecCacheTable _ANSI_ARGS_((
    ClientData clientData, Tcl_Interp *interp));

/*
 *--------------------------------------------------------------
 *
 * FindSwitchSpec --
 *
 *	Search through a table of configuration specs, looking for
 *	one that matches a given argvName.
 *
 * Results:
 *	The return value is a pointer to the matching entry, or NULL
 *	if nothing matched.  In that case an error message is left
 *	in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */
static Blt_SwitchSpec *
FindSwitchSpec(interp, specs, name, needFlags, hateFlags, flags)
    Tcl_Interp *interp;		/* Used for reporting errors. */
    Blt_SwitchSpec *specs;	/* Pointer to table of configuration
				 * specifications for a widget. */
    char *name;			/* Name (suitable for use in a "switch"
				 * command) identifying particular option. */
    int needFlags;		/* Flags that must be present in matching
				 * entry. */
    int hateFlags;		/* Flags that must NOT be present in
				 * matching entry. */
    int flags;
{
    register Blt_SwitchSpec *specPtr;
    register char c;		/* First character of current argument. */
    Blt_SwitchSpec *matchPtr;	/* Matching spec, or NULL. */
    size_t length;

    c = name[1];
    length = strlen(name);
    matchPtr = NULL;
    specs = Blt_GetCachedSwitchSpecs(interp, specs);
    for (specPtr = specs; specPtr->type != BLT_SWITCH_END; specPtr++) {
	if (specPtr->switchName == NULL) {
	    continue;
	}
	if ((specPtr->switchName[1] != c) 
	    || (strncmp(specPtr->switchName, name, length) != 0)) {
	    continue;
	}
	if ((flags&BLT_SWITCH_EXACT) && specPtr->switchName[length] != 0) {
	    continue;
        }
	if (((specPtr->flags & needFlags) != needFlags)
	    || (specPtr->flags & hateFlags)) {
	    continue;
	}
	if (specPtr->switchName[length] == 0) {
	    return specPtr;	/* Stop on a perfect match. */
	}
	if (matchPtr != NULL) {
	    Tcl_AppendResult(interp, "ambiguous option \"", name, "\"", 
		(char *) NULL);
	    return (Blt_SwitchSpec *) NULL;
	}
	matchPtr = specPtr;
    }

    if (matchPtr == NULL) {
	Tcl_AppendResult(interp, "unknown option \"", name, "\" not one of: ", (char *)NULL);
        for (specPtr = specs; specPtr->type != BLT_SWITCH_END; specPtr++) {
            if (specPtr->switchName == NULL) {
                continue;
            }
            if (name[1] != '?' || specPtr->type < 0 || specPtr->type >= BLT_SWITCH_END) {
                Tcl_AppendResult(interp, specPtr->switchName, " ", 0);
            } else {
                static char *typenames[BLT_SWITCH_END+10] = {
                    "bool", "int", "posint", "nonneg", "double", "string",
                    "list", "flag", "value", "custom", "END"
                };
                if (typenames[BLT_SWITCH_END] == 0 ||
                strcmp(typenames[BLT_SWITCH_END],"END")) {
                    fprintf(stderr, "Blt_SwitchTypes changed\n");
                    continue;
                }
                Tcl_AppendResult(interp, "?", specPtr->switchName, 0);
                if (specPtr->type != BLT_SWITCH_FLAG) {
                    Tcl_AppendResult(interp, " ", typenames[specPtr->type], 0);
                }
                Tcl_AppendResult(interp, "? ", 0);
            }
        }
        return (Blt_SwitchSpec *) NULL;
    }
    return matchPtr;
}

/*
 *--------------------------------------------------------------
 *
 * DoSwitch --
 *
 *	This procedure applies a single configuration option
 *	to a widget record.
 *
 * Results:
 *	A standard Tcl return value.
 *
 * Side effects:
 *	WidgRec is modified as indicated by specPtr and value.
 *	The old value is recycled, if that is appropriate for
 *	the value type.
 *
 *--------------------------------------------------------------
 */
static int
DoSwitch(interp, specPtr, string, record, obj)
    Tcl_Interp *interp;		/* Interpreter for error reporting. */
    Blt_SwitchSpec *specPtr;	/* Specifier to apply. */
    char *string;		/* Value to use to fill in widgRec. */
    ClientData record;		/* Record whose fields are to be
				 * modified.  Values must be properly
				 * initialized. */
    Tcl_Obj *obj;
{
    char *ptr;
    int isNull;
    int count;

    isNull = ((*string == '\0') && (specPtr->flags & BLT_SWITCH_NULL_OK));
    do {
	ptr = (char *)record + specPtr->offset;
	switch (specPtr->type) {
	case BLT_SWITCH_BOOLEAN:
	    if (Tcl_GetBoolean(interp, string, (int *)ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;

	case BLT_SWITCH_INT:
	    if (Tcl_GetInt(interp, string, (int *)ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;

	case BLT_SWITCH_INT_NONNEGATIVE:
	    if (Tcl_GetInt(interp, string, &count) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (count < 0) {
		Tcl_AppendResult(interp, "bad value \"", string, "\": ",
				 "can't be negative", (char *)NULL);
		return TCL_ERROR;
	    }
	    *((int *)ptr) = count;
	    break;

	case BLT_SWITCH_INT_POSITIVE:
	    if (Tcl_GetInt(interp, string, &count) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (count <= 0) {
		Tcl_AppendResult(interp, "bad value \"", string, "\": ",
			"must be positive", (char *)NULL);
		return TCL_ERROR;
	    }
	    *((int *)ptr) = count;
	    break;

	case BLT_SWITCH_DOUBLE:
	    if (Tcl_GetDouble(interp, string, (double *)ptr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;
	case BLT_SWITCH_OBJ:
	    (*((Tcl_Obj **)ptr)) = obj;
	    break;

	case BLT_SWITCH_STRING: 
	    {
		char *old, *new, **strPtr;
		
		strPtr = (char **)ptr;
		if (isNull) {
		    new = NULL;
		} else {
		    new = Blt_Strdup(string);
		}
		old = *strPtr;
		if (old != NULL) {
		    Blt_Free(old);
		}
		*strPtr = new;
	    }
	    break;

	case BLT_SWITCH_LIST:
	    if (Tcl_SplitList(interp, string, &count, (char ***)ptr) 
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	    break;

	case BLT_SWITCH_CUSTOM:
	    if ((*specPtr->customPtr->parseProc) \
		(specPtr->customPtr->clientData, interp, specPtr->switchName,
			string, record, specPtr->offset) != TCL_OK) {
		return TCL_ERROR;
	    }
	    break;

	default: 
	    Tcl_AppendResult(interp, "bad switch table: unknown type \"",
		 Blt_Itoa(specPtr->type), "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	specPtr++;
    } while ((specPtr->switchName == NULL) && 
	     (specPtr->type != BLT_SWITCH_END));
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Blt_ProcessSwitches --
 *
 *	Process command-line options and database options to
 *	fill in fields of a widget record with resources and
 *	other parameters.
 *
 * Results:
 *	Returns the number of arguments comsumed by parsing the
 *	command line.  If an error occurred, -1 will be returned
 *	and an error messages can be found as the interpreter
 *	result.
 *
 * Side effects:
 *	The fields of widgRec get filled in with information
 *	from argc/argv and the option database.  Old information
 *	in widgRec's fields gets recycled.
 *
 *--------------------------------------------------------------
 */
int
Blt_ProcessSwitches(interp, specs, argc, argv, record, flags)
    Tcl_Interp *interp;		/* Interpreter for error reporting. */
    Blt_SwitchSpec *specs;	/* Describes legal options. */
    int argc;			/* Number of elements in argv. */
    char **argv;		/* Command-line options. */
    char *record;		/* Record whose fields are to be
				 * modified.  Values must be properly
				 * initialized. */
    int flags;			/* Used to specify additional flags
				 * that must be present in switch specs
				 * for them to be considered.  Also,
				 * may have BLT_SWITCH_ARGV_ONLY set. */
{
    register int count;
    char *arg;
    register Blt_SwitchSpec *specPtr;
    int needFlags;		/* Specs must contain this set of flags
				 * or else they are not considered. */
    int hateFlags;		/* If a spec contains any bits here, it's
				 * not considered. */

    needFlags = flags & ~(BLT_SWITCH_USER_BIT - 1);
    hateFlags = 0;

    /*
     * Pass 1:  Clear the change flags on all the specs so that we 
     *          can check it later.
     */
    specs = Blt_GetCachedSwitchSpecs(interp, specs);
    for (specPtr = specs; specPtr->type != BLT_SWITCH_END; specPtr++) {
	specPtr->flags &= ~BLT_SWITCH_SPECIFIED;
    }
    /*
     * Pass 2:  Process the arguments that match entries in the specs.
     *		It's an error if the argument doesn't match anything.
     */
    for (count = 0; count < argc; count++) {
	arg = argv[count];
	if (flags & BLT_SWITCH_OBJV_PARTIAL) {
	    if ((arg[0] != '-') || ((arg[1] == '-') && (argv[2] == '\0'))) {
		/* 
		 * If the argument doesn't start with a '-' (not a switch)
		 * or is '--', stop processing and return the number of
		 * arguments comsumed. 
		 */
		return count;
	    }
	}
	specPtr = FindSwitchSpec(interp, specs, arg, needFlags, hateFlags, flags);
	if (specPtr == NULL) {
	    return -1;
	}
	if (specPtr->type == BLT_SWITCH_FLAG) {
	    char *ptr;
	    
	    ptr = record + specPtr->offset;
	    *((int *)ptr) |= specPtr->value;
	} else if (specPtr->type == BLT_SWITCH_VALUE) {
	    char *ptr;
	    
	    ptr = record + specPtr->offset;
	    *((int *)ptr) = specPtr->value;
	} else {
	    if ((count + 1) == argc) {
		Tcl_AppendResult(interp, "value for \"", arg, "\" missing", 
			(char *) NULL);
		return -1;
	    }
	    count++;
	    if (DoSwitch(interp, specPtr, argv[count], record, NULL) != TCL_OK) {
		char msg[100];

		sprintf(msg, "\n    (processing \"%.40s\" option)", 
			specPtr->switchName);
		Tcl_AddErrorInfo(interp, msg);
		return -1;
	    }
	}
	specPtr->flags |= BLT_SWITCH_SPECIFIED;
    }
    return count;
}

#if (TCL_VERSION_NUMBER >= _VERSION(8,0,0)) 

/*
 *--------------------------------------------------------------
 *
 * Blt_ProcessObjSwitches --
 *
 *	Process command-line options and database options to
 *	fill in fields of a widget record with resources and
 *	other parameters.
 *
 * Results:
 *	Returns the number of arguments comsumed by parsing the
 *	command line.  If an error occurred, -1 will be returned
 *	and an error messages can be found as the interpreter
 *	result.
 *
 * Side effects:
 *	The fields of widgRec get filled in with information
 *	from argc/argv and the option database.  Old information
 *	in widgRec's fields gets recycled.
 *
 *--------------------------------------------------------------
 */
int
Blt_ProcessObjSwitches(interp, specs, objc, objv, record, flags)
    Tcl_Interp *interp;		/* Interpreter for error reporting. */
    Blt_SwitchSpec *specs;	/* Describes legal options. */
    int objc;			/* Number of elements in argv. */
    Tcl_Obj *CONST *objv;	/* Command-line options. */
    char *record;		/* Record whose fields are to be
				 * modified.  Values must be properly
				 * initialized. */
    int flags;			/* Used to specify additional flags
				 * that must be present in switch specs
				 * for them to be considered.  Also,
				 * may have BLT_SWITCH_ARGV_ONLY set. */
{
    register Blt_SwitchSpec *specPtr;
    register int count;
    int needFlags;		/* Specs must contain this set of flags
				 * or else they are not considered. */
    int hateFlags;		/* If a spec contains any bits here, it's
				 * not considered. */

    needFlags = flags & ~(BLT_SWITCH_USER_BIT - 1);
    hateFlags = 0;

    /*
     * Pass 1:  Clear the change flags on all the specs so that we 
     *          can check it later.
     */
    specs = Blt_GetCachedSwitchSpecs(interp, specs);
    for (specPtr = specs; specPtr->type != BLT_SWITCH_END; specPtr++) {
	specPtr->flags &= ~BLT_SWITCH_SPECIFIED;
    }
    /*
     * Pass 2:  Process the arguments that match entries in the specs.
     *		It's an error if the argument doesn't match anything.
     */
    for (count = 0; count < objc; count++) {
	char *arg;

	arg = Tcl_GetString(objv[count]);
	if (flags & BLT_SWITCH_OBJV_PARTIAL) {
	    if ((arg[0] != '-') || ((arg[1] == '-') && (arg[2] == '\0'))) {
		/* 
		 * If the argument doesn't start with a '-' (not a switch)
		 * or is '--', stop processing and return the number of
		 * arguments comsumed. 
		 */
		return count;
	    }
	}
	specPtr = FindSwitchSpec(interp, specs, arg, needFlags, hateFlags, flags);
	if (specPtr == NULL) {
	    return -1;
	}
	if (specPtr->type == BLT_SWITCH_FLAG) {
	    char *ptr;
	    
	    ptr = record + specPtr->offset;
	    *((int *)ptr) |= specPtr->value;
	} else if (specPtr->type == BLT_SWITCH_VALUE) {
	    char *ptr;
	    
	    ptr = record + specPtr->offset;
	    *((int *)ptr) = specPtr->value;
	} else {
	    count++;
	    if (count == objc) {
		Tcl_AppendResult(interp, "value for \"", arg, "\" missing", 
				 (char *) NULL);
		return -1;
	    }
	    arg = Tcl_GetString(objv[count]);
	    if (DoSwitch(interp, specPtr, arg, record, objv[count]) != TCL_OK) {
		char msg[100];

		sprintf(msg, "\n    (processing \"%.40s\" option)", 
			specPtr->switchName);
		Tcl_AddErrorInfo(interp, msg);
		return -1;
	    }
	}
	specPtr->flags |= BLT_SWITCH_SPECIFIED;
    }
    return count;
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * Blt_FreeSwitches --
 *
 *	Free up all resources associated with switch options.
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
void
Blt_FreeSwitches(interp, specs, record, needFlags)
    Tcl_Interp *interp;
    Blt_SwitchSpec *specs;	/* Describes legal options. */
    char *record;		/* Record whose fields contain current
				 * values for options. */
    int needFlags;		/* Used to specify additional flags
				 * that must be present in config specs
				 * for them to be considered. */
{
    register Blt_SwitchSpec *specPtr;

    specs = Blt_GetCachedSwitchSpecs(interp, specs);
    for (specPtr = specs; specPtr->type != BLT_SWITCH_END; specPtr++) {
	if ((specPtr->flags & needFlags) == needFlags) {
	    char *ptr;

	    ptr = record + specPtr->offset;
	    switch (specPtr->type) {
	    case BLT_SWITCH_STRING:
	    case BLT_SWITCH_LIST:
		if (*((char **) ptr) != NULL) {
		    Blt_Free(*((char **) ptr));
		    *((char **) ptr) = NULL;
		}
		break;

	    case BLT_SWITCH_CUSTOM:
		if ((*(char **)ptr != NULL) && 
		    (specPtr->customPtr->freeProc != NULL)) {
		    (*specPtr->customPtr->freeProc)(*(char **)ptr);
		    *((char **) ptr) = NULL;
		}
		break;

	    default:
		break;
	    }
	}
    }
}


/*
 *----------------------------------------------------------------------
 *
 * Blt_SwitchModified --
 *
 *      Given the configuration specifications and one or more option
 *	patterns (terminated by a NULL), indicate if any of the matching
 *	configuration options has been reset.
 *
 * Results:
 *      Returns 1 if one of the options has changed, 0 otherwise.
 *
 *----------------------------------------------------------------------
 */
int Blt_SwitchChanged
TCL_VARARGS_DEF(Blt_SwitchSpec *, arg1)
{
    va_list argList;
    Blt_SwitchSpec *specs;
    register Blt_SwitchSpec *specPtr;
    register char *switchName;
    Tcl_Interp *interp;

    specs = TCL_VARARGS_START(Blt_SwitchSpec *, arg1, argList);
    interp = va_arg(argList, Tcl_Interp *);
    specs = Blt_GetCachedSwitchSpecs(interp, specs);
    while ((switchName = va_arg(argList, char *)) != NULL) {
	for (specPtr = specs; specPtr->type != BLT_SWITCH_END; specPtr++) {
	    if ((Tcl_StringMatch(specPtr->switchName, switchName)) &&
		(specPtr->flags & BLT_SWITCH_SPECIFIED)) {
		va_end(argList);
		return 1;
	    }
	}
    }
    va_end(argList);
    return 0;
}

/*
*--------------------------------------------------------------
*
* DeleteSpecCacheTable --
*
*	Delete the per-interpreter copy of all the Blt_SwitchSpec tables which
*	were stored in the interpreter's assoc-data store.
*
* Results:
*	None
*
* Side effects:
*	None
*
*--------------------------------------------------------------
*/

static void
DeleteSpecCacheTable(clientData, interp)
ClientData clientData;
Tcl_Interp *interp;
{
    Tcl_HashTable *tablePtr = (Tcl_HashTable *) clientData;
    Tcl_HashEntry *entryPtr;
    Tcl_HashSearch search;

    for (entryPtr = Tcl_FirstHashEntry(tablePtr,&search); entryPtr != NULL;
    entryPtr = Tcl_NextHashEntry(&search)) {
        /*
        * Someone else deallocates the Tk_Uids themselves.
        */

        ckfree((char *) Tcl_GetHashValue(entryPtr));
    }
    Tcl_DeleteHashTable(tablePtr);
    ckfree((char *) tablePtr);
}


Blt_SwitchSpec *
Blt_GetCachedSwitchSpecs(interp, staticSpecs)
Tcl_Interp *interp;
const Blt_SwitchSpec *staticSpecs;
{
    return GetCachedSwitchSpecs(interp, staticSpecs);
}

static Blt_SwitchSpec *
GetCachedSwitchSpecs(interp, staticSpecs)
Tcl_Interp *interp;		/* Interpreter in which to store the cache. */
const Blt_SwitchSpec *staticSpecs;
/* Value to cache a copy of; it is also used
* as a key into the cache. */
{
    Blt_SwitchSpec *cachedSpecs;
    Tcl_HashTable *specCacheTablePtr;
    Tcl_HashEntry *entryPtr;
    int isNew;

    /*
    * Get (or allocate if it doesn't exist) the hash table that the writable
    * copies of the widget specs are stored in. In effect, this is
    * self-initializing code.
    */

    specCacheTablePtr = (Tcl_HashTable *)
    Tcl_GetAssocData(interp, "bltSwitchSpec.threadTable", NULL);
    if (specCacheTablePtr == NULL) {
        specCacheTablePtr = (Tcl_HashTable *) ckalloc(sizeof(Tcl_HashTable));
        Tcl_InitHashTable(specCacheTablePtr, TCL_ONE_WORD_KEYS);
        Tcl_SetAssocData(interp, "bltSwitchSpec.threadTable",
            DeleteSpecCacheTable, (ClientData) specCacheTablePtr);
    }

    /*
    * Look up or create the hash entry that the constant specs are mapped to,
        * which will have the writable specs as its associated value.
        */

        entryPtr = Tcl_CreateHashEntry(specCacheTablePtr, (char *) staticSpecs,
        &isNew);
        if (isNew) {
        unsigned int entrySpace = sizeof(Blt_SwitchSpec);
        const Blt_SwitchSpec *staticSpecPtr;

        /*
        * OK, no working copy in this interpreter so copy. Need to work out
        * how much space to allocate first.
        */

        for (staticSpecPtr=staticSpecs; staticSpecPtr->type!=BLT_SWITCH_END;
        staticSpecPtr++) {
            entrySpace += sizeof(Blt_SwitchSpec);
        }

        /*
        * Now allocate our working copy's space and copy over the contents
        * from the master copy.
        */

        cachedSpecs = (Blt_SwitchSpec *) ckalloc(entrySpace);
        memcpy((void *) cachedSpecs, (void *) staticSpecs, entrySpace);
        Tcl_SetHashValue(entryPtr, (ClientData) cachedSpecs);

    } else {
        cachedSpecs = (Blt_SwitchSpec *) Tcl_GetHashValue(entryPtr);
    }

    return cachedSpecs;
}
