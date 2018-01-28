.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH peruse 3 "" "New UNITY"
.SH NAME                \" required
peruse \- Step through tuples in a UNITY relation and optionally change them.
.SH SYNOPSIS            \" required
.nf
\fB#include "uquery.h"

struct uperuseinfo *
init_peruse( relptr, mode )
struct urelation *relptr;
char *mode;

int
peruse( perptr, attrvals )
struct uperuseinfo *perptr;
char **attrvals;

int
peruse_tuple( perptr, tplptr )
struct uperuseinfo *perptr;
struct utuple *tplptr;

int
savetuple( perptr, attrvals )
struct uperuseinfo *perptr;
char **attrvals;

int
end_peruse( perptr, commit )
struct uperuseinfo *perptr;
int commit;

struct uperusinfo *
peruse_lock( relptr )
struct urelation *relptr;

int
start_save( perptr )
struct uperuseinfo *perptr;

int
stop_save( perptr, keep_remain )
struct uperuseinfo *perptr;
int keep_remain;

int
stop_save_tc( perptr, keep_remain, tuplecnt )
struct uperuseinfo *perptr;
int keep_remain;
unsigned long *tuplecnt;\fP
.SH DESCRIPTION         \" required - 1/2 page in length
This set of routines is used to step through the tuples of a
relation one by one.
The perusal may be done for reading only, or with updates.
.P
.BR init_peruse ()
is used to initialize a peruse operation for the given
relation.
It returns a pointer to the peruse information that must be
used in the calls to all other peruse routines.
.P
.BR peruse ()
reads a tuple from the relation and places pointers to each
attribute value in the array provided.
This routine is used when only the attribute
values are needed while doing the perusal.
.P
.BR peruse_tuple ()
also reads a tuple from the relation and places
the seek location (\f(CWtplptr->lseek\fR),
the tuple number (\f(CWtplptr->tuplenum\fR),
the tuple size (\f(CWtplptr->tplsize\fR),
tuple syntax error information (see the EXAMPLE section),
and
pointers to each attribute value (\f(CWtplptr->tplval\fR)
in the tuple structure whose pointer is passed in.
This routine is normally used only when information other than the
attribute values is needed while doing the perusal.
.P
Calls to
.BR peruse ()
and
.BR peruse_tuple ()
may be freely intermixed as needed.
.P
When a perusal is in write mode, the
.BR savetuple ()
routine is used to write the attribute values back out to the
relation after possible alterations.
If a tuple is not saved, it is deleted.
.P
The
.BR peruse_lock (),
.BR start_save (),
.BR stop_save (),
and
.BR stop_save_tc ()
routines are for finer control over update perusals.
.BR peruse_lock ()
opens a relation for a write-mode perusal,
by locking the relation; however, no files are opened.
.BR start_save ()
is used immediately before
.BR peruse ()
or
.BR peruse_tuple ()
are first called to open the original and updated files.
.BR init_peruse
in write mode is roughly equivalent to a call to
.BR peruse_lock ()
and
.BR start_save ().
The
.BR stop_save ()
routine closes all files associated with the update
and writes all changes to disk, but does not commit the
changes.
The
.BR stop_save_tc ()
is identical to
.BR stop_save ()
except that it returns a count of the number of
unread tuples that are saved when
.BR keep_remain
is set to true (non-zero).
.P
After all tuples are perused,
.BR end_peruse ()
is used to either commit or abort the perusal.
It is smart enough to know what still needs to be done to
commit the perusal, depending on whether on
whether
.BR init_peruse ()
or
.BR peruse_lock ()
was used.
If this routine is called with
.I perptr
equal to NULL, then all pending perusals are either
aborted or committed according to the
.I commit
flag.
This can be used in signal handlers to clean up on terminating
signals.
.P
.so Compile.h
.SH PARAMETERS          \" required - if command takes parameters - alphabetical order
.TP
.I relptr
A pointer to the relation information for the perused relation.
The relation must exist and be readable.
The directory where the relation exists must be writeable
by the effective user-id or group-id of the process
if the perusal is in write mode.
.TP
.I mode
The perusal mode.
If
.I mode
starts with ``w'', the perusal is in write mode; otherwise, it is
for reading only.
.TP
.I perptr
The pointer to a perusal structure previously obtained from
.BR init_peruse ().
.IP
If a NULL value is passed for this parameter to
.BR end_peruse (),
all pending perusals will either be committed or aborted
according to the
.I commit
flag.
.TP
.I attrvals
One tuple's attribute values.
When passed to
.BR peruse (),
pointers to the next tuple's attribute values are placed in
this array.
When passed to
.BR savetuple (),
the attribute values in the array are written out to the relation.
.TP
.I tplptr
A pointer to a tuple structure which will be filled in
with the seek location, tuple number, and attribute values
of the tuple.
.IP
Note:  Tuple structures do not normally have an attribute value
array associated with them.
It is essential that \f(CWtplptr->tplval\fR be set to point
to a character pointer arry large enough for all the attributes
of the tuple.
(See the EXAMPLE section below.)
.TP
.I commit
A boolean flag indicating whether to commit the previous
perusals (non-zero) or abort the changes (zero).
.TP
.I keep_remain
A boolean flag saying whether to keep any un-read tuples.
This flag only makes a difference if
.BR stop_save ()
is called before
.BR peruse ()
has detected end-of-file.
If this parameter is true (non-zero), the un-read tuples
are saved in the updated relation unchanged.
If false (zero), the un-read tuples are deleted from the
updated relation.
.TP
.I tuplecnt
A pointer to an unsigned long integer that is
used to return the number of un-read tuples
that are saved in the updated relation when
.BR stop_save_tc ()
is called with
.I keep_remain
set to true.
.SH EXIT CODES          \" strongly recommended
.BR init_peruse ()
and
.BR peruse_lock ()
return a pointer to a perusal structure on success or
NULL on failure.
.P
All other functions return true (non-zero) on success and
false (zero) on failure.
.P
If any of these routines fail, the external variable
.BR uerror
contains the error code.
.SH RESTRICTIONS        \" optional - includes caveats and bugs
The maximum number of relations open for perusal concurrently is twenty.
Furthermore, the number of concurrent perusals is limited  by the
number of open files allowed, usually twenty.
Write mode perusals require two open files, while read mode
perusals require one open file.
If
.BR peruse_lock (),
.BR start_save ()
and
.BR stop_save ()
are used, the number of concurrently open files will be minimized.
.P
Each relation can have at most \f(CWMAXATT\fP (currently defined as 100 in
.IR urelation.h )
attributes.
.P
It is imperative that the application provide a signal handler
to call
.BR end_peruse ()
on terminating signals.
Otherwise, relations will not be unlocked when the process exits.
.SH EXAMPLE             \" strongly recommended
Modify a
.I passwd
file for a given login to a given set of new values.
Handle signals, also.
.P
.RS .5i
.nf
\f(CW#include "uquery.h"
#include "uerror.h"
#include <signal.h>

catch_sig( sig )
int sig;
{
	signal( sig, SIG_IGN );		/* ignore any subsequent signals */
	end_peruse( NULL, FALSE );	/* abort all pending perusals */
	/* print message about aborted perusals */
	exit( 2 );
}
 . . .

char *attrvals[MAXATT], *newvals[MAXATT], **tmpval;
struct urelation *relptr;
struct unperuseinfo *perptr;
struct utuple tpl;

signal( SIGINT, catch_sig );
signal( SIGTERM, catch_sig );
signal( SIGQUIT, catch_sig );
signal( SIGHUP, catch_sig );

/*
 * Initialize the relation for perusal in write mode.
 */
perptr = init_peruse( relptr, "w" );
if ( perptr == NULL ) {
	pruerror();
	/* print message and return failure */
}
tpl.tplval = attrvals;
while( peruse_tuple( perptr, &tpl ) ) {
	/*
	 * Check if the tuple had any syntax errors.  If there are any
	 * errors, (tpl.flags & TPL_ERRORMSK) will be non-zero and
	 * tpl.badattr will be set to the number of the first attribute
	 * that had an error.  If there are multiple errors in tpl.flags,
	 * which error applies to which attribute is unknown.  The routine
	 * prtplerror() can be used to print a message describing the
	 * error(s).  If there are no errors, this routine does nothing.
	 *
	 * If there are errors in the tuple, all attributes that weren't
	 * read will be set the an empty string (i.e., ""), so there is
	 * no need to provide special handling for the attribute values
	 * in this case.
	 */
	if ( (tpl.flags & TPL_ERRORMSK) != 0 ) {
		/* print message, but continue anyway */
		prtplerror( relptr, &tpl );
	}

	/*
	 * We assume here that the login is field 0.  Normally,
	 * we should really use findattr() (See lookupattr(3).) to
	 * know which attributes to compare.
	 */
	if ( strcmp( attrvals[0], newvals[0] )	/* found the login */
		tmpval = newvals;		/* save the new values */
	else
		tmpval = attrvals;		/* no match - same tuple */

	if ( ! savetuple( perptr, tmpval ) ) {	/* save the tuple */
		pruerror( );
		/* print message */
		end_peruse( perptr, FALSE );
		/* return failure */
	}
}
if ( uerror != UE_NOERR ) {
	pruerror( );
	/* abort the update */
	(void)end_peruse( perptr, FALSE );
	/* print message and return failure */
}
if ( ! end_peruse( perptr, TRUE ) ) {		/* commit the changes */
	pruerror( );
	/* print message and return failure */
}\fR
.fi
.RE
.P
Modify a list of relations, but use the
.BR peruse_lock (),
.BR start_save ()
and
.BR stop_save ()
routines so we don't run out of file descriptors.
(We don't show the signal handler this time for brevity.)
.P
.RS .5i
.nf
\f(CW#include "uquery.h"
#include "uerror.h"

struct urelation **rellist;		/* list of modified relations */
struct unperuseinfo **perlist;		/* perusal pointers for rellist */
int relcnt;				/* number of relations */
int i;
char *attrvals[MAXATT];

/*
 * Initialize each relation for perusal in write mode.
 */
for( i = 0; i < relcnt; i++ ) {
	perlist[i] = peruse_lock( rellist[i] );
	if ( perlist[i] == NULL ) {
		pruerror();
		/* print message */
		while( i-- > 0 )	/* abort previous inits */
			end_peruse( perlist[i], FALSE );
		/* return failure */
	}
}
/*
 * Go through each relation and make changes.
 */
for( i = 0; i < relcnt; i++ ) {
	/*
	 * Start saving tuples for the relation.  This will open
	 * the necessary files.
	 */
	if ( ! start_save( perlist[i] ) ) {
		pruerror();
		/* print message */
		for( i = 0; i < relcnt; i++ )	/* abort all perusals */
			end_peruse( perlist[i], FALSE );
		/* return failure */
	}
	while( peruse( perlist, attrvals ) ) {
		/*
		 * Decide how the tuple should be modified or if it
		 * should be deleted.  If it should be deleted, the
		 * call to savetuple() should be skipped.
		 */
		. . .
		if ( ! savetuple( perlist, attrvals ) ) { /* save the tuple */
			pruerror( );
			/* print message */
			for( i = 0; i < relcnt; i++ ) /* abort all perusals */
				end_peruse( perlist[i], FALSE );
			/* return failure */
		}
	}
	if ( uerror != UE_NOERR ) {
		pruerror( );
		/* print message */
		for( i = 0; i < relcnt; i++ ) /* abort all perusals */
			end_peruse( perlist[i], FALSE );
		/* return failure */
	}
	/*
	 * Finished saving tuples.  This next call will close all files
	 * and write the changes, but not commit them.
	 */
	if ( ! stop_save( perlist[i], TRUE ) ) {
		pruerror();
		/* print message */
		for( i = 0; i < relcnt; i++ )	/* abort all perusals */
			end_peruse( perlist[i], FALSE );
		/* return failure */
	}
}
/*
 * Now commit all the changes.
 */
for( i = 0; i < relcnt; i++ ) {
	if ( ! end_peruse( perlist[i], TRUE ) ) { /* commit the changes */
		/*
		 * This should never happen.  If it does the data is
		 * now inconsistent.
		 */
		pruerror( );
		/* print message and return failure */
	}
}\fP
.fi
.RE
.P
.SH SEE ALSO            \" strongly recommended
.nh
cmpquery(1), do_insert(3), lookupattr(3), nalter(1), and uerror(3).
.hy
