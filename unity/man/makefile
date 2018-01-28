NROFF=nroff
NROFF_FLAGS=-man -T37
MV=mv
SOEXPAND=soelim

#
# Suffix information and rules to guide manual page contsruction
#
.SUFFIXES: .1 .mp

.mp.1:
	${SOEXPAND} $*.mp > $*.1m
	$(NROFF) $(NROFF_FLAGS) $*.1m > TMP.1
	$(MV) TMP.1 $@

MANPGS =		\
	Contents.1	\
	Index.1		\
	Intro.1		\
	Uinthl.1	\
	Uintll.1	\
	alter.1		\
	asort.1		\
	catalog.1	\
	compute.1	\
	dist.1		\
	ei.1		\
	et.1		\
	uindex.1	\
	insert.1	\
	load.1		\
	maketable.1	\
	merge.1		\
	nprintf.1	\
	outerjoin.1	\
	packet.1	\
	regjoin.1	\
	setdiff.1	\
	stat.1		\
	table.1		\
	tuple.1		\
	uadmin.1	\
	ucheck.1	\
	udelete.1	\
	udelta.1	\
	udisplay.1	\
	uedit.1		\
	uenter.1	\
	uformat.1	\
	uget.1		\
	ujoin.1		\
	umenu.1		\
	unity.1		\
	unload.1	\
	uprint.1	\
	uprintf.1	\
	uselect.1	\
	val.1		\
	valcmp.1	\
	validate.1

LINKPGS =		\
	ddist.1		\
	dtally.1	\
	erase-index.1	\
	erase-table.1	\
	tally.1		\
	delete.1	\
	unmerge.1

all: manpgs $(LINKPGS)

manpgs: $(MANPGS)

ddist.1 dtally.1 tally.1 : dist.1
	rm -f $(@) $*.1m
	ln dist.1 $(@)
	ln dist.1m $*.1m

dis.1 : udisplay.1
	rm -f $(@) dis.1m
	ln udisplay.1 $(@)
	ln udisplay.1m dis.1m

display.1 : udisplay.1
	rm -f $(@) display.1m
	ln udisplay.1 $(@)
	ln udisplay.1m display.1m

udis.1 : udisplay.1
	rm -f $(@) udis.1m
	ln udisplay.1 $(@)
	ln udisplay.1m udis.1m

erase-index.1 : ei.1
	rm -f $(@) erase-index.1m
	ln ei.1 $(@)
	ln ei.1m erase-index.1m

erase-table.1 : et.1
	rm -f $(@) erase-table.1m
	ln et.1 $(@)
	ln et.1m erase-table.1m

delete.1 : udelete.1
	rm -f $(@) delete.1m
	ln udelete.1 $(@)
	ln udelete.1m delete.1m

format.1 : uformat.1
	rm -f $(@) format.1m
	ln uformat.1 $(@)
	ln uformat.1m format.1m

unmerge.1 : merge.1
	rm -f $(@) unmerge.1m
	ln merge.1 $(@)
	ln merge.1m unmerge.1m

install:

lint:

clean:
	rm -f *.o *.out core *.ln

clobber: clean
	rm -f *.1 *.1m
