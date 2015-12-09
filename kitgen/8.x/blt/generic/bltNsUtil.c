/*
 * bltNsUtil.c --
 *
 *	This module implements utility procedures for namespaces
 *	in the BLT toolkit.
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
#include "bltList.h"
#include "bltNsUtil.h"

/* Namespace related routines */

/*
 * ----------------------------------------------------------------------
 *
 * Blt_GetVariableNamespace --
 *
 *	Returns the namespace context of the vector variable.  If NULL,
 *	this indicates that the variable is local to the call frame.
 *
 *	Note the ever-dangerous manner in which we get this information.
 *	All of these structures are "private".   Now who's calling Tcl
 *	an "extension" language?
 *
 * Results:
 *	Returns the context of the namespace in an opaque type.
 *
 * ----------------------------------------------------------------------
 */


/*
 * A Command structure exists for each command in a namespace. The
 * Tcl_Command opaque type actually refers to these structures.
 */

Tcl_Namespace *
Blt_GetVariableNamespace(interp, name)
    Tcl_Interp *interp;
    CONST char *name;
{
    Tcl_Var varPtr;
    Tcl_Namespace *nsPtr;
    Tcl_Obj *objPtr;
    CONST char *str, *cp;

    varPtr = Tcl_FindNamespaceVar(interp, (char *)name, 
	(Tcl_Namespace *)NULL, 0);
    if (varPtr == NULL) {
	return NULL;
    }
    objPtr = Tcl_NewObj();
    Tcl_GetVariableFullName(interp, varPtr, objPtr);
    str = Tcl_GetString(objPtr);
    if (Blt_ParseQualifiedName(interp, str, &nsPtr, &cp) != TCL_OK) {
        nsPtr = NULL;
    }
    Tcl_DecrRefCount(objPtr);
    return nsPtr;
}

/*ARGSUSED*/
Tcl_Namespace *
Blt_GetCommandNamespace(interp, cmdToken)
    Tcl_Interp *interp;		/* Not used. */
    Tcl_Command cmdToken;
{
    Tcl_CmdInfo info;

    if (Tcl_GetCommandInfoFromToken(cmdToken, &info) == 0) {
        return NULL;
    }
    return info.namespacePtr;
}

Tcl_CallFrame *
Blt_EnterNamespace(interp, nsPtr)
    Tcl_Interp *interp;
    Tcl_Namespace *nsPtr;
{
    Tcl_CallFrame *framePtr;

    framePtr = Blt_Malloc(sizeof(Tcl_CallFrame));
    assert(framePtr);
    if (Tcl_PushCallFrame(interp, framePtr, (Tcl_Namespace *)nsPtr, 0)
	!= TCL_OK) {
	Blt_Free(framePtr);
	return NULL;
    }
    return framePtr;
}

void
Blt_LeaveNamespace(interp, framePtr)
    Tcl_Interp *interp;
    Tcl_CallFrame *framePtr;
{
    Tcl_PopCallFrame(interp);
    Blt_Free(framePtr);
}

int
Blt_ParseQualifiedName(interp, qualName, nsPtrPtr, namePtrPtr)
    Tcl_Interp *interp;
    CONST char *qualName;
    Tcl_Namespace **nsPtrPtr;
    CONST char **namePtrPtr;
{
    register char *p, *colon;
    Tcl_Namespace *nsPtr;

    colon = NULL;
    p = (char *)(qualName + strlen(qualName));
    while (--p > qualName) {
	if ((*p == ':') && (*(p - 1) == ':')) {
	    p++;		/* just after the last "::" */
	    colon = p - 2;
	    break;
	}
    }
    if (colon == NULL) {
	*nsPtrPtr = NULL;
	*namePtrPtr = (char *)qualName;
	return TCL_OK;
    }
    *colon = '\0';
    if (qualName[0] == '\0') {
	nsPtr = Tcl_GetGlobalNamespace(interp);
    } else {
	nsPtr = Tcl_FindNamespace(interp, (char *)qualName, 
		(Tcl_Namespace *)NULL, 0);
    }
    *colon = ':';
    if (nsPtr == NULL) {
	return TCL_ERROR;
    }
    *nsPtrPtr = nsPtr;
    *namePtrPtr = p;
    return TCL_OK;
}

char *
Blt_GetQualifiedName(nsPtr, name, resultPtr)
    Tcl_Namespace *nsPtr;
    CONST char *name;
    Tcl_DString *resultPtr;
{
    Tcl_DStringInit(resultPtr);
    if ((nsPtr->fullName[0] != ':') || (nsPtr->fullName[1] != ':') ||
	(nsPtr->fullName[2] != '\0')) {
	Tcl_DStringAppend(resultPtr, nsPtr->fullName, -1);
    }
    Tcl_DStringAppend(resultPtr, "::", -1);
    Tcl_DStringAppend(resultPtr, (char *)name, -1);
    return Tcl_DStringValue(resultPtr);
}


typedef struct {
    Tcl_HashTable clientTable;

    /* Original clientdata and delete procedure. */
    ClientData origClientData;
    Tcl_NamespaceDeleteProc *origDeleteProc;

} Callback;

static Tcl_CmdProc NamespaceDeleteCmd;
static Tcl_NamespaceDeleteProc NamespaceDeleteNotify;

#define NS_DELETE_CMD	"#NamespaceDeleteNotifier"

/*ARGSUSED*/
static int
NamespaceDeleteCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/*  */
    int argc;
    char **argv;
{
    Tcl_AppendResult(interp, "command \"", argv[0], "\" shouldn't be invoked",
	(char *)NULL);
    return TCL_ERROR;
}

static void
NamespaceDeleteNotify(clientData)
    ClientData clientData;
{
    Blt_List list;
    Blt_ListNode node;
    Tcl_CmdDeleteProc *deleteProc;

    list = (Blt_List)clientData;
    for (node = Blt_ListFirstNode(list); node != NULL;
	node = Blt_ListNextNode(node)) {
	deleteProc = (Tcl_CmdDeleteProc *)Blt_ListGetValue(node);
	clientData = (ClientData)Blt_ListGetKey(node);
	(*deleteProc) (clientData);
    }
    Blt_ListDestroy(list);
}

void
Blt_DestroyNsDeleteNotify(interp, nsPtr, clientData)
    Tcl_Interp *interp;
    Tcl_Namespace *nsPtr;
    ClientData clientData;
{
    Blt_List list;
    Blt_ListNode node;
    char *string;
    Tcl_CmdInfo cmdInfo;

    string = Blt_Malloc(sizeof(nsPtr->fullName) + strlen(NS_DELETE_CMD) + 4);
    strcpy(string, nsPtr->fullName);
    strcat(string, "::");
    strcat(string, NS_DELETE_CMD);
    if (!Tcl_GetCommandInfo(interp, string, &cmdInfo)) {
	goto done;
    }
    list = (Blt_List)cmdInfo.clientData;
    node = Blt_ListGetNode(list, clientData);
    if (node != NULL) {
	Blt_ListDeleteNode(node);
    }
  done:
    Blt_Free(string);
}

int
Blt_CreateNsDeleteNotify(interp, nsPtr, clientData, deleteProc)
    Tcl_Interp *interp;
    Tcl_Namespace *nsPtr;
    ClientData clientData;
    Tcl_CmdDeleteProc *deleteProc;
{
    Blt_List list;
    char *string;
    Tcl_CmdInfo cmdInfo;

    string = Blt_Malloc(sizeof(nsPtr->fullName) + strlen(NS_DELETE_CMD) + 4);
    strcpy(string, nsPtr->fullName);
    strcat(string, "::");
    strcat(string, NS_DELETE_CMD);
    if (!Tcl_GetCommandInfo(interp, string, &cmdInfo)) {
	list = Blt_ListCreate(BLT_ONE_WORD_KEYS);
	Blt_CreateCommand(interp, string, NamespaceDeleteCmd, list, 
		NamespaceDeleteNotify);
    } else {
	list = (Blt_List)cmdInfo.clientData;
    }
    Blt_Free(string);
    Blt_ListAppend(list, clientData, (ClientData)deleteProc);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_CreateCommand --
 *
 *	Like Tcl_CreateCommand, but creates command in current namespace
 *	instead of global, if one isn't defined.  Not a problem with
 *	[incr Tcl] namespaces.
 *
 * Results:
 *	The return value is a token for the command, which can
 *	be used in future calls to Tcl_GetCommandName.
 *
 *----------------------------------------------------------------------
 */
Tcl_Command
Blt_CreateCommand(interp, cmdName, proc, clientData, deleteProc)
    Tcl_Interp *interp;		/* Token for command interpreter returned by
				 * a previous call to Tcl_CreateInterp. */
    CONST char *cmdName;	/* Name of command. If it contains namespace
				 * qualifiers, the new command is put in the
				 * specified namespace; otherwise it is put
				 * in the global namespace. */
    Tcl_CmdProc *proc;		/* Procedure to associate with cmdName. */
    ClientData clientData;	/* Arbitrary value passed to string proc. */
    Tcl_CmdDeleteProc *deleteProc;
    /* If not NULL, gives a procedure to call

				 * when this command is deleted. */
{
    register CONST char *p;

    p = cmdName + strlen(cmdName);
    while (--p > cmdName) {
	if ((*p == ':') && (*(p - 1) == ':')) {
	    p++;		/* just after the last "::" */
	    break;
	}
    }
    if (cmdName == p) {
	Tcl_DString dString;
	Tcl_Namespace *nsPtr;
	Tcl_Command cmdToken;

	Tcl_DStringInit(&dString);
	nsPtr = Tcl_GetCurrentNamespace(interp);
	Tcl_DStringAppend(&dString, nsPtr->fullName, -1);
	Tcl_DStringAppend(&dString, "::", -1);
	Tcl_DStringAppend(&dString, cmdName, -1);
	cmdToken = Tcl_CreateCommand(interp, Tcl_DStringValue(&dString), proc,
	    clientData, deleteProc);
	Tcl_DStringFree(&dString);
	return cmdToken;
    }
    return Tcl_CreateCommand(interp, (char *)cmdName, proc, clientData, 
	deleteProc);
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_CreateCommandObj --
 *
 *	Like Tcl_CreateCommand, but creates command in current namespace
 *	instead of global, if one isn't defined.  Not a problem with
 *	[incr Tcl] namespaces.
 *
 * Results:
 *	The return value is a token for the command, which can
 *	be used in future calls to Tcl_GetCommandName.
 *
 *----------------------------------------------------------------------
 */
Tcl_Command
Blt_CreateCommandObj(interp, cmdName, proc, clientData, deleteProc)
    Tcl_Interp *interp;		/* Token for command interpreter returned by
				 * a previous call to Tcl_CreateInterp. */
    CONST char *cmdName;	/* Name of command. If it contains namespace
				 * qualifiers, the new command is put in the
				 * specified namespace; otherwise it is put
				 * in the global namespace. */
    Tcl_ObjCmdProc *proc;	/* Procedure to associate with cmdName. */
    ClientData clientData;	/* Arbitrary value passed to string proc. */
    Tcl_CmdDeleteProc *deleteProc;
				/* If not NULL, gives a procedure to call
				 * when this command is deleted. */
{
    register CONST char *p;

    p = cmdName + strlen(cmdName);
    while (--p > cmdName) {
	if ((*p == ':') && (*(p - 1) == ':')) {
	    p++;		/* just after the last "::" */
	    break;
	}
    }
    if (cmdName == p) {
	Tcl_DString dString;
	Tcl_Namespace *nsPtr;
	Tcl_Command cmdToken;

	Tcl_DStringInit(&dString);
	nsPtr = Tcl_GetCurrentNamespace(interp);
	Tcl_DStringAppend(&dString, nsPtr->fullName, -1);
	Tcl_DStringAppend(&dString, "::", -1);
	Tcl_DStringAppend(&dString, cmdName, -1);
	cmdToken = Tcl_CreateObjCommand(interp, Tcl_DStringValue(&dString), 
		proc, clientData, deleteProc);
	Tcl_DStringFree(&dString);
	return cmdToken;
    }
    return Tcl_CreateObjCommand(interp, (char *)cmdName, proc, clientData, 
	deleteProc);
}
