.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH bldquery 3 "" "New UNITY"
.SH NAME                \" required
bldquery \- Build a query structure from relation, attribute, and where-clause information.
.SH SYNOPSIS            \" required
.nf
\fB#include "uquery.h"

struct uquery *
bldquery( nodelist, nodecnt, attrlist, attrcnt, qptr )
struct qnode *nodelist;
int nodecnt;
struct qprojection *attrlist;
int attrcnt;
struct queryexpr *qptr;

struct uquery *
fbldquery( nodelist, nodecnt, attrlist, attrcnt, qptr, flags )
struct qnode *nodelist;
int nodecnt;
struct qprojection *attrlist;
int attrcnt;
struct queryexpr *qptr;
int flags;\fP
.fi
.SH DESCRIPTION         \" required - 1/2 page in length
The
.BR bldquery ()
and
.BR fbldquery ()
routines assemble a query structure from the query node (i.e., relation),
attribute, and query expression (i.e., where-clause) information.
.P
The only difference between
.BR bldquery ()
and
.BR fbldquery ()
is in the last parameter,
.IR flags .
This parameter gives query flags for modifying the normal operation
of the query.
If
.BR bldquery ()
is used, references to the ``all'' attribute are always expanded,
the query results are not sorted,
and printed attribute names are the normal attribute names,
not the ``friendly'' names.
.BR fbldquery ()
should normally be used in new code;
.BR bldquery ()
is provided for backward compatibility.
.P
.so Compile.h
.SH PARAMETERS          \" required - if command takes parameters - alphabetical order
.so Nodelist.h
.TP
.I attrlist
The list of projected attributes for the query.
The attribute information can be initialized by calling the
.BR lookupprojattr (3)
routine on each projected attribute.
.IP
If
.BR bldquery ()
is used, or the
.I flags
variable does not contain ``Q_NOEXPAND'',
any references to the ``all'' attribute will
be expanded to all attributes in the given relation.
In this case, a new array of \f(CWqprojection\fP structures
will be allocated using
.BR malloc (3)
to hold the expanded projection list.
.IP
If this parameter is NULL, or
.I attrcnt
is less than or equal to zero,
then only the success or failure of the query will be reported when the
query is evaluated.
The
.BR queryeval (3)
routine will return true (non-zero) if a tuple exists matching the
given query expression, but
.BR initresult (3)
will always fail, since no tuple will be retrieved.
.TP
.I attrcnt
The number of structures in the projection list,
.IR attrlist .
.IP
If this parameter is less than or equal to zero, or
.I attrlist
is NULL,
then only the success or failure of the query will be reported when the
query is evaluated.
The
.BR queryeval (3)
routine will return true (non-zero) if a tuple exists matching the
given query expression, but
.BR initresult (3)
will always fail, since no tuple will be retrieved.
.IP
The maximum for this parameter is 100.
Any attributes over this limit will not be retrieved.
.TP
.I qptr
.br
The query expression to be evaluated.
This is the parsed version of the where-clause of the query,
as returned from
.BR parsewhere (3).
.so Qflags.h
.SH EXIT CODES          \" strongly recommended
Upon success, a pointer to the created query structure
is returned.
NULL is returned on failure and
.BR uerror
contains the error code.
.SH RESTRICTIONS        \" optional - includes caveats and bugs
No more than \f(CWMAXRELATION\fP (currently defined in
.I uquery.h
as 8) relations may be used in the query.
.P
At most \f(CWMAXATT\fP attributes can be retrieved by the query.
This is currently defined in
.I urelation.h
as 300 for main frames and 100 for other machines.
Any attributes over this limit are not retrieved.
.SH EXAMPLE             \" strongly recommended
Build a query for a given list of relation names, attribute names,
and a where clause.
(This is essentially the code used in the
.BR mkquery (3)
routine.)
.P
.RS .5i
.nf
\f(CW#include "uquery.h"
\f(CW#include "message.h"

extern struct queryexpr *parsewhere();
extern struct uquery *bldquery();
extern char *calloc();

char **relnames;
int relcnt;
char **attrnames;
int anamecnt;
char **where;
int wherecnt;

register int i;
struct urelation *rellist;	/* relation information */
struct qnode *nodelist;		/* node space for query */
struct queryexpr *qptr;		/* query expression from where-clause */
struct uquery *query;		/* compiled version of query */
int attrcnt;			/* projected attribute count */
struct qprojection attrlist[MAXATT];	/* projected attributes */

/*
 * Get all the information about each relation.
 */
nodelist = (struct qnode *)calloc( relcnt, sizeof( *nodelist ) );
rellist = (struct urelation *)calloc( relcnt, sizeof( *rellist));
if ( nodelist == NULL || rellist == NULL ) {
	prmsg( MSG_INTERNAL, "cannot allocate space for relation information" );
	return( NULL );
}
for( i = 0; i < relcnt; i++ ) {
	/*
	 * In mkquery() additional code is included here to strip off
	 * any alternate relation description (i.e., "<rel>=<alt_rel>")
	 * and use that to get the descriptor information.  That code
	 * is not included here for simplicity.
	 */
	if ( ! getrelinfo( relnames[i], &rellist[i], FALSE ) ) {
		pruerror( );
		prmsg( MSG_ERROR, "cannot get descriptor info for table '%s'",
			ptr );
		return( NULL );
	}

	nodelist[i].rel = &rellist[i];
}
/*
 * Look up each projected attribute.
 */
attrcnt = 0;
for( i = 0; i < anamecnt; i++, attrcnt++ ) {
	/*
	 * If any attribute renaming is needed (e.g., "<attr> as <new_name>"),
	 * the code to do that should be included here.  It is not included
	 * here for simplicity.
	 */
	if ( ! lookupprojattr( attrnames[i], nodelist, relcnt,
			&attrlist[attrcnt], FALSE ) ) {
		prmsg( MSG_ERROR, "unrecognized attribute name '%s'",
			attrnames[i] );
		return( NULL );
	}
}
/*
 * Parse any where clause.
 */
if ( wherecnt > 0 ) {
	qptr = parsewhere( where, wherecnt, nodelist, relcnt );
	if ( qptr == NULL ) {
		pruerror( );
		prmsg( MSG_ERROR, "unsuccessful parse for where-clause" );
		return( NULL );
	}
}
else
	qptr = NULL;
/*
 * Build the query itself.
 */
query = bldquery( nodelist, relcnt, attrlist, attrcnt, qptr );
if ( query == NULL )
	pruerror( );
return( query );\fR
.fi
.RE
.SH SEE ALSO            \" strongly recommended
.nh
cmpquery(1), exprojlist(3), getrelinfo(3), lookupattr(3),
mkquery(3), parsewhere(3), queryeval(3), retrieve(1),
setunpackenv(3), and uerror(3).
.hy
