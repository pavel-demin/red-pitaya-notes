
/*
 * bltTreeViewColumn.c --
 *
 *	This module implements an hierarchy widget for the BLT toolkit.
 *
 * Copyright 1998-1999 Lucent Technologies, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and warranty
 * disclaimer appear in supporting documentation, and that the names
 * of Lucent Technologies or any of their entities not be used in
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
 *
 *	The "treeview" widget was created by George A. Howlett.
 *      Extensive cleanups and enhancements by Peter MacDonald.
 */

/*
 * TODO:
 *
 * BUGS:
 *   1.  "open" operation should change scroll offset so that as many
 *	 new entries (up to half a screen) can be seen.
 *   2.  "open" needs to adjust the scrolloffset so that the same entry
 *	 is seen at the same place.
 */
#include "bltInt.h"

#ifndef NO_TREEVIEW

#include "bltTreeView.h"
#include <X11/Xutil.h>

#define RULE_AREA		(8)

static Blt_OptionParseProc ObjToColumn;
static Blt_OptionPrintProc ColumnToObj;
static Blt_OptionParseProc ObjToData;
static Blt_OptionPrintProc DataToObj;
static Blt_OptionParseProc ObjToColorPat;
static Blt_OptionPrintProc ColorPatToObj;

static char *sortTypeStrings[] = {
    "dictionary", "ascii", "integer", "real", "command", "none", NULL
};

enum SortTypeValues { 
    SORT_TYPE_DICTIONARY, SORT_TYPE_ASCII, SORT_TYPE_INTEGER, 
    SORT_TYPE_REAL, SORT_TYPE_COMMAND, SORT_TYPE_NONE
};

#define DEF_SORT_COLUMN		(char *)NULL
#define DEF_SORT_COMMAND	(char *)NULL
#define DEF_SORT_DECREASING	"0"
#define DEF_SORT_SETFLAT	"0"
#define DEF_SORT_TYPE		"dictionary"

#ifdef WIN32
#define DEF_COLUMN_ACTIVE_TITLE_BG	RGB_GREY85
#else
#define DEF_COLUMN_ACTIVE_TITLE_BG	RGB_GREY90
#endif
#define DEF_COLUMN_AUTOWIDTH			"0"
#define DEF_COLUMN_ACTIVE_TITLE_FG	STD_ACTIVE_FOREGROUND
#define DEF_COLUMN_BIND_TAGS		"all"
#define DEF_COLUMN_BORDERWIDTH		STD_BORDERWIDTH
#define DEF_COLUMN_COLOR		RGB_BLACK
#define DEF_COLUMN_EDIT			"no"
#define DEF_COLUMN_FONT			STD_FONT
#define DEF_COLUMN_HIDE			"no"
#define DEF_COLUMN_JUSTIFY		"left"
#define DEF_COLUMN_MAX			"0"
#define DEF_COLUMN_MIN			"0"
#define DEF_COLUMN_PAD			"2"
#define DEF_COLUMN_RELIEF		"flat"
#define DEF_COLUMN_STATE		"normal"
#define DEF_COLUMN_STYLE		"text"
#define DEF_COLUMN_TITLE_BACKGROUND	STD_NORMAL_BACKGROUND
#define DEF_COLUMN_TITLE_BORDERWIDTH	STD_BORDERWIDTH
#define DEF_COLUMN_TITLE_FOREGROUND	STD_NORMAL_FOREGROUND
#define DEF_COLUMN_TITLE_RELIEF		"raised"
#define DEF_COLUMN_UNDERLINE		"-1"
#define DEF_COLUMN_WEIGHT		"1.0"
#define DEF_COLUMN_WIDTH		"0"
#define DEF_COLUMN_RULE_DASHES		"dot"
#define DEF_COLUMN_SCROLL_TILE          "no"
#define DEF_COLUMN_TITLEJUSTIFY		"center"

extern Blt_OptionParseProc Blt_ObjToEnum;
extern Blt_OptionPrintProc Blt_EnumToObj;

static Blt_CustomOption patColorOption =
{
    ObjToColorPat, ColorPatToObj, NULL, (ClientData)0
};

static Blt_CustomOption regColorOption =
{
    ObjToColorPat, ColorPatToObj, NULL, (ClientData)1
};

static Blt_CustomOption typeOption =
{
    Blt_ObjToEnum, Blt_EnumToObj, NULL, (ClientData)sortTypeStrings
};

Blt_CustomOption bltTreeViewColumnOption =
{
    ObjToColumn, ColumnToObj, NULL, (ClientData)0
};

Blt_CustomOption bltTreeViewDataOption =
{
    ObjToData, DataToObj, NULL, (ClientData)0,
};

static Blt_OptionParseProc ObjToStyle;
static Blt_OptionPrintProc StyleToObj;
static Blt_OptionFreeProc FreeStyle;
Blt_CustomOption bltTreeViewStyleOption =
{
    /* Contains a pointer to the widget that's currently being
     * configured.  This is used in the custom configuration parse
     * routine for icons.  */
    ObjToStyle, StyleToObj, FreeStyle, NULL,
};

static Blt_OptionParseProc ObjToStyles;
static Blt_OptionPrintProc StylesToObj;
static Blt_OptionFreeProc FreeStyles;
Blt_CustomOption bltTreeViewStylesOption =
{
    /* Contains a pointer to the widget that's currently being
    * configured.  This is used in the custom configuration parse
    * routine for icons.  */
    ObjToStyles, StylesToObj, FreeStyles, NULL,
};

static Blt_TreeApplyProc SortApplyProc;

static Blt_ConfigSpec columnSpecs[] =
{
    {BLT_CONFIG_BORDER, "-activetitlebackground", "activeTitleBackground", 
	"Background", DEF_COLUMN_ACTIVE_TITLE_BG, 
	Blt_Offset(TreeViewColumn, activeTitleBorder), 0},
    {BLT_CONFIG_COLOR, "-activetitleforeground", "activeTitleForeground", 
	"Foreground", DEF_COLUMN_ACTIVE_TITLE_FG, 
	Blt_Offset(TreeViewColumn, activeTitleFgColor), 0},
    {BLT_CONFIG_DISTANCE, "-autowidth",(char *)NULL, (char *)NULL,
	DEF_COLUMN_AUTOWIDTH, Blt_Offset(TreeViewColumn, autoWidth), 
        0},
    {BLT_CONFIG_BORDER, "-background", (char *)NULL, (char *)NULL,
	(char *)NULL, Blt_Offset(TreeViewColumn, border), 
        BLT_CONFIG_NULL_OK|BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-bd", (char *)NULL, (char *)NULL, (char *)NULL, 
	0, 0, (ClientData)"-borderwidth"},
    {BLT_CONFIG_SYNONYM, "-bg", (char *)NULL, (char *)NULL, (char *)NULL, 
	0, 0, (ClientData)"-background"},
    {BLT_CONFIG_CUSTOM, "-bindtags", "bindTags", "BindTags",
	DEF_COLUMN_BIND_TAGS, Blt_Offset(TreeViewColumn, tagsUid),
	BLT_CONFIG_NULL_OK, &bltTreeViewUidOption},
    {BLT_CONFIG_DISTANCE, "-borderwidth", (char *)NULL, (char *)NULL,
	DEF_COLUMN_BORDERWIDTH, Blt_Offset(TreeViewColumn, borderWidth),
	0},
    {BLT_CONFIG_CUSTOM, "-colorpattern", "colorPattern", "ColorPattern",
	NULL, Blt_Offset(TreeViewColumn, colorPats),
        BLT_CONFIG_NULL_OK, &patColorOption},
    {BLT_CONFIG_CUSTOM, "-colorregex", "colorRegex", "ColorRegex",
	NULL, Blt_Offset(TreeViewColumn, colorRegex),
        BLT_CONFIG_NULL_OK, &regColorOption},
    {BLT_CONFIG_STRING, "-command",  (char *)NULL, (char *)NULL, 
	(char *)NULL, Blt_Offset(TreeViewColumn, titleCmd),
	BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK}, 
    {BLT_CONFIG_ARROW, "-titlearrow", "titleArrow", "TitleArrow",
	"none", Blt_Offset(TreeViewColumn, drawArrow), 
        0},
    {BLT_CONFIG_BOOLEAN, "-edit", "edit", "Edit",
	DEF_COLUMN_EDIT, Blt_Offset(TreeViewColumn, editable), 
        0},
    {BLT_CONFIG_STRING, "-editopts", "editOpts", "EditOpts",
	(char *)NULL, Blt_Offset(TreeViewColumn, editOpts), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-fg", (char *)NULL, (char *)NULL, (char *)NULL, 0, 0,
        (ClientData)"-foreground"},
    {BLT_CONFIG_OBJCMD, "-fillcmd", "fillCmd", "FillCmd",
	(char *)NULL, Blt_Offset(TreeViewColumn, fillCmd), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FONT, "-font", (char *)NULL, (char *)NULL,
	(char *)NULL, Blt_Offset(TreeViewColumn, font),
	BLT_CONFIG_NULL_OK|BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_COLOR, "-foreground", (char *)NULL, (char *)NULL,
	(char *)NULL, Blt_Offset(TreeViewColumn, fgColor), 
        BLT_CONFIG_NULL_OK|BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJCMD, "-formatcmd", "formatCmd", "FormatCmd",
	NULL, Blt_Offset(TreeViewColumn, formatCmd), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BOOLEAN, "-hide", "hide", "Hide",
	DEF_COLUMN_HIDE, Blt_Offset(TreeViewColumn, hidden),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-icon", "icon", "icon",
	(char *)NULL, Blt_Offset(TreeViewColumn, titleIcon),
         BLT_CONFIG_DONT_SET_DEFAULT|BLT_CONFIG_NULL_OK,
         &bltTreeViewIconOption},
    {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify",
	DEF_COLUMN_JUSTIFY, Blt_Offset(TreeViewColumn, justify), 
        0},
    {BLT_CONFIG_DISTANCE, "-max", "max", "Max",
	DEF_COLUMN_MAX, Blt_Offset(TreeViewColumn, reqMax), 
	0},
    {BLT_CONFIG_DISTANCE, "-min", "min", "Min",
	DEF_COLUMN_MIN, Blt_Offset(TreeViewColumn, reqMin), 
	0},
    {BLT_CONFIG_PAD, "-pad", "pad", "Pad",
	DEF_COLUMN_PAD, Blt_Offset(TreeViewColumn, pad), 
	0},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief",
	DEF_COLUMN_RELIEF, Blt_Offset(TreeViewColumn, relief), 
        0},
    {BLT_CONFIG_DASHES, "-ruledashes", "ruleDashes", "RuleDashes",
	DEF_COLUMN_RULE_DASHES, Blt_Offset(TreeViewColumn, ruleDashes),
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BOOLEAN, "-scrolltile", "scrollTile", "ScrollTile",
	DEF_COLUMN_SCROLL_TILE, Tk_Offset(TreeViewColumn, scrollTile),
	0},
    {BLT_CONFIG_STRING, "-sortcommand", "sortCommand", "SortCommand",
	DEF_SORT_COMMAND, Blt_Offset(TreeViewColumn, sortCmd), 
	BLT_CONFIG_NULL_OK}, 
    {BLT_CONFIG_LISTOBJ, "-sortaltcolumns", "sortAltColumns", "SortAltColumns",
	DEF_SORT_COMMAND, Blt_Offset(TreeViewColumn, sortAltColumns), 
	BLT_CONFIG_NULL_OK}, 
    {BLT_CONFIG_CUSTOM, "-sortmode", "sortMode", "SortMode",
	DEF_SORT_TYPE, Blt_Offset(TreeViewColumn, sortType), 0, &typeOption},
    {BLT_CONFIG_STATE, "-state", "state", "State",
	DEF_COLUMN_STATE, Blt_Offset(TreeViewColumn, state), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-style", "style", "Style",
	DEF_COLUMN_STYLE, Blt_Offset(TreeViewColumn, stylePtr), 
        0, &bltTreeViewStyleOption},
    {BLT_CONFIG_TILE, "-tile", "columnTile", "ColumnTile",
	(char *)NULL, Tk_Offset(TreeViewColumn, tile), BLT_CONFIG_NULL_OK, },
    {BLT_CONFIG_STRING, "-title", "title", "Title",
	(char *)NULL, Blt_Offset(TreeViewColumn, title), 0},
    {BLT_CONFIG_BORDER, "-titlebackground", "titleBackground", 
	"TitleBackground", DEF_COLUMN_TITLE_BACKGROUND, 
	Blt_Offset(TreeViewColumn, titleBorder),0},
    {BLT_CONFIG_DISTANCE, "-titleborderwidth", "BorderWidth", 
	"TitleBorderWidth", DEF_COLUMN_TITLE_BORDERWIDTH, 
	Blt_Offset(TreeViewColumn, titleBorderWidth),
	0},
    {BLT_CONFIG_FONT, "-titlefont", (char *)NULL, (char *)NULL,
	(char *)NULL, Blt_Offset(TreeViewColumn, titleFont),
	BLT_CONFIG_NULL_OK|BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_COLOR, "-titleforeground", "titleForeground", "TitleForeground",
	DEF_COLUMN_TITLE_FOREGROUND, 
	Blt_Offset(TreeViewColumn, titleFgColor), 0},
    {BLT_CONFIG_JUSTIFY, "-titlejustify", "titleJustify", "TitleJustify",
	DEF_COLUMN_TITLEJUSTIFY, Blt_Offset(TreeViewColumn, titleJustify), 
        0},
    {BLT_CONFIG_RELIEF, "-titlerelief", "titleRelief", "TitleRelief",
	DEF_COLUMN_TITLE_RELIEF, Blt_Offset(TreeViewColumn, titleRelief), 
        0},
    {BLT_CONFIG_SHADOW, "-titleshadow", "titleShadow", "TitleShadow",
	(char *)NULL, Blt_Offset(TreeViewColumn, titleShadow), 0},
    {BLT_CONFIG_CUSTOM, "-titlestyle", "titleStyle", "TitleStyle",
	DEF_COLUMN_STYLE, Blt_Offset(TreeViewColumn, titleStylePtr), 
        0, &bltTreeViewStyleOption},
    {BLT_CONFIG_STRING, "-validatecmd", (char *)NULL, (char *)NULL,
        (char *)NULL, Blt_Offset(TreeViewColumn, validCmd), 
	BLT_CONFIG_NULL_OK, 0},
    {BLT_CONFIG_INT, "-underline", (char *)NULL, (char *)NULL,
	DEF_COLUMN_UNDERLINE, Blt_Offset(TreeViewColumn, underline), 
	0},
    {BLT_CONFIG_DOUBLE, "-weight", (char *)NULL, (char *)NULL,
	DEF_COLUMN_WEIGHT, Blt_Offset(TreeViewColumn, weight), 
	0},
    {BLT_CONFIG_DISTANCE, "-width",(char *)NULL, (char *)NULL,
	DEF_COLUMN_WIDTH, Blt_Offset(TreeViewColumn, reqWidth), 
        0},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

static Blt_ConfigSpec sortSpecs[] =
{
    {BLT_CONFIG_STRING, "-command", "command", "Command",
	DEF_SORT_COMMAND, Blt_Offset(TreeView, sortCmd),
	BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-column", "column", "Column",
	DEF_SORT_COLUMN, Blt_Offset(TreeView, sortColumnPtr),
	BLT_CONFIG_DONT_SET_DEFAULT, &bltTreeViewColumnOption},
    {BLT_CONFIG_BOOLEAN, "-decreasing", "decreasing", "Decreasing",
	DEF_SORT_DECREASING, Blt_Offset(TreeView, sortDecreasing),
        BLT_CONFIG_DONT_SET_DEFAULT}, 
    {BLT_CONFIG_BOOLEAN, "-setflat", "setFlat", "SetFlat",
	DEF_SORT_SETFLAT, Blt_Offset(TreeView, setFlatView),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-mode", "mode", "Mode",
	DEF_SORT_TYPE, Blt_Offset(TreeView, sortType), 0, &typeOption},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

static Blt_TreeCompareNodesProc CompareNodes;
static Blt_TreeApplyProc SortApplyProc;

/*
 *----------------------------------------------------------------------
 *
 * ObjToColumn --
 *
 *	Convert the string reprsenting a scroll mode, to its numeric
 *	form.
 *
 * Results:
 *	If the string is successfully converted, TCL_OK is returned.
 *	Otherwise, TCL_ERROR is returned and an error message is left
 *	in interpreter's result field.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToColumn(clientData, interp, tkwin, objPtr, widgRec, offset)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/* Interpreter to send results back to */
    Tk_Window tkwin;		/* Not used. */
    Tcl_Obj *objPtr;		/* New legend position string */
    char *widgRec;
    int offset;
{
    TreeViewColumn **columnPtrPtr = (TreeViewColumn **)(widgRec + offset);
    char *string;

    string = Tcl_GetString(objPtr);
    if (*string == '\0') {
	*columnPtrPtr = NULL;
    } else {
	TreeView *tvPtr = (TreeView *)widgRec;

	if (Blt_TreeViewGetColumn(interp, tvPtr, objPtr, columnPtrPtr) 
	    != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ColumnToString --
 *
 * Results:
 *	The string representation of the button boolean is returned.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ColumnToObj(clientData, interp, tkwin, widgRec, offset)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    Tk_Window tkwin;		/* Not used. */
    char *widgRec;
    int offset;
{
    TreeViewColumn *columnPtr = *(TreeViewColumn **)(widgRec + offset);

    return Tcl_NewStringObj(columnPtr?columnPtr->key:"", -1);
}

/*
 *----------------------------------------------------------------------
 *
 * ObjToData --
 *
 *	Convert the string reprsenting a data, to its tree
 *	form.
 *
 * Results:
 *	If the string is successfully converted, TCL_OK is returned.
 *	Otherwise, TCL_ERROR is returned and an error message is left
 *	in interpreter's result field.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToData(clientData, interp, tkwin, objPtr, widgRec, offset)
    ClientData clientData;	/* Unused. */
    Tcl_Interp *interp;		/* Interpreter to send results back to */
    Tk_Window tkwin;		/* Not used. */
    Tcl_Obj *objPtr;		/* Tcl_Obj representing new data. */
    char *widgRec;
    int offset;
{
    Tcl_Obj **objv;
    TreeViewColumn *columnPtr;
    TreeViewEntry *entryPtr = (TreeViewEntry *)widgRec;
    char *string;
    int objc;
    register int i;
    TreeView *tvPtr;

    string = Tcl_GetString(objPtr);
    if (*string == '\0') {
	return TCL_OK;
    } 
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 0) {
	return TCL_OK;
    }
    if (objc & 0x1) {
	Tcl_AppendResult(interp, "data \"", string, 
		 "\" must be in even name-value pairs", (char *)NULL);
	return TCL_ERROR;
    }
    tvPtr = entryPtr->tvPtr;
    for (i = 0; i < objc; i += 2) {
        int result;
        
	if (Blt_TreeViewGetColumn(interp, tvPtr, objv[i], &columnPtr) 
	    != TCL_OK) {
	    return TCL_ERROR;
	}
        result = Blt_TreeSetValueByKey(tvPtr->interp, tvPtr->tree, entryPtr->node, 
             columnPtr->key, objv[i + 1]);
        if ((entryPtr->flags & ENTRY_DELETED) || (tvPtr->flags & TV_DELETED)) {
            return TCL_ERROR;
        }
	if (result != TCL_OK) {
	    return TCL_ERROR;
	}
	Blt_TreeViewAddValue(entryPtr, columnPtr);
    }
    return TCL_OK;
}

static int
ObjToColorPat(clientData, interp, tkwin, objPtr, widgRec, offset)
    ClientData clientData;	/* Unused. */
    Tcl_Interp *interp;		/* Interpreter to send results back to */
    Tk_Window tkwin;		/* Not used. */
    Tcl_Obj *objPtr;		/* Tcl_Obj representing new data. */
    char *widgRec;
    int offset;
{
    Tcl_Obj **objv;
    Tcl_Obj **objPtrPtr = (Tcl_Obj **)(widgRec + offset);
    int objc, i;
    XColor *color = NULL;
    
    if (objPtr != NULL && strlen(Tcl_GetString(objPtr))) {
         
        if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
            return TCL_ERROR;
        }
        if (objc%2) {
            Tcl_AppendResult(interp, "odd length: ", Tcl_GetString(objPtr),0);
            return TCL_ERROR;
        }
        for (i = 0; i < objc; i += 2) {
            if (clientData != 0 && 
                Tcl_RegExpMatchObj(interp, objv[i], objv[i]) < 0) {
                return TCL_ERROR;
            }
            color = Tk_AllocColorFromObj(interp, tkwin, objv[i+1]);
            if (color == NULL) {
                Tcl_AppendResult(interp, "bad color: ", Tcl_GetString(objv[i+1]),0);
                return TCL_ERROR;
            }
        }
    }
    if (*objPtrPtr != NULL) {
        Tcl_DecrRefCount(*objPtrPtr);
    }
    Tcl_IncrRefCount(objPtr);
    *objPtrPtr = objPtr;
    return TCL_OK;
}

static Tcl_Obj *
ColorPatToObj(clientData, interp, tkwin, widgRec, offset)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    Tk_Window tkwin;		/* Not used. */
    char *widgRec;
    int offset;
{
    TreeViewColumn *columnPtr = (TreeViewColumn *)widgRec;
    Tcl_Obj *objPtr;
    if (clientData == 0) {
        objPtr = columnPtr->colorPats;
    } else {
        objPtr = columnPtr->colorRegex;
    }
    if (objPtr == NULL) {
        objPtr = Tcl_NewStringObj("", -1);
    }
    Tcl_IncrRefCount(objPtr);
    return objPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * DataToObj --
 *
 * Results:
 *	The string representation of the data is returned.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
DataToObj(clientData, interp, tkwin, widgRec, offset)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    Tk_Window tkwin;		/* Not used. */
    char *widgRec;
    int offset;
{
    Tcl_Obj *listObjPtr, *objPtr;
    TreeViewEntry *entryPtr = (TreeViewEntry *)widgRec;
    TreeViewValue *valuePtr;

    /* Add the key-value pairs to a new Tcl_Obj */
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (valuePtr = entryPtr->values; valuePtr != NULL; 
	valuePtr = valuePtr->nextPtr) {
	objPtr = Tcl_NewStringObj(valuePtr->columnPtr->key, -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	if (Blt_TreeViewGetData(entryPtr, valuePtr->columnPtr->key, &objPtr)
	    != TCL_OK) {
	        objPtr = Tcl_NewStringObj("", -1);
	} 
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    return listObjPtr;
}

int
Blt_TreeViewGetColumn(interp, tvPtr, objPtr, columnPtrPtr)
    Tcl_Interp *interp;
    TreeView *tvPtr;
    Tcl_Obj *objPtr;
    TreeViewColumn **columnPtrPtr;
{
    char *string;
    Blt_TreeKey key;

    string = Tcl_GetString(objPtr);
    if (strcmp(string, "BLT TreeView") == 0) {
        *columnPtrPtr = &tvPtr->treeColumn;
    } else {
        int cNum, n = 0;
        Blt_HashEntry *hPtr;
    
        key = Blt_TreeKeyGet(interp, tvPtr->tree?tvPtr->tree->treeObject:NULL, string);
        hPtr = Blt_FindHashEntry(&tvPtr->columnTable, key);
        if (hPtr == NULL) {
            if (Tcl_GetIntFromObj(NULL, objPtr, &cNum) == TCL_OK &&
            cNum >= 0) {
                Blt_ChainLink *linkPtr;
                TreeViewColumn *columnPtr;

                for (linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); linkPtr != NULL;
                linkPtr = Blt_ChainNextLink(linkPtr)) {
                    columnPtr = Blt_ChainGetValue(linkPtr);
                    if (cNum == n) {
                        *columnPtrPtr = columnPtr;
                        return TCL_OK;
                    }
                    n++;
                }
            }
            if (interp != NULL) {
                Tcl_AppendResult(interp, "can't find column \"", string, 
                    "\" in \"", Tk_PathName(tvPtr->tkwin), "\"", 
                    (char *)NULL);
            }
            return TCL_ERROR;
        } 
        *columnPtrPtr = Blt_GetHashValue(hPtr);
    }
    return TCL_OK;
}

static int
ParseParentheses(
    Tcl_Interp *interp,
    CONST char *string,
    char **leftPtr, 
    char **rightPtr)
{
    register char *p;
    char *left, *right;

    left = right = NULL;
    for (p = (char *)string; *p != '\0'; p++) {
	if (*p == '(') {
	    left = p;
	} else if (*p == ')') {
	    right = p;
	}
    }
    if (left != right) {
	if (((left != NULL) && (right == NULL)) ||
	    ((left == NULL) && (right != NULL)) ||
	    (left > right) || (right != (p - 1))) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "bad array specification \"", string,
			     "\"", (char *)NULL);
	    }
	    return TCL_ERROR;
	}
    }
    *leftPtr = left;
    *rightPtr = right;
    return TCL_OK;
}



int
Blt_TreeViewGetColumnKey(interp, tvPtr, objPtr, columnPtrPtr, keyPtrPtr)
    Tcl_Interp *interp;
    TreeView *tvPtr;
    Tcl_Obj *objPtr;
    TreeViewColumn **columnPtrPtr;
    char **keyPtrPtr;
{
    char *right = NULL;
    char *string;
    int cNum, n = 0;
    Blt_HashEntry *hPtr;
    Blt_TreeKey key;
    Blt_TreeObject tree;

    string = Tcl_GetString(objPtr);

    if (strcmp(string, "BLT TreeView") == 0) {
        *columnPtrPtr = &tvPtr->treeColumn;
        return TCL_OK;
    }

    if (ParseParentheses(interp, string, keyPtrPtr, &right) != TCL_OK) {
        return TCL_ERROR;
    }
    
    tree = tvPtr->tree?tvPtr->tree->treeObject:NULL;
    if (right == NULL) {
        key = Blt_TreeKeyGet(interp, tree, string);
    } else {
        Tcl_DString dStr;
        Tcl_DStringInit(&dStr);
        Tcl_DStringAppend(&dStr, string, (*keyPtrPtr)-string);
        key = Blt_TreeKeyGet(interp, tree,  Tcl_DStringValue(&dStr) );
        Tcl_DStringFree(&dStr);
    }
    hPtr = Blt_FindHashEntry(&tvPtr->columnTable, key);
    if (hPtr == NULL) {
        if (Tcl_GetIntFromObj(NULL, objPtr, &cNum) == TCL_OK &&
        cNum >= 0) {
            Blt_ChainLink *linkPtr;
            TreeViewColumn *columnPtr;

            for (linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); linkPtr != NULL;
            linkPtr = Blt_ChainNextLink(linkPtr)) {
                columnPtr = Blt_ChainGetValue(linkPtr);
                if (cNum == n) {
                    *columnPtrPtr = columnPtr;
                    return TCL_OK;
                }
                n++;
            }
        }
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find column \"", string, 
                "\" in \"", Tk_PathName(tvPtr->tkwin), "\"", 
                (char *)NULL);
        }
        return TCL_ERROR;
    } 
    *columnPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

int
Blt_TreeViewNumColumns(tvPtr)
TreeView *tvPtr;
{
    int n = 0;
    Blt_ChainLink *linkPtr;

    for (linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); linkPtr != NULL;
    linkPtr = Blt_ChainNextLink(linkPtr)) {
        n++;
    }
    return n;
}

/*
* Refresh the tree-local definition for column keys.
*/
void
Blt_TreeViewColumnRekey(tvPtr)
TreeView *tvPtr;
{
    Blt_ChainLink *linkPtr;
    TreeViewColumn *columnPtr;

    for (linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); linkPtr != NULL;
    linkPtr = Blt_ChainNextLink(linkPtr)) {
        columnPtr = Blt_ChainGetValue(linkPtr);
        columnPtr->key = Blt_TreeKeyGet(tvPtr->interp, tvPtr->tree?tvPtr->tree->treeObject:NULL, columnPtr->name);
    }
}

int
Blt_TreeViewColumnNum(tvPtr, string)
TreeView *tvPtr;
char *string;
{
    int n = 0, m = -1, isTree;
    Blt_ChainLink *linkPtr;
    TreeViewColumn *columnPtr;


    isTree = (!strcmp(string, "BLT TreeView"));
    for (linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); linkPtr != NULL;
        linkPtr = Blt_ChainNextLink(linkPtr)) {
        columnPtr = Blt_ChainGetValue(linkPtr);
        if (!strcmp(string, columnPtr->key)) {
            return n;
        } else if (isTree && columnPtr == &tvPtr->treeColumn) {
            m = n;
        }
        n++;
    }
    return m;
}

int
Blt_TreeViewColumnInd(tvPtr, columnPtr)
    TreeView *tvPtr;
    TreeViewColumn *columnPtr;
{
    int n = 0;
    Blt_ChainLink *linkPtr;
    TreeViewColumn *curPtr;

    for (linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); linkPtr != NULL;
        linkPtr = Blt_ChainNextLink(linkPtr)) {
        curPtr = Blt_ChainGetValue(linkPtr);
        if (curPtr == columnPtr) {
            return n;
        }
        n++;
    }
    return -1;
}


/*
 *----------------------------------------------------------------------
 *
 * ObjToStyle --
 *
 *	Convert the name of an icon into a treeview style.
 *
 * Results:
 *	If the string is successfully converted, TCL_OK is returned.
 *	Otherwise, TCL_ERROR is returned and an error message is left in
 *	interpreter's result field.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToStyle(clientData, interp, tkwin, objPtr, widgRec, offset)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/* Interpreter to send results back to */
    Tk_Window tkwin;		/* Not used. */
    Tcl_Obj *objPtr;		/* Tcl_Obj representing the new value. */
    char *widgRec;
    int offset;
{
    TreeView *tvPtr = clientData;
    TreeViewStyle **stylePtrPtr = (TreeViewStyle **)(widgRec + offset);
    TreeViewStyle *stylePtr;

    if (Blt_TreeViewGetStyleMake(interp, tvPtr, Tcl_GetString(objPtr), 
	     &stylePtr, NULL, NULL, NULL) != TCL_OK) {
        *stylePtrPtr = tvPtr->stylePtr;
	return TCL_ERROR;
    }
    stylePtr->flags |= STYLE_DIRTY;
    tvPtr->flags |= (TV_LAYOUT | TV_DIRTY);
    *stylePtrPtr = stylePtr;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * IconToObj --
 *
 *	Converts the icon into its string representation (its name).
 *
 * Results:
 *	The name of the icon is returned.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
StyleToObj(clientData, interp, tkwin, widgRec, offset)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    Tk_Window tkwin;		/* Not used. */
    char *widgRec;
    int offset;
{
    TreeViewStyle *stylePtr = *(TreeViewStyle **)(widgRec + offset);

    return Tcl_NewStringObj(stylePtr?stylePtr->name:"", -1);
}

/*ARGSUSED*/
static int
FreeStyle(clientData, display, widgRec, offset, oldPtr)
    ClientData clientData;
    Display *display;		/* Not used. */
    char *widgRec;
    int offset;
    char *oldPtr;
{
    TreeView *tvPtr = clientData;
    TreeViewStyle *stylePtr = (TreeViewStyle *)(oldPtr);

    Blt_TreeViewFreeStyle(tvPtr, stylePtr);
    return TCL_OK;
}

static int
ObjToStyles(clientData, interp, tkwin, objPtr, widgRec, offset)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/* Interpreter to send results back to */
    Tk_Window tkwin;		/* Not used. */
    Tcl_Obj *objPtr;		/* Tcl_Obj representing the new value. */
    char *widgRec;
    int offset;
{
    TreeView *tvPtr = clientData;
    TreeViewStyle ***stylesPtrPtr = (TreeViewStyle ***)(widgRec + offset);
    TreeViewStyle **sPtr = NULL;
    Tcl_Obj **objv;
    int objc, i;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc != 0) {
        sPtr = Blt_Calloc(objc+1, sizeof(*sPtr));
        for (i=0; i<objc; i++) {
            if (Blt_TreeViewGetStyleMake(interp, tvPtr, Tcl_GetString(objv[i]), 
                &sPtr[i], NULL, NULL, NULL) != TCL_OK) {
                    Blt_Free(sPtr);
                    return TCL_ERROR;
            }
            sPtr[i]->flags |= STYLE_DIRTY;
        }
    }
    tvPtr->flags |= (TV_LAYOUT | TV_DIRTY);
    *stylesPtrPtr = sPtr;
    return TCL_OK;
}

/*ARGSUSED*/
static Tcl_Obj *
StylesToObj(clientData, interp, tkwin, widgRec, offset)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    Tk_Window tkwin;		/* Not used. */
    char *widgRec;
    int offset;
{
    TreeViewStyle **sPtr = *(TreeViewStyle ***)(widgRec + offset);
    Tcl_Obj *obj;
    int i;
    
    if (sPtr == NULL) {
        return Tcl_NewStringObj("", -1);
    }
    obj = Tcl_NewListObj(0,0);
    i = 0;
    while (sPtr[i]) {
        Tcl_ListObjAppendElement(interp, obj, Tcl_NewStringObj(sPtr[i]->name, -1));
        i++;
    }
    return obj;
}

/*ARGSUSED*/
static int
FreeStyles(clientData, display, widgRec, offset, oldPtr)
    ClientData clientData;
    Display *display;		/* Not used. */
    char *widgRec;
    int offset;
    char *oldPtr;
{
    TreeView *tvPtr = clientData;
    TreeViewStyle **sPtr = (TreeViewStyle **)(oldPtr);
    int i;
    
    if (sPtr == NULL) { return TCL_OK; }
    i = 0;
    while (sPtr[i] != NULL) {
        Blt_TreeViewFreeStyle(tvPtr, sPtr[i]);
        i++;
    }
    Blt_Free(sPtr);
    return TCL_OK;
}

void
Blt_TreeViewUpdateColumnGCs(tvPtr, columnPtr)
    TreeView *tvPtr;
    TreeViewColumn *columnPtr;
{
    Drawable drawable;
    GC newGC;
    Tk_3DBorder border;
    XGCValues gcValues;
    int ruleDrawn;
    unsigned long gcMask;
    int iconWidth, iconHeight;
    int textWidth, textHeight;
    gcMask = GCForeground | GCFont;

    gcValues.font = Tk_FontId((columnPtr->font?columnPtr->font: tvPtr->font));
        
    /* Normal text */
    gcValues.foreground = CHOOSE(tvPtr->fgColor,columnPtr->fgColor)->pixel;
    newGC = Tk_GetGC(tvPtr->tkwin, gcMask, &gcValues);
    if (columnPtr->textGC != NULL) {
        Tk_FreeGC(tvPtr->display, columnPtr->textGC);
    }
    columnPtr->textGC = newGC;

    gcValues.font = Tk_FontId((columnPtr->titleFont?columnPtr->titleFont:
        tvPtr->titleFont));

    /* Normal title text */
    gcValues.foreground = columnPtr->titleFgColor->pixel;
    newGC = Tk_GetGC(tvPtr->tkwin, gcMask, &gcValues);
    if (columnPtr->titleGC != NULL) {
	Tk_FreeGC(tvPtr->display, columnPtr->titleGC);
    }
    columnPtr->titleGC = newGC;

    /* Active title text */
    gcValues.foreground = columnPtr->activeTitleFgColor->pixel;
    newGC = Tk_GetGC(tvPtr->tkwin, gcMask, &gcValues);
    if (columnPtr->activeTitleGC != NULL) {
	Tk_FreeGC(tvPtr->display, columnPtr->activeTitleGC);
    }
    columnPtr->activeTitleGC = newGC;

    columnPtr->titleWidth = 0;
    iconWidth = iconHeight = 0;
    if (columnPtr->titleIcon != NULL) {
	iconWidth = TreeViewIconWidth(columnPtr->titleIcon);
	iconHeight = TreeViewIconHeight(columnPtr->titleIcon);
	columnPtr->titleWidth += iconWidth;
    }
    if (columnPtr->titleTextPtr != NULL) {
	Blt_Free(columnPtr->titleTextPtr);
	columnPtr->titleTextPtr = NULL;
    }
    textWidth = textHeight = 0;
    if (columnPtr->title != NULL) {
	TextStyle ts;

        memset(&ts, 0, sizeof(TextStyle));
	ts.font = (columnPtr->titleFont?columnPtr->titleFont:tvPtr->titleFont);
	ts.justify = TK_JUSTIFY_LEFT;
	ts.underline = columnPtr->underline;
	ts.shadow.offset = columnPtr->titleShadow.offset;
	columnPtr->titleTextPtr = Blt_GetTextLayout(columnPtr->title, &ts);
	textHeight = columnPtr->titleTextPtr->height + tvPtr->titlePad*2;
        if (columnPtr->underline>=0) {
            textHeight += 2;
        }
	textWidth = columnPtr->titleTextPtr->width;
	columnPtr->titleWidth += textWidth;
    }
    if ((iconWidth > 0) && (textWidth > 0)) {
	columnPtr->titleWidth += 8;
    }
    columnPtr->titleWidth += STD_ARROW_HEIGHT;
    columnPtr->titleHeight = MAX(iconHeight, textHeight);

    gcMask = (GCFunction | GCLineWidth | GCLineStyle | GCForeground);

    /* 
     * If the rule is active, turn it off (i.e. draw again to erase
     * it) before changing the GC.  If the color changes, we won't be
     * able to erase the old line, since it will no longer be
     * correctly XOR-ed with the background.
     */
    drawable = Tk_WindowId(tvPtr->tkwin);
    ruleDrawn = ((tvPtr->flags & TV_RULE_ACTIVE) &&
		 (tvPtr->activeTitleColumnPtr == columnPtr) && 
		 (drawable != None));
    if (ruleDrawn) {
	Blt_TreeViewDrawRule(tvPtr, columnPtr, drawable);
    }
    /* XOR-ed rule column divider */ 
    gcValues.line_width = LineWidth(columnPtr->ruleLineWidth);
    gcValues.foreground = 
	Blt_TreeViewGetStyleFg(tvPtr, columnPtr, columnPtr->stylePtr)->pixel;
    if (LineIsDashed(columnPtr->ruleDashes)) {
	gcValues.line_style = LineOnOffDash;
    } else {
	gcValues.line_style = LineSolid;
    }
    gcValues.function = GXxor;

    border = CHOOSE(tvPtr->border, columnPtr->border);
    gcValues.foreground ^= Tk_3DBorderColor(border)->pixel; 
    newGC = Blt_GetPrivateGC(tvPtr->tkwin, gcMask, &gcValues);
    if (columnPtr->ruleGC != NULL) {
	Blt_FreePrivateGC(tvPtr->display, columnPtr->ruleGC);
    }
    if (LineIsDashed(columnPtr->ruleDashes)) {
	Blt_SetDashes(tvPtr->display, newGC, &columnPtr->ruleDashes);
    }
    columnPtr->ruleGC = newGC;
    if (ruleDrawn) {
	Blt_TreeViewDrawRule(tvPtr, columnPtr, drawable);
    }
    columnPtr->flags |= COLUMN_DIRTY;
    tvPtr->flags |= TV_UPDATE;
}

static void
DestroyColumnNow(DestroyData data)
{
    TreeViewColumn *columnPtr = (TreeViewColumn *)data;
    TreeView *tvPtr;
    
    tvPtr = columnPtr->tvPtr;
    if (columnPtr->title != NULL) {
        Blt_Free(columnPtr->title);
        columnPtr->title = NULL;
    }
    if (columnPtr->titleTextPtr != NULL) {
        Blt_Free(columnPtr->titleTextPtr);
        columnPtr->titleTextPtr = NULL;
    }
    if (columnPtr->stylePtr != NULL) {
	Blt_TreeViewFreeStyle(tvPtr, columnPtr->stylePtr);
	columnPtr->stylePtr = NULL;
    }
    if (columnPtr->titleStylePtr != NULL) {
        Blt_TreeViewFreeStyle(tvPtr, columnPtr->titleStylePtr);
        columnPtr->titleStylePtr = NULL;
    }
    if (columnPtr->tile != NULL) {
        Blt_FreeTile(columnPtr->tile);
        columnPtr->tile = NULL;
    }
    if (columnPtr->defValue != NULL) {
        Blt_PoolFreeItem(tvPtr->valuePool, columnPtr->defValue);
        columnPtr->defValue = NULL;
    }
    if (columnPtr->trace != NULL) {
        Blt_TreeDeleteTrace(columnPtr->trace);
        columnPtr->trace = NULL;
    }
    Blt_Free(columnPtr->name);
    if (columnPtr != &tvPtr->treeColumn) {
        Blt_Free(columnPtr);
    }
}
    
static void
DestroyColumn(tvPtr, columnPtr)
    TreeView *tvPtr;
    TreeViewColumn *columnPtr;
{
    ClientData object;
    Blt_HashEntry *hPtr;
    
    columnPtr->flags |= COLUMN_DELETED;

    if (tvPtr->selAnchorCol == columnPtr) { tvPtr->selAnchorCol = NULL; }
    if (tvPtr->activeColumnPtr == columnPtr) { tvPtr->activeColumnPtr = NULL; }
    if (tvPtr->activeTitleColumnPtr == columnPtr) { tvPtr->activeTitleColumnPtr = NULL; }
    if (tvPtr->resizeColumnPtr == columnPtr) { tvPtr->resizeColumnPtr = NULL; }
    if (tvPtr->sortColumnPtr == columnPtr) { tvPtr->sortColumnPtr = NULL; }
    Blt_TreeViewWindowRelease(NULL, columnPtr);
    Blt_TreeViewOptsInit(tvPtr);
    object = Blt_TreeViewColumnTag(tvPtr, columnPtr->key);
    if (object) {
        Blt_DeleteBindings(tvPtr->bindTable, object);
    }
    Blt_DeleteBindings(tvPtr->bindTable, columnPtr);
    hPtr = Blt_FindHashEntry(&tvPtr->columnTagTable, columnPtr->key);
    if (hPtr != NULL) {
        Blt_DeleteHashEntry(&tvPtr->columnTagTable, hPtr);
    }
    Blt_FreeObjOptions(tvPtr->interp, columnSpecs, (char *)columnPtr, tvPtr->display, 0);
    if (columnPtr->titleGC != NULL) {
        Tk_FreeGC(tvPtr->display, columnPtr->titleGC);
        columnPtr->titleGC = NULL;
    }
    if (columnPtr->textGC != NULL) {
        Tk_FreeGC(tvPtr->display, columnPtr->textGC);
        columnPtr->textGC = NULL;
    }
    if (columnPtr->ruleGC != NULL) {
        Blt_FreePrivateGC(tvPtr->display, columnPtr->ruleGC);
        columnPtr->ruleGC = NULL;
    }
    if (columnPtr->activeTitleGC != NULL) {
        Tk_FreeGC(tvPtr->display, columnPtr->activeTitleGC);
        columnPtr->activeTitleGC = NULL;
    }
    hPtr = Blt_FindHashEntry(&tvPtr->columnTable, columnPtr->key);
    if (hPtr != NULL) {
        Blt_DeleteHashEntry(&tvPtr->columnTable, hPtr);
    }
    if (columnPtr->linkPtr != NULL) {
        Blt_ChainDeleteLink(tvPtr->colChainPtr, columnPtr->linkPtr);
        columnPtr->linkPtr = NULL;
    }
    Tcl_EventuallyFree(columnPtr, DestroyColumnNow);
}

void
Blt_TreeViewDestroyColumns(tvPtr)
    TreeView *tvPtr;
{
    if (tvPtr->colChainPtr != NULL) {
	Blt_ChainLink *linkPtr;
	TreeViewColumn *columnPtr;
	
	for (linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); linkPtr != NULL;
	     linkPtr = Blt_ChainNextLink(linkPtr)) {
	    columnPtr = Blt_ChainGetValue(linkPtr);
	    columnPtr->linkPtr = NULL;
	    DestroyColumn(tvPtr, columnPtr);
	}
	Blt_ChainDestroy(tvPtr->colChainPtr);
	tvPtr->colChainPtr = NULL;
    }
    Blt_DeleteHashTable(&tvPtr->columnTable);
}

void
Blt_TreeViewConfigureColumns(tvPtr)
TreeView *tvPtr;
{
    if (tvPtr->colChainPtr != NULL) {
        Blt_ChainLink *linkPtr;
        TreeViewColumn *columnPtr;
	
        for (linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); linkPtr != NULL;
        linkPtr = Blt_ChainNextLink(linkPtr)) {
            columnPtr = Blt_ChainGetValue(linkPtr);
            Blt_TreeViewUpdateColumnGCs(tvPtr, columnPtr);
        }
    }
}

static void
ColumnConfigChanges(tvPtr, interp, columnPtr)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    TreeViewColumn *columnPtr;
{
    if (Blt_ObjConfigModified(columnSpecs, interp, "-background", 0)) {
        columnPtr->hasbg = 1;
    }
    if (Blt_ObjConfigModified(columnSpecs, tvPtr->interp, "-titlebackground", 0)) {
        columnPtr->hasttlbg = 1;
    }
    if (columnPtr->tile != NULL) {
        Blt_SetTileChangedProc(columnPtr->tile, Blt_TreeViewTileChangedProc, tvPtr);
    }
    if (columnPtr->stylePtr == NULL) {
    }
    if (Blt_ObjConfigModified(columnSpecs, tvPtr->interp, "-justify", "-hide", "-weight", "-formatcmd", "-font", 0)) {
        Blt_TreeViewMakeStyleDirty(tvPtr);
    }
}

int
Blt_TreeViewCreateColumn(tvPtr, columnPtr, name, defTitle)
    TreeView *tvPtr;
    TreeViewColumn *columnPtr;
    char *name, *defTitle;
{
    Blt_HashEntry *hPtr;
    int isNew;
    Tcl_Interp *interp;
    Blt_TreeObject treeObj;
    char *left = NULL, *right = NULL;

    interp = tvPtr->interp;
    if (ParseParentheses(interp, name, &left, &right) != TCL_OK ||
        left != NULL || right != NULL) {
        Blt_Free(columnPtr);
        Tcl_AppendResult(interp, "column key may not use parens", 0);
        return TCL_ERROR;
    }
    treeObj = tvPtr->tree?tvPtr->tree->treeObject:NULL;
    columnPtr->tvPtr = tvPtr;
    columnPtr->name = Blt_Strdup(name);
    columnPtr->key = Blt_TreeKeyGet(interp, treeObj, name);
    columnPtr->title = Blt_Strdup(defTitle);
    columnPtr->justify = TK_JUSTIFY_CENTER;
    columnPtr->titleJustify = TK_JUSTIFY_CENTER;
    columnPtr->relief = TK_RELIEF_FLAT;
    columnPtr->borderWidth = 1;
    columnPtr->pad.side1 = columnPtr->pad.side2 = 2;
    columnPtr->state = STATE_NORMAL;
    columnPtr->weight = 1.0;
    columnPtr->editable = FALSE;
    columnPtr->ruleLineWidth = 1;
    columnPtr->titleBorderWidth = 2;
    columnPtr->titleRelief = TK_RELIEF_RAISED;
    columnPtr->titleIcon = NULL;
    columnPtr->tile = NULL;
    columnPtr->scrollTile = 0;
    columnPtr->hasbg = 0;
    columnPtr->hasttlbg = 0;
    columnPtr->defValue = Blt_TreeViewMakeValue(tvPtr, columnPtr, NULL);
    hPtr = Blt_CreateHashEntry(&tvPtr->columnTable, columnPtr->key, &isNew);
    Blt_SetHashValue(hPtr, columnPtr);

    Blt_TreeViewOptsInit(tvPtr);
    if (Blt_ConfigureComponentFromObj(tvPtr->interp, tvPtr->tkwin, name, 
	"Column", columnSpecs, 0, (Tcl_Obj **)NULL, (char *)columnPtr, 0) 
	!= TCL_OK) {
	DestroyColumn(tvPtr, columnPtr);
	return TCL_ERROR;
    }
    if (Blt_ObjConfigModified(columnSpecs, tvPtr->interp, "-background", 0)) {
        columnPtr->hasbg = 1;
    }
    if (Blt_ObjConfigModified(columnSpecs, tvPtr->interp, "-titlebackground", 0)) {
        columnPtr->hasttlbg = 1;
    }
    if (columnPtr->tile != NULL) {
        Blt_SetTileChangedProc(columnPtr->tile, Blt_TreeViewTileChangedProc, tvPtr);
    }
    if (Blt_ObjConfigModified(columnSpecs, tvPtr->interp, "-*font", "-foreground", "-titleborderwidth", "-titlerelief", "-titleshadow", 0)) {
        Blt_TreeViewMakeStyleDirty(tvPtr);
    }
    ColumnConfigChanges(tvPtr, interp, columnPtr);
    Blt_ObjConfigModified(columnSpecs, tvPtr->interp, 0);
    return TCL_OK;
    
}

static TreeViewColumn *
CreateColumn(tvPtr, nameObjPtr, objc, objv)
    TreeView *tvPtr;
    Tcl_Obj *nameObjPtr;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewColumn *columnPtr;
    Tcl_DString dString;
    char *string, *nStr;
    int colIdx, len;

    colIdx = 1;
    nStr = string = Tcl_GetStringFromObj(nameObjPtr, &len);

    columnPtr = Blt_Calloc(1, sizeof(TreeViewColumn));
    assert(columnPtr);
    Tcl_DStringInit(&dString);
    while (string[0] == 0 || (len>=5 && strncmp(string+len-5,"#auto", 5)==0)) {
        Tcl_DStringSetLength(&dString, 0);
        if (len <= 5) {
            Tcl_DStringAppend(&dString, "Col", -1);
        } else {
            Tcl_DStringAppend(&dString, string, len-5);
        }
        Tcl_DStringAppend(&dString, Blt_Itoa(colIdx), -1);
        colIdx++;
        nStr = Tcl_DStringValue(&dString);
        if (Blt_TreeViewColumnNum(tvPtr, nStr)<0) {
            break;
        }
    }
    if (Blt_TreeViewCreateColumn(tvPtr, columnPtr, nStr, nStr) != TCL_OK) {
        Tcl_DStringFree(&dString);
	return NULL;
    }
    Tcl_DStringFree(&dString);
    Blt_TreeViewOptsInit(tvPtr);
    if (Blt_ConfigureComponentFromObj(tvPtr->interp, tvPtr->tkwin, 
	columnPtr->key, "Column", columnSpecs, objc, objv, (char *)columnPtr, 
	BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	DestroyColumn(tvPtr, columnPtr);
	return NULL;
    }
    if (Blt_ObjConfigModified(columnSpecs, tvPtr->interp, "-background", 0)) {
        columnPtr->hasbg = 1;
    }
    if (columnPtr->tile != NULL) {
        Blt_SetTileChangedProc(columnPtr->tile, Blt_TreeViewTileChangedProc, tvPtr);
    }
    Blt_TreeViewUpdateColumnGCs(tvPtr, columnPtr);
    return columnPtr;
}

TreeViewColumn *
Blt_TreeViewNearestColumn(tvPtr, x, y, contextPtr)
    TreeView *tvPtr;
    int x, y;
    ClientData *contextPtr;
{
    if (tvPtr->nVisible > 0) {
	Blt_ChainLink *linkPtr;
	TreeViewColumn *columnPtr;
	int right;

	/*
	 * Determine if the pointer is over the rightmost portion of the
	 * column.  This activates the rule.
	 */
	x = WORLDX(tvPtr, x);
	for(linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); linkPtr != NULL;
	    linkPtr = Blt_ChainNextLink(linkPtr)) {
	    columnPtr = Blt_ChainGetValue(linkPtr);
	    right = columnPtr->worldX + columnPtr->width;
	    if ((x >= columnPtr->worldX) && (x <= right)) {
		if (contextPtr != NULL) {
		    *contextPtr = NULL;
		    if ((tvPtr->flags & TV_SHOW_COLUMN_TITLES) && 
			(y >= tvPtr->insetY) &&
			(y < (tvPtr->titleHeight + tvPtr->insetY))) {
			*contextPtr = (x >= (right - RULE_AREA)) 
			    ? ITEM_COLUMN_RULE : ITEM_COLUMN_TITLE;
		    } 
		}
		return columnPtr;
	    }
	}
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * ColumnActivateOp --
 *
 *	Selects the button to appear active.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnActivateOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    if (objc == 4) {
	Drawable drawable;
	TreeViewColumn *columnPtr;
	char *string;

	string = Tcl_GetString(objv[3]);
	if (string[0] == '\0') {
	    columnPtr = NULL;
	} else {
	    if (Blt_TreeViewGetColumn(interp, tvPtr, objv[3], &columnPtr) 
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	    if (((tvPtr->flags & TV_SHOW_COLUMN_TITLES) == 0) || 
		(columnPtr->hidden) || (columnPtr->state == STATE_DISABLED)) {
		columnPtr = NULL;
	    }
	}
	tvPtr->activeTitleColumnPtr = tvPtr->activeColumnPtr = columnPtr;
	drawable = Tk_WindowId(tvPtr->tkwin);
	if (drawable != None) {
	    Blt_TreeViewDrawHeadings(tvPtr, drawable);
	    Blt_TreeViewDrawOuterBorders(tvPtr, drawable);
	}
    }
    if (tvPtr->activeTitleColumnPtr != NULL) {
	Tcl_SetResult(interp, tvPtr->activeTitleColumnPtr->key, TCL_VOLATILE);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ColumnBindOp --
 *
 *	  .t bind tag sequence command
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnBindOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    ClientData object;
    TreeViewColumn *columnPtr;

    if (Blt_TreeViewGetColumn(NULL, tvPtr, objv[3], &columnPtr) == TCL_OK) {
	object = Blt_TreeViewColumnTag(tvPtr, columnPtr->key);
    } else {
	object = Blt_TreeViewColumnTag(tvPtr, Tcl_GetString(objv[3]));
    }
    return Blt_ConfigureBindingsFromObj(interp, tvPtr->bindTable, object,
	objc - 4, objv + 4);
}


/*
 *----------------------------------------------------------------------
 *
 * ColumnCgetOp --
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnCgetOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewColumn *columnPtr;

    if (Blt_TreeViewGetColumn(interp, tvPtr, objv[3], &columnPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    Blt_TreeViewOptsInit(tvPtr);
    return Blt_ConfigureValueFromObj(interp, tvPtr->tkwin, columnSpecs, 
	(char *)columnPtr, objv[4], 0);
}

/*
 *----------------------------------------------------------------------
 *
 * ColumnConfigureOp --
 *
 * 	This procedure is called to process a list of configuration
 *	options database, in order to reconfigure the one of more
 *	entries in the widget.
 *
 *	  .h entryconfigure node node node node option value
 *
 * Results:
 *	A standard Tcl result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for tvPtr; old resources get freed, if there
 *	were any.  The hypertext is redisplayed.
 *
 *----------------------------------------------------------------------
 */
static int
ColumnConfigureOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewColumn *columnPtr;
    int nOptions, start, result;
    register int i;

    /* Figure out where the option value pairs begin */
    for(i = 4; i < objc; i++) {
	if (Blt_ObjIsOption(tvPtr->interp, columnSpecs, objv[i], 0)) {
	    break;
	}
	if (Blt_TreeViewGetColumn(interp, tvPtr, objv[i], &columnPtr) 
	    != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (i<=3) {
        Tcl_AppendResult(interp, "column name missing", 0);
        return TCL_ERROR;
    }
    start = i;
    nOptions = objc - start;
    
    Blt_TreeViewOptsInit(tvPtr);
    for (i = 3; i < start; i++) {
        char *oldStyle;
        int isdel;
        
	if (Blt_TreeViewGetColumn(interp, tvPtr, objv[i], &columnPtr) 
	    != TCL_OK) {
	    return TCL_ERROR;
	}
	if (nOptions == 0) {
	    return Blt_ConfigureInfoFromObj(interp, tvPtr->tkwin, columnSpecs, 
		(char *)columnPtr, (Tcl_Obj *)NULL, 0);
	} else if (nOptions == 1) {
	    return Blt_ConfigureInfoFromObj(interp, tvPtr->tkwin, columnSpecs, 
		(char *)columnPtr, objv[start], 0);
	}
        oldStyle = (columnPtr->stylePtr ? columnPtr->stylePtr->name : NULL);
        Tcl_Preserve(columnPtr);
        result = Blt_ConfigureWidgetFromObj(tvPtr->interp, tvPtr->tkwin, 
             columnSpecs, nOptions, objv + start, (char *)columnPtr, 
             BLT_CONFIG_OBJV_ONLY, NULL);
         isdel = (columnPtr->flags & ENTRY_DELETED);
         Tcl_Release(columnPtr);
         if (isdel) {
             return TCL_ERROR;
         }
         if (columnPtr->sortAltColumns != NULL) {
             
             Tcl_Obj **sobjv;
             int sobjc, n;
             TreeViewColumn *acPtr;
    
             if (Tcl_ListObjGetElements(interp, columnPtr->sortAltColumns,
                 &sobjc, &sobjv) != TCL_OK) {
                     Tcl_DecrRefCount(columnPtr->sortAltColumns);
                     columnPtr->sortAltColumns = NULL;
                     return TCL_ERROR;
             }
             for (n = 0; n < sobjc; n++) {

                 if (Blt_TreeViewGetColumn(interp, tvPtr, sobjv[n], &acPtr)
                     != TCL_OK || acPtr == columnPtr || acPtr == &tvPtr->treeColumn) {
                     if (acPtr == columnPtr) {
                         Tcl_AppendResult(interp, "self reference", 0);
                     }
                     if (acPtr == &tvPtr->treeColumn) {
                         Tcl_AppendResult(interp, "tree column not valid", 0);
                     }
                     Tcl_DecrRefCount(columnPtr->sortAltColumns);
                     columnPtr->sortAltColumns = NULL;
                     return TCL_ERROR;
                 }
             }
        }
         if (columnPtr->stylePtr == NULL && oldStyle) {
             TreeViewStyle *stylePtr = NULL;
             
             Blt_TreeViewGetStyleMake(interp, tvPtr, oldStyle, &stylePtr, columnPtr,
                NULL, NULL);
             columnPtr->stylePtr = stylePtr;
         }
         if (result != TCL_OK) {
            return TCL_ERROR;
         }
         ColumnConfigChanges(tvPtr, interp, columnPtr);
         Blt_TreeViewUpdateColumnGCs(tvPtr, columnPtr);
    }
    /*FIXME: Makes every change redo everything. */
    tvPtr->flags |= (TV_LAYOUT | TV_DIRTY);
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ColumnDeleteOp --
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnDeleteOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;		/* Not used. */
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewColumn *columnPtr;
    TreeViewEntry *entryPtr;
    register int i;

    for(i = 3; i < objc; i++) {
	if (Blt_TreeViewGetColumn(interp, tvPtr, objv[i], &columnPtr) 
	    != TCL_OK) {
	    return TCL_ERROR;
	}
	if (columnPtr == &tvPtr->treeColumn) {
	    /* Quietly ignore requests to delete tree. */
	    continue;
	}
	if (columnPtr == tvPtr->sortColumnPtr) {
	    tvPtr->sortColumnPtr = NULL;
	}
	/* Traverse the tree deleting values associated with the column.  */
	for(entryPtr = tvPtr->rootPtr; entryPtr != NULL;
	    entryPtr = Blt_TreeViewNextEntry(entryPtr, 0)) {
	    if (entryPtr != NULL) {
		TreeViewValue *valuePtr, *lastPtr, *nextPtr;
		
		lastPtr = NULL;
		for (valuePtr = entryPtr->values; valuePtr != NULL; 
		     valuePtr = nextPtr) {
		    nextPtr = valuePtr->nextPtr;
		    if (valuePtr->columnPtr == columnPtr) {
			Blt_TreeViewDestroyValue(tvPtr, entryPtr, valuePtr);
			if (lastPtr == NULL) {
			    entryPtr->values = nextPtr;
			} else {
			    lastPtr->nextPtr = nextPtr;
			}
			break;
		    }
		    lastPtr = valuePtr;
		}
	    }
	}
	DestroyColumn(tvPtr, columnPtr);
    }
    /* Deleting a column may affect the height of an entry. */
    tvPtr->flags |= (TV_LAYOUT | TV_DIRTY | TV_RESORT);
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ColumnIssetOp --
 *
 *   Return columns that have data in the currently visible entries.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnIssetOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;		/* Not used. */
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewColumn *columnPtr;
    TreeViewEntry *entryPtr, **p;
    TreeViewValue *valuePtr;
    Blt_ChainLink *linkPtr;
    Tcl_Obj *listObjPtr, *objPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (objc == 3) {
        for (linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); 
            linkPtr != NULL; linkPtr = Blt_ChainNextLink(linkPtr)) {
            columnPtr = Blt_ChainGetValue(linkPtr);
            if (columnPtr == &tvPtr->treeColumn) continue;

            for (p = tvPtr->visibleArr; *p != NULL; p++) {
                entryPtr = *p;
                valuePtr = Blt_TreeViewFindValue(entryPtr, columnPtr);
                if (valuePtr != NULL) {
                    objPtr = Tcl_NewStringObj(columnPtr->key, -1);
                    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                    break;
                }
            }
        }
    } else if (objc == 4) {
        TreeViewTagInfo info = {0};

        for (linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); 
            linkPtr != NULL; linkPtr = Blt_ChainNextLink(linkPtr)) {
            
            columnPtr = Blt_ChainGetValue(linkPtr);
            if (columnPtr == &tvPtr->treeColumn) continue;
            if (Blt_TreeViewFindTaggedEntries(tvPtr, objv[3], &info) != TCL_OK) {
                return TCL_ERROR;
            }
            for (entryPtr = Blt_TreeViewFirstTaggedEntry(&info); entryPtr != NULL; 
                entryPtr = Blt_TreeViewNextTaggedEntry(&info)) {
                
                valuePtr = Blt_TreeViewFindValue(entryPtr, columnPtr);
                if (valuePtr != NULL) {
                    objPtr = Tcl_NewStringObj(columnPtr->key, -1);
                    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                    break;
                }
            }
            Blt_TreeViewDoneTaggedEntries(&info);
        }

    } else if (objc == 5) {
        TreeViewEntry *ePtr, *entryPtr2;
        if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if (Blt_TreeViewGetEntry(tvPtr, objv[4], &entryPtr2) != TCL_OK) {
            return TCL_ERROR;
        }
        for (linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); 
            linkPtr != NULL; linkPtr = Blt_ChainNextLink(linkPtr)) {
            columnPtr = Blt_ChainGetValue(linkPtr);
            if (columnPtr == &tvPtr->treeColumn) continue;
            for (ePtr = entryPtr; ePtr;
                ePtr = Blt_TreeViewNextEntry(ePtr, ENTRY_MASK)) {
                valuePtr = Blt_TreeViewFindValue(ePtr, columnPtr);
                if (valuePtr != NULL) {
                    objPtr = Tcl_NewStringObj(columnPtr->key, -1);
                    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                    break;
                }
                if (ePtr == entryPtr2) break;
            }

        }
    }
    Tcl_SetObjResult(interp, listObjPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ColumnIndexOp --
 *
 *	Return column index.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnIndexOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    int n, cnt;
    char *string;
    
    string = Tcl_GetString(objv[3]);
    
    cnt = Blt_TreeViewNumColumns(tvPtr);
    if (strncmp("end", string, 3) == 0 &&
        Blt_GetPositionSize(interp, string, cnt, &n) == TCL_OK) {
            Tcl_SetObjResult(interp, Tcl_NewIntObj(n));
            return TCL_OK;
    }
    n = Blt_TreeViewColumnNum(tvPtr, string);
    if (n<0) {
        if ((Tcl_GetInt(NULL, string, &n) != TCL_OK)) {
            goto err;
        }
        if (n>=cnt || n<0) {
            goto err;
        }
    }
    if (n<0) {
err:
        Tcl_AppendResult(interp, "unknown column: ", string, 0);
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(n));
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * ColumnIstreeOp --
 *
 *	Return 1 if is the tree column.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnIstreeOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewColumn *columnPtr;
    if (Blt_TreeViewGetColumn(interp, tvPtr, objv[3], &columnPtr) != TCL_OK) {
        return TCL_ERROR;
    }

    Tcl_SetObjResult(interp, Tcl_NewIntObj(columnPtr==&tvPtr->treeColumn));
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * ColumnInsertOp --
 *
 *	Add new columns to the tree.
 *      .t col insert POS NAME ...
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnInsertOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    Blt_ChainLink *beforePtr;
    Tcl_Obj *CONST *options;
    TreeViewColumn *columnPtr;
    TreeViewEntry *entryPtr;
    int insertPos;
    int nOptions;
    int start;
    register int i;
    int maxCol = Blt_ChainGetLength(tvPtr->colChainPtr);

    if (Blt_GetPositionSizeFromObj(tvPtr->interp, objv[3], maxCol,
        &insertPos) != TCL_OK) {
        if ((insertPos = Blt_TreeViewColumnNum(tvPtr, Tcl_GetString(objv[3])))<0) {
            return TCL_ERROR;
        }
        Tcl_ResetResult(interp);
    }
    if ((insertPos == -1) || 
	(insertPos >= maxCol)) {
	beforePtr = NULL;
    } else {
	beforePtr =  Blt_ChainGetNthLink(tvPtr->colChainPtr, insertPos);
    }
    /*
     * Count the column names that follow.  Count the arguments until we
     * spot one that looks like a configuration option (i.e. starts
     * with a minus ("-")).
     */
    for (i = 5; i < objc; i++) {
        char *cp = Tcl_GetString(objv[i]);
        if (cp[0] == '-') break;
    }
    start = i;
    nOptions = objc - i;
    options = objv + start;

    if ((objc-nOptions) < 5) {
        Tcl_AppendResult(interp, "column insert must have a name", 0);
        return TCL_ERROR;
    }
    if ((objc-start)%2) {
        Tcl_AppendResult(interp, "odd number of column options", 0);
        return TCL_ERROR;
    }
    for (i = start; i < objc; i+=2) {
        if (!Blt_ObjIsOption(tvPtr->interp, columnSpecs, objv[i], 0)) {
            Tcl_AppendResult(interp, "unknown option \"", Tcl_GetString(objv[i]), "\", should be one of one: ", 0);
            Blt_FormatSpecOptions(interp, columnSpecs);
            return TCL_ERROR;
        }
    }

    for (i = 4; i < start; i++) {
	if (Blt_TreeViewGetColumn(NULL, tvPtr, objv[i], &columnPtr) == TCL_OK) {
	    Tcl_AppendResult(interp, "column \"", Tcl_GetString(objv[i]), 
		"\" already exists", (char *)NULL);
	    return TCL_ERROR;
	}
	columnPtr = CreateColumn(tvPtr, objv[i], nOptions, options);
	if (columnPtr == NULL) {
	    return TCL_ERROR;
	}
	if (beforePtr == NULL) {
	    columnPtr->linkPtr = Blt_ChainAppend(tvPtr->colChainPtr, columnPtr);
	} else {
	    columnPtr->linkPtr = Blt_ChainNewLink();
	    Blt_ChainSetValue(columnPtr->linkPtr, columnPtr);
	    Blt_ChainLinkBefore(tvPtr->colChainPtr, columnPtr->linkPtr, 
		beforePtr);
	}
	Tcl_AppendResult(interp, i>4?" ":"", columnPtr->key, 0);
	/* 
	 * Traverse the tree adding column entries where needed.
	 */
	for(entryPtr = tvPtr->rootPtr; entryPtr != NULL;
	    entryPtr = Blt_TreeViewNextEntry(entryPtr, 0)) {
	    Blt_TreeViewAddValue(entryPtr, columnPtr);
	}
	Blt_TreeViewTraceColumn(tvPtr, columnPtr);
    }
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}



/*
 *----------------------------------------------------------------------
 *
 * ColumnCurrentOp --
 *
 *	Make the rule to appear active.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnCurrentOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;	/* Not used. */
{
    ClientData context;
    TreeViewColumn *columnPtr;

    columnPtr = NULL;
    context = Blt_GetCurrentContext(tvPtr->bindTable);
    if ((context == ITEM_COLUMN_TITLE) || (context == ITEM_COLUMN_RULE)) {
	columnPtr = Blt_GetCurrentItem(tvPtr->bindTable);
    }
    if (context >= ITEM_STYLE) {
	TreeViewValue *valuePtr = context;
	
	columnPtr = valuePtr->columnPtr;
    }
    if (columnPtr != NULL) {
	Tcl_SetResult(interp, columnPtr->key, TCL_VOLATILE);
    }
    return TCL_OK;
}

static void
ColumnPercentSubst(tvPtr, columnPtr, command, resultPtr)
    TreeView *tvPtr;
    TreeViewColumn *columnPtr;
    char *command;
    Tcl_DString *resultPtr;
{
    register char *last, *p;
    int one = (command[0] == '%' && strlen(command)==2);

    /*
     * Get the full path name of the node, in case we need to
     * substitute for it.
     */
    Tcl_DStringInit(resultPtr);
    /* Append the widget name and the node .t 0 */
    for (last = p = command; *p != '\0'; p++) {
	if (*p == '%') {
	    char *string;
	    char buf[3];

	    if (p > last) {
		*p = '\0';
		Tcl_DStringAppend(resultPtr, last, -1);
		*p = '%';
	    }
	    switch (*(p + 1)) {
	    case '%':		/* Percent sign */
		string = "%";
		break;
	    case 'W':		/* Widget name */
		string = Tk_PathName(tvPtr->tkwin);
		break;
	    case 'C':		/* Node identifier */
		string = columnPtr->key;
                if (one) {
                    Tcl_DStringAppend(resultPtr, string, -1);
                } else {
                    Tcl_DStringAppendElement(resultPtr, string);
                }
                p++;
                last = p + 1;
                continue;
                break;
	    default:
		if (*(p + 1) == '\0') {
		    p--;
		}
		buf[0] = *p, buf[1] = *(p + 1), buf[2] = '\0';
		string = buf;
		break;
	    }
	    Tcl_DStringAppend(resultPtr, string, -1);
	    p++;
	    last = p + 1;
	}
    }
    if (p > last) {
	*p = '\0';
	Tcl_DStringAppend(resultPtr, last, -1);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * ColumnInvokeOp --
 *
 * 	This procedure is called to invoke a column command.
 *
 *	  .h column invoke columnName
 *
 * Results:
 *	A standard Tcl result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnInvokeOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;		/* Not used. */
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewColumn *columnPtr;
    char *string;

    string = Tcl_GetString(objv[3]);
    if (string[0] == '\0') {
	return TCL_OK;
    }
    if (Blt_TreeViewGetColumn(interp, tvPtr, objv[3], &columnPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((columnPtr->state == STATE_NORMAL) && (columnPtr->titleCmd != NULL)) {
        Tcl_DString dString;
        int result;

        Tcl_DStringInit(&dString);
        ColumnPercentSubst(tvPtr, columnPtr, columnPtr->titleCmd, &dString);
        Tcl_Preserve(tvPtr);
	Tcl_Preserve(columnPtr);
        result = Tcl_GlobalEval(interp, Tcl_DStringValue(&dString));
	Tcl_Release(columnPtr);
	Tcl_Release(tvPtr);
	Tcl_DStringFree(&dString);
	return result;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ColumnValuesOp --
 *
 * 	This procedure is called to get all column values.
 *
 *	  .h column values ?-visible? ?-default value? columnName ?start? ?end?
 *
 * Results:
 *	A standard Tcl result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnValuesOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;		/* Not used. */
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewColumn *columnPtr;
    TreeViewEntry *entryPtr, *lastPtr = NULL, *firstPtr = NULL;
    int isTree;
    int mask;
    Tcl_Obj *listObjPtr, *objPtr, *defObj;
    char *string;

    mask = 0;
    defObj = NULL;
    while (objc>4) {
        string = Tcl_GetString(objv[3]);
        if (string[0] == '-' && strcmp(string, "-visible") == 0) {
            mask = ENTRY_MASK;
            objv++;
            objc--;
        } else if (string[0] == '-' && strcmp(string, "-default") == 0) {
            defObj = objv[4];
            objv += 2;
            objc -= 2;
        } else {
            break;
        }
    }
    if (objc>6) {
        Tcl_AppendResult(interp, "too many args", 0);
        return TCL_ERROR;
    }
    if (Blt_TreeViewGetColumn(interp, tvPtr, objv[3], &columnPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    isTree = (columnPtr == &tvPtr->treeColumn);

    if (objc > 4 && Blt_TreeViewGetEntry(tvPtr, objv[4], &firstPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc > 5 && Blt_TreeViewGetEntry(tvPtr, objv[5], &lastPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (firstPtr == NULL) {
        firstPtr = tvPtr->rootPtr;
    }
    if (mask && firstPtr == tvPtr->rootPtr) {
        if ((tvPtr->flags & TV_HIDE_ROOT)) {
            firstPtr = Blt_TreeViewNextEntry(firstPtr, mask);
        }
    } else if (mask  && (firstPtr->flags & mask)) {
        firstPtr = Blt_TreeViewNextEntry(firstPtr, mask);
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    
    for (entryPtr = firstPtr; entryPtr != NULL; ) {
        if (!isTree) {
            if (Blt_TreeGetValueByKey(NULL, tvPtr->tree, entryPtr->node, 
                columnPtr->key, &objPtr) != TCL_OK) {
                if (defObj == NULL) {
                    objPtr = Tcl_NewStringObj("",0);
                } else {
                    objPtr = defObj;
                }
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            } else {
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            }
        } else {
            objPtr = Tcl_NewStringObj(Blt_TreeNodeLabel(entryPtr->node),-1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
        if (lastPtr && entryPtr == lastPtr) break;
        entryPtr = Blt_TreeViewNextEntry(entryPtr, mask);
    }
    Tcl_SetObjResult(interp, listObjPtr);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ColumnMoveOp --
 *
 *	Move a column.
 *
 * .h column move field1 position
 * NOT IMPLEMENTED;
 *----------------------------------------------------------------------
 */
static int
ColumnMoveOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewColumn *columnPtr, *beforeColumn;
    Blt_ChainLink *beforePtr;
    char *string;
    
    if (Blt_TreeViewGetColumn(interp, tvPtr, objv[3], &columnPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (columnPtr->linkPtr == NULL) {
        return TCL_OK;
    }
    string = Tcl_GetString(objv[4]);
    if (!strcmp(string, "end")) {
        beforePtr = NULL;
    } else if (Blt_TreeViewGetColumn(interp, tvPtr, objv[4], &beforeColumn) != TCL_OK) {
        return TCL_ERROR;
        
    } else {
        beforePtr = beforeColumn->linkPtr;
    }
    if (beforePtr == columnPtr->linkPtr) {
        return TCL_OK;
    }
    Blt_ChainUnlinkLink(tvPtr->colChainPtr, columnPtr->linkPtr);
    Blt_ChainLinkBefore(tvPtr->colChainPtr, columnPtr->linkPtr, beforePtr);
    tvPtr->flags |= (TV_DIRTY | TV_LAYOUT);
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ColumnNamesOp --
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnNamesOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;		/* Not used. */
{
    Blt_ChainLink *linkPtr;
    Tcl_Obj *listObjPtr, *objPtr;
    TreeViewColumn *columnPtr;
    int vis;
    char *pattern = NULL;

    vis = 0;
    if (objc > 3) {
        if (strcmp("-visible", Tcl_GetString(objv[3]))) {
            if (objc>4) {
                Tcl_AppendResult( interp, "expected -visible", (char*)NULL);
                return TCL_ERROR;
            } else {
                pattern = Tcl_GetString(objv[3]);
            }
        } else {
            vis =  1;
            if (objc>4) {
                pattern = Tcl_GetString(objv[4]);
            }
        }
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for(linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); linkPtr != NULL;
	linkPtr = Blt_ChainNextLink(linkPtr)) {
	columnPtr = Blt_ChainGetValue(linkPtr);
	if (vis && columnPtr->hidden) {
	    continue;
	}
	if (pattern != NULL && !Tcl_StringMatch(columnPtr->key, pattern)) {
	    continue;
	}
	objPtr = Tcl_NewStringObj(columnPtr->key, -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*ARGSUSED*/
static int
ColumnNearestOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    int x, y;			/* Screen coordinates of the test point. */
    TreeViewColumn *columnPtr;
    ClientData context;
    int checkTitle;
#ifdef notdef
    int isRoot;

    isRoot = FALSE;
    string = Tcl_GetString(objv[3]);

    if (strcmp("-root", string) == 0) {
	isRoot = TRUE;
	objv++, objc--;
    }
    if (objc != 5) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " ", Tcl_GetString(objv[1]), 
		Tcl_GetString(objv[2]), " ?-root? x y\"", (char *)NULL);
	return TCL_ERROR;
			 
    }
#endif
    if (Tk_GetPixelsFromObj(interp, tvPtr->tkwin, objv[3], &x) != TCL_OK) {
	return TCL_ERROR;
    } 
    y = 0;
    checkTitle = FALSE;
    if (objc == 5) {
	if (Tk_GetPixelsFromObj(interp, tvPtr->tkwin, objv[4], &y) != TCL_OK) {
	    return TCL_ERROR;
	}
	checkTitle = TRUE;
    }
    columnPtr = Blt_TreeViewNearestColumn(tvPtr, x, y, &context);
    if ((checkTitle) && (context == NULL)) {
	columnPtr = NULL;
    }
    if (columnPtr != NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(columnPtr->key,-1));
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
ColumnOffsetsOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    Tcl_Obj *listPtr;
    Blt_ChainLink *linkPtr;
    TreeViewColumn *columnPtr;
    int x;
    int vis;

    vis = 0;
    if (objc > 3) {
        if (strcmp("-visible", Tcl_GetString(objv[3]))) {
            Tcl_AppendResult( interp, "expected -visible", (char*)NULL);
            return TCL_ERROR;
        }
        vis =  1;
    }
    
    listPtr = Tcl_NewListObj(0,0);
    for (linkPtr = Blt_ChainFirstLink(tvPtr->colChainPtr); 
        linkPtr != NULL; linkPtr = Blt_ChainNextLink(linkPtr)) {
        columnPtr = Blt_ChainGetValue(linkPtr);
        if (vis && columnPtr->hidden) {
            continue;
        }
        x = SCREENX(tvPtr, columnPtr->worldX);
        Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewIntObj(x));
    }
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;

}

/* .t column bbox field entry */
/*ARGSUSED*/
static int
ColumnBboxOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    Tcl_Obj *listPtr;
    TreeViewColumn *colPtr;
    TreeViewEntry *entryPtr = NULL;
    int x, y, w, h, visible = 0;
    const char *string;
    int mw, mh;
    mw = Tk_Width(tvPtr->tkwin) - tvPtr->padX;
    mh = Tk_Height(tvPtr->tkwin) - tvPtr->padY;
    
    if (objc == 6) {
        string = Tcl_GetString(objv[3]);
        if (strcmp("-visible", string)) {
            Tcl_AppendResult(interp, "expected -visible", 0);
            return TCL_ERROR;
        }
        visible = 1;
        objc--;
        objv++;
    }
    if (objc != 5) {
        Tcl_AppendResult(interp, "missing args", 0);
        return TCL_ERROR;
    }
    if (Blt_TreeViewGetColumn(interp, tvPtr, objv[3], &colPtr) != TCL_OK ||
        colPtr == NULL) {
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[4]);
    if (!strcmp(string, "-1")) {
    } else if (Blt_TreeViewGetEntry(tvPtr, objv[4], &entryPtr) != TCL_OK ||
        entryPtr == NULL) {
        return TCL_ERROR;
    }
    if (tvPtr->flags & TV_LAYOUT) {
        Blt_TreeViewComputeLayout(tvPtr);
    }
    if (entryPtr == NULL) {
        if (!(tvPtr->flags & TV_SHOW_COLUMN_TITLES)) return TCL_OK;
        listPtr = Tcl_NewListObj(0,0);
        x = SCREENX(tvPtr, colPtr->worldX);
        y = (tvPtr->yOffset + tvPtr->insetY);
        w = colPtr->width;
        h = tvPtr->titleHeight;
        if (visible) {
            if ((x+w) > mw) {
                w = (mw-x-2);
            }
            if ((y+h) > mh) {
                w = (mh-y-2);
            }
        }
        Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewIntObj(x));
        Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewIntObj(y));
        Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewIntObj(w));
        Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewIntObj(h));
        Tcl_SetObjResult(interp, listPtr);
        return TCL_OK;
    }
    if (Blt_TreeViewEntryIsHidden(entryPtr)) {
        return TCL_OK;
    }
    listPtr = Tcl_NewListObj(0,0);
    x = SCREENX(tvPtr, colPtr->worldX);
    y = SCREENY(tvPtr, entryPtr->worldY);
    w = colPtr->width;
    h = entryPtr->height;
    if (visible) {
        if ((x+w) > mw) {
            w = (mw-x-2);
        }
        if ((y+h) > mh) {
            w = (mh-y-2);
        }
    }
    Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewIntObj(x));
    Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewIntObj(y));
    Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewIntObj(w));
    Tcl_ListObjAppendElement(interp, listPtr, Tcl_NewIntObj(h));
    Tcl_SetObjResult(interp, listPtr);
    return TCL_OK;

}

/*ARGSUSED*/
static int
ColumnSeeOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;		/* Not used. */
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewColumn *colPtr;
    int width;
    Tk_Anchor anchor;
    int left, right;
    char *string;

    string = Tcl_GetString(objv[3]);
    anchor = TK_ANCHOR_W;	/* Default anchor is West */
    if ((string[0] == '-') && (strcmp(string, "-anchor") == 0)) {
	if (objc == 4) {
	    Tcl_AppendResult(interp, "missing \"-anchor\" argument",
		(char *)NULL);
	    return TCL_ERROR;
	}
	if (Tk_GetAnchorFromObj(interp, objv[4], &anchor) != TCL_OK) {
	    return TCL_ERROR;
	}
	objc -= 2, objv += 2;
    }
    if (objc != 4) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
            "see ?-anchor anchor? tagOrId\"", (char *)NULL);
            return TCL_ERROR;
    }
    if (Blt_TreeViewGetColumn(interp, tvPtr, objv[3], &colPtr) != TCL_OK ||
        colPtr == NULL) {
        return TCL_ERROR;
    }
    if (colPtr->hidden) {
        return TCL_OK;
    }
    width = VPORTWIDTH(tvPtr);

    left = tvPtr->xOffset;
    right = tvPtr->xOffset + width;

    if (colPtr->worldX >= left && (colPtr->worldX + colPtr->width) <= right) {
        return TCL_OK;
    }
    if (colPtr->worldX < left) {
        tvPtr->xOffset = colPtr->worldX;
    } else {
        tvPtr->xOffset = colPtr->worldX;
    }
    tvPtr->flags |= TV_XSCROLL;
        
#if 0
    switch (anchor) {
    case TK_ANCHOR_W:
    case TK_ANCHOR_NW:
    case TK_ANCHOR_SW:
	x = 0;
	break;
    case TK_ANCHOR_E:
    case TK_ANCHOR_NE:
    case TK_ANCHOR_SE:
	x = entryPtr->worldX + entryPtr->width + 
	    ICONWIDTH(DEPTH(tvPtr, entryPtr->node)) - width;
	break;
    default:
	if (entryPtr->worldX < left) {
	    x = entryPtr->worldX;
	} else if ((entryPtr->worldX + entryPtr->width) > right) {
	    x = entryPtr->worldX + entryPtr->width - width;
	} else {
	    x = tvPtr->xOffset;
	}
	break;
    }
    if (x != tvPtr->xOffset) {
	tvPtr->xOffset = x;
	tvPtr->flags |= TV_XSCROLL;
    }
#endif
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

static void
UpdateMark(tvPtr, newMark)
    TreeView *tvPtr;
    int newMark;
{
    Drawable drawable;
    TreeViewColumn *columnPtr;
    int dx;
    int width;

    columnPtr = tvPtr->resizeColumnPtr;
    if (columnPtr == NULL) {
	return;
    }
    drawable = Tk_WindowId(tvPtr->tkwin);
    if (drawable == None) {
	return;
    }

    /* Erase any existing rule. */
    if (tvPtr->flags & TV_RULE_ACTIVE) { 
	Blt_TreeViewDrawRule(tvPtr, columnPtr, drawable);
    }
    
    dx = newMark - tvPtr->ruleAnchor; 
    width = columnPtr->width - 
	(PADDING(columnPtr->pad) + 2 * columnPtr->borderWidth);
    if ((columnPtr->reqMin > 0) && ((width + dx) < columnPtr->reqMin)) {
	dx = columnPtr->reqMin - width;
    }
    if ((columnPtr->reqMax > 0) && ((width + dx) > columnPtr->reqMax)) {
	dx = columnPtr->reqMax - width;
    }
    if ((width + dx) < 4) {
	dx = 4 - width;
    }
    tvPtr->ruleMark = tvPtr->ruleAnchor + dx;

    /* Redraw the rule if required. */
    if (tvPtr->flags & TV_RULE_NEEDED) {
	Blt_TreeViewDrawRule(tvPtr, columnPtr, drawable);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ResizeActivateOp --
 *
 *	Turns on/off the resize cursor.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ResizeActivateOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewColumn *columnPtr;
    char *string;

    string = Tcl_GetString(objv[4]);
    if (string[0] == '\0') {
	if (tvPtr->cursor != None) {
	    Tk_DefineCursor(tvPtr->tkwin, tvPtr->cursor);
	} else {
	    Tk_UndefineCursor(tvPtr->tkwin);
	}
	tvPtr->resizeColumnPtr = NULL;
    } else if (Blt_TreeViewGetColumn(interp, tvPtr, objv[4], &columnPtr) 
	       == TCL_OK) {
	if (tvPtr->resizeCursor != None) {
	    Tk_DefineCursor(tvPtr->tkwin, tvPtr->resizeCursor);
	} 
	tvPtr->resizeColumnPtr = columnPtr;
    } else {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ResizeAnchorOp --
 *
 *	Set the anchor for the resize.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ResizeAnchorOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    int x;

    if (Tcl_GetIntFromObj(NULL, objv[4], &x) != TCL_OK) {
	return TCL_ERROR;
    } 
    tvPtr->ruleAnchor = x;
    tvPtr->flags |= TV_RULE_NEEDED;
    UpdateMark(tvPtr, x);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ResizeMarkOp --
 *
 *	Sets the resize mark.  The distance between the mark and the anchor
 *	is the delta to change the width of the active column.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ResizeMarkOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    int x;

    if (Tcl_GetIntFromObj(NULL, objv[4], &x) != TCL_OK) {
	return TCL_ERROR;
    } 
    tvPtr->flags |= TV_RULE_NEEDED;
    UpdateMark(tvPtr, x);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ResizeSetOp --
 *
 *	Returns the new width of the column including the resize delta.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ResizeSetOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;	/* Not used. */
{
    tvPtr->flags &= ~TV_RULE_NEEDED;
    UpdateMark(tvPtr, tvPtr->ruleMark);
    if (tvPtr->resizeColumnPtr != NULL) {
	int width, delta;
	TreeViewColumn *columnPtr;

	columnPtr = tvPtr->resizeColumnPtr;
	delta = (tvPtr->ruleMark - tvPtr->ruleAnchor);
	width = tvPtr->resizeColumnPtr->width + delta - 
	    (PADDING(columnPtr->pad) + 2 * columnPtr->borderWidth) - 1;
	Tcl_SetObjResult(interp, Tcl_NewIntObj(width));
    }
    return TCL_OK;
}

static Blt_OpSpec resizeOps[] =
{ 
    {"activate", 2, (Blt_Op)ResizeActivateOp, 5, 5, "column"},
    {"anchor", 2, (Blt_Op)ResizeAnchorOp, 5, 5, "x"},
    {"mark", 1, (Blt_Op)ResizeMarkOp, 5, 5, "x"},
    {"set", 1, (Blt_Op)ResizeSetOp, 4, 4, "",},
};

static int nResizeOps = sizeof(resizeOps) / sizeof(Blt_OpSpec);

/*
 *----------------------------------------------------------------------
 *
 * ColumnResizeOp --
 *
 *----------------------------------------------------------------------
 */
static int
ColumnResizeOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    Blt_Op proc;
    int result;

    proc = Blt_GetOpFromObj(interp, nResizeOps, resizeOps, BLT_OP_ARG3, 
	objc, objv,0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (tvPtr, interp, objc, objv);
    return result;
}


static Blt_OpSpec columnOps[] =
{
    {"activate", 1, (Blt_Op)ColumnActivateOp, 3, 4, "?field?",},
    {"bbox", 2, (Blt_Op)ColumnBboxOp, 5, 6, "?-visible? field entry",},
    {"bind", 2, (Blt_Op)ColumnBindOp, 4, 6, "tagName ?sequence command?",},
    {"cget", 2, (Blt_Op)ColumnCgetOp, 5, 5, "field option",},
    {"configure", 2, (Blt_Op)ColumnConfigureOp, 4, 0, 
	"field ?option value?...",},
    {"current", 2, (Blt_Op)ColumnCurrentOp, 3, 3, "",},
    {"delete", 1, (Blt_Op)ColumnDeleteOp, 3, 0, "?field...?",},
    {"index", 3, (Blt_Op)ColumnIndexOp, 4, 4, "field", },
    {"insert", 3, (Blt_Op)ColumnInsertOp, 5, 0, 
	"position field ?field...? ?option value?...",},
    {"invoke", 3, (Blt_Op)ColumnInvokeOp, 4, 4, "field",},
    {"isset", 3, (Blt_Op)ColumnIssetOp, 3, 5, "?startOrTag? ?end?", },
    {"istree", 3, (Blt_Op)ColumnIstreeOp, 4, 4, "field", },
    {"move", 1, (Blt_Op)ColumnMoveOp, 5, 5, "src dest",},
    {"names", 2, (Blt_Op)ColumnNamesOp, 3, 5, "?-visible? ?PATTERN?",},
    {"nearest", 2, (Blt_Op)ColumnNearestOp, 4, 5, "x ?y?",},
    {"offsets", 2, (Blt_Op)ColumnOffsetsOp, 3, 4, "?-visible?",},
    {"resize", 1, (Blt_Op)ColumnResizeOp, 3, 0, "arg ...",},
    {"see", 1, (Blt_Op)ColumnSeeOp, 4, 6, "field ?-anchor pos?",},
    {"values", 1, (Blt_Op)ColumnValuesOp, 4, 9, "?-visible? ?-default value? field ?startOrTag? ?end?",},
    };
static int nColumnOps = sizeof(columnOps) / sizeof(Blt_OpSpec);

/*
 *----------------------------------------------------------------------
 *
 * Blt_TreeViewColumnOp --
 *
 *----------------------------------------------------------------------
 */
int
Blt_TreeViewColumnOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    Blt_Op proc;
    int result;

    proc = Blt_GetOpFromObj(interp, nColumnOps, columnOps, BLT_OP_ARG2, 
	objc, objv,0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (tvPtr, interp, objc, objv);
    return result;
}


static int
InvokeCompare(tvPtr, e1Ptr, e2Ptr, command)
    TreeView *tvPtr;
    TreeViewEntry *e1Ptr, *e2Ptr;
    char *command;
{
    int result;
    Tcl_Obj *objv[8];
    int i;

    objv[0] = Tcl_NewStringObj(command, -1);
    objv[1] = Tcl_NewStringObj(Tk_PathName(tvPtr->tkwin), -1);
    objv[2] = Tcl_NewIntObj(Blt_TreeNodeId(e1Ptr->node));
    objv[3] = Tcl_NewIntObj(Blt_TreeNodeId(e2Ptr->node));
    objv[4] = Tcl_NewStringObj(tvPtr->sortColumnPtr->key, -1);
	     
    if (tvPtr->flatView) {
	objv[5] = Tcl_NewStringObj(e1Ptr->fullName, -1);
	objv[6] = Tcl_NewStringObj(e2Ptr->fullName, -1);
    } else {
	objv[5] = Tcl_NewStringObj(GETLABEL(e1Ptr), -1);
	objv[6] = Tcl_NewStringObj(GETLABEL(e2Ptr), -1);
    }
    for(i = 0; i < 7; i++) {
	Tcl_IncrRefCount(objv[i]);
    }
    objv[7] = NULL;
    result = Tcl_EvalObjv(tvPtr->interp, 7, objv, TCL_EVAL_GLOBAL);
    if ((result != TCL_OK) ||
	(Tcl_GetIntFromObj(tvPtr->interp, Tcl_GetObjResult(tvPtr->interp), 
			   &result) != TCL_OK)) {
	Tcl_BackgroundError(tvPtr->interp);
    }
    for(i = 0; i < 7; i++) {
	Tcl_DecrRefCount(objv[i]);
    }
    Tcl_ResetResult(tvPtr->interp);
    return result;
}

static TreeView *treeViewInstance;

static int
CompareEntry( CONST void *a, CONST void *b, Tcl_Obj *colName)
{
    TreeView *tvPtr;
    TreeViewColumn *columnPtr;
    TreeViewEntry *e1Ptr, **e1PtrPtr = (TreeViewEntry **)a;
    TreeViewEntry *e2Ptr, **e2PtrPtr = (TreeViewEntry **)b;
    Tcl_Obj *obj1, *obj2;
    char *s1, *s2;
    int result, sType;

    tvPtr = (*e1PtrPtr)->tvPtr;
    columnPtr = tvPtr->sortColumnPtr;
    sType = tvPtr->sortType;
    
    obj1 = (*e1PtrPtr)->dataObjPtr;
    obj2 = (*e2PtrPtr)->dataObjPtr;
    if (colName != NULL) {
        if (Blt_TreeViewGetColumn(NULL, tvPtr, colName, &columnPtr)
            != TCL_OK) {
            return 1;
        }
        e1Ptr = *e1PtrPtr;
        e2Ptr = *e2PtrPtr;
        if (Blt_TreeGetValueByKey(tvPtr->interp, tvPtr->tree,
            e1Ptr->node, columnPtr->key, &obj1) != TCL_OK) {
            return 1;
        }
        if (Blt_TreeGetValueByKey(tvPtr->interp, tvPtr->tree,
            e2Ptr->node, columnPtr->key, &obj2) != TCL_OK) {
                return 1;
        }
        sType = columnPtr->sortType;
    }
    s1 = Tcl_GetString(obj1);
    s2 = Tcl_GetString(obj2);
    result = 0;
    switch (sType) {
    case SORT_TYPE_ASCII:
	result = strcmp(s1, s2);
	break;

    case SORT_TYPE_COMMAND:
	{
	    char *cmd;

	    cmd = columnPtr->sortCmd;
	    if (cmd == NULL) {
		cmd = tvPtr->sortCmd;
	    }
	    if (cmd == NULL) {
		result = Blt_DictionaryCompare(s1, s2);
	    } else {
		result = InvokeCompare(tvPtr, *e1PtrPtr, *e2PtrPtr, cmd);
	    }
	}
	break;

    case SORT_TYPE_DICTIONARY:
	result = Blt_DictionaryCompare(s1, s2);
	break;

    case SORT_TYPE_INTEGER:
	{
	    int i1, i2;

	    if (Tcl_GetIntFromObj(NULL, obj1, &i1)==TCL_OK) {
		if (Tcl_GetIntFromObj(NULL, obj2, &i2) == TCL_OK) {
		    result = i1 - i2;
		} else {
		    result = -1;
		} 
	    } else if (Tcl_GetIntFromObj(NULL, obj2, &i2) == TCL_OK) {
		result = 1;
	    } else {
		result = Blt_DictionaryCompare(s1, s2);
	    }
	}
	break;

    case SORT_TYPE_REAL:
	{
	    double r1, r2;

	    if (Tcl_GetDoubleFromObj(NULL, obj1, &r1) == TCL_OK) {
		if (Tcl_GetDoubleFromObj(NULL, obj2, &r2) == TCL_OK) {
		    result = (r1 < r2) ? -1 : (r1 > r2) ? 1 : 0;
		} else {
		    result = -1;
		} 
	    } else if (Tcl_GetDoubleFromObj(NULL, obj2, &r2) == TCL_OK) {
		result = 1;
	    } else {
		result = Blt_DictionaryCompare(s1, s2);
	    }
	}
	break;
    }
    if (tvPtr->sortDecreasing) {
	return -result;
    } 
    return result;
}

static int
CompareEntries(a, b)
    CONST void *a, *b;
{
    TreeView *tvPtr;
    int result, i;
    TreeViewEntry **e1PtrPtr = (TreeViewEntry **)a;
    int objc;
    Tcl_Obj **objv;
    
    result = CompareEntry(a, b, NULL);
    
    tvPtr = (*e1PtrPtr)->tvPtr;
    if (result != 0) { return result; }
    if (result != 0 || tvPtr->sortColumnPtr == NULL) return result;
    if (tvPtr->sortColumnPtr->sortAltColumns == NULL) return result;
		
    if (Tcl_ListObjGetElements(NULL, tvPtr->sortColumnPtr->sortAltColumns,
        &objc, &objv) != TCL_OK) {
            return result;
    }
    for (i = 0; i < objc && result == 0; i++) {
        result = CompareEntry(a, b, objv[i]);
    }
    
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * CompareNodes --
 *
 *	Comparison routine (used by qsort) to sort a chain of subnodes.
 *
 * Results:
 *	1 is the first is greater, -1 is the second is greater, 0
 *	if equal.
 *
 *----------------------------------------------------------------------
 */
static int
CompareNodes( Blt_TreeNode *n1Ptr, Blt_TreeNode *n2Ptr )
{
    TreeView *tvPtr = treeViewInstance;
    TreeViewEntry *e1Ptr, *e2Ptr;
    
    e1Ptr = Blt_NodeToEntry(tvPtr, *n1Ptr);
    e2Ptr = Blt_NodeToEntry(tvPtr, *n2Ptr);

    TreeViewColumn *columnPtr = tvPtr->sortColumnPtr;

    /* Fetch the data for sorting. */
    if (tvPtr->sortType == SORT_TYPE_COMMAND) {
	e1Ptr->dataObjPtr = Tcl_NewIntObj(Blt_TreeNodeId(*n1Ptr));
	e2Ptr->dataObjPtr = Tcl_NewIntObj(Blt_TreeNodeId(*n2Ptr));
    } else if (columnPtr == &tvPtr->treeColumn) {
	Tcl_DString dString;

	Tcl_DStringInit(&dString);
	if (e1Ptr->fullName == NULL) {
	    Blt_TreeViewGetFullName(tvPtr, e1Ptr, TRUE, &dString);
	    e1Ptr->fullName = Blt_Strdup(Tcl_DStringValue(&dString));
	}
	e1Ptr->dataObjPtr = Tcl_NewStringObj(e1Ptr->fullName, -1);
	if (e2Ptr->fullName == NULL) {
	    Blt_TreeViewGetFullName(tvPtr, e2Ptr, TRUE, &dString);
	    e2Ptr->fullName = Blt_Strdup(Tcl_DStringValue(&dString));
	}
	e2Ptr->dataObjPtr = Tcl_NewStringObj(e2Ptr->fullName, -1);
	Tcl_DStringFree(&dString);
    } else {
	Blt_TreeKey key;
	Tcl_Obj *objPtr;

	key = columnPtr->key;
	if (Blt_TreeViewGetData(e1Ptr, key, &objPtr) != TCL_OK) {
	    e1Ptr->dataObjPtr = Tcl_NewStringObj("",-1);
	} else {
	    e1Ptr->dataObjPtr = objPtr;
	}
	if (Blt_TreeViewGetData(e2Ptr, key, &objPtr) != TCL_OK) {
            e2Ptr->dataObjPtr = Tcl_NewStringObj("",-1);
	} else {
	    e2Ptr->dataObjPtr = objPtr;
	}
    }
    return CompareEntries(&e1Ptr, &e2Ptr);
}

static int
SortAutoOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;		/* Not used. */
    int objc;
    Tcl_Obj *CONST *objv;
{

    if (objc == 4) {
	int bool;
	int isAuto;

	isAuto = ((tvPtr->flags & TV_SORT_AUTO) != 0);
	if (Tcl_GetBooleanFromObj(interp, objv[3], &bool) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (isAuto != bool) {
	    tvPtr->flags |= (TV_LAYOUT | TV_DIRTY | TV_RESORT);
	    Blt_TreeViewEventuallyRedraw(tvPtr);
	}
	if (bool) {
	    tvPtr->flags |= TV_SORT_AUTO;
	} else {
	    tvPtr->flags &= ~TV_SORT_AUTO;
	}
    }
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(tvPtr->flags & TV_SORT_AUTO));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SortCgetOp --
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SortCgetOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    Blt_TreeViewOptsInit(tvPtr);
    return Blt_ConfigureValueFromObj(interp, tvPtr->tkwin, sortSpecs, 
	(char *)tvPtr, objv[3], 0);
}

/*
 *----------------------------------------------------------------------
 *
 * SortConfigureOp --
 *
 * 	This procedure is called to process a list of configuration
 *	options database, in order to reconfigure the one of more
 *	entries in the widget.
 *
 *	  .h sort configure option value
 *
 * Results:
 *	A standard Tcl result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for tvPtr; old resources get freed, if there
 *	were any.  The hypertext is redisplayed.
 *
 *----------------------------------------------------------------------
 */
static int
SortConfigureOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    int oldType;
    char *oldCommand;
    TreeViewColumn *oldColumn;

    Blt_TreeViewOptsInit(tvPtr);
    if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, tvPtr->tkwin, sortSpecs, 
		(char *)tvPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, tvPtr->tkwin, sortSpecs, 
		(char *)tvPtr, objv[3], 0);
    }
    oldColumn = tvPtr->sortColumnPtr;
    oldType = tvPtr->sortType;
    oldCommand = tvPtr->sortCmd;
    if (Blt_ConfigureWidgetFromObj(interp, tvPtr->tkwin, sortSpecs, 
	objc - 3, objv + 3, (char *)tvPtr, BLT_CONFIG_OBJV_ONLY, NULL) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((oldColumn != tvPtr->sortColumnPtr) ||
	(oldType != tvPtr->sortType) ||
	(oldCommand != tvPtr->sortCmd)) {
	tvPtr->flags &= ~TV_SORTED;
	tvPtr->flags |= (TV_DIRTY | TV_RESORT);
    } 
    if (tvPtr->flags & TV_SORT_AUTO) {
	tvPtr->flags |= TV_SORT_PENDING;
    }
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

/*ARGSUSED*/
static int
SortOnceOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;		/* Not used. */
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    int recurse, result;
    register int i;

    recurse = FALSE;
    if (objc > 3) {
	char *string;
	int length;

	string = Tcl_GetStringFromObj(objv[3], &length);
	if ((string[0] == '-') && (length > 1) &&
	    (strncmp(string, "-recurse", length) == 0)) {
	    objv++, objc--;
	    recurse = TRUE;
	}
    }
    if (tvPtr->sortColumnPtr == NULL) {
        Tcl_AppendResult(interp, "must select column to sort by", 0);
        return TCL_ERROR;
    }
    for (i = 3; i < objc; i++) {
	if (Blt_TreeViewGetEntry(tvPtr, objv[i], &entryPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (recurse) {
	    result = Blt_TreeApply(entryPtr->node, SortApplyProc, tvPtr);
	} else {
	    result = SortApplyProc(entryPtr->node, tvPtr, TREE_PREORDER);
	}
	if (result != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    tvPtr->flags |= TV_LAYOUT;
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_TreeViewSortOp --
 *
 *	Comparison routine (used by qsort) to sort a chain of subnodes.
 *	A simple string comparison is performed on each node name.
 *
 *	.h sort auto
 *	.h sort once -recurse root
 *
 * Results:
 *	1 is the first is greater, -1 is the second is greater, 0
 *	if equal.
 *
 *----------------------------------------------------------------------
 */
static Blt_OpSpec sortOps[] =
{
    {"auto", 1, (Blt_Op)SortAutoOp, 3, 4, "?boolean?",},
    {"cget", 2, (Blt_Op)SortCgetOp, 4, 4, "option",},
    {"configure", 2, (Blt_Op)SortConfigureOp, 3, 0, "?option value?...",},
    {"once", 1, (Blt_Op)SortOnceOp, 3, 0, "?-recurse? node...",},
};
static int nSortOps = sizeof(sortOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
int
Blt_TreeViewSortOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;		/* Not used. */
    int objc;
    Tcl_Obj *CONST *objv;
{
    Blt_Op proc;
    int result;

    proc = Blt_GetOpFromObj(interp, nSortOps, sortOps, BLT_OP_ARG2, objc, 
	    objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (tvPtr, interp, objc, objv);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * SortApplyProc --
 *
 *	Sorts the subnodes at a given node.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SortApplyProc(node, clientData, order)
    Blt_TreeNode node;
    ClientData clientData;
    int order;			/* Not used. */
{
    TreeView *tvPtr = clientData;

    if (!Blt_TreeIsLeaf(node)) {
        treeViewInstance = tvPtr;
        Blt_TreeSortNode(tvPtr->tree, node, CompareNodes);
    }
    return TCL_OK;
}
 
/*
 *----------------------------------------------------------------------
 *
 * Blt_TreeViewSortFlatView --
 *
 *	Sorts the flatten array of entries.
 *
 *----------------------------------------------------------------------
 */
void
Blt_TreeViewSortFlatView(tvPtr)
    TreeView *tvPtr;
{
    TreeViewEntry *entryPtr, **p;

    tvPtr->flags &= ~TV_SORT_PENDING;
    if ((tvPtr->sortType == SORT_TYPE_NONE) || (tvPtr->sortColumnPtr == NULL) ||
	(tvPtr->nEntries == 1)) {
	return;
    }
    if (tvPtr->flags & TV_SORTED) {
	int first, last;
	TreeViewEntry *hold;

	if (tvPtr->sortDecreasing == tvPtr->viewIsDecreasing) {
	    return;
	}

	/* 
	 * The view is already sorted but in the wrong direction. 
	 * Reverse the entries in the array.
	 */
 	for (first = 0, last = tvPtr->nEntries - 1; last > first; 
	     first++, last--) {
	    hold = tvPtr->flatArr[first];
	    tvPtr->flatArr[first] = tvPtr->flatArr[last];
	    tvPtr->flatArr[last] = hold;
	}
	tvPtr->viewIsDecreasing = tvPtr->sortDecreasing;
	tvPtr->flags |= TV_SORTED | TV_LAYOUT;
	return;
    }
    /* Fetch each entry's data as Tcl_Objs for sorting. */
    if (tvPtr->sortColumnPtr == &tvPtr->treeColumn) {
	for(p = tvPtr->flatArr; *p != NULL; p++) {
	    entryPtr = *p;
	    if (entryPtr->fullName == NULL) {
		Tcl_DString dString;

		Tcl_DStringInit(&dString);
		Blt_TreeViewGetFullName(tvPtr, entryPtr, TRUE, &dString);
		entryPtr->fullName = Blt_Strdup(Tcl_DStringValue(&dString));
		Tcl_DStringFree(&dString);
	    }
	    entryPtr->dataObjPtr = Tcl_NewStringObj(entryPtr->fullName, -1);
	    Tcl_IncrRefCount(entryPtr->dataObjPtr);
	}
    } else {
	/*Blt_TreeKey key;*/
	Tcl_Obj *objPtr;
	int isFmt;

	/*key = tvPtr->sortColumnPtr->key;*/
	isFmt = Blt_TreeViewStyleIsFmt(tvPtr, tvPtr->sortColumnPtr->stylePtr);
        for(p = tvPtr->flatArr; *p != NULL; p++) {
            TreeViewValue *valuePtr;
            TreeViewColumn *columnPtr = tvPtr->sortColumnPtr;
             
	    entryPtr = *p;
	   /* if (Blt_TreeViewGetData(entryPtr, key, &objPtr) != TCL_OK) {
                objPtr =  Tcl_NewStringObj("",-1);
	    }*/
             if (isFmt &&
                ((valuePtr = Blt_TreeViewFindValue(entryPtr, columnPtr))) &&
                valuePtr->textPtr) {
                 Tcl_DString dStr;
                 Tcl_DStringInit(&dStr);
                 Blt_TextLayoutValue( valuePtr->textPtr, &dStr);
                 objPtr = Tcl_NewStringObj( Tcl_DStringValue(&dStr), -1);
                 Tcl_DStringFree(&dStr);
             } else if (Blt_TreeGetValueByKey(tvPtr->interp, tvPtr->tree,
                entryPtr->node, columnPtr->key, &objPtr) != TCL_OK) {
                     objPtr = Tcl_NewStringObj("",0);
             }

	    entryPtr->dataObjPtr = objPtr;
	    Tcl_IncrRefCount(entryPtr->dataObjPtr);
	}
    }
    qsort((char *)tvPtr->flatArr, tvPtr->nEntries, sizeof(TreeViewEntry *),
	  (QSortCompareProc *)CompareEntries);

    /* Free all the Tcl_Objs used for comparison data. */
    for(p = tvPtr->flatArr; *p != NULL; p++) {
	Tcl_DecrRefCount((*p)->dataObjPtr);
         (*p)->dataObjPtr = NULL;
    }
    tvPtr->viewIsDecreasing = tvPtr->sortDecreasing;
    tvPtr->flags |= TV_SORTED;
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_TreeViewSortTreeView --
 *
 *	Sorts the tree array of entries.
 *
 *----------------------------------------------------------------------
 */
void
Blt_TreeViewSortTreeView(tvPtr)
    TreeView *tvPtr;
{
    tvPtr->flags &= ~TV_SORT_PENDING;
    if ((tvPtr->sortType != SORT_TYPE_NONE) && (tvPtr->sortColumnPtr != NULL)) {
	treeViewInstance = tvPtr;
	Blt_TreeApply(tvPtr->rootPtr->node, SortApplyProc, tvPtr);
    }
    tvPtr->viewIsDecreasing = tvPtr->sortDecreasing;
}


#endif /* NO_TREEVIEW */
