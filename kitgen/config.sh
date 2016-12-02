#set -x

args="$*"

verbose=0; case $1 in -v) verbose=1; shift ;; esac

root=`dirname $1`
base=`basename $1`
shift

case $root in .) root=8.4;; esac
path=$root/$base
  
if test ! -d $root
  then echo "error: directory '$root' does not exist"; exit 1; fi

for v in allenc allmsgs aqua b64 cli dyn gui ppc \
          gcov gprof sym thread tzdata univ x86
  do eval $v=0; done

while test $# != 0
  do eval $1=1; shift; done

#for v in thread allenc allmsgs tzdata cli dyn gui aqua x86 ppc univ
#  do eval val=$`echo $v`; echo $v = "$val"; done

make=$path/Makefile
mach=`uname`
plat=unix

echo "Configuring $make for $mach."
mkdir -p $path

case $cli-$dyn-$gui in 0-0-0) cli=1 dyn=1 gui=1 ;; esac

( echo "# Generated `date`:"
  echo "#   `basename $0` $args"
  echo
  
  case $mach in
  
    Darwin)
      case $aqua in
        1) echo "GUI_OPTS   = -framework Carbon -framework IOKit" ;;
        *) echo "GUI_OPTS   = -L/usr/X11R6/lib -lX11 -weak-lXss -lXext" ;;
      esac
      
      echo "LDFLAGS    = -framework CoreFoundation"
      echo "LDSTRIP    = -x"
      
      case $b64-$univ-$ppc-$x86 in
        0-0-0-0) ;;
        0-0-1-0) echo "CFLAGS    += -arch ppc" ;;
        0-0-0-1) echo "CFLAGS    += -arch x86" ;;
        0-?-?-?) echo "CFLAGS    += -arch ppc -arch i386" ;;
        1-0-1-0) echo "CFLAGS    += -arch ppc64" ;;
        1-0-0-1) echo "CFLAGS    += -arch x86_64" ;;
        1-?-?-?) echo "CFLAGS    += -arch ppc64 -arch x86_64" ;;
      esac
      echo "CFLAGS    += -isysroot /Developer/SDKs/MacOSX10.4u.sdk" \
                          "-mmacosx-version-min=10.4"
      
      case $aqua in 1)
        echo "TK_OPTS    = --enable-aqua"
        echo "TKDYN_OPTS = --enable-aqua" ;;
      esac
      ;;
      
    Linux)
      echo "LDFLAGS    = -ldl -lm"
      echo "GUI_OPTS   = -L/usr/X11R6/lib -lX11 -lXft -lXss -lfontconfig"
      case $b64 in 1)
        echo "CFLAGS     += -m64" ;; 
      esac
      ;;

    *BSD)
      echo "CFLAGS    += -I/usr/X11R6/include"
      echo "LDFLAGS    = -lm"
      echo "GUI_OPTS   = -L/usr/X11R6/lib -lX11 -lXss"
      case $b64 in 1)
        echo "CFLAGS     += -m64" ;; 
      esac
      ;;

    MINGW*)
      echo 'LDFLAGS    = -lws2_32 -lnetapi32 build/lib/dde1*/*tcldde1*.a build/lib/reg1*/*tclreg1*.a'
      echo 'GUI_OPTS   = -lgdi32 -lcomdlg32 -limm32 -lcomctl32 -lshell32'
      echo 'GUI_OPTS  += -lole32 -loleaut32 -luuid -lwinspool -luserenv'
      echo 'GUI_OPTS  += build/tk/wish.res.o -mwindows'
      echo 'CLIOBJ     = $(OBJ) $(OUTDIR)/tclAppInit.o $(OUTDIR)/tclkitsh.res.o'
      echo 'DYNOBJ     = $(CLIOBJ) $(OUTDIR)/tkdyn/wish.res.o'
      echo 'GUIOBJ     = $(OBJ) $(OUTDIR)/winMain.o $(OUTDIR)/tclkit.res.o'
      echo 'PRIV       = install-private-headers'
      echo 'EXE        = .exe'
      plat=win
      ;;

    SunOS)
      echo "LDFLAGS    = -ldl -lsocket -lnsl -lm"
      echo "GUI_OPTS   = -lX11 -lXext"
      ;;

    *) echo "warning: no settings known for '$mach'" >&2 ;;
  esac

  echo "PLAT       = $plat"
  case $plat in unix)
    echo "PRIV       = install-headers install-private-headers" ;;
  esac
  case $b64 in 1)
    echo "TCL_OPTS   += --enable-64bit" 
    echo "TK_OPTS    += --enable-64bit" 
    echo "VFS_OPTS   += --enable-64bit" 
    echo "VLERQ_OPTS += --enable-64bit" ;; 
  esac

  #case $verbose in 1) kitopts=" -d" ;; esac
  case $allenc  in 1) kitopts="$kitopts -e" ;; esac
  case $allmsgs in 1) kitopts="$kitopts -m" ;; esac
  case $tzdata  in 1) kitopts="$kitopts -z" ;; esac
  
  case $thread in
    1) case $mach in Linux|SunOS)
	       echo "LDFLAGS   += -lpthread" ;;
       esac
       echo "TCL_OPTS   = --enable-threads"
       echo "KIT_OPTS   = -t$kitopts" ;;
    0) echo "KIT_OPTS   =$kitopts" ;;
  esac
  
  case $tzdata in 1) echo "TCL_OPTS  += --with-tzdata" ;; esac

  case $gprof in 1) 
    echo "CFLAGS    += -pg"
    sym=1 ;; 
  esac

  case $gcov in 1) 
    echo "CFLAGS    += -fprofile-arcs -ftest-coverage -O0"
    echo "LDFLAGS   += -lgcov"
    sym=1 ;; 
  esac

  case $sym in 1)
    echo "STRIP      = :"
    echo
    echo "TCL_OPTS       += --enable-symbols"
    echo "THREADDYN_OPTS += --enable-symbols"
    echo "TK_OPTS        += --enable-symbols"
    echo "TKDYN_OPTS     += --enable-symbols"
    echo "VFS_OPTS       += --enable-symbols"
    echo "VLERQ_OPTS     += --enable-symbols"
    echo ;;
  esac
  
  case $cli in 1) targets="$targets tclkit-cli" ;; esac
  case $dyn in 1) targets="$targets tclkit-dyn" ;; esac
  case $gui in 1) targets="$targets tclkit-gui" ;; esac

  case $thread in
    1) echo "all: threaded$targets" ;;
    0) echo "all:$targets" ;;
  esac

  case $mach in MINGW*)
    echo
    echo "tclkit-cli: tclkit-cli.exe"
    echo "tclkit-dyn: tclkit-dyn.exe"
    echo "tclkit-gui: tclkit-gui.exe"
  esac
  
  echo
  echo "include ../../makefile.include"
  
) >$make

case $verbose in 1)
  echo
  echo "Contents of $make:"
  echo "======================================================================="
  cat $make
  echo "======================================================================="
  echo
  echo "To build, run these commands:"
  echo "    cd $path"
  echo "    make"
  echo
  echo "This produces the following executable(s):"
  case $cli in 1) echo "    $path/tclkit-cli   (command-line)" ;; esac
  case $dyn in 1) echo "    $path/tclkit-dyn   (Tk as shared lib)" ;; esac
  case $gui in 1) echo "    $path/tclkit-gui   (Tk linked statically)" ;; esac
  echo
  echo "To remove all intermediate builds, use 'make clean'."
  echo "To remove all executables as well, use 'make distclean'."
  echo
esac
