.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH nexttuple 3 "" "New UNITY"
.SH NAME                \" required
initresult, nexttuple, freeresult \- Traverse the result of a query.
.SH SYNOPSIS            \" required
.nf
\fB#include "uquery.h"

int
initresult( result )
struct qresult *result;

int
nexttuple( query, result, attrvals )
struct uquery *query;
struct qresult *result;
char **attrvals;

int
freeresult( result )
struct qresult *result;\fP
.fi
.SH DESCRIPTION         \" required - 1/2 page in length
The
.BR initresult (),
.BR nexttuple (),
and
.BR freeresult ()
routines are used to traverse the result of of a query
obtained with the
.BR queryeval (3)
function.
.P
The
.BR initresult ()
routine is called once each time the query
.I result
is to be traversed.
(The
.I result
may be traversed as many times as needed before
.BR freeresult (3)
is called.)
This routine resets all counters and pointers to the
first tuple in the query
.IR result .
A successful initialization returns true (non-zero).
False (zero) is returned whenever there are no tuples in
.I result
or there is an error.
The
.B uerror
variable will be set on an error.
.P
The
.BR nexttuple ()
routine gets the next queried tuple from
.I result
and places pointers to its attribute values into the
.I attrvals
array.
It is up to the application to make sure that
.I attrvals
contains enough space to hold all the pointers for the attributes.
The attribute value pointers are valid only until the next
call to
.BR nexttuple ()
or
.BR freeresult ().
.BR nexttuple ()
returns true (non-zero) on success.
It returns false (zero) when there are no more tuples in the result
or an error was encountered.
The
.B uerror
variable will be set on an error.
.P
The
.BR freeresult ()
routine frees all buffers and closes all relations associated with
.I result.
It is imperative that
.BR freeresult ()
is called after a query result has been used as needed.
Even though only a few tuples may be retrieved for a query,
the buffer page where each retrieved attribute value is stored cannot be
re-used until
.BR freeresult ()
is called.
This can tie up considerable more space than that used
for just the retrieved tuples.
Additionally, certain internal flags are not reset until
.BR freeresult ()
is called.
True (non-zero) is returned if no errors were encountered.
Otherwise, the uerror variable is set and false (zero)
is returned.
When querying packed relations, it is especially important
that the return code be checked since this is when the
pipe used to read a packed relation is closed and the
exit code from the unpack command is checked.
See the setunpackenv(3) manual page for more information.
.P
.so Compile.h
.SH PARAMETERS          \" required - if command takes parameters - alphabetical order
.TP
.I result
A pointer to the data structure holding the result of the given
.IR query .
The
.I result
structure must be allocated by the application (usually as a local variable).
.I result
must have been previously set up by
.BR queryeval (3).
.TP
.I query
A pointer to the query data structure previously evaluated using
.BR queryeval (3).
This parameter is used to get the projection information
about the query.
.TP
.I attrvals
An array of character pointers where pointers to individual
attribute values will be stored.
The application must make sure
at least \f(CWquery->attrcnt\fP pointers are allocated in this array.
.SH EXIT CODES          \" strongly recommended
.BR initresult (),
.BR freeresult (),
and
.BR nexttuple ()
return true (non-zero) on success and false (zero) when
there are no more tuples in the query result or on failure.
In the case of failure, the external variable
.B uerror
will contain the error code.
.SH RESTRICTIONS        \" optional - includes caveats and bugs
At most \f(CWMAXATT\fP attributes can be retrieved.
This is currently defined in
.I urelation.h
as 300 for main frames and 100 for other machines.
Any attributes over this limit are not retrieved.
.SH EXAMPLE             \" strongly recommended
Print the attribute values retrieved from a query.
Use a colon (:) as each attribute's delimiter except for the last
attribute which should have a new-line delimiter.
.P
.RS .5i
.nf
\f(CW#include "uquery.h"
#incldue "uerror.h"

register int i;
struct uquery *query;
struct qresult result;
char *attrvals[MAXATT];
 . . .
if ( ! queryeval( query, &result ) ) {
	pruerror();
	/* Print message and return failure */
}
if ( ! initresult( &result ) ) {
	pruerror();
	/* Print message and return failure */
}
while( nexttuple( query, &result, attrvals ) ) {
	for( i = 0; i < query->attrcnt; i++ )
		writeattrval( stdout, attrvals[i],
			i == query->attrcnt - 1 ? '\en' : ':' );
}
if ( uerror != UE_NOERR ) {
	pruerror();
	/* Print message and return failure */
}

if ( ! freeresult( &result ) ) {
	pruerror();
	/* Print message and return failure */
}\fP
.fi
.RE
.P
Print the attribute values retrieved from a query again.
However, this time use each attribute's original delimiter, except for the last
attribute which should still have a new-line delimiter.
Fixed width attributes should have no delimiter.
.P
.RS .5i
.nf
\f(CW#include "uquery.h"
#include "uerror.h"

register int i;
struct qprojection *projptr;
struct uattribute *attrptr;
struct uquery *query;
struct qresult result;
char *attrvals[MAXATT];
 . . .
if ( ! queryeval( query, &result ) ) {
	pruerror();
	/* Print message and return failure */
}
if ( ! initresult( &result ) ) {
	pruerror();
	/* Print message and return failure */
}
while( nexttuple( query, &result, attrvals ) ) {
	/*
	 * Go thru each projected attribute.  "projptr" points to the
	 * projection information.  This is used to access the attribute
	 * information, which contains the delimiter.  If the attribute
	 * is a fixed width attribute, no delimiter should be printed
	 * unless it's the last projected attribute.
	 */
	for( i = 0, projptr = query->attrlist; i < query->attrcnt;
			i++, projptr++ ) {
		attrptr = &projptr->rel->rel->attrs[projptr->attr];
		writeattrval( stdout, attrvals[i],
			i == query->attrcnt - 1 ? '\en' :
			(attrptr->flags & A_ATYPEMASK) == A_FIXEDWIDTH ? '\e0' :
			attrptr->terminate );
	}
}
if ( uerror != UE_NOERR ) {
	pruerror();
	/* Print message and return failure */
}

if ( ! freeresult( &result ) ) {
	pruerror();
	/* Print message and return failure */
}\fP
.fi
.RE
.P
See the
.BR getrelinfo (3)
manual page for details on the \f(CWurelation\fP and
\f(CWuattribute\fP structures.
.SH EXTENDED DESCRIPTION    \" strongly recommended
.SS Projected Attribute Information Structures
While most applications need only reference the attribute values
of a query result,
there may be need to access the projected attribute information.
The structures to do this, as defined in
.IR uquery.h ,
are shown below:
.P
.RS .5i
.nf
\f(CWstruct uquery {
	...
	short attrcnt;			/* cnt of attr in projection list */
	struct qprojection *attrlist;	/* attr projection list */
	...
};

struct qnode {
	...
	struct urelation *rel;		/* relation info for node */
	...
};

struct qprojection {
	struct qnode *rel;	/* node for the attribute */
	short attr;		/* attribute number */
	short prwidth;	/* width attribute shoule be displayed with */
	char *prname;	/* attribute name printed for attribute */
	short flags;		/* flags about attribute */
};\fP
.fi
.RE
.P
The \f(CWqprojection\fP structure gives information about each
projected attribute.
It gives the query node (i.e., relation) the attribute is from,
the index of the attribute in the relation, and
print information about the attribute.
(This is used to override the information in the attribute
structure itself.)
A query node is an instance of a relation;
the same relation may be used multiple times in one query.
Each query node points to the relation it represents, i.e.,
the \f(CWurelation\fP structure.
To access relation information about a projected attribute,
the following reference will suffice:
.P
.RS .5i
.nf
\f(CWstruct uquery *query;
 . . .
query->attrlist[i].rel->rel->...\fP
.fi
.RE
.P
See the
.BR getrelinfo (3)
manual page for details on the \f(CWurelation\fP and
\f(CWuattribute\fP structures.
.SH SEE ALSO            \" strongly recommended
.nh
cmpquery(1), getrelinfo(3), queryeval(3), retrieve(1), settplfunc(3)
setunpackenv(3), and uerror(3).
.hy
