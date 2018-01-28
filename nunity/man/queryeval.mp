.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH queryeval 3 "" "New UNITY"
.SH NAME                \" required
queryeval \- Find tuples matching a given query.
.SH SYNOPSIS            \" required
.nf
\fB#include "uquery.h"

int
queryeval( query, result )
struct uquery *query;
struct qresult *result;\fP
.fi
.SH DESCRIPTION         \" required - 1/2 page in length
The
.BR queryeval ()
routine finds tuples matching a given
.I query
produced by
.BR bldquery (3),
.BR mkquery (3),
or
.BR cmpquery (1).
Normally, the tuples matching the
.I query
are returned in the
.I result
parameter; however, 
.BR settplfunc (3)
may be used to set up a function
to be called for each retrieved tuple.
.P
.so Compile.h
.SH PARAMETERS          \" required - if command takes parameters - alphabetical order
.TP
.I query
A pointer to the query data structure to be evaluated.
This structure should be obtained using
.BR bldquery (3),
.BR mkquery (3),
or
.BR cmpquery (1).
If
.BR cmpquery (1)
is used, the query's initialization function,
\f(CWinit\fI<query_name>\fR(),
must be called at run-time to bind the query to its relations
and initialize the query.
If an un-initialized query is passed to
.BR queryeval (),
it will always fail.
.TP
.I result
A pointer to the structure where matching tuples will be stored.
The structure must be allocated in the application
(usually as a local variable),
but no initialization need be done on the structure.
The structure will be initialized each time
.BR queryeval (3)
is called.
.SH EXIT CODES          \" strongly recommended
Normally (i.e., when there are projected attribute values),
.BR queryeval (3)
returns true (non-zero) if there was no error in evaluating the query
and false (zero) if an error occurred.
If a single null attribute was supplied to
.BR bldquery (3),
.BR makequery (3),
or
.BR cmpquery (1),
then
.BR queryeval (3)
returns true (non-zero) if there were tuples that matched the
query and no error occurred.
Otherwise, it returns false (zero).
In the case of an error, the variable
.B uerror
will contain the error number.
.SH RESTRICTIONS        \" optional - includes caveats and bugs
No more than \f(CWMAXRELATION\fP (currently defined in
.I uquery.h
as 8) relations may be used in the query.
.P
At most \f(CWMAXATT\fP attributes can be retrieved by the query.
This is currently defined in
.I urelation.h
as 300 from main frames and 100 for other machines.
Any attributes over this limit are not retrieved.
All relations must have less than \f(CWMAXATT\fP attributes, also.
.SH EXAMPLE             \" strongly recommended
Evaluate a query built with
.BR mkquery (3)
or
.BR bldquery (3).
.P
.RS .5i
.nf
\f(CW#include "uquery.h"
#include "urelation.h"		/* needed for UCHECKFATAL */

extern int uerror;
extern int uchecktuple;
struct uquery *query;
struct qresult result;
char *attrvals[MAXATT];
 . . .
query = mkquery( . . . );	/* could also use bldquery() */
if ( query == NULL ) {
	pruerror( );
	/* Return failure. */
}
/* check and treat any tuple syntax errors as being fatal */
uchecktuple = UCHECKFATAL;
if ( ! queryeval( query, &result ) ) {
	if ( uerror )
		pruerror( );
	/* Return failure. */
}
if ( ! initresult( &result ) ) {
	if ( uerror )
		pruerror( );
	/* Return failure. */
}
while( nexttuple( query, &result, attrvals ) ) {
	/* Do something with each tuple */
}
/* Did we read all tuples or was there an error? */
if ( uerror ) {
	pruerror( );
	/* Return failure. */
}
if ( ! freeresult( result ) ) {
	if ( uerror )
		pruerror( );
	/* Return failure. */
}\fP
.fi
.RE
.P
Evaluate a query (``query1'') built using
.BR cmpquery (1).
.P
.RS .5i
.nf
\f(CW#include "uquery.h"
\f(CW#include "query1.h"

extern int uerror;
extern struct uquery query1;
struct urelation relation1;
struct qresult result;
char *attrvals[QUERY1_MAXATT];
 . . .
if ( ! initquery1( &relation1 ) {
	pruerror( );
	/* Return failure. */
}
if ( ! queryeval( &query1, &result ) ) {
	if ( uerror )
		pruerror( );
	/* Return failure. */
}
if ( ! initresult( result ) ) {
	pruerror( );
	/* Return failure. */
}
while( nexttuple( query1, &result, attrvals ) ) {
	/* Do something with each tuple */
}
/* Did we read all tuples or was there an error? */
if ( uerror ) {
	pruerror( );
	/* Return failure. */
}
if ( ! freeresult( result ) ) {
	if ( uerror )
		pruerror( );
	/* Return failure. */
}\fR
.fi
.RE
.SH SEE ALSO            \" strongly recommended
.nh
bldquery(3), cmpquery(1), mkquery(3), nexttuple(3), retrieve(1),
settplfunc(3), setunpackenv(3), and uerror(3).
.hy
