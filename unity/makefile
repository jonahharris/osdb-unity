SRCDIRS = src
MANDIR = man
DIRS = $(SRCDIRS) $(MANDIR)
#
# Below are the #defines recognized in the code, which can be passed
# down using the PASSDOWN macro:
#
# RETSIGTYPE	- A portability defintion.  Defines what signal catching
#		  functions return.  Normally this is set to "int".
#		  On some systems (3b2), however, it must be defined
#		  as "void".
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
# DEFS		- List of definitions used in CFLAGS.  RETSIGTYPE and TMPLOCK 
#		are included in this macro and then included in PASSDOWN.
#
# PW, PW1, PW2	- Programmer's Workbench definitions.  See src/makefile
#		  for the complete discussion on these.  These are used
#		  to tell where the regular expression routines are
#		  located.
#
# An example of PASSDOWN is shown below.  It is the definition typically
# used on a sun3 machine where there is no Programmer's Workbench availble.
# It is imperative that sub-parts of PASSDOWN be quoted to avoid
# interpretation as individual arguments.
#
#PASSDOWN="DEFS='-DRETSIGTYPE=void' PW= PW1=regcmp.o PW2=regex.o"
PASSDOWN=

#
# iomv is a private command which can re-structure a cpio archive based
# on a set of patterns for file names.  This is used to rename all
# manual pages so the section number is the first character, e.g.,
# "man/uselect.1" becomes "man/1.uselect".  It's basically a
# convenience to avoid hassles and problems with prefixes instead
# of suffixes when using make.
#
# CHGMANCMD should normally be defined as null except when building for
# an archive of exptools man pages.
#
IOMVFLAGS= -c
#CHGMANCMD= | iomv $(IOMVFLAGS) '^(.*/)$$0([^/][^/]*)$$1\.([0-9])$$2$$' '$$0$$2.$$1'
CHGMANCMD=

all clean clobber install:
	for i in $(DIRS); do	\
		cd $$i;		\
		echo "\n$$i:";	\
		$(MAKE) $(PASSDOWN) $(@);	\
		cd ..;		\
	done

allsrc cleansrc installsrc clobbersrc lint:
	for i in $(SRCDIRS); do	\
		cd $$i;		\
		echo "\n$$i:";	\
		$(MAKE) $(PASSDOWN) $(@:src=);	\
		cd ..;		\
	done

allman cleanman installman clobberman:
	cd $(MANDIR);		\
	echo "\n$(MANDIR):";	\
	$(MAKE) $(@:man=);		\
	cd .. ;

CPIO=cpio
CPIOFLAGS= -ocav

COMPRESS=compress
CMPFLAGS=

unity: installsrc
	(find bin lib -type f -print ) | $(CPIO) $(CPIOFLAGS) | $(COMPRESS) $(CMPFLAGS) > $(@)

unity.man: allman
	find man -type f -name '*.[134]' -print	\
	| $(CPIO) $(CPIOFLAGS) $(CHGMANCMD) > $@

mansrc.cpio:
	find $(MANDIR) -type f ! -perm 755 ! -perm 775			\
		! -name '*.[ao134]' ! -name '*.l[on]' ! -name '[134].*' -print \
	| $(CPIO) $(CPIOFLAGS) > $(@)

hdrsrc.cpio:
	find src -type f -name '*.h' -print \
	| $(CPIO) $(CPIOFLAGS) > $(@)

allsrc.rm src.rm unity.rm :
	-rm -f $(@:.rm=.cpio)

unity.cpio: unity.rm
	( ls umake mkunity iosize;					\
	find README BUGLIST WISHLIST PORTING makefile $(DIRS) -type f	\
		! -perm 755 ! -perm 775	! -name '[ON]*' ! -name '.*'	\
		! -name '*.[ao134]' ! -name '*.l[on]' ! -name '[134].*'	\
		-print	\
	) | $(CPIO) $(CPIOFLAGS) > $(@)

src.cpio: src.rm
	( find makefile umake iosize PORTING README $(SRCDIRS) -type f	\
		! -perm 755 ! -perm 775	 ! -name '[ON]*'		\
		! -name '*.[ao134]' ! -name '*.l[no]' -print		\
	) | $(CPIO) $(CPIOFLAGS) > $(@)

allsrc.cpio: allsrc.rm
	( echo mkunity;						\
	find makefile iosize PORTING README $(DIRS) 		\
		-type f ! -perm 755 ! -perm 775 ! -name '[ON]*'	\
		! -name '*.[ao134]' ! -name '*.l[no]' -print	\
	) | $(CPIO) $(CPIOFLAGS) > $(@)

unity.zoo:
	( find makefile umake iosize PORTING README $(SRCDIRS) -type f ! -perm 755 ! -perm 775	\
		! -name '*.[ao134]' ! -name '*.l[on]' -print		\
	) | zoo aPPI unity

unity.zip:
	( find makefile umake iosize PORTING README $(SRCDIRS) -type f ! -perm 755 ! -perm 775	\
		! -name '*.[ao134]' ! -name '*.l[on]' -print		\
	) | zip -@ -k unity

unity.Z: unity
	rm -f $(@)
	$(COMPRESS) $(CMPFLAGS) $?

src.cpio.Z: src.cpio
	rm -f $(@)
	$(COMPRESS) $(CMPFLAGS) $?

allsrc.cpio.Z: allsrc.cpio
	rm -f $(@)
	$(COMPRESS) $(CMPFLAGS) $?

mansrc.cpio.Z: mansrc.cpio
	rm -f $(@)
	$(COMPRESS) $(CMPFLAGS) $?

unity.man.Z: unity.man
	rm -f $(@)
	$(COMPRESS) $(CMPFLAGS) $?

check.cpio: makefile
	echo makefile | $(CPIO) $(CPIOFLAGS) > $(@)

check:
	uname -a
	echo TOOLS=$$TOOLS
	cd /opt/exptools/lib/unity; pwd; find . ../toolnews/unity -type f -print | xargs ls -l
