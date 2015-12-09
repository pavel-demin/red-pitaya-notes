package require BLT

pack [blt::graph .g]

#  Proc passed as a callback to BLT to draw custom tick labels.
#
proc format_yAxis_tick {win value} {
    if {$value == [$win axis cget y -max]} {
        set yAxisHeightPixels [expr {abs (
            [$win axis transform y [$win axis cget y -max]] -
            [$win axis transform y [$win axis cget y -min]])}]

        set font [$win axis cget y -tickfont]

        set yAxisHeightLines  [expr {$yAxisHeightPixels /
            [font metrics $font -linespace]}]

        set v [string repeat "\n" [expr {$yAxisHeightLines / 2}]]
        set h [string repeat " "  [expr {
            [font measure $font "$::y_title           "] /
            [font measure $font " "]}]]
        return "$v$h$value$v$::y_title"
    } else {
        return $value
    }
}

#  The following global is used by the callback "format_yAxis_tick".
set y_title "Y axis title"

#  Configure the vertical axis.
.g axis configure y -min 0 -max 100 -majorticks     \
        {0 10 20 30 40 50 60 70 80 90 100}

#  Need to force the graph to be drawn before distances can be
#  measured.
update

.g axis configure y -command format_yAxis_tick
