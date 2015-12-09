
# ======================================================================
#
# treeview.tcl
#
# ----------------------------------------------------------------------
# Bindings for the BLT treeview widget
# ----------------------------------------------------------------------
#
#   AUTHOR:  George Howlett
#            Bell Labs Innovations for Lucent Technologies
#            gah@lucent.com
#            http://www.tcltk.com/blt
#
#      RCS:  $Id: treeview.tcl,v 1.24 2010/05/06 22:26:17 pcmacdon Exp $
#
# ----------------------------------------------------------------------
# Copyright (c) 1998  Lucent Technologies, Inc.
# ----------------------------------------------------------------------
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted,
# provided that the above copyright notice appear in all copies and that
# both that the copyright notice and warranty disclaimer appear in
# supporting documentation, and that the names of Lucent Technologies
# any of their entities not be used in advertising or publicity
# pertaining to distribution of the software without specific, written
# prior permission.
#
# Lucent Technologies disclaims all warranties with regard to this
# software, including all implied warranties of merchantability and
# fitness.  In no event shall Lucent be liable for any special, indirect
# or consequential damages or any damages whatsoever resulting from loss
# of use, data or profits, whether in an action of contract, negligence
# or other tortuous action, arising out of or in connection with the use
# or performance of this software.
#
# ======================================================================

namespace eval ::blt::tv {
    variable afterId ""
    variable scroll 0
    variable column ""
    variable space   off
    variable x 0
    variable y 0
    variable script [info script]
    variable dirname [file dirname [info script]]
    if {[info exists ::tcl_warn(level)] && $::tcl_warn(level)} {
       source [file join $dirname tvutil.tcl]
    } else {
       set ::auto_index(::blt::tv::TableWid) [list source [file join $dirname tvutil.tcl]]
       set ::auto_index(::blt::tv::TableLoad) [list source [file join $dirname tvutil.tcl]]
       set ::auto_index(::blt::tv::TreeLoad) [list source [file join $dirname tvutil.tcl]]
       set ::auto_index(::blt::tv::TreeDump) [list source [file join $dirname tvutil.tcl]]
       set ::auto_index(::blt::tv::TreeFill) [list source [file join $dirname tvutil.tcl]]
}
    }


image create photo blt::tv::normalCloseFolder -format gif -data {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM8WBrM+rAEQWmIb5KxiWjNInCkV32AJHRlGQBgDA7vdN4vUa8tC78qlrCWmvRKsJTquHkp
    ZTKAsiCtWq0JADs=
}
image create photo blt::tv::normalOpenFolder -format gif -data {
    R0lGODlhEAANAMIAAAAAAH9/f///////AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM1WBrM+rAEMigJ8c3Kb3OSII6kGABhp1JnaK1VGwjwKwtvHqNzzd263M3H4n2OH1QBwGw6
    nQkAOw==
}
image create photo blt::tv::activeCloseFolder -format gif -data {
    R0lGODlhEAANAMIAAAAAAH9/f/////+/AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM8WBrM+rAEQWmIb5KxiWjNInCkV32AJHRlGQBgDA7vdN4vUa8tC78qlrCWmvRKsJTquHkp
    ZTKAsiCtWq0JADs=
}
image create photo blt::tv::activeOpenFolder -format gif -data {
    R0lGODlhEAANAMIAAAAAAH9/f/////+/AL+/vwAA/wAAAAAAACH5BAEAAAUALAAAAAAQAA0A
    AAM1WBrM+rAEMigJ8c3Kb3OSII6kGABhp1JnaK1VGwjwKwtvHqNzzd263M3H4n2OH1QBwGw6
    nQkAOw==
}

image create photo blt::tv::normalFile -format gif -data {
    R0lGODlhFAAQAMIAAP///wAAALq2VYKCgtvb2wAAAAAAAAAAACH5BAEAAAAA
    LAAAAAAUABAAAAM7CLrcriHK8BxlsWIgOqCXFkKkyHnT2KjYUFFdjLoPuwS0
    dcrCl6+i34Y3ewWNL+EtlVIuBtCodEBgJAAAOw==
}

image create photo blt::tv::openFile -format gif -data {
R0lGODlhEQAQAIUAAPwCBFxaXNze3Ly2rJyanPz+/Ozq7GxqbPz6/PT29HRy
dMzOzDQyNExGBIyKhERCRPz+hPz+BPz29MTCBPzy7PTq3NS+rPz+xPz27Pzu
5PTi1My2pPTm1PTezPTm3PTaxMyynMyqjPTizOzWvOzGrOTi3OzOtMSehNTK
xNTCtAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACH5BAEAAAAALAAAAAARABAAAAaj
QIBQGCgajcMkICBoNgcBQkBJFBSuBcMBIaUysYWEYsFlKBtWsENxeJgBDTik
UaBfE4KBw9yYNyINfXYFEnlvfxMRiYMSFBUWfA0TFxAXE4EFGBkVGhsMfRER
EAUQoXObHB2ecJKUloEJHB4aHyCHirgNGRmzHx8hfH6Agh4iHyMkwEJxghkN
HCXHJiQnb0MNCwsoKRYbICEh1UoBDOXm5wx2QQA7
}

image create photo blt::tv::empty

image create photo blt::tv::downarrow -format gif -data {
R0lGODlhEQAJAPABAAAAAP///yH5BAEAAAEALAAAAAARAAkAAAJXTJgwYcKE
CRMmTJgQIECAAAEiTJgwIUCAAAEiTJgwYUKAAAEiTJgwYcKEAAEiTJgwYcKE
CQEiTJgwYcKECRMiTJgwYcKECRMmTJgwYcKECRMmTJgwYcIUADs=
}

image create photo blt::tv::rightarrow -format gif -data {
R0lGODlhEAAQAIAAAPwCBAQCBCH5BAEAAAAALAAAAAAQABAAAAIdhI+pyxCt
woNHTmpvy3rxnnwQh1mUI52o6rCu6hcAIf5oQ3JlYXRlZCBieSBCTVBUb0dJ
RiBQcm8gdmVyc2lvbiAyLjUNCqkgRGV2ZWxDb3IgMTk5NywxOTk4LiBBbGwg
cmlnaHRzIHJlc2VydmVkLg0KaHR0cDovL3d3dy5kZXZlbGNvci5jb20AOw==
}

image create photo blt::tv::ball -format gif -data {
R0lGODlhEQAJAPABAAAAAP///yH5BAEAAAEALAAAAAARAAkAAAJXTJgwYcKE
CRMmTJgwYcKECRMmTJgwYcKAABMmTJgwYcKAAAEmTJgwYcKEAAEiTJgwYcKE
CQEiTJgwYcKECRMmTJgwYcKECRMmTJgwYcKECRMmTJgwYcIUADs=
}

# Seems to be a memory leak in @cursors and another in binds.
if { $tcl_platform(platform) == "windows" } {
    if { $tk_version >= 8.3  && ![string match /zvfs* $blt_library]} {
	set cursor "@[file join $blt_library treeview.cur]"
    } else {
	set cursor "size_we"
    }
    option add *${className}.ResizeCursor [list $cursor]
} else {
    option add *${className}.ResizeCursor \
	"@$blt_library/treeview.xbm $blt_library/treeview_m.xbm black white"
}

# ----------------------------------------------------------------------
#
# Initialize --
#
#	Invoked by internally by Treeview_Init routine.  Initializes
#	the default bindings for the treeview widget entries.  These
#	are local to the widget, so they can't be set through the
#	widget's class bind tags.
#
#       TODO: get rid of most of this in favor of class binds.
#
# ----------------------------------------------------------------------
variable ::blt::tv::oldedit 0
proc ::blt::tv::Initialize { w } {
    #
    # Active entry bindings
    #
    variable oldedit
    $w bind Entry <Enter> {
	%W entry activate current
    }
    $w bind Entry <Leave> {
	%W entry activate ""
    }

    #
    # Button bindings
    #
    $w button bind all <ButtonRelease-1> {
	blt::tv::Toggle %W current
    }
    $w button bind all <Enter> {
	%W button activate current
    }
    $w button bind all <Leave> {
	%W button activate ""
    }

    #
    # ButtonPress-1
    #
    #	Performs the following operations:
    #
    #	1. Clears the previous selection.
    #	2. Selects the current entry.
    #	3. Sets the focus to this entry.
    #	4. Scrolls the entry into view.
    #	5. Sets the selection anchor to this entry, just in case
    #	   this is "multiple" mode.
    #

    $w bind Entry <ButtonPress-1> {
	blt::tv::SetSelectionSetAnchor %W %x %y
	set blt::tv::scroll 1
    }

    #$w bind Entry <Double-ButtonPress-1> { %W toggle current }

    #
    # B1-Motion
    #
    #	For "multiple" mode only.  Saves the current location of the
    #	pointer for auto-scrolling.  Resets the selection mark.
    #
    $w bind Entry <B1-Motion> {
	set blt::tv::x %x
	set blt::tv::y %y
	set index [%W nearest %x %y]
	if { [%W cget -selectmode] == "multiple" } {
	    %W selection mark $index
	} elseif { [%W cget -selectmode] != "none" } {
	    blt::tv::SetSelectionAnchor %W $index
	}
    }

    #
    # ButtonRelease-1
    #
    #	For "multiple" mode only.
    #
    $w bind Entry <ButtonRelease-1> {
	if { [%W cget -selectmode] == "multiple" } {
	    %W selection anchor current
	}
	after cancel $blt::tv::afterId
	set blt::tv::scroll 0
    }

    #
    # Shift-ButtonPress-1
    #
    #	For "multiple" mode only.
    #

    $w bind Entry <Shift-ButtonPress-1> {
        blt::tv::SetSelectionExtendAnchor %W %x %y
    }
    $w bind Entry <Shift-Double-ButtonPress-1> {
	# do nothing
    }
    $w bind Entry <Shift-B1-Motion> {
	# do nothing
    }
    $w bind Entry <Shift-ButtonRelease-1> {
	after cancel $blt::tv::afterId
	set blt::tv::scroll 0
    }

    #
    # Control-ButtonPress-1
    #
    #	For "multiple" mode only.
    #
    $w bind Entry <Control-ButtonPress-1> {
        blt::tv::SetSelectionAdd %W %x %y
    }
    $w bind Entry <Control-Double-ButtonPress-1> {
	# do nothing
    }
    $w bind Entry <Control-B1-Motion> {
	# do nothing
    }
    $w bind Entry <Control-ButtonRelease-1> {
	after cancel $blt::tv::afterId
	set blt::tv::scroll 0
    }

    $w bind Entry <Control-Shift-ButtonPress-1> {
	if { [%W cget -selectmode] == "multiple" && [%W selection present] } {
	    if { [%W index anchor] == "" } {
		%W selection anchor current
	    }
	    if { [%W selection includes anchor] } {
		%W selection set anchor current
	    } else {
		%W selection clear anchor current
		%W selection set current
	    }
	} elseif { [%W cget -selectmode] != "none" } {
	    blt::tv::SetSelectionAnchor %W current
	}
    }
    $w bind Entry <Control-Shift-Double-ButtonPress-1> {
	# do nothing
    }
    $w bind Entry <Control-Shift-B1-Motion> {
	# do nothing
    }

    $w bind Entry <Shift-ButtonPress-3> {
	blt::tv::EditColumn %W %X %Y
    }

    $w column bind all <Enter> {
	%W column activate [%W column current]
    }
    $w column bind all <Leave> {
	%W column activate ""
    }
    $w column bind Rule <Enter> {
	%W column activate [%W column current]
	%W column resize activate [%W column current]
    }
    $w column bind Rule <Leave> {
	%W column activate ""
	%W column resize activate ""
    }
    $w column bind Rule <ButtonPress-1> {
	%W column resize anchor %x
    }
    $w column bind Rule <B1-Motion> {
	%W column resize mark %x
    }
    $w column bind Rule <ButtonRelease-1> {
	%W column configure [%W column current] -width [%W column resize set]
    }
    set ::blt::tv::curRelief raised
    $w column bind all <ButtonPress-1> {
	set blt::tv::column [%W column current]
	set blt::tv::curRelief [%W column cget $blt::tv::column -titlerelief]
	%W column configure $blt::tv::column -titlerelief sunken
    }
    $w column bind all <ButtonRelease-1> {
	set column [%W column current]
	if { $column != "" } {
	    %W column invoke $column
	}
         %W column configure $blt::tv::column -titlerelief $blt::tv::curRelief
    }
    if {$oldedit} {
    $w bind TextBoxStyle <Alt-ButtonPress-3> {
	if { [%W edit -root -test %X %Y] } {
	    break
	}
    }
    $w bind TextBoxStyle <Shift-ButtonRelease-1> {
        if { [%W edit -root -test %X %Y] } {
            blt::tv::EditColumn %W %X %Y
            break
        }
    }
    $w bind TextBoxStyle <Double-1> {
	if { [%W edit -root -test %X %Y] } {
	    blt::tv::EditColumn %W %X %Y
	    break
	}
    }
    }
#    $w bind CheckBoxStyle <Enter> {
#	set column [%W column current]
#	if { [%W column cget $column -edit] } {
#	    %W style activate current $column
#	}
#    }
#    $w bind CheckBoxStyle <Leave> {
#	%W style activate ""
#    }
    $w bind CheckBoxStyle <ButtonPress-1> {
        if { [%W edit -root -test %X %Y] } {
            event generate %W <<TreeViewEditStart>> -x [%W col index [%W col current]] -y [%W index @%x,%y]
            break
        }
    }
    $w bind CheckBoxStyle <B1-Motion> {
	if { [%W column cget [%W column current] -edit] } {
	    break
	}
    }
    $w bind CheckBoxStyle <ButtonRelease-1> {
	if { [%W edit -root -test %X %Y] } {
	    %W edit -root %X %Y
            event generate %W <<TreeViewEditEnd>> -x [%W col index [%W col current]] -y [%W index @%x,%y]
	    break
	}
    }
    if 0 {
    $w bind ComboBoxStyle <ButtonPress-1> {
	set column [%W column current]
        %W style activate focus $column
	if { [%W column cget $column -edit] } {
	    break
	}
    }
    $w bind ComboBoxStyle <ButtonRelease-1> {
        %W style activate 0
	if { [%W edit -root -test %X %Y] } {
	    %W edit -root %X %Y
	    break
	}
    }

    $w bind ComboBoxStyle <Double-1> {
        if { [%W edit -root -test %X %Y] } {
            blt::tv::EditColumn %W %X %Y
            break
        }
    }
}
    if {$oldedit} {
    $w bind ComboBoxStyle <Shift-ButtonRelease-1> {
        if { [%W edit -root -test %X %Y] } {
            blt::tv::EditColumn %W %X %Y
            break
        }
    }

    } else {
    $w bind ComboBoxStyle <ButtonPress-1> {}
    }
    $w bind ComboBoxStyle <ButtonRelease-1> {
        if { [%W edit -root -test %X %Y] } {
           blt::tv::Combobox-List %W %X %Y
        }
    }

}


proc ::blt::tv::Combobox-List-Done {W entry col args} {
    set i [$W index active]
    set v [$W get $i]
    set l [winfo parent $W]
    set w [winfo parent $l]
    $w entry set $entry $col $v
    event generate $w <<TreeViewEditEnd>> -x [$w col index $col] -y $entry
    destroy $l
    $w style activate 0
}

proc ::blt::tv::Combobox-List-Close {l} {
    set w [winfo parent $l]
    destroy $l
    $w style activate 0
}

proc ::blt::tv::Combobox-List {w x y} {
    # Popup combo-list for combobox.
    # TODO: could put frame in toplevel so not clipped.
    set Opts {
        { -height   6      "Listbox height" }
        { -leafs    False   "Edit only leaf nodes" }
        { -leave    1       "Setup handler for leave" }
        { -conf     {}      "Listbox widget configuration options" }
        { -optscmd  {}      "Callback to get edit options" -type {cmd w r c} }
        { -readonly False   "Do not allow editing" }
        { -useframe 1       "Use a frame below treeview widget" }
        { -usetopframe 0    "Use a frame at toplevel" }
        { -withouttag {}    "Edit only entries without tag"}
        { -withtag  {}      "Edit only entries with tag"}
    }
    if {[winfo exists $w.edit]} return
    set lx [expr {$x-[winfo rootx $w]}]
    set ly [expr {$y-[winfo rooty $w]}]
    set ind [$w index @$lx,$ly]
    #if {[llength [set lst [$w cget -values]]] == 0} return
    set col [$w column current]
    if {![$w column cget $col -edit]} return
    set widopts [$w column cget $col -editopts]
    set cellstyle [lindex [$w style get $col $ind] 0]
    if {$cellstyle != {}} {
         if {[$w style cget $cellstyle -readonly]} return
         set widopts [concat $widopts [$w style cget $cellstyle -editopts]]
    }
    set edopts {}
    foreach i $Opts {
        set q([lindex $i 0]) [lindex $i 1]
        lappend edopts [lindex $i 0] [lindex $i 1]
    }
    set opts { -activestyle dotbox -bd 2 -pad 10 -relief sunken -selectmode single }
    set style $cellstyle
    if {$style == {}} {
      set style [$w column cget $col -style]
    }
    set ckey [$w style cget $style -choicekey]
    set cmd [$w style cget $style -choicecmd]
    set lst [$w style cget $style -choices]
    if {$ckey != {}} {
        set lst [$w entry get $ind $ckey {}]
    }
    if {$cmd != {} && $lst == {}} {
        set cmd [string map [list %% % %W $w %X $x %Y $y %# $ind %C $col] $cmd]
        set lst [namespace eval :: $cmd]
    }
    set offs [$w column offsets]
    set cind [$w column index $col]
    set xstart [lindex $offs $cind]
    if {$cind >= ([llength $offs]-1)} {
        set xend [winfo width $w]
    } else {
        set xend [lindex $offs [expr {$cind+1}]]
    }
    if {$q(-optscmd) != {}} {
        set ewopts [eval $q(-optscmd) $w $ind $col]
        if {[llength $ewopts]%2} {
           tclLog "TreeView -optscmd: odd length: '$ewopts' for $w"
        } else {
           array set q $ewopts
        }
    }
    if {$q(-readonly)} return
    if {$q(-leafs) && [llength [$w entry children $ind]]} return
    set tags [$w tag names $ind]
    if {$q(-withtag) != {}} {
        if {[lsearch -exact $tags $q(-withtag)]<0} return
    }
    if {$q(-withouttag) != {}} {
        if {[lsearch -exact $tags $q(-withouttag)]>=0} return
    }
    set xsiz [expr {$xend-$xstart}]
    set entry $ind
    set bb [$w bbox -screen $ind]
    if {[llength $bb]!=4} return
    foreach {xbegin ystart xwidth ywidth} $bb break
    #set ystart [lindex $bb 1]
    set yend [expr {$ywidth+$ystart}]
    #tclLog "COL: $col, $lst, entry=$entry, offs=$offs, cind=$cind, xsiz=$xsiz, xstart=$xstart, bb=$bb"
    if {$q(-usetopframe)} {
        set wl [string trimright [winfo toplevel $w] .]._list
    } else {
        set wl $w._list
    }
    if {[winfo exists $wl]} {
        destroy $wl
        focus -force $w
        $w style activate 0
        return
    }
    event generate $w <<TreeViewEditStart>> -x $cind -y $ind
    $w style activate focus $cind
    focus $w
    if {$q(-useframe) || $q(-usetopframe)} {
        canvas $wl
        if {$q(-leave)} {
            bind $wl  <Leave> { ::blt::tv::Combobox-List-Close %W }
        }
    } else {
        toplevel $wl
        wm withdraw $wl
        wm transient $wl [winfo toplevel $w]
        raise [winfo toplevel $w]
        wm overrideredirect $wl 1
        if {$q(-leave)} {
            bind $wl  <Leave> { if {[winfo toplevel %W] == "%W"} { ::blt::tv::Combobox-List-Close %W } }
        }
    }
    set h $q(-height)
    if {[llength $lst]<=$h} {
        set h [expr {1+[llength $lst]}]
    }
    set opts [concat -height $h $opts]
    set l $wl._l
    listbox $l
    foreach {i j} $opts {
       catch { $l conf $i $j }
    }
    if {$q(-conf) != {}} {
       catch { eval $l conf $q(-conf) }
    }
    bindtags $l [concat [bindtags $l] TVComboBox::List]
    $l conf -yscrollcommand [list $wl._vscroll set]
    scrollbar $wl._vscroll -orient vertical -command [list $l yview]
    if {[llength $lst]>$h} {
        grid $wl._vscroll -row 1 -column 2 -sticky ns
    }
    grid $l -row 1 -column 1 -sticky news
    grid columnconf $wl 1 -weight 1
    grid rowconf $wl 1 -weight 1
    set val [$w get]
    foreach i $lst {
        $l insert end $i
        if {[string equal $i $val]} { $l activate end; $l see end }
    }
    bind $l <Visibility> [subst -nocommands {
        bind $l <Visibility> {}
        if {[$l xview] != "0 1"} {
            $l conf -xscrollcommand [list $wl._hscroll set]
            scrollbar $wl._hscroll -orient horizontal -command [list $wl xview]
            grid $wl._hscroll -row 2 -column 1 -sticky we
            grid $wl._vscroll -row 1 -column 2 -sticky ns
            focus -force $l
        }
    }]
    bind $l <<TVComboBox-List-Done>> [list [namespace current]::Combobox-List-Done  $l $entry $col]
    set H [winfo reqheight $l]
    set Xstart [expr {$xstart+[winfo x $w]}]
    set Yend [expr {$yend+[winfo x $w]}]
    set wwhig [winfo height $w]
    if {!(($ystart+$H) <= $wwhig || ($ystart-$H)<0)} {
        set Yend [expr {$ystart-$H}]
    }
    if {$q(-usetopframe)} {
        place $wl -in $w -width ${xsiz} -height $H -x $Xstart -y $Yend
        $wl conf -width $xsiz -height $H
    } elseif {$q(-useframe)} {
        place $wl -in $w -width ${xsiz} -height $H -x $Xstart -y $Yend
        $wl conf -width $xsiz -height $H
    } else {
        wm geometry $wl ${xsiz}x${H}+$Xstart+$Yend
        wm deiconify $wl
    }
    $w edit -noscroll
    if {$q(-usetopframe)} {
        bind $e <Destroy> +[list catch "$wl edit -scroll" ]
    } else {
        set tag [namespace current]
        bindtags $wl [concat $tag [bindtags $wl]]
        bind $tag <Destroy> {catch {[winfo parent %W] edit -scroll}}
    }
    after idle [list catch "focus -force $l"]
    return -code break
}

proc ::blt::tv::Combobox-List-Key {l ch args} {
    # Navigate to the first item starting with char ch.
    array set p { -now 0 }
    array set p $args
    if {![string is alpha -strict $ch]} return
    set cur [$l index active]
    set e [$l index end]
    foreach i {0 1} {
        set n -1
        while {[incr n]<$e} {
            set c [string index [$l get $n] 0]
            if {$i} { set c [string toupper $c] }
            if {[string equal $c $ch]} {
                $l activate $n
                $l see $n
                if {$n == $cur || $p(-now)} {
                    event generate $l <Return>
                }
                return
            }
        }
        set ch [string toupper $ch]
    }
    return -code break
}

proc ::blt::tv::SortColumn {t {column {}} args} {
    # Provide sorting for a column.
    array set p {-hold 1 -see 1 -highlight 0}
    array set p $args
    set do 1
    if {$column == {}} {
       set column [$t column current]
    }
    if {[string equal $column [$t sort cget -column]]} {
        set decr [expr {![$t sort cget -decreasing]}]
        if {!$decr} {
           $t sort conf -column {} -decreasing 0
           if {[$t sort cget -setflat] } {
              $t configure -flat no
              $t sort configure -setflat no
           }
           set do 0
        }
    } else {
        set decr 0
    }
    if {$do} {
        set mode [$t column cget $column -sortmode]
        if {$mode != "none"} {
           $t sort configure  -mode $mode
        }
        $t sort configure -decreasing $decr -column $column
        if {![$t cget -flat] } {
            $t configure -flat yes
            $t sort configure -setflat yes
        }
        $t sort auto yes
        if {$p(-hold)} {
            blt::busy hold $t
            update
            blt::busy release $t
        }
        if {$p(-highlight)} {
           after 300 [list $t column activate $column]
        }
    } else {
        if {$p(-highlight)} {
           $t column activate {}
        }
    }
    if {$p(-see)} {
       set sel [$t curselection]
       if {$sel != {}} {
         after idle [list $t see [lindex $sel 0]]
       }
    }
    set cind [$t column index $column]
    event generate $t <<TreeViewSortColumn>> -x $cind
}


bind TVComboBox <3>  [list blt::tv::Combobox-List %W %x %y]

bind TVComboBox::List  <Enter> { focus -force %W }
bind TVComboBox::List  <KeyRelease-Escape> { destroy [winfo parent %W] }
bind TVComboBox::List  <Return> { event generate %W <<TVComboBox-List-Done>>}
bind TVComboBox::List  <space> [bind TVComboBox::List <Return>]
bind TVComboBox::List  <ButtonRelease-1> [bind TVComboBox::List <Return>]
bind TVComboBox::List  <KeyRelease-space> [bind TVComboBox::List <Return>]
bind TVComboBox::List  <Control-n> [bind Listbox <Down>]
bind TVComboBox::List  <Control-p> [bind Listbox <Up>]
bind TVComboBox::List  <Control-d> [bind Listbox <Next>]
bind TVComboBox::List  <Control-u> [bind Listbox <Prior>]
bind TVComboBox::List  <KeyPress> [list blt::tv::Combobox-List-Key %W %A]

proc blt::tv::SetSelectionAnchor { w tagOrId } {
    if {$tagOrId == ""} return
    set index [$w index $tagOrId]
    # If the anchor hasn't changed, don't do anything
    if { $index != [$w index anchor] } {
	$w selection clearall
	$w see $index
	$w focus $index
	$w selection set $index
	$w selection anchor $index
    }
}

proc blt::tv::SetSelectionSetAnchor { w x y} {
    set mode [$w cget -selectmode]
    switch -- $mode {
        none return
        cell { }
        multicell {}
        default {
            set blt::tv::x $x
            set blt::tv::y $y
            SetSelectionAnchor $w @$x,$y
            return
        }
    }
    $w selection clearall
    set index [$w index @$x,$y]
    set col [$w column nearest $x]
    if {$index != {} && $col != {}} {
        $w selection set $index $index $col
        $w selection anchor $index $col
        $w focus $index
    }
}

proc  blt::tv::SetSelectionAdd {w x y} {
    set mode [$w cget -selectmode]
    set index [$w index @$x,$y]
    switch -- $mode {
        none return
        cell { $w selection clearall }
        multicell { }
        multiple {
            set index [$w index current]
            $w selection toggle $index
            $w selection anchor $index
            return
        }
        default {
            SetSelectionAnchor $w current
            return
        }
    }
    set col [$w column nearest $x]
    if {$index != {} && $col != {}} {
        $w selection toggle $index $index $col
    }
}

proc blt::tv::SetSelectionExtendAnchor {w x y} {
    set mode [$w cget -selectmode]
    switch -- $mode {
        none {}
        multiple {
            if {[$w selection present] } {
                if { [$w index anchor] == "" } {
                    $w selection anchor current
                }
                set index [$w index anchor]
                $w selection clearall
                $w selection set $index current
            }
        }
        single {
            blt::tv::SetSelectionAnchor $w current
        }
        cell {
            SetSelectionSetAnchor $w $x $y
        }
        multicell {
            # Select range.
            set col [$w column nearest $x]
            set oanch [$w selection anchor]
            set anch [$w index anchor]
            set ocell [lindex $oanch 1]
            set index [$w index @$x,$y]
            if {$col == {} || $ocell == {}} {
                return [SetSelectionSetAnchor $w $x $y]
            }
            set cols [$w column names]
            set coli [lsearch $cols $col]
            set ocelli [lsearch $cols $ocell]
            if {$coli<0 || $ocelli<0} {
                return [SetSelectionSetAnchor $w $x $y]
            }
            if {$coli<$ocelli} {
                set sci $coli
                set coli $ocelli
                set ocelli $sci
            }
            set clst {}
            foreach c [lrange $cols $ocelli $coli] {
                if {$c == "#0"} continue
                if {[$w column cget $c -hide]} continue
                lappend clst $c
            }
            set nlst {}
            foreach n [$w find $index $anch] {
                if {[$w entry cget $n -hide]} continue
                lappend nlst $n
            }
            $w selection clearall
            foreach n $nlst {
                foreach c $clst {
                    $w selection set $n $n $c
                }
            }
        }
    }
}

# ----------------------------------------------------------------------
#
# AutoScroll --
#
#	Invoked when the user is selecting elements in a treeview
#	widget and drags the mouse pointer outside of the widget.
#	Scrolls the view in the direction of the pointer.
#
# ----------------------------------------------------------------------
proc blt::tv::AutoScroll { w } {
    if { ![winfo exists $w] } {
	return
    }
    set x $blt::tv::x
    set y $blt::tv::y

    set index [$w nearest $x $y]

    if {$y >= [winfo height $w]} {
	$w yview scroll 1 units
	set neighbor down
    } elseif {$y < 0} {
	$w yview scroll -1 units
	set neighbor up
    } else {
	set neighbor $index
    }
    if { [$w cget -selectmode] == "single" } {
	blt::tv::SetSelectionAnchor $w $neighbor
    } elseif { [$w cget -selectmode] != "none" } {
	catch {$w selection mark $index}
    }
    set ::blt::tv::afterId [after 50 blt::tv::AutoScroll $w]
}

proc blt::tv::SetFocus { w tagOrId } {
    # Set focus at index given by tagOrId.
    if {[catch {$w index $tagOrId} t]} return
    if {[catch {$w focus $t}]} return
    $w selection clearall
    if {[catch {$w selection set $t}]} return
    $w selection anchor $t
    $w entry activate $t
    $w see $t
    event generate $w <<TreeViewFocusEvent>>
    return $t
}
# ----------------------------------------------------------------------
#
# MoveFocus --
#
#	Invoked by KeyPress bindings.  Moves the active selection to
#	the entry <where>, which is an index such as "up", "down",
#	"prevsibling", "nextsibling", etc.
#
# ----------------------------------------------------------------------
proc blt::tv::MoveFocus { w tagOrId {flag 0}} {
    set mode [$w cget -selectmode]
    switch -- $mode {
        multiple {
           catch {$w focus $tagOrId}
           if {!$flag} {
               $w selection clearall
           }
           if {[catch {$w selection set focus}]} return
           $w selection anchor focus
       }
        single {
           catch {$w focus $tagOrId}
           $w selection clearall
           if {[catch {$w selection set focus}]} return
	   $w selection anchor focus
        }
        multicell -
        cell {
           set cells [$w selection cells]
           catch {$w focus $tagOrId}
           $w selection clearall
           if {[catch {$w selection set focus}]} return
	   $w selection anchor focus
	   set ind [$w index focus]
           if {$cells != {}} {
               set col [lindex $cells 1]
           } else {
               set vcols [$w column names -visible]
               set col [lindex $vcols 0]
           }
           $w selection set focus focus $col
       }
    }
    $w see focus
    event generate $w <<TreeViewFocusEvent>>
}

# ----------------------------------------------------------------------
#
# MovePage --
#
#	Invoked by KeyPress bindings.  Pages the current view up or
#	down.  The <where> argument should be either "top" or
#	"bottom".
#
# ----------------------------------------------------------------------
proc blt::tv::MovePage { w where } {

    # If the focus is already at the top/bottom of the window, we want
    # to scroll a page. It's really one page minus an entry because we
    # want to see the last entry on the next/last page.
    set focus [$w index focus]
    if {$where == "top"} {
        if {[$w index up] != $focus} {
            $w yview scroll -1 pages
            $w yview scroll 1 units
        }
        if {[$w index focus] == $focus} {
           catch { $w entry select up }
        }
    } else {
        if {[$w index down] != $focus} {
            $w yview scroll 1 pages
            $w yview scroll -1 units
        }
        if {[$w index focus] == $focus} {
           catch { $w entry select down }
        }
    }
    update

    # Adjust the entry focus and the view.  Also activate the entry.
    # just in case the mouse point is not in the widget.
    $w entry activate view.$where
    $w focus view.$where
    $w see view.$where
    if { [$w cget -selectmode] == "single" } {
        $w selection clearall
        catch {$w selection set focus}
    }
    event generate $w <<TreeViewFocusEvent>>
}

# ----------------------------------------------------------------------
#
# NextMatch --
#
#	Invoked by KeyPress bindings.  Searches for an entry that
#	starts with the letter <char> and makes that entry active.
#
# ----------------------------------------------------------------------
proc blt::tv::NextMatch { w key state} {
    if {$state != 0 && $state != 1} return
    if {[string match {[ -~]} $key]} {
	set last [$w index focus]
	set next [$w index next]
	while { $next != {} && $next != $last } {
	    set label [$w entry cget $next -label]
	    set label [string index $label 0]
	    if { [string tolower $label] == [string tolower $key] } {
		break
	    }
	    set next [$w index -at $next next]
	}
        if {$next == {}} return
	$w focus $next
	if {[$w cget -selectmode] == "single"} {
	    $w selection clearall
	    $w selection set focus
            event generate $w <<TreeViewFocusEvent>>
	}
	$w see focus
    }
}

#------------------------------------------------------------------------
#
# InsertText --
#
#	Inserts a text string into an entry at the insertion cursor.
#	If there is a selection in the entry, and it covers the point
#	of the insertion cursor, then delete the selection before
#	inserting.
#
# Arguments:
#	w 	Widget where to insert the text.
#	text	Text string to insert (usually just a single character)
#
#------------------------------------------------------------------------
proc blt::tv::InsertText { w text } {
    if { [string length $text] > 0 } {
	set index [$w index insert]
	if { ($index >= [$w index sel.first]) &&
	     ($index <= [$w index sel.last]) } {
	    $w delete sel.first sel.last
	}
	$w insert $index $text
    }
}

#------------------------------------------------------------------------
#
# Transpose -
#
#	This procedure implements the "transpose" function for entry
#	widgets.  It tranposes the characters on either side of the
#	insertion cursor, unless the cursor is at the end of the line.
#	In this case it transposes the two characters to the left of
#	the cursor.  In either case, the cursor ends up to the right
#	of the transposed characters.
#
# Arguments:
#	w 	The entry window.
#
#------------------------------------------------------------------------
proc blt::tv::Transpose { w } {
    set i [$w index insert]
    if {$i < [$w index end]} {
	incr i
    }
    set first [expr {$i-2}]
    if {$first < 0} {
	return
    }
    set new [string index [$w get] [expr {$i-1}]][string index [$w get] $first]
    $w delete $first $i
    $w insert insert $new
}

#------------------------------------------------------------------------
#
# GetSelection --
#
#	Returns the selected text of the entry with respect to the
#	-show option.
#
# Arguments:
#	w          Entry window from which the text to get
#
#------------------------------------------------------------------------

proc blt::tv::GetSelection { w } {
    set text [string range [$w get] [$w index sel.first] \
                       [expr {[$w index sel.last] - 1}]]
    if {[$w cget -show] != ""} {
	regsub -all . $text [string index [$w cget -show] 0] text
    }
    return $text
}

proc blt::tv::TextCopy {w {edit 0} {aslist 0}} {
    # Handle <<Copy>> event, copying selection/focus to clipboard.
    if {!$edit} {
      catch {
       set inds [$w curselection]
       if {$inds == {}} {
           set inds [$w index focus]
       }
       set all {}
       set n -1
       foreach ind $inds {
           incr n
           set data {}
           foreach i [$w column names] {
               if {[$w col cget $i -hid]} continue
               if {$i == "#0"} {
                   set val [$w entry cget $ind -label]
               } else {
                   set val [$w entry set $ind $i]
               }
               if {$aslist} {
                   lappend data $val
               } else {
                   append data " " $val
               }
           }
           if {$aslist} {
               lappend all $data
           } else {
               if {$n} { append all \n }
               append all $data
           }

       }
       clipboard clear -displayof $w
       clipboard append -displayof $w $all
      }
    } else {
    catch {
       set w [winfo parent $w]
       set ind [$w index focus]
       set col [$w column current]
       if {$col == {}} {
           set col $::blt::tv::curCol
       }
       set data [$w entry set $ind $col]
       clipboard clear -displayof $w
       clipboard append -displayof $w $data
    }
   }
}

proc ::blt::tv::Toggle {w ind} {
    # Toggle and set view.
    set ind [$w index $ind]
    if {$ind == {}} return
    $w toggle $ind
    if {[$w entry isopen $ind] && [$w cget -openanchor] != {} && [$w entry children $ind] != {}} {
        $w see -anchor [$w cget -openanchor] $ind
    } else {
        $w see $ind
    }
}

proc ::blt::tv::Click {w x y} {
   if {[focus] != $w} { focus $w }
   set ::blt::tv::space off
   #if {[winfo exists $w.edit]} { destroy $w.edit }
   event generate $w <<TreeViewFocusEvent>>
}

bind $className <ButtonRelease-1> {::blt::tv::Click %W %x %y}
bind $className <Double-ButtonPress-1> {blt::tv::EditCol %W %x %y }
bind $className <Alt-ButtonPress-1> {blt::tv::EditCol %W %x %y }
bind $className <Control-minus> {if {[%W index parent]>0} { after idle "%W entry select [%W entry parent focus]"; %W close [%W entry parent focus] }}
bind $className <Control-o> { ::blt::tv::Toggle %W focus }
bind $className <Control-a> { blt::tv::MoveFocus %W parent }
bind $className <Control-Shift-O> { %W open -recurse focus }
bind $className <Control-Shift-C> { %W close -recurse focus }

bind TreeViewEditWin <KeyRelease-Escape> {focus [winfo parent %W]; destroy %W; break}
bind TreeViewEditWin <KeyPress-Return> {event generate %W <<TreeViewEditComplete>>; break}
#bind TreeViewEditWin <KeyPress-Return> {break}

proc ::blt::tv::EditDone {w e x y ind col cind data styledata cellstyle ied endcmd treelabel vcmd} {
    # # Handle edit completion: call $endcmd and widget -vcmd if req.
    switch -- [winfo class $e] {
        Entry - Spinbox {
            set newdata [$e get]
        }
        Text {
            set newdata [string trimright [$e get 1.0 end]]
        }
        default {
            set newdata $data
        }
    }
    # Invoke validation for Entry/Spinbox/Text if string changed.
    set ok 1
    if {![string equal $data $newdata]} {
        if {$vcmd == {}} {
            set vcmd [$w column cget $col -validatecmd]
        }
        if {$vcmd != {}} {
            if {[string first % $vcmd]>=0} {
                set ccmd [string map [list %% % %W $w %X $x %Y $y %# $ind %C $cind %V [list $newdata]] $vcmd]
            } else {
                set ccmd [concat $vcmd [list $w $newdata $data $ind $col]]
	    }
            set newdata [namespace eval :: $ccmd]
        }
        if {![winfo exists $w]} return
    }
    if {![string equal $data $newdata]} {
        set istree [$w column istree $cind]
        if {$ind == -1} {
            $w col conf $cind -title $newdata
        } elseif {$istree} {
            if {$treelabel} {
                [$w cget -tree] label $ind $newdata
            } else {
                $w entry conf $ind -label $newdata
            }
        } else {
            if {$styledata != {}} {
                set newdata [list $styledata $newdata]
            }
            $w entry set $ind $col $newdata
        }
        if {$endcmd != {}} {
            if {[string first % $endcmd]>=0} {
                set ccmd [string map [list %% % %W $w %X $x %Y $y %# $ind %C $cind %V [list $newdata]] $endcmd]
                set ccmd [concat $endcmd [list $w $newdata $data $ind $col]]
            }
            namespace eval :: $ccmd
        }
        if {![winfo exists $w]} return
        event generate $w <<TreeViewEditEnd>> -x $cind -y $ind
    }
    if {$ied} {
      catch { bind $e <Destroy> {} }
      $w style set $cellstyle $col $ind
    } else {
      catch { place forget $e }
    }
    destroy $e
    catch { focus $w }
    #after idle [list destroy $e]
}

proc ::blt::tv::TabMove {w ind cind args} {
    # Handle Tab char.
    #Opts p $args {
    #    { -cmd      {}      "Callback to get next cell" -type {cmd ind cind} }
    #    { -endcol   {}      "Maximum column (defaults to last col)" }
    #    { -startcol 0       "Column to start new row at" }
    #    { -wrap     True    "At last row return to top" }
    #}
    array set p { -cmd {} -endcol {} -startcol 0 -wrap True -opts {}}
    array set p $args
    set vis [$w column names -visible]
    set maxc [expr {[llength $vis]-1}]
    if {$p(-endcol) == {} || $p(-endcol) > $maxc} {
        set p(-endcol) $maxc
    }
    set maxr [$w index end]
    if {$p(-cmd) != {}} {
        set ncol [eval $p(-cmd) $ind,$cind]
        if {$ncol == {}} return
        foreach {ind col} $ncol break
        EditCell $w $ind $col
        return

    }
    set down [expr {$p(-wrap)?"next":"down"}]
    set cnt 100
    while 1 {
        if {[incr cnt -1] == 0} return
        incr cind 1
        if {$cind > $p(-endcol)} {
            set cind $p(-startcol)
            set ind [$w index $down]
            $w focus $ind
        }
        if {[$w column cget $cind -edit] && ![$w column cget $cind -hide]} break
    }
    EditCell $w $ind $cind
    return
}

proc ::blt::tv::EditCell { w ind col {x {}} {y {}}} {
    # Handle text editing of a cell.
    if {![winfo exists $w]} return
    # Option choices for -editopts.
    set Opts {
        { -allowtypes textbox "List of types to allow text editing for (or *)" }
        { -autonl   False   "Default to text widget if newlines in data"}
        { -choices  {}      "Choices for combo/spinbox" }
        { -conf     {}      "Extra entry/text widget options to set" }
        { -embed    False   "Use an embedded window style for edit window" }
        { -endcmd   {}      "Command to invoke at end of edit" -type cmd  }
        { -leafs    False   "Edit only leaf nodes" }
        { -nlkeys {<Control-r> <Shift-Return>} "Keys for inserting newline" }
        { -notnull  False   "Field may not be null" }
        { -optscmd  {}      "Callback to get edit options" -type {cmd w r c} }
        { -readonly False   "Do not allow editing" }
        { -sel      True    "Value is selected on edit" }
        { -startcmd {}      "Command to invoke at start of edit" -type {cmd w r c}  }
        { -tab      {}      "bind Tab char in edit (bool or args passed to TabMove)" }
        { -titles   False   "Allow edit of titles" }
        { -treelabel True   "Edit -tree cmd label rather than treeview label" }
        { -type     {}      "Support basic Wize types like bool, int, and choice" }
        { -typecol  {}      "Column/key to get -type from" }
        { -undo     True    "Text widget enables undo" }
        { -vcmd     {}      "Validate command to override -validatecmd" -type cmd }
        { -widget   {}      "Widget to use (defaults to entry)" }
        { -withouttag {}    "Edit only entries without tag"}
        { -withtag  {}      "Edit only entries with tag"}
        { -wrap     none    "Wrap mode for text widget" }
    }
    if {[winfo exists $w._list]} return
    $w see current
    set e $w.edit
    if { [winfo exists $e] } { destroy $e }
    set ind [$w index $ind]
    set cind [$w column index $col]
    set ed [$w column cget $col -edit]
    if { !$ed  } return
    set intitle 0
    if {$x == {}} {
        set bb [$w col bbox $col $ind]
        set x [lindex $bb 0]
        set y [lindex $bb 1]
    }
    set istree [$w column istree $col]
    set edopts {}
    foreach i $Opts {
        set q([lindex $i 0]) [lindex $i 1]
        lappend edopts [lindex $i 0] [lindex $i 1]
    }
    set widopts [$w column cget $col -editopts]
    set cellstyle [lindex [$w style get $cind $ind] 0]
    if {$cellstyle != {}} {
         if {[$w style cget $cellstyle -readonly]} return
         set widopts [concat $widopts [$w style cget $cellstyle -editopts]]
    }
    if {$widopts != {}} {
        if {[llength $widopts]%2} {
            tclLog "TreeView -editopts: odd length: '$widopts' for $w"
        } else {
            array set q $widopts
            if {$q(-optscmd) != {}} {
                set ewopts [eval $q(-optscmd) $w $ind $col]
                if {[llength $ewopts]%2} {
                   tclLog "TreeView -optscmd: odd length: '$ewopts' for $w"
                } else {
                   array set q $ewopts
                }
            }
            if {[array size q] != ([llength $edopts]/2)} {
               set bad {}
               array set r $edopts
               set good [lsort [array names r]]
               foreach {i j} $widopts {
                  if {![info exists r($i)]} { lappend bad $i }
               }
               tclLog "TreeView -editopts: bad option: '$bad' not in '$good'"
            }
        }
    }
    if {$q(-typecol) != {} && $q(-type) == {}} {
        if {[catch { set q(-type) [$w entry set $ind $q(-typecol)] }] &&
            [catch { set q(-type) [[$w cget -tree] get $ind $q(-typecol)] }]} {
            tclLog "Failed to get -typecol $q(-typecol)"
        }
    }
    set wopts {}
    if {$q(-type) != {}} {
        switch -- [lindex $q(-type) 0] {
            bool {
                set q(-choices) {"" True False}
                set q(-widget) spinbox
            }
            Bool {
                lset q(-choices) {True False}
                set q(-widget) spinbox
            }
            int - Int -
            double - Double {
                array set qq {-min -999999999 -max 99999999 -incr 1}
                array set qq [lrange $q(-type) 1 end]
                set wopts [list -from $qq(-min) -to $qq(-max) -increment $qq(-incr)]
                set q(-widget) spinbox
            }
            Choice {
                set q(-choices) [lrange $q(-type) 1 end]
                set q(-widget) spinbox
            }
        }
    }
    if {$q(-readonly)} return
    if {!$q(-titles) && [set intitle [expr {[$w column nearest $x $y] != {}}]]} return
    if {$q(-leafs) && [llength [$w entry children $ind]]} return
    set tags [$w tag names $ind]
    if {$q(-withtag) != {}} {
        if {[lsearch -exact $tags $q(-withtag)]<0} return
    }
    if {$q(-withouttag) != {}} {
        if {[lsearch -exact $tags $q(-withouttag)]>=0} return
    }
    set styledata {}
    if {$intitle} {
        set data [$w column cget $col -title]
        set ind -1
    } elseif {$istree } {
        if {$q(-treelabel) && [namespace which [$w cget -tree]] == {}} {
            set q(-treelabel) 0
        }
        if {$q(-treelabel)} {
            set data [[$w cget -tree] label $ind]
        } else {
            set data [$w entry cget $ind -label]
        }
    } else {
        set data [$w entry set $ind $col]
        if {[$w cget -inlinedata] && [string first @ $data]>=0} {
            if {![catch {llength $data} len] && $len <= 2 &&
            [string match @?* [lindex $data 0]] &&
            [lsearch -exact [$w style names] [string range [lindex $data 0] 1 end]]>=0} {
                #set styledata [lindex $data 0]
                set data [lindex $data 1]
            }
        }
    }
    set bbox [$w column bbox -visible $col $ind]
    if {![llength $bbox]} return
    foreach {X Y W H} $bbox break
    set wid entry
    set style [$w col cget $cind -style]
    set rstyle [expr {$cellstyle == {} ? $style : $cellstyle }]
    set stype [expr {$rstyle == {} ? {} : [$w style type $rstyle]}]
    #if {[$w style cget $style -readonly]} return
    if {[lsearch $q(-allowtypes) $stype]<0 && $q(-allowtypes) != "*"} return
    if {$widopts != {}} {
        if {$q(-widget) != {}} {
            set wid $q(-widget)
        } elseif {$q(-autonl)} {
            if {$stype == "combobox"} {
                set wid spinbox
            } elseif {[string first \n $data]>=0} {
                set wid text
            } else {
                set wid entry
            }
        }
    }
    if {[catch {eval $wid $e} err]} {
        entry $e
    }
    foreach {i j} $wopts {
       catch { $e conf $i $j}
    }
    catch { $e conf -font [$w cget -font] }
    set ied 0
    if {$q(-embed) && !$istree} {
      catch {$w style create windowbox editwin}
      if {$bbox != {}} {
        #TODO: embedded should temporarily set col width, if currently is 0.
        set mwid $W
        if {$mwid>16} { incr mwid -10 }
        set mhig $H
        $w style conf editwin -minheight $mhig -minwidth $mwid
      }
      $w style set editwin $cind $ind
      $w entry set $ind $col $e
      bind $e <Destroy> [list $w entry set $ind $col $data]
      bind $e <Destroy> +[list $w style set $cellstyle $col $ind]
      set ied 1
    } else {
      place $e -x $X -y $Y -width $W -height $H
    }
    switch -- [winfo class $e] {
        Entry {
            $e insert end $data
            if {$q(-sel)} { $e selection range 0 end }
            foreach i $q(-nlkeys) {
                bind $e $i "$e insert insert {\n}; break"
            }
        }
        Spinbox {
            $e insert end $data
            if {$q(-choices) != {}} {
                $e conf -values $q(-choices)
            }
            if {$q(-sel)} { $e selection range 0 end }
            set style [$w col cget $cind -style]
            if {$stype == "combobox"} {
                set ch [$w style cget $style -choices]
                if {$ch == {} && [set ccmd [$w style cget $style -choicecmd]] != {}} {
                    set ccmd [string map [list %% % %W $w %X $x %Y $y %# $ind %C $cind] $ccmd]
                    set ch [namespace eval :: $ccmd]
                }
                if {$ch != {}} {
                    if {[set nn [lsearch -exact $ch $data]]<0} {
                        set ch [concat [list $data] $ch]
                    } elseif {$n != 0} {
                        set ch [concat [list $data] [lreplace $ch $nn $nn]]
                    }
                    $e conf -values $ch
                }
            }
            foreach i $q(-nlkeys) {
                bind $e $i "$e insert insert {\n}; break"
            }
        }
        Text {
            $e conf -highlightthick 0 -padx 0 -pady 0 -bd 1
            $e conf -undo $q(-undo) -wrap $q(-wrap)
            $e insert end $data
            if {$q(-sel)} { $e tag add sel 1.0 end }
            foreach i $q(-nlkeys) {
                bind $e $i "$e insert insert {\n}; $e see insert; break"
            }
        }
    }
    catch {$e conf -highlightthick 0}
    if {$q(-conf) != {}} {
        if {[catch {eval $e conf $q(-conf)} err]} {
            tclLog "Opts err: $err"
        }
    }
    bind $e <1> [list focus $e]
    bindtags $e [concat TreeViewEditWin [bindtags $e]]
    tkwait visibility $e
    focus $e
    after 100 [list catch [list focus $e]]
    after 300 [list catch [list focus $e]]
    bind $e <<TreeViewEditComplete>> [list ::blt::tv::EditDone $w $e $x $y $ind $col $cind $data $styledata $cellstyle $ied $q(-endcmd) $q(-treelabel) $q(-vcmd)]
    if {![string is false $q(-tab)]} {
        set topts {}
        if {![string is true $q(-tab)]} {
            set topts $q(-tab)
        }
        bind $e <Tab> "event generate $e <<TreeViewEditComplete>>; [namespace current]::TabMove $w $ind $cind $topts; break"
    }
    event generate $w <<TreeViewEditStart>> -x $cind -y $ind
    if {[winfo exists $e] && $q(-startcmd) != {}} {
       if {[string first % $vcmd]>=0} {
           set ccmd [string map [list %% % %W $w %X $x %Y $y %# $ind %C $cind %V [list $data]] $q(-startcmd)]
       } else {
           set ccmd [concat $q(-startcmd) [list $w $col $ind] ]
       }
       namespace eval :: $ccmd
    }
    if {![winfo exists $e]} return
    $w edit -noscroll
    set tag [namespace current]
    bindtags $e [concat $tag [bindtags $e]]
    bind $tag <Destroy> { catch {[winfo parent %W] edit -scroll} }
    return
}

proc ::blt::tv::commify {num {sep ,}} {
    # Make number comma seperated every 3 digits.
    while {[regsub {^([-+]?\d+)(\d\d\d)} $num "\\1$sep\\2" num]} {}
    return $num
}

proc ::blt::tv::EditCol { w x y } {
    # Main handler for cell-edit/toggle-open.
    if {![winfo exists $w]} return
    $w see current
    set e $w.edit
    if { [winfo exists $e] } { destroy $e }
    set ::blt::tv::curCol [set col [$w column nearest $x]]
    if {$col == {}} return
    set cind [$w column index $col]
    set ind [$w index @$x,$y]
    if { ![$w column cget $col -edit]  } {
        if {$cind == 0} {
            Toggle $w $ind
        }
        return
    } elseif {$cind == 0} {
        set object {}
        $w nearest $x $y object
        if {$object != "label"} {
            Toggle $w $ind
            return
        }
    }
    EditCell $w $ind $col $x $y
}


proc blt::tv::EditColumn { w x y } {
    # Old edit function.
    $w see current
    if { [winfo exists $w.edit] } {
        destroy $w.edit
    }
    set col [$w column current]
    if {$col == {}} return
    set ::blt::tv::curCol $col
    if { ![$w edit -root -test $x $y] } {
        return
    }
    set ind [$w index @$x,$y]
    if {$ind == {}} return
    set data [$w entry set $ind $col]
    $w edit -root $x $y
    update
    focus $w.edit
    $w.edit selection range 0 end
    event generate $w <<TreeViewEditStart>> -x $x -y $y
    grab set $w.edit
    tkwait window $w.edit
    grab release $w.edit
    if {[winfo exists $w]} {
        event generate $w <<TreeViewEditEnd>> -x $x -y $y
    }
}

proc ::blt::tv::SortTree {t column {ind {}} {uselabel 1} {see 1}} {
    # Sort the children of tree.
    set istree [$t column istree $column]
    if {$ind == {}} {
       set ind [lindex [$t curselection] 0]
       if {$ind == {}} {
          if {$istree} { set ind 0 } else {  set ind focus }
       }
    }
    set ind [$t index $ind]
    set clst [$t entry children $ind]
    if {$clst == {}} return
    set slst {}
    foreach i $clst {

       if {$istree} {
          set txt [expr {$uselabel?[$t entry cget $i -label]:[$t get $i]}]
       } else {
          set txt [$t entry set $i $column]
       }
       lappend slst [list $txt $i]
    }
    if {[set decreasing [$t sort cget -decreasing]]} {
      set dec -decreasing
    } else {
      set dec -increasing
    }
    #set decreasing [expr {!$decreasing}]
    #$t sort conf -decreasing $decreasing
    set mode [$t column cget $column -sortmode]
    if {$mode == "none"} {
       set mode [$t sort cget -mode]
    }
    if {$mode == "none"} return
    if {$mode == "command"} {
       set slst [lsort $dec -command [$t column cget $column -sortcommand] $slst]
    } else {
       set slst [lsort $dec -$mode $slst]
    }
    foreach i $slst {
       set oi [lindex $i 1]
       $t move $oi into $ind
    }
    if {$see} {
       set sel [$t curselection]
       if {$sel != {}} {
         after idle [list $t see [lindex $sel 0]]
       }
    }
    set cind [$t column index $column]
    event generate $t <<TreeViewSortTree>> -x $cind -y $ind
}


#
# ButtonPress assignments
#
#	B1-Enter	start auto-scrolling
#	B1-Leave	stop auto-scrolling
#	ButtonPress-2	start scan
#	B2-Motion	adjust scan
#	ButtonRelease-2 stop scan
#
bind ${className} <ButtonPress-2> {
    focus %W
}

bind ${className} <ButtonPress-2> {
    set blt::tv::cursor [%W cget -cursor]
    %W configure -cursor hand1
    %W scan mark %x %y
}

bind ${className} <B2-Motion> {
    catch { %W scan dragto %x %y }
}

bind ${className} <ButtonRelease-2> {
    catch { %W configure -cursor $blt::tv::cursor }
}

bind ${className} <B1-Leave> {
    if { $blt::tv::scroll } {
	blt::tv::AutoScroll %W
    }
}

bind ${className} <B1-Enter> {
    after cancel $blt::tv::afterId
}

#
# KeyPress assignments
#
#	Up
#	Down
#	Shift-Up
#	Shift-Down
#	Prior (PageUp)
#	Next  (PageDn)
#	Left
#	Right
#	space		Start selection toggle of entry currently with focus.
#	Return		Start selection toggle of entry currently with focus.
#	Home
#	End
#	F1
#	F2
#	ASCII char	Go to next open entry starting with character.
#
# KeyRelease
#
#	space		Stop selection toggle of entry currently with focus.
#	Return		Stop selection toggle of entry currently with focus.


bind ${className} <KeyPress-Up> {
    blt::tv::MoveFocus %W up
    if { $blt::tv::space } {
	%W selection toggle focus
    }
}

bind ${className} <KeyPress-Down> {
    blt::tv::MoveFocus %W down
    if { $blt::tv::space } {
	%W selection toggle focus
    }
}
bind ${className} <Control-KeyPress-n> [bind ${className} <KeyPress-Down>]
bind ${className} <Control-KeyPress-p> [bind ${className} <KeyPress-Up>]

bind ${className} <Shift-KeyPress-Up> {
    blt::tv::MoveFocus %W up 1
}

bind ${className} <Shift-KeyPress-Down> {
    blt::tv::MoveFocus %W down 1
}

bind ${className} <KeyPress-Prior> {
    blt::tv::MovePage %W top
}

bind ${className} <KeyPress-Next> {
    blt::tv::MovePage %W bottom
}
bind ${className} <Control-KeyPress-d> [bind ${className} <KeyPress-Next>]
bind ${className} <Control-KeyPress-u> [bind ${className} <KeyPress-Prior>]

#bind ${className} <KeyPress-Left> {
#    %W close focus
#}
#bind ${className} <KeyPress-Right> {
#    %W open focus
#    %W see focus -anchor w
#}
proc blt::tv::MoveKey {w cnt} {
    set mode [$w cget -selectmode]
    set iscell [expr {$mode == "cell" || $mode == "multicell"}]
    if {!$iscell} { return [$w xview scroll $cnt unit] }
    set cells [$w selection cells]
    if {$cells != {}} {
        set vcols [$w col names -visible]
        foreach {ind col} $cells break
        set cind [lsearch $vcols $col]
        if {$cind >= 0} {
            set cind [expr {$cind+$cnt}]
            if {$cind>=[llength $vcols]} {
                set cind [expr {[llength $vcols]-1}]
            } elseif {$cind < 0} {
                set cind 0
            }
            set ncol [lindex $vcols $cind]
            $w selection clearall
            $w selection set $ind $ind $ncol
            $w column see $ncol
        }
    }
}

proc blt::tv::MarkPos {w} {
    if { [$w cget -selectmode] == "single" } {
	if { [$w selection includes focus] } {
	    $w selection clearall
	} else {
	    $w selection clearall
	    $w selection set focus
	}
     } elseif { [$w cget -selectmode] != "none" } {
	$w selection toggle focus
    }
    set blt::tv::space on
}

bind ${className} <KeyPress-Left> { blt::tv::MoveKey %W -1}
bind ${className} <KeyPress-Right> { blt::tv::MoveKey %W 1}
#bind ${className} <KeyPress-Left> { %W xview scroll -1 unit}
#bind ${className} <KeyPress-Right> { %W xview scroll 1 unit}
bind ${className} <Control-KeyPress-Left> { %W xview scroll -1 page}
bind ${className} <Control-KeyPress-Right> { %W xview scroll 1 page}

bind ${className} <KeyPress-space> { catch {blt::tv::MarkPos %W } }

bind ${className} <KeyRelease-space> {
    set blt::tv::space off
}

#bind ${className} <KeyPress-Return> {
#    blt::tv::MoveFocus %W focus
#    set blt::tv::space on
#}

#bind ${className} <KeyRelease-Return> {
#    set blt::tv::space off
#}

bind ${className} <KeyPress-Return> {
    #set blt::tv::space on
    blt::tv::Toggle %W focus
    #set blt::tv::space off
}

bind ${className} <KeyRelease-Return> {
    #set blt::tv::space off
}

bind ${className} <KeyPress> {
    blt::tv::NextMatch %W %A %s
}

bind ${className} <KeyPress-Home> {
    blt::tv::MoveFocus %W top
}

bind ${className} <KeyPress-End> {
    blt::tv::MoveFocus %W bottom
}

bind ${className} <Control-F1> {
    %W open -trees root
}

bind ${className} <Control-F2> {
    eval %W close -trees root
}

bind ${className} <Control-F3> {
    %W conf -flat [expr {![%W cget -flat]}]
}

bind ${className} <Control-F4> {
    eval %W col conf [%W col names] -width 0
}

bind ${className} <MouseWheel> {
        if {%D >= 0} {
            %W yview scroll [expr {-%D/30}] units
        } else {
            %W yview scroll [expr {(2-%D)/30}] units
        }
}

if {[tk windowingsystem] == "x11"} {
  bind ${className} <4> { %W yview scroll -3 unit }
  bind ${className} <5> { %W yview scroll 3 unit }
}

#
# Differences between id "current" and operation nearest.
#
#	set index [$w index current]
#	set index [$w nearest $x $y]
#
#	o Nearest gives you the closest entry.
#	o current is "" if
#	   1) the pointer isn't over an entry.
#	   2) the pointer is over a open/close button.
#	   3)
#

#
#  Edit mode assignments
#
#	ButtonPress-3   Enables/disables edit mode on entry.  Sets focus to
#			entry.
#
#  KeyPress
#
#	Left		Move insertion position to previous.
#	Right		Move insertion position to next.
#	Up		Move insertion position up one line.
#	Down		Move insertion position down one line.
#	Return		End edit mode.
#	Shift-Return	Line feed.
#	Home		Move to first position.
#	End		Move to last position.
#	ASCII char	Insert character left of insertion point.
#	Del		Delete character right of insertion point.
#	Delete		Delete character left of insertion point.
#	Ctrl-X		Cut
#	Ctrl-V		Copy
#	Ctrl-P		Paste
#
#  KeyRelease
#
#	ButtonPress-1	Start selection if in entry, otherwise clear selection.
#	B1-Motion	Extend/reduce selection.
#	ButtonRelease-1 End selection if in entry, otherwise use last
#			selection.
#	B1-Enter	Disabled.
#	B1-Leave	Disabled.
#	ButtonPress-2	Same as above.
#	B2-Motion	Same as above.
#	ButtonRelease-2	Same as above.
#
#


# Standard Motif bindings:

bind ${className}Editor <ButtonPress-1> {
    %W icursor @%x,%y
    %W selection clear
}

bind ${className}Editor <Left> {
    %W icursor prev
    %W selection clear
}

bind ${className}Editor <Right> {
    %W icursor next
    %W selection clear
}

bind ${className}Editor <Shift-Left> {
    set new [expr {[%W index insert] - 1}]
    if {![%W selection present]} {
	%W selection from insert
	%W selection to $new
    } else {
	%W selection adjust $new
    }
    %W icursor $new
}

bind ${className}Editor <Shift-Right> {
    set new [expr {[%W index insert] + 1}]
    if {![%W selection present]} {
	%W selection from insert
	%W selection to $new
    } else {
	%W selection adjust $new
    }
    %W icursor $new
}

bind ${className}Editor <Home> {
    %W icursor 0
    %W selection clear
}
bind ${className}Editor <Shift-Home> {
    set new 0
    if {![%W selection present]} {
	%W selection from insert
	%W selection to $new
    } else {
	%W selection adjust $new
    }
    %W icursor $new
}
bind ${className}Editor <End> {
    %W icursor end
    %W selection clear
}
bind ${className}Editor <Shift-End> {
    set new end
    if {![%W selection present]} {
	%W selection from insert
	%W selection to $new
    } else {
	%W selection adjust $new
    }
    %W icursor $new
}

bind ${className}Editor <Delete> {
    if { [%W selection present]} {
	%W delete sel.first sel.last
    } else {
	%W delete insert
    }
}

bind ${className}Editor <BackSpace> {
    if { [%W selection present] } {
	%W delete sel.first sel.last
    } else {
	set index [expr [%W index insert] - 1]
	if { $index >= 0 } {
	    %W delete $index $index
	}
    }
}

bind ${className}Editor <Control-space> {
    %W selection from insert
}

bind ${className}Editor <Select> {
    %W selection from insert
}

bind ${className}Editor <Control-Shift-space> {
    %W selection adjust insert
}

bind ${className}Editor <Shift-Select> {
    %W selection adjust insert
}

bind ${className}Editor <Control-slash> {
    %W selection range 0 end
}

bind ${className}Editor <Control-backslash> {
    %W selection clear
}

bind ${className}Editor <KeyPress> {
    blt::tv::InsertText %W %A
}

# Ignore all Alt, Meta, and Control keypresses unless explicitly bound.
# Otherwise, if a widget binding for one of these is defined, the
# <KeyPress> class binding will also fire and insert the character,
# which is wrong.  Ditto for Escape, Return, and Tab.

bind ${className}Editor <Alt-KeyPress> {
    # nothing
}

bind ${className}Editor <Meta-KeyPress> {
    # nothing
}

bind ${className}Editor <Control-KeyPress> {
    # nothing
}

bind ${className}Editor <KeyRelease-Escape> {
    %W cancel
}

bind ${className}Editor <Return> {
    %W apply
}

bind ${className}Editor <Shift-Return> {
    blt::tv::InsertText %W "\n"
}

bind ${className}Editor <KP_Enter> {
    # nothing
}

bind ${className}Editor <Tab> {
    # nothing
}

if {![string compare $tcl_platform(platform) "macintosh"]} {
    bind ${className}Editor <Command-KeyPress> {
	# nothing
    }
}

# On Windows, paste is done using Shift-Insert.  Shift-Insert already
# generates the <<Paste>> event, so we don't need to do anything here.
if { [string compare $tcl_platform(platform) "windows"] != 0 } {
    bind ${className}Editor <Insert> {
	catch {blt::tv::InsertText %W [::tk::GetSelection %W CLIPBOARD]}
        #catch {blt::tv::InsertText %W [selection get -displayof %W]}
    }
}
bind ${className}Editor <<Paste>> {
    catch {blt::tv::InsertText %W [::tk::GetSelection %W CLIPBOARD]}
}

bind ${className}Editor <<Copy>> { ::blt::tv::TextCopy %W 1 }

bind ${className} <<Copy>> { ::blt::tv::TextCopy %W }


# Additional emacs-like bindings:
bind ${className}Editor <Double-1> {
    set parent [winfo parent %W]
    %W cancel
    after idle {
	blt::tv::EditColumn $parent %X %Y
    }
}
bind ${className}Editor <ButtonPress-3> {
    set parent [winfo parent %W]
    %W cancel
    after idle {
	blt::tv::EditColumn $parent %X %Y
    }
}

bind ${className}Editor <Control-a> {
    %W icursor 0
    %W selection clear
}

bind ${className}Editor <Control-b> {
    catch {%W icursor [expr {[%W index insert] - 1}]}
    %W selection clear
}

bind ${className}Editor <Control-d> {
    %W delete insert
}

bind ${className}Editor <Control-e> {
    %W icursor end
    %W selection clear
}

bind ${className}Editor <Control-f> {
    %W icursor [expr {[%W index insert] + 1}]
    %W selection clear
}

bind ${className}Editor <Control-h> {
    if {[%W selection present]} {
	%W delete sel.first sel.last
    } else {
	set index [expr [%W index insert] - 1]
	if { $index >= 0 } {
	    %W delete $index $index
	}
    }
}

bind ${className}Editor <Control-k> {
    %W delete insert end
}

if 0 {
    bind ${className}Editor <Control-t> {
	blt::tv::Transpose %W
    }
    bind ${className}Editor <Meta-b> {
	%W icursor [blt::tv::PreviousWord %W insert]
	%W selection clear
    }
    bind ${className}Editor <Meta-d> {
	%W delete insert [blt::tv::NextWord %W insert]
    }
    bind ${className}Editor <Meta-f> {
	%W icursor [blt::tv::NextWord %W insert]
	%W selection clear
    }
    bind ${className}Editor <Meta-BackSpace> {
	%W delete [blt::tv::PreviousWord %W insert] insert
    }
    bind ${className}Editor <Meta-Delete> {
	%W delete [blt::tv::PreviousWord %W insert] insert
    }
    # tkEntryNextWord -- Returns the index of the next word position
    # after a given position in the entry.  The next word is platform
    # dependent and may be either the next end-of-word position or the
    # next start-of-word position after the next end-of-word position.
    #
    # Arguments:
    # w -		The entry window in which the cursor is to move.
    # start -	Position at which to start search.

    if {![string compare $tcl_platform(platform) "windows"]}  {
	proc ::blt::tv::NextWord {w start} {
	    set pos [tcl_endOfWord [$w get] [$w index $start]]
	    if {$pos >= 0} {
		set pos [tcl_startOfNextWord [$w get] $pos]
	    }
	    if {$pos < 0} {
		return end
	    }
	    return $pos
	}
    } else {
	proc ::blt::tv::NextWord {w start} {
	    set pos [tcl_endOfWord [$w get] [$w index $start]]
	    if {$pos < 0} {
		return end
	    }
	    return $pos
	}
    }

    # PreviousWord --
    #
    # Returns the index of the previous word position before a given
    # position in the entry.
    #
    # Arguments:
    # w -		The entry window in which the cursor is to move.
    # start -	Position at which to start search.

    proc ::blt::tv::PreviousWord {w start} {
	set pos [tcl_startOfPreviousWord [$w get] [$w index $start]]
	if {$pos < 0} {
	    return 0
	}
	return $pos
    }

}

proc ::blt::tv::FmtStr {str {len 12}} {
    # Break a string into fixed size chunks.
    if {[string length $str]<=$len} {
        return $str
    }
    set lm [expr {$len-1}]
    set rc {}
    while {[string length $str]>0} {
        append rc [string range $str 0 $lm]
        set str [string range $str $len end]
        if {$str != {}} { append rc \n }
    }
    return $rc
}

proc ::blt::tv::FmtString {str {len 12} {class alnum}} {
    # Wrap long strings at word boundries.
    if {[string length $str]<=$len} {
        return $str
    }
    if {[string is $class $str]} {
        return [FmtStr $str $len]
    }
    set rc {}
    set crc {}
    set lw 1
    foreach i [split $str {}] {
        set isw [string is $class $i]
        if {(($isw && $lw) || (!$isw && !$lw)) && [string length $crc]<$len} {
            append crc $i
        } else {
            lappend rc $crc
            set crc $i
        }
        set lw $isw
    }
    if {$crc != {}} {
        lappend rc $crc
    }
    set src {}
    set cln {}
    foreach i $rc {
        if {[string length $cln$i]<=$len} {
            append cln $i
        } else {
            if {$src != {}} { append src \n }
            append src $cln
            set cln $i
        }
    }
    append src \n $cln
    return $src
}



