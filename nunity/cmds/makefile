#
# Below are the #defines recognized in the code:
#
# DEBUG		- Include code for debugging purposes in DB code.
# DBBLKSIZE	- Size of DB input buffer blocks.  Normally set to 4096.
#		  Should be between 6144 and 10240 for larger machines
#		  (e.g., maxis and vaxen), should be 2048 for smaller
#		  machines (e.g., 3b2).
# ADVLOCK	- Define to use UNIX Advisory lock with L<relation>
#		  lock files.  Do NOT define ADVLOCK if TMPLOCK is
#		  defined (used).
# RETSIGTYPE	- A portability defintion.  Defines what signal catching
#		  functions return.  Normally this is set to "int".
#		  On some systems (3b2), however, it must be defined
#		  as "void".
#
# IMPORTANT: These #defines should be the same in the libunity directory;
# otherwise, errors are guaranteed to occur!
#
SILENT = @	# don't echo command

DEFS=
HDR=../hdr
INCLDIRS = -I$(HDR)
OPTFLAG = -O
CFLAGS = $(OPTFLAG) $(KFLAGS) $(INCLDIRS) $(DEFS)
LINT=lint
LINTFLAGS= -x -u
#
# If the commands should not be stripped, change the definition of
# this next macro
#
#STRIP = $(SILENT):
STRIP=strip

#
# Installation directory for commands
#
DESTBIN= ../bin

#
# Libraries needed to build UNITY and the formatters.
#
LIBUNITY = ../libunity/libnunity.a
LIBMISC = ../libmisc/libmisc.a
LIBPW = -lPW

#
# Switch the comments on the next lines if the malloc(3X)
# library is desired.  (This results in better memory
# allocation, but doesn't effect functionality.)
#
LIBMALLOC =
#LIBMALLOC = -lmalloc

UNITYLIBS = $(LIBUNITY) $(LIBMISC) $(LIBMALLOC) $(LIBPW)
FORMLIBS = $(LIBMISC) $(LIBMALLOC) $(LIBPW)

#
# Souce code files for UNITY commands.
#
INSERT_SRC =		\
	ninsert.c

INSERT_OBJS = $(INSERT_SRC:.c=.o)

ALTER_SRC =		\
	seekval.c	\
	nalter.c

ALTER_OBJS = $(ALTER_SRC:.c=.o)

DEL_SRC =		\
	seekval.c	\
	ndelete.c

DEL_OBJS = $(DEL_SRC:.c=.o)

EDIT_SRC =		\
	seekval.c	\
	nuedit.c

EDIT_OBJS = $(EDIT_SRC:.c=.o)

MRG_SRC =		\
	umerge.c

MRG_OBJS = $(MRG_SRC:.c=.o)

RTRV_SRC =		\
	retrieve.c

RTRV_OBJS = $(RTRV_SRC:.c=.o)

SORT_SRC =		\
	usort.c

SORT_OBJS = $(SORT_SRC:.c=.o)

DIFF_SRC =		\
	getrec.c	\
	udiff.c

DIFF_OBJS = $(DIFF_SRC:.c=.o)

CMPQ_SRC =		\
	genheader.c	\
	genquery.c	\
	cmpquery.c

CMPQ_OBJS = $(CMPQ_SRC:.c=.o)

RMLK_SRC =		\
	rmulock.c

RMLK_OBJS = $(RMLK_SRC:.c=.o)

LK_SRC =		\
	ulock.c

LK_OBJS = $(LK_SRC:.c=.o)

PRIDX_SRC =		\
	prindex.c

PRIDX_OBJS = $(PRIDX_SRC:.c=.o)

#
# UNITY formatting commands -- also useful for other things.
#
QPRINTF_SRC =		\
	getrec.c	\
	qprintf.c

QPRINTF_OBJS = $(QPRINTF_SRC:.c=.o)

PRTABLE_SRC =		\
	getrec.c	\
	prtable.c

PRTABLE_OBJS = $(PRTABLE_SRC:.c=.o)

PRBLOCK_SRC =		\
	getrec.c	\
	prblock.c

PRBLOCK_OBJS = $(PRBLOCK_SRC:.c=.o)

#
# Headers used by most all commands
#
HEADERS =			\
	$(HDR)/message.h	\
	$(HDR)/urelation.h	\
	$(HDR)/uquery.h

#
# Define the products that will be built
#
MAJOR_PRODUCTS = 	\
	cmpquery	\
	nalter		\
	ndelete		\
	ninsert		\
	nuedit		\
	prblock		\
	prindex		\
	prtable		\
	qprintf		\
	retrieve	\
	rmulock		\
	udiff		\
	ulock		\
	umerge		\
	usort

all : $(MAJOR_PRODUCTS)

install : all
	@if [ ! -d $(DESTBIN) ]; then mkdir $(DESTBIN); fi
	cd $(DESTBIN); rm -f $(MAJOR_PRODUCTS)
	for i in $(MAJOR_PRODUCTS) ; do \
		if [ -s $$i$(EXE) ] ; then \
			ln $$i$(EXE) $(DESTBIN); \
		else \
			ln $$i $(DESTBIN); \
		fi; \
	done

$(INSERT_OBJS) : $(HEADERS)

$(ALTER_OBJS) : $(HEADERS)

$(DEL_OBJS) : $(HEADERS)

$(EDIT_OBJS) : $(HEADERS)
ndelete.o nuedit.o nalter.o : seekval.h

$(MRG_OBJS) : $(HEADERS)

$(RTRV_OBJS) : $(HEADERS)

$(SORT_OBJS) : $(HEADERS)

$(CMPQ_OBJS) : $(HEADERS)

$(RMLK_OBJS) : $(HDR)/message.h

$(LK_OBJS) : $(HDR)/message.h

$(PRIDX_OBJS) : $(HDR)/uindex.h

ninsert : $(INSERT_OBJS) $(LIBUNITY) $(LIBMISC)
	$(CC) $(CFLAGS) -o $(@) $(INSERT_OBJS) $(UNITYLIBS)
	$(STRIP) $(@)$(EXE)

nalter : $(ALTER_OBJS) $(LIBUNITY) $(LIBMISC)
	$(CC) $(CFLAGS) -o $(@) $(ALTER_OBJS) $(UNITYLIBS)
	$(STRIP) $(@)$(EXE)

ndelete : $(DEL_OBJS) $(LIBUNITY) $(LIBMISC)
	$(CC) $(CFLAGS) -o $(@) $(DEL_OBJS) $(UNITYLIBS)
	$(STRIP) $(@)$(EXE)

nuedit : $(EDIT_OBJS) $(LIBUNITY) $(LIBMISC)
	$(CC) $(CFLAGS) -o $(@) $(EDIT_OBJS) $(UNITYLIBS)
	$(STRIP) $(@)$(EXE)

retrieve : $(RTRV_OBJS) $(LIBUNITY) $(LIBMISC)
	$(CC) $(CFLAGS) -o $(@) $(RTRV_OBJS) $(UNITYLIBS)
	$(STRIP) $(@)$(EXE)

udiff : $(DIFF_OBJS) $(LIBUNITY) $(LIBMISC)
	$(CC) $(CFLAGS) -o $(@) $(DIFF_OBJS) $(UNITYLIBS)
	$(STRIP) $(@)$(EXE)

umerge : $(MRG_OBJS) $(LIBUNITY) $(LIBMISC)
	$(CC) $(CFLAGS) -o $(@) $(MRG_OBJS) $(UNITYLIBS)
	$(STRIP) $(@)$(EXE)

usort : $(SORT_OBJS) $(LIBUNITY) $(LIBMISC)
	$(CC) $(CFLAGS) -o $(@) $(SORT_OBJS) $(UNITYLIBS)
	$(STRIP) $(@)$(EXE)

cmpquery : $(CMPQ_OBJS) $(LIBUNITY) $(LIBMISC)
	$(CC) $(CFLAGS) -o $(@) $(CMPQ_OBJS) $(UNITYLIBS)
	$(STRIP) $(@)$(EXE)

rmulock : rmulock.sh
	cp rmulock.sh ${@}
	chmod 755 ${@}

rmulock.sh :
	echo "exec ulock -unlock \"\$${@}\"" > ${@}

ulock : $(LK_OBJS) $(LIBUNITY) $(LIBMISC)
	$(CC) $(CFLAGS) -o $(@) $(LK_OBJS) $(UNITYLIBS)
	$(STRIP) $(@)$(EXE)

prindex : $(PRIDX_OBJS) $(LIBMISC)
	$(CC) $(CFLAGS) -o $(@) $(PRIDX_OBJS) $(UNITYLIBS)
	$(STRIP) $(@)$(EXE)

#
# Format command rules.  These commands are called by retrieve for
# formatting information.  prtable is analogous to UNITY uprint,
# qprintf is analogous to uprintf.  prblock is analogous to UNITY catalog.
#
prblock : $(PRBLOCK_OBJS) $(LIBMISC)
	$(CC) $(CFLAGS) -o $(@) $(PRBLOCK_OBJS) $(FORMLIBS)
	$(STRIP) $(@)$(EXE)

prtable : $(PRTABLE_OBJS) $(LIBMISC)
	$(CC) $(CFLAGS) -o $(@) $(PRTABLE_OBJS) $(FORMLIBS)
	$(STRIP) $(@)$(EXE)

qprintf : $(QPRINTF_OBJS) $(LIBMISC)
	$(CC) $(CFLAGS) -o $(@) $(QPRINTF_OBJS) $(FORMLIBS)
	$(STRIP) $(@)$(EXE)

clean:
	rm -f *.o *.out *.ln core rmulock.sh

clobber: clean
	rm -f $(MAJOR_PRODUCTS)

LINTUNITY=../libunity/llib-lunity.ln
LINTMISC=../libmisc/llib-lmisc.ln
LINTLIBS=

lint:
	$(LINT) $(CFLAGS) $(LINTFLAGS) $(INSERT_SRC) $(LINTUNITY) $(LINTMISC) $(LINTLIBS)
	$(LINT) $(CFLAGS) $(LINTFLAGS) $(ALTER_SRC) $(LINTUNITY) $(LINTMISC) $(LINTLIBS)
	$(LINT) $(CFLAGS) $(LINTFLAGS) $(DEL_SRC) $(LINTUNITY) $(LINTMISC) $(LINTLIBS)
	$(LINT) $(CFLAGS) $(LINTFLAGS) -DUEDIT $(EDIT_SRC) $(LINTUNITY) $(LINTMISC) $(LINTLIBS)
	$(LINT) $(CFLAGS) $(LINTFLAGS) $(MRG_SRC) $(LINTUNITY) $(LINTMISC) $(LINTLIBS)
	$(LINT) $(CFLAGS) $(LINTFLAGS) $(RTRV_SRC) $(LINTUNITY) $(LINTMISC) $(LINTLIBS)
	$(LINT) $(CFLAGS) $(LINTFLAGS) $(SORT_SRC) $(LINTUNITY) $(LINTMISC) $(LINTLIBS)
	$(LINT) $(CFLAGS) $(LINTFLAGS) $(DIFF_SRC) $(LINTUNITY) $(LINTMISC) $(LINTLIBS)
	$(LINT) $(CFLAGS) $(LINTFLAGS) $(CMPQ_SRC) $(LINTUNITY) $(LINTMISC) $(LINTLIBS)
	$(LINT) $(CFLAGS) $(LINTFLAGS) $(PRBLOCK_SRC) $(LINTMISC) $(LINTLIBS)
	$(LINT) $(CFLAGS) $(LINTFLAGS) $(PRTABLE_SRC) $(LINTMISC) $(LINTLIBS)
	$(LINT) $(CFLAGS) $(LINTFLAGS) $(QPRINTF_SRC) $(LINTMISC) $(LINTLIBS)
