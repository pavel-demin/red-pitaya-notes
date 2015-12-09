/*
 * bltWindow.c --
 *
 *	This module implements additional window functionality for
 *	the BLT toolkit, such as transparent Tk windows,
 *	and reparenting Tk windows.
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

#include <X11/Xlib.h>
#ifndef WIN32
#include <X11/Xproto.h>
#endif

#include <tkInt.h>

#ifdef XNQueryInputStyle
#define TK_USE_INPUT_METHODS
#endif

/*
 * This defines whether we should try to use XIM over-the-spot style
 * input.  Allow users to override it.  It is a much more elegant use
 * of XIM, but uses a bit more memory.
 */
#ifndef TK_XIM_SPOT
#   define TK_XIM_SPOT	1
#endif

#ifndef TK_REPARENTED
#define TK_REPARENTED 	0
#endif

#ifdef WIN32
/*
 *----------------------------------------------------------------------
 *
 * GetWindowHandle --
 *
 *      Returns the XID for the Tk_Window given.  Starting in Tk 8.0,
 *      the toplevel widgets are wrapped by another window.
 *      Currently there's no way to get at that window, other than
 *      what is done here: query the X window hierarchy and grab the
 *      parent.
 *
 * Results:
 *      Returns the X Window ID of the widget.  If it's a toplevel, then
 *	the XID of the wrapper is returned.
 *
 *----------------------------------------------------------------------
 */
static HWND
GetWindowHandle(Tk_Window tkwin)
{
    HWND hWnd;
    Window window;
    
    window = Tk_WindowId(tkwin);
    if (window == None) {
	Tk_MakeWindowExist(tkwin);
    }
    hWnd = Tk_GetHWND(Tk_WindowId(tkwin));
#if (TK_MAJOR_VERSION > 4)
    if (Tk_IsTopLevel(tkwin)) {
	hWnd = GetParent(hWnd);
    }
#endif /* TK_MAJOR_VERSION > 4 */
    return hWnd;
}

Window
Blt_GetParent(display, window)
    Display *display;
    Window window;
{
    HWND hWnd;
    hWnd = GetWindowHandle(window);
    return (Window)hWnd;
}

#else

Window
Blt_GetParent(display, window)
    Display *display;
    Window window;
{
    Window root, parent;
    Window *dummy;
    unsigned int count;

    if (XQueryTree(display, window, &root, &parent, &dummy, &count) > 0) {
	XFree(dummy);
	return parent;
    }
    return None;
}

static Window
GetWindowId(tkwin)
    Tk_Window tkwin;
{
    Window window;

    Tk_MakeWindowExist(tkwin);
    window = Tk_WindowId(tkwin);
#if (TK_MAJOR_VERSION > 4)
    if (Tk_IsTopLevel(tkwin)) {
	Window parent;

	parent = Blt_GetParent(Tk_Display(tkwin), window);
        if (parent != None && parent != XRootWindow(Tk_Display(tkwin), Tk_ScreenNumber(tkwin))) {
	    window = parent;
	}
	/*window = parent; */
    }
#endif /* TK_MAJOR_VERSION > 4 */
    return window;
}

#endif /* WIN32 */

/*
 *----------------------------------------------------------------------
 *
 * DoConfigureNotify --
 *
 *	Generate a ConfigureNotify event describing the current
 *	configuration of a window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An event is generated and processed by Tk_HandleEvent.
 *
 *----------------------------------------------------------------------
 */
static void
DoConfigureNotify(winPtr)
    Tk_FakeWin *winPtr;		/* Window whose configuration was just
				 * changed. */
{
    XEvent event;

    event.type = ConfigureNotify;
    event.xconfigure.serial = LastKnownRequestProcessed(winPtr->display);
    event.xconfigure.send_event = False;
    event.xconfigure.display = winPtr->display;
    event.xconfigure.event = winPtr->window;
    event.xconfigure.window = winPtr->window;
    event.xconfigure.x = winPtr->changes.x;
    event.xconfigure.y = winPtr->changes.y;
    event.xconfigure.width = winPtr->changes.width;
    event.xconfigure.height = winPtr->changes.height;
    event.xconfigure.border_width = winPtr->changes.border_width;
    if (winPtr->changes.stack_mode == Above) {
	event.xconfigure.above = winPtr->changes.sibling;
    } else {
	event.xconfigure.above = None;
    }
    event.xconfigure.override_redirect = winPtr->atts.override_redirect;
    Tk_HandleEvent(&event);
}

/*
 *--------------------------------------------------------------
 *
 * Blt_MakeTransparentWindowExist --
 *
 *	Similar to Tk_MakeWindowExist but instead creates a
 *	transparent window to block for user events from sibling
 *	windows.
 *
 *	Differences from Tk_MakeWindowExist.
 *
 *	1. This is always a "busy" window. There's never a
 *	   platform-specific class procedure to execute instead.
 *	2. The window is transparent and never will contain children,
 *	   so colormap information is irrelevant.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the procedure returns, the internal window associated
 *	with tkwin is guaranteed to exist.  This may require the
 *	window's ancestors to be created too.
 *
 *--------------------------------------------------------------
 */
void
Blt_MakeTransparentWindowExist(tkwin, parent, isBusy)
    Tk_Window tkwin;		/* Token for window. */
    Window parent;		/* Parent window. */
    int isBusy;			/*  */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    TkWindow *winPtr2;
    Tcl_HashEntry *hPtr;
    int notUsed;
    TkDisplay *dispPtr;
#ifdef WIN32
    HWND hParent;
    int style;
    DWORD exStyle;
    HWND hWnd;
#else
    long int mask;
#endif /* WIN32 */

    if (winPtr->window != None) {
	return;			/* Window already exists. */
    }
#ifdef notdef			
    if ((winPtr->parentPtr == NULL) || (winPtr->flags & TK_TOP_LEVEL)) {
	parent = XRootWindow(winPtr->display, winPtr->screenNum);
	/* TODO: Make the entire screen busy */
    } else {
	if (Tk_WindowId(winPtr->parentPtr) == None) {
	    Tk_MakeWindowExist((Tk_Window)winPtr->parentPtr);
	}
    }
#endif

    /* Create a transparent window and put it on top.  */

#ifdef WIN32
    hParent = (HWND) parent;
    style = (WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    exStyle = (WS_EX_TRANSPARENT | WS_EX_TOPMOST);
#define TK_WIN_CHILD_CLASS_NAME "TkChild"
    hWnd = CreateWindowEx(exStyle, TK_WIN_CHILD_CLASS_NAME, NULL, style,
	Tk_X(tkwin), Tk_Y(tkwin), Tk_Width(tkwin), Tk_Height(tkwin),
	hParent, NULL, (HINSTANCE) Tk_GetHINSTANCE(), NULL);
    winPtr->window = Tk_AttachHWND(tkwin, hWnd);
#else
    mask = (!isBusy) ? 0 : (CWDontPropagate | CWEventMask);
    /* Ignore the important events while the window is mapped.  */
#define USER_EVENTS  (EnterWindowMask | LeaveWindowMask | KeyPressMask | \
	KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask)
#define PROP_EVENTS  (KeyPressMask | KeyReleaseMask | ButtonPressMask | \
	ButtonReleaseMask | PointerMotionMask)

    winPtr->atts.do_not_propagate_mask = PROP_EVENTS;
    winPtr->atts.event_mask = USER_EVENTS;
    winPtr->changes.border_width = 0;
    winPtr->depth = 0; 

    winPtr->window = XCreateWindow(winPtr->display, parent,
	winPtr->changes.x, winPtr->changes.y,
	(unsigned)winPtr->changes.width,	/* width */
	(unsigned)winPtr->changes.height,	/* height */
	(unsigned)winPtr->changes.border_width,	/* border_width */
	winPtr->depth,		/* depth */
	InputOnly,		/* class */
	winPtr->visual,		/* visual */
        mask,			/* valuemask */
	&(winPtr->atts)		/* attributes */ );
#endif /* WIN32 */

    dispPtr = winPtr->dispPtr;
    hPtr = Tcl_CreateHashEntry(&(dispPtr->winTable), (char *)winPtr->window,
	&notUsed);
    Tcl_SetHashValue(hPtr, winPtr);
    winPtr->dirtyAtts = 0;
    winPtr->dirtyChanges = 0;
#ifdef TK_USE_INPUT_METHODS
    winPtr->inputContext = NULL;
#endif /* TK_USE_INPUT_METHODS */
    if (!(winPtr->flags & TK_TOP_LEVEL)) {
	/*
	 * If any siblings higher up in the stacking order have already
	 * been created then move this window to its rightful position
	 * in the stacking order.
	 *
	 * NOTE: this code ignores any changes anyone might have made
	 * to the sibling and stack_mode field of the window's attributes,
	 * so it really isn't safe for these to be manipulated except
	 * by calling Tk_RestackWindow.
	 */
	for (winPtr2 = winPtr->nextPtr; winPtr2 != NULL;
	    winPtr2 = winPtr2->nextPtr) {
	    if ((winPtr2->window != None) && !(winPtr2->flags & TK_TOP_LEVEL)) {
		XWindowChanges changes;
		changes.sibling = winPtr2->window;
		changes.stack_mode = Below;
		XConfigureWindow(winPtr->display, winPtr->window,
		    CWSibling | CWStackMode, &changes);
		break;
	    }
	}
    }

    /*
     * Issue a ConfigureNotify event if there were deferred configuration
     * changes (but skip it if the window is being deleted;  the
     * ConfigureNotify event could cause problems if we're being called
     * from Tk_DestroyWindow under some conditions).
     */
    if ((winPtr->flags & TK_NEED_CONFIG_NOTIFY)
	&& !(winPtr->flags & TK_ALREADY_DEAD)) {
	winPtr->flags &= ~TK_NEED_CONFIG_NOTIFY;
	DoConfigureNotify((Tk_FakeWin *) tkwin);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_FindChild --
 *
 *      Performs a linear search for the named child window in a given
 *	parent window.
 *
 *	This can be done via Tcl, but not through Tk's C API.  It's 
 *	simple enough, if you peek into the Tk_Window structure.
 *
 * Results:
 *      The child Tk_Window. If the named child can't be found, NULL
 *	is returned.
 *
 *----------------------------------------------------------------------
 */

/*LINTLIBRARY*/
Tk_Window
Blt_FindChild(parent, name)
    Tk_Window parent;
    char *name;
{
    register TkWindow *winPtr;
    TkWindow *parentPtr = (TkWindow *)parent;

    for (winPtr = parentPtr->childList; winPtr != NULL; 
	winPtr = winPtr->nextPtr) {
	if (strcmp(name, winPtr->nameUid) == 0) {
	    return (Tk_Window)winPtr;
	}
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_FirstChildWindow --
 *
 *      Performs a linear search for the named child window in a given
 *	parent window.
 *
 *	This can be done via Tcl, but not through Tk's C API.  It's 
 *	simple enough, if you peek into the Tk_Window structure.
 *
 * Results:
 *      The child Tk_Window. If the named child can't be found, NULL
 *	is returned.
 *
 *----------------------------------------------------------------------
 */
/*LINTLIBRARY*/
Tk_Window
Blt_FirstChild(parent)
    Tk_Window parent;
{
    TkWindow *parentPtr = (TkWindow *)parent;
    return (Tk_Window)parentPtr->childList;
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_FindChild --
 *
 *      Performs a linear search for the named child window in a given
 *	parent window.
 *
 *	This can be done via Tcl, but not through Tk's C API.  It's 
 *	simple enough, if you peek into the Tk_Window structure.
 *
 * Results:
 *      The child Tk_Window. If the named child can't be found, NULL
 *	is returned.
 *
 *----------------------------------------------------------------------
 */

/*LINTLIBRARY*/
Tk_Window
Blt_NextChild(tkwin)
    Tk_Window tkwin;
{
    TkWindow *winPtr = (TkWindow *)tkwin;

    if (winPtr == NULL) {
	return NULL;
    }
    return (Tk_Window)winPtr->nextPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * UnlinkWindow --
 *
 *	This procedure removes a window from the childList of its
 *	parent.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window is unlinked from its childList.
 *
 *----------------------------------------------------------------------
 */
static void
UnlinkWindow(winPtr)
    TkWindow *winPtr;	/* Child window to be unlinked. */
{
    TkWindow *prevPtr;

    prevPtr = winPtr->parentPtr->childList;
    if (prevPtr == winPtr) {
	winPtr->parentPtr->childList = winPtr->nextPtr;
	if (winPtr->nextPtr == NULL) {
	    winPtr->parentPtr->lastChildPtr = NULL;
	}
    } else {
	while (prevPtr->nextPtr != winPtr) {
	    prevPtr = prevPtr->nextPtr;
	    if (prevPtr == NULL) {
		panic("UnlinkWindow couldn't find child in parent");
	    }
	}
	prevPtr->nextPtr = winPtr->nextPtr;
	if (winPtr->nextPtr == NULL) {
	    winPtr->parentPtr->lastChildPtr = prevPtr;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_RelinkWindow --
 *
 *	Relinks a window into a new parent.  The window is unlinked
 *	from its original parent's child list and added onto the end
 *	of the new parent's list.
 *
 *	FIXME:  If the window has focus, the focus should be moved
 *		to an ancestor.  Otherwise, Tk becomes confused 
 *		about which Toplevel turns on focus for the window. 
 *		Right now this is done at the Tcl layer.  For example,
 *		see blt::CreateTearoff in tabset.tcl.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window is unlinked from its childList.
 *
 *----------------------------------------------------------------------
 */
void
Blt_RelinkWindow(tkwin, newParent, x, y)
    Tk_Window tkwin;		/* Child window to be linked. */
    Tk_Window newParent;
    int x, y;
{
    TkWindow *winPtr, *parentWinPtr;

    if (Blt_ReparentWindow(Tk_Display(tkwin), Tk_WindowId(tkwin),
	    Tk_WindowId(newParent), x, y) != TCL_OK) {
	return;
    }
    winPtr = (TkWindow *)tkwin;
    parentWinPtr = (TkWindow *)newParent;

    winPtr->flags &= ~TK_REPARENTED;
    UnlinkWindow(winPtr);	/* Remove the window from its parent's list */

    /* Append the window onto the end of the parent's list of children */
    winPtr->parentPtr = parentWinPtr;
    winPtr->nextPtr = NULL;
    if (parentWinPtr->childList == NULL) {
	parentWinPtr->childList = winPtr;
    } else {
	parentWinPtr->lastChildPtr->nextPtr = winPtr;
    }
    parentWinPtr->lastChildPtr = winPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_RelinkWindow --
 *
 *	Relinks a window into a new parent.  The window is unlinked
 *	from its original parent's child list and added onto the end
 *	of the new parent's list.
 *
 *	FIXME:  If the window has focus, the focus should be moved
 *		to an ancestor.  Otherwise, Tk becomes confused 
 *		about which Toplevel turns on focus for the window. 
 *		Right now this is done at the Tcl layer.  For example,
 *		see blt::CreateTearoff in tabset.tcl.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window is unlinked from its childList.
 *
 *----------------------------------------------------------------------
 */
void
Blt_RelinkWindow2(tkwin, window, newParent, x, y)
    Tk_Window tkwin;		/* Child window to be linked. */
    Window window;
    Tk_Window newParent;
    int x, y;
{
#ifdef notdef
    TkWindow *winPtr, *parentWinPtr;
#endif
    if (Blt_ReparentWindow(Tk_Display(tkwin), window,
	    Tk_WindowId(newParent), x, y) != TCL_OK) {
	return;
    }
#ifdef notdef
    winPtr = (TkWindow *)tkwin;
    parentWinPtr = (TkWindow *)newParent;

    winPtr->flags &= ~TK_REPARENTED;
    UnlinkWindow(winPtr);	/* Remove the window from its parent's list */

    /* Append the window onto the end of the parent's list of children */
    winPtr->parentPtr = parentWinPtr;
    winPtr->nextPtr = NULL;
    if (parentWinPtr->childList == NULL) {
	parentWinPtr->childList = winPtr;
    } else {
	parentWinPtr->lastChildPtr->nextPtr = winPtr;
    }
    parentWinPtr->lastChildPtr = winPtr;
#endif
}

void
Blt_UnlinkWindow(tkwin)
    Tk_Window tkwin;		/* Child window to be linked. */
{
    TkWindow *winPtr;
    Window root;

    root = XRootWindow(Tk_Display(tkwin), Tk_ScreenNumber(tkwin));
    if (Blt_ReparentWindow(Tk_Display(tkwin), Tk_WindowId(tkwin),
	    root, 0, 0) != TCL_OK) {
	return;
    }
    winPtr = (TkWindow *)tkwin;
    winPtr->flags &= ~TK_REPARENTED;
#ifdef notdef
    UnlinkWindow(winPtr);	/* Remove the window from its parent's list */
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_Toplevel --
 *
 *      Climbs up the widget hierarchy to find the top level window of
 *      the window given.
 *
 * Results:
 *      Returns the Tk_Window of the toplevel widget.
 *
 *----------------------------------------------------------------------
 */
Tk_Window
Blt_Toplevel(tkwin)
    register Tk_Window tkwin;
{
    while (!Tk_IsTopLevel(tkwin)) {
	tkwin = Tk_Parent(tkwin);
    }
    return tkwin;
}

void
Blt_RootCoordinates(tkwin, x, y, rootXPtr, rootYPtr)
    Tk_Window tkwin;
    int x, y;
    int *rootXPtr, *rootYPtr;
{
    int vx, vy, vw, vh;
    int rootX, rootY;

    Tk_GetRootCoords(tkwin, &rootX, &rootY);
    x += rootX;
    y += rootY;
    Tk_GetVRootGeometry(tkwin, &vx, &vy, &vw, &vh);
    x += vx;
    y += vy;
    *rootXPtr = x;
    *rootYPtr = y;
}


/* Find the toplevel then  */
int
Blt_RootX(tkwin)
    Tk_Window tkwin;
{
    int x;
    
    for (x = 0; tkwin != NULL;  tkwin = Tk_Parent(tkwin)) {
	x += Tk_X(tkwin) + Tk_Changes(tkwin)->border_width;
	if (Tk_IsTopLevel(tkwin)) {
	    break;
	}
    }
    return x;
}

int
Blt_RootY(tkwin)
    Tk_Window tkwin;
{
    int y;
    
    for (y = 0; tkwin != NULL;  tkwin = Tk_Parent(tkwin)) {
	y += Tk_Y(tkwin) + Tk_Changes(tkwin)->border_width;
	if (Tk_IsTopLevel(tkwin)) {
	    break;
	}
    }
    return y;
}

#ifdef WIN32
/*
 *----------------------------------------------------------------------
 *
 * Blt_GetRealWindowId --
 *
 *      Returns the XID for the Tk_Window given.  Starting in Tk 8.0,
 *      the toplevel widgets are wrapped by another window.
 *      Currently there's no way to get at that window, other than
 *      what is done here: query the X window hierarchy and grab the
 *      parent.
 *
 * Results:
 *      Returns the X Window ID of the widget.  If it's a toplevel, then
 *	the XID of the wrapper is returned.
 *
 *----------------------------------------------------------------------
 */
Window
Blt_GetRealWindowId(Tk_Window tkwin)
{
    return (Window) GetWindowHandle(tkwin);
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_GetToplevel --
 *
 *      Retrieves the toplevel window which is the nearest ancestor of
 *      of the specified window.
 *
 * Results:
 *      Returns the toplevel window or NULL if the window has no
 *      ancestor which is a toplevel.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
Tk_Window
Blt_GetToplevel(Tk_Window tkwin) /* Window for which the toplevel
				  * should be deterined. */
{
     while (!Tk_IsTopLevel(tkwin)) {
         tkwin = Tk_Parent(tkwin);
	 if (tkwin == NULL) {
             return NULL;
         }
     }
     return tkwin;
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_RaiseToLevelWindow --
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Blt_RaiseToplevel(Tk_Window tkwin)
{
    SetWindowPos(GetWindowHandle(tkwin), HWND_TOP, 0, 0, 0, 0,
	SWP_NOMOVE | SWP_NOSIZE);
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_MapToplevel --
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Blt_MapToplevel(Tk_Window tkwin)
{
    ShowWindow(GetWindowHandle(tkwin), SW_SHOWNORMAL);
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_UnmapToplevel --
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Blt_UnmapToplevel(Tk_Window tkwin)
{
    ShowWindow(GetWindowHandle(tkwin), SW_HIDE);
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_MoveResizeToplevel --
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Blt_MoveResizeToplevel(tkwin, x, y, width, height)
    Tk_Window tkwin;
    int x, y, width, height;
{
    SetWindowPos(GetWindowHandle(tkwin), HWND_TOP, x, y, width, height, 0);
}

int
Blt_ReparentWindow(
    Display *display,
    Window window, 
    Window newParent,
    int x, int y)
{
    XReparentWindow(display, window, newParent, x, y);
    return TCL_OK;
}

#else  /* WIN32 */

/*
 *----------------------------------------------------------------------
 *
 * Blt_GetRealWindowId --
 *
 *      Returns the XID for the Tk_Window given.  Starting in Tk 8.0,
 *      the toplevel widgets are wrapped by another window.
 *      Currently there's no way to get at that window, other than
 *      what is done here: query the X window hierarchy and grab the
 *      parent.
 *
 * Results:
 *      Returns the X Window ID of the widget.  If it's a toplevel, then
 *	the XID of the wrapper is returned.
 *
 *----------------------------------------------------------------------
 */
Window
Blt_GetRealWindowId(tkwin)
    Tk_Window tkwin;
{
    return GetWindowId(tkwin);
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_RaiseToplevel --
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Blt_RaiseToplevel(tkwin)
    Tk_Window tkwin;
{
    XRaiseWindow(Tk_Display(tkwin), GetWindowId(tkwin));
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_LowerToplevel --
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Blt_LowerToplevel(tkwin)
    Tk_Window tkwin;
{
    XLowerWindow(Tk_Display(tkwin), GetWindowId(tkwin));
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_ResizeToplevel --
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Blt_ResizeToplevel(tkwin, width, height)
    Tk_Window tkwin;
    int width, height;
{
    XResizeWindow(Tk_Display(tkwin), GetWindowId(tkwin), width, height);
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_MoveResizeToplevel --
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Blt_MoveResizeToplevel(tkwin, x, y, width, height)
    Tk_Window tkwin;
    int x, y, width, height;
{
    XMoveResizeWindow(Tk_Display(tkwin), GetWindowId(tkwin), x, y, 
	      width, height);
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_ResizeToplevel --
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Blt_MoveToplevel(tkwin, x, y)
    Tk_Window tkwin;
    int x, y;
{
    XMoveWindow(Tk_Display(tkwin), GetWindowId(tkwin), x, y);
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_MapToplevel --
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Blt_MapToplevel(tkwin)
    Tk_Window tkwin;
{
    XMapWindow(Tk_Display(tkwin), GetWindowId(tkwin));
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_UnmapToplevel --
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
Blt_UnmapToplevel(tkwin)
    Tk_Window tkwin;
{
    XUnmapWindow(Tk_Display(tkwin), GetWindowId(tkwin));
}

/* ARGSUSED */
static int
XReparentWindowErrorProc(clientData, errEventPtr)
    ClientData clientData;
    XErrorEvent *errEventPtr;
{
    int *errorPtr = clientData;

    *errorPtr = TCL_ERROR;
    return 0;
}

int
Blt_ReparentWindow(display, window, newParent, x, y)
    Display *display;
    Window window, newParent;
    int x, y;
{
    Tk_ErrorHandler handler;
    int result;
    int any = -1;

    result = TCL_OK;
    handler = Tk_CreateErrorHandler(display, any, X_ReparentWindow, any,
	XReparentWindowErrorProc, &result);
    XReparentWindow(display, window, newParent, x, y);
    Tk_DeleteErrorHandler(handler);
    XSync(display, False);
    return result;
}

#endif /* WIN32 */

#if (TK_MAJOR_VERSION == 4)
#include <bltHash.h>
static int initialized = FALSE;
static Blt_HashTable windowTable;

void
Blt_SetWindowInstanceData(tkwin, instanceData)
    Tk_Window tkwin;
    ClientData instanceData;
{
    Blt_HashEntry *hPtr;
    int isNew;

    if (!initialized) {
	Blt_InitHashTable(&windowTable, BLT_ONE_WORD_KEYS);
	initialized = TRUE;
    }
    hPtr = Blt_CreateHashEntry(&windowTable, (char *)tkwin, &isNew);
    assert(isNew);
    Blt_SetHashValue(hPtr, instanceData);
}

ClientData
Blt_GetWindowInstanceData(tkwin)
    Tk_Window tkwin;
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&windowTable, (char *)tkwin);
    if (hPtr == NULL) {
	return NULL;
    }
    return Blt_GetHashValue(hPtr);
}

void
Blt_DeleteWindowInstanceData(tkwin)
    Tk_Window tkwin;
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&windowTable, (char *)tkwin);
    assert(hPtr);
    Blt_DeleteHashEntry(&windowTable, hPtr);
}

#else

void
Blt_SetWindowInstanceData(tkwin, instanceData)
    Tk_Window tkwin;
    ClientData instanceData;
{
    TkWindow *winPtr = (TkWindow *)tkwin;

    winPtr->instanceData = instanceData;
}

ClientData
Blt_GetWindowInstanceData(tkwin)
    Tk_Window tkwin;
{
    TkWindow *winPtr = (TkWindow *)tkwin;

    return winPtr->instanceData;
}

void
Blt_DeleteWindowInstanceData(tkwin)
    Tk_Window tkwin;
{
}

#endif

