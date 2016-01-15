package require BLT

proc compute args {
  puts $::a
  puts $::b
  puts $::c
  puts $::d
  #puts [blt::vector expr {($::a * (::x ^ 3))  + ($::b * (::x ^ 2)) + ($::c * ::x) +  $::d}]
  puts [blt::vector expr {::x}]
}

label .l -text "Function: ax^3 + bx^2 + cx + d"
blt::table . .l 0,0

blt::graph .g -width 700 -height 600 -bd 2 -relief groove
blt::table . .g 1,0 -fill both
Blt_ZoomStack .g

blt::vector create ::x
::x seq -10 10 0.5
blt::vector create ::y

set items {label variable to from digits resolution tickinterval gridpos}
set scale_cfg {
  a ::a  -0.31  0.3 5 0.001 0.1   1,1
  b ::b  -1.01  1.0 5 0.002 0.5   1,2
  c ::c  -4.01  4.0 4 0.01  2.0   1,3
  d ::d -10.01 10.0 3 0.1 5.0     1,4
}

foreach $items  $scale_cfg {
  set $variable 1.0
  trace variable $variable w compute

  set w .sc$label
  set label [ string totitle $label ]
  scale $w -label $label \
    -variable $variable -to $to -from $from \
    -digits $digits -resolution $resolution \
    -tickinterval $tickinterval \
    -bd 2 -relief groove
  blt::table . $w $gridpos -fill y
}

# fill vector ::y
compute

.g element create Function -xdata ::x -ydata ::y -pixels 3
.g axis configure y -min -20 -max 20
.g grid configure -hide no -dashes { 2 2 }

# ready
