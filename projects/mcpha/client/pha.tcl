lappend auto_path [pwd]

package require TclOO
package require oo::util

wm minsize . 230 400

# -------------------------------------------------------------------------

namespace eval ::pha {

# -------------------------------------------------------------------------

  proc validate {min max size value} {
    if {[string equal $value {}]} {
      return 1
    } elseif {[string equal $value {-}]} {
      return 1
    } elseif {![regexp -- {^-?[0-9]*$} $value]} {
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
    if {[string equal $value {}]} {
      return 1
    } elseif {![regexp -- {^[0-9]{0,2}\.?[0-9]{0,3}$} $value]} {
      return 0
    } elseif {[regexp -- {^0[0-9]+$} $value]} {
      return 0
    } elseif {$value > $max} {
      return 0
    } else {
      return 1
    }
  }

# -------------------------------------------------------------------------

  proc addrvalidate {value} {
    set ipnum {\d|[1-9]\d|1\d\d|2[0-4]\d|25[0-5]}
    set rx {^(($ipnum)\.){0,3}($ipnum)?$}
    set rx [subst -nocommands -nobackslashes $rx]
    return [regexp -- $rx $value]
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

  oo::class create RecDisplay

# -------------------------------------------------------------------------

  oo::define RecDisplay constructor args {
    my variable master host port

    foreach {param value} $args {
      if {$param eq "-master"} {
        set master $value
      } else {
        error "unsupported parameter $param"
      }
    }

    set host 192.168.1.100
    set port 1002

    my setup
  }

# -------------------------------------------------------------------------

  oo::define RecDisplay method start {} {
    my variable master config after

    trace add variable [my varname rate] write [mymethod rate_update]

    trace add variable [my varname base] write [mymethod base_update]
    trace add variable [my varname thrs] write [mymethod thrs_update]

    for {set i 1} {$i <= 2} {incr i} {
      trace add variable [my varname base_val_${i}] write "[mymethod base_val_update] [my varname base_val_${i}]"
      trace add variable [my varname thrs_min_${i}] write "[mymethod thrs_val_update] [my varname thrs_min_${i}]"
      trace add variable [my varname thrs_max_${i}] write "[mymethod thrs_val_update] [my varname thrs_max_${i}]"

      ${config}.thrs_frame_${i}.min_field set 300
      ${config}.thrs_frame_${i}.max_field set 16300

      ${config}.base_field_${i} set 0
    }

    ${config}.rate_field set 4

    ${config}.thrs_check select

    ${config}.base_frame.mode_0 select

    set after {}
  }

# -------------------------------------------------------------------------

  oo::define RecDisplay method setup {} {
    my variable master config host port

    set config [frame ${master}.config -borderwidth 0]

    label ${config}.addr_label -text {IP address}
    entry ${config}.address_field -width 15 -textvariable [my varname host] \
      -validate all -vcmd {::pha::addrvalidate %P}

    frame ${config}.spc1 -height 10

    label ${config}.rate_label -text {decimation factor}
    spinbox ${config}.rate_field -from 4 -to 8192 \
      -increment 4 -width 10 -textvariable [my varname rate] \
      -validate all -vcmd {::pha::validate 0 8192 4 %P}
    frame ${config}.neg_frame -borderwidth 0
    checkbutton ${config}.neg_frame.neg_check_0 -text {negative IN1} -variable [my varname neg_0]
    checkbutton ${config}.neg_frame.neg_check_1 -text {negative IN2} -variable [my varname neg_1]

    frame ${config}.spc2 -height 10

    grid ${config}.neg_frame.neg_check_0 ${config}.neg_frame.neg_check_1

    frame ${config}.spc3 -height 10

    frame ${config}.base_frame -borderwidth 0

    label ${config}.base_frame.mode_label -text {baseline subtraction mode}
    radiobutton ${config}.base_frame.mode_0 -variable [my varname base] -text {manual} -value 0
    radiobutton ${config}.base_frame.mode_1 -variable [my varname base] -text {auto} -value 1
    grid ${config}.base_frame.mode_label -columnspan 2 -sticky w -padx 3
    grid ${config}.base_frame.mode_0 ${config}.base_frame.mode_1 -sticky w

    for {set i 1} {$i <= 2} {incr i} {

       label ${config}.base_label_${i} -text "baseline level ${i}"
      spinbox ${config}.base_field_${i} -from -16380 -to 16380 \
        -increment 5 -width 10 -textvariable [my varname base_val_${i}] \
        -validate all -vcmd {::pha::validate -16380 16380 6 %P}
    }

    frame ${config}.spc4 -height 10

    checkbutton ${config}.thrs_check -text {amplitude threshold} -variable [my varname thrs]

    for {set i 1} {$i <= 2} {incr i} {

      frame ${config}.thrs_frame_${i} -borderwidth 0

      label ${config}.thrs_frame_${i}.min_title -anchor w -text "min ${i}:"
      spinbox ${config}.thrs_frame_${i}.min_field -from 0 -to 16380 \
        -increment 5 -width 5 -textvariable [my varname thrs_min_${i}] \
        -validate all -vcmd {::pha::validate 0 16380 5 %P}
      frame ${config}.thrs_frame_${i}.spc1 -width 10
      label ${config}.thrs_frame_${i}.max_title -anchor w -text "max ${i}:"
      spinbox ${config}.thrs_frame_${i}.max_field -from 0 -to 16380 \
        -increment 5 -width 5 -textvariable [my varname thrs_max_${i}] \
        -validate all -vcmd {::pha::validate 0 16380 5 %P}
      grid ${config}.thrs_frame_${i}.min_title ${config}.thrs_frame_${i}.min_field \
        ${config}.thrs_frame_${i}.spc1 ${config}.thrs_frame_${i}.max_title \
        ${config}.thrs_frame_${i}.max_field
      grid columnconfigure ${config}.thrs_frame_${i} 3 -weight 1
    }

    frame ${config}.spc5 -height 10

    button ${master}.connect -text Start \
      -bg lightgreen -activebackground lightgreen -command [mymethod connect]

    grid ${config}.addr_label -sticky w -padx 3
    grid ${config}.address_field  -sticky ew -padx 5
    grid ${config}.spc1
    grid ${config}.rate_label -sticky w -padx 3
    grid ${config}.rate_field -sticky ew -padx 5
    grid ${config}.spc2
    grid ${config}.neg_frame -sticky ew
    grid ${config}.spc3
    grid ${config}.base_frame -sticky ew
    grid ${config}.base_label_1 -sticky w -padx 3
    grid ${config}.base_field_1 -sticky ew -padx 5
    grid ${config}.base_label_2 -sticky w -padx 3
    grid ${config}.base_field_2 -sticky ew -padx 5
    grid ${config}.spc4
    grid ${config}.thrs_check -sticky w
    grid ${config}.thrs_frame_1 -sticky ew -padx 5
    grid ${config}.thrs_frame_2 -sticky ew -padx 5
    grid ${config}.spc5

    grid ${config}

    grid ${master}.connect -sticky ew -pady 3 -padx 5

    grid ${master} -row 0 -column 0

    grid rowconfigure ${master} 0 -weight 1
    grid columnconfigure ${master} 0 -weight 1

  }

# -------------------------------------------------------------------------

  oo::define RecDisplay method config_disable {path} {
    catch {$path configure -state disabled}
    foreach child [winfo children $path] {
      my config_disable $child
    }
  }

# -------------------------------------------------------------------------

  oo::define RecDisplay method config_enable {path} {
    catch {$path configure -state normal}
    foreach child [winfo children $path] {
      my config_enable $child
    }
  }

# -------------------------------------------------------------------------

  oo::define RecDisplay method connect {} {
    my variable master config host port socket after

    set ipnum {\d|[1-9]\d|1\d\d|2[0-4]\d|25[0-5]}
    set rx {^(($ipnum)\.){3}($ipnum)$}
    set rx [subst -nocommands -nobackslashes $rx]

    if {![regexp -- $rx $host]} {
      tk_messageBox -icon error -message "IP address is incomplete"
      return
    }

    my config_disable ${config}
    ${master}.connect configure -state disabled

    set socket [socket -async $host $port]
    set after [after 5000 [mymethod display_error]]
    fileevent $socket writable [mymethod connected]
  }

# -------------------------------------------------------------------------

  oo::define RecDisplay method display_error {} {
    my variable master config host port socket after

    set answer [tk_messageBox -icon error -type retrycancel \
      -message "Cannot connect to $host:$port"]

    catch {close $socket}

    if {[string equal $answer retry]} {
      my connect
      return
    }

    my config_enable ${config}
    ${master}.connect configure -state active
  }

# -------------------------------------------------------------------------

  oo::define RecDisplay method connected {} {
    my variable master socket output after

    after cancel $after
    fileevent $socket writable {}

    set types {
      {{Data Files}       {.dat}}
      {{All Files}        *     }
    }

    set result [fconfigure $socket -error]
    if {[string equal $result {}]} {

      set stamp [clock format [clock seconds] -format {%Y%m%d_%H%M%S}]
      set fname pulses_${stamp}.dat

      set fname [tk_getSaveFile -filetypes $types -initialfile $fname]
      if {[string equal $fname {}]} {
        my disconnect
        return
      }

      if {[catch {open $fname w+} output]} {
        tk_messageBox -icon error \
          -message "An error occurred while opening \"$fname\""
        my disconnect
        return
      }

      fconfigure $output -translation binary -encoding binary
      fconfigure $socket -translation binary -encoding binary

      my pha_setup

      my command 10 0

      fcopy $socket $output -command [mymethod disconnect]

      ${master}.connect configure -state active -text Stop \
        -bg yellow -activebackground yellow -command [mymethod disconnect]
    } else {
      my display_error
    }
  }

# -------------------------------------------------------------------------

  oo::define RecDisplay method disconnect {args} {
    my variable master config socket output

    catch {close $socket}
    catch {close $output}

    my config_enable ${config}
    ${master}.connect configure -state active -text Start \
      -bg lightgreen -activebackground lightgreen -command [mymethod connect]
  }

# -------------------------------------------------------------------------

  oo::define RecDisplay method command {code chan {data 0}} {
    my variable socket

    if {[eof $socket]} {
      my disconnect
      return
    }

    set buffer [format {%01x} $code][format {%01x} $chan][string range [format {%016lx} $data] 2 15]

    if {[catch {
      puts -nonewline $socket [binary decode hex [::pha::reverse $buffer]]
      flush $socket
    } result]} {
      my disconnect
      return
    }
  }

# -------------------------------------------------------------------------

  oo::define RecDisplay method pha_setup {} {
    my variable rate neg_0 neg_1 base base_val_1 base_val_2
    my variable thrs thrs_min_1 thrs_max_1 thrs_min_2 thrs_max_2

    my command 0 0
    my command 1 0

    my command 2 0 $rate

    my command 3 0 $neg_0
    my command 3 1 $neg_1

    my command 4 0 $base

    my command 5 0 $base_val_1
    my command 5 1 $base_val_2

    my command 6 0 100
    my command 6 1 100

    switch -- $thrs {
      1 {
        set min_1 $thrs_min_1
        set max_1 $thrs_max_1
        set min_2 $thrs_min_2
        set max_2 $thrs_max_2
      }
      0 {
        set min_1 0
        set max_1 16380
        set min_2 0
        set max_2 16380
      }
    }

    my command 7 0 $min_1
    my command 8 0 $max_1

    my command 7 1 $min_2
    my command 8 1 $max_2

    my command 9 0 0xffffffffffffff
    my command 9 1 0xffffffffffffff
  }

# -------------------------------------------------------------------------

  oo::define RecDisplay method rate_update args {
    my variable rate

    if {[string equal $rate {}]} {
      set rate 4
    } elseif {$rate < 4} {
      set rate 4
    }
  }

# -------------------------------------------------------------------------

  oo::define RecDisplay method base_update args {
    my variable config base

    switch -- $base {
      1 {
        ${config}.base_field_1 configure -state disabled
        ${config}.base_field_2 configure -state disabled
      }
      0 {
        ${config}.base_field_1 configure -state normal
        ${config}.base_field_2 configure -state normal
      }
    }
  }

# -------------------------------------------------------------------------

  oo::define RecDisplay method base_val_update {name args} {
    if {[string equal [set $name] {}]} {
      set $name 0
    }
  }

# -------------------------------------------------------------------------

  oo::define RecDisplay method thrs_update args {
    my variable config thrs

    switch -- $thrs {
      1 {
        ${config}.thrs_frame_1.min_field configure -state normal
        ${config}.thrs_frame_1.max_field configure -state normal
        ${config}.thrs_frame_2.min_field configure -state normal
        ${config}.thrs_frame_2.max_field configure -state normal
      }
      0 {
        ${config}.thrs_frame_1.min_field configure -state disabled
        ${config}.thrs_frame_1.max_field configure -state disabled
        ${config}.thrs_frame_2.min_field configure -state disabled
        ${config}.thrs_frame_2.max_field configure -state disabled
      }
    }
  }

# -------------------------------------------------------------------------

  oo::define RecDisplay method thrs_val_update {name args} {
    if {[string equal [set $name] {}]} {
      set $name 0
    }
  }

}

# -------------------------------------------------------------------------

set main [frame .main]

pha::RecDisplay create rec -master $main

pack $main

update

rec start
