.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH settplfunc 3 "" "New UNITY"
.SH NAME                \" required
settplfunc \- Set a function to be called for each retrieved tuple of a query.
.SH SYNOPSIS            \" required    
.nf
\fB#include "uquery.h"

int
(*settplfunc( query, tplfunc ))()
struct uquery *query;
int (*tplfunc)();\fP
.fi
.SH DESCRIPTION         \" required - 1/2 page in length
The
.BR settplfunc ()
routine associates a function,
.IR tplfunc ,
to be called for each retrieved tuple of a
.IR query .
The tuple function is called as each matching tuple is found by the
.BR queryeval (3)
routine.
The function is passed the retrieved tuple's attribute values and projection
information about each attribute.
(The projection information is explained in the \fBEXTENDED DESCRIPTION\fP
section below.)
All information passed to the tuple function is only valid until
the tuple function returns.
After
.BR queryeval ()
has returned, the query result (the \f(CWqresult\fP structure)
will be empty.
Therefore, it is not possible to traverse the query result using
.BR nexttuple (3)
if a tuple function is used with a query.
.P
It is not possible to set up a tuple function for a query
if no attributes are projected as part of the query.
If this is done, the tuple function will never be called,
even though \fBqueryeval\fR() may succeed.
.P
.so Compile.h
.SH PARAMETERS          \" required - if command takes parameters - alphabetical order
.TP
.I query
A pointer to the query structure which will have the tuple function
associated with it.
The query must have some projected attributes
or the tuple function will never be called.
If the query previously had another tuple function associated with it,
the address of the previous function is returned.
.TP
.I tplfunc
An application function that will be called once for each matching
tuple retrieved as part of the query evaluation.
The interface for
.I tplfunc
is
.P
.RS 1i
.nf
\f(CWint
\fItplfunc\f(CW( attrvals, attrcnt, projlist )
char **attrvals;
int attrcnt;
struct qprojtuple *projlist;\fR
.fi
.RE
.IP
The tuple function must indicate whether to continue the query evaluation
or stop further evaluation
by returning either true (non-zero) or false (zero), respectively.
.SH EXIT CODES          \" strongly recommended
\fBsettplfunc\fR() returns the address of
any tuple function previously associated with the
query, or NULL if there wasn't one.
.P
The tuple function must indicate whether to continue the query evaluation
or stop further evaluation
by returning either true (non-zero) or false (zero), respectively.
.SH RESTRICTIONS        \" optional - includes caveats and bugs
Only one tuple function may be associated with a query.
.SH EXAMPLE             \" strongly recommended
Set up a tuple function which will print each retrieved attribute value.
Each attribute value should have a colon as a delimiter
except the last attribute,
which should have a new-line delimiter.
.P
.RS .5i
.nf
\f(CW#include "uquery.h"

int
writetplval( attrvals, attrcnt, projptr )
char **attrvals;
int attrcnt;
struct qprojtuple *projptr;
{
	register int i;

	for( i = 0; i < attrcnt; i++ )
		writeattrval( stdout, attrvals[i],
			i == attrcnt - 1 ? '\en' : ':' );
	return( TRUE );		/* always continue with the query */
}
 . . .
struct uquery *query;
struct qresult result;
 . . .
(void)settplfunc( query, writetplval );
queryeval( query, &result );\fP
.fi
.RE
.P
Set up the same tuple function again, but this time use the attribute's
normal delimiter in the output.
If the attribute is a fixed width attribute, no delimiter will be used.
.P
.RS .5i
.nf
\f(CW#include "uquery.h"

int
writetplval( attrvals, attrcnt, projlist )
char **attrvals;
int attrcnt;
struct qprojtuple *projlist;
{
	register int i;
	struct uattribute *attrptr;

	for( i = 0; i < attrcnt; i++ ) {
		attrptr = &projlist[i]->rel->rel->attrs[projlist[i]->attr];
		writeattrval( stdout, attrvals[i],
			i == attrcnt - 1 ? '\en' :
			(attrptr->flags & A_ATYPEMASK) == A_FIXEDWIDTH ? '\e0' :
			attrptr->terminate );
	}
	return( TRUE );		/* always continue with the query */
}
 . . .
struct uquery *query;
struct qresult result;
 . . .
(void)settplfunc( query, writetplval );
queryeval( query, &result );\fP
.fi
.RE
.SH EXTENDED DESCRIPTION    \" strongly recommended
.SS Projection Information Structures
While most applications need only reference the attribute values
within a tuple function,
there may occasionally be a need to access the tuple or projection information.
The structures accessible to a tuple function,
are defined in
.IR uquery.h
and
.IR urelation.h .
The structures of interest are:
\f(CWqprojtuple\fR, \f(CWqprojection\fR, \f(CWqnode\fR,
\f(CWurelation\fR, \f(CWuattribute\fR, and \f(CWutuple\fR.
.P
For each projected attribute, there is a
corresponding \f(CWqprojtuple\fR structure.
This structure serves only to point to the tuple information
and the projection information.
.P
The \f(CWutuple\fP structure contains information about each tuple.
The most interesting element in this structure is the
\f(CWtuplenum\fP, which contains each tuple's positional number in the original
relation.
.P
The \f(CWqprojection\fP structure gives information about each
projected attribute.
It gives the query node (i.e., relation) the attribute it is from,
the index of the attribute in the relation, and
print information about the attribute.
(This is mainly used to override the information in the attribute
structure itself.)
.P
The \f(CWqnode\fR structure contains information about
one ``query node''.
A query node is an instance of a relation;
the same relation may be used multiple times in one query.
Each query node points to the relation it represents, i.e.,
the \f(CWurelation\fP structure.
To access relation information about a projected attribute,
the following reference will suffice:
.P
.RS .5i
.nf
\f(CWstruct qprojection *projptr;
 . . .
projptr->rel->rel->...\fP
.fi
.RE
.P
See the
.BR getrelinfo (3)
manual page for details on the \f(CWurelation\fP and
\f(CWuattribute\fP structures.
.SH SEE ALSO            \" strongly recommended
cmpquery(1), nexttuple(3) and queryeval(3).
