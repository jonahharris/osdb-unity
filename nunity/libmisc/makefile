#
# Below are the #defines recognized in the code:
#
# RETSIGTYPE	- A portability defintion.  Defines what signal catching
#		  functions return.  Normally this is set to "int".
#		  On some systems (3b2), however, it must be defined
#		  as "void".
#
SILENT = @		# don't echo commands

DEFS=
HDR=../hdr
INCLDIRS = -I$(HDR)
OPTFLAG = -O
CFLAGS = $(OPTFLAG) $(KFLAGS) $(INCLDIRS) $(DEFS)
LINT=lint
LINTFLAGS = -x -u
#
# If ranlib is needed on your machine, change the following macro
#
#RANLIB=ranlib
RANLIB = $(SILENT):

#
# Installation directory for library
#
DESTLIB= ../lib

#
# Source files for normal misc library.
#
# popen2.c is needed because of a bug in SysVR2 popen() when multiple
# popen() calls are pending.  The bug is fixed in later releases.
#
NORM_CSRC =		\
	basename.c	\
	chkperm.c	\
	cnvtbslsh.c	\
	cntsflds.c	\
	copyfp.c	\
	copysrc.c	\
	cpdirname.c	\
	cstrprt.c	\
	getdate.c	\
	nocasecmp.c	\
	ncregcmp.c	\
	popen2.c	\
	prmsg.c		\
	prog.c		\
	restring.c	\
	splitsflds.c

#
# Regular expression functions - normally provided as
# part of libPW on under UNIX SysV.  For other OS's
# we need to include them directly.
#
REGEX_CSRC =		\
	regex.c		\
	regcmp.c

#
#
# Below are the files that don't contribute directly to NUNITY.
# However, they might be useful functions in other applications.
# If they are needed, include them below in the MISCOBJS macro.
#
EXTRA_CSRC =		\
	cntflds.c	\
	copyfile.c	\
	copydest.c	\
	dirname.c	\
	expandvar.c	\
	exprange.c	\
	getcwd.c	\
	localmach.c	\
	oneline.c	\
	realuser.c	\
	shellcmd.c	\
	splitflds.c	\
	teefp.c

NORMOBJS = $(NORM_CSRC:.c=.o)
#REGEXOBJS = $(REGEX_CSRC:.c=.o)
REGEXOBJS =
#EXTRAOBJS = $(EXTRA_CSRC:.c=.o)
EXTRAOBJS = 

MISCOBJS = $(NORMOBJS) $(REGEXOBJS) $(EXTRAOBJS)

MISCLIB = libmisc.a

MAJOR_PRODUCTS = $(MISCLIB)

all : $(MAJOR_PRODUCTS)

install : all
	@if [ ! -d $(DESTLIB) ]; then mkdir $(DESTLIB); fi
	cd $(DESTLIB); rm -f $(MAJOR_PRODUCTS)
	ln $(MAJOR_PRODUCTS) $(DESTLIB)

$(MISCLIB): $(MISCOBJS)
	rm -f $@
	ar r $@ $(MISCOBJS)
	-$(RANLIB) $@

#
# re_unity is/can be used to compile regular expressions
# that need to be added to libunity/redef.c when new REs
# are added to libunity/regexpdef.re
#
# To make:
#		cd libmisc
#		../mkunity -s -mre_unity <machine>
#
re_unity:	regcmp.o restring.o re_unity.o
	cc -s -O -o re_unity re_unity.o regcmp.o restring.o

clean:
	rm -f *.o *.out *.ln core

clobber: clean
	rm -f $(MISCLIB) re_unity

lint: llib-lmisc.ln

llib-lmisc.ln: $(HEADERS) $(NORM_CSRC)
	$(LINT) $(LINTFLAGS) $(CFLAGS) -o misc $(NORM_CSRC)

chkperm.o: $(HDR)/permission.h
prmsg.o realuser.o usercmd.o: $(HDR)/message.h
