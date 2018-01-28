#
# Copyright (C) 2002 by Lucent Technologies
#

#
# top-level Makefile for expmake of UNITY
#

# the beginning of this Makefile is the part that varies per-package
PKG = unity
PKGBALL = $(PKG).cpio.gz
SRCDIR = unity nunity contrib-un2html
OUTDIR = ins
WORLDWIDE=-worldwide 
VERSION=2.7

# Build for all types
TYPES = `machines gnutypes|xargs|tr ' ' ','`
#TYPES = ilinux
# PORTTYPE is the machine type from which portable files are taken
PORTTYPE = sparc-sun-solaris
# TYPE and ANSICC are overridden on remote machine
TYPE = unspecified
ANSICC = cc

# LOCAL_PREFIX is only used when building for a local installation.  It
# must be supplied by the user on the make command line.
LOCAL_PREFIX = unspecified

# PREFIX specifies what prefix is used when building the tools
# (which is also directory where tools will be found when they are actually
# installed under exptools).  This defaults to the /opt/exp directory, but
# is overridden by the "local" target if the tool is being built for
# local installation.
PREFIX = unspecified

# INSTALL_PREFIX and INSTALL_EXEC_PREFIX specify where portable and 
# non-portable files are installed.  They default to directories under
# the remote build node, but are overridden by the "local" target if the
# tool is being built for local installation
INSTALL_PREFIX = unspecified
INSTALL_EXEC_PREFIX = unspecified

MAKE_ARGS = PKG=$(PKG) SRCDIR="$(SRCDIR)" ANSICC="$(ANSICC)" TYPE=$(TYPE)

default: help

# rule to configure, make, and "install"
# portable files should be installed into $(INSTALL_PREFIX) and
# non-portable files should be installed into $(INSTALL_EXEC_PREFIX)
# in a mirror of where they will ultimately go under /opt/exp.  For
# remote builds via expmake, $(INSTALL_PREFIX) will be set to
# $(OUTDIR)/share and $(INSTALL_EXEC_PREFIX) will be $(OUTDIR)/$(TYPE).
# When building for a local installation, both will be set to 
# the user supplied value for $(LOCAL_PREFIX).
# (Also note that hand-generated portable files, those not generated
# by this rule but also to be installed under /opt/exp, go under
# the "misc" directory on the source machine)

commonbuild:
	@set -ex; HERE=`pwd`; \
	export CC; \
	CC=$(ANSICC); \
	STYPE=`machines -ftype  gnutype=$(TYPE)| tr -d ' '`; \
	rm -f config.cache; \
	if [ $$STYPE = ilinux ] || [ $$STYPE = zlinux ] || [ $$STYPE = hppa ] || [ $$STYPE = cygwin ] || [ $$STYPE = darwin ] ; \
	then \
		sed -e 's/$$$$@.o/%: %.o/' unity/src/makefile > tmp.makefile; \
		mv tmp.makefile unity/src/makefile; \
	fi; \
	case $$STYPE in \
	darwin ) \
		CC="gcc2"; \
		INSTALLARGS="LIBPW= PW= PW1=regcmp.o PW2=regex.o"; \
		;; \
	cygwin ) \
		INSTALLARGS="EXE=.exe"; \
		;; \
	hppa ) \
		CC="/opt/exp/gnu/bin/gcc"; \
		PATH=/opt/exp/gnu/bin:$$PATH; \
		;; \
	ilinux | solaris | sgi | sparc ) \
		CC="/opt/exp/gnu/bin/gcc"; \
		;; \
	esac; \
	for DIR in $(SRCDIR); \
	do \
		cd $$DIR; \
		ksh umake -l $$STYPE; \
		make $$INSTALLARGS install; \
		cd ..; \
	done; \
	if [ ! -d $(INSTALL_EXEC_PREFIX) ] ; \
	then \
		mkdir -p $(INSTALL_EXEC_PREFIX); \
	fi; \
	if [ ! -d $(INSTALL_PREFIX) ] ; \
	then \
		mkdir -p $(INSTALL_PREFIX) ; \
	fi; \
	rm -rf $(INSTALL_EXEC_PREFIX)/lib; \
	rm -rf $(INSTALL_PREFIX)/lib; \
	mkdir -p $(INSTALL_EXEC_PREFIX)/lib/unity; \
	for DIR in $(SRCDIR); \
	do \
		cd $$DIR; \
		find bin lib hdr -print | cpio -pdvum $(INSTALL_EXEC_PREFIX)/lib/unity; \
		cd man;  \
		if [ $$DIR = unity ] ; then \
			for i in *.mp ; do \
				cp $$i `basename $$i .mp`.1m; \
			done; \
			MANSECTS="1"; \
		elif [ $$DIR = nunity ] ; then \
			MANSECTS="1 3 4"; \
		else \
			MANSECTS="1"; \
		fi; \
		for i in $$MANSECTS; \
		do \
			pwd; \
			MANDIR=$(INSTALL_PREFIX)/lib/unity/man/man$$i; \
			if [ ! -d $$MANDIR ] ; \
			then \
				mkdir -p $$MANDIR ; \
			fi; \
			for j in *.$${i}m ; do \
				cp $$j $$MANDIR/`basename $$j .$${i}m`.$${i}; \
			done; \
		done; \
		cd ../..;  \
	done; \
	if [ ! -d $(OUTDIR)/share/bin ] ; \
	then \
		mkdir -p $(OUTDIR)/share/bin ; \
		ln -s ../lib/unity/bin/unity $(OUTDIR)/share/bin; \
	fi; 

# the remainder of this file should pretty much be able to be common
#  to most packages

help:
	@echo "No default target; choose one of"
	@echo "   build, preview, provide, or clean"
	@echo "clean is made up of cleanlocal and cleanremote"

build: saveold prepareremote buildremote unpack

saveold:
	@set -ex; \
       COMMATYPES=$(TYPES); \
       for TYPE in `echo $$COMMATYPES|tr ',' ' '`; do \
         rm -rf $(OUTDIR)/$$TYPE.old; \
         if [ -d $(OUTDIR)/$$TYPE ]; then \
           mv $(OUTDIR)/$$TYPE $(OUTDIR)/$$TYPE.old; \
         fi; \
       done

prepareremote: cleanlocal
	@set -ex; \
       expmake $(WORLDWIDE) version="$(VERSION)" -prepareOnly updateParameters="targets $(PKGBALL)" $(PKG)

buildremote:
	@set -ex; \
       expmake $(WORLDWIDE) version="$(VERSION)" -skipPrepare -parallel executableTypes=$(TYPES) $(PKG)

unpack:
	@set -ex; \
       for file in $(PKGBALL).*; do \
         gzcat <$$file | cpio -ivdum; \
         rm -f $$file; \
       done

# this is the target on the remote machine
# $(PKGBALL) will be returned to the local machine by expmake
$(PKGBALL): precleanoutdir always
	@set -e; HERE=`pwd`; \
	make commonbuild $(MAKE_ARGS) INSTALL_PREFIX=$$HERE/$(OUTDIR)/share \
		INSTALL_EXEC_PREFIX=$$HERE/$(OUTDIR)/$(TYPE) \
		PREFIX=/opt/exp
	@set -e; DIRS="$(OUTDIR)/$(TYPE)"; \
	if [ "$(TYPE)" = "$(PORTTYPE)" ]; then \
	  DIRS="$$DIRS $(OUTDIR)/share"; \
	fi; \
	set -x; find $$DIRS -print | cpio -ocv | gzip >$(PKGBALL)
	rm -rf $(OUTDIR)

precleanoutdir:
	rm -rf $(OUTDIR)

private: always
	@if [ $(TYPE) = unspecified ] ; then \
		echo 'Machine type must be specified.  (Use TYPE=xxx)' ; \
		exit 1; \
	fi
	@if [ $(LOCAL_PREFIX) = unspecified ] ; then \
		echo 'Local installation prefix must be specified.';  \
		echo '(Use LOCAL_PREFIX=/xxx/yyy/zzz)' ; \
		exit 1 ; \
	fi
	make commonbuild $(MAKE_ARGS) INSTALL_PREFIX=$(LOCAL_PREFIX) \
		INSTALL_EXEC_PREFIX=$(LOCAL_PREFIX) \
		PREFIX=$(LOCAL_PREFIX) 

preview:
	@set -ex; expprovide $(WORLDWIDE) version="$(VERSION)" -preview -askreason executableTypes=$(TYPES) $(PKG)

provide: preview
	@set -ex; expprovide $(WORLDWIDE) version="$(VERSION)"  -skipPrepare $(PKG)


clean: cleanremote cleanlocal

cleanlocal:
	@set -ex; \
	for DIR in $(SRCDIR); \
	do \
		cd $$DIR; \
		if [ -f makefile ]; then \
			make clobber; \
			rm -rf bin lib; \
		fi; \
		cd ..; \
	done

cleanremote:
	@set -ex; \
       if [ -f $(PKG)src.nsb ]; then \
         expmake -skipPrepare -clean executableTypes=$(TYPES) $(PKG); \
         rm -f $(PKG)src.nsb; \
       fi

distclean: clean
	rm -rf expmakeout* $(PKGBALL)* $(OUTDIR) 

always:
