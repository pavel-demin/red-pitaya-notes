package ifneeded tdom 0.8.3 [list tdom_load $dir]

proc tdom_load {dir} {
    load "" tdom
    source [file join $dir tdom.tcl]
    rename tdom_load {}
}
