
/*
 * bltTreeViewStyle.c --
 *
 *	This module implements styles for treeview widget cells.
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

#include "bltInt.h"

#ifndef NO_TREEVIEW

#include "bltTreeView.h"
#include "bltList.h"
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#define STYLE_GAP		2

static Blt_OptionParseProc ObjToSticky;
static Blt_OptionPrintProc StickyToObj;

static Blt_OptionParseProc ObjToIcon;
static Blt_OptionPrintProc IconToObj;
static Blt_OptionFreeProc FreeIcon;
Blt_CustomOption bltTreeViewIconOption =
{
    /* Contains a pointer to the widget that's currently being
     * configured.  This is used in the custom configuration parse
     * routine for icons.  */
    ObjToIcon, IconToObj, FreeIcon, NULL,
};
extern Blt_CustomOption bltTreeViewIconsOption;


#define DEF_STYLE_HIGHLIGHT_BACKGROUND	STD_NORMAL_BACKGROUND
#define DEF_STYLE_HIGHLIGHT_FOREGROUND	STD_NORMAL_FOREGROUND
#ifdef WIN32
#define DEF_STYLE_ACTIVE_BACKGROUND	RGB_GREY85
#else
#define DEF_STYLE_ACTIVE_BACKGROUND	RGB_GREY95
#endif
#define DEF_STYLE_ACTIVE_FOREGROUND 	STD_ACTIVE_FOREGROUND
#define DEF_STYLE_GAP			"2"

typedef struct {
    TREEVIEW_STYLE_COMMON

    /* TextBox-specific fields */
    Tcl_Obj *formatCmd;

    int iconside;			/* Position of the text in relation to
				 * the icon.  */
    int side;			/* Side to anchor cell. */

} TreeViewTextBox;

#ifdef WIN32
#define DEF_TEXTBOX_CURSOR		"arrow"
#else
#define DEF_TEXTBOX_CURSOR		"hand2"
#endif /*WIN32*/
#define DEF_TEXTBOX_SIDE		"left"

#define CONF_STYLES \
    {BLT_CONFIG_BORDER, "-activebackground", "activeBackground",  \
        "ActiveBackground", DEF_STYLE_ACTIVE_BACKGROUND, \
        Blt_Offset(TreeViewStyle, activeBorder), BLT_CONFIG_NULL_OK}, \
    {BLT_CONFIG_SYNONYM, "-activebg", (char *)NULL, \
	(char *)NULL, (char *)NULL, 0, 0, (ClientData)"-activebackground"}, \
    {BLT_CONFIG_SYNONYM, "-activefg", (char *)NULL, \
	(char *)NULL, (char *)NULL, 0, 0, (ClientData)"-activeforeground"}, \
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground",  \
	"ActiveForeground", DEF_STYLE_ACTIVE_FOREGROUND,  \
	Blt_Offset(TreeViewStyle, activeFgColor), 0}, \
    {BLT_CONFIG_BORDER, "-background", (char *)NULL, (char *)NULL, \
	(char *)NULL, Blt_Offset(TreeViewStyle, border), \
        BLT_CONFIG_NULL_OK|BLT_CONFIG_DONT_SET_DEFAULT}, \
    {BLT_CONFIG_SYNONYM, "-bg", (char *)NULL, (char *)NULL, (char *)NULL, \
	0, 0, (ClientData)"-background"}, \
    {BLT_CONFIG_CURSOR, "-cursor", "cursor", "Cursor", \
	DEF_TEXTBOX_CURSOR, Blt_Offset(TreeViewStyle, cursor), 0}, \
    {BLT_CONFIG_BOOLEAN, "-readonly", "readOnly", "ReadOnly", \
	"False", Blt_Offset(TreeViewStyle, noteditable), \
	BLT_CONFIG_DONT_SET_DEFAULT},\
    {BLT_CONFIG_STRING, "-editopts", "editOpts", "EditOpts", \
	(char *)NULL, Blt_Offset(TreeViewStyle, editOpts), \
	BLT_CONFIG_NULL_OK}, \
    {BLT_CONFIG_SYNONYM, "-fg", (char *)NULL, (char *)NULL, (char *)NULL, \
	0, 0, (ClientData)"-foreground"}, \
    {BLT_CONFIG_FONT, "-font", (char *)NULL, (char *)NULL, \
	(char *)NULL, Blt_Offset(TreeViewStyle, font), \
        BLT_CONFIG_NULL_OK|BLT_CONFIG_DONT_SET_DEFAULT}, \
    {BLT_CONFIG_COLOR, "-foreground", (char *)NULL, (char *)NULL, \
	(char *)NULL, Blt_Offset(TreeViewStyle, fgColor), \
	BLT_CONFIG_NULL_OK|BLT_CONFIG_DONT_SET_DEFAULT }, \
    {BLT_CONFIG_DISTANCE, "-gap", "gap", "Gap", \
	DEF_STYLE_GAP, Blt_Offset(TreeViewStyle, gap), \
	BLT_CONFIG_DONT_SET_DEFAULT}, \
    {BLT_CONFIG_BOOLEAN, "-hide", (char *)NULL, (char *)NULL, \
	"False", Blt_Offset(TreeViewStyle, hidden), 0}, \
    {BLT_CONFIG_BORDER, "-highlightbackground", "highlightBackground", \
	"HighlightBackground", DEF_STYLE_HIGHLIGHT_BACKGROUND, \
        Blt_Offset(TreeViewStyle, highlightBorder), \
        BLT_CONFIG_COLOR_ONLY|BLT_CONFIG_NULL_OK}, \
    {BLT_CONFIG_COLOR, "-highlightforeground", "highlightForeground", \
	"HighlightForeground", DEF_STYLE_HIGHLIGHT_FOREGROUND, \
         Blt_Offset(TreeViewStyle, highlightFgColor), BLT_CONFIG_NULL_OK}, \
    {BLT_CONFIG_SYNONYM, "-highlightbg",(char *)NULL, \
	(char *)NULL, (char *)NULL, 0, 0, (ClientData)"-highlightbackground"}, \
    {BLT_CONFIG_SYNONYM, "-highlightfg", (char *)NULL, \
	(char *)NULL, (char *)NULL, 0, 0, (ClientData)"-highlightforeground"}, \
    {BLT_CONFIG_CUSTOM, "-icon", (char *)NULL, (char *)NULL, \
	(char *)NULL, Blt_Offset(TreeViewStyle, icon), \
	BLT_CONFIG_NULL_OK, &bltTreeViewIconOption}, \
    {BLT_CONFIG_INT, "-priority", (char *)NULL, (char *)NULL, \
	"0", Blt_Offset(TreeViewStyle, priority), 0}, \
    {BLT_CONFIG_SHADOW, "-shadow", "Shadow", "Shadow", \
	(char *)NULL, Blt_Offset(TreeViewStyle, shadow), 0}, \
    {BLT_CONFIG_TILE, "-tile", (char *)NULL, (char *)NULL, \
	(char *)NULL, Tk_Offset(TreeViewStyle, tile), BLT_CONFIG_NULL_OK, },

static Blt_ConfigSpec textBoxSpecs[] =
{
    CONF_STYLES
    {BLT_CONFIG_OBJCMD, "-formatcmd", "formatCmd", "FormatCmd",
	NULL, Blt_Offset(TreeViewTextBox, formatCmd), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SIDE, "-side", (char *)NULL, (char *)NULL,
	DEF_TEXTBOX_SIDE, Tk_Offset(TreeViewTextBox, side),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SIDE, "-iconside", (char *)NULL, (char *)NULL,
	DEF_TEXTBOX_SIDE, Tk_Offset(TreeViewTextBox, iconside),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

typedef struct {
    TREEVIEW_STYLE_COMMON

    /* Checkbox specific fields. */
    int size;			/* Size of the checkbox. */
    int showValue;		/* If non-zero, display the on/off value.  */
    char *onValue;
    char *offValue;
    int lineWidth;		/* Linewidth of the surrounding box. */
    GC bgGC;
    Tk_3DBorder checkBg;		/* Normal background color of cell. */ \

    XColor *boxColor;		/* Rectangle (box) color (grey). */
    XColor *fillColor;		/* Fill color (white) */
    XColor *checkColor;		/* Check color (red). */

    GC boxGC;
    GC fillGC;			/* Box fill GC */
    GC checkGC;

    TextLayout *onPtr, *offPtr;
    TreeViewIcon *icons;	/* Tk images */
    int boxX, boxW, boxH;
    int halo;
    
} TreeViewCheckBox;

#define DEF_CHECKBOX_BOX_COLOR		"black"
#define DEF_CHECKBOX_CHECK_COLOR	"darkblue"
#define DEF_CHECKBOX_FILL_COLOR		"white"
#define DEF_CHECKBOX_OFFVALUE		"0"
#define DEF_CHECKBOX_ONVALUE		"1"
#define DEF_CHECKBOX_SHOWVALUE		"yes"
#define DEF_CHECKBOX_HALO		"0"
#define DEF_CHECKBOX_SIZE		"11"
#define DEF_CHECKBOX_LINEWIDTH		"1"
#define DEF_CHECKBOX_GAP		"4"
#ifdef WIN32
#define DEF_CHECKBOX_CURSOR		"arrow"
#else
#define DEF_CHECKBOX_CURSOR		"hand2"
#endif /*WIN32*/

static Blt_ConfigSpec checkBoxSpecs[] =
{
    CONF_STYLES
    {BLT_CONFIG_COLOR, "-boxcolor", "boxColor", "BoxColor", 
	DEF_CHECKBOX_BOX_COLOR, Blt_Offset(TreeViewCheckBox, boxColor), 0},
    {BLT_CONFIG_DISTANCE, "-boxsize", "boxSize", "BoxSize",
	DEF_CHECKBOX_SIZE, Blt_Offset(TreeViewCheckBox, size), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BORDER, "-checkbg", "checkBg", "checkBg",
	(char *)NULL, Blt_Offset(TreeViewCheckBox, checkBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-checkcolor", "checkColor", "CheckColor", 
	DEF_CHECKBOX_CHECK_COLOR, Blt_Offset(TreeViewCheckBox, checkColor), 0},
    {BLT_CONFIG_CUSTOM, "-checkicons", "checkIcons", "CheckIcons",
	(char *)NULL, Blt_Offset(TreeViewCheckBox, icons),
	BLT_CONFIG_NULL_OK, &bltTreeViewIconsOption},
    {BLT_CONFIG_COLOR, "-fillcolor", "fillColor", "FillColor", 
	DEF_CHECKBOX_FILL_COLOR, Blt_Offset(TreeViewCheckBox, fillColor), 0},
    {BLT_CONFIG_TILE, "-filltile", "fillTile", "FillTile",
	(char *)NULL, Blt_Offset(TreeViewCheckBox, fillTile), BLT_CONFIG_NULL_OK, },
    {BLT_CONFIG_INT, "-halo", "halo", "Halo",
	DEF_CHECKBOX_HALO, Blt_Offset(TreeViewCheckBox, halo), 
        0},
    {BLT_CONFIG_DISTANCE, "-linewidth", "lineWidth", "LineWidth",
	DEF_CHECKBOX_LINEWIDTH, 
	Blt_Offset(TreeViewCheckBox, lineWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-offvalue", "offValue", "OffValue",
	DEF_CHECKBOX_OFFVALUE, Blt_Offset(TreeViewCheckBox, offValue), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-onvalue", "onValue", "OnValue",
	DEF_CHECKBOX_ONVALUE, Blt_Offset(TreeViewCheckBox, onValue), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BOOLEAN, "-showvalue", "showValue", "ShowValue",
	DEF_CHECKBOX_SHOWVALUE, Blt_Offset(TreeViewCheckBox, showValue), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

typedef struct {
    TREEVIEW_STYLE_COMMON

    /* ComboBox-specific fields */

    int borderWidth;		/* Width of outer border surrounding
				 * the entire box. */
    char *choiceCmd;		/* Command to get list of available choices. */
    char *choices;		/* List of available choices. */
    char *choiceKey;		/* Key to get list of available choices. */
    int scrollWidth;
    int button;
    int buttonWidth;
    int buttonBorderWidth;	/* Border width of button. */
    int buttonRelief;		/* Normal relief of button. */
    TreeViewIcon *buttonIcons;

} TreeViewComboBox;

#define DEF_COMBOBOX_BORDERWIDTH	"1"
#define DEF_COMBOBOX_BUTTON_BORDERWIDTH	"1"
#define DEF_COMBOBOX_BUTTON_RELIEF	"raised"
#define DEF_COMBOBOX_RELIEF		"flat"
#ifdef WIN32
#define DEF_COMBOBOX_CURSOR		"arrow"
#else
#define DEF_COMBOBOX_CURSOR		"hand2"
#endif /*WIN32*/


static Blt_ConfigSpec comboBoxSpecs[] =
{
    CONF_STYLES
    {BLT_CONFIG_SYNONYM, "-bd", (char *)NULL, (char *)NULL, (char *)NULL, 0, 
	0, (ClientData)"-borderwidth"},
    {BLT_CONFIG_DISTANCE, "-borderwidth", (char *)NULL, (char *)NULL,
	DEF_COMBOBOX_BORDERWIDTH, Blt_Offset(TreeViewComboBox, borderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_DISTANCE, "-buttonborderwidth", "buttonBorderWidth", 
	"ButtonBorderWidth", DEF_COMBOBOX_BUTTON_BORDERWIDTH, 
	Blt_Offset(TreeViewComboBox, buttonBorderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-buttonicons", "buttonIcons", "ButtonIcons",
	(char *)NULL, Blt_Offset(TreeViewComboBox, buttonIcons),
	BLT_CONFIG_NULL_OK, &bltTreeViewIconsOption},
    {BLT_CONFIG_RELIEF, "-buttonrelief", "buttonRelief", "ButtonRelief",
	DEF_COMBOBOX_BUTTON_RELIEF, Blt_Offset(TreeViewComboBox, buttonRelief),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-choicecmd", "choiceCmd", "ChoiceCmd",
        (char *)NULL, Blt_Offset(TreeViewComboBox, choiceCmd), 
	BLT_CONFIG_NULL_OK, 0},
    {BLT_CONFIG_STRING, "-choicekey", "choiceKey", "ChoiceKey",
        (char *)NULL, Blt_Offset(TreeViewComboBox, choiceKey), 
	BLT_CONFIG_NULL_OK, 0},
    {BLT_CONFIG_STRING, "-choices", "choices", "Choices",
        (char *)NULL, Blt_Offset(TreeViewComboBox, choices), 
	BLT_CONFIG_NULL_OK, 0},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

typedef struct {
    TREEVIEW_STYLE_COMMON

    int windowHeight;
    int windowWidth;
    char *windowCmd;
    int sticky;

} TreeViewWindowBox;

#define STICK_NORTH	(1<<0)
#define STICK_EAST	(1<<1)
#define STICK_SOUTH	(1<<2)
#define STICK_WEST	(1<<3)

static Blt_CustomOption bltStickyOption	= { 
    ObjToSticky, StickyToObj, NULL, NULL,
};

#define DEF_COMBOBOX_BORDERWIDTH	"1"
#define DEF_COMBOBOX_BUTTON_BORDERWIDTH	"1"
#define DEF_COMBOBOX_BUTTON_RELIEF	"raised"
#define DEF_COMBOBOX_RELIEF		"flat"
#ifdef WIN32
#define DEF_COMBOBOX_CURSOR		"arrow"
#else
#define DEF_COMBOBOX_CURSOR		"hand2"
#endif /*WIN32*/

static Blt_ConfigSpec windowBoxSpecs[] =
{
    {BLT_CONFIG_STRING, "-windowcmd", "windowCmd", "windowCmd",
	(char *)NULL, Blt_Offset(TreeViewWindowBox, windowCmd), 
	BLT_CONFIG_NULL_OK, 0},
    {BLT_CONFIG_INT, "-minwidth", "WindowWidth", "WindowWidth",
	"0", Blt_Offset(TreeViewWindowBox, windowWidth), 0},
    {BLT_CONFIG_INT, "-minheight", "WindowHeight", "WindowHeight",
	"0", Blt_Offset(TreeViewWindowBox, windowHeight), 0},
    {BLT_CONFIG_CUSTOM, "-sticky", "sticky", "Sticky",
	"w", Tk_Offset(TreeViewWindowBox, sticky),
	0, &bltStickyOption},
	
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

typedef struct {
    TREEVIEW_STYLE_COMMON

    /* Barbox specific fields. */
    GC bgGC;
    Tk_3DBorder barBg;		/* Normal background color of cell. */ \

    int showValue;		/* If non-zero, display the on/off value.  */
    double minValue;
    double maxValue;
    int lineWidth;		/* Linewidth of the surrounding box. */
    Tcl_Obj *formatCmd;

    XColor *boxColor;		/* Rectangle (box) color (grey). */
    XColor *fillColor;		/* Fill color (white) */

    GC boxGC;
    GC fillGC;			/* Box fill GC */

    int barWidth;
    int barHeight;
    
} TreeViewBarBox;

#define DEF_BARBOX_BOX_COLOR		"black"
#define DEF_BARBOX_FILL_COLOR		"darkgreen"
#define DEF_BARBOX_MINVALUE		"0.0"
#define DEF_BARBOX_MAXVALUE		"100.0"
#define DEF_BARBOX_WIDTH		"80"
#define DEF_BARBOX_HEIGHT		"10"
#define DEF_BARBOX_SHOWVALUE		"yes"
#define DEF_BARBOX_LINEWIDTH		"1"
#define DEF_BARBOX_GAP		"4"
#ifdef WIN32
#define DEF_BARBOX_CURSOR		"arrow"
#else
#define DEF_BARBOX_CURSOR		"hand2"
#endif /*WIN32*/

static Blt_ConfigSpec barBoxSpecs[] =
{
    CONF_STYLES
    {BLT_CONFIG_BORDER, "-barbg", "barBg", "BarBg",
	(char *)NULL, Blt_Offset(TreeViewBarBox, barBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-barfg", "barFg", "BarFg", 
	DEF_BARBOX_FILL_COLOR, Blt_Offset(TreeViewBarBox, fillColor), 0},
    {BLT_CONFIG_COLOR, "-boxcolor", "boxColor", "BoxColor", 
	DEF_BARBOX_BOX_COLOR, Blt_Offset(TreeViewBarBox, boxColor), 0},
    {BLT_CONFIG_INT, "-barwidth", "barWidth", "BarWidth",
	DEF_BARBOX_WIDTH, Blt_Offset(TreeViewBarBox, barWidth), 0},
    {BLT_CONFIG_INT, "-barheight", "BarHeight", "BarHeight",
	DEF_BARBOX_HEIGHT, Blt_Offset(TreeViewBarBox, barHeight), 0},
    {BLT_CONFIG_TILE, "-filltile", "fillTile", "FillTile",
	(char *)NULL, Tk_Offset(TreeViewBarBox, fillTile), BLT_CONFIG_NULL_OK, },
    {BLT_CONFIG_OBJCMD, "-formatcmd", "formatCmd", "FormatCmd",
	NULL, Blt_Offset(TreeViewBarBox, formatCmd), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_DISTANCE, "-linewidth", "lineWidth", "LineWidth",
	DEF_BARBOX_LINEWIDTH, 
	Blt_Offset(TreeViewBarBox, lineWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_DOUBLE, "-maxvalue", "maxValue", "MaxValue",
	DEF_BARBOX_MAXVALUE, Blt_Offset(TreeViewBarBox, maxValue), 0},
    {BLT_CONFIG_DOUBLE, "-minvalue", "minValue", "MinValue",
	DEF_BARBOX_MINVALUE, Blt_Offset(TreeViewBarBox, minValue), 0},
    {BLT_CONFIG_BOOLEAN, "-showvalue", "showValue", "ShowValue",
	DEF_BARBOX_SHOWVALUE, Blt_Offset(TreeViewBarBox, showValue), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

typedef union {
    TreeViewTextBox tb;
    TreeViewCheckBox cb;
    TreeViewComboBox cob;
    TreeViewBarBox bb;
    TreeViewWindowBox wb;
} TreeViewAllStyles;

typedef TreeViewStyle *(StyleCreateProc) _ANSI_ARGS_((TreeView *tvPtr, 
	Blt_HashEntry *hPtr));

static StyleConfigProc ConfigureTextBox, ConfigureCheckBox, ConfigureComboBox, ConfigureWindowBox, ConfigureBarBox;
static StyleCreateProc CreateTextBox, CreateCheckBox, CreateComboBox, CreateWindowBox, CreateBarBox;
static StyleDrawProc DrawTextBox, DrawCheckBox, DrawComboBox, DrawWindowBox, DrawBarBox;
static StyleEditProc EditTextBox, EditCheckBox, EditComboBox, EditWindowBox, EditBarBox;
static StyleFreeProc FreeTextBox, FreeCheckBox, FreeComboBox, FreeWindowBox, FreeBarBox;
static StyleMeasureProc MeasureTextBox, MeasureCheckBox, MeasureComboBox, MeasureWindowBox, MeasureBarBox;
static StylePickProc PickComboBox;


static Tcl_Obj *
StickyToObj(clientData, interp, tkwin, widgRec, offset)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    Tk_Window tkwin;		/* Not used. */
    char *widgRec;
    int offset;
{
    int flags = *(int *)(widgRec + offset);

    int count = 0;
    char result[10];

    if (flags&STICK_NORTH) result[count++] = 'n';
    if (flags&STICK_EAST)  result[count++] = 'e';
    if (flags&STICK_SOUTH) result[count++] = 's';
    if (flags&STICK_WEST)  result[count++] = 'w';

    result[count] = '\0';
    return Tcl_NewStringObj(result, -1);
}

static int
ObjToSticky(clientData, interp, tkwin, objPtr, widgRec, offset)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/* Interpreter to send results back to */
    Tk_Window tkwin;		/* Not used. */
    Tcl_Obj *objPtr;		/* Tcl_Obj representing the new value. */
    char *widgRec;
    int offset;
{
    /* TreeView *tvPtr = clientData; */
    int *stickyPtr = (int *)(widgRec + offset);
    int sticky = 0;
    char c;
    char *value;
    
    value = Tcl_GetString(objPtr);

    while ((c = *value++) != '\0') {
	switch (c) {
	case 'n': case 'N': sticky |= STICK_NORTH; break;
	case 'e': case 'E': sticky |= STICK_EAST;  break;
	case 's': case 'S': sticky |= STICK_SOUTH; break;
	case 'w': case 'W': sticky |= STICK_WEST;  break;
	case ' ': case ',': case '\t': case '\r': case '\n': break;
	default:
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
				   "bad sticky value \"", --value,
				   "\": must contain n, s, e or w",
				   (char *) NULL);
	    return TCL_ERROR;
	}
    }
    *stickyPtr = sticky;
    return TCL_OK;
}		


/*
 *----------------------------------------------------------------------
 *
 * ObjToIcon --
 *
 *	Convert the name of an icon into a Tk image.
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
ObjToIcon(clientData, interp, tkwin, objPtr, widgRec, offset)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;		/* Interpreter to send results back to */
    Tk_Window tkwin;		/* Not used. */
    Tcl_Obj *objPtr;		/* Tcl_Obj representing the new value. */
    char *widgRec;
    int offset;
{
    TreeView *tvPtr = clientData;
    TreeViewIcon *iconPtr = (TreeViewIcon *)(widgRec + offset);
    TreeViewIcon icon;

    icon = Blt_TreeViewGetIcon(tvPtr, Tcl_GetString(objPtr));
    if (icon == NULL) {
	return TCL_ERROR;
    }
    *iconPtr = icon;
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
IconToObj(clientData, interp, tkwin, widgRec, offset)
    ClientData clientData;	/* Not used. */
    Tcl_Interp *interp;
    Tk_Window tkwin;		/* Not used. */
    char *widgRec;
    int offset;
{
    TreeViewIcon icon = *(TreeViewIcon *)(widgRec + offset);

    return Tcl_NewStringObj(icon?Blt_NameOfImage((icon)->tkImage):"", -1);
}

/*ARGSUSED*/

int
Blt_TreeViewTextbox(TreeView *tvPtr,
		    TreeViewEntry *entryPtr,
		    TreeViewColumn *columnPtr);
static int
FreeIcon(clientData, display, widgRec, offset, oldPtr)
    ClientData clientData;
    Display *display;		/* Not used. */
    char *widgRec;
    int offset;
    char *oldPtr;
{
    TreeViewIcon icon = (TreeViewIcon)(oldPtr);
    TreeView *tvPtr = clientData;

    Blt_TreeViewFreeIcon(tvPtr, icon);
    return TCL_OK;
}

static TreeViewStyleClass textBoxClass = {
    "TextBoxStyle",
    textBoxSpecs,
    ConfigureTextBox,
    MeasureTextBox,
    DrawTextBox,
    NULL,
    EditTextBox,
    FreeTextBox,
};

static TreeViewStyleClass checkBoxClass = {
    "CheckBoxStyle",
    checkBoxSpecs,
    ConfigureCheckBox,
    MeasureCheckBox,
    DrawCheckBox,
    NULL,
    EditCheckBox,
    FreeCheckBox,
};

static TreeViewStyleClass comboBoxClass = {
    "ComboBoxStyle", 
    comboBoxSpecs,
    ConfigureComboBox,
    MeasureComboBox,
    DrawComboBox,
    PickComboBox,
    EditComboBox,
    FreeComboBox,
};

static TreeViewStyleClass windowBoxClass = {
    "WindowBoxStyle",
    windowBoxSpecs,
    ConfigureWindowBox,
    MeasureWindowBox,
    DrawWindowBox,
    NULL,
    EditWindowBox,
    FreeWindowBox,
};

static TreeViewStyleClass barBoxClass = {
    "BarBoxStyle",
    barBoxSpecs,
    ConfigureBarBox,
    MeasureBarBox,
    DrawBarBox,
    NULL,
    EditBarBox,
    FreeBarBox,
};

int Blt_TreeViewStyleIsFmt (TreeView *tvPtr,
    TreeViewStyle *stylePtr)
{
    if (stylePtr->classPtr ==  &textBoxClass) {
        TreeViewTextBox *tbPtr;
        tbPtr = (TreeViewTextBox *)stylePtr;
        if (tbPtr->formatCmd != NULL && strlen(Tcl_GetString(tbPtr->formatCmd))) {
            return 1;
        }
        return 0;
    }
    if (stylePtr->classPtr ==  &barBoxClass) {
        TreeViewBarBox *bbPtr;
        bbPtr = (TreeViewBarBox *)stylePtr;
        if (bbPtr->formatCmd != NULL && strlen(Tcl_GetString(bbPtr->formatCmd))) {
            return 1;
        }
        return 0;
    }
    return 0;
}

/* Allocate a new style. */
static TreeViewStyle *newStyle(TreeView *tvPtr, Blt_HashEntry *hPtr, int size) {
    TreeViewStyle *stylePtr;
    stylePtr = Blt_Calloc(1, sizeof(TreeViewAllStyles));
    /*stylePtr = Blt_Calloc(1, size); */
    assert(stylePtr);
    stylePtr->gap = STYLE_GAP;
    stylePtr->name = Blt_Strdup(Blt_GetHashKey(&tvPtr->styleTable, hPtr));
    stylePtr->hashPtr = hPtr;
    stylePtr->refCount = 1;
    Blt_SetHashValue(hPtr, stylePtr);
    return stylePtr;
}
/*
 *----------------------------------------------------------------------
 *
 * CreateTextBox --
 *
 *	Creates a "textbox" style.
 *
 * Results:
 *	A pointer to the new style structure.
 *
 *----------------------------------------------------------------------
 */
static TreeViewStyle *
CreateTextBox(tvPtr, hPtr)
    TreeView *tvPtr;
    Blt_HashEntry *hPtr;
{
    TreeViewTextBox *tbPtr;

    tbPtr = (TreeViewTextBox *)newStyle(tvPtr, hPtr, sizeof(TreeViewTextBox));
    tbPtr->classPtr = &textBoxClass;
    tbPtr->iconside = SIDE_LEFT;
    tbPtr->side = SIDE_TOP;
    tbPtr->flags = STYLE_TEXTBOX;
    return (TreeViewStyle *)tbPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureTextBox --
 *
 *	Configures a "textbox" style.  This routine performs 
 *	generates the GCs required for a textbox style.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GCs are created for the style.
 *
 *----------------------------------------------------------------------
 */
static void
ConfigureTextBox(tvPtr, stylePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
{
    GC newGC;
    XColor *bgColor;
    XGCValues gcValues;
    unsigned long gcMask;

    gcMask = GCForeground | GCBackground | GCFont;
    gcValues.font = Tk_FontId(CHOOSE(tvPtr->font, stylePtr->font));
    bgColor = Tk_3DBorderColor(CHOOSE(tvPtr->border, stylePtr->border));

    gcValues.background = bgColor->pixel;
    gcValues.foreground = CHOOSE(tvPtr->fgColor, stylePtr->fgColor)->pixel;
    newGC = Tk_GetGC(tvPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->gc != NULL) {
	Tk_FreeGC(tvPtr->display, stylePtr->gc);
    }
    stylePtr->gc = newGC;
    if (stylePtr->highlightBorder) {
        gcValues.background = Tk_3DBorderColor(stylePtr->highlightBorder)->pixel;
    } else {
        gcValues.background = bgColor->pixel;
    }
    if (stylePtr->highlightFgColor) {
        gcValues.foreground = stylePtr->highlightFgColor->pixel;
    } else {
        gcValues.foreground = CHOOSE(tvPtr->fgColor, stylePtr->fgColor)->pixel;
    }
    newGC = Tk_GetGC(tvPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(tvPtr->display, stylePtr->highlightGC);
    }
    stylePtr->highlightGC = newGC;

    if (stylePtr->activeBorder) {
        gcValues.background = Tk_3DBorderColor(stylePtr->activeBorder)->pixel;
    } else {
        gcValues.background = bgColor->pixel;
    }
    if (stylePtr->activeFgColor) {
        gcValues.foreground = stylePtr->activeFgColor->pixel;
    } else {
        gcValues.foreground = CHOOSE(tvPtr->fgColor, stylePtr->fgColor)->pixel;
    }
    newGC = Tk_GetGC(tvPtr->tkwin, gcMask, &gcValues);
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(tvPtr->display, stylePtr->activeGC);
    }
    stylePtr->activeGC = newGC;
    stylePtr->flags |= STYLE_DIRTY;
}

/*
 *----------------------------------------------------------------------
 *
 * MeasureTextBox --
 *
 *	Determines the space requirements for the "textbox" given
 *	the value to be displayed.  Depending upon whether an icon
 *	or text is displayed and their relative placements, this
 *	routine computes the space needed for the text entry.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The width and height fields of *valuePtr* are set with the
 *	computed dimensions.
 *
 *----------------------------------------------------------------------
 */
static void
MeasureTextBox(tvPtr, stylePtr, valuePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
    TreeViewValue *valuePtr;
{
    TreeViewTextBox *tbPtr = (TreeViewTextBox *)stylePtr;
    TreeViewColumn *columnPtr = valuePtr->columnPtr;
    int iconWidth, iconHeight;
    int textWidth, textHeight;
    int gap;
    TreeViewIcon icon;
    int rel = 0;

    textWidth = textHeight = 0;
    iconWidth = iconHeight = 0;
    valuePtr->width = valuePtr->height = 0;
    icon = (stylePtr->icon?stylePtr->icon:(columnPtr->stylePtr?columnPtr->stylePtr->icon:NULL));
    if (icon != NULL && tvPtr->hideStyleIcons == 0) {
	iconWidth = TreeViewIconWidth(icon);
	iconHeight = TreeViewIconHeight(icon);
    } 
    if (valuePtr->textPtr != NULL) {
	Blt_Free(valuePtr->textPtr);
	valuePtr->textPtr = NULL;
    }
#define NotNullObj(p) ((p != NULL && strlen(Tcl_GetString(p))) ? p : NULL)

    if (valuePtr->string != NULL) {
        /* New string defined. */
	TextStyle ts;
	Tcl_Obj *fmtObj;
	
        fmtObj = NotNullObj(tbPtr->formatCmd);
        if (fmtObj == NULL) {
            fmtObj = NotNullObj(columnPtr->formatCmd);
        }
        if (fmtObj == NULL) {
            fmtObj = NotNullObj(tvPtr->formatCmd);
        }

	Blt_InitTextStyle(&ts);
	ts.font = CHOOSE3(tvPtr->font, columnPtr->font , stylePtr->font);
	ts.anchor = TK_ANCHOR_NW;
	ts.justify = columnPtr->justify;
        if (fmtObj != NULL) {
            Tcl_DString cmdString;
            char *string;
            int result;
            Tcl_Interp *interp = tvPtr->interp;

            Tcl_Preserve(valuePtr->entryPtr);
            rel = 1;
            Blt_TreeViewPercentSubst(tvPtr, valuePtr->entryPtr, columnPtr, Tcl_GetString(fmtObj), valuePtr->string, &cmdString);
            result = Tcl_GlobalEval(interp, Tcl_DStringValue(&cmdString));
            Blt_TreeViewOptsInit(tvPtr);
            Tcl_DStringFree(&cmdString);
            if (result == TCL_OK) {
                string = Tcl_GetStringResult(interp);
                valuePtr->textPtr = Blt_GetTextLayoutStr(string, &ts);
            }
         }
         if (valuePtr->textPtr == NULL) {
             valuePtr->textPtr = Blt_GetTextLayoutStr(valuePtr->string, &ts);
         }
    } 
    gap = 0;
    if (valuePtr->textPtr != NULL && tvPtr->hideStyleText == 0) {
	textWidth = valuePtr->textPtr->width;
	textHeight = valuePtr->textPtr->height;
	if (stylePtr->icon != NULL) {
	    gap = stylePtr->gap;
	}
    }
    if (SIDE_VERTICAL(tbPtr->iconside)) {
	valuePtr->height = iconHeight + gap + textHeight;
	valuePtr->width = MAX(textWidth, iconWidth);
    } else {
	valuePtr->width = iconWidth + gap + textWidth;
	valuePtr->height = MAX(textHeight, iconHeight);
    }
    if (rel) {
        Tcl_Release(valuePtr->entryPtr);
    }
}

/* 
* Fill in the bg, fg, font from styles from value, entry, column and tvjj. */
static void
GetPriorityStyle(
    TreeView *tvPtr,
    TreeViewEntry *entryPtr,
    TreeViewValue *valuePtr,
    TreeViewStyle *inStylePtr,
    TreeViewStyle *stylePtr
    ) {
    TreeViewStyle *s[4];
    int i = -1, bgprio = -1, fgprio = -1, fntprio = -1;
    int disabled = (entryPtr->state == STATE_DISABLED);

    s[0] = valuePtr?valuePtr->stylePtr:NULL;
    s[1] = entryPtr?entryPtr->stylePtr:NULL;
    s[2] = valuePtr?valuePtr->columnPtr->stylePtr:NULL;
    s[3] = tvPtr->stylePtr;
    /*stylePtr->font = CHOOSE3(tvPtr->font, inStylePtr->font, entryPtr->font); */
    stylePtr->font = tvPtr->font;
    if (valuePtr != NULL && valuePtr->columnPtr->font) {
        stylePtr->font = valuePtr->columnPtr->font;
        stylePtr->gc = valuePtr->columnPtr->textGC;
    } else if (inStylePtr->font) {
        stylePtr->font = inStylePtr->font;
        stylePtr->gc = stylePtr->gc;
    }
    stylePtr->border = CHOOSE3(tvPtr->border, inStylePtr->border, entryPtr->border);
    if (disabled) {
        stylePtr->fgColor = tvPtr->disabledColor;
    } else if (valuePtr != NULL) {
        stylePtr->fgColor = CHOOSE4(tvPtr->fgColor, valuePtr->columnPtr->fgColor, inStylePtr->fgColor, entryPtr->color);
    } else {
        stylePtr->fgColor = CHOOSE3(tvPtr->fgColor, inStylePtr->fgColor, entryPtr->color);
    }
    while (++i<4) {
        if (s[i]) {
            if (s[i]->border && (s[i]->priority>bgprio||stylePtr->border==NULL)) {
                stylePtr->border = s[i]->border;
                bgprio = s[i]->priority;
            } 
            if (disabled == 0 && s[i]->fgColor && (s[i]->priority>fgprio||stylePtr->fgColor==NULL)) {
                stylePtr->fgColor = s[i]->fgColor;
                fgprio = s[i]->priority;
            } 
            if (s[i]->font && (s[i]->priority>fntprio||stylePtr->font==NULL)) {
                stylePtr->font = s[i]->font;
                stylePtr->gc = s[i]->gc;
                fntprio = s[i]->priority;
            } 
        }
    }
}

/* Fill in stylePtr with font, border, fgColor, etc. UNUSED */
void
Blt_GetPriorityStyle(
    TreeViewStyle *stylePtr,  /* Style to fillin. */
    TreeView *tvPtr,
    TreeViewColumn *columnPtr,
    TreeViewEntry *entryPtr,
    TreeViewValue *valuePtr,
    TreeViewStyle *inStylePtr,
    int flags
) {
    TreeViewStyle *s[20];
    int n = 0, i = -1;
    int bgprio = -1, fgprio = -1, fntprio = -1, shadprio = -1, tileprio = -1;
    int iconprio = -1;
    int disabled = (entryPtr->state == STATE_DISABLED);

    if ((flags&STYLEFLAG_NOCLEAR) == 0) {
        memset(stylePtr, 0, sizeof(*stylePtr));
    }
    /* Is a sub label */
    if ((flags&STYLEFLAG_SUBSTYLE) && tvPtr->subStylePtr) {
        s[n++] = tvPtr->subStylePtr;
    }
    /* Is an empty value. */
    if ((flags&STYLEFLAG_EMPTYSTYLE) && tvPtr->emptyStylePtr) {
        s[n++] = tvPtr->emptyStylePtr;
    }
    /* Is an alt row. */
    if ((flags&STYLEFLAG_ALTSTYLE) && tvPtr->altStylePtr) {
        s[n++] = tvPtr->altStylePtr;
    }
    /* Is a title. */
    if ((flags&STYLEFLAG_TITLESTYLE) && columnPtr && columnPtr->titleStylePtr) {
        s[n++] = columnPtr->titleStylePtr;
    }
    if (inStylePtr) s[n++] = inStylePtr;
    if (valuePtr && valuePtr->stylePtr) s[n++] = valuePtr->stylePtr;
    if (entryPtr && entryPtr->stylePtr) s[n++] = entryPtr->stylePtr;
    if (columnPtr && columnPtr->stylePtr) s[n++] = columnPtr->stylePtr;
    if (tvPtr->stylePtr)  s[n++] = tvPtr->stylePtr;
    if (inStylePtr == NULL) inStylePtr = tvPtr->stylePtr;
    /*stylePtr->font = CHOOSE(tvPtr->font,  entryPtr->font); */
    /*stylePtr->font = tvPtr->font;
    stylePtr->border = tvPtr->border;
    stylePtr->fgColor = tvPtr->fgColor;
    stylePtr->shadow.color = NULL;
    stylePtr->tile = NULL;
    stylePtr->icon = NULL;*/
    stylePtr->gc = (entryPtr?entryPtr->gc:NULL);
    /* stylePtr->font = tvPtr->font; */
    if (valuePtr != NULL && valuePtr->columnPtr->font) {
        stylePtr->font = valuePtr->columnPtr->font;
        stylePtr->gc = valuePtr->columnPtr->textGC;
    } else if (inStylePtr->font) {
        stylePtr->font = inStylePtr->font;
        stylePtr->gc = stylePtr->gc;
    } else if (columnPtr && columnPtr->font) {
        stylePtr->font = columnPtr->font;
        stylePtr->gc = columnPtr->textGC;
    }
    stylePtr->border = CHOOSE3(tvPtr->border, inStylePtr->border, entryPtr->border);
    if (disabled) {
        stylePtr->fgColor = tvPtr->disabledColor;
    } else if (valuePtr != NULL) {
        stylePtr->fgColor = CHOOSE4(tvPtr->fgColor, valuePtr->columnPtr->fgColor, inStylePtr->fgColor, entryPtr->color);
    } else {
        stylePtr->fgColor = CHOOSE3(tvPtr->fgColor, inStylePtr->fgColor, entryPtr->color);
    }
    i = -1;
    while (++i<n) {
        if (s[i]->border && (s[i]->priority>bgprio||stylePtr->border==NULL)) {
            stylePtr->border = s[i]->border;
            bgprio = s[i]->priority;
        } 
        if (disabled == 0 && s[i]->fgColor && (s[i]->priority>fgprio||stylePtr->fgColor==NULL)) {
            stylePtr->fgColor = s[i]->fgColor;
            fgprio = s[i]->priority;
        } 
        if (s[i]->font && (s[i]->priority>fntprio||stylePtr->font==NULL)) {
            stylePtr->font = s[i]->font;
            fntprio = s[i]->priority;
            stylePtr->gc = s[i]->gc;
        } 
        if (s[i]->shadow.color && (s[i]->priority>shadprio||stylePtr->shadow.color==NULL)) {
            stylePtr->shadow = s[i]->shadow;
            shadprio = s[i]->priority;
        } 
        if (s[i]->icon && (s[i]->priority>iconprio||stylePtr->icon==NULL)) {
            stylePtr->icon = s[i]->icon;
            iconprio = s[i]->priority;
        } 
        if (s[i]->tile && (s[i]->priority>tileprio||stylePtr->tile==NULL)) {
            stylePtr->tile = s[i]->tile;
            tileprio = s[i]->priority;
        } 
    }
    if (entryPtr == NULL) {
        if (stylePtr->font==NULL) stylePtr->font = tvPtr->font;
        if (stylePtr->border==NULL) stylePtr->border = tvPtr->border;
        if (stylePtr->fgColor==NULL) stylePtr->fgColor = tvPtr->fgColor;
    } else {
        if (stylePtr->font==NULL) stylePtr->font = CHOOSE3(tvPtr->font,(columnPtr?columnPtr->font:NULL),entryPtr->font);
        if (stylePtr->border==NULL) stylePtr->border = CHOOSE(tvPtr->border,entryPtr->border);
        if (stylePtr->fgColor==NULL) stylePtr->fgColor = CHOOSE(tvPtr->fgColor,entryPtr->color);
        if (stylePtr->shadow.color==NULL) stylePtr->shadow = entryPtr->shadow;
    }
    if (stylePtr->tile==NULL) stylePtr->tile = tvPtr->tile;
}

#define IFSET(var,val) var = (val?val:var)

/* Fill background. */
/* TODO: return GC for element/style that sets the font!!!! */
/* TODO: lookup font from style for Measure routines. */
void
drawTextBox(tvPtr, drawable, entryPtr, valuePtr, stylePtr, icon, x, y, sRec)
    TreeView *tvPtr;
    Drawable drawable;
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr;
    TreeViewStyle *stylePtr;
    TreeViewIcon icon;
    int x, y;
    TreeViewStyle *sRec;
{
    TreeViewColumn *columnPtr;
    int altRow = (entryPtr->flags&ENTRY_ALTROW);

    columnPtr = valuePtr->columnPtr;
    sRec->font = CHOOSE(tvPtr->font, columnPtr->font);
    sRec->fgColor = CHOOSE(tvPtr->fgColor, columnPtr->fgColor);
    sRec->border = tvPtr->border;
    sRec->gc = tvPtr->stylePtr->gc;
    GetPriorityStyle(tvPtr, entryPtr, valuePtr, stylePtr, sRec);
    /* TODO: fix font/gc bug where altStyle has tile and bg */
    if ((tvPtr->activeColumnPtr == valuePtr->columnPtr && tvPtr->actCol) ||
        (valuePtr == tvPtr->activeValuePtr) ||
        (tvPtr->activeButtonPtr == entryPtr && tvPtr->actEntry) ||
        (stylePtr->flags & STYLE_HIGHLIGHT)) {
	IFSET(sRec->border, stylePtr->highlightBorder);
	IFSET(sRec->fgColor, stylePtr->highlightFgColor);
     } else if (valuePtr->stylePtr && valuePtr->stylePtr->border) {
         sRec->border = valuePtr->stylePtr->border;
     } else if (columnPtr->hasbg) {
         sRec->border = columnPtr->border;
     /*((tvPtr->tile != NULL || columnPtr->tile != NULL) && stylePtr->tile==NULL)*/
     } else if ((Blt_HasTile(tvPtr->tile) || Blt_HasTile(columnPtr->tile)) && Blt_HasTile(stylePtr->tile)==0) {
         sRec->border = NULL;
     } else if (altRow && tvPtr->altStylePtr &&  Blt_HasTile(tvPtr->altStylePtr->tile)) {
         stylePtr = tvPtr->altStylePtr;
    }
    if (!Blt_TreeViewEntryIsSelected(tvPtr, entryPtr, columnPtr)) {
        /*
        * Draw the active or normal background color over the entire
        * label area.  This includes both the tab's text and image.
        * The rectangle should be 2 pixels wider/taller than this
        * area. So if the label consists of just an image, we get an
        * halo around the image when the tab is active.
        */
        if (sRec->border != NULL && (altRow==0 ||
        (altRow && stylePtr->tile && stylePtr == tvPtr->altStylePtr))) {
             Blt_TreeViewFill3DTile(tvPtr, drawable, sRec->border,
               x - columnPtr->pad.side1, y - tvPtr->leader/2, 
	       columnPtr->width, entryPtr->height,
	       0, TK_RELIEF_FLAT,  entryPtr->gc?NULL:stylePtr->tile, tvPtr->scrollTile, 1);
	}
    }
}

/*
 * Set fg color based on string pattern match
 */
static void
GetColorPats(tvPtr, entryPtr, valuePtr, stylePtr, fgPtr)
    TreeView *tvPtr;
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr;
    TreeViewStyle *stylePtr;
    XColor **fgPtr;
{
    XColor *color = NULL;
    Tcl_Interp *interp = tvPtr->interp;
    int objc, i;
    Tcl_Obj **objv, *objPtr = NULL;
    TreeViewColumn *columnPtr = valuePtr->columnPtr;
            
    if (columnPtr->colorPats != NULL && strlen(Tcl_GetString(columnPtr->colorPats))) {
        if (Tcl_ListObjGetElements(NULL, columnPtr->colorPats, &objc, &objv) != TCL_OK) return;
        if (objc%2) return;
        for (i = 0; i < objc; i += 2) {
            if (Tcl_StringMatch(valuePtr->string, Tcl_GetString(objv[i]))) {
                color = Tk_AllocColorFromObj(interp, tvPtr->tkwin, objv[i+1]);
                if (color != NULL) {
                    *fgPtr = color;
                    return;
                }
            }
        }
    } 
    if (columnPtr->colorRegex != NULL && strlen(Tcl_GetString(columnPtr->colorRegex))) {
        if (Tcl_ListObjGetElements(NULL, columnPtr->colorRegex, &objc, &objv) != TCL_OK) return;
        if (objc%2) return;
        for (i = 0; i < objc; i += 2) {
            if (objPtr == NULL) {
                objPtr = Tcl_NewStringObj(valuePtr->string,-1);
                Tcl_IncrRefCount(objPtr);
            }
            if (Tcl_RegExpMatchObj(NULL, objPtr, objv[i]) == 1) {
                color = Tk_AllocColorFromObj(interp, tvPtr->tkwin, objv[i+1]);
                if (color != NULL) {
                    *fgPtr = color;
                    break;
                }
            }
        }
        if (objPtr != NULL) {
            Tcl_DecrRefCount(objPtr);
        }
    } 
}

/*
 *----------------------------------------------------------------------
 *
 * DrawTextBox --
 *
 *	Draws the "textbox" given the screen coordinates and the
 *	value to be displayed.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The textbox value is drawn.
 *
 *----------------------------------------------------------------------
 */
static void
DrawTextBox(tvPtr, drawable, entryPtr, valuePtr, stylePtr, icon, x, y)
    TreeView *tvPtr;
    Drawable drawable;
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr;
    TreeViewStyle *stylePtr;
    TreeViewIcon icon;
    int x, y;
{
    GC gc;
    TreeViewColumn *columnPtr;
    TreeViewTextBox *tbPtr = (TreeViewTextBox *)stylePtr;
    int iconX, iconY, iconWidth, iconHeight;
    int textX, textY, textWidth, textHeight;
    int gap, columnWidth, ix = x;
    TreeViewStyle sRec = *stylePtr;
    int valWidth;
    TextLayout *layPtr;

    layPtr = (tvPtr->hideStyleText?NULL:valuePtr->textPtr);
    columnPtr = valuePtr->columnPtr;
    valWidth = (layPtr?valuePtr->width:(icon?TreeViewIconWidth(icon):0));
    if (!stylePtr) return;
    drawTextBox(tvPtr, drawable, entryPtr, valuePtr, stylePtr, icon, x, y, &sRec);
    gc = sRec.gc;
    columnWidth = columnPtr->width - 
	(2 * columnPtr->borderWidth + PADDING(columnPtr->pad));

    textX = textY = iconX = iconY = 0;	/* Suppress compiler warning. */
    
    iconWidth = iconHeight = 0;
    if (icon != NULL) {
	iconWidth = TreeViewIconWidth(icon);
	iconHeight = TreeViewIconHeight(icon);
    }
    textWidth = textHeight = 0;
    if (layPtr != NULL) {
	textWidth = layPtr->width;
	textHeight = layPtr->height;
    }
    gap = 0;
    if ((icon != NULL) && (layPtr != NULL)) {
        gap = stylePtr->gap;
    }
    if (columnWidth >= valWidth) {
	switch(columnPtr->justify) {
	case TK_JUSTIFY_RIGHT:
	    ix = x + columnWidth - iconWidth;
	    x += (columnWidth - valWidth);
	    break;
	case TK_JUSTIFY_CENTER:
	    ix = x + (columnWidth - iconWidth)/2;
	    x += (columnWidth - valWidth) / 2;
	    break;
	case TK_JUSTIFY_LEFT:
	    break;
	}
    }

    switch (tbPtr->iconside) {
    case SIDE_RIGHT:
	textX = x;
	textY = y + (entryPtr->height - textHeight) / 2;
	iconX = textX + textWidth + gap;
	iconY = y + (entryPtr->height - iconHeight) / 2;
	break;
    case SIDE_LEFT:
	iconX = x;
	iconY = y + (entryPtr->height - iconHeight) / 2;
	textX = iconX + iconWidth + gap;
	textY = y + (entryPtr->height - textHeight) / 2;
	break;
    case SIDE_TOP:
        iconY = y;
        if (tbPtr->side == SIDE_BOTTOM) {
            iconY = y + (entryPtr->height - textHeight - iconHeight - gap*2);
        }
	iconX = ix;
	textY = iconY + iconHeight + gap;
	textX = x;
        if (iconWidth>textWidth) {
            textX = x + (iconWidth-textWidth)/2;
        }
        break;
    case SIDE_BOTTOM:
	textX = x;
	textY = y;
        if (tbPtr->side == SIDE_BOTTOM) {
            textY = y + (entryPtr->height - textHeight - iconHeight - gap*2);
        }
	if (iconWidth>textWidth) {
	    textX = x + (iconWidth-textWidth)/2;
	}
	iconY = textY + textHeight + gap;
	iconX = ix;
	break;
    }
    valuePtr->iX = iconX;
    valuePtr->iY = iconY;
    valuePtr->iW = iconWidth;
    valuePtr->iH = iconHeight;
    valuePtr->tX = textX;
    valuePtr->tY = textY;
    valuePtr->tW = textWidth;
    valuePtr->tH = textHeight;
    if (icon != NULL) {
        if (Blt_TreeViewRedrawIcon(tvPtr, entryPtr, columnPtr, icon, 0, 0, iconWidth, 
		       iconHeight, drawable, iconX, iconY) != TCL_OK) return;
    }
    if (layPtr != NULL) {
	TextStyle ts;
	XColor *color;
        int disabled = (entryPtr->state == STATE_DISABLED);
	
	if (disabled) {
	    color = tvPtr->disabledColor;
	} else if (Blt_TreeViewEntryIsSelected(tvPtr, entryPtr, columnPtr)) {
	    color = SELECT_FG(tvPtr);
	} else if (entryPtr->color != NULL) {
	    color = entryPtr->color;
	} else {
	    color = sRec.fgColor;
            GetColorPats(tvPtr, entryPtr, valuePtr, stylePtr, &color );
        }
        XSetForeground(tvPtr->display, gc, color->pixel);
	Blt_SetDrawTextStyle(&ts, sRec.font, gc, color, sRec.fgColor,
	    stylePtr->shadow.color, 0.0, 
            TK_ANCHOR_NW, columnPtr->justify, 0, stylePtr->shadow.offset);
	Blt_DrawTextLayout(tvPtr->tkwin, drawable, layPtr, 
		&ts, textX, textY);
	if (color != sRec.fgColor) {
	    XSetForeground(tvPtr->display, gc, sRec.fgColor->pixel);
	}
     } else {
         valuePtr->tW = 0;
     }
    stylePtr->flags &= ~STYLE_DIRTY;
}

/*
 *----------------------------------------------------------------------
 *
 * EditTextbox --
 *
 *	Edits the "textbox".
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The checkbox value is drawn.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EditTextBox(tvPtr, entryPtr, valuePtr, stylePtr, x, y, retVal)
    TreeView *tvPtr;
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr;
    TreeViewStyle *stylePtr;	/* Not used. */
    int x;
    int y;
    int *retVal;
{
    int isTest = *retVal;
    *retVal = 0;
    if (isTest) {
        return TCL_OK;
    }
    return Blt_TreeViewTextbox(tvPtr, entryPtr, valuePtr->columnPtr);
}


/*
 *----------------------------------------------------------------------
 *
 * FreeTextBox --
 *
 *	Releases resources allocated for the textbox. The resources
 *	freed by this routine are specific only to the "textbox".   
 *	Other resources (common to all styles) are freed in the 
 *	Blt_TreeViewFreeStyle routine.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GCs allocated for the textbox are freed.
 *
 *----------------------------------------------------------------------
 */
static void
FreeTextBox(tvPtr, stylePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
{
    if (!stylePtr) return;
    if (stylePtr->highlightGC != NULL) {
	Tk_FreeGC(tvPtr->display, stylePtr->highlightGC);
    }
    if (stylePtr->activeGC != NULL) {
	Tk_FreeGC(tvPtr->display, stylePtr->activeGC);
    }
    if (stylePtr->gc != NULL) {
	Tk_FreeGC(tvPtr->display, stylePtr->gc);
    }
    if (stylePtr->icon != NULL) {
	Blt_TreeViewFreeIcon(tvPtr, stylePtr->icon);
    }
    
}


/*
 *----------------------------------------------------------------------
 *
 * CreateCheckbox --
 *
 *	Creates a "checkbox" style.
 *
 * Results:
 *	A pointer to the new style structure.
 *
 *----------------------------------------------------------------------
 */
static TreeViewStyle *
CreateCheckBox(tvPtr, hPtr)
    TreeView *tvPtr;
    Blt_HashEntry *hPtr;
{
    TreeViewCheckBox *cbPtr;

    cbPtr = (TreeViewCheckBox *)newStyle(tvPtr, hPtr, sizeof(TreeViewCheckBox));
    cbPtr->classPtr = &checkBoxClass;
    cbPtr->gap = 4;
    cbPtr->size = 11;
    cbPtr->lineWidth = 1;
    cbPtr->showValue = TRUE;
    cbPtr->flags = STYLE_CHECKBOX;
    return (TreeViewStyle *)cbPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureCheckbox --
 *
 *	Configures a "checkbox" style.  This routine performs 
 *	generates the GCs required for a checkbox style.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GCs are created for the style.
 *
 *----------------------------------------------------------------------
 */
static void
ConfigureCheckBox(tvPtr, stylePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
{
    GC newGC;
    TreeViewCheckBox *cbPtr = (TreeViewCheckBox *)stylePtr;
    XColor *bgColor;
    XGCValues gcValues;
    unsigned long gcMask;

    bgColor = Tk_3DBorderColor(CHOOSE(tvPtr->border, stylePtr->border));

    gcValues.background = bgColor->pixel;
    gcValues.foreground = CHOOSE(tvPtr->fgColor, stylePtr->fgColor)->pixel;
    ConfigureTextBox(tvPtr, stylePtr);
    gcMask = GCForeground;
    gcValues.foreground = cbPtr->fillColor->pixel;
    newGC = Tk_GetGC(tvPtr->tkwin, gcMask, &gcValues);
    if (cbPtr->fillGC != NULL) {
	Tk_FreeGC(tvPtr->display, cbPtr->fillGC);
    }
    cbPtr->fillGC = newGC;

    gcMask = GCForeground | GCLineWidth;
    gcValues.line_width = cbPtr->lineWidth;
    gcValues.foreground = cbPtr->boxColor->pixel;
    newGC = Tk_GetGC(tvPtr->tkwin, gcMask, &gcValues);
    if (cbPtr->boxGC != NULL) {
	Tk_FreeGC(tvPtr->display, cbPtr->boxGC);
    }
    cbPtr->boxGC = newGC;

    gcMask = GCForeground | GCLineWidth;
    gcValues.line_width = 1;
    gcValues.foreground = cbPtr->checkColor->pixel;
    newGC = Tk_GetGC(tvPtr->tkwin, gcMask, &gcValues);
    if (cbPtr->checkGC != NULL) {
	Tk_FreeGC(tvPtr->display, cbPtr->checkGC);
    }
    cbPtr->checkGC = newGC;
    if (cbPtr->checkBg) {
        bgColor = Tk_3DBorderColor(cbPtr->checkBg);
        gcValues.background = bgColor->pixel;
        gcValues.foreground = bgColor->pixel;
        newGC = Tk_GetGC(tvPtr->tkwin, gcMask, &gcValues);
        if (cbPtr->bgGC != NULL) {
            Tk_FreeGC(tvPtr->display, cbPtr->bgGC);
        }
        cbPtr->bgGC = newGC;
    }
    stylePtr->flags |= STYLE_DIRTY;
}

/*
 *----------------------------------------------------------------------
 *
 * MeasureCheckbox --
 *
 *	Determines the space requirements for the "checkbox" given
 *	the value to be displayed.  Depending upon whether an icon
 *	or text is displayed and their relative placements, this
 *	routine computes the space needed for the text entry.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The width and height fields of *valuePtr* are set with the
 *	computed dimensions.
 *
 *----------------------------------------------------------------------
 */
static void
MeasureCheckBox(tvPtr, stylePtr, valuePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
    TreeViewValue *valuePtr;
{
    TreeViewCheckBox *cbPtr = (TreeViewCheckBox *)stylePtr;
    TreeViewColumn *columnPtr = valuePtr->columnPtr;
    int iconWidth, iconHeight;
    int textWidth, textHeight;
    int gap;
    int boxWidth, boxHeight;
    TreeViewIcon icon;

    boxWidth = boxHeight = ODD(cbPtr->size);

    if (cbPtr->icons) {
        TreeViewIcon mIcon;
        int i;
        
        for (i=0; i<2; i++) {
            mIcon = cbPtr->icons[i];
            if (mIcon) {
                int mwid = TreeViewIconWidth(mIcon);
                int mhig = TreeViewIconHeight(mIcon);
                if (mwid>boxWidth) { boxWidth = mwid; }
                if (mhig>boxHeight) { boxHeight = mhig; }
            }
        }
    }
    textWidth = textHeight = iconWidth = iconHeight = 0;
    valuePtr->width = valuePtr->height = 0;
    icon = (stylePtr->icon?stylePtr->icon:(columnPtr->stylePtr?columnPtr->stylePtr->icon:NULL));
    if (icon != NULL && tvPtr->hideStyleIcons == 0) {
        iconWidth = TreeViewIconWidth(icon);
        iconHeight = TreeViewIconHeight(icon);
    } 
    if (cbPtr->onPtr != NULL) {
	Blt_Free(cbPtr->onPtr);
	cbPtr->onPtr = NULL;
    }
    if (cbPtr->offPtr != NULL) {
	Blt_Free(cbPtr->offPtr);
	cbPtr->offPtr = NULL;
    }
    gap = 0;
    if (cbPtr->showValue && tvPtr->hideStyleText == 0) {
	TextStyle ts;
	char *string;

	Blt_InitTextStyle(&ts);
        ts.font = CHOOSE3(tvPtr->font, columnPtr->font, stylePtr->font);
	ts.anchor = TK_ANCHOR_NW;
	ts.justify = columnPtr->justify;
	string = (cbPtr->onValue != NULL) ? cbPtr->onValue : valuePtr->string;
	cbPtr->onPtr = Blt_GetTextLayout(string, &ts);
	string = (cbPtr->offValue != NULL) ? cbPtr->offValue : valuePtr->string;
	cbPtr->offPtr = Blt_GetTextLayout(string, &ts);
	textWidth = MAX(cbPtr->offPtr->width, cbPtr->onPtr->width);
	textHeight = MAX(cbPtr->offPtr->height, cbPtr->onPtr->height);
	if (stylePtr->icon != NULL) {
	    gap = stylePtr->gap;
	}
    }
    valuePtr->width = stylePtr->gap * 2 + boxWidth + iconWidth + gap + textWidth;
    valuePtr->height = MAX3(boxHeight, textHeight, iconHeight);
}

/*
 *----------------------------------------------------------------------
 *
 * 
box --
 *
 *	Draws the "checkbox" given the screen coordinates and the
 *	value to be displayed.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The checkbox value is drawn.
 *
 *----------------------------------------------------------------------
 */
static void
DrawCheckBox(tvPtr, drawable, entryPtr, valuePtr, stylePtr, icon, x, y)
    TreeView *tvPtr;
    Drawable drawable;
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr;
    TreeViewStyle *stylePtr;
    TreeViewIcon icon;
    int x, y;
{
    GC gc;
    TreeViewColumn *columnPtr;
    TreeViewCheckBox *cbPtr = (TreeViewCheckBox *)stylePtr;
    int iconX, iconY, iconWidth, iconHeight;
    int textX, textY, textHeight;
    int gap, columnWidth, ttlWidth;
    int bool;
    /* int borderWidth, relief; */
    TextLayout *textPtr;
    int boxX, boxY, boxWidth, boxHeight;
    TreeViewStyle *csPtr, sRec = *stylePtr;
    int valWidth = (valuePtr->textPtr?valuePtr->width:(icon?TreeViewIconWidth(icon):0));
    int showValue;

    showValue = (tvPtr->hideStyleText ? 0 : cbPtr->showValue);

    columnPtr = valuePtr->columnPtr;
    csPtr = columnPtr->stylePtr;
    
    drawTextBox(tvPtr, drawable, entryPtr, valuePtr, stylePtr, icon, x, y, &sRec);
    gc = sRec.gc;

    /* if (gc != stylePtr->activeGC) {
        borderWidth = 0;
        relief = TK_RELIEF_FLAT;
    } else {
        borderWidth = 1;
        relief = TK_RELIEF_RAISED;
    } */

    columnWidth = columnPtr->width - PADDING(columnPtr->pad);
    if (!valuePtr->string) {
        return;
    }

    if (cbPtr->size) {
        boxWidth = boxHeight = ODD(cbPtr->size);
    } else {
        boxWidth = boxHeight = 0;
    }
    ttlWidth = ( boxWidth + stylePtr->gap * 2);
    if (showValue) {
        ttlWidth += (valWidth + stylePtr->gap);
    }
    if (icon != NULL) {
        iconWidth = TreeViewIconWidth(icon);
        iconHeight = TreeViewIconHeight(icon);
        ttlWidth += iconWidth + stylePtr->gap;
    }

    if (columnWidth > ttlWidth) {
        switch(columnPtr->justify) {
            case TK_JUSTIFY_RIGHT:
            x += (columnWidth - ttlWidth - stylePtr->gap*2);
            break;
            case TK_JUSTIFY_CENTER:
            x += (columnWidth - ttlWidth) / 2;
            break;
            case TK_JUSTIFY_LEFT:
            break;
        }
    }

    bool = (strcmp(valuePtr->string, cbPtr->onValue) == 0);
    textPtr = (bool) ? cbPtr->onPtr : cbPtr->offPtr;

    cbPtr->boxW = cbPtr->boxH = 0;
    cbPtr->boxX = boxX = x + stylePtr->gap;
    boxY = y + (entryPtr->height - boxHeight) / 2;

    if (cbPtr->icons != NULL) {
        TreeViewIcon mIcon;
        mIcon = cbPtr->icons[bool];
        if (mIcon) {
            int mwid = TreeViewIconWidth(mIcon);
            int mhig = TreeViewIconHeight(mIcon);
            cbPtr->boxW = mwid;
            cbPtr->boxH = mhig;
            boxY = y + (entryPtr->height - mhig) / 2;
            if (Blt_TreeViewRedrawIcon(tvPtr, entryPtr, columnPtr, mIcon, 0, 0, mwid, 
                mhig, drawable, boxX, boxY) != TCL_OK) return;
        }
        
    } else {
        /*
         * Draw the box and check. 
         *
         *		+-----------+
         *		|           |
         *		|         * |
         *      |        *  |
         *      | *     *   |
         *      |  *   *    |
         *      |   * *     |
         *		|    *      |
         *		+-----------+
         */
         if (boxWidth) {
             if (cbPtr->checkBg) {
                 XFillRectangle(tvPtr->display, drawable, cbPtr->bgGC, boxX, boxY, 
                    boxWidth, boxHeight);
             } else {
                 if (Blt_HasTile(stylePtr->fillTile)) {
                     Blt_SetTileOrigin(tvPtr->tkwin, stylePtr->fillTile, -boxX, -boxY);
                     Blt_TileRectangle(tvPtr->tkwin, drawable, stylePtr->fillTile,
                         boxX, boxY, boxWidth, boxHeight );
                 } else if (csPtr != NULL && Blt_HasTile(csPtr->fillTile)) {
                     Blt_SetTileOrigin(tvPtr->tkwin, csPtr->fillTile, -boxX, -boxY);
                     Blt_TileRectangle(tvPtr->tkwin, drawable, csPtr->fillTile,
                         boxX, boxY, boxWidth, boxHeight );
                } else {
                    XFillRectangle(tvPtr->display, drawable, cbPtr->fillGC, boxX, boxY, boxWidth, boxHeight);
    
                 }
            }
            cbPtr->boxW = boxWidth;
            cbPtr->boxH = boxHeight;
            XDrawRectangle(tvPtr->display, drawable, cbPtr->boxGC, boxX, boxY, 
                boxWidth, boxHeight);
    
            if (bool) {
                int midX, midY;
                int i;
    
                for (i = 0; i < 3; i++) {
                    midX = boxX + 2 * boxWidth / 5;
                    midY = boxY + boxHeight - 5 + i;
                    XDrawLine(tvPtr->display, drawable, cbPtr->checkGC, 
                    boxX + 2, boxY + boxHeight / 3 + 1 + i, midX, midY);
                    XDrawLine(tvPtr->display, drawable, cbPtr->checkGC, 
                    midX, midY, boxX + boxWidth - 2, boxY + i + 1);
                }
            }
        }
    }
#ifdef notdef
    textX = textY = iconX = iconY = 0;	/* Suppress compiler warning. */
#endif
    iconWidth = iconHeight = 0;
    if (icon != NULL) {
	iconWidth = TreeViewIconWidth(icon);
	iconHeight = TreeViewIconHeight(icon);
    }
    textHeight = 0;
    gap = 0;
    if (showValue) {
	textHeight = textPtr->height;
	if (icon != NULL) {
	    gap = stylePtr->gap;
	}
    }
    x = boxX + boxWidth + stylePtr->gap;

    /* The icon sits to the left of the text. */
    iconX = x;
    iconY = y + (entryPtr->height - iconHeight) / 2;
    textX = iconX + iconWidth + gap;
    textY = y + (entryPtr->height - textHeight) / 2;

    valuePtr->iX = iconX;
    valuePtr->iY = iconY;
    valuePtr->iW = iconWidth;
    valuePtr->iH = iconHeight;
    valuePtr->tX = textX;
    valuePtr->tY = textY;
    valuePtr->tW = (textPtr?textPtr->width:0);
    valuePtr->tH = textHeight;
    
    if (icon != NULL) {
        if (Blt_TreeViewRedrawIcon(tvPtr, entryPtr, columnPtr, icon, 0, 0, iconWidth, 
		       iconHeight, drawable, iconX, iconY) != TCL_OK) return;
    } else {
        valuePtr->iW = 0;
    }
    if ((showValue) && (textPtr != NULL)) {
	TextStyle ts;
	XColor *color;
	
         int disabled = (entryPtr->state == STATE_DISABLED);
        
        if (disabled) {
            color = tvPtr->disabledColor;
        } else	if (Blt_TreeViewEntryIsSelected(tvPtr, entryPtr, columnPtr)) {
	    color = SELECT_FG(tvPtr);
	} else if (entryPtr->color != NULL) {
	    color = entryPtr->color;
	} else {
	    color = sRec.fgColor;
            GetColorPats(tvPtr, entryPtr, valuePtr, stylePtr, &color );
         }
	XSetForeground(tvPtr->display, gc, color->pixel);
	Blt_SetDrawTextStyle(&ts, sRec.font, gc, color, sRec.fgColor,
	   stylePtr->shadow.color, 0.0, 
	   TK_ANCHOR_NW, columnPtr->justify, 0, stylePtr->shadow.offset);
	Blt_DrawTextLayout(tvPtr->tkwin, drawable, textPtr, &ts, textX, textY);
	if (color != sRec.fgColor) {
	    XSetForeground(tvPtr->display, gc, sRec.fgColor->pixel);
	}
    } else {
        valuePtr->tW = 0;
    }
    stylePtr->flags &= ~STYLE_DIRTY;
}

#if 0
/*
 *----------------------------------------------------------------------
 *
 * PickCheckbox --
 *
 *	Draws the "checkbox" given the screen coordinates and the
 *	value to be displayed.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The checkbox value is drawn.
 *
 *----------------------------------------------------------------------
 */
static int
PickCheckBox(entryPtr, valuePtr, stylePtr, worldX, worldY)
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr;
    TreeViewStyle *stylePtr;
    int worldX, worldY;
{
    TreeViewColumn *columnPtr;
    TreeViewCheckBox *cbPtr = (TreeViewCheckBox *)stylePtr;
    int columnWidth;
    int x, y, width, height;

    columnPtr = valuePtr->columnPtr;
    columnWidth = columnPtr->width - 
	(2 * columnPtr->borderWidth + PADDING(columnPtr->pad));
    if (columnWidth > valuePtr->width) {
	switch(columnPtr->justify) {
	case TK_JUSTIFY_RIGHT:
	    worldX += (columnWidth - valuePtr->width);
	    break;
	case TK_JUSTIFY_CENTER:
	    worldX += (columnWidth - valuePtr->width) / 2;
	    break;
	case TK_JUSTIFY_LEFT:
	    break;
	}
    }
    width = height = ODD(cbPtr->size) + 2 * cbPtr->lineWidth;
    x = columnPtr->worldX + columnPtr->pad.side1 + stylePtr->gap - 
	cbPtr->lineWidth;
    y = entryPtr->worldY + (entryPtr->height - height) / 2;
    if ((worldX >= x) && (worldX < (x + width)) && 
	(worldY >= y) && (worldY < (y + height))) {
	return TRUE;
    }
    return FALSE;
}
#endif
/*
 *----------------------------------------------------------------------
 *
 * EditCheckbox --
 *
 *	Edits the "checkbox".
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The checkbox value is drawn.
 *
 *----------------------------------------------------------------------
 */
static int
EditCheckBox(tvPtr, entryPtr, valuePtr, stylePtr, x, y, retVal)
    TreeView *tvPtr;
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr;
    TreeViewStyle *stylePtr;
    int x;
    int y;
    int *retVal;
{
    TreeViewColumn *columnPtr;
    TreeViewCheckBox *cbPtr = (TreeViewCheckBox *)stylePtr;
    Tcl_Obj *objPtr;
    int boxY, isTest = *retVal;
    TreeViewStyle *vsPtr;

    *retVal = 1;
    columnPtr = valuePtr->columnPtr;
    if (Blt_TreeGetValueByKey(tvPtr->interp, tvPtr->tree, 
	      entryPtr->node, columnPtr->key, &objPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    vsPtr = valuePtr->stylePtr;
    if (vsPtr && vsPtr->noteditable) {
        *retVal = 0;
        return TCL_OK;
    }
    boxY = SCREENY(tvPtr, entryPtr->worldY) +  (entryPtr->height -cbPtr->boxH) / 2;
    if (cbPtr->halo>=0) {
        int h = cbPtr->halo;
        
        if (x < (cbPtr->boxX - h) || x > (cbPtr->boxX + cbPtr->boxW + h) ||
            y < (boxY - h) || y > (boxY + cbPtr->boxH + h)) {
            *retVal = 0;
            return TCL_OK;
        }
    }
    if (isTest) {
        return TCL_OK;
    }
    if (strcmp(Tcl_GetString(objPtr), cbPtr->onValue) == 0) {
	objPtr = Tcl_NewStringObj(cbPtr->offValue, -1);
    } else {
	objPtr = Tcl_NewStringObj(cbPtr->onValue, -1);
    }
    entryPtr->flags |= ENTRY_DIRTY;
    tvPtr->flags |= (TV_DIRTY | TV_LAYOUT | TV_SCROLL | TV_RESORT);
    if (Blt_TreeSetValueByKey(tvPtr->interp, tvPtr->tree, 
	      entryPtr->node, columnPtr->key, objPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeCheckbox --
 *
 *	Releases resources allocated for the checkbox. The resources
 *	freed by this routine are specific only to the "checkbox".   
 *	Other resources (common to all styles) are freed in the 
 *	Blt_TreeViewFreeStyle routine.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GCs allocated for the checkbox are freed.
 *
 *----------------------------------------------------------------------
 */
static void
FreeCheckBox(tvPtr, stylePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
{
    TreeViewCheckBox *cbPtr = (TreeViewCheckBox *)stylePtr;

    FreeTextBox(tvPtr, stylePtr);
    if (cbPtr->fillGC != NULL) {
	Tk_FreeGC(tvPtr->display, cbPtr->fillGC);
    }
    if (cbPtr->boxGC != NULL) {
	Tk_FreeGC(tvPtr->display, cbPtr->boxGC);
    }
    if (cbPtr->checkGC != NULL) {
	Tk_FreeGC(tvPtr->display, cbPtr->checkGC);
    }
    if (cbPtr->offPtr != NULL) {
	Blt_Free(cbPtr->offPtr);
    }
    if (cbPtr->onPtr != NULL) {
	Blt_Free(cbPtr->onPtr);
    }
    if (cbPtr->bgGC != NULL) {
        Tk_FreeGC(tvPtr->display, cbPtr->bgGC);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CreateComboBox --
 *
 *	Creates a "combobox" style.
 *
 * Results:
 *	A pointer to the new style structure.
 *
 *----------------------------------------------------------------------
 */
static TreeViewStyle *
CreateComboBox(tvPtr, hPtr)
    TreeView *tvPtr;
    Blt_HashEntry *hPtr;
{
    TreeViewComboBox *cbPtr;

    cbPtr = (TreeViewComboBox *)newStyle(tvPtr, hPtr, sizeof(TreeViewComboBox));
    cbPtr->classPtr = &comboBoxClass;
    cbPtr->buttonRelief = TK_RELIEF_RAISED;
    cbPtr->buttonBorderWidth = 1;
    cbPtr->borderWidth = 1;
    cbPtr->flags = STYLE_COMBOBOX;
    return (TreeViewStyle *)cbPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureComboBox --
 *
 *	Configures a "combobox" style.  This routine performs 
 *	generates the GCs required for a combobox style.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GCs are created for the style.
 *
 *----------------------------------------------------------------------
 */
static void
ConfigureComboBox(tvPtr, stylePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
{
    ConfigureTextBox(tvPtr, stylePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * MeasureComboBox --
 *
 *	Determines the space requirements for the "combobox" given
 *	the value to be displayed.  Depending upon whether an icon
 *	or text is displayed and their relative placements, this
 *	routine computes the space needed for the text entry.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The width and height fields of *valuePtr* are set with the
 *	computed dimensions.
 *
 *----------------------------------------------------------------------
 */
static void
MeasureComboBox(tvPtr, stylePtr, valuePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
    TreeViewValue *valuePtr;
{
    TreeViewComboBox *cbPtr = (TreeViewComboBox *)stylePtr;
    TreeViewColumn *columnPtr = valuePtr->columnPtr;
    int iconWidth, iconHeight, biconWidth, biconHeight;
    int textWidth, textHeight;
    int gap;
    Tk_Font font;
    TreeViewIcon icon, *butIcons;

    textWidth = textHeight = 0;
    iconWidth = iconHeight = 0;
    valuePtr->width = valuePtr->height = 0;

    butIcons = cbPtr->buttonIcons;
    icon = (stylePtr->icon?stylePtr->icon:(columnPtr->stylePtr?columnPtr->stylePtr->icon:NULL));
    if (icon != NULL && tvPtr->hideStyleIcons == 0) {
        iconWidth = TreeViewIconWidth(icon);
        iconHeight = TreeViewIconHeight(icon);
    } 
    if (valuePtr->textPtr != NULL) {
	Blt_Free(valuePtr->textPtr);
	valuePtr->textPtr = NULL;
    }
    font = CHOOSE3(tvPtr->font, columnPtr->font, stylePtr->font);
    if (valuePtr->string != NULL) {	/* New string defined. */
	TextStyle ts;

	Blt_InitTextStyle(&ts);
	ts.font = font;
	ts.anchor = TK_ANCHOR_NW;
	ts.justify = valuePtr->columnPtr->justify;
	valuePtr->textPtr = Blt_GetTextLayoutStr(valuePtr->string, &ts);
    } 
    gap = 0;
    if (valuePtr->textPtr != NULL && tvPtr->hideStyleText == 0) {
	textWidth = valuePtr->textPtr->width;
	textHeight = valuePtr->textPtr->height;
	if (stylePtr->icon != NULL) {
	    gap = stylePtr->gap;
	}
    }
    if (butIcons != NULL) {
        TreeViewIcon mIcon;
        int i;
        
        biconWidth = 0;
        biconHeight = 0;
        for (i=0; i<2; i++) {
            mIcon = butIcons[i];
            if (!mIcon) continue;
            biconWidth = MAX(biconWidth,TreeViewIconWidth(mIcon));
            biconHeight = MAX(biconHeight,TreeViewIconHeight(mIcon));
        }
        biconHeight = MAX(iconHeight,biconHeight);
    } else {
        biconWidth = STD_ARROW_WIDTH + 6;
        biconHeight = iconHeight;
    }
    cbPtr->buttonWidth = biconWidth + 2 * cbPtr->buttonBorderWidth;
    valuePtr->width = 2 * cbPtr->borderWidth + iconWidth + 4 * gap + 
	cbPtr->buttonWidth + textWidth;
    valuePtr->height = MAX(textHeight, biconHeight) + 2 * cbPtr->borderWidth;
}

/*
 *----------------------------------------------------------------------
 *
 * DrawComboBox --
 *
 *	Draws the "combobox" given the screen coordinates and the
 *	value to be displayed.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The combobox value is drawn.
 *
 *----------------------------------------------------------------------
 */
static void
DrawComboBox(tvPtr, drawable, entryPtr, valuePtr, stylePtr, icon, x, y)
    TreeView *tvPtr;
    Drawable drawable;
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr;
    TreeViewStyle *stylePtr;
    TreeViewIcon icon;
    int x, y;
{
    GC gc;
    TreeViewColumn *columnPtr;
    TreeViewComboBox *cbPtr = (TreeViewComboBox *)stylePtr;
    int iconX, iconY, iconWidth, iconHeight;
    int textX, textY, textHeight;
    int buttonX, buttonY;
    int gap, columnWidth;
    TreeViewStyle sRec;
    int valWidth = (valuePtr->textPtr?valuePtr->width:(icon?TreeViewIconWidth(icon):0));
    TextLayout *layPtr;

    layPtr = (tvPtr->hideStyleText?NULL:valuePtr->textPtr);

    columnPtr = valuePtr->columnPtr;
    drawTextBox(tvPtr, drawable, entryPtr, valuePtr, stylePtr, icon, x, y, &sRec);
    gc = sRec.gc;

    if (!valuePtr->string) {
        return;
    }
    buttonX = x + columnPtr->width;
    buttonX -= columnPtr->pad.side2 + cbPtr->borderWidth  +
	cbPtr->buttonWidth + stylePtr->gap;
    buttonY = y;

    columnWidth = columnPtr->width - 
	(2 * columnPtr->borderWidth + PADDING(columnPtr->pad));
    if (columnWidth > valWidth) {
	switch(columnPtr->justify) {
	case TK_JUSTIFY_RIGHT:
	    x += (columnWidth - valWidth);
	    break;
	case TK_JUSTIFY_CENTER:
	    x += (columnWidth - valWidth) / 2;
	    break;
	case TK_JUSTIFY_LEFT:
	    break;
	}
    }

#ifdef notdef
    textX = textY = iconX = iconY = 0;	/* Suppress compiler warning. */
#endif
    
    iconWidth = iconHeight = 0;
    if (icon != NULL) {
	iconWidth = TreeViewIconWidth(icon);
	iconHeight = TreeViewIconHeight(icon);
    }
    textHeight = 0;
    if (layPtr != NULL) {
	textHeight = layPtr->height;
    }
    gap = 0;
    if ((icon != NULL) && (layPtr != NULL)) {
	gap = stylePtr->gap;
    }

    iconX = x + gap;
    iconY = y + (entryPtr->height - iconHeight) / 2;
    textX = iconX + iconWidth + gap;
    textY = y + (entryPtr->height - textHeight) / 2;

    valuePtr->iX = iconX;
    valuePtr->iY = iconY;
    valuePtr->iW = iconWidth;
    valuePtr->iH = iconHeight;
    valuePtr->tX = textX;
    valuePtr->tY = textY;
    valuePtr->tW = valuePtr->textPtr->width;
    valuePtr->tH = textHeight;
    if (icon != NULL) {
        if (Blt_TreeViewRedrawIcon(tvPtr, entryPtr, columnPtr, icon, 0, 0, iconWidth, 
	       iconHeight, drawable, iconX, iconY) != TCL_OK) return;
    } else {
        valuePtr->iW = 0;
    }
    if (layPtr != NULL) {
	TextStyle ts;
	XColor *color;

        int disabled = (entryPtr->state == STATE_DISABLED);
        
        if (disabled) {
            color = tvPtr->disabledColor;
        } else	if (Blt_TreeViewEntryIsSelected(tvPtr, entryPtr, columnPtr)) {
	   color = SELECT_FG(tvPtr);
        } else if (entryPtr->color != NULL) {
	    color = entryPtr->color;
	} else {
	    color = sRec.fgColor;
            GetColorPats(tvPtr, entryPtr, valuePtr, stylePtr, &color );
         }
	XSetForeground(tvPtr->display, gc, color->pixel);
	Blt_SetDrawTextStyle(&ts, sRec.font, gc, color, sRec.fgColor,
            stylePtr->shadow.color, 0.0, 
            TK_ANCHOR_NW, columnPtr->justify, 0, stylePtr->shadow.offset);
	Blt_DrawTextLayout(tvPtr->tkwin, drawable, valuePtr->textPtr, 
		&ts, textX, textY);
	if (color != sRec.fgColor) {
	    XSetForeground(tvPtr->display, gc, sRec.fgColor->pixel);
	}
    } else {
        valuePtr->tW = 0;
    }
    if (cbPtr->buttonIcons == NULL) {
        if (valuePtr == tvPtr->activeValuePtr) {
            Blt_Fill3DRectangle(tvPtr->tkwin, drawable, stylePtr->activeBorder, 
                buttonX, buttonY + cbPtr->borderWidth, cbPtr->buttonWidth, 
                entryPtr->height - 2 * cbPtr->borderWidth, 
                cbPtr->buttonBorderWidth, cbPtr->buttonRelief); 
        } else {
            Blt_Fill3DRectangle(tvPtr->tkwin, drawable, columnPtr->titleBorder, 
                buttonX, buttonY + cbPtr->borderWidth, cbPtr->buttonWidth, 
                entryPtr->height - 2 * cbPtr->borderWidth, 
                cbPtr->buttonBorderWidth, cbPtr->buttonRelief); 
        }
        buttonX += cbPtr->buttonWidth / 2;
        buttonY += entryPtr->height / 2;
        Blt_DrawArrow(tvPtr->display, drawable, gc, buttonX, buttonY, 
            STD_ARROW_HEIGHT, ARROW_DOWN);
        stylePtr->flags &= ~STYLE_DIRTY;
    } else {
        TreeViewIcon butIcon;
        int biconWidth;
        int biconHeight;
        if (valuePtr == tvPtr->activeValuePtr && cbPtr->buttonIcons[1]) {
            butIcon = cbPtr->buttonIcons[1];
        } else {
            butIcon = cbPtr->buttonIcons[0];
        }
        biconWidth = TreeViewIconWidth(butIcon);
        biconHeight = MAX(iconHeight,TreeViewIconHeight(butIcon));

        buttonY += (entryPtr->height-biconHeight)/2;
        Blt_TreeViewRedrawIcon(tvPtr, entryPtr, columnPtr, butIcon, 0, 0, biconWidth, 
            biconHeight, drawable, buttonX, buttonY);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PickCombobox --
 *
 *	Draws the "checkbox" given the screen coordinates and the
 *	value to be displayed.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The checkbox value is drawn.
 *
 *----------------------------------------------------------------------
 */
static int
PickComboBox(entryPtr, valuePtr, stylePtr, worldX, worldY)
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr;
    TreeViewStyle *stylePtr;
    int worldX, worldY;
{
    TreeViewColumn *columnPtr;
    TreeViewComboBox *cbPtr = (TreeViewComboBox *)stylePtr;
    int x, y, width, height;

    columnPtr = valuePtr->columnPtr;
    width = cbPtr->buttonWidth;
    height = entryPtr->height - 4;
    x = columnPtr->worldX + columnPtr->width - columnPtr->pad.side2 - 
	cbPtr->borderWidth - columnPtr->borderWidth - width;
    y = entryPtr->worldY + cbPtr->borderWidth;
    if ((worldX >= x) && (worldX < (x + width)) && 
	(worldY >= y) && (worldY < (y + height))) {
	return TRUE;
    }
    return FALSE;
}

/*
 *----------------------------------------------------------------------
 *
 * EditCombobox --
 *
 *	Edits the "combobox".
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The checkbox value is drawn.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EditComboBox(tvPtr, entryPtr, valuePtr, stylePtr, x, y, retVal)
    TreeView *tvPtr;
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr;
    TreeViewStyle *stylePtr;	/* Not used. */
    int x;
    int y;
    int *retVal;
{
    int isTest = *retVal;
    *retVal = 1;
    if (isTest) {
        return TCL_OK;
    }
    return Blt_TreeViewTextbox(tvPtr, entryPtr, valuePtr->columnPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * FreeComboBox --
 *
 *	Releases resources allocated for the combobox. The resources
 *	freed by this routine are specific only to the "combobox".   
 *	Other resources (common to all styles) are freed in the 
 *	Blt_TreeViewFreeStyle routine.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GCs allocated for the combobox are freed.
 *
 *----------------------------------------------------------------------
 */
static void
FreeComboBox(tvPtr, stylePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
{
    FreeTextBox(tvPtr, stylePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * CreateBarbox --
 *
 *	Creates a "checkbox" style.
 *
 * Results:
 *	A pointer to the new style structure.
 *
 *----------------------------------------------------------------------
 */
static TreeViewStyle *
CreateBarBox(tvPtr, hPtr)
    TreeView *tvPtr;
    Blt_HashEntry *hPtr;
{
    TreeViewBarBox *cbPtr;

    cbPtr = (TreeViewBarBox *)newStyle(tvPtr, hPtr, sizeof(TreeViewBarBox));
    cbPtr->classPtr = &barBoxClass;
    cbPtr->gap = 4;
    cbPtr->lineWidth = 1;
    cbPtr->showValue = TRUE;
    cbPtr->flags = STYLE_BARBOX;
    return (TreeViewStyle *)cbPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureBarbox --
 *
 *	Configures a "checkbox" style.  This routine performs 
 *	generates the GCs required for a checkbox style.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GCs are created for the style.
 *
 *----------------------------------------------------------------------
 */
static void
ConfigureBarBox(tvPtr, stylePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
{
    GC newGC;
    TreeViewBarBox *cbPtr = (TreeViewBarBox *)stylePtr;
    XColor *bgColor;
    XGCValues gcValues;
    unsigned long gcMask;

    bgColor = Tk_3DBorderColor(CHOOSE(tvPtr->border, stylePtr->border));

    gcValues.background = bgColor->pixel;
    gcValues.foreground = CHOOSE(tvPtr->fgColor, stylePtr->fgColor)->pixel;
    ConfigureTextBox(tvPtr, stylePtr);
    gcMask = GCForeground;
    gcValues.foreground = cbPtr->fillColor->pixel;
    newGC = Tk_GetGC(tvPtr->tkwin, gcMask, &gcValues);
    if (cbPtr->fillGC != NULL) {
	Tk_FreeGC(tvPtr->display, cbPtr->fillGC);
    }
    cbPtr->fillGC = newGC;

    gcMask = GCForeground | GCLineWidth;
    gcValues.line_width = cbPtr->lineWidth;
    gcValues.foreground = cbPtr->boxColor->pixel;
    newGC = Tk_GetGC(tvPtr->tkwin, gcMask, &gcValues);
    if (cbPtr->boxGC != NULL) {
	Tk_FreeGC(tvPtr->display, cbPtr->boxGC);
    }
    cbPtr->boxGC = newGC;
    if (cbPtr->barBg) {
        bgColor = Tk_3DBorderColor(cbPtr->barBg);
        gcValues.background = bgColor->pixel;
        gcValues.foreground = bgColor->pixel;
        newGC = Tk_GetGC(tvPtr->tkwin, gcMask, &gcValues);
        if (cbPtr->bgGC != NULL) {
            Tk_FreeGC(tvPtr->display, cbPtr->bgGC);
        }
        cbPtr->bgGC = newGC;
    }
    stylePtr->flags |= STYLE_DIRTY;
}

/*
 *----------------------------------------------------------------------
 *
 * MeasureBarbox --
 *
 *	Determines the space requirements for the "barbox" given
 *	the value to be displayed.  Depending upon whether an icon
 *	or text is displayed and their relative placements, this
 *	routine computes the space needed for the text entry.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The width and height fields of *valuePtr* are set with the
 *	computed dimensions.
 *
 *----------------------------------------------------------------------
 */
static void
MeasureBarBox(tvPtr, stylePtr, valuePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
    TreeViewValue *valuePtr;
{
    TreeViewBarBox *cbPtr = (TreeViewBarBox *)stylePtr;
    TreeViewColumn *columnPtr = valuePtr->columnPtr;
    TreeViewEntry *entryPtr;
    int iconWidth, iconHeight;
    int textWidth, textHeight;
    int gap, bGap;
    int boxWidth, boxHeight;
    TreeViewIcon icon;
    int rel = 0;

    entryPtr = valuePtr->entryPtr;
    bGap = stylePtr->gap;
    if (cbPtr->barWidth <= 0) {
        boxWidth = 0;
        bGap = 0;
    } else if (cbPtr->barWidth >= 4000) {
        boxWidth = 4000;
    } else {
        boxWidth = ODD(cbPtr->barWidth);
    }
    if (cbPtr->barHeight <= 0) {
        boxHeight = 0;
    } else if (cbPtr->barHeight >= 4000) {
        boxHeight = 4000;
    } else {
        boxHeight = ODD(cbPtr->barHeight);
    }

    textWidth = textHeight = iconWidth = iconHeight = 0;
    valuePtr->width = valuePtr->height = 0;
    icon = (stylePtr->icon?stylePtr->icon:(columnPtr->stylePtr?columnPtr->stylePtr->icon:NULL));
    if (icon != NULL && tvPtr->hideStyleIcons == 0) {
        iconWidth = TreeViewIconWidth(icon);
        iconHeight = TreeViewIconHeight(icon);
    } 
    /* TODO: recalc over/under only on change.*/
    gap = 0;
    if (cbPtr->showValue) {

        if (valuePtr->textPtr != NULL) {
            Blt_Free(valuePtr->textPtr);
            valuePtr->textPtr = NULL;
        }
        if (valuePtr->string != NULL) {
            /* New string defined. */
            TextStyle ts;
            Tcl_Obj *fmtObj;
	
            fmtObj = NotNullObj(cbPtr->formatCmd);
            if (fmtObj == NULL) {
                fmtObj = NotNullObj(columnPtr->formatCmd);
            }
            if (fmtObj == NULL) {
                fmtObj = NotNullObj(tvPtr->formatCmd);
            }

            Blt_InitTextStyle(&ts);
            ts.font = CHOOSE3(tvPtr->font, columnPtr->font, stylePtr->font);
            ts.anchor = TK_ANCHOR_NW;
            ts.justify = valuePtr->columnPtr->justify;
            if (fmtObj) {
                Tcl_DString cmdString;
                char *string;
                int result;
                Tcl_Interp *interp = tvPtr->interp;

                Tcl_Preserve(entryPtr);
                rel = 1;
                Blt_TreeViewPercentSubst(tvPtr, valuePtr->entryPtr, valuePtr->columnPtr, Tcl_GetString(fmtObj), valuePtr->string, &cmdString);
                result = Tcl_GlobalEval(interp, Tcl_DStringValue(&cmdString));
                if ((entryPtr->flags & ENTRY_DELETED) || (tvPtr->flags & TV_DELETED)) {
                    Tcl_Release(entryPtr);
                    return;
                }
                
                Blt_TreeViewOptsInit(tvPtr);
                Tcl_DStringFree(&cmdString);
                if (result == TCL_OK) {
                    string = Tcl_GetStringResult(interp);
                    valuePtr->textPtr = Blt_GetTextLayoutStr(string, &ts);
                }
            }
            if (valuePtr->textPtr == NULL) {
                valuePtr->textPtr = Blt_GetTextLayoutStr(valuePtr->string, &ts);
            }
        } 
        if (valuePtr->textPtr != NULL  && tvPtr->hideStyleText == 0) {
            textWidth = valuePtr->textPtr->width;
            textHeight = valuePtr->textPtr->height;
            if (stylePtr->icon != NULL) {
                gap = stylePtr->gap;
            }
        }
     
	if (stylePtr->icon != NULL) {
	    gap = stylePtr->gap;
	}
    }
    valuePtr->width = stylePtr->gap + bGap + boxWidth + iconWidth + gap + textWidth + 2;
    valuePtr->height = MAX3(boxHeight, textHeight, iconHeight);
    if (rel) Tcl_Release(entryPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * DrawBarbox --
 *
 *	Draws the "checkbox" given the screen coordinates and the
 *	value to be displayed.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The checkbox value is drawn.
 *
 *----------------------------------------------------------------------
 */
static void
DrawBarBox(tvPtr, drawable, entryPtr, valuePtr, stylePtr, icon, x, y)
    TreeView *tvPtr;
    Drawable drawable;
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr;
    TreeViewStyle *stylePtr;
    TreeViewIcon icon;
    int x, y;
{
    GC gc;
    TreeViewColumn *columnPtr;
    TreeViewBarBox *cbPtr = (TreeViewBarBox *)stylePtr;
    int iconX, iconY, iconWidth, iconHeight;
    int textX, textY, textHeight, ttlWidth, bGap, iGap;
    int gap, columnWidth;
    TextLayout *textPtr;
    int boxX, boxY, boxWidth, boxHeight, bw;
    TreeViewStyle *csPtr, sRec = *stylePtr;
    int valWidth = (valuePtr->textPtr?valuePtr->width:(icon?TreeViewIconWidth(icon):0));
    double curValue;
    TextStyle ts;
    XColor *color;
    int showValue = (tvPtr->hideStyleText ? 0 : cbPtr->showValue);

    textPtr = NULL;
    columnPtr = valuePtr->columnPtr;
    csPtr = columnPtr->stylePtr;
    
    drawTextBox(tvPtr, drawable, entryPtr, valuePtr, stylePtr, icon, x, y, &sRec);
    gc = sRec.gc;

    columnWidth = columnPtr->width - PADDING(columnPtr->pad);
    if (!valuePtr->string) {
        return;
    }
    if (Tcl_GetDouble(NULL, valuePtr->string, &curValue) != TCL_OK) {
        if (sscanf(valuePtr->string, "%lg", &curValue) != 1) {
            return;
        }
    }
    if (cbPtr->maxValue<=cbPtr->minValue) {
        return;
    }

#ifdef notdef
    textX = textY = iconX = iconY = 0;	/* Suppress compiler warning. */
#endif
    iconWidth = iconHeight = 0;
    bGap = stylePtr->gap;
    iGap = 0;
    if (cbPtr->barWidth<=0) {
        boxWidth = 0;
        bGap = 0;
    } else if (cbPtr->barWidth >= 4000) {
        boxWidth = 4000;
    } else {
        boxWidth = ODD(cbPtr->barWidth);
    }
    if (cbPtr->barHeight<=0) {
        boxHeight = 0;
    } else if (cbPtr->barHeight >= 4000) {
        boxHeight = 4000;
    } else {
        boxHeight = ODD(cbPtr->barHeight);
    }
    ttlWidth = ( boxWidth + stylePtr->gap * 2);
    if (showValue) {
        ttlWidth += (valWidth + stylePtr->gap);
        Blt_InitTextStyle(&ts);
        ts.font = CHOOSE(tvPtr->font, stylePtr->font);
        ts.anchor = TK_ANCHOR_NW;
        ts.justify = columnPtr->justify;
    }
    if (icon != NULL) {
        iGap = stylePtr->gap;
        iconWidth = TreeViewIconWidth(icon);
        iconHeight = TreeViewIconHeight(icon);
        ttlWidth += iconWidth + stylePtr->gap;
    }

    if (columnWidth > ttlWidth) {
	switch(columnPtr->justify) {
	case TK_JUSTIFY_RIGHT:
	    x += (columnWidth - ttlWidth - stylePtr->gap*2);
	    break;
	case TK_JUSTIFY_CENTER:
	    x += (columnWidth - ttlWidth) / 2;
	    break;
	case TK_JUSTIFY_LEFT:
	    break;
	}
    }

    /*
     * Draw the barbox 
     */
    boxX = x + iGap;
    boxY = y + (entryPtr->height - boxHeight) / 2;
    if (curValue<cbPtr->minValue) {
        bw = 0;
    } else if (curValue > cbPtr->maxValue) {
        bw = boxWidth;
    } else if (cbPtr->minValue >= cbPtr->maxValue) {
        bw = 0;
    } else {
        int diff = (cbPtr->maxValue-cbPtr->minValue);
        bw = (int)(boxWidth*(1.0*(curValue-cbPtr->minValue)/diff));
    }
    if (boxHeight && boxWidth) {
        if (cbPtr->barBg) {
            XFillRectangle(tvPtr->display, drawable, cbPtr->bgGC, boxX, boxY, boxWidth, boxHeight);
        }
        if (bw>0) {
            if (Blt_HasTile(stylePtr->fillTile)) {
                Blt_SetTileOrigin(tvPtr->tkwin, stylePtr->fillTile, -boxX, -boxY);
                Blt_TileRectangle(tvPtr->tkwin, drawable, stylePtr->fillTile,
                    boxX, boxY, bw, boxHeight );
            } else if (csPtr != NULL && Blt_HasTile(csPtr->fillTile)) {
                Blt_SetTileOrigin(tvPtr->tkwin, csPtr->fillTile, -boxX, -boxY);
                Blt_TileRectangle(tvPtr->tkwin, drawable, csPtr->fillTile,
                    boxX, boxY, bw, boxHeight );
            } else {

                XFillRectangle(tvPtr->display, drawable, cbPtr->fillGC, boxX, boxY, bw, boxHeight);
            }
        }
        XDrawRectangle(tvPtr->display, drawable, cbPtr->boxGC, boxX, boxY, 
            boxWidth, boxHeight);
    }

    textHeight = 0;
    gap = 0;
    if (showValue) {
	
        int disabled = (entryPtr->state == STATE_DISABLED);
        
        if (disabled) {
            color = tvPtr->disabledColor;
        } else if (Blt_TreeViewEntryIsSelected(tvPtr, entryPtr, columnPtr)) {
	    color = SELECT_FG(tvPtr);
	} else if (entryPtr->color != NULL) {
	    color = entryPtr->color;
	} else {
	    color = sRec.fgColor;
            GetColorPats(tvPtr, entryPtr, valuePtr, stylePtr, &color );
	}
	Blt_SetDrawTextStyle(&ts, sRec.font, gc, color, sRec.fgColor, stylePtr->shadow.color, 0.0, TK_ANCHOR_NW, columnPtr->justify, 0, stylePtr->shadow.offset);
        textPtr = valuePtr->textPtr; // Blt_GetTextLayout(string, &ts);
        textHeight = textPtr->height;
	if (icon != NULL) {
	    gap = stylePtr->gap;
	}
    }
    x = boxX + boxWidth + bGap;

    /* The icon sits to the left of the text. */
    iconX = x;
    iconY = y + (entryPtr->height - iconHeight) / 2;
    textX = iconX + iconWidth + gap;
    textY = y + (entryPtr->height - textHeight) / 2;

    valuePtr->iX = iconX;
    valuePtr->iY = iconY;
    valuePtr->iW = iconWidth;
    valuePtr->iH = iconHeight;
    valuePtr->tX = textX;
    valuePtr->tY = textY;
    valuePtr->tW = (textPtr?textPtr->width:0);
    valuePtr->tH = textHeight;
    
    if (icon != NULL) {
        if (Blt_TreeViewRedrawIcon(tvPtr, entryPtr, columnPtr, icon, 0, 0, iconWidth, 
		       iconHeight, drawable, iconX, iconY) != TCL_OK) return;
    } else {
        valuePtr->iW = 0;
    }
    if ((showValue) && (textPtr != NULL)) {
        XSetForeground(tvPtr->display, gc, color->pixel);
        Blt_DrawTextLayout(tvPtr->tkwin, drawable, textPtr, &ts, textX, textY);
	if (color != sRec.fgColor) {
	    XSetForeground(tvPtr->display, gc, sRec.fgColor->pixel);
	}
    } else {
        valuePtr->tW = 0;
    }
    stylePtr->flags &= ~STYLE_DIRTY;
}

#if 0
/*
 *----------------------------------------------------------------------
 *
 * PickBarbox --
 *
 *	Draws the "checkbox" given the screen coordinates and the
 *	value to be displayed.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The checkbox value is drawn.
 *
 *----------------------------------------------------------------------
 */
static int
PickBarBox(entryPtr, valuePtr, stylePtr, worldX, worldY)
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr;
    TreeViewStyle *stylePtr;
    int worldX, worldY;
{
    TreeViewColumn *columnPtr;
    TreeViewBarBox *cbPtr = (TreeViewBarBox *)stylePtr;
    int columnWidth;
    int x, y, width, height;

    columnPtr = valuePtr->columnPtr;
    columnWidth = columnPtr->width - 
	(2 * columnPtr->borderWidth + PADDING(columnPtr->pad));
    if (columnWidth > valuePtr->width) {
	switch(columnPtr->justify) {
	case TK_JUSTIFY_RIGHT:
	    worldX += (columnWidth - valuePtr->width);
	    break;
	case TK_JUSTIFY_CENTER:
	    worldX += (columnWidth - valuePtr->width) / 2;
	    break;
	case TK_JUSTIFY_LEFT:
	    break;
	}
    }
    width = ODD(cbPtr->barWidth) + 2 * cbPtr->lineWidth;
    height = ODD(cbPtr->barHeight) + 2 * cbPtr->lineWidth;
    x = columnPtr->worldX + columnPtr->pad.side1 + cbPtr->gap - 
	cbPtr->lineWidth;
    y = entryPtr->worldY + (entryPtr->height - height) / 2;
    if ((worldX >= x) && (worldX < (x + width)) && 
	(worldY >= y) && (worldY < (y + height))) {
	return TRUE;
    }
    return FALSE;
}
#endif
/*
 *----------------------------------------------------------------------
 *
 * EditBarbox --
 *
 *	Edits the "checkbox".
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The checkbox value is drawn.
 *
 *----------------------------------------------------------------------
 */
static int
EditBarBox(tvPtr, entryPtr, valuePtr, stylePtr, x, y, retVal)
    TreeView *tvPtr;
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr;
    TreeViewStyle *stylePtr;
    int x;
    int y;
    int *retVal;
{
    *retVal = 0;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeBarbox --
 *
 *	Releases resources allocated for the checkbox. The resources
 *	freed by this routine are specific only to the "checkbox".   
 *	Other resources (common to all styles) are freed in the 
 *	Blt_TreeViewFreeStyle routine.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GCs allocated for the checkbox are freed.
 *
 *----------------------------------------------------------------------
 */
static void
FreeBarBox(tvPtr, stylePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
{
    TreeViewBarBox *cbPtr = (TreeViewBarBox *)stylePtr;

    FreeTextBox(tvPtr, stylePtr);
    if (cbPtr->fillGC != NULL) {
	Tk_FreeGC(tvPtr->display, cbPtr->fillGC);
    }
    if (cbPtr->boxGC != NULL) {
	Tk_FreeGC(tvPtr->display, cbPtr->boxGC);
    }
    if (cbPtr->bgGC != NULL) {
        Tk_FreeGC(tvPtr->display, cbPtr->bgGC);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CreateWindowBox --
 *
 *	Creates a "textbox" style.
 *
 * Results:
 *	A pointer to the new style structure.
 *
 *----------------------------------------------------------------------
 */
static TreeViewStyle *
CreateWindowBox(tvPtr, hPtr)
    TreeView *tvPtr;
    Blt_HashEntry *hPtr;
{
    TreeViewWindowBox *tbPtr;

    tbPtr = (TreeViewWindowBox *)newStyle(tvPtr, hPtr, sizeof(TreeViewWindowBox));
    tbPtr->classPtr = &windowBoxClass;
    tbPtr->flags = STYLE_WINDOWBOX;
    return (TreeViewStyle *)tbPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureWindowBox --
 *
 *	Configures a "textbox" style.  This routine performs 
 *	generates the GCs required for a textbox style.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GCs are created for the style.
 *
 *----------------------------------------------------------------------
 */
static void
ConfigureWindowBox(tvPtr, stylePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
{
    TreeViewWindowBox *tbPtr = (TreeViewWindowBox *)stylePtr;
    tbPtr->flags |= STYLE_DIRTY;
}

/*
 *----------------------------------------------------------------------
 *
 * MeasureWindowBox --
 *
 *	Determines the space requirements for the "textbox" given
 *	the value to be displayed.  Depending upon whether an icon
 *	or text is displayed and their relative placements, this
 *	routine computes the space needed for the text entry.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The width and height fields of *valuePtr* are set with the
 *	computed dimensions.
 *
 *----------------------------------------------------------------------
 */
static void
MeasureWindowBox(tvPtr, stylePtr, valuePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
    TreeViewValue *valuePtr;
{
    TreeViewWindowBox *tbPtr = (TreeViewWindowBox *)stylePtr;
    valuePtr->width = tbPtr->windowWidth;
    valuePtr->height = tbPtr->windowHeight;
}


typedef struct {
    int flags;
    char *name;
    TreeView *tvPtr;
    TreeViewEntry *entryPtr;
    TreeViewColumn *columnPtr;
    Blt_HashEntry *hPtr;
    Tk_Window tkwin;
    TreeViewWindowBox *stylePtr;
} WindowCell;

static void EmbWinStructureProc( ClientData clientData, XEvent *eventPtr);

static void
EmbWinRemove(WindowCell *wcPtr) {
    Blt_HashEntry *hPtr;
    TreeView *tvPtr = wcPtr->tvPtr;
    hPtr = Blt_FindHashEntry(&tvPtr->winTable, wcPtr->name);
    if (hPtr != NULL && wcPtr == (WindowCell*)Blt_GetHashValue(hPtr)) {
        Blt_DeleteHashEntry(&tvPtr->winTable, hPtr);
    }
    Blt_Free(wcPtr->name);
    wcPtr->name = NULL;
    if (wcPtr->hPtr) Blt_DeleteHashEntry(&tvPtr->winCellTable, wcPtr->hPtr);
    wcPtr->hPtr = NULL;
    if (wcPtr->tkwin) {
        Tk_DeleteEventHandler(wcPtr->tkwin, StructureNotifyMask,
            EmbWinStructureProc, (ClientData) wcPtr);
        Tk_ManageGeometry(wcPtr->tkwin, NULL, NULL);
        wcPtr->tkwin = NULL;
    }
    Blt_Free(wcPtr);
}

/* Free all windows on widget delete. */
void
Blt_TreeViewFreeWindows( TreeView *tvPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    WindowCell *wcPtr;

    for (hPtr = Blt_FirstHashEntry(&tvPtr->winCellTable, &cursor); hPtr != NULL;
        hPtr = Blt_NextHashEntry(&cursor)) {
        wcPtr = Blt_GetHashValue(hPtr);
        if (wcPtr) {
            EmbWinRemove(wcPtr);
        }
    }
}

static void
EmbWinUnmapNow(Tk_Window ewTkwin, Tk_Window tkwin)
{
    if (tkwin != Tk_Parent(ewTkwin)) {
        Tk_UnmaintainGeometry(ewTkwin, tkwin);
    }
    Tk_UnmapWindow(ewTkwin);
}

static void EmbWinRequestProc(ClientData clientData,
    Tk_Window tkwin)
{
}
    
static void	EmbWinLostSlaveProc(ClientData clientData, Tk_Window tkwin);

static Tk_GeomMgr windowGeomType = {
    "treeview",			/* name */
    EmbWinRequestProc,		/* requestProc */
    EmbWinLostSlaveProc,	/* lostSlaveProc */
};


void
Blt_TreeViewMarkWindows( TreeView *tvPtr, int flag)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    WindowCell *wcPtr;

    for (hPtr = Blt_FirstHashEntry(&tvPtr->winCellTable, &cursor); hPtr != NULL;
    hPtr = Blt_NextHashEntry(&cursor)) {
        wcPtr = Blt_GetHashValue(hPtr);
        if (wcPtr) {
            if (flag == TV_WINDOW_CLEAR) {
                /* Clear window drawn flags. */
                wcPtr->flags &= ~TV_WINDOW_DRAW;
            } else if (flag == TV_WINDOW_UNMAP && (wcPtr->flags&TV_WINDOW_DRAW)==0) {
                /* Unmapped undrawn window. */
                EmbWinUnmapNow(wcPtr->tkwin, tvPtr->tkwin);
            }
        }
    }
}

static WindowCell *
bltTreeViewFindWindow(TreeViewEntry *entryPtr,
    TreeViewColumn *columnPtr)
{
    TreeView *tvPtr = columnPtr->tvPtr;
    Blt_HashEntry *hPtr;
    WindowCell *wcPtr;
    Tcl_DString dStr;
    char *key;
    
    Tcl_DStringInit(&dStr);
    Tcl_DStringAppendElement(&dStr, columnPtr->key);
    Tcl_DStringAppendElement(&dStr, Blt_Itoa(entryPtr->node->inode));
    key = Tcl_DStringValue(&dStr);
    hPtr = Blt_FindHashEntry(&tvPtr->winCellTable, key);
    Tcl_DStringFree(&dStr);
    if (hPtr == NULL) return NULL;
    wcPtr = (WindowCell*)Blt_GetHashValue(hPtr);
    return wcPtr;
}

static int isValidSubWin( TreeView *tvPtr, Tk_Window tkwin) {
    Tk_Window ancestor, parent;
    parent = Tk_Parent(tkwin);
    for (ancestor = tvPtr->tkwin; ; ancestor = Tk_Parent(ancestor)) {
        if (ancestor == parent) {
            break;
        }
        if (Tk_IsTopLevel(ancestor)) {
            return 0;
        }
    }
    if (Tk_IsTopLevel(tkwin) || (tkwin == tvPtr->tkwin)) {
        return 0;
    }
    return 1;
}

static WindowCell *
bltTreeViewWindowMake( TreeView *tvPtr, TreeViewEntry *entryPtr,
    TreeViewValue *valuePtr,  TreeViewWindowBox *wbPtr) 
{
    char *name, *key;
    Blt_HashEntry *hPtr;
    WindowCell *wcPtr = NULL, *w2Ptr;
    TreeViewColumn *columnPtr = valuePtr->columnPtr;
    Tcl_DString dStr;
    Tk_Window tkwin;
    int result, isNew;
    Tcl_Interp *interp;
    char *execCmd = wbPtr->windowCmd;

    interp = tvPtr->interp;

    Tcl_DStringInit(&dStr);
    Tcl_DStringAppendElement(&dStr, columnPtr->key);
    Tcl_DStringAppendElement(&dStr, Blt_Itoa(entryPtr->node->inode));
    key = Tcl_DStringValue(&dStr);
    hPtr = Blt_CreateHashEntry(&tvPtr->winCellTable, key, &isNew);
    Tcl_DStringFree(&dStr);
    Tcl_DStringInit(&dStr);

    if (hPtr == NULL) return NULL;
    if (!isNew) {
        wcPtr = (WindowCell*)Blt_GetHashValue(hPtr);
        if (wcPtr && wcPtr->tkwin == NULL) {
            return NULL;
        }
        return wcPtr;
    }
    
    wcPtr = (WindowCell*)Blt_Calloc(1, sizeof(WindowCell));
    Blt_SetHashValue(hPtr, wcPtr);
    wcPtr->name = NULL;
    wcPtr->tvPtr = tvPtr;
    wcPtr->columnPtr = columnPtr;
    wcPtr->entryPtr = entryPtr;
    wcPtr->hPtr = hPtr;
    wcPtr->stylePtr = wbPtr;
    wcPtr->flags |= TV_WINDOW_DRAW;
    
    if (execCmd != NULL) {
        Tcl_DString cmdString;

        Blt_TreeViewPercentSubst(tvPtr, entryPtr, columnPtr, execCmd, valuePtr->string, &cmdString);
        result = Tcl_GlobalEval(interp, Tcl_DStringValue(&cmdString));
        Blt_TreeViewOptsInit(tvPtr);
        Tcl_DStringFree(&cmdString);
        if ((entryPtr->flags & ENTRY_DELETED) || (tvPtr->flags & TV_DELETED)) {
            result = TCL_ERROR;
        }
        if (result != TCL_OK) {
            goto error;
        }
        name = Tcl_GetStringResult(interp);
    } else {
        name = valuePtr->string;
        if (name == NULL) {
            goto error;
        }
        if (name[0] != '.') {
            Tcl_DStringAppend(&dStr, Tk_PathName(tvPtr->tkwin), -1);
            Tcl_DStringAppend(&dStr, ".", -1);
            Tcl_DStringAppend(&dStr, name, -1);
            name = Tcl_DStringValue(&dStr);
        }
    }
    tkwin = Tk_NameToWindow(tvPtr->interp, name, tvPtr->tkwin);
    if (tkwin == NULL || !isValidSubWin(tvPtr, tkwin)) {
        goto error;
    }
    if (name && name[0]) {

        wcPtr->name = Blt_Strdup(name);
        hPtr = Blt_CreateHashEntry(&tvPtr->winTable, wcPtr->name, &isNew);
        if (hPtr != NULL) {
            w2Ptr = (WindowCell*)Blt_GetHashValue(hPtr);
            if (w2Ptr && (w2Ptr->flags&TV_WINDOW_DRAW)) {
                goto error;
            }
            Blt_SetHashValue(hPtr, wcPtr);
        }
    }
    Tcl_DStringFree(&dStr);
    if (execCmd == NULL) {
        entryPtr->flags |= ENTRY_DATA_WINDOW;
    }
    entryPtr->flags |= ENTRY_WINDOW;
    wcPtr->tkwin = tkwin;
    Tk_ManageGeometry(tkwin, &windowGeomType, wcPtr);
    Tk_CreateEventHandler(tkwin, StructureNotifyMask,
        EmbWinStructureProc, (ClientData) wcPtr);
    return wcPtr;

error:
    if (wcPtr) {
        Blt_DeleteHashEntry(&tvPtr->winCellTable, wcPtr->hPtr);
        if (wcPtr->name) Blt_Free(wcPtr->name);
        Blt_Free(wcPtr);
    }
    Tcl_DStringFree(&dStr);
    return NULL;
}

/* Some other manager took control. */
static void	EmbWinLostSlaveProc(ClientData clientData, Tk_Window tkwin)
{
    register WindowCell *wcPtr = (WindowCell *) clientData;
    EmbWinUnmapNow(tkwin, wcPtr->tvPtr->tkwin);
    EmbWinRemove(wcPtr);
}

static void
EmbWinStructureProc(clientData, eventPtr)
    ClientData clientData;	/* Pointer to record describing window item. */
    XEvent *eventPtr;		/* Describes what just happened. */
{
    register WindowCell *ewPtr = (WindowCell *) clientData;

    if (eventPtr->type != DestroyNotify) {
	return;
    }
    EmbWinRemove(ewPtr);
}

int
Blt_WinResizeAlways(Tk_Window tkwin)
{
    while (tkwin != NULL) {
#ifdef Tk_IsResizeYes
        if (Tk_IsResizeYes(tkwin)) return 1;
#endif
        if (Tk_TopWinHierarchy(tkwin)) break;
        tkwin = Tk_Parent(tkwin);
    }
    return 0;
}

static void
EmbWinDisplay(TreeView *tvPtr, WindowCell *wcPtr,
	      int x, int y, int width, int height)
{
    Tk_Window tkwin = tvPtr->tkwin;
    Tk_Window cwTkwin = wcPtr->tkwin;
    int diff, reqWidth, reqHeight, sticky, diffx, diffy;


    if ((diff = (y-tvPtr->titleHeight- tvPtr->insetY))<0) {
        height += diff;
        y -= diff;
    }
    if (width < 2 || height < 2) {
	if (wcPtr->flags&TV_WINDOW_DRAW) {
	    EmbWinUnmapNow(cwTkwin, tkwin);
	}
	return;
    }
    reqWidth = Tk_ReqWidth(cwTkwin);
    if (reqWidth > width) { reqWidth = width; }
    reqHeight = Tk_ReqHeight(cwTkwin);
    if (reqHeight > height) { reqHeight = height; }
    diffx = width - reqWidth;
    diffy = height - reqHeight;
    
    sticky = wcPtr->stylePtr->sticky;
    
    if (sticky&STICK_EAST && sticky&STICK_WEST) {
        reqWidth += diffx;
    }
    if (sticky&STICK_NORTH && sticky&STICK_SOUTH) {
        reqHeight += diffy;
    }
    if (!(sticky&STICK_WEST)) {
        x += (sticky&STICK_EAST) ? diffx : diffx/2;
    }
    if (!(sticky&STICK_NORTH)) {
        y += (sticky&STICK_SOUTH) ? diffy : diffy/2;
    }
    width = reqWidth;
    height = reqHeight;
    if (tkwin == Tk_Parent(cwTkwin)) {
	if ((x != Tk_X(cwTkwin)) || (y != Tk_Y(cwTkwin))
	    || (width != Tk_Width(cwTkwin))
	    || (height != Tk_Height(cwTkwin))
	    || Blt_WinResizeAlways(cwTkwin)
	) {
	    Tk_MoveResizeWindow(cwTkwin, x, y, width, height);
	}
	Tk_MapWindow(cwTkwin);
    } else {
	Tk_MaintainGeometry(cwTkwin, tkwin, x, y, width, height);
    }
    wcPtr->flags |= TV_WINDOW_DRAW;
}

/*
 * Called when a cell has a window from data.
 */
void
Blt_TreeViewWindowUpdate(TreeViewEntry *entryPtr, TreeViewColumn *columnPtr)
{
    TreeViewWindowBox *wbPtr;
    WindowCell *wcPtr;
    
    if (entryPtr->flags & ENTRY_DATA_WINDOW) {
        /* Cell may already have a window from data. */

    } else if (columnPtr->stylePtr != NULL &&
        columnPtr->stylePtr->classPtr->className[0] == 'w') {
        wbPtr = (TreeViewWindowBox *)columnPtr->stylePtr;
        if (wbPtr->windowCmd != NULL) return;
        
    } else {
        return;
    }
    wcPtr = bltTreeViewFindWindow(entryPtr, columnPtr);
    if (wcPtr == NULL || wcPtr->tkwin == NULL) return;
    EmbWinUnmapNow(wcPtr->tkwin, wcPtr->tvPtr->tkwin);
    Tk_ManageGeometry(wcPtr->tkwin, NULL, NULL);
    EmbWinRemove(wcPtr);
}

void
Blt_TreeViewWindowRelease(TreeViewEntry *entryPtr, TreeViewColumn *columnPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    WindowCell *wcPtr;
    TreeView *tvPtr;
    
    tvPtr = (columnPtr?columnPtr->tvPtr:entryPtr->tvPtr);

    for (hPtr = Blt_FirstHashEntry(&tvPtr->winCellTable, &cursor); hPtr != NULL;
    hPtr = Blt_NextHashEntry(&cursor)) {
        wcPtr = (WindowCell*)Blt_GetHashValue(hPtr);
        if (wcPtr == NULL) continue;
        if (entryPtr && entryPtr == wcPtr->entryPtr) {
            EmbWinUnmapNow(wcPtr->tkwin, wcPtr->tvPtr->tkwin);
            EmbWinRemove(wcPtr);
            continue;
        }
        if (columnPtr && columnPtr == wcPtr->columnPtr) {
            EmbWinUnmapNow(wcPtr->tkwin, wcPtr->tvPtr->tkwin);
            EmbWinRemove(wcPtr);
            continue;
        }
    }
}
/*
 *----------------------------------------------------------------------
 *
 * DrawWindowBox --
 *
 *	Draws the "windowbox" given the screen coordinates.
 *	The window name is taken from the data value.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The window is drawn.
 *
 *----------------------------------------------------------------------
  */
static void
DrawWindowBox(tvPtr, drawable, entryPtr, valuePtr, stylePtr, icon, x, y)
    TreeView *tvPtr;
    Drawable drawable;
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr;
    TreeViewStyle *stylePtr;
    TreeViewIcon icon;
    int x, y;
{
    TreeViewColumn *columnPtr;
    TreeViewWindowBox *tbPtr = (TreeViewWindowBox *)stylePtr;
    int width, height, diff;
    WindowCell *wcPtr;
    TreeViewStyle sRec = *stylePtr;

    columnPtr = valuePtr->columnPtr;
    
    Tcl_Preserve(entryPtr);
    wcPtr = bltTreeViewWindowMake(tvPtr, entryPtr, valuePtr, tbPtr);
    if ((entryPtr->flags & ENTRY_DELETED) || (tvPtr->flags & TV_DELETED)) {
        Tcl_Release(entryPtr);
        return;
    }
    Tcl_Release(entryPtr);
    
    drawTextBox(tvPtr, drawable, entryPtr, valuePtr, stylePtr, icon, x, y, &sRec);

    if (wcPtr != NULL) {
        int columnWidth = columnPtr->width - 
            (2 * columnPtr->borderWidth + PADDING(columnPtr->pad));
            
        width = columnWidth;
        height = entryPtr->height-1;
        if ((diff = (y-tvPtr->titleHeight- tvPtr->insetY))<0) {
            height += diff;
            y -= diff;
        }
        EmbWinDisplay(tvPtr, wcPtr, x, y, width, height);
    }
    stylePtr->flags &= ~STYLE_DIRTY;
}

/*
 *----------------------------------------------------------------------
 *
 * EditWindowbox --
 *
 *	Edits the "windowbox".
 *
 * Results:
 *	None.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
EditWindowBox(tvPtr, entryPtr, valuePtr, stylePtr, x, y, retVal)
    TreeView *tvPtr;
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr;
    TreeViewStyle *stylePtr;	/* Not used. */
    int x;
    int y;
    int *retVal;
{
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeWindowBox --
 *
 *	Releases resources allocated for the textbox. The resources
 *	freed by this routine are specific only to the "textbox".   
 *	Other resources (common to all styles) are freed in the 
 *	Blt_TreeViewFreeStyle routine.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	GCs allocated for the textbox are freed.
 *
 *----------------------------------------------------------------------
 */
static void
FreeWindowBox(tvPtr, stylePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
{
    TreeViewWindowBox *tbPtr = (TreeViewWindowBox *)stylePtr;
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    WindowCell *wcPtr;

    if (!tbPtr) return;

    for (hPtr = Blt_FirstHashEntry(&tvPtr->winCellTable, &cursor); hPtr != NULL;
        hPtr = Blt_NextHashEntry(&cursor)) {
        wcPtr = (WindowCell*)Blt_GetHashValue(hPtr);
        if (wcPtr && stylePtr == (TreeViewStyle*)wcPtr->stylePtr) {
            EmbWinRemove(wcPtr);
        }
    }

}

static TreeViewStyle *
GetStyle(interp, tvPtr, styleName)
    Tcl_Interp *interp;
    TreeView *tvPtr;
    char *styleName;
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&tvPtr->styleTable, styleName);
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find style \"", styleName, 
		"\"", (char *)NULL);
	}
	return NULL;
    }
    return Blt_GetHashValue(hPtr);
}

int
Blt_TreeViewGetStyle(interp, tvPtr, styleName, stylePtrPtr)
    Tcl_Interp *interp;
    TreeView *tvPtr;
    char *styleName;
    TreeViewStyle **stylePtrPtr;
{
    TreeViewStyle *stylePtr;

    stylePtr = GetStyle(interp, tvPtr, styleName);
    if (stylePtr == NULL) {
	return TCL_ERROR;
    }
    stylePtr->refCount++;
    *stylePtrPtr = stylePtr;
    return TCL_OK;
}

int
Blt_TreeViewGetStyleMake(interp, tvPtr, styleName, stylePtrPtr, columnPtr, entryPtr, valuePtr)
    Tcl_Interp *interp;
    TreeView *tvPtr;
    char *styleName;
    TreeViewStyle **stylePtrPtr;
    TreeViewColumn *columnPtr;
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr;
{
    TreeViewStyle *stylePtr = NULL;
    Tcl_DString dString;
    int result;
    
    if (strlen(styleName)==0) {
        Tcl_AppendResult(interp, "can not use style with empty name", 0);
        return TCL_ERROR;
    }
    if (Blt_TreeViewGetStyle(NULL, tvPtr, styleName, &stylePtr) != TCL_OK) {
        if (tvPtr->styleCmd == NULL) {
            if (stylePtr == NULL) {
                if (interp != NULL) {
                    Tcl_AppendResult(interp, "style not found: ", styleName, 0);
                }
                return TCL_ERROR;
            }
        } else if (!strcmp(tvPtr->styleCmd, "%W style create textbox %V")) {
            stylePtr = Blt_TreeViewCreateStyle(interp, tvPtr, STYLE_TEXTBOX, styleName);
            assert(stylePtr);
            Blt_TreeViewUpdateStyleGCs(tvPtr, stylePtr);

        } else {
            Tcl_DStringInit(&dString);
            Blt_TreeViewPercentSubst(tvPtr, entryPtr, columnPtr, tvPtr->styleCmd, styleName, &dString);
            /* TODO: should save/restore intep state. */
            result = Tcl_GlobalEval(tvPtr->interp, Tcl_DStringValue(&dString));
            Tcl_DStringFree(&dString);
            if ((tvPtr->flags & TV_DELETED)) {
                return TCL_ERROR;
            }
            if (result != TCL_OK) {
                if (interp != NULL) {
                    Tcl_AppendResult(interp, "error in -stylecommand ", tvPtr->styleCmd, " for: ", styleName, 0);
                }
                return TCL_ERROR;
            }
            result = Blt_TreeViewGetStyle(interp, tvPtr, styleName, &stylePtr);
            if (result != TCL_OK) {
                if (interp != NULL) {
                    Tcl_AppendResult(interp, "style not found: ", styleName, 0);
                }
                return TCL_ERROR;
            }
            Blt_TreeViewUpdateStyleGCs(tvPtr, stylePtr);
            if (interp != NULL) {
                Tcl_ResetResult(interp);
            }
        }
    }
    *stylePtrPtr = stylePtr;
    return TCL_OK;
}

static TreeViewStyle *
CreateStyle(interp, tvPtr, type, styleName, objc, objv, create)
     Tcl_Interp *interp;
     TreeView *tvPtr;		/* TreeView widget. */
     int type;			/* Type of style: either
				 * STYLE_TEXTBOX,
				 * STYLE_COMBOBOX, or
				 * STYLE_CHECKBOX */
    char *styleName;		/* Name of the new style. */
    int objc;
    Tcl_Obj *CONST *objv;
    int create;
{    
    Blt_HashEntry *hPtr;
    int isNew, ref = 1;
    TreeViewStyle *stylePtr;
    
    hPtr = Blt_CreateHashEntry(&tvPtr->styleTable, styleName, &isNew);
    if (!isNew) {
        /* Don't kick an error unless styleCmd is changed. */
        if (create && (tvPtr->styleCmd == NULL || strcmp(tvPtr->styleCmd, "%W style create textbox %V") == 0 || (tvPtr->styleCmd[0] == 0))) {
            stylePtr = Blt_GetHashValue(hPtr);
            if (type == (stylePtr->flags&STYLE_TYPE)) {
                ref = 0;
                goto doconf;
            }
        }
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "style \"", styleName, 
			     "\" already exists", (char *)NULL);
	}
	return NULL;
    }
    /* Create the new marker based upon the given type */
    switch (type) {
    case STYLE_TEXTBOX:
	stylePtr = CreateTextBox(tvPtr, hPtr);
	break;
    case STYLE_COMBOBOX:
	stylePtr = CreateComboBox(tvPtr, hPtr);
	break;
    case STYLE_CHECKBOX:
	stylePtr = CreateCheckBox(tvPtr, hPtr);
	break;
    case STYLE_WINDOWBOX:
	stylePtr = CreateWindowBox(tvPtr, hPtr);
	break;
    case STYLE_BARBOX:
	stylePtr = CreateBarBox(tvPtr, hPtr);
	break;
    default:
	return NULL;
    }
doconf:
    Blt_TreeViewOptsInit(tvPtr);
    if (Blt_ConfigureComponentFromObj(tvPtr->interp, tvPtr->tkwin, styleName, 
	stylePtr->classPtr->className, stylePtr->classPtr->specsPtr, 
	objc, objv, (char *)stylePtr, 0) != TCL_OK) {
        if (ref) {
            Blt_TreeViewFreeStyle(tvPtr, stylePtr);
        }
        return NULL;
    }
    if (stylePtr->tile != NULL) {
        Blt_SetTileChangedProc(stylePtr->tile, Blt_TreeViewTileChangedProc, tvPtr);
    }
    if (stylePtr->fillTile != NULL) {
        Blt_SetTileChangedProc(stylePtr->fillTile, Blt_TreeViewTileChangedProc, tvPtr);
    }
    stylePtr->refCount += ref;
    return stylePtr;
}

void
Blt_TreeViewUpdateStyleGCs(tvPtr, stylePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
{
    if (tvPtr->font == NULL) return;
    (*stylePtr->classPtr->configProc)(tvPtr, stylePtr);
    stylePtr->flags |= STYLE_DIRTY;
    Blt_TreeViewEventuallyRedraw(tvPtr);
}

void
Blt_TreeViewUpdateStyles(tvPtr)
    TreeView *tvPtr;
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    TreeViewStyle *stylePtr;
    Blt_TreeViewUpdateStyleGCs(tvPtr, tvPtr->stylePtr);

    for (hPtr = Blt_FirstHashEntry(&tvPtr->styleTable, &cursor); hPtr != NULL;
        hPtr = Blt_NextHashEntry(&cursor)) {
        stylePtr = Blt_GetHashValue(hPtr);
        Blt_TreeViewUpdateStyleGCs(tvPtr, stylePtr);
    }
}

TreeViewStyle *
Blt_TreeViewCreateStyle(interp, tvPtr, type, styleName)
     Tcl_Interp *interp;
     TreeView *tvPtr;		/* TreeView widget. */
     int type;			/* Type of style: either
				 * STYLE_TEXTBOX,
				 * STYLE_COMBOBOX, or
				 * STYLE_CHECKBOX */
    char *styleName;		/* Name of the new style. */
{    
    return CreateStyle(interp, tvPtr, type, styleName, 0, (Tcl_Obj **)NULL, 0);
}

void
Blt_TreeViewFreeStyle(tvPtr, stylePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
{
    stylePtr->refCount--;
#ifdef notdef
    fprint(f(stderr, "Blt_TreeViewFreeStyle %s count=%d\n", stylePtr->name,
	    stylePtr->refCount);
#endif
    /* Remove the style from the hash table so that it's name can be used.*/
    /* If no cell is using the style, remove it.*/
    if ((stylePtr->refCount <= 0) && !(stylePtr->flags & STYLE_USER)){
#ifdef notdef
	fprintf(stderr, "freeing %s\n", stylePtr->name);
#endif
        /*Blt_HashEntry *hPtr;
        hPtr = Blt_FindHashEntry(&tvPtr->styleTagTable, stylePtr->name, &isNew);
        if (hPtr != NULL) {
            Blt_DeleteHashEntry(&tvPtr->styleTagTable, hPtr);
        }*/
        Blt_TreeViewOptsInit(tvPtr);
	Blt_FreeObjOptions(tvPtr->interp,
	           stylePtr->classPtr->specsPtr, (char *)stylePtr, 
		   tvPtr->display, 0);
	(*stylePtr->classPtr->freeProc)(tvPtr, stylePtr); 
	if (stylePtr->hashPtr != NULL) {
	    Blt_DeleteHashEntry(&tvPtr->styleTable, stylePtr->hashPtr);
	} 
	if (stylePtr->name != NULL) {
	    Blt_Free(stylePtr->name);
	}
	if (tvPtr->subStylePtr == stylePtr) { tvPtr->subStylePtr = NULL; }
	if (tvPtr->altStylePtr == stylePtr) { tvPtr->altStylePtr = NULL; }
	if (tvPtr->emptyStylePtr == stylePtr) { tvPtr->emptyStylePtr = NULL; }
	Blt_Free(stylePtr);
    } 
}

void
Blt_TreeViewSetStyleIcon(tvPtr, stylePtr, icon)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
    TreeViewIcon icon;
{
    TreeViewTextBox *tbPtr = (TreeViewTextBox *)stylePtr;

    if (tbPtr && tbPtr->icon != NULL) {
	Blt_TreeViewFreeIcon(tvPtr, tbPtr->icon);
    }
    tbPtr->icon = icon;
}

GC
Blt_TreeViewGetStyleGC(tvPtr, stylePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
{
    TreeViewTextBox *tbPtr = (TreeViewTextBox *)stylePtr;
    return (tbPtr && tbPtr->gc) ? tbPtr->gc : tvPtr->stylePtr->gc;
}

Tk_3DBorder
Blt_TreeViewGetStyleBorder(tvPtr, stylePtr)
    TreeView *tvPtr;
    TreeViewStyle *stylePtr;
{
    TreeViewTextBox *tbPtr = (TreeViewTextBox *)stylePtr;
    Tk_3DBorder border;

    if (!tbPtr) {
        return tvPtr->border;
    }
    border = (tbPtr->flags & STYLE_HIGHLIGHT) 
        ? tbPtr->highlightBorder : tbPtr->border;
    return (border != NULL) ? border : tvPtr->border;
}

Tk_Font
Blt_TreeViewGetStyleFont(tvPtr, columnPtr, stylePtr)
    TreeView *tvPtr;
    TreeViewColumn *columnPtr;
    TreeViewStyle *stylePtr;
{
    TreeViewTextBox *tbPtr = (TreeViewTextBox *)stylePtr;

    if (tbPtr != NULL && tbPtr->font != NULL) {
	return tbPtr->font;
    }
    if (columnPtr != NULL && columnPtr->font != NULL) {
        return columnPtr->font;
    }
    return tvPtr->font;
}

XColor *
Blt_TreeViewGetStyleFg(tvPtr, columnPtr, stylePtr)
    TreeView *tvPtr;
    TreeViewColumn *columnPtr;
    TreeViewStyle *stylePtr;
{
    TreeViewTextBox *tbPtr = (TreeViewTextBox *)stylePtr;

    if (tbPtr && tbPtr->fgColor != NULL) {
	return tbPtr->fgColor;
    }
    if (columnPtr && columnPtr->fgColor) {
        return columnPtr->fgColor;
    }
    return tvPtr->fgColor;
}

static void
DrawValue(tvPtr, entryPtr, valuePtr)
    TreeView *tvPtr;
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr;
{
    Drawable drawable;
    int sx, sy, dx, dy;
    int width, height, ishid;
    int left, right, top, bottom;
    TreeViewColumn *columnPtr;
    TreeViewStyle *stylePtr;
    Tk_3DBorder selBorder;

    stylePtr = valuePtr->stylePtr;
    ishid = (stylePtr?stylePtr->hidden:0);
    if (stylePtr == NULL) {
	stylePtr = CHOOSE(tvPtr->stylePtr, valuePtr->columnPtr->stylePtr);
    }
    if (stylePtr->cursor != None) {
	if (valuePtr == tvPtr->activeValuePtr) {
	    Tk_DefineCursor(tvPtr->tkwin, stylePtr->cursor);
	} else {
	    if (tvPtr->cursor != None) {
		Tk_DefineCursor(tvPtr->tkwin, tvPtr->cursor);
	    } else {
		Tk_UndefineCursor(tvPtr->tkwin);
	    }
	}
    }
    columnPtr = valuePtr->columnPtr;
    dx = SCREENX(tvPtr, columnPtr->worldX) + columnPtr->pad.side1-1;
    dy = SCREENY(tvPtr, entryPtr->worldY);
    height = entryPtr->height - 1;
    width = valuePtr->columnPtr->width - PADDING(columnPtr->pad);

    top = tvPtr->titleHeight + tvPtr->insetY;
    bottom = Tk_Height(tvPtr->tkwin) - tvPtr->insetY;
    left = tvPtr->insetX;
    right = Tk_Width(tvPtr->tkwin) - tvPtr->insetX;

    if (((dx + width) < left) || (dx > right) ||
	((dy + height) < top) || (dy > bottom)) {
	return;			/* Value is clipped. */
    }

    drawable = Tk_GetPixmap(tvPtr->display, Tk_WindowId(tvPtr->tkwin), 
	width, height, Tk_Depth(tvPtr->tkwin));
    /* Draw the background of the value. */

    if (Blt_TreeViewEntryIsSelected(tvPtr, entryPtr, columnPtr)) {
        selBorder = SELECT_BORDER(tvPtr);
        Blt_Fill3DRectangle(tvPtr->tkwin, drawable, selBorder, 0, 0, width, height,
            0, TK_RELIEF_FLAT);
    } else if ((valuePtr == tvPtr->activeValuePtr) ||
	(!Blt_TreeViewEntryIsSelected(tvPtr, entryPtr, columnPtr))) {
	Tk_3DBorder border;

	border = Blt_TreeViewGetStyleBorder(tvPtr, tvPtr->stylePtr);
	Blt_Fill3DRectangle(tvPtr->tkwin, drawable, border, 0, 0, width, height,
		0, TK_RELIEF_FLAT);
    } else if (Blt_HasTile(tvPtr->tile) == 0 && Blt_HasTile(columnPtr->tile) == 0) {

	Blt_Draw3DRectangle(tvPtr->tkwin, drawable, SELECT_BORDER(tvPtr), 0, 0, 
		width, height, tvPtr->selBorderWidth, tvPtr->selRelief);
    }
    Blt_TreeViewDrawValue(tvPtr, entryPtr, valuePtr, drawable, 0, 0, entryPtr->flags & ENTRY_ALTROW, ishid);
    
    /* Clip the drawable if necessary */
    sx = sy = 0;
    if (dx < left) {
	width -= left - dx;
	sx += left - dx;
	dx = left;
    }
    if ((dx + width) >= right) {
	width -= (dx + width) - right;
    }
    if (dy < top) {
	height -= top - dy;
	sy += top - dy;
	dy = top;
    }
    if ((dy + height) >= bottom) {
	height -= (dy + height) - bottom;
    }
    XCopyArea(tvPtr->display, drawable, Tk_WindowId(tvPtr->tkwin), 
      tvPtr->lineGC, sx, sy, width,  height, dx, dy);
    Tk_FreePixmap(tvPtr->display, drawable);
}

/*
 *----------------------------------------------------------------------
 *
 * StyleActivateOp --
 *
 * 	Turns on/off highlighting for a particular style.
 *
 *	  .t style activate entry column
 *
 * Results:
 *	A standard Tcl result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleActivateOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;

    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr, *oldPtr;

    oldPtr = tvPtr->activeValuePtr;
    if (objc == 3) {
	Tcl_Obj *listObjPtr; 

	valuePtr = tvPtr->activeValuePtr;
	entryPtr = tvPtr->activePtr;
	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	if ((entryPtr != NULL) && (valuePtr != NULL)) {
	    Tcl_Obj *objPtr; 
	    objPtr = Tcl_NewIntObj(Blt_TreeNodeId(entryPtr->node));
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    objPtr = Tcl_NewStringObj(valuePtr->columnPtr->key, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	} 
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    } else if (objc == 4) {
	tvPtr->activeValuePtr = NULL;
	if ((oldPtr != NULL)  && (tvPtr->activePtr != NULL)) {
	    DrawValue(tvPtr, tvPtr->activePtr, oldPtr);
	}
    } else {
	TreeViewColumn *columnPtr;

	if (Blt_TreeViewGetEntry(tvPtr, objv[3], &entryPtr) != TCL_OK) {
	    return TCL_ERROR;
	}

	if (Blt_TreeViewGetColumn(interp, tvPtr, objv[4], &columnPtr) 
	    != TCL_OK) {
	    return TCL_ERROR;
	}
	valuePtr = Blt_TreeViewFindValue(entryPtr, columnPtr);
	if (valuePtr == NULL) {
	    return TCL_OK;
	}
	tvPtr->activePtr = entryPtr;
	tvPtr->activeColumnPtr = columnPtr;
	oldPtr = tvPtr->activeValuePtr;
	tvPtr->activeValuePtr = valuePtr;
	if (valuePtr != oldPtr) {
	    if (oldPtr != NULL) {
		DrawValue(tvPtr, entryPtr, oldPtr);
	    }
	    if (valuePtr != NULL) {
		DrawValue(tvPtr, entryPtr, valuePtr);
	    }
	}
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * StyleCgetOp --
 *
 *	  .t style cget "styleName" -background
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleCgetOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewStyle *stylePtr;

    stylePtr = GetStyle(interp, tvPtr, Tcl_GetString(objv[3]));
    if (stylePtr == NULL) {
	return TCL_ERROR;
    }
    Blt_TreeViewOptsInit(tvPtr);
    return Blt_ConfigureValueFromObj(interp, tvPtr->tkwin, 
	stylePtr->classPtr->specsPtr, (char *)stylePtr, objv[4], 0);
}

/*
 *----------------------------------------------------------------------
 *
 * StyleCheckBoxOp --
 *
 *	  .t style checkbox "styleName" -background blue
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleCheckBoxOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewStyle *stylePtr;

    stylePtr = CreateStyle(interp, tvPtr, STYLE_CHECKBOX, 
	Tcl_GetString(objv[3]), objc - 4, objv + 4, 1);
    if (stylePtr == NULL) {
	return TCL_ERROR;
    }
    stylePtr->refCount = 0;
    stylePtr->flags |= STYLE_USER;
    Blt_TreeViewUpdateStyleGCs(tvPtr, stylePtr);
    Tcl_SetObjResult(interp, objv[3]);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * StyleComboBoxOp --
 *
 *	  .t style combobox "styleName" -background blue
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleComboBoxOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewStyle *stylePtr;

    stylePtr = CreateStyle(interp, tvPtr, STYLE_COMBOBOX, 
	Tcl_GetString(objv[3]), objc - 4, objv + 4, 1);
    if (stylePtr == NULL) {
	return TCL_ERROR;
    }
    stylePtr->refCount = 0;
    stylePtr->flags |= STYLE_USER;
    Blt_TreeViewUpdateStyleGCs(tvPtr, stylePtr);
    Tcl_SetObjResult(interp, objv[3]);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StyleConfigureOp --
 *
 * 	This procedure is called to process a list of configuration
 *	options database, in order to reconfigure a style.
 *
 *	  .t style configure "styleName" option value
 *
 * Results:
 *	A standard Tcl result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for stylePtr; old resources get freed, if there
 *	were any.  
 *
 *----------------------------------------------------------------------
 */
static int
StyleConfigureOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewStyle *stylePtr;

    if (Blt_TreeViewGetStyleMake(interp, tvPtr, Tcl_GetString(objv[3]), &stylePtr,
        NULL, NULL, NULL) != TCL_OK) {
        return TCL_ERROR;
    }
    stylePtr->refCount--;

    Blt_TreeViewOptsInit(tvPtr);
    if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, tvPtr->tkwin, 
	    stylePtr->classPtr->specsPtr, (char *)stylePtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 5) {
	return Blt_ConfigureInfoFromObj(interp, tvPtr->tkwin, 
		stylePtr->classPtr->specsPtr, (char *)stylePtr, objv[4], 0);
    }
    if (Blt_ConfigureWidgetFromObj(interp, tvPtr->tkwin, 
	stylePtr->classPtr->specsPtr, objc - 4, objv + 4, (char *)stylePtr, 
	BLT_CONFIG_OBJV_ONLY, NULL) != TCL_OK) {	
	return TCL_ERROR;
    }
    if (stylePtr->tile != NULL) {
        Blt_SetTileChangedProc(stylePtr->tile, Blt_TreeViewTileChangedProc, tvPtr);
    }
    if (stylePtr->fillTile != NULL) {
        Blt_SetTileChangedProc(stylePtr->fillTile, Blt_TreeViewTileChangedProc, tvPtr);
    }
    (*stylePtr->classPtr->configProc)(tvPtr, stylePtr);
    Blt_TreeViewMakeStyleDirty(tvPtr);
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StyleForgetOp --
 *
 * 	Eliminates one or more style names whose
 * 	reference count is zero (i.e. no one else is using it).
 *
 *	  .t style forget "styleName"...
 *
 * Results:
 *	A standard Tcl result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 *----------------------------------------------------------------------
 */
static int
StyleForgetOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;

    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewStyle *stylePtr;
    int i;

    for (i = 3; i < objc; i++) {
	stylePtr = GetStyle(interp, tvPtr, Tcl_GetString(objv[i]));
	if (stylePtr == NULL) {
	    return TCL_ERROR;
	}
	if (stylePtr->refCount > 1 || stylePtr == tvPtr->stylePtr) {
	    continue;
	}
	if (stylePtr->hashPtr != NULL) {
	    Blt_DeleteHashEntry(&tvPtr->styleTable, stylePtr->hashPtr);
	    stylePtr->hashPtr = NULL;
	}
	stylePtr->refCount--;
	stylePtr->flags &= ~STYLE_USER;
        Blt_TreeViewFreeStyle(tvPtr, stylePtr);
    }
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

static int
StyleUseOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;

    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewStyle *stylePtr;

    stylePtr = GetStyle(interp, tvPtr, Tcl_GetString(objv[3]));
    if (stylePtr == NULL) {
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(stylePtr->refCount-1));
    return TCL_OK;
}


static int StyleLookup(char *string) {
    if (!strcmp(string, "combobox")) {
        return STYLE_COMBOBOX;
    }
    if (!strcmp(string, "textbox")) {
        return STYLE_TEXTBOX;
    }
    if (!strcmp(string, "barbox")) {
        return STYLE_BARBOX;
    }
    if (!strcmp(string, "windowbox")) {
        return STYLE_WINDOWBOX;
    }
    if (!strcmp(string, "checkbox")) {
        return STYLE_CHECKBOX;
    }
    return -1;
}

/*
 *----------------------------------------------------------------------
 *
 * StyleTypeOp --
 *
 * 	Return the type of a style.
 *
 *	  .t style type "styleName"...
 *
 * Results:
 *	A standard Tcl result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 *----------------------------------------------------------------------
 */
static int
StyleTypeOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;

    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewStyle *stylePtr, *newStylePtr;
    TreeViewAllStyles all;
    const char *cp;
    char *newType;
    int nType;
    
    if (objc == 3) {
        cp = "textbox combobox checkbox barbox windowbox";
        Tcl_AppendResult(interp, cp, 0);
        return TCL_OK;
    }
    stylePtr = GetStyle(interp, tvPtr, Tcl_GetString(objv[3]));
    if (stylePtr == NULL) {
        return TCL_ERROR;
    }
    if (stylePtr->flags & STYLE_CHECKBOX) {
        cp = "checkbox";
    } else if (stylePtr->flags & STYLE_COMBOBOX) {
        cp = "combobox";
    } else if (stylePtr->flags & STYLE_WINDOWBOX) {
        cp = "windowbox";
    } else if (stylePtr->flags & STYLE_BARBOX) {
        cp = "barbox";
    } else  {
        cp = "textbox";
    }

    if (objc == 4) {
        Tcl_AppendResult(interp, cp, 0);
        return TCL_OK;
    }
    if (!strcmp(stylePtr->name, "text")) {
        Tcl_AppendResult(interp, "can not change type of style \"text\"", 0);
        return TCL_ERROR;
    }
    newType = Tcl_GetString(objv[4]);
    nType = StyleLookup( newType );
    if (nType<0) {
        Tcl_AppendResult(interp, "unknown type: ", newType, 0);
        return TCL_ERROR;
    }
    /* if (!strcmp(newType, cp)) {
        return TCL_OK;
    }*/
    /* Change the style */
    newStylePtr = CreateStyle(interp, tvPtr, nType, "__%%StyleTypeSet%%_", 0, 0, 0);
    if (newStylePtr == NULL) {
        return TCL_ERROR;
    }
    newStylePtr->flags |= STYLE_USER;
    memcpy(&all, stylePtr, sizeof(all));
    memcpy(stylePtr, newStylePtr, sizeof(all));
    memcpy(newStylePtr, &all, sizeof(all));
    newStylePtr->refCount = stylePtr->refCount;
    stylePtr->refCount = all.tb.refCount ;
    newStylePtr->hashPtr = stylePtr->hashPtr;
    stylePtr->hashPtr = all.tb.hashPtr ;
    newStylePtr->name = stylePtr->name;
    stylePtr->name = all.tb.name ;
    
    newStylePtr->flags &= ~STYLE_USER;
    Blt_TreeViewFreeStyle(tvPtr, newStylePtr);
    Blt_TreeViewUpdateStyleGCs(tvPtr, stylePtr);
    tvPtr->flags |= (TV_DIRTY | TV_LAYOUT | TV_SCROLL | TV_UPDATE);
    Blt_TreeViewMakeStyleDirty(tvPtr);
    Blt_TreeViewEventuallyRedraw(tvPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StyleHighlightOp --
 *
 * 	Turns on/off highlighting for a particular style.
 *
 *	  .t style highlight styleName ?on|off?
 *
 * Results:
 *	A standard Tcl result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleHighlightOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;

    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewStyle *stylePtr;
    int bool, oldBool;

    stylePtr = GetStyle(interp, tvPtr, Tcl_GetString(objv[3]));
    if (stylePtr == NULL) {
	return TCL_ERROR;
    }
    oldBool = ((stylePtr->flags & STYLE_HIGHLIGHT) != 0);
    if (objc<=4) {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(oldBool));
        return TCL_OK;
    }
    if (Tcl_GetBooleanFromObj(interp, objv[4], &bool) != TCL_OK) {
	return TCL_ERROR;
    }
    if (oldBool != bool) {
	if (bool) {
	    stylePtr->flags |= STYLE_HIGHLIGHT;
	} else {
	    stylePtr->flags &= ~STYLE_HIGHLIGHT;
	}
	Blt_TreeViewEventuallyRedraw(tvPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StyleNamesOp --
 *
 * 	Lists the names of all the current styles in the treeview widget.
 *
 *	  .t style names
 *
 * Results:
 *	Always TCL_OK.
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleNamesOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;

    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;	/* Not used. */
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    Tcl_Obj *listObjPtr, *objPtr;
    TreeViewStyle *stylePtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (hPtr = Blt_FirstHashEntry(&tvPtr->styleTable, &cursor); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&cursor)) {
	stylePtr = Blt_GetHashValue(hPtr);
	objPtr = Tcl_NewStringObj(stylePtr->name, -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static int
StyleSlavesOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;

    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    char *name, *window = NULL;
    Tcl_DString dStr;
    TreeViewColumn *columnPtr = NULL;
    TreeViewEntry *entryPtr = NULL;
    TreeViewStyle *stylePtr = NULL;
    int visible;
    WindowCell  *wcPtr;
    int sub = 0, chkVis = 0;

    if (!(objc%2))  {
        goto argerr;
    }
    while (objc>3) {
        char *string;

        string = Tcl_GetString(objv[3]);
        if (string[0] != '-') {
argerr:
            Tcl_AppendResult(interp, "expected -col, -id, -style, -visible, or -info", 0);
            return TCL_ERROR;
        }
        if ((strcmp(string, "-col") == 0)) {
            if (Blt_TreeViewGetColumn(interp, tvPtr, objv[4], &columnPtr) 
            != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((strcmp(string, "-info") == 0)) {
            
            window = Tcl_GetString(objv[4]);
            
        } else if ((strcmp(string, "-id") == 0)) {
            if (Blt_TreeViewGetEntry(tvPtr, objv[4], &entryPtr) 
            != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((strcmp(string, "-visible") == 0)) {
            if (Tcl_GetBooleanFromObj(interp, objv[4], &visible) != TCL_OK) {
                return TCL_ERROR;
            }
            chkVis = 1;
        } else if ((strcmp(string, "-style") == 0)) {
            stylePtr = GetStyle(interp, tvPtr, Tcl_GetString(objv[4]));
            if (stylePtr == NULL) {
                return TCL_ERROR;
            }
        } else break;
        
        objc -= 2;
        objv += 2;
        sub++;
    }
    if (objc != 3)  {
        goto argerr;
    }
    
    
    Tcl_DStringInit(&dStr);
    for (hPtr = Blt_FirstHashEntry(&tvPtr->winTable, &cursor); hPtr != NULL;
        hPtr = Blt_NextHashEntry(&cursor)) {
        name = (char*)Blt_GetHashKey(&tvPtr->winTable, hPtr);
        if (sub) {
            wcPtr = (WindowCell*)Blt_GetHashValue(hPtr);
            if (wcPtr == NULL) continue;
            if (columnPtr && columnPtr != wcPtr->columnPtr) continue;
            if (entryPtr && entryPtr != wcPtr->entryPtr) continue;
            if (stylePtr && stylePtr != (TreeViewStyle*)wcPtr->stylePtr) continue;
            if (chkVis) {
                if (wcPtr->flags&TV_WINDOW_DRAW) {
                    if (!visible) continue;
                } else {
                    if (visible) continue;
                }
            }
            if (window) {
                if (wcPtr->tkwin == NULL) continue;
                if (strcmp(window,Tk_PathName(wcPtr->tkwin))) continue;
                Tcl_DStringAppendElement(&dStr, "-style");
                Tcl_DStringAppendElement(&dStr, wcPtr->stylePtr->name);
                Tcl_DStringAppendElement(&dStr, "-col");
                Tcl_DStringAppendElement(&dStr, wcPtr->columnPtr->key);
                Tcl_DStringAppendElement(&dStr, "-id");
                Tcl_DStringAppendElement(&dStr, Blt_Itoa(Blt_TreeNodeId(wcPtr->entryPtr->node)));
                Tcl_DStringAppendElement(&dStr, "-visible");
                Tcl_DStringAppendElement(&dStr, (wcPtr->flags&TV_WINDOW_DRAW)?"True":"False");
                Tcl_DStringResult(interp, &dStr);
                return TCL_OK;
            }
        }
        if (name != NULL) {
            Tcl_DStringAppendElement(&dStr, name);
        }
    }
    Tcl_DStringResult(interp, &dStr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StyleSetOp --
 *
 * 	Sets a style for a given key for all the ids given.
 *
 *	  .t style set styleName column node...
 *
 * Results:
 *	A standard Tcl result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 *----------------------------------------------------------------------
 */
static int
StyleSetOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;

    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewColumn *columnPtr;
    TreeViewEntry *entryPtr;
    TreeViewStyle *stylePtr = NULL, *oldStylePtr;
    TreeViewTagInfo info = {0};
    char *string;
    int i, count;

    count = 0;
    if (Blt_TreeViewGetColumn(interp, tvPtr, objv[4], &columnPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[3]);
    if (string[0]) {
        if (Blt_TreeViewGetStyleMake(interp, tvPtr, string, &stylePtr, columnPtr,
            NULL, NULL) != TCL_OK) {
            return TCL_ERROR;
        }
        stylePtr->refCount--;
    }
    if (stylePtr) {
        stylePtr->flags |= STYLE_LAYOUT;
    }
    for (i = 5; i < objc; i++) {
	if (Blt_TreeViewFindTaggedEntries(tvPtr, objv[i], &info) != TCL_OK) {
	    return TCL_ERROR;
	}
	for (entryPtr = Blt_TreeViewFirstTaggedEntry(&info); entryPtr != NULL; 
	     entryPtr = Blt_TreeViewNextTaggedEntry(&info)) {
	    register TreeViewValue *valuePtr;

             if (columnPtr == &tvPtr->treeColumn) {
                 if (stylePtr == entryPtr->realStylePtr) continue;
                 if (stylePtr) stylePtr->refCount++;
                 oldStylePtr = entryPtr->realStylePtr;
                 entryPtr->realStylePtr = stylePtr;
                 if (stylePtr) stylePtr->refCount++;
                 if (oldStylePtr != NULL) {
                     Blt_TreeViewFreeStyle(tvPtr, oldStylePtr);
                 }
                 count++;
                 continue;
             }
             
             for (valuePtr = entryPtr->values; valuePtr != NULL; 
		 valuePtr = valuePtr->nextPtr) {
		if (valuePtr->columnPtr == columnPtr) {
                    if (stylePtr == valuePtr->stylePtr) break;
		    if (stylePtr) stylePtr->refCount++;
		    oldStylePtr = valuePtr->stylePtr;
		    valuePtr->stylePtr = stylePtr;
                    if (stylePtr) stylePtr->refCount++;
		    if (oldStylePtr != NULL) {
			Blt_TreeViewFreeStyle(tvPtr, oldStylePtr);
		    }
		    count++;
		    break;
		}
	    }
	}
        Blt_TreeViewDoneTaggedEntries(&info);
    }
    tvPtr->flags |= (TV_DIRTY | TV_LAYOUT | TV_SCROLL | TV_UPDATE);
    Blt_TreeViewMakeStyleDirty(tvPtr);
    Blt_TreeViewEventuallyRedraw(tvPtr);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(count));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StyleFindOp --
 *
 * 	Find all entries with the given style.
 *      If a column is given, find entries with the given style in that column.
 *
 *	  .t style find styleName ?tagorid? ?column?
 *
 * Results:
 *	A standard Tcl result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 *----------------------------------------------------------------------
 */
static int
StyleFindOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;

    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewEntry *entryPtr;
    TreeViewStyle *stylePtr;
    TreeViewTagInfo info = {0};
    TreeViewColumn *columnPtr = NULL;
    Tcl_Obj *listObjPtr, *objPtr;

    if (tvPtr->styleCmd == NULL || strcmp(tvPtr->styleCmd, "%W style create textbox %V")) {
        stylePtr = GetStyle(interp, tvPtr, Tcl_GetString(objv[3]));
        if (stylePtr == NULL) {
            return TCL_ERROR;
        }
    } else {
        stylePtr = GetStyle(NULL, tvPtr, Tcl_GetString(objv[3]));
        if (stylePtr == NULL) {
            return TCL_OK;
        }
    }
    if (objc>5) {
        if (Blt_TreeViewGetColumn(interp, tvPtr, objv[5], &columnPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (objc>4) {
        if (Blt_TreeViewFindTaggedEntries(tvPtr, objv[4], &info) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        objPtr = Tcl_NewStringObj("all", -1);
        if (Blt_TreeViewFindTaggedEntries(tvPtr, objPtr, &info) != TCL_OK) {
            Tcl_DecrRefCount(objPtr);
            return TCL_ERROR;
        }
        Tcl_DecrRefCount(objPtr);
    }
        
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (entryPtr = Blt_TreeViewFirstTaggedEntry(&info); entryPtr != NULL; 
        entryPtr = Blt_TreeViewNextTaggedEntry(&info)) {
        register TreeViewValue *valuePtr;
	    
        if (columnPtr == NULL || columnPtr == &tvPtr->treeColumn) {
            if (entryPtr->realStylePtr == stylePtr) {
                objPtr = Tcl_NewIntObj(Blt_TreeNodeId(entryPtr->node));
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                continue;
            }
        }

        for (valuePtr = entryPtr->values; valuePtr != NULL; 
            valuePtr = valuePtr->nextPtr) {
            if (columnPtr && valuePtr->columnPtr != columnPtr) continue;
            if (valuePtr->stylePtr != stylePtr) continue;
            objPtr = Tcl_NewIntObj(Blt_TreeNodeId(entryPtr->node));
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            break;
        }
    }
    Blt_TreeViewDoneTaggedEntries(&info);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StyleTextBoxOp --
 *
 *	  .t style textbox "styleName" -background blue
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleTextBoxOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewStyle *stylePtr;

    stylePtr = CreateStyle(interp, tvPtr, STYLE_TEXTBOX, 
	Tcl_GetString(objv[3]), objc - 4, objv + 4, 1);
    if (stylePtr == NULL) {
	return TCL_ERROR;
    }
    stylePtr->refCount = 0;
    stylePtr->flags |= STYLE_USER;
    Blt_TreeViewUpdateStyleGCs(tvPtr, stylePtr);
    Tcl_SetObjResult(interp, objv[3]);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StyleWindowBoxOp --
 *
 *	  .t style window "styleName"
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleWindowBoxOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewStyle *stylePtr;

    stylePtr = CreateStyle(interp, tvPtr, STYLE_WINDOWBOX, 
	Tcl_GetString(objv[3]), objc - 4, objv + 4, 1);
    if (stylePtr == NULL) {
	return TCL_ERROR;
    }
    stylePtr->refCount = 0;
    stylePtr->flags |= STYLE_USER;
    Blt_TreeViewUpdateStyleGCs(tvPtr, stylePtr);
    Tcl_SetObjResult(interp, objv[3]);
    return TCL_OK;
}
/*
 *----------------------------------------------------------------------
 *
 * StyleBarBoxOp --
 *
 *	  .t style textbox "styleName" -background blue
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleBarBoxOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    TreeViewStyle *stylePtr;

    stylePtr = CreateStyle(interp, tvPtr, STYLE_BARBOX, 
	Tcl_GetString(objv[3]), objc - 4, objv + 4, 1);
    if (stylePtr == NULL) {
	return TCL_ERROR;
    }
    stylePtr->refCount = 0;
    stylePtr->flags |= STYLE_USER;
    Blt_TreeViewUpdateStyleGCs(tvPtr, stylePtr);
    Tcl_SetObjResult(interp, objv[3]);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * StyleGetOp --
 *
 * 	Get style set for each given key for the id given.
 *
 *	  .t style get column tagorid
 *
 * Results:
 *	A standard Tcl result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 *----------------------------------------------------------------------
 */
static int
StyleGetOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewColumn *columnPtr;
    TreeViewEntry *entryPtr;
    TreeViewTagInfo info = {0};
    Tcl_Obj *listObjPtr, *objPtr;

    if (Blt_TreeViewGetColumn(interp, tvPtr, objv[3], &columnPtr) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Blt_TreeViewFindTaggedEntries(tvPtr, objv[4], &info) != TCL_OK) {
        return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (entryPtr = Blt_TreeViewFirstTaggedEntry(&info); entryPtr != NULL; 
        entryPtr = Blt_TreeViewNextTaggedEntry(&info)) {
        register TreeViewValue *valuePtr;

        if (columnPtr == &tvPtr->treeColumn) {
            if (entryPtr->realStylePtr != NULL) {
                objPtr = Tcl_NewStringObj(entryPtr->realStylePtr->name, -1);
            } else {
                objPtr = Tcl_NewStringObj("",0);
            }
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            continue;
        }
        
        for (valuePtr = entryPtr->values; valuePtr != NULL; 
            valuePtr = valuePtr->nextPtr) {
            if (valuePtr->columnPtr == columnPtr) {
                if (valuePtr->stylePtr != NULL) {
                    objPtr = Tcl_NewStringObj(valuePtr->stylePtr->name, -1);
                } else {
                    objPtr = Tcl_NewStringObj("",0);
                }
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                break;
            }
        }
    }
    Blt_TreeViewDoneTaggedEntries(&info);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

#if 0
/*
 *----------------------------------------------------------------------
 *
 * StylePriorityOp --
 *
 * 	Get style with highest priority for given key for the id given.
 *
 *	  .t style priority column tagorid
 *
 * Results:
 *	A standard Tcl result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 *----------------------------------------------------------------------
 */
static int
StylePriorityOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    TreeViewColumn *columnPtr;
    TreeViewEntry *entryPtr;
    TreeViewValue *valuePtr;
    TreeViewStyle *s[4];
    int i, h = -1, p = -1;

    s[3] = tvPtr->stylePtr;
    
    if (Blt_TreeViewGetColumn(interp, tvPtr, objv[3], &columnPtr) != TCL_OK) {
        return TCL_ERROR;
    }

    if (Blt_TreeViewGetEntry(tvPtr, objv[4], &entryPtr) != TCL_OK) {
        return TCL_ERROR;
    }

    if (columnPtr == &tvPtr->treeColumn) {
        s[0] = entryPtr->realStylePtr;
    } else {
        s[0] = NULL;
        for (valuePtr = entryPtr->values; valuePtr != NULL; 
            valuePtr = valuePtr->nextPtr) {
            if (valuePtr->columnPtr == columnPtr) {
                s[0] = valuePtr->stylePtr;
                break;
            }
        }
    }
    s[1] = entryPtr->stylePtr;
    s[2] = columnPtr->stylePtr;
    for (i=0; i<4; i++) {
        if (s[i]==0) continue;
        if (h<0) { h = i; }
        if (s[i]->priority > p) {
            h = i;
            p = s[i]->priority;
        }
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(s[h]->name, -1));
    return TCL_OK;
}
#endif
/*
 *----------------------------------------------------------------------
 *
 * StyleCreateOp --
 *
 *	  .t style create "styleType" "styleName" -background blue
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StyleCreateOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *CONST *objv;
{
    char *string;
    
    string = Tcl_GetString(objv[3]);
    if (!strcmp(string, "combobox")) {
        return StyleComboBoxOp(tvPtr, interp, objc-1, objv+1);
    }
    if (!strcmp(string, "textbox")) {
        return StyleTextBoxOp(tvPtr, interp, objc-1, objv+1);
    }
    if (!strcmp(string, "barbox")) {
        return StyleBarBoxOp(tvPtr, interp, objc-1, objv+1);
    }
    if (!strcmp(string, "windowbox")) {
        return StyleWindowBoxOp(tvPtr, interp, objc-1, objv+1);
    }
    if (!strcmp(string, "checkbox")) {
        return StyleCheckBoxOp(tvPtr, interp, objc-1, objv+1);
    }
    Tcl_AppendResult(interp, "bad style type \"", string, "\", should be one of: textbox, barbox, checkbox, combobox, or windowbox", 0);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * StyleOp --
 *
 *	.t style activate $node $column
 *	.t style activate 
 *	.t style cget "highlight" -foreground
 *	.t style configure "highlight" -fg blue -bg green
 *	.t style checkbox "highlight"
 *	.t style highlight "highlight" on|off
 *	.t style combobox "highlight"
 *	.t style text "highlight"
 *	.t style forget "highlight"
 *	.t style get "mtime" $node
 *	.t style names
 *	.t style set "highlight" "mtime" all
 *	.t style unset "mtime" all
 *
 *---------------------------------------------------------------------- 
 */
static Blt_OpSpec styleOps[] = {
    {"activate", 1, (Blt_Op)StyleActivateOp, 3, 5,"?entry column?",},
    /*{"barbox", 2, (Blt_Op)StyleBarBoxOp, 4, 0, "styleName options...",},*/
    {"cget", 2, (Blt_Op)StyleCgetOp, 5, 5, "styleName option",},
    /*{"checkbox", 2, (Blt_Op)StyleCheckBoxOp, 4, 0, "styleName options...",},
    {"combobox", 3, (Blt_Op)StyleComboBoxOp, 4, 0, "styleName options...",},*/
    {"configure", 3, (Blt_Op)StyleConfigureOp, 4, 0, "styleName options...",},
    {"create", 2, (Blt_Op)StyleCreateOp, 5, 0, "type styleName options...",},
    {"find", 2, (Blt_Op)StyleFindOp, 4, 6, "styleName ?tagOrId? ?column?",},
    {"forget", 2, (Blt_Op)StyleForgetOp, 3, 0, "styleName...",},
    {"get", 1, (Blt_Op)StyleGetOp, 5, 5, "column tagOrId",},
    {"highlight", 1, (Blt_Op)StyleHighlightOp, 4, 5, "styleName ?boolean?",},
    {"names", 1, (Blt_Op)StyleNamesOp, 3, 3, "",}, 
    /*{"priority", 1, (Blt_Op)StylePriorityOp, 5, 5, "column tagOrId",}, */
    {"set", 2, (Blt_Op)StyleSetOp, 6, 0, "styleName column tagOrId ...",},
    {"slaves", 2, (Blt_Op)StyleSlavesOp, 3, 0, "?-col col? ?-entry id? ?-style name? ?-visible bool? ?-info win?",},
    /*{"textbox", 2, (Blt_Op)StyleTextBoxOp, 4, 0, "styleName options...",}, */
    {"type", 2, (Blt_Op)StyleTypeOp, 3, 5, "?styleName? ?newType?",},
    {"use", 2, (Blt_Op)StyleUseOp, 4, 4, "styleName",},
    /*{"windowbox", 2, (Blt_Op)StyleWindowBoxOp, 4, 0, "styleName options...",},*/
};

static int nStyleOps = sizeof(styleOps) / sizeof(Blt_OpSpec);

int
Blt_TreeViewStyleOp(tvPtr, interp, objc, objv)
    TreeView *tvPtr;
    Tcl_Interp *interp;
    int objc;
    Tcl_Obj *CONST *objv;
{
    Blt_Op proc;
    int result;

    proc = Blt_GetOpFromObj(interp, nStyleOps, styleOps, BLT_OP_ARG2, objc, 
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc)(tvPtr, interp, objc, objv);
    return result;
}
#endif /* NO_TREEVIEW */
