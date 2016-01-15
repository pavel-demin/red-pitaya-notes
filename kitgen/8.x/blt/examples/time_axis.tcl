package require BLT

pack [blt::graph .g -width 10i]

#  Proc passed as a callback to BLT to draw custom tick labels.
#
proc format_timeAxis_tick {win seconds} {
    set hour [clock format $seconds -format "%H"]
    regsub {^0*} $hour {} label
    if {[string equal $label {}]} {
        return "$label\n[string repeat { } $::nSpaces]\
                [clock format $seconds -format "%d/%m"]"
    } else {
        return $label
    }
}

#  Construct a list of major tick positions in seconds - the
#  month, year and the range of days can be varied to suit
#  the application.
#
for {set day 20} {$day <= 23} {incr day} {
    foreach hours {0 4 8 12 16 20} {
        lappend majorticks [clock scan "3/$day/2001 $hours:00"]
    }
}
lappend majorticks [clock scan "3/$day/2001 00:00"]

#  Create the graph.
.g axis configure x                            \
        -min          [lindex $majorticks 0]   \
        -max          [lindex $majorticks end] \
        -title        "Day"                    \
        -majorticks   $majorticks

#  Need to do an update to display the graph before the
#  distance can be measured.
update

#  Measure the width of a day on the graph - the example
#  dates need not be in the displayed range.
set dayFieldWidth [expr {
        [.g axis transform x [clock scan 3/2/2001]] -
        [.g axis transform x [clock scan 3/1/2001]]}]

#  Work out how many spaces this corresponds to in the
#  font for the tick labels.
set nSpaces [expr {$dayFieldWidth /
                   [font measure [.g axis cget x -tickfont] " "]}]

#  Configure the axis to use the custom label command.
.g axis configure x -command format_timeAxis_tick
