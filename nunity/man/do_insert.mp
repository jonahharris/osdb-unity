.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH do_insert 3 "" "New UNITY"
.SH NAME                \" required
do_insert \- Insert tuples in a UNITY relation.
.SH SYNOPSIS            \" required
.nf
\fB#include "uquery.h"

struct uinsertinfo *
init_insert( relptr, attrlist )
struct urelation *relptr;
char *attrlist;

struct uinsertinfo *
init_insert_tc( relptr, attrlist, tuplecnt )
struct urelation *relptr;
char *attrlist;
unsigned long *tuplecnt;

int
addattr( insptr, attrname )
struct uinsertinfo *insptr;
char *attrname;

int
do_insert( insptr, attrvals )
struct uinsertinfo *insptr;
char **attrvals;

int
vdo_insert( insptr, attrval1, attrval2, ... attrvalN )
struct uinsertinfo *insptr;
char *attrval1;
char *attrval2;
\...
char *attrvalN;

int
end_insert( insptr, commit )
struct uinsertinfo *insptr;
int commit;\fP
.SH DESCRIPTION         \" required - 1/2 page in length
This set of routines is used to add tuples to a given UNITY relation.
.P
.BR init_insert ()
is used to initialize an insertion operation for the given
relation with the given attributes.
It returns a pointer to the insertion information that must be
used in the calls to all other insertion routines.
.P
.BR init_insert_tc ()
is used to initialize an insertion option just like
.BR init_insert ()
except that it also returns the number of tuples
that were copied, if any, from the existing relation.
The format (syntax) of each tuple in the existing relation
is also verified when it is copied from the original file.
.P
.BR addattr ()
can be used to add one attribute
to the list of attributes inserted into a relation.
.P
.BR do_insert ()
actually places a tuple into the relation.
This routine may be called as often as needed to insert tuples.
Any attributes not specified previously with
.BR init_insert ()
or
.BR addattr ()
will recieve an empty value.
.P
.BR vdo_insert ()
is an alternate interface to
.BR do_insert ().
It allows the user to specify the attribute values
as function parameters, rather than in an array of
character pointers.
There must be as many attributes given as parameters
as were specified to
.BR init_insert ()
and
.BR addattr ().
.P
After all tuples are inserted,
.BR end_insert ()
is used to either commit or abort the insertion.
If this routine is called with
.I insptr
equal to NULL, then all pending insertions are either
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
A pointer to the relation information for the insertion relation.
The relation must exist and be readable.
The directory where the relation exists must be writeable
by the effective user-id or group-id of the process.
.TP
.I attrlist
A string containing white space separated attribute names that
will be inserted in the relation.
This list can be NULL, but in this case
.BR addattr ()
must be called later to specify at least one attribute to insert.
Each attribute name
must be unique within
.I attrlist .
If an attribute is specified multiple times,
.BR init_insert ()
will fail.
The order of the attributes in
.I attrlist
together with any additional calls to
.BR addattr ()
is the order the attribute values must be specified in
.I attrvals
when
.BR do_insert ()
or
.BR vdo_insert ()
is called.
.TP
.I insptr
The pointer to an insertion structure previously obtained from
.BR init_insert ().
.IP
If a NULL value is passed for this parameter to
.I end_insert (),
all pending insertions will either be committed or aborted
according to the
.I commit
flag.
.TP
.I attrname
The name of one attribute to be inserted in the relation.
The attribute must not have been previously specified for insertion,
either through
.BR addattr ()
or in the
.I attrlist
parameter passed to
.BR init_insert ().
.TP
.I attrvals
One tuple's attribute values to insert in the relation.
Each entry in the array corresponds to one attribute value.
There must be as many attribute values given,
and in the same order, as specified in
.I attrlist
and calls to
.BR addattr ().
.TP
.I "attrval1, attrval2, ... attrvalN"
One tuple's attribute values to insert in the relation.
Each function argument corresponds to one attribute value.
There must be as many attribute values given,
and in the same order, as specified in
.I attrlist
and calls to
.BR addattr ().
.TP
.I commit
A boolean flag indicating whether to commit the previous
insertions (non-zero) or abort the changes (zero).
.SH EXIT CODES          \" strongly recommended
.BR init_insert ()
returns a pointer to an insertion structure on success or
NULL on failure.
.P
All other functions return true (non-zero) on success and
false (zero) on failure.
.P
If any of these routines fail, the external variable
.BR uerror
contains the error code.
.SH RESTRICTIONS        \" optional - includes caveats and bugs
The number of current insertions is limited  by the
number of open files allowed, usually twenty.
.P
At most \f(CWMAXATT\fP attributes
can be inserted in any relation at one time.
This is currently defined in
.IR urelation.h
as 300 for mainframes and 100 for other machines.
.P
It is imperative that the application provide a signal handler
to call
.BR end_insert ()
on terminating signals.
Otherwise, relations will not be unlocked.
.SH EXAMPLE             \" strongly recommended
Add some attributes to a password file.
The \f(CWlogin\fP attribute will always be the first attribute inserted;
any additional attributes will be given in a list.
Be sure and handle signals.
.P
.RS .5i
.nf
\f(CW#include "uquery.h"
\f(CW#include <signal.h>

catch_sig( sig )
int sig;
{
	signal( sig, SIG_IGN );		/* ignore any subsequent signals */

	end_insert( NULL, FALSE );	/* abort all pending insertions */

	/* print message about aborted insertions */
	exit( 2 );
}
 . . .

char **attrlist;
int attrcnt, i;
char **attrvals;
struct urelation *relptr;
struct uninsertinfo *insptr;

signal( SIGINT, catch_sig );
signal( SIGTERM, catch_sig );
signal( SIGQUIT, catch_sig );
signal( SIGHUP, catch_sig );

/*
 * Get the relation information.
 */
relptr = getrelinfo( "passwd", NULL, FALSE );
if ( relptr == NULL ) {
	pruerror();
	/* print message and return failure */
}
/*
 * Initialize the insertion and give the "login" attribute
 * as part of the insertion.
 */
insptr = init_insert( relptr, "login" );
if ( insptr == NULL ) {
	pruerror();
	/* print message and return failure */
}
/*
 * For any other attributes given, also use them in the insertion.
 * We assume here that attrcnt is the total number of attributes,
 * including "login" that are inserted.
 */
for( i = 1; i < attrcnt; i++ ) {
	if ( ! addattr( insptr, attrlist[i - 1] ) ) {
		pruerror( );
		/* print message */
		end_insert( insptr, FALSE );
		/* return failure */
	}
}
while( /* more tuples to insert . . . */ ) {
	if ( ! do_insert( insptr, attrvals ) ) {  /* insert each tuple */
		pruerror( );
		/* print message */
		end_insert( insptr, FALSE );
		/* return failure */
	}
}
if ( ! end_insert( insptr, TRUE ) ) {	/* commit the changes */
	pruerror( );
	/* print message and return failure */
}\fR
.fi
.RE
.P
.SH SEE ALSO            \" strongly recommended
.nh
cmpquery(1), ninsert(1), getrelinfo(3), peruse(3), and uerror(3).
.hy
