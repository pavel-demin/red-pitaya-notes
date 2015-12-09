package ifneeded Tk $::tcl_patchLevel \
  [string map [list @@ [file join $dir .. libtk$::tcl_version[info sharedlibext]]] {
    if {[lsearch -exact [info loaded] {{} Tk}] >= 0} {
      load "" Tk
    } else {
      load @@ Tk
    }
  }]
