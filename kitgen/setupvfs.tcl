# setupvfs.tcl -- new tclkit-{cli,gui} generation bootstrap
#
# jcw, 2006-11-16

proc history {args} {} ;# since this runs so early, all debugging support helps

if {[lindex $argv 0] ne "-init-"} {
  puts stderr "setupvfs.tcl has to be run by kit-cli with the '-init-' flag"
  exit 1
}

set argv [lrange $argv 2 end] ;# strip off the leading "-init- setupvfs.tcl"

set debugOpt 0
set encOpt 0
set msgsOpt 0
set threadOpt 0
set tzOpt 0

while {1} {
  switch -- [lindex $argv 0] {
    -d { incr debugOpt }
    -e { incr encOpt }
    -m { incr msgsOpt }
    -t { incr threadOpt }
    -z { incr tzOpt }
    default { break }
  }
  set argv [lrange $argv 1 end]
}

if {[llength $argv] != 2} {
  puts stderr "Usage: [file tail [info nameofexe]] -init- [info script]\
    ?-d? ?-e? ?-m? ?-t? ?-z? destfile (cli|gui)
    -d    output some debugging info from this setup script
    -e    include all encodings i.s.o. 7 basic ones (encodings/)
    -m    include all localized message files (tcl 8.5, msgs/)
    -t    include the thread extension as shared lib in vfs
    -z    include timezone data files (tcl 8.5, tzdata/)"
  exit 1
}

set tcl_library ../tcl/library
source ../tcl/library/init.tcl ;# for tcl::CopyDirectory

package require platform

set platform [lindex [split [platform::generic] -] 0]

load {} vfs
load {} g2lite

# map of proper version numbers to replace @ markers in paths given to vfscopy
# this relies on having all necessary extensions already loaded at this point
set versmap [list tcl8@ tcl$tcl_version tk8@ tk$tcl_version \
                  vfs1@ vfs[package require vfs] \
                  g2lite0@ g2lite[package require g2lite]]

if {[string equal $platform win32]} {
  load {} registry
  lappend versmap registry1@ registry[package require registry]
}

if {$debugOpt} {
  puts "Starting [info script]"
  puts "     exe: [info nameofexe]"
  puts "    argv: $argv"
  puts "   tcltk: $tcl_version"
  puts "  loaded: [info loaded]"
  puts " versmap: $versmap"
  puts ""
}

# Create package index files for the static extensions.
set exts [list g2lite]
if {[string equal $platform win32]} {
  lappend exts registry
}
foreach ext $exts {
  load {} $ext
  set dst [file join lib "[string tolower $ext][package provide $ext]" pkgIndex.tcl]
  puts $dst
  set index($dst) "package ifneeded $ext [package provide $ext] {load {} [string tolower $ext]}"
}

set clifiles {
  boot.tcl
  config.tcl
  lib/tcl8@/auto.tcl
  lib/tcl8@/history.tcl
  lib/tcl8@/init.tcl
  lib/tcl8@/opt0.4
  lib/tcl8@/package.tcl
  lib/tcl8@/parray.tcl
  lib/tcl8@/safe.tcl
  lib/tcl8@/tclIndex
  lib/tcl8@/word.tcl
  lib/vfs1@/mk4vfs.tcl
  lib/vfs1@/pkgIndex.tcl
  lib/vfs1@/starkit.tcl
  lib/vfs1@/vfslib.tcl
  lib/vfs1@/vfsUtils.tcl
  lib/vfs1@/zipvfs.tcl
  lib/g2lite0@/pkgIndex.tcl
  lib/tcllib1.19/pkgIndex.tcl
  lib/tcllib1.19/asn
  lib/tcllib1.19/base64
  lib/tcllib1.19/comm
  lib/tcllib1.19/cmdline
  lib/tcllib1.19/csv
  lib/tcllib1.19/fileutil
  lib/tcllib1.19/ldap
  lib/tcllib1.19/log
  lib/tcllib1.19/math
  lib/tcllib1.19/ooutil
  lib/tcllib1.19/snit
  lib/tcllib1.19/struct
  lib/tcllib1.19/uri
}

if {[string equal $platform win32]} {
  lappend clifiles lib/registry1@/pkgIndex.tcl
}

set guifiles {
  tclkit.ico
  lib/tk8@/bgerror.tcl
  lib/tk8@/button.tcl
  lib/tk8@/choosedir.tcl
  lib/tk8@/clrpick.tcl
  lib/tk8@/comdlg.tcl
  lib/tk8@/console.tcl
  lib/tk8@/dialog.tcl
  lib/tk8@/entry.tcl
  lib/tk8@/focus.tcl
  lib/tk8@/iconlist.tcl
  lib/tk8@/icons.tcl
  lib/tk8@/listbox.tcl
  lib/tk8@/megawidget.tcl
  lib/tk8@/menu.tcl
  lib/tk8@/mkpsenc.tcl
  lib/tk8@/msgbox.tcl
  lib/tk8@/msgs
  lib/tk8@/obsolete.tcl
  lib/tk8@/optMenu.tcl
  lib/tk8@/palette.tcl
  lib/tk8@/panedwindow.tcl
  lib/tk8@/pkgIndex.tcl
  lib/tk8@/safetk.tcl
  lib/tk8@/scale.tcl
  lib/tk8@/scrlbar.tcl
  lib/tk8@/spinbox.tcl
  lib/tk8@/tclIndex
  lib/tk8@/tearoff.tcl
  lib/tk8@/text.tcl
  lib/tk8@/tk.tcl
  lib/tk8@/tkfbox.tcl
  lib/tk8@/unsupported.tcl
  lib/tk8@/xmfbox.tcl
  lib/BLT2.5/pkgIndex.tcl
  lib/BLT2.5/graph.tcl
  lib/BLT2.5/tabnotebook.tcl
  lib/BLT2.5/treeview.cur
  lib/BLT2.5/treeview.xbm
  lib/BLT2.5/treeview.tcl
  lib/BLT2.5/treeview_m.xbm
  lib/BLT2.5/bltCanvEps.pro
  lib/BLT2.5/bltGraph.pro
}

if {$encOpt} {
  lappend clifiles lib/tcl8@/encoding
} else {
  lappend clifiles lib/tcl8@/encoding/ascii.enc \
                   lib/tcl8@/encoding/cp1251.enc \
                   lib/tcl8@/encoding/cp1252.enc \
                   lib/tcl8@/encoding/iso8859-1.enc \
                   lib/tcl8@/encoding/iso8859-2.enc \
                   lib/tcl8@/encoding/iso8859-15.enc \
                   lib/tcl8@/encoding/koi8-r.enc \
                   lib/tcl8@/encoding/macRoman.enc
}

if {$threadOpt} {
  lappend clifiles lib/[glob -tails -dir build/lib thread2*]
}

if {$tcl_version eq "8.4"} {
  lappend clifiles lib/tcl8@/http2.5 \
            	   lib/tcl8@/ldAout.tcl \
            	   lib/tcl8@/msgcat1.3 \
            	   lib/tcl8@/tcltest2.2
} else {
  lappend clifiles lib/tcl8 \
                   lib/tcl8@/clock.tcl \
                   lib/tcl8@/tm.tcl

  lappend guifiles lib/tk8@/ttk

  if {$msgsOpt} {
    lappend clifiles lib/tcl8@/msgs
  }
  if {$tzOpt} {
    lappend clifiles lib/tcl8@/tzdata
  }
}

# look for a/b/c in three places:
#   1) build/files/b-c
#   2) build/files/a/b/c
#   3) build/a/b/c

proc timet_to_dos {time_t} {
    set s [clock format $time_t -format {%Y %m %e %k %M %S}]
    scan $s {%d %d %d %d %d %d} year month day hour min sec
    expr {(($year-1980) << 25) | ($month << 21) | ($day << 16)
          | ($hour << 11) | ($min << 5) | ($sec >> 1)}
}

proc walk {path} {
    set result {}
    set files [glob -nocomplain -types f -directory $path *]
    foreach file $files {
        set excluded 0
        foreach glob $excludes {
            if {[string match $glob $file]} {
                set excluded 1
                break
            }
        }
        if {!$excluded} {lappend result $file}
    }
    foreach dir [glob -nocomplain -types d -directory $path $match] {
        set subdir [walk $dir $excludes $match]
        if {[llength $subdir]>0} {
            set result [concat $result $dir $subdir]
        }
    }
    return $result
}

proc mkzipfile {zipchan dst {comment {}}} {
  global index

  set mtime [timet_to_dos [clock seconds]]
  set utfpath [encoding convertto utf-8 $dst]
  set utfcomment [encoding convertto utf-8 $comment]
  set flags [expr {(1<<10)}] ;# use utf-8
  set method 0               ;# store 0, deflate 8
  set attr 0                 ;# text or binary (default binary)
  set extra ""
  set crc 0
  set csize 0
  set version 20

  if {[info exists index($dst)]} {
    set data $index($dst)
  } else {
    set a [file split $dst]
    set src build/files/[lindex $a end-1]-[lindex $a end]
    if {[file exists $src]} {
      if {$::debugOpt} {
        puts "  $src  ==>  $dst"
      }
    } else {
      set src build/files/$dst
      if {[file exists $src]} {
        if {$::debugOpt} {
          puts "  $src  ==>  $dst"
        }
      } else {
        set src build/$dst
      }
    }
    if {[file isfile $src]} {
      set mtime [timet_to_dos [file mtime $src]]
      set fin [open $src r]
      fconfigure $fin -translation binary -encoding binary
      set data [read $fin]
      close $fin
    } else {
      error "cannot find $src"
    }
  }

  set attrex 0x81b60000  ;# 0o100666 (-rw-rw-rw-)
  if {[file extension $dst] eq ".tcl"} {
    set attr 1         ;# text
  }

  set size [string length $data]
  set crc [zlib crc32 $data]
  set cdata [zlib deflate $data]
  if {[string length $cdata] < $size} {
    set method 8
    set data $cdata
  }
  set csize [string length $data]


  set local [binary format a4sssiiiiss PK\03\04 \
    $version $flags $method $mtime $crc $csize $size \
    [string length $utfpath] [string length $extra]]
  append local $utfpath $extra

  set offset [tell $zipchan]
  puts -nonewline $zipchan $local
  puts -nonewline $zipchan $data

  set hdr [binary format a4ssssiiiisssssii PK\01\02 0x0317 \
    $version $flags $method $mtime $crc $csize $size \
    [string length $utfpath] [string length $extra]\
    [string length $utfcomment] 0 $attr $attrex $offset]
  append hdr $utfpath $extra $utfcomment

  return $hdr
}

# copy file to vfs
proc vfscopy {zf argv} {
  global versmap count cd
  
  foreach f $argv {
    set dst [string map $versmap $f]

    if {[file isdirectory build/$dst]} {
      vfscopy $zf [glob -nocomplain -tails -directory build $dst/*]
      continue
    }

    append cd [mkzipfile $zf $dst]
    incr count
  }
}

set zf [open [lindex $argv 0] a]
fconfigure $zf -translation binary -encoding binary

set count 0
set cd ""

switch [lindex $argv 1] {
  cli {
    vfscopy $zf $clifiles
  }
  gui {
    vfscopy $zf $clifiles
    vfscopy $zf $guifiles
  }
  default {
    puts stderr "Unknown type, must be cli or gui"
    exit 1
  }
}

set cdoffset [tell $zf]
set endrec [binary format a4ssssiis PK\05\06 0 0 \
    $count $count [string length $cd] $cdoffset 0]
puts -nonewline $zf $cd
puts -nonewline $zf $endrec
close $zf

if {$debugOpt} {
  puts "\nDone with [info script]"
}
