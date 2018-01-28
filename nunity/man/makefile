NROFF=nroff
NROFF_FLAGS=-man -T37
SOEXPAND=soelim

#
# Suffix information and rules to guide manual page contsruction
#
.SUFFIXES: .1 .3 .4 .mp

.mp.1:
	$(SOEXPAND) $*.mp > $*.1m
	$(NROFF) $(NROFF_FLAGS) $*.1m > $@

.mp.3:
	$(SOEXPAND) $*.mp > $*.3m
	$(NROFF) $(NROFF_FLAGS) $*.3m > $@

.mp.4:
	$(SOEXPAND) $*.mp > $*.4m
	$(NROFF) $(NROFF_FLAGS) $*.4m > $@

SEC4_MANPGS =		\
	descriptor.4	\
	$(DUMMY)

SEC3_MANPGS =		\
	bldquery.3	\
	do_insert.3	\
	exprojlist.3	\
	getrelinfo.3	\
	lookupattr.3	\
	mkquery.3	\
	nexttuple.3	\
	parsewhere.3	\
	peruse.3	\
	prmsg.3		\
	queryeval.3	\
	setalloc.3	\
	settplfunc.3	\
	setpackenv.3x	\
	uerror.3	\
	$(DUMMY)

SEC1_MANPGS =		\
	Contents.1	\
	cmpquery.1	\
	nalter.1	\
	ndelete.1	\
	ninsert.1	\
	nuedit.1	\
	prblock.1	\
	prtable.1	\
	qprintf.1	\
	retrieve.1	\
	rmulock.1	\
	udiff.1		\
	ulock.1		\
	umerge.1	\
	usort.1		\
	$(DUMMY)

UNITY_MANPGS =		\
	$(SEC4_MANPGS)	\
	$(SEC1_MANPGS)	\
	$(SEC3_MANPGS)	\
	$(DUMMY)

#
# Set up targets and macros for general builds (all and Install)
#
MAJOR_PRODUCTS = $(UNITY_MANPGS)

all: $(MAJOR_PRODUCTS)

install: all

lint:

clean:

clobber:
	rm -f *.[134] *.[134]m *.[134]man *.[134]x

setpackenv.3x:	setpackenv.mp
	rm -f setpackenv.3* setunpackenv.3m setunpackenv.3
	$(SOEXPAND) setpackenv.mp > setpackenv.3man
	if [ setpackenv.3m* = setpackenv.3man ] ; \
	then \
		mv setpackenv.3man setunpackenv.3m ; \
		$(NROFF) $(NROFF_FLAGS) setunpackenv.3m > setunpackenv.3 ; \
	else \
		mv setpackenv.3man setpackenv.3m ; \
		$(NROFF) $(NROFF_FLAGS) setpackenv.3m > setpackenv.3 ; \
	fi
	touch setpackenv.3x

bldquery.3:  Nodelist.h
bldquery.3: Compile.h
bldquery.3: Qflags.h
cmpquery.1:  Where.u.h
cmpquery.1:  Environ.u.h
cmpquery.1:  Exit.codes.h
do_insert.3: Compile.h
exprojlist.3: Compile.h
getrelinfo.3: Compile.h
lookupattr.3:  Nodelist.h
lookupattr.3: Compile.h
mkquery.3:  Where3.u.h
mkquery.3: Compile.h
mkquery.3: Qflags.h
nexttuple.3: Compile.h
nalter.1:  Where.u.h
nalter.1:  Environ.u.h
nalter.1:  Envunpack.u.h
nalter.1:  Exit.codes.h
nalter.1:  Files.u.h
ndelete.1:  Where.u.h
ndelete.1:  Environ.u.h
ndelete.1:  Envunpack.u.h
ndelete.1:  Exit.codes.h
ndelete.1:  Files.u.h
ninsert.1:  Environ.u.h
ninsert.1:  Envunpack.u.h
ninsert.1:  Exit.codes.h
ninsert.1:  Files.u.h
nuedit.1:  Environ.u.h
nuedit.1:  Envunpack.u.h
nuedit.1:  Exit.codes.h
nuedit.1:  Files.u.h
parsewhere.3:  Where3.u.h
parsewhere.3:  Nodelist.h
parsewhere.3: Compile.h
peruse.3: Compile.h
prblock.1:  Exit.codes.h
prtable.1:  Exit.codes.h
qprintf.1:  Exit.codes.h
queryeval.3: Compile.h
retrieve.1:  Environ.u.h
retrieve.1:  Envunpack.u.h
retrieve.1:  Exit.codes.h
retrieve.1:  Files.u.h
setalloc.3: Compile.h
settplfunc.3: Compile.h
udiff.1:  Environ.u.h
udiff.1:  Envunpack.u.h
udiff.1:  Exit.codes.h
udiff.1:  Files.u.h
umerge.1:  Environ.u.h
umerge.1:  Envunpack.u.h
umerge.1:  Restrict.h
umerge.1:  Files.u.h
usort.1:  Environ.u.h
usort.1:  Envunpack.u.h
usort.1:  Exit.codes.h
usort.1:  Restrict.h
usort.1:  Files.u.h
uerror.3: Compile.h
