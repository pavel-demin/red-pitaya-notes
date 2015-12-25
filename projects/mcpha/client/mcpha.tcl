package require TclOO
package require oo::util
package require BLT
load [file join [pwd] mcpha.so]

wm minsize . 880 660

image create bitmap leftarrow -data "
#define leftarrow_width 5\n
#define leftarrow_height 5\n
static unsigned char leftarrow_bits\[\] = {\n
0x10, 0x1C, 0x1F, 0x1C, 0x10};"
image create bitmap rightarrow -data "
#define rightarrow_width 5\n
#define rightarrow_height 5\n
static unsigned char rightarrow_bits\[\] = {\n
0x01, 0x07, 0x1F, 0x07, 0x01};"

# -------------------------------------------------------------------------

namespace eval ::mcpha {

# -------------------------------------------------------------------------

  proc validate {min max size value} {
    if {![regexp -- {^-?[0-9]*$} $value]} {
      return 0
    } elseif {[regexp -- {^0[0-9]+$} $value]} {
      return 0
    } elseif {$value < $min} {
      return 0
    } elseif {$value > $max} {
      return 0
    } elseif {[string length $value] > $size} {
      return 0
    } else {
      return 1
    }
  }

# -------------------------------------------------------------------------

  proc doublevalidate {max value} {
    if {![regexp -- {^[0-9]{0,2}\.?[0-9]{0,3}$} $value]} {
      return 0
    } elseif {[regexp -- {^0[0-9]+$} $value]} {
      return 0
    } elseif {$value > $max} {
      return 0
    } else {
      return 1
    }
  }

  proc addrvalidate {value} {
    set ipnum {\d|[1-9]\d|1\d\d|2[0-4]\d|25[0-5]}
    set rx {^(($ipnum)\.){0,3}($ipnum)?$}
    set rx [subst -nocommands -nobackslashes $rx]
    return [regexp -- $rx $value]
  }

# -------------------------------------------------------------------------

  proc legendLabel {master row key title} {
    label ${master}.${key}_label -anchor w -text ${title}
    label ${master}.${key}_value -width 10 -anchor e -text {}

    grid ${master}.${key}_label -row ${row} -column 1 -sticky w
    grid ${master}.${key}_value -row ${row} -column 2 -sticky ew
  }

# -------------------------------------------------------------------------

  proc legendButton {master row key title var bg {fg black}} {
    checkbutton ${master}.${key}_check -variable $var
    label ${master}.${key}_label -anchor w -text ${title} -bg ${bg} -fg $fg
    label ${master}.${key}_value -width 10 -anchor e -text {} -bg ${bg} -fg $fg

    grid ${master}.${key}_check -row ${row} -column 0 -sticky w
    grid ${master}.${key}_label -row ${row} -column 1 -sticky w
    grid ${master}.${key}_value -row ${row} -column 2 -sticky ew
  }

# -------------------------------------------------------------------------
  proc reverse {str} {
    set res {}
    set i [string length $str]
    while {$i > 0} {
      set last [incr i -1]
      set first [incr i -1]
      append res [string range $str $first $last]
    }
    set res
  }

# -------------------------------------------------------------------------

  oo::class create CfgDisplay

# -------------------------------------------------------------------------

  oo::define CfgDisplay constructor args {
    my variable master connected host port

    foreach {param value} $args {
      if {$param eq "-master"} {
        set master $value
      } else {
        error "unsupported parameter $param"
      }
    }

    set connected false
    set host 192.168.1.100
    set port 1001

    my setup
  }

# -------------------------------------------------------------------------

  oo::define CfgDisplay method start {} {
    my variable master

    trace add variable [my varname rate] write [mymethod rate_update]

    ${master}.rate_4 select
  }

# -------------------------------------------------------------------------

  oo::define CfgDisplay method setup {} {
    my variable master host port

    label ${master}.addr_label -text {IP address:}
    entry ${master}.address_field -width 15 -textvariable [my varname host] \
      -validate all -vcmd {::mcpha::addrvalidate %P}
    button ${master}.connect -text Connect \
      -bg lightgreen -activebackground lightgreen -command [mymethod connect]

    frame ${master}.spc1 -width 10

    label ${master}.rate_label -text {Sample rate, MSPS:}
    radiobutton ${master}.rate_0 -variable [my varname rate] -text 0.5 -value 0
    radiobutton ${master}.rate_1 -variable [my varname rate] -text 1.25 -value 1
    radiobutton ${master}.rate_2 -variable [my varname rate] -text 2.5 -value 2
    radiobutton ${master}.rate_3 -variable [my varname rate] -text 6.25 -value 3
    radiobutton ${master}.rate_4 -variable [my varname rate] -text 12.5 -value 4

    grid ${master}.addr_label ${master}.address_field ${master}.connect \
      ${master}.spc1 ${master}.rate_label ${master}.rate_0 ${master}.rate_1 \
      ${master}.rate_2 ${master}.rate_3 ${master}.rate_4 -padx 5
  }

# -------------------------------------------------------------------------

  oo::define CfgDisplay method connect {} {
    my variable master connected host port socket after

    puts connect

    if {$connected} return

    set ipnum {\d|[1-9]\d|1\d\d|2[0-4]\d|25[0-5]}
    set rx {^(($ipnum)\.){3}($ipnum)$}
    set rx [subst -nocommands -nobackslashes $rx]

    if {![regexp -- $rx $host]} {
      tk_messageBox -icon error -message "IP address is incomplete"
      return
    }

    ${master}.address_field configure -state disabled
    ${master}.connect configure -state disabled

    set socket [socket -async $host $port]
    set after [after 5000 [mymethod timeout]]
    fileevent $socket writable [mymethod connected]
  }

# -------------------------------------------------------------------------

  oo::define CfgDisplay method timeout {} {
    my variable master connected host port socket after

    set answer [tk_messageBox -icon error -type retrycancel \
      -message "Cannot connect to $host:$port"]

    catch {close $socket}

    if {[string equal $answer retry]} {
      my connect
      return
    }

    ${master}.address_field configure -state normal
    ${master}.connect configure -state active
  }

# -------------------------------------------------------------------------

  oo::define CfgDisplay method connected {} {
    my variable master connected socket after

    after cancel $after
    fileevent $socket writable {}
    fconfigure $socket -translation binary -encoding binary
    set connected true
    ${master}.connect configure -state active -text Disconnect \
      -bg yellow -activebackground yellow -command [mymethod disconnect]
  }

# -------------------------------------------------------------------------

  oo::define CfgDisplay method disconnect {} {
    my variable master connected socket

    set connected false
    catch {close $socket}

    ${master}.address_field configure -state normal
    ${master}.connect configure -state active -text Connect \
      -bg lightgreen -activebackground lightgreen -command [mymethod connect]
  }

# -------------------------------------------------------------------------

  oo::define CfgDisplay method command {code chan {data 0}} {
    my variable connected socket
    if {!$connected} return

    set buffer [format {%02x} $code][format {%01x} $chan][format {%013x} $data]

    set rc [catch {
      puts -nonewline $socket [binary decode hex [::mcpha::reverse $buffer]]
      flush $socket
    } result]

    switch -- $rc {
      1 {
        puts $result
      }
    }
  }

# -------------------------------------------------------------------------

  oo::define CfgDisplay method commandReadRaw {code chan size data} {
    my variable connected socket
    if {!$connected} return

    my command $code $chan

    set rc [catch {read $socket $size} result]

    switch -- $rc {
      0 {
        set $data $result
      }
      1 {
        puts $result
      }
    }
  }

# -------------------------------------------------------------------------

  oo::define CfgDisplay method commandReadHex {code chan size data} {
    my variable connected socket
    if {!$connected} return

    my command $code $chan

    set rc [catch {read $socket $size} result]

    switch -- $rc {
      0 {
        set $data 0x[::mcpha::reverse [binary encode hex $result]]
      }
      1 {
        puts $result
      }
    }
  }

# -------------------------------------------------------------------------

  oo::define CfgDisplay method rate_update args {
    my variable rate

    my command 4 0 $rate
  }

# -------------------------------------------------------------------------

  oo::class create HstDisplay

# -------------------------------------------------------------------------

  oo::define HstDisplay constructor args {
    my variable number master controller
    my variable data

    foreach {param value} $args {
      if {$param eq "-number"} {
        set number $value
      } elseif {$param eq "-master"} {
        set master $value
      } elseif {$param eq "-controller"} {
        set controller $value
      } else {
        error "unsupported parameter $param"
      }
    }

    set data {}

    blt::vector create [my varname xvec](16384)
    blt::vector create [my varname yvec](16384)

    # fill one vector for the x axis with 16384 points
    [my varname xvec] seq -0.5 16383.5

    my setup
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method start {} {
    my variable config after
    my variable xmin_val xmax_val
    my variable yvec_bak yvec_old
    my variable rate_val date_val
    my variable cntr_val cntr_bak cntr_old

    trace add variable [my varname data] write [mymethod data_update]
    trace add variable [my varname cntr_val] write [mymethod cntr_val_update]
    trace add variable [my varname rate_val] write [mymethod rate_val_update]

    trace add variable [my varname axis] write [mymethod axis_update]
    trace add variable [my varname thrs] write [mymethod thrs_update]
    trace add variable [my varname thrs_val] write [mymethod thrs_update]

    ${config}.axis_check select

    ${config}.thrs_check select
    ${config}.thrs_field set 25

    set xmin_val 0
    set xmax_val 16383

    trace add variable [my varname xmin_val] write [mymethod xmin_val_update]
    trace add variable [my varname xmax_val] write [mymethod xmax_val_update]

    my stat_update

    set cntr_tmp 7500000000
    set cntr_val $cntr_tmp
    set cntr_bak $cntr_tmp
    set cntr_old $cntr_tmp
    set yvec_bak 0.0
    set yvec_old 0.0

    set rate_val(inst) 0.0
    set rate_val(mean) 0.0

    set date_val(start) {}
    set date_val(stop) {}
    set after {}

    ${config}.chan_frame.entr_value configure -text 0.0

    ${config}.chan_frame.axisy_value configure -text 0.0
    ${config}.chan_frame.axisx_value configure -text 0.0

    ${config}.stat_frame.tot_value configure -text 0.0
    ${config}.stat_frame.bkg_value configure -text 0.0

#    my cntr_reset
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method setup {} {
    my variable number master
    my variable xvec yvec graph
    my variable config thrs thrs_val
    my variable cntr_h cntr_m cntr_s

    # create a graph widget and show a grid
    set graph [blt::graph ${master}.graph -height 250 -leftmargin 80]
    $graph crosshairs configure -hide no -linewidth 1 -color darkblue -dashes {2 2}
    $graph grid configure -hide no
    $graph legend configure -hide yes

    $graph marker create line -name xmin -coords "0 -Inf 0 Inf" -linewidth 2 -outline red
    $graph marker create line -name xmax -coords "16383 -Inf 16383 Inf" -linewidth 2 -outline red
    $graph marker bind xmin <Enter> [list [self] marker_enter xmin]
    $graph marker bind xmin <Leave> [list [self] marker_leave xmin]
    $graph marker bind xmax <Enter> [list [self] marker_enter xmax]
    $graph marker bind xmax <Leave> [list [self] marker_leave xmax]

    set config [frame ${master}.config -width 170]

    checkbutton ${config}.axis_check -text {log scale} -variable [my varname axis]

    frame ${config}.spc1 -width 170 -height 20

    frame ${config}.rate_frame -borderwidth 0 -width 170
    mcpha::legendLabel ${config}.rate_frame 0 inst {Inst. rate, 1/s}
    mcpha::legendLabel ${config}.rate_frame 1 mean {Avg. rate, 1/s}

    frame ${config}.spc2 -width 170 -height 10

    frame ${config}.chan_frame -borderwidth 0 -width 170
    mcpha::legendLabel ${config}.chan_frame 0 entr  {Total entries}
    frame ${config}.chan_frame.spc1 -height 10
    grid ${config}.chan_frame.spc1 -row 1
    mcpha::legendLabel ${config}.chan_frame 2 axisy {Bin entries}
    mcpha::legendLabel ${config}.chan_frame 3 axisx {Bin number}

    frame ${config}.spc3 -width 170 -height 10

    label ${config}.roi -text {Region of interest}
    frame ${config}.roi_frame -borderwidth 0 -width 170
    label ${config}.roi_frame.min_title -anchor w -text {start:}
    label ${config}.roi_frame.min_value -width 5 -anchor e -text {}
    label ${config}.roi_frame.spc1 -width 5 -anchor w -text {}
    label ${config}.roi_frame.max_title -anchor w -text {end:}
    label ${config}.roi_frame.max_value -width 5 -anchor e -text {}

    grid ${config}.roi_frame.min_title ${config}.roi_frame.min_value \
      ${config}.roi_frame.spc1 ${config}.roi_frame.max_title \
      ${config}.roi_frame.max_value

    frame ${config}.stat_frame -borderwidth 0 -width 17

    mcpha::legendLabel ${config}.stat_frame 0 tot {total entries}
    mcpha::legendLabel ${config}.stat_frame 1 bkg {bkg entries}

    frame ${config}.spc4 -width 170 -height 20

    checkbutton ${config}.thrs_check -text {amplitude threshold} -variable [my varname thrs]
    spinbox ${config}.thrs_field -from 0 -to 16380 \
      -increment 5 -width 10 -textvariable [my varname thrs_val] \
      -validate all -vcmd {::mcpha::validate 0 16380 5 %P}

    frame ${config}.spc5 -width 170 -height 20

    label ${config}.cntr -text {time of exposure}
    frame ${config}.cntr_frame -borderwidth 0 -width 170

    label ${config}.cntr_frame.h -width 3 -anchor w -text {h}
    entry ${config}.cntr_frame.h_field -width 3 -textvariable [my varname cntr_h] \
      -validate all -vcmd {::mcpha::validate 0 999 3 %P}
    label ${config}.cntr_frame.m -width 3 -anchor w -text {m}
    entry ${config}.cntr_frame.m_field -width 3 -textvariable [my varname cntr_m] \
      -validate all -vcmd {::mcpha::validate 0 59 2 %P}
    label ${config}.cntr_frame.s -width 3 -anchor w -text {s}
    entry ${config}.cntr_frame.s_field -width 6 -textvariable [my varname cntr_s] \
      -validate all -vcmd {::mcpha::doublevalidate 59.999 %P}

    grid ${config}.cntr_frame.h_field ${config}.cntr_frame.h \
      ${config}.cntr_frame.m_field ${config}.cntr_frame.m ${config}.cntr_frame.s_field ${config}.cntr_frame.s

    frame ${config}.spc6 -width 170 -height 20

    button ${config}.start -text Start \
      -bg yellow -activebackground yellow -command [mymethod cntr_start]
    button ${config}.reset -text Reset \
      -bg red -activebackground red -command [mymethod cntr_reset]

    frame ${config}.spc7 -width 170 -height 20

    button ${config}.register -text Register \
      -bg lightblue -activebackground lightblue -command [mymethod register]

    frame ${config}.spc8 -width 170 -height 20

    button ${config}.recover -text {Read file} \
      -bg lightblue -activebackground lightblue -command [mymethod recover]

    grid ${config}.axis_check -sticky w
    grid ${config}.spc1
    grid ${config}.rate_frame -sticky ew -padx 5
    grid ${config}.spc2
    grid ${config}.chan_frame -sticky ew -padx 5
    grid ${config}.spc3
    grid ${config}.roi -sticky w -pady 1 -padx 5
    grid ${config}.roi_frame -sticky ew -padx 5
    grid ${config}.stat_frame -sticky ew -padx 5
    grid ${config}.spc4
    grid ${config}.thrs_check -sticky w
    grid ${config}.thrs_field -sticky ew -pady 1 -padx 5
    grid ${config}.spc5
    grid ${config}.cntr -sticky w -pady 1 -padx 3
    grid ${config}.cntr_frame -sticky ew -padx 5
    grid ${config}.spc6
    grid ${config}.start -sticky ew -pady 3 -padx 5
    grid ${config}.reset -sticky ew -pady 3 -padx 5
    grid ${config}.spc7
    grid ${config}.register -sticky ew -pady 3 -padx 5
    grid ${config}.spc8
    grid ${config}.recover -sticky ew -pady 3 -padx 5

    grid ${graph} -row 0 -column 0 -sticky news
    grid ${config} -row 0 -column 1

    grid rowconfigure ${master} 0 -weight 1
    grid columnconfigure ${master} 0 -weight 1
    grid columnconfigure ${master} 1 -weight 0 -minsize 80

    grid columnconfigure ${config}.rate_frame 1 -weight 1
    grid columnconfigure ${config}.chan_frame 1 -weight 1
    grid columnconfigure ${config}.stat_frame 1 -weight 1

    my crosshairs $graph

#    bind .graph <Motion> {%W crosshairs configure -position @%x,%y}

    # create one element with data for the x and y axis, no dots
    $graph element create element1 -color blue -linewidth 2 -symbol none -smooth step -xdata [my varname xvec] -ydata [my varname yvec]
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method marker_enter {marker} {
    my variable config graph
    $graph configure -cursor hand2
    $graph crosshairs off
    blt::RemoveBindTag $graph zoom-$graph
    $graph marker bind $marker <ButtonPress-1> [list [self] marker_press $marker]
    $graph marker bind $marker <ButtonRelease-1> [list [self] marker_release $marker]
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method marker_leave {marker} {
    my variable config graph
    $graph configure -cursor crosshair
    $graph crosshairs on
    blt::AddBindTag $graph zoom-$graph
    $graph marker bind $marker <ButtonPress-1> {}
    $graph marker bind $marker <ButtonRelease-1> {}
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method marker_press {marker} {
    my variable config graph
    $graph marker bind $marker <Motion> [list [self] ${marker}_motion %W %x %y]
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method marker_release {marker} {
    my variable config graph
    $graph marker bind $marker <Motion> {}
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method xmin_motion {W x y} {
    my variable config graph xmin_val
    set index [$graph axis invtransform x $x]
    set index [::tcl::mathfunc::round $index]
    if {$index < 0} {
      set index 0
    }
    set xmin_val $index
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method xmax_motion {W x y} {
    my variable config graph xmax_val
    set index [$graph axis invtransform x $x]
    set index [::tcl::mathfunc::round $index]
    if {$index > 16383} {
      set index 16383
    }
    set xmax_val $index
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method coor_update {W x y} {
    my variable config graph

    $W crosshairs configure -position @${x},${y}

    set index [$W axis invtransform x $x]
    set index [::tcl::mathfunc::round $index]
    catch {
      ${config}.chan_frame.axisy_value configure -text [[my varname yvec] index $index]
      ${config}.chan_frame.axisx_value configure -text ${index}.0
    }
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method crosshairs {graph} {
    set method [mymethod coor_update]
    bind $graph <Motion> [list [self] coor_update %W %x %y]
    bind $graph <Leave> {
      %W crosshairs off
    }
    bind $graph <Enter> {
      %W crosshairs on
    }
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method axis_update args {
    my variable axis graph
    $graph axis configure x -min 0 -max 16384
    Blt_ZoomStack $graph
    if {$axis} {
      $graph axis configure y -min 1 -max 1E10 -logscale yes
    } else {
      $graph axis configure y -min {} -max {} -logscale no
    }
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method thrs_update args {
    my variable controller config number thrs thrs_val

    if {[string equal $thrs_val {}]} {
      set thrs_val 0
    }

    if {$thrs} {
      ${config}.thrs_field configure -state normal
      set value $thrs_val
    } else {
      ${config}.thrs_field configure -state disabled
      set value 0
    }

    $controller command 6 $number $value
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method stat_update {} {
    my variable config graph xmin_val xmax_val
    set ymin_val [[my varname yvec] index $xmin_val]
    set ymax_val [[my varname yvec] index $xmax_val]

    ${config}.roi_frame.min_value configure -text $xmin_val
    ${config}.roi_frame.max_value configure -text $xmax_val

    ${config}.stat_frame.tot_value configure \
      -text [mcpha::integrateBlt [my varname yvec] $xmin_val $xmax_val 0]

    ${config}.stat_frame.bkg_value configure \
      -text [expr {($xmax_val - $xmin_val + 1) * ($ymin_val + $ymax_val) / 2.0}]
 }
# -------------------------------------------------------------------------

  oo::define HstDisplay method xmin_val_update args {
    my variable config graph xmin_val xmax_val
    if {$xmin_val > 16283} {
      set xmin_val 16283
    }
    if {$xmin_val > $xmax_val - 100} {
      set xmax_val [expr {$xmin_val + 100}]
    }
    $graph marker configure xmin -coords "$xmin_val -Inf $xmin_val Inf"
    my stat_update
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method xmax_val_update args {
    my variable config graph xmin_val xmax_val
    if {$xmax_val < 100} {
      set xmax_val 100
    }
    if {$xmax_val < $xmin_val + 100} {
      set xmin_val [expr {$xmax_val - 100}]
    }
    $graph marker configure xmax -coords "$xmax_val -Inf $xmax_val Inf"
    my stat_update
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method rate_val_update {name key op} {
    my variable config rate_val

    ${config}.rate_frame.${key}_value configure -text [format {%.2e} $rate_val(${key})]
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method cntr_val_update args {
    my variable cntr_val cntr_h cntr_m cntr_s

    set cntr_tmp [expr {${cntr_val}/125000}]
    set cntr_h [expr {${cntr_tmp}/3600000}]
    set cntr_m [expr {${cntr_tmp}%3600000/60000}]
    set cntr_s [expr {${cntr_tmp}%3600000%60000/1000.0}]
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method cntr_setup {} {
    my variable controller number cntr_val

    # send counter value
    $controller command 8 $number $cntr_val
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method cntr_reset {} {
    my variable controller config number
    my variable cntr_val cntr_bak cntr_old yvec_bak yvec_old rate_val date_val

    my cntr_stop

    $controller command 2 $number

    set cntr_val $cntr_bak
    my cntr_setup

    set cntr_old $cntr_bak
    set yvec_bak 0.0
    set yvec_old 0.0

    set rate_val(inst) 0.0
    set rate_val(mean) 0.0
    ${config}.chan_frame.entr_value configure -text 0.0

    set date_val(start) {}
    set date_val(stop) {}

    my acquire

    my cntr_ready
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method cntr_ready {} {
    my variable config cntr_val cntr_bak

    set cntr_val $cntr_bak

    ${config}.start configure -text Start -command [mymethod cntr_start]
    ${config}.reset configure -state active

    ${config}.cntr_frame.h_field configure -state normal
    ${config}.cntr_frame.m_field configure -state normal
    ${config}.cntr_frame.s_field configure -state normal
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method cntr_start {} {
    my variable config
    my variable cntr_h cntr_m cntr_s
    my variable cntr_val cntr_bak cntr_old yvec_bak yvec_old date_val

    set h $cntr_h
    set m $cntr_m
    set s $cntr_s

    if {[string equal $h {}]} {
      set h 0
    }
    if {[string equal $m {}]} {
      set m 0
    }
    if {[string equal $s {}]} {
      set s 0
    }
    if {[string equal $date_val(start) {}]} {
      set date_val(start) [clock format [clock seconds] -format {%d/%m/%Y %H:%M:%S}]
    }

    set cntr_tmp [expr {${h}*3600000 + ${m}*60000 + ${s}*1000}]
    set cntr_tmp [expr {entier(125000 * ${cntr_tmp})}]

    if {$cntr_tmp > 0} {
      ${config}.cntr_frame.h_field configure -state disabled
      ${config}.cntr_frame.m_field configure -state disabled
      ${config}.cntr_frame.s_field configure -state disabled

      set cntr_val $cntr_tmp
      set cntr_bak $cntr_tmp
      set cntr_old $cntr_tmp
      set yvec_bak [mcpha::integrateBlt [my varname yvec] 0 16383 0]
      set yvec_old $yvec_bak

      my cntr_setup

      my cntr_resume
    }
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method cntr_pause {} {
    my variable config

    my cntr_stop

    ${config}.start configure -text Resume -command [mymethod cntr_resume]
#    ${config}.reset configure -state active
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method cntr_resume {} {
    my variable controller config number auto after

    ${config}.start configure -text Pause -command [mymethod cntr_pause]
#    ${config}.reset configure -state disabled

    $controller command 9 $number

    set auto 1

    set after [after 100 [mymethod acquire_loop]]
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method cntr_stop {} {
    my variable controller config number auto after date_val

    set date_val(stop) [clock format [clock seconds] -format {%d/%m/%Y %H:%M:%S}]

    $controller command 10 $number

    set auto 0

    after cancel $after
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method data_update args {
    my variable data
    mcpha::convertBlt $data 4 [my varname yvec]
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method acquire_loop {} {
    my variable cntr_val auto after

    my acquire

    if {$cntr_val == 0} {
      my cntr_stop
      my cntr_ready
    } elseif {$auto} {
      set after [after 1000 [mymethod acquire_loop]]
    }
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method acquire {} {
    my variable controller config number
    my variable cntr_val cntr_bak cntr_old yvec_bak yvec_old rate_val

    set size 16384

    $controller commandReadRaw 12 $number [expr {$size * 4}] [my varname data]
    set yvec_new [mcpha::integrateBlt [my varname yvec] 0 16383 0]

    $controller commandReadHex 11 $number 8 [my varname cntr_val]
    set cntr_new $cntr_val

    if {$cntr_new < $cntr_old} {
      set rate_val(inst) [expr {($yvec_new - $yvec_old)*125000000/($cntr_old - $cntr_new)}]
      set rate_val(mean) [expr {($yvec_new - $yvec_bak)*125000000/($cntr_bak - $cntr_new)}]
      ${config}.chan_frame.entr_value configure -text $yvec_new
      my stat_update

      set yvec_old $yvec_new
      set cntr_old $cntr_new
    }
 }

# -------------------------------------------------------------------------

  oo::define HstDisplay method save_data {data} {
    my variable number
    my variable yvec_old rate_val date_val

    set types {
      {{Data Files}       {.dat}}
      {{All Files}        *     }
    }

    set stamp [clock format [clock seconds] -format {%Y%m%d_%H%M%S}]
    set fname spectrum_[expr {$number + 1}]_${stamp}.dat

    set fname [tk_getSaveFile -filetypes $types -initialfile $fname]
    if {[string equal $fname {}]} {
      return
    }

    set x [catch {
      set fid [open $fname w+]
      puts $fid "info {"
      puts $fid "start date: $date_val(start)"
      if {[string equal $date_val(stop) {}]} {
          puts $fid "stop date: [clock format [clock seconds] -format {%d/%m/%Y %H:%M:%S}]"
      } else {
          puts $fid "stop date: $date_val(stop)"
      }
      puts $fid "average rate: [format {%.2e} $rate_val(mean)] counts/s"
      puts $fid "total counts: $yvec_old"
      puts $fid "}"
      puts $fid "data {"
      puts $fid $data
      puts $fid "}"
      close $fid
    }]

    if { $x || ![file exists $fname] || ![file isfile $fname] || ![file readable $fname] } {
      tk_messageBox -icon error \
        -message "An error occurred while writing to \"$fname\""
    } else {
      tk_messageBox -icon info \
        -message "File \"$fname\" written successfully"
    }
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method open_data {} {
    set types {
      {{Data Files}       {.dat}}
      {{All Files}        *     }
    }

    set fname [tk_getOpenFile -filetypes $types]
    if {[string equal $fname {}]} {
      return
    }

    set x [catch {
      set fid [open $fname r+]
      set content [read $fid 131072]
      set yvec_new [split [dict get $content data] \n]
      close $fid
    }]

    if { $x || ![file exists $fname] || ![file isfile $fname] || ![file readable $fname] } {
      tk_messageBox -icon error \
        -message "An error occurred while reading \"$fname\""
    } else {
      tk_messageBox -icon info \
        -message "File \"$fname\" read successfully"
      my cntr_reset
      [my varname yvec] set $yvec_new
    }
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method register {} {
    my save_data [join [[my varname yvec] range 0 16383] \n]
  }

# -------------------------------------------------------------------------

  oo::define HstDisplay method recover {} {
    my variable config
    my open_data
    ${config}.chan_frame.entr_value configure -text [mcpha::integrateBlt [my varname yvec] 0 16383 0]
    my stat_update
  }

# -------------------------------------------------------------------------

  oo::class create OscDisplay

# -------------------------------------------------------------------------

  oo::define OscDisplay constructor args {
    my variable master controller
    my variable sequence data xvec yvec

    foreach {param value} $args {
      if {$param eq "-master"} {
        set master $value
      } elseif {$param eq "-controller"} {
        set controller $value
      } else {
        error "unsupported parameter $param"
      }
    }

    set data {}

    set sequence 0

    set xvec [blt::vector create #auto(10000)]

    for {set i 1} {$i <= 9} {incr i} {
      dict set yvec $i [blt::vector create #auto(10000)]
    }

    # fill one vector for the x axis
    $xvec seq 0 10000

    my setup
  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method start {} {
    my variable config
    my variable recs_val directory

    set directory $::env(HOME)
    set recs_val 100

    trace add variable [my varname chan] write [mymethod chan_update]

    trace add variable [my varname data] write [mymethod data_update]

    trace add variable [my varname auto] write [mymethod auto_update]

    trace add variable [my varname thrs] write [mymethod thrs_update 0]
    trace add variable [my varname thrs_val] write [mymethod thrs_update 0]

    trace add variable [my varname recs_val] write [mymethod recs_val_update]

    trace add variable [my varname last] write [mymethod last_update]

    for {set i 1} {$i <= 2} {incr i} {
      ${config}.chan_frame.chan${i}_check select
      ${config}.chan_frame.chan${i}_value configure -text 0.0
    }
    ${config}.chan_frame.axisx_value configure -text 0.0

    ${config}.thrs_check select
    ${config}.thrs_field set 100
  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method setup {} {
    my variable master
    my variable xvec yvec graph
    my variable config

    # create a graph widget and show a grid
    set graph [blt::graph ${master}.graph -height 250 -leftmargin 80]
    $graph crosshairs configure -hide no -linewidth 1 -color darkblue -dashes {2 2}
    $graph grid configure -hide no
    $graph legend configure -hide yes
    $graph axis configure x -min 0 -max 10000
    $graph axis configure y -min -8200 -max 8200

#    scale ${master}.last -orient horizontal -from 1 -to 27 -tickinterval 0 -showvalue no -variable [my varname last]

    set config [frame ${master}.config -width 170]

    frame ${config}.chan_frame -width 170
    mcpha::legendButton ${config}.chan_frame 0 chan1 {Channel 1} [my varname chan(1)] turquoise3
    mcpha::legendButton ${config}.chan_frame 1 chan2 {Channel 2} [my varname chan(2)] SpringGreen3
    mcpha::legendLabel  ${config}.chan_frame 6 axisx {Time axis}

    frame ${config}.spc1 -width 170 -height 30

    checkbutton ${config}.auto_check -text {auto update} -variable [my varname auto]

    frame ${config}.spc2 -width 170 -height 30

    checkbutton ${config}.thrs_check -text threshold -variable [my varname thrs]
    spinbox ${config}.thrs_field -from -8190 -to 8190 \
      -increment 5 -width 10 -textvariable [my varname thrs_val] \
      -validate all -vcmd {::mcpha::validate -8190 8190 5 %P}

    frame ${config}.spc3 -width 170 -height 30

    button ${config}.acquire -text Acquire \
      -bg lightgreen -activebackground lightgreen -command [mymethod acquire_start]
    button ${config}.register -text Register \
      -bg lightblue -activebackground lightblue -command [mymethod register]

    frame ${config}.spc4 -width 170 -height 30

    label ${config}.recs -text {number of records}
    spinbox ${config}.recs_field -from 0 -to 10000 \
      -increment 10 -width 10 -textvariable [my varname recs_val] \
      -validate all -vcmd {::mcpha::validate 0 10000 5 %P}

    frame ${config}.spc5 -width 170 -height 10

    button ${config}.sequence -text {Start Recording} -command [mymethod sequence_start] \
      -bg yellow -activebackground yellow

    frame ${config}.spc6 -width 170 -height 30

    button ${config}.recover -text {Read file} \
      -bg lightblue -activebackground lightblue -command [mymethod recover]

    grid ${config}.chan_frame -sticky ew
    grid ${config}.spc1
    grid ${config}.auto_check -sticky w
    grid ${config}.spc2
    grid ${config}.thrs_check -sticky w
    grid ${config}.thrs_field -sticky ew -pady 1 -padx 5
    grid ${config}.spc3
    grid ${config}.acquire -sticky ew -pady 3 -padx 5
    grid ${config}.register -sticky ew -pady 3 -padx 5
    grid ${config}.spc4
    grid ${config}.recs -sticky w -pady 1 -padx 3
    grid ${config}.recs_field -sticky ew -pady 1 -padx 5
    grid ${config}.spc5
    grid ${config}.sequence -sticky ew -pady 3 -padx 5
    grid ${config}.spc6
    grid ${config}.recover -sticky ew -pady 3 -padx 5

    grid ${graph} -row 0 -column 0 -sticky news
    grid ${config} -row 0 -column 1

#    grid ${master}.last -row 1 -column 0 -columnspan 2 -sticky ew

    grid rowconfigure ${master} 0 -weight 1
    grid columnconfigure ${master} 0 -weight 1
    grid columnconfigure ${master} 1 -weight 0 -minsize 120

    grid columnconfigure ${config}.chan_frame 2 -weight 1

    # enable zooming
    Blt_ZoomStack $graph

    my crosshairs $graph

    # create one element with data for the x and y axis, no dots
    $graph pen create pen1 -color turquoise3 -linewidth 2 -symbol none
    $graph pen create pen2 -color SpringGreen3 -linewidth 2 -symbol none

    $graph element create element1 -pen pen1 -xdata $xvec -ydata [dict get $yvec 1]
    $graph element create element2 -pen pen2 -xdata $xvec -ydata [dict get $yvec 2]
  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method coor_update {W x y} {
    my variable xvec yvec graph
    my variable config

    $W crosshairs configure -position @${x},${y}

    set index [$W axis invtransform x $x]
    set index [::tcl::mathfunc::round $index]
    catch {
      ${config}.chan_frame.chan1_value configure -text [[dict get $yvec 1] index $index]
      ${config}.chan_frame.chan2_value configure -text [[dict get $yvec 2] index $index]
      ${config}.chan_frame.axisx_value configure -text ${index}.0
    }
  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method crosshairs {graph} {
    set method [mymethod coor_update]
    bind $graph <Motion> [list [self] coor_update %W %x %y]
    bind $graph <Leave> {
      %W crosshairs off
    }
    bind $graph <Enter> {
      %W crosshairs on
    }
  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method chan_update {name key op} {
    my variable config graph chan

    if {$chan(${key})} {
      $graph pen configure pen${key} -linewidth 2
    } else {
      $graph pen configure pen${key} -linewidth 0
    }
  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method recs_val_update args {
    my variable recs_val
    if {[string equal $recs_val {}]} {
      set recs_val 0
    }
  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method last_update args {
    my variable graph last

    set first [expr {$last - 1}]

    $graph axis configure x -min ${first}0000 -max ${last}0000
  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method thrs_update {reset args} {
    my variable controller config thrs thrs_val

    if {[string equal $thrs_val {}]} {
      set thrs_val 0
    }

    if {$thrs} {
      ${config}.thrs_field configure -state normal
      set value $thrs_val
    } else {
      ${config}.thrs_field configure -state disabled
      set value 0
    }

    $controller command 15 0 $value

    if {$reset} {
      $controller command 3 0
    }

  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method data_update args {
    my variable data yvec
    my variable graph chan waiting sequence auto

    mcpha::convertOsc $data $yvec

    foreach {key value} [array get chan] {
      $graph pen configure pen${key} -dashes 0
    }

    set waiting 0

    if {$sequence} {
      my sequence_register
    } elseif {$auto} {
      after 1000 [mymethod acquire_start]
    }
  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method acquire_start {} {
    my variable graph chan controller waiting

    foreach {key value} [array get chan] {
      $graph pen configure pen${key} -dashes dot
    }

    # restart
    my thrs_update 1

    set waiting 1

    $controller command 17 0 10000

    after 200 [mymethod acquire_loop]
  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method acquire_loop {} {
    my variable controller waiting

#    set size 262144
    set size 10000

    set status 0
    $controller commandReadHex 18 0 4 status
    puts $status

    if {$status == 0} {
      $controller commandReadRaw 19 0 [expr {$size * 4}] [my varname data]
    }

    if {$waiting} {
      after 200 [mymethod acquire_loop]
    }
  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method auto_update args {
    my variable config auto

    if {$auto} {
      ${config}.recs_field configure -state disabled
      ${config}.sequence configure -state disabled
      ${config}.acquire configure -state disabled
      ${config}.register configure -state disabled
      ${config}.recover configure -state disabled

      my acquire_start
    } else {
      ${config}.recs_field configure -state normal
      ${config}.sequence configure -state active
      ${config}.acquire configure -state active
      ${config}.register configure -state active
      ${config}.recover configure -state active
    }
  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method save_data {fname} {
    my variable data

    set fid [open $fname w+]
    fconfigure $fid -translation binary -encoding binary

#    puts -nonewline $fid [binary format "H*iH*" "1f8b0800" [clock seconds] "0003"]
#    puts -nonewline $fid [zlib deflate $data]
    puts -nonewline $fid $data
#    puts -nonewline $fid [binary format i [zlib crc32 $data]]
#    puts -nonewline $fid [binary format i [string length $data]]

    close $fid
  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method open_data {} {
    my variable data

    set types {
      {{Data Files}       {.dat}}
      {{All Files}        *     }
    }

    set fname [tk_getOpenFile -filetypes $types]
    if {[string equal $fname {}]} {
      return
    }

    set x [catch {
      set fid [open $fname r+]
      fconfigure $fid -translation binary -encoding binary
#      set size [file size $fname]
#      seek $fid 10
#      set data [zlib inflate [read $fid [expr {$size - 18}]]]
      set data [read $fid]
      close $fid
    }]

    if { $x || ![file exists $fname] || ![file isfile $fname] || ![file readable $fname] } {
      tk_messageBox -icon error \
        -message "An error occurred while reading \"$fname\""
    } else {
      tk_messageBox -icon info \
        -message "File \"$fname\" read successfully"
    }
  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method register {} {
    set types {
      {{Data Files}       {.dat}}
      {{All Files}        *     }
    }

    set stamp [clock format [clock seconds] -format {%Y%m%d_%H%M%S}]
    set fname oscillogram_${stamp}.dat

    set fname [tk_getSaveFile -filetypes $types -initialfile $fname]
    if {[string equal $fname {}]} {
      return
    }

    if {[catch {my save_data $fname} result]} {
      tk_messageBox -icon error \
        -message "An error occurred while writing to \"$fname\""
    } else {
      tk_messageBox -icon info \
        -message "File \"$fname\" written successfully"
    }
  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method recover {} {
    my open_data
  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method sequence_start {} {
    my variable config recs_val recs_bak directory counter sequence

    set counter 1
    if {$counter > $recs_val} {
      return
    }

    set directory [tk_chooseDirectory -initialdir $directory -title {Choose a directory}]

    if {[string equal $directory {}]} {
     return
    }

    ${config}.recs_field configure -state disabled
    ${config}.sequence configure -text {Stop Recording} -command [mymethod sequence_stop]
    ${config}.acquire configure -state disabled
    ${config}.register configure -state disabled
    ${config}.recover configure -state disabled

    set recs_bak $recs_val

    set sequence 1

    my acquire_start
  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method sequence_register {} {
    my variable config recs_val recs_bak directory counter

    set fname [file join $directory oscillogram_$counter.dat]

    my incr counter

    if {[catch {my save_data $fname} result]} {
      tk_messageBox -icon error \
        -message "An error occurred while writing to \"$fname\""
    } elseif {$counter <= $recs_bak} {
      set recs_val [expr {$recs_bak - $counter}]
      my acquire_start
      return
    }

    my sequence_stop
  }

# -------------------------------------------------------------------------

  oo::define OscDisplay method sequence_stop {} {
    my variable config recs_val recs_bak sequence

    set sequence 0

    set recs_val $recs_bak

    ${config}.recs_field configure -state normal
    ${config}.sequence configure -text {Start Recording} -command [mymethod sequence_start]
    ${config}.acquire configure -state active
    ${config}.register configure -state active
    ${config}.recover configure -state active
  }

# -------------------------------------------------------------------------

  namespace export HstDisplay
  namespace export OscDisplay
}

# -------------------------------------------------------------------------

set config [frame .config]

mcpha::CfgDisplay create cfg -master $config
set notebook [blt::tabnotebook .notebook -borderwidth 1 -selectforeground black -side bottom]

grid ${config} -row 0 -column 0 -sticky news -pady 5
grid ${notebook} -row 1 -column 0 -sticky news

grid rowconfigure . 1 -weight 1
grid columnconfigure . 0 -weight 1

set window [frame ${notebook}.hst_0]
$notebook insert end -text "Spectrum histogram 1" -window $window -fill both
mcpha::HstDisplay create hst_0 -number 0 -master $window -controller cfg

set window [frame ${notebook}.hst_1]
$notebook insert end -text "Spectrum histogram 2" -window $window -fill both
mcpha::HstDisplay create hst_1 -number 1 -master $window -controller cfg

set window [frame ${notebook}.osc]
$notebook insert end -text "Oscilloscope" -window $window -fill both
mcpha::OscDisplay create osc -master $window -controller cfg

update

cfg start

hst_0 start

hst_1 start

osc start
