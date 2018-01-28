SRCDIRS = libmisc libunity cmds
MANDIR = man
DIRS = $(SRCDIRS) $(MANDIR)
#
# Below are the #defines specifiable to make:
#
# DEBUG	-	Include code for debugging purposes in DB code.
#		The debugging code is turned on by including the
#		flag "-debug" on the command line of each NUNITY
#		command.  There are also particular portions of
#		the debug code that can be turned on with "debug=<opt>,..."
#		flags.  See libunity/qdebug.c for more info.  The debug
#		options have intentionally been left out of the man pages.
#
# MACHINE -	Type of machine that the code is being built for.
#		It is used to set up machine dependent parameters,
#		such as the size of DB input buffer blocks.
#		The current list of possible values is:
#
#			ibm - Mainframe machines
#			M3b - 3b20 machines
#			M3b2 - 3b2 machines
#			vax - Vax machines
#			sun - (old) Sun machines
#			sparc - Sun sparc machines with SunOS 
#			solaris - Sun sparc machines with Solaris
#			i386 - Intel x86 platforms running SVR3
#			i486 - Intel x86 platforms running SVR4
#
# DBBLKSIZE -	Size of DB input buffer blocks.  Normally, this is
#		set by indicating which machine you are on.  The default
#		size is 4096.  For large mainframes it is set to 10240.
#		For vaxen it is set to 6144.  (See hdr/udbio.h.)  This
#		definition can affect the number of reads done to get
#		data and the amount of internal fragmentation there is
#		in memory (since a block cannot be re-used until all
#		attributes in the block are unused).
#
#		IMPORTANT:  This #define MUST NOT BE LESS THAN the
#		definition of MAXREC in hdr/urelation.h (currently 4096).
#		Otherwise, buffer management code will break.  (It
#		assumes that no tuple can span more than two input
#		buffers.)
#
# RETSIGTYPE -	A portability definition.  Defines what signal catching
#		functions return.  Normally this is set to "int".
#		On some systems (3b2, suns), however, it must be defined
#		as "void".
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
# IMPORTANT: These #defines should be the same across all directories;
# otherwise, errors are guaranteed to occur!
#
PASSDOWN=

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
	$(MAKE) $(PASSDOWN) $(@:man=);	\
	cd .. ;

CPIO=cpio
CPIOFLAGS= -ocav

COMPRESS=compress
CMPFLAGS=

nunity: installsrc
	ls -aliR bin lib
	find bin lib -type f -print | $(CPIO) $(CPIOFLAGS) | $(COMPRESS) $(CMPFLAGS) > $(@)

manpgs.cpio: allman manpgs.rm
	find man -type f -name '*.[134]' -print	\
	| $(CPIO) $(CPIOFLAGS) > $@

mansrc.cpio: mansrc.rm
	find $(MANDIR) -type f ! -perm 755 ! -perm 775			\
		! -name '*.[ao134]' ! -name '*.ln' ! -name '*.out'	\
		-print							\
	| $(CPIO) $(CPIOFLAGS) > $(@)

nunity.man : manpgs.cpio
	rm -f $(@)
	ln manpgs.cpio $(@)

nunity.man.Z : manpgs.cpio.Z
	rm -f $(@)
	ln manpgs.cpio.Z $(@)

allsrc.cpio:
	( echo mkunity;							\
	find makefile hdr $(DIRS) -type f ! -perm 755 ! -perm 775	\
		! -name '*.[ao134]' ! -name '*.ln' ! -name '*.out'	\
		! -name core -print					\
	) | $(CPIO) $(CPIOFLAGS) > $(@)

hdrsrc.cpio:
	find hdr -type f -name '*.h' -print \
	| $(CPIO) $(CPIOFLAGS) > $(@)

src.rm nunity.rm build.rm manpgs.rm mansrc.rm:
	-rm -f $(@:.rm=.cpio)

src.cpio: src.rm
	find makefile PORTING README hdr $(SRCDIRS) -type f ! -perm 755 ! -perm 775	\
		! -name '*.[ao134]' ! -name '*.ln' ! -name '*.out'	\
		! -name core -print					\
	| $(CPIO) $(CPIOFLAGS) > $(@)

build.cpio: build.rm
	( echo umake;							\
	find makefile hdr $(SRCDIRS) -type f ! -perm 755 ! -perm 775	\
		! -name '*.[ao134]' ! -name '*.ln' ! -name '*.out'	\
		! -name core -print					\
	) | $(CPIO) $(CPIOFLAGS) > $(@)

nunity.cpio: nunity.rm
	( ls umake mkunity iosize;					\
	find PORTING README NUNITY.NOTES makefile hdr $(DIRS) -type f	\
		! -perm 755 ! -perm 775	! -name '.*'			\
		! -name '*.[ao134]' ! -name '*.l[no]' ! -name '*.out'	\
		! -name core -print					\
	) | $(CPIO) $(CPIOFLAGS) > $(@)

#		! -perm 755 ! -perm 775	! -name '[ON]*' ! -name '.*'	\

nunity.cpio.Z: nunity.cpio
	rm -f $(@)
	$(COMPRESS) $(CMPFLAGS) $?

src.cpio.Z: src.cpio
	rm -f $(@)
	$(COMPRESS) $(CMPFLAGS) $?

allsrc.cpio.Z: allsrc.cpio
	rm -f $(@)
	$(COMPRESS) $(CMPFLAGS) $?

nunity.zip:
	( find makefile umake iosize PORTING README $(SRCDIRS) -type f ! -perm 755 ! -perm 775  \
		! -name '*.[ao134]' ! -name '*.l[on]' ! -name '[.O]*' -print            \
	) | zip -@ -k nunity

nunity.zoo:
	( find makefile PORTING README $(SRCDIRS) -type f ! -perm 755 ! -perm 775	\
		! -name '*.[ao134]' ! -name '*.ln' -print		\
	) | zoo aPPI nunity

mansrc.cpio.Z: mansrc.cpio
	rm -f $(@)
	$(COMPRESS) $(CMPFLAGS) $?

manpgs.cpio.Z: manpgs.cpio
	rm -f $(@)
	$(COMPRESS) $(CMPFLAGS) $?
