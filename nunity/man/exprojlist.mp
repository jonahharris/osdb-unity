.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH exprojlist 3 "" "New UNITY"
.SH NAME                \" required
exprojlist \- Expand references to the ``all'' attibute in a projection list.
.SH SYNOPSIS            \" required    
.nf
\fB#include "uquery.h"

int
exprojlist( query, friendly )
struct uquery *query;
int friendly;\fP
.SH DESCRIPTION         \" required - 1/2 page in length
The
.BR exprojlist ()
routine is used to expand any references to the special ``all''
attribute within the projection list of a query.
This attribute is replaced by all attributes of a relation in
the order they appear in the relation.
In this case,
.BR malloc (3)
is used to allocate space for new projection information.
If there are no references to the ``all'' attribute,
then the projection list is unchanged.
.P
This routine is normally called by
.BR bldquery (3)
or if the query is compiled, by the query's initialization function.
It only need be called explicitly in situations where
explicit control is needed, such as when using
.BR _bldquery (3)
instead.
.P
.so Compile.h
.SH PARAMETERS          \" required - if command takes parameters - alphabetical order
.TP
.I query
A UNITY query previously built using
.BR bldquery (3)
or
.BR cmpquery (1).
If there are no references to the ``all'' attribute in the
projection list of the query, the query will remain unchanged.
.TP
.I friendly
A boolean flag saying whether the user-friendly names for attributes
should be used as the printed names for any expanded attributes.
.SH EXIT CODES          \" strongly recommended
.BR exprojlist ()
returns true (non-zero) on success
and false (zero) on failure.
It is not an error if there are no references to the ``all'' attribute.
.SH EXAMPLE             \" strongly recommended
Expand the projection list after building a query using
.BR _bldquery ().
.P
.RS .5i
.nf
\f(CWquery = _bldquery( projlist, projcnt, nodelist, nodecnt, queryexpr, FALSE );
if ( query == NULL ) {
	pruerror();
	/* print error message and return failure */
}
 . . .

if ( ! exprojlist( query ) ) {
	pruerror();
	/* print error message and return failure */
}\fP
.fi
.RE
.SH SEE ALSO            \" strongly recommended
bldquery(3) and cmpquery(1).
