#
# Below are the #defines recognized in the code:
#
# DEBUG		- If defined code for debugging NUNITY code is included.
# DBBLKSIZE	- Size of DB input buffer blocks.  Normally set to 4096.
#		  Should be between 6144 and 10240 for larger machines
#		  (e.g., maxis and vaxen), should be 2048 for smaller
#		  machines (e.g., 3b2).
# RETSIGTYPE	- A portability defintion.  Defines what signal catching
#		  functions return.  Normally this is set to "int".
#		  On some systems (3b2), however, it must be defined
#		  as "void".
# ADVLOCK -	Define to use UNIX Advisory lock with L<relation>
#		lock files.  Do NOT define ADVLOCK if TMPLOCK is
#		defined (used).
# TMPLOCK -	Define to create lock files in /tmp based on the
#		inode and device numbers of the relation.  Leave
#		undefined to get lock files "L<relation>" in the
#		same directory as the relation.
#		Locks in /tmp don't work for NFS-mounted relations.
#
# IMPORTANT: These #defines should be the same in the unity directory;
# otherwise, errors are garaunteed to occur!
#
SILENT = @		# don't echo commands

DEFS=
MOREDEFS=
HDR=../hdr
INCLDIRS = -I$(HDR)
OPTFLAG = -O
CFLAGS = $(OPTFLAG) $(KFLAGS) $(INCLDIRS) $(DEFS) $(MOREDEFS)

#
# Command for regular expressions
#
REGCMP=regcmp -
LINT = lint
LINTFLAGS = -x -u
#
# Change this next macro to turn on ranlib for the archive
#
#RANLIB=ranlib
RANLIB = $(SILENT):

#
# Destination directory for libraries.
#
DESTLIB=../lib

.SUFFIXES:
.SUFFIXES: .o .c .y .h .re

.re.o :
	$(REGCMP) $*.re
	mv $*.re.c $*.c
	$(CC) -c $(CFLAGS) $*.c
	rm -f $*.c

.re.c :
	$(REGCMP) $*.re
	mv $*.re.c $*.c

#
# C-source code files.  There should be NO use of $(DUMMY) in this list!!
# The last item in this list must not have a '\' after it.  Build does not
# expand macros correctly and the substitutions below (``:.c=.o'') causes
# build to complain about not knowing how to build "DUMMY".
#
# First comes the query code, then the rest of UNITY.
#
C_SOURCE=		\
	addjoin.c	\
	addtuple.c	\
	bindquery.c	\
	bldquery.c	\
	blkjoin.c	\
	bopen.c		\
	chkaname.c	\
	chkindex.c	\
	chkjoin.c	\
	chktuple.c	\
	chkupdate.c	\
	cmpdate.c	\
	cnttuples.c	\
	cprelation.c	\
	deltuple.c	\
	doinsert.c	\
	exprnode.c	\
	exprojlist.c	\
	findattr.c	\
	fmkquery.c	\
	freerel.c	\
	getrelinfo.c	\
	gettuple.c	\
	hash.c		\
	hashjoin.c	\
	indexch.c	\
	isqvar.c	\
	lookupattr.c	\
	lookuptpl.c	\
	mklock.c	\
	mkquery.c	\
	modify.c	\
	ndcheck.c	\
	ndread.c	\
	newqexpr.c	\
	newrelblk.c	\
	newsnode.c	\
	newtuple.c	\
	nexttuple.c	\
	outerjoin.c	\
	packet.c	\
	peruse.c	\
	prcmp.c		\
	prexpr.c	\
	prtplerror.c	\
	prtree.c	\
	prtuples.c	\
	pruerror.c	\
	prunejoin.c	\
	qdebug.c	\
	queryeval.c	\
	querytorel.c	\
	rdindexed.c	\
	rdnext.c	\
	readtuple.c	\
	relreset.c	\
	relropen.c	\
	result.c	\
	scan.c		\
	seektuple.c	\
	smkquery.c	\
	sort_tpl.c	\
	sprintfb.c	\
	starttable.c	\
	tplfunc.c	\
	uerror.c	\
	unmkquery.c	\
	unpacket.c	\
	vmkquery.c	\
	vsmkquery.c	\
	writeattr.c	\
	writedescr.c	\
	writetuple.c

HEADERS =			\
	$(HDR)/uerror.h		\
	$(HDR)/urelation.h	\
	$(HDR)/uquery.h		\
	$(HDR)/message.h

#
# Yacc source code.  These must have no '\' on the last item
# in the list for the same reasons as above.
#
YACC_SOURCE =		\
	where.y

#
# Regular expression definitions.  These must have no '\' on the last item
# in the list for the same reasons as above.
#
RE_SOURCE =		\
	regexpdef.re

SOURCE =		\
	$(YACC_SOURCE)	\
	$(C_SOURCE)	\
	$(RE_SOURCE)	\
	$(DUMMY)

LINT_SOURCE =			\
	$(YACC_SOURCE:.y=.c)	\
	$(C_SOURCE)		\
	$(RE_SOURCE:.re=.c)	\
	uversion.c		\
	$(DUMMY)

#
# Below are listed the objects that compose the library.
# We do not sort the objects explicitly so that the loader
# can do everything in one pass.  We're counting on a smart
# loader that can make multiple passes through the libraries.
#
OBJS =				\
	$(YACC_SOURCE:.y=.o)	\
	$(C_SOURCE:.c=.o)	\
	$(RE_SOURCE:.re=.o)	\
	uversion.o		\
	$(DUMMY)

LIB = libnunity.a

MAJOR_PRODUCTS = $(LIB)

all : $(MAJOR_PRODUCTS)

install : all
	@if [ ! -d $(DESTLIB) ]; then mkdir $(DESTLIB); fi
	cd $(DESTLIB); rm -f $(MAJOR_PRODUCTS)
	ln $(MAJOR_PRODUCTS) $(DESTLIB)

hash.o hashjoin.o sort_tpl.o : hash.h
qdebug.c : qdebug.h

#
# This is the current version of the New Unity commands.
# It should be updated every time there are significant
# changes to the libnunity library or any command.
#
# Version 2.3.6 - sun bug fixes, longer attribute names,
# a minor changes in the way descriptors are found
#
# Version 2.4 - more bug fixes, date compare with 4-digit year,
# UNITYDSEARCH, and alternate table with "/./" or "././" prefix.
#
# Version 2.5 - binary, octal, and hexadecimal numeric comparisons
#		and more bug fixes
#
# Version 2.6 - more bug fixes plus noverbose, verbose=, value=,
#		width, and all:nodisplay= attribute modifiers
#		as well as new type of locks
#
# Version 2.7 - Allow "8-bit" characters with accents, umlauts, etc.
#
VERSION = 2.7

uversion.c : $(SOURCE) $(HEADERS)
	echo "char uversion[] = \"New Unity, Version $(VERSION), `date '+%h %d, 20%y'`\";" > $(@)

$(OBJS) : $(HEADERS)

$(LIB): $(OBJS)
	rm -f $@
	ar r $@ $(OBJS)
	-$(RANLIB) $@

lint: llib-lunity.ln

llib-lunity.ln: $(HEADERS) $(LINT_SOURCE)
	$(LINT) $(LINTFLAGS) $(CFLAGS) -o unity $(LINT_SOURCE) $(LINTLIBS)

clean:
	rm -f *.o *.out *.ln core where.c regexpdef.c uversion.c

clobber: clean
	rm -f $(LIB)
