/*
 * tclAppInit.c --
 *
 *  Provides a default version of the main program and Tcl_AppInit
 *  procedure for Tcl applications (without Tk).  Note that this
 *  program must be built in Win32 console mode to work properly.
 *
 * Copyright (c) 1996-1997 by Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 * Copyright (c) 2000-2006 Jean-Claude Wippler <jcw@equi4.com>
 * Copyright (c) 2003-2006 ActiveState Software Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: kitInit.c,v 1.3 2008/05/11 20:36:50 demin Exp $
 */

#ifdef KIT_INCLUDES_TK
#include <tk.h>
#else
#include <tcl.h>
#endif

#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#endif

Tcl_AppInitProc Vfs_Init, Zlib_Init;
#ifdef TCL_THREADS
Tcl_AppInitProc Thread_Init;
#endif
#ifdef _WIN32
Tcl_AppInitProc Dde_Init, Registry_Init;
#endif
#ifdef KIT_INCLUDES_TK
Tcl_AppInitProc Blt_Init, Blt_SafeInit;
#endif

Tcl_AppInitProc G2lite_Init;

#ifdef WIN32
#define DEV_NULL "NUL"
#else
#define DEV_NULL "/dev/null"
#endif

static void TclKit_InitStdChannels(void);

static char appInitCmd[] =
"proc tclKitInit {} {\n"
    "rename tclKitInit {}\n"
    "if {![info exists ::tcl::basekit]} {\n"
        "namespace eval ::tcl { variable basekit [info nameofexecutable] }\n"
    "}\n"
    "if {[file isfile /zvfs/boot.tcl]} {\n"
        "source /zvfs/boot.tcl\n"
    "} elseif {[lindex $::argv 0] eq \"-init-\"} {\n"
        "uplevel #0 { source [lindex $::argv 1] }\n"
        "exit\n"
    "} else {\n"
        "error \"\n  $::tcl::basekit has no VFS data to start up\"\n"
    "}\n"
"}\n"
"tclKitInit"
;

static const char initScript[] =
"if {[file isfile [file join /zvfs main.tcl]]} {\n"
    "if {[info commands console] != {}} { console hide }\n"
    "set tcl_interactive 0\n"
    "incr argc\n"
    "set argv [linsert $argv 0 $argv0]\n"
    "set argv0 [file join /zvfs main.tcl]\n"
"} else continue\n"
;

#ifdef WIN32
__declspec(dllexport) int
#else
extern int
#endif
TclKit_AppInit(Tcl_Interp *interp)
{
    /*
     * Ensure that std channels exist (creating them if necessary)
     */
    TclKit_InitStdChannels();

    Tcl_StaticPackage(0, "vfs", Vfs_Init, NULL);
#ifdef TCL_THREADS
    Tcl_StaticPackage(0, "Thread", Thread_Init, NULL);
#endif
#ifdef _WIN32
    Tcl_StaticPackage(0, "dde", Dde_Init, NULL);
    Tcl_StaticPackage(0, "registry", Registry_Init, NULL);
#endif
#ifdef KIT_INCLUDES_TK
    Tcl_StaticPackage(0, "Tk", Tk_Init, Tk_SafeInit);
    Tcl_StaticPackage(0, "Blt", Blt_Init, Blt_SafeInit);
#endif

    Tcl_StaticPackage(0, "g2lite", G2lite_Init, NULL);

    /* the tcl_rcFileName variable only exists in the initial interpreter */
#ifdef _WIN32
    Tcl_SetVar(interp, "tcl_rcFileName", "~/tclkitrc.tcl", TCL_GLOBAL_ONLY);
#else
    Tcl_SetVar(interp, "tcl_rcFileName", "~/.tclkitrc", TCL_GLOBAL_ONLY);
#endif

    Zvfs_Init(interp);
    Tcl_SetVar(interp, "extname", "", TCL_GLOBAL_ONLY);
    Zvfs_Mount(interp, (char *)Tcl_GetNameOfExecutable(), "/zvfs");
    Tcl_SetVar2(interp, "env", "TCL_LIBRARY", "/zvfs/lib/tcl", TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, "env", "TK_LIBRARY", "/zvfs/lib/tk", TCL_GLOBAL_ONLY);

    if ((Tcl_EvalEx(interp, appInitCmd, -1, TCL_EVAL_GLOBAL) == TCL_ERROR) || (Tcl_Init(interp) == TCL_ERROR))
        goto error;

#ifdef KIT_INCLUDES_TK
    if (Tk_Init(interp) == TCL_ERROR)
        goto error;
#ifdef _WIN32
    if (Tk_CreateConsoleWindow(interp) == TCL_ERROR)
        goto error;
#endif
#endif

    /* messy because TclSetStartupScriptPath is called slightly too late */
    if (Tcl_Eval(interp, initScript) == TCL_OK) {
        Tcl_Obj* path = Tcl_GetStartupScript(NULL);
        Tcl_SetStartupScript(Tcl_GetObjResult(interp), NULL);
        if (path == NULL)
            Tcl_Eval(interp, "incr argc -1; set argv [lrange $argv 1 end]");
    }

    Tcl_SetVar(interp, "errorInfo", "", TCL_GLOBAL_ONLY);
    Tcl_ResetResult(interp);
    return TCL_OK;

error:
#if defined(KIT_INCLUDES_TK) && defined(_WIN32)
    MessageBeep(MB_ICONEXCLAMATION);
    MessageBox(NULL, Tcl_GetStringResult(interp), "Error in Tclkit",
        MB_ICONSTOP | MB_OK | MB_TASKMODAL | MB_SETFOREGROUND);
    ExitProcess(1);
    /* we won't reach this, but we need the return */
#endif
    return TCL_ERROR;
}

static void
TclKit_InitStdChannels(void)
{
    Tcl_Channel chan;

    /*
     * We need to verify if we have the standard channels and create them if
     * not.  Otherwise internals channels may get used as standard channels
     * (like for encodings) and panic.
     */
    chan = Tcl_GetStdChannel(TCL_STDIN);
    if (chan == NULL) {
        chan = Tcl_OpenFileChannel(NULL, DEV_NULL, "r", 0);
        if (chan != NULL) {
            Tcl_SetChannelOption(NULL, chan, "-encoding", "utf-8");
        }
        Tcl_SetStdChannel(chan, TCL_STDIN);
    }
    chan = Tcl_GetStdChannel(TCL_STDOUT);
    if (chan == NULL) {
        chan = Tcl_OpenFileChannel(NULL, DEV_NULL, "w", 0);
        if (chan != NULL) {
            Tcl_SetChannelOption(NULL, chan, "-encoding", "utf-8");
        }
        Tcl_SetStdChannel(chan, TCL_STDOUT);
    }
    chan = Tcl_GetStdChannel(TCL_STDERR);
    if (chan == NULL) {
        chan = Tcl_OpenFileChannel(NULL, DEV_NULL, "w", 0);
        if (chan != NULL) {
            Tcl_SetChannelOption(NULL, chan, "-encoding", "utf-8");
        }
        Tcl_SetStdChannel(chan, TCL_STDERR);
    }
}
