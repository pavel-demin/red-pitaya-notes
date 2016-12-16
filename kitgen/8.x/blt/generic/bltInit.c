
/*
 * bltInit.c --
 *
 *	This module initials the BLT toolkit, registering its commands
 *	with the Tcl/Tk interpreter.
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

#define EXACT 0

double bltNaN;
Tcl_Obj *bltEmptyStringObjPtr;

static Tcl_MathProc MinMathProc, MaxMathProc;

static Tcl_AppInitProc *bltCmds[] =
{
    Blt_BusyInit,
    Blt_VectorInit,
    Blt_SplineInit,
    Blt_Crc32Init,
    Blt_GraphInit,
    Blt_TableInit,
    Blt_TabnotebookInit,
    Blt_BitmapInit,
    Blt_TreeInit,
    Blt_TreeViewInit,
#ifndef NO_PRINTER
    Blt_PrinterInit,
#endif
    (Tcl_AppInitProc *) NULL
};

#ifdef __BORLANDC__
static double
MakeNaN(void)
{
    union Real {
        struct DoubleWord {
            int lo, hi;
        } doubleWord;
        double number;
    } real;

    real.doubleWord.lo = real.doubleWord.hi = 0x7FFFFFFF;
    return real.number;
}
#endif /* __BORLANDC__ */

#ifdef _MSC_VER
static double
MakeNaN(void)
{
    return sqrt(-1.0);  /* Generate IEEE 754 Quiet Not-A-Number. */
}
#endif /* _MSC_VER */

#if !defined(__BORLANDC__) && !defined(_MSC_VER)
static double
MakeNaN(void)
{
    return 0.0 / 0.0;           /* Generate IEEE 754 Not-A-Number. */
}
#endif /* !__BORLANDC__  && !_MSC_VER */

/* ARGSUSED */
static int
MinMathProc(clientData, interp, argsPtr, resultPtr)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    Tcl_Value *argsPtr;
    Tcl_Value *resultPtr;
{
    Tcl_Value *op1Ptr, *op2Ptr;

    op1Ptr = argsPtr, op2Ptr = argsPtr + 1;
    if ((op1Ptr->type == TCL_INT) && (op2Ptr->type == TCL_INT)) {
	resultPtr->intValue = MIN(op1Ptr->intValue, op2Ptr->intValue);
	resultPtr->type = TCL_INT;
    } else {
	double a, b;

	a = (op1Ptr->type == TCL_INT) 
	    ? (double)op1Ptr->intValue : op1Ptr->doubleValue;
	b = (op2Ptr->type == TCL_INT)
	    ? (double)op2Ptr->intValue : op2Ptr->doubleValue;
	resultPtr->doubleValue = MIN(a, b);
	resultPtr->type = TCL_DOUBLE;
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
MaxMathProc(clientData, interp, argsPtr, resultPtr)
    ClientData clientData;	/* Not Used. */
    Tcl_Interp *interp;
    Tcl_Value *argsPtr;
    Tcl_Value *resultPtr;
{
    Tcl_Value *op1Ptr, *op2Ptr;

    op1Ptr = argsPtr, op2Ptr = argsPtr + 1;
    if ((op1Ptr->type == TCL_INT) && (op2Ptr->type == TCL_INT)) {
	resultPtr->intValue = MAX(op1Ptr->intValue, op2Ptr->intValue);
	resultPtr->type = TCL_INT;
    } else {
	double a, b;

	a = (op1Ptr->type == TCL_INT)
	    ? (double)op1Ptr->intValue : op1Ptr->doubleValue;
	b = (op2Ptr->type == TCL_INT)
	    ? (double)op2Ptr->intValue : op2Ptr->doubleValue;
	resultPtr->doubleValue = MAX(a, b);
	resultPtr->type = TCL_DOUBLE;
    }
    return TCL_OK;
}

/*LINTLIBRARY*/
EXPORT int
Blt_Init(interp)
    Tcl_Interp *interp;		/* Interpreter to add extra commands */
{
    Tcl_AppInitProc **p;
    Tcl_Namespace *nsPtr;
    Tcl_ValueType args[2];
    
    /*
     * Check that the versions of Tcl that have been loaded are
     * the same ones that BLT was compiled against.
     */

    if (
#ifdef USE_TCL_STUBS
        Tcl_InitStubs(interp, TCL_VERSION, EXACT)
#else
        Tcl_PkgRequire(interp, "Tcl", TCL_VERSION, EXACT)
#endif
        == NULL) {
        return TCL_ERROR;
    }

    /* Set the "blt_version", "blt_patchLevel", and "blt_libPath" Tcl
     * variables. We'll use them in the following script. */
    if ((Tcl_SetVar(interp, "blt_version", BLT_VERSION, 
    		TCL_GLOBAL_ONLY) == NULL) ||
        (Tcl_SetVar(interp, "blt_patchLevel", BLT_PATCH_LEVEL, 
    		TCL_GLOBAL_ONLY) == NULL)) {
        return TCL_ERROR;
    }

    nsPtr = Tcl_CreateNamespace(interp, "blt", NULL,
    			    (Tcl_NamespaceDeleteProc *) NULL);
    if (nsPtr == NULL) {
        return TCL_ERROR;
    }
    /* Initialize the BLT commands that only require Tcl. */
    for (p = bltCmds; *p != NULL; p++) {
        if ((**p) (interp) != TCL_OK) {
    	Tcl_DeleteNamespace(nsPtr);
    	return TCL_ERROR;
        }
    }
    args[0] = args[1] = TCL_EITHER;
    Tcl_CreateMathFunc(interp, "min", 2, args, MinMathProc, (ClientData)0);
    Tcl_CreateMathFunc(interp, "max", 2, args, MaxMathProc, (ClientData)0);
    Blt_RegisterArrayObj(interp);
    bltEmptyStringObjPtr = Tcl_NewStringObj("", -1);
    bltNaN = MakeNaN();

    return Tcl_PkgProvide(interp, "BLT", BLT_VERSION);
}


/*LINTLIBRARY*/
EXPORT int
Blt_SafeInit(interp)
    Tcl_Interp *interp;		/* Interpreter to add extra commands */
{
    return Blt_Init(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_InitCmd --
 *
 *      Given the name of a command, return a pointer to the
 *      clientData field of the command.
 *
 * Results:
 *      A standard TCL result. If the command is found, TCL_OK
 *	is returned and clientDataPtr points to the clientData
 *	field of the command (if the clientDataPtr in not NULL).
 *
 * Side effects:
 *      If the command is found, clientDataPtr is set to the address
 *	of the clientData of the command.  If not found, an error
 *	message is left in interp->result.
 *
 *----------------------------------------------------------------------
 */

/*ARGSUSED*/
Tcl_Command
Blt_InitCmd(interp, nsName, specPtr)
    Tcl_Interp *interp;
    char *nsName;
    Blt_CmdSpec *specPtr;
{
    char *cmdPath;
    Tcl_DString dString;
    Tcl_Command cmdToken;

    Tcl_DStringInit(&dString);

    if (nsName != NULL) {
	Tcl_DStringAppend(&dString, nsName, -1);
    }
    Tcl_DStringAppend(&dString, "::", -1);

    Tcl_DStringAppend(&dString, specPtr->name, -1);

    cmdPath = Tcl_DStringValue(&dString);
    cmdToken = Tcl_FindCommand(interp, cmdPath, (Tcl_Namespace *)NULL, 0);
    if (cmdToken != NULL) {
	Tcl_DStringFree(&dString);
	return cmdToken;	/* Assume command was already initialized */
    }
    cmdToken = Tcl_CreateCommand(interp, cmdPath, specPtr->cmdProc,
	specPtr->clientData, specPtr->cmdDeleteProc);
    Tcl_DStringFree(&dString);

    {
	Tcl_Namespace *nsPtr;
	int dontResetList = 0;

	nsPtr = Tcl_FindNamespace(interp, nsName, (Tcl_Namespace *)NULL,
	    TCL_LEAVE_ERR_MSG);
	if (nsPtr == NULL) {
	    return NULL;
	}
	if (Tcl_Export(interp, nsPtr, specPtr->name, dontResetList) != TCL_OK) {
	    return NULL;
	}
    }
    return cmdToken;
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_InitObjCmd --
 *
 *      Given the name of a command, return a pointer to the
 *      clientData field of the command.
 *
 * Results:
 *      A standard TCL result. If the command is found, TCL_OK
 *	is returned and clientDataPtr points to the clientData
 *	field of the command (if the clientDataPtr in not NULL).
 *
 * Side effects:
 *      If the command is found, clientDataPtr is set to the address
 *	of the clientData of the command.  If not found, an error
 *	message is left in interp->result.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
Tcl_Command
Blt_InitObjCmd(interp, nsName, specPtr)
    Tcl_Interp *interp;
    char *nsName;
    Blt_ObjCmdSpec *specPtr;
{
    char *cmdPath;
    Tcl_DString dString;
    Tcl_Command cmdToken;
    Tcl_Namespace *nsPtr;

    Tcl_DStringInit(&dString);
    if (nsName != NULL) {
	Tcl_DStringAppend(&dString, nsName, -1);
    }
    Tcl_DStringAppend(&dString, "::", -1);
    Tcl_DStringAppend(&dString, specPtr->name, -1);

    cmdPath = Tcl_DStringValue(&dString);
    cmdToken = Tcl_FindCommand(interp, cmdPath, (Tcl_Namespace *)NULL, 0);
    if (cmdToken != NULL) {
	Tcl_DStringFree(&dString);
	return cmdToken;	/* Assume command was already initialized */
    }
    cmdToken = Tcl_CreateObjCommand(interp, cmdPath, 
		(Tcl_ObjCmdProc *)specPtr->cmdProc, 
		specPtr->clientData, 
		specPtr->cmdDeleteProc);
    Tcl_DStringFree(&dString);

    nsPtr = Tcl_FindNamespace(interp, nsName, (Tcl_Namespace *)NULL,
	      TCL_LEAVE_ERR_MSG);
    if (nsPtr == NULL) {
	return NULL;
    }
    if (Tcl_Export(interp, nsPtr, specPtr->name, FALSE) != TCL_OK) {
	return NULL;
    }
    return cmdToken;
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_InitCmds --
 *
 *      Given the name of a command, return a pointer to the
 *      clientData field of the command.
 *
 * Results:
 *      A standard TCL result. If the command is found, TCL_OK
 *	is returned and clientDataPtr points to the clientData
 *	field of the command (if the clientDataPtr in not NULL).
 *
 * Side effects:
 *      If the command is found, clientDataPtr is set to the address
 *	of the clientData of the command.  If not found, an error
 *	message is left in interp->result.
 *
 *----------------------------------------------------------------------
 */
int
Blt_InitCmds(interp, nsName, specPtr, nCmds)
    Tcl_Interp *interp;
    char *nsName;
    Blt_CmdSpec *specPtr;
    int nCmds;
{
    Blt_CmdSpec *endPtr;

    for (endPtr = specPtr + nCmds; specPtr < endPtr; specPtr++) {
	if (Blt_InitCmd(interp, nsName, specPtr) == NULL) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}
