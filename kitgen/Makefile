TCLLIB_URL  = http://github.com/tcltk/tcllib/archive/tcllib_1_16.tar.gz
ZLIB_URL    = http://heanet.dl.sourceforge.net/project/libpng/zlib/1.2.3/zlib-1.2.3.tar.gz
TKTABLE_URL = http://heanet.dl.sourceforge.net/project/tktable/tktable/2.10/Tktable2.10.tar.gz
SQLITE_URL  = https://www.sqlite.org/2015/sqlite-autoconf-3090200.tar.gz
TCL_URL     = http://heanet.dl.sourceforge.net/project/tcl/Tcl/8.6.4/tcl8.6.4-src.tar.gz
TK_URL      = http://heanet.dl.sourceforge.net/project/tcl/Tcl/8.6.4/tk8.6.4-src.tar.gz

VFS_CVS     = :pserver:anonymous@tclvfs.cvs.sourceforge.net:/cvsroot/tclvfs

unspecified-target:

cvs:
	mkdir -p 8.x && cd 8.x && \
	  cvs -d $(VFS_CVS) co tclvfs
tars:
	mkdir -p 8.6 && cd 8.6 && \
	  curl -LO $(TCL_URL) && \
	  curl -LO $(TK_URL)
	mkdir -p 8.x && cd 8.x && \
	  curl -LO $(TCLLIB_URL) && \
	  curl -LO $(ZLIB_URL) && \
	  curl -LO $(TKTABLE_URL) && \
	  curl -LO $(SQLITE_URL)

untar:
	mkdir -p 8.6 && cd 8.6 && \
	  tar xfz tcl8.6.4-src.tar.gz && \
	  tar xfz tk8.6.4-src.tar.gz && \
	  mv tcl8.6.4 tcl && \
	  mv tk8.6.4 tk
	mkdir -p 8.x && cd 8.x && \
	  tar xfz tcllib_1_16.tar.gz && \
	  tar xfz zlib-1.2.3.tar.gz && \
	  tar xfz Tktable2.10.tar.gz && \
	  tar xfz sqlite-autoconf-3090200.tar.gz && \
	  mv tcllib-tcllib_1_16 tcllib && \
	  mv zlib-1.2.3 zlib && \
	  mv Tktable2.10 Tktable && \
	  mv sqlite-autoconf-3090200 sqlite

configs:
	sh config.sh 8.6/base-std
	sh config.sh 8.6/kit-small gui

small: configs
	cd 8.6/kit-small && $(MAKE)

base tidy:
	for i in 8*/base-*/Makefile; do (cd `dirname $$i`; $(MAKE) $@); done

all clean distclean tclkit-gui:
	for i in 8*/kit-*/Makefile; do (cd `dirname $$i`; $(MAKE) $@); done

.PHONY: all base tidy clean distclean small tars configs
