#
# Copyright (C) 2002 by Lucent Technologies
#

#
#	@(#)makefile	1.24

#
# Place to install $(DBLIB), db.h, and val.h
#
DESTLIB=../lib

#
# Place to install executable programs
#
DESTBIN=../bin

#
# Programmers' Workbench library definitions.  For
# systems that have the Programmers' Workbench PW
# should be set to PW library (libPW.a) - normally "-lPW"
# and PW1 and PW2 should be null.  For systems w/o
# the Programmers' Workbench PW must be null and
# PW1 set to "regcmp.o" and PW2 set to "regex.o".
#
PW = -lPW
PW1 =
PW2 =
#PW=
#PW1=regcmp.o
#PW2=regex.o

#
# Main UNITY library.
#
DBLIB = dblib.a

.PRECIOUS : $(DBLIB)

#
# C compiler flags.
# For UNIX/370, set CFLAGS to CFLAGS= -O -b1,1
# For 3b2/3b5, set CFLAGS to  CFLAGS=-O -f
#
# Below are the defines recognized in the code:
#
# RETSIGTYPE -	Portability definition to get rid of warnings from
#		compiler about signal handler functions.  On most
#		systems expect signal handlers to return an int
#		(which is ignored).  On some machines (sun, 3b2)
#		signal handlers are expected to be defined as void.
#		If your machine expects a void, add "-DRETSIGTYPE=void"
#		to the DEFS macro.
#
# ADVLOCK -	Define to use UNIX Advisory lock with L<relation>
#		lock files.  Do NOT define ADVLOCK if TMPLOCK is
#		defined (used).
#
# TMPLOCK -	Define to create lock files in /tmp based on the
#		inode and device numbers of the relation.  Leave
#		undefined to get lock files "L<relation>" in the
#		same directory as the relation.
#		Locks in /tmp don't work for NFS-mounted relations.
#
DEFS=
OPTFLAG= -O
INCDIRS=		# include file directories
CFLAGS= $(OPTFLAG) $(KFLAGS) $(INCDIRS) $(DEFS)

#
# Strip command for getting rid of symbol table info.
#
STRIP=strip

#
# NORMPROGS is the list of normal C-programs.
# LNKPROGS is the list of C-programs linked to other C-programs.
# VALPROGS is the list of C-programs for validation.
# SHELLS is the list of UNITY shell commands.
# PROGS is the combination of everything.
#
NORMPROGS =		\
	alter		\
	asort		\
	catalog		\
	compute		\
	ddist		\
	dist		\
	ei		\
	et		\
	uindex		\
	index2		\
	insert		\
	load		\
	maketable	\
	merge		\
	nprintf		\
	outerjoin 	\
	packet		\
	regjoin		\
	setdiff		\
	stat		\
	table		\
	tuple		\
	uadmin		\
	ucheck		\
	udelete		\
	udelta		\
	udis		\
	uedit		\
	uenter		\
	uget		\
	uformat		\
	ujoin		\
	umenu		\
	unload		\
	unmerge		\
	uprint		\
	uprintf		\
	uselect

LNKPROGS =		\
	erase-index	\
	erase-table	\
	delete		\
	dtally		\
	index		\
	select		\
	tally		\
	udisplay

VALPROGS =		\
	valcmp		\
	validate

PROGS = $(NORMPROGS) $(LNKPROGS) $(VALPROGS)
#PROGS= $(NORMPROGS) $(VALPROGS)
SHELLS = unity

#
# Files to install in other directories
#
BINFILES = $(PROGS) $(SHELLS)
LIBFILES = $(DBLIB) db.h val.h Dpasswd Vpasswd Epasswd

LIBOBJS =			\
	zalter.o	\
	zasort.o	\
	zcatalog.o	\
	zcompute.o	\
	chkaccess.o	\
	uexpr.o		\
	zddist.o	\
	zdelete.o	\
	zdis.o		\
	zdist.o		\
	zei.o		\
	zet.o		\
	zformat.o	\
	zindex.o	\
	zindex2.o	\
	zinsert.o	\
	zload.o		\
	zmaketable.o	\
	zmerge.o	\
	znprintf.o	\
	zouterjoin.o	\
	zpacket.o	\
	zuprint.o	\
	zregjoin.o	\
	zuselect.o	\
	rdnext.o	\
	zsetdiff.o	\
	zstat.o		\
	ztable.o	\
	ztuple.o	\
	zuadmin.o	\
	zucheck.o	\
	check.o		\
	zuedit.o	\
	zuenter.o	\
	mktbl2.o	\
	zuget.o		\
	zujoin.o	\
	indexch.o	\
	bopen.o		\
	rdindexed.o	\
	mvgbt.o		\
	ndread.o	\
	ndcheck.o	\
	scan.o		\
	zunload.o	\
	zunmerge.o	\
	zuprintf.o	\
	copy.o		\
	getrec.o	\
	mklock.o	\
	mktbl.o		\
	getfile.o	\
	cnvtdate.o	\
	dirname.o	\
	putrec.o	\
	where.o		\
	getop.o		\
	regexp.o	\
	error.o		\
	setnum.o	\
	sprintfb.o	\
	descrinfo.o	\
	uclean.o

all: $(BINFILES)

install: all
	if [ ! -d $(DESTLIB) ]; then mkdir $(DESTLIB); fi
	cd $(DESTLIB); rm -f $(LIBFILES)
	ln  $(LIBFILES) $(DESTLIB)
	if [ ! -d $(DESTBIN) ]; then mkdir $(DESTBIN); fi
	cd $(DESTBIN); rm -f $(BINFILES)
	for i in $(BINFILES) ; 	do \
		if [ -s $$i$(EXE) ] ; then \
			ln $$i$(EXE) $(DESTBIN); \
		else \
			ln $$i $(DESTBIN); \
		fi; \
	done

clean:
	rm -f *.o

clobber: clean
	rm -f $(DBLIB) $(PROGS) $(SHELLS)

$(DBLIB): $(LIBOBJS)
	rm -f $@
	ar r $@ $(LIBOBJS)
	-ranlib $@

$(LIBOBJS) : db.h
umenu.o : db.h

# PSF swapped $$@.o and $(DBLIB).  The gnu make on ilinux doesn't support
# $$@.o, so we translate it to "%: %.o" but it needs to be the first
# thing on the line.
# PSF - cygwin creates .exe files

$(NORMPROGS): $$@.o $(DBLIB) 
	$(CC) $(CFLAGS) -o $@ $@.o $(DBLIB) -lm
	$(STRIP) $(@)$(EXE)

#
# regjoin, outerjoin, and ujoin are all built from ujoin.c;
# there are just #ifdef's for the differences.
#
regjoin.o : ujoin.c
	rm -f $(@:.o=.c)
	ln ujoin.c $(@:.o=.c)
	$(CC) $(CFLAGS) -DREGJOIN -c $(@:.o=.c)
	rm -f $(@:.o=.c)

zregjoin.o : zujoin.c
	rm -f $(@:.o=.c)
	ln zujoin.c $(@:.o=.c)
	$(CC) $(CFLAGS) -DREGJOIN -c $(@:.o=.c)
	rm -f $(@:.o=.c)

outerjoin.o : ujoin.c
	rm -f $(@:.o=.c)
	ln ujoin.c $(@:.o=.c)
	$(CC) $(CFLAGS) -DOUTERJOIN -c $(@:.o=.c)
	rm -f $(@:.o=.c)

zouterjoin.o : zujoin.c
	rm -f $(@:.o=.c)
	ln zujoin.c $(@:.o=.c)
	$(CC) $(CFLAGS) -DOUTERJOIN -c $(@:.o=.c)
	rm -f $(@:.o=.c)

#
# Below are the special dependencies for validation programs
#
valcmp:	valcmp.o valload.o $(DBLIB) $(PW1)
	$(CC) $(CFLAGS) -o valcmp valcmp.o valload.o $(DBLIB) $(PW) $(PW1)
	$(STRIP) $(@)$(EXE)

valcmp.o: val.h db.h valcmp.y

validate: val.o valload.o validate.o $(DBLIB) $(PW2)
	$(CC) $(CFLAGS) -o validate validate.o val.o valload.o $(DBLIB) $(PW) $(PW2)
	$(STRIP) $(@)$(EXE)

validate.o: validate.c db.h val.h
val.o: val.c val.h db.h

#
# The commands below are linked to other commands.
#
tally: dist
	rm -f tally$(EXE)
	ln dist$(EXE) tally$(EXE)

erase-index: ei
	rm -f erase-index$(EXE)
	ln ei$(EXE) erase-index$(EXE)

erase-table: et
	rm -f erase-table$(EXE)
	ln et$(EXE) erase-table$(EXE)

dtally: ddist
	rm -f dtally$(EXE)
	ln ddist$(EXE) dtally$(EXE)

display: dis
	rm -f display$(EXE)
	ln dis$(EXE) display$(EXE)

join: ujoin
	rm -f join$(EXE)
	cp ujoin$(EXE) join$(EXE)
	chmod 644 join$(EXE)

select: uselect
	rm -f select$(EXE)
	ln uselect$(EXE) select$(EXE)

print: uprint
	rm -f print$(EXE)
	cp uprint print$(EXE)
	chmod 644 print$(EXE)

delete: udelete
	rm -f delete$(EXE)
	ln udelete$(EXE) delete$(EXE)

udisplay: udis
	rm -f udisplay$(EXE)
	ln udis$(EXE) udisplay$(EXE)

index: uindex
	rm -f index$(EXE)
	ln uindex$(EXE) index$(EXE)

dis: udis
	rm -f dis$(EXE)
	cp udis$(EXE) dis$(EXE)
	chmod 644 dis$(EXE)

format: uformat
	rm -f format$(EXE)
	cp uformat$(EXE) format$(EXE)
	chmod 644 format$(EXE)

uman: uman.sh
	cp uman.sh uman
	chmod 755 uman

unity: unity.sh
	cp unity.sh unity
	chmod 755 unity
