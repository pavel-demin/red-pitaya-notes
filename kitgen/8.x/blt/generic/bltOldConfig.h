/* Old config headers. */

#include <tk.h>

EXTERN int		Blt_ConfigureInfo _ANSI_ARGS_((Tcl_Interp * interp, 
				Tk_Window tkwin, Tk_ConfigSpec * specs, 
				char * widgRec, CONST char * argvName, 
				int flags));
EXTERN int		Blt_ConfigureValue _ANSI_ARGS_((Tcl_Interp * interp, 
				Tk_Window tkwin, Tk_ConfigSpec * specs, 
				char * widgRec, CONST char * argvName, 
				int flags));
EXTERN int		Blt_ConfigureWidget _ANSI_ARGS_((Tcl_Interp * interp, 
				Tk_Window tkwin, Tk_ConfigSpec * specs, 
				int argc, CONST char ** argv, 
				char * widgRec, int flags));

EXTERN void		Blt_FreeOptions _ANSI_ARGS_((Tk_ConfigSpec * specs, 
				char * widgRec, Display * display, 
				int needFlags));

EXTERN Tk_ConfigSpec *	Blt_GetCachedSpecs _ANSI_ARGS_((Tcl_Interp *interp,
			    const Tk_ConfigSpec *staticSpecs));

#define Tk_ConfigureInfo(interp, tkwin, specs, widgRec, argvName, flags) Blt_ConfigureInfo(interp, tkwin, specs, widgRec, argvName, flags)

#define Tk_ConfigureValue(interp, tkwin, specs, widgRec, argvName, flags) Blt_ConfigureValue(interp, tkwin, specs, widgRec, argvName, flags)

#define Tk_FreeOptions(specs, widgRec, display, needFlags) Blt_FreeOptions(specs, widgRec, display, needFlags)

#define Tk_ConfigureWidget(interp, tkwin, origSpecs, argc, argv, widgRec, flags) Blt_ConfigureWidget(interp, tkwin, origSpecs, argc, argv, widgRec, flags)


