.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH parsewhere 3 "" "New UNITY"
.SH NAME                \" required
parsewhere \- Parse the where-clause for a query.
.SH SYNOPSIS            \" required
.nf
\fB#include "uquery.h"

struct queryexpr *
parsewhere( where, wherecnt, nodelist, nodecnt )
char **where;
int wherecnt;
struct qnode *nodelist;
int nodecnt;\fP
.fi
.SH DESCRIPTION         \" required - 1/2 page in length
The
.BR parsewhere ()
routine is used to parse a where-clause for a query into a
query expression, which can then be passed to
.BR bldquery (3)
to create a query structure.
The where-clause is given as a list of character strings,
each of which represents one argument in the where-clause.
A list of query nodes (relations) must be passed in for
locating attribute references.
A query node is an instance of a relation; the same relation
may be used multiple times in a query.
.P
Upon a successful parse,
a pointer to the query expression is returned.
NULL is returned on failure.
This routine is one of the few New UNITY routines that prints
error messages rather than setting an error code in
.BR uerror .
This is done because there is no way to include context information
when setting
.BR uerror .
This routine uses the
.BR prmsg (3)
interface for printing the messages.
.P
.so Compile.h
.SH PARAMETERS          \" required - if command takes parameters - alphabetical order
.TP
.I where
An array of character pointers which represent the arguments of
the where-clause.
.so Where3.u.h
.TP
.I wherecnt
The number of arguments given in
.IR where .
.so Nodelist.h
.SH EXIT CODES          \" strongly recommended
Upon a successful parse,
a pointer to the query expression is returned.
NULL is returned on failure and an explanatory error message is printed
using
.BR prmsg (3).
.SH RESTRICTIONS        \" optional - includes caveats and bugs
No more than \f(CWMAXRELATION\fP (currently defined in
.I uquery.h
as 8) relations may be used in the query.
.P
Since
.BR parsewhere ()
uses
.BR yacc (1),
no other portions of an application my use
.BR yacc (1)
within the same process.
.SH EXAMPLE             \" strongly recommended
Parse a given where-clause.
.P
.RS .5i
.nf
\f(CWchar *where[];
int wherecnt;
 . . .

if ( wherecnt > 0 ) {
	qptr = parsewhere( where, wherecnt, nodelist, relcnt );
	if ( qptr == NULL ) {
		prmsg( MSG_ERROR, "unsuccessful parse for where-clause" );
		/* return failure */
	}
}
else
	qptr = NULL;\fP
.fi
.RE
.SH SEE ALSO            \" strongly recommended
bldquery(3), cmpquery(1), lookupattr(3), mkquery(3), and retrieve(1).
