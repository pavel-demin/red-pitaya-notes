package require Tk
package require BLT

# vector and stripchart are blt components.
# if you have a vector v, you can update it in realtime with
# v set $list

# init the vectors to a fixed size.

set Hz 200

blt::vector create xvec($Hz) y1vec($Hz) y2vec($Hz) sx1($Hz) sy1($Hz)

# fill xvec with 0 .. $Hz-1

xvec seq 0 [expr {$Hz - 1}]
#xvec populate sx1 $Hz
sx1 seq 0 [expr {$Hz - 1}]

blt::stripchart .s1 -height 2i -width 8i -bufferelements no
blt::stripchart .s2 -height 2i -width 8i -bufferelements no

pack .s1 .s2

.s1 element create line1 -xdata xvec -ydata y1vec -symbol none
.s2 element create spline -x sx1 -y sy1 -symbol none -color red

#.s2 element create line2 -xdata xvec -ydata y2vec -symbol none -color red

# update $Hz values with random data once per second

proc proc1sec {} {

  # this can be done more concisely with vector random,
  # but if you need to fill a vector from scalar calculations,
  # do it this way:

  for {set i 0} {$i < $::Hz} {incr i} {
    lappend y1list [expr {rand()}]
    lappend y2list [expr {rand()}]
  }
  y1vec set $y1list
  y2vec set $y2list

  blt::spline natural xvec y1vec sx1 sy1

  after 200 proc1sec
}

proc1sec
