package ifneeded BLT 2.5 [list BLT_load $dir]

proc BLT_load {dir} {
    global blt_library
    load "" BLT
    source [file join $dir graph.tcl]
    set blt_library $dir
    rename BLT_load {}
}
