.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH uerror 3 "" "New UNITY"
.SH NAME                \" required
uerror, pruerror(), prtplerror() \- UNITY error code variable and error message print routines.
.SH SYNOPSIS            \" required
.nf
\fB#include "uerror.h"
#include "urelation.h"

extern int uerror;
extern int uchecktuple;
extern int utplmsgtype;
extern long utplelimit;
extern long utplerrors;

int
pruerror( );

int prtplerror( relptr, tplptr )
struct urelation *relptr;
struct utuple *tplptr;\fR
.SH DESCRIPTION         \" required - 1/2 page in length
The
.B uerror
variable is set to contain the error number for errors that
occur within UNITY routines.
When no error has occurred, it is set to zero.
The particular error numbers are defined in the header file
.IR uerror.h ;
however, this header file is not needed unless
the application needs to modify the value of
.BR uerror
or check for a specific error number.
.P
The
.BR pruerror ()
routine is used to print a message explaining the error
number stored in
.BR uerror.
If no error has occurred,
.BR pruerror ()
prints nothing.
Because there are no arguments associated with each error
message, the messages are worded very generally.
It is best to print another message after calling
\fBpruerror\fP() giving the context or circumstances
in which the error occurred.
For example, if reading in the descriptor information for a
relation fails,
\fBpruerror\fP()
will say why it failed,
while the application message should say what relation had problems.
.P
\fBpruerror\fR() always returns the error number of the error
that occured or zero if none has occured.
.P
The \fBprtplerror\fR() routine is used to print errors associated
with reading a given tuple, e.g. syntax errors.
If the tuple has no errors, no message will be printed.
All messages indicate the relation name, the tuple number,
and any attribute where the error occured.
\fBprtplerror\fR() returns TRUE (non-zero) if an error occured
while reading the tuple or FALSE (zero) if one did not occur.
.P
The
.B uchecktuple
variable can be set to have the low-level tuple read
function check for tuple errors as each tuple is being read
from a relation and do one of the following actions:
.TP
.I UCHECKIGNORE
Gracefully ignore any tuple errors.
This is the default action when
.B uchecktuple
is not set.
.TP
.I UCHECKCOUNT
Count the number of tuples read with tuple errors.
The
.B utplerrors
variable is incremented by one for each tuple read
which had a tuple error.
The tuple error flag bits are left set so that the
application program can check which tuples were
flagged with tuple errors.
.TP
.I UCHECKPRINT
Count the number of tuples read with a tuple error and
call \fBprtplerror\fR() to print the tuple error message(s).
The
.B utplerrors
variable will reflect the number of tuples read which
had a tuple error.
However,
the application program can no longer determine which
tuples had a tuple error since \fBprtplerror\fP()
clears the tuple error flag bits after printing the
error message(s).
.TP
.I UCHECKFATAL
Terminate the query (or perusal) and return a failure code
if any tuple is read with a tuple error.
The
.B uerror
variable is set and \fBprtplerror\fP() is called
to print the appropriate error message(s) for the
offending tuple when a tuple error is encountered.
In this case, the
.B utplerrors
variable is not set (pegged).
.P
The
.B utplmsgtype
variable can be set to
MSG_ERROR,
defined in
.IR urelation.h ,
to print all tuple error messages with an ERROR prefix
instead of the default WARNING type message prefix.
If
.B utplmsgtype
is set to to either MSG_ERROR or MSG_WARN then the
.B utplelimit
variable is used to limit the number of tuple error (warning)
messages that are printed.
If
.B utplelimit
is reset to zero (0),
no tuple error (warning) messages are printed.
However, the tuple error flag bits will still be cleared when
\fBprtplerror\fP() is called.
If set to non-zero,
.B utplelimit
is decremented by one after printing the appropriate tuple error (warning)
message(s).
It should be noted that
\fBprtplerror\fP()
ignores
.B utplelimit
when
.B utplmsgtype
has not been set.
This is needed for backwards compatability
with eariler releases of New Unity.
.P
.so Compile.h
.SH PARAMETERS
.TP
.I relptr
A pointer to the relation information for the relation the
tuple was read from.
.TP
.I tplptr
A pointer to the tuple in which the error may have occurred.
.SH RESTRICTIONS        \" optional - includes caveats and bugs
.BR pruerror ()
and
.BR prtplerror ()
use the
.BR prmsg (3)
routine to print error messages.
This routine normally prints the error message to
.BR stderr ,
but can be redefined as needed to
handle printing differently, such as printing the message to a window.
.SH EXAMPLE             \" strongly recommended
Print a message when the
.BR getrelinfo (3)
routine fails on
.IR /etc/passwd .
.P
.RS .5i
.nf
\f(CW#include "urelation.h"
#include "message.h"

struct urelation r_passwd;

if ( getrelinfo( "/etc/passwd", &r_passwd, FALSE ) == NULL ) {
	pruerror( );
	prmsg( MSG_ERROR,
		"cannot read descriptor information for /etc/passwd file" );
	return( FALSE );
}\fR
.fi
.RE
.P
Perform individual tuple syntax checking for a relation:
.P
.RS .5i
.nf
\f(CW#include "uquery.h"
#include "message.h"

struct urelation *relptr;	/* previously initialized */
struct unperuseinfo *perptr;
int errorcnt;
struct utuple tpl;
char *attrvals[MAXATT];

/* Initialize the relation for perusal in read mode. */
perptr = init_peruse( relptr, "r" );
if ( perptr == NULL ) {
	pruerror();
	/* print message and return failure */
}
errorcnt = 0;
tpl.tplval = attrvals;
while( peruse_tuple( perptr, &tpl ) ) {
	if ( prtplerror( relptr, &tpl ) != 0 ) {
		/* An error occured; up the error count. */
		++errorcnt;
	}
}
/* end the perusal */
if ( ! end_peruse( perptr, FALSE ) ) {
	pruerror();
	/* print message and return failure */
}
prmsg( MSG_NOTE, "%d tuple%s with errors found",
	errorcnt, errorcnt != 1 ? "s" : "" );\fR
.fi
.RE
.P
Request automatic tuple syntax checking for a relation
and limit the number of errors printed to one (1):
.P
.RS .5i
.nf
\f(CW#include "uquery.h"
#include "urelation.h"
#include "message.h"

extern int uchecktuple;
extern int utplmsgtype;
extern long utplelimit;
extern long utplerrors;

struct urelation *relptr;	/* previously initialized */
struct unperuseinfo *perptr;
int exitcode;
int tuplecnt;
struct utuple tpl;
char *attrvals[MAXATT];

/* Initialize the relation for perusal in read mode. */
perptr = init_peruse( relptr, "r" );
if ( perptr == NULL ) {
	pruerror();
	/* print message and return failure */
}
uchecktuple = UCHECKPRINT;
utplmsgtype = MSG_ERROR;/* print ERROR instead of WARNING message */
utplelimit = 1;		/* only print the first error message */
utplerrors = 0;
tuplecnt = 0;
tpl.tplval = attrvals;
while( peruse_tuple( perptr, &tpl ) ) {
	++tuplecnt;
}
/* end the perusal */
if ( ! end_peruse( perptr, FALSE ) ) {
	pruerror();
	/* print message and return failure */
}
if ( utplerrors ) {
	prmgs( MSG_ERROR: "%d tuple error%s - %d tuple%s checked",
		utplerrors, utplerrors != 1 ? "s" : "",
		tuplecnt, tuplecnt != 1 ? "s" : "" );
	exitcode = 2;
} else {
	prmsg( MSG_ERRNOTE, "Ok - %d tuple%s checked",
		tuplecnt, tuplecnt != 1 ? "s" : "" );
	exitcode = 0;
}\fR
.fi
.RE
.SH SEE ALSO            \" strongly recommended
cmpquery(1), prmsg(3).
