# scroll_canvas.tcl --
#     Experiment with scrolling canvas items selectively
#

canvas .c -width 400 -height 200
scrollbar .y -command scrollc

grid .c .y -sticky news

.c create text 200 10 -text "Title"
.c lower [.c create rectangle 0 0 400 30 -fill white -outline {} ]

.c lower [.c create text 10 50 -text "Item" -anchor w -tag Move]
.c lower [.c create polygon 80 50 140 50 140 350 -fill green -tag Move]

# Set the parameters for the scrollbar
.y set 0.0 0.5
set currentPos 0.0

proc scrollc {operation number {unit ""}} {
    # Ignore scroll operation
    if { $operation == "moveto" } {
	set dely [expr {400*($::currentPos-$number)}]
	set ::currentPos $number
	.c move Move 0 $dely
    }
}
