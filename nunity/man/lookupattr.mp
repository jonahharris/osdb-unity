.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH lookupattr 3 "" "New UNITY"
.SH NAME                \" required
lookupattr, lookupprojattr, findattr \- Locate information about a particular attribute.
.SH SYNOPSIS            \" required
.nf
\fB#include "uquery.h"

int
lookupattr( attrname, nodelist, nodecnt, attrptr )
char *attrname;
struct qnode *nodelist;
int nodecnt;
struct attrref *attrptr;

int
lookupprojattr( attrname, nodelist, nodecnt, projptr, friendly )
char *attrname;
struct qnode *nodelist;
int nodecnt;
struct qprojection *projptr;
int friendly;

int
findattr( attrlist, attrcnt, attrname )
struct uattribute *attrlist;
int attrcnt;
char *attrname;\fP
.fi
.SH DESCRIPTION         \" required - 1/2 page in length
The
.BR lookupattr ()
routine is used to locate an attribute in a list of query nodes
(i.e., relations).
A pointer to an attribute reference (\f(CWattrref\fP) structure is
passed in for storing the query node pointer and the attribute index
on successfull look up.
True (non-zero) is returned on success; false (zero) on failure.
This routine is used mainly to look up an attribute reference as part of
the where-clause of a query.
.P
The
.BR lookupprojattr ()
routine is used to locate a projected attribute
in a list of query nodes (i.e., relations).
A pointer to an attribute projection (\f(CWqprojection\fP) structure is
passed in for storing the query node pointer, the attribute index,
the attribute's printed name,
and the attribute's print width
on successfull look up.
True (non-zero) is returned on success; false (zero) on failure.
This routine is used mainly to look up an attribute reference as part of
the projection list of a query.
.P
This routine will recognize "all:nodisplay=", when passed in as a single
parameter string, as the special ``all'' attribute regardless of
whether on not there is a "normal" attributed named "all".
.P
If
.I friendly
is true (non-zero) and a friendly name is given in the descriptor
information,
the attribute's printed name will be the friendly name; otherwise,
it will be the attributes official name.
.P
The
.BR findattr ()
routine is a low-level routine used to locate an attribute
within a relation's attribute list.
If
.I attrname
is found,
.BR findattr ()
returns the index of the attribute in
.IR attrlist .
If
.I attrname
is not found, minus one (-1) is returned.
Unlike, the above commands,
.I attrname
must contain only the attribute name \(em relation names,
print width and other modifiers (including ":nodisplay=") are not allowed.
.P
.so Compile.h
.SH PARAMETERS          \" required - if command takes parameters - alphabetical order
.TP
.I attrname
The name of an attribute to look up.
.IP
For
.BR findattr (),
this must be just the attribute name.
.IP
For
.BR lookupattr ()
and
.BR lookupprojattr (),
if an attribute is not unique among the given query nodes,
it must be prefixed by the file name of the relation it belongs to
and a period (e.g., ``\f(CWpasswd.login\fP'')
or the relation's positional index (starting at one) in the
.IR nodelist ,
and a period (e.g., ``\f(CW1.login\fP'').
If an attribute is unique among the query nodes,
this syntax is still recognized.
.IP
An optional colon and print width is allowed at the end of each
attribute name passed to
.BR lookupprojattr ()
(e.g., ``\f(CWlogin:12\fP'').
If given, the print width for the attribute will be this number.
If not given, the print width for the attribute will be that contained
in the descriptor information.
.IP
A special attribute ``\f(CWrec#\fP'', is recognized by both
\f(CWlookup\fP routines.
This attribute represents each tuple's positional number within its
original relation.
If this attribute is recognized, the attribute index in the
attribute reference or projection structure will be set
to \f(CWATTR_RECNUM\fP, as defined in
.IR uquery.h .
.IP
In addition to \f(CWrec#\fP,
.BR lookupprojattr ()
recognizes another special attribute, ``\f(CWall\fP''.
If the relation contains a normal attribute named "all"
then "all:nodisplay=" can be used to indicate the special ``all'' attribute.
This attribute is a short-hand for all attributes of a relation.
If this attribute is recognized, the attribute index in the projection
structure will be set
to \f(CWATTR_ALL\fP, as defined in
.IR uquery.h .
.so Nodelist.h
.TP
.I attrptr
A pointer to an \f(CWattrref\fP structure supplied
by the calling routine for storing the pointer
to the query node and the attribute index
upon successful lookup.
.TP
.I projptr
A pointer to an \f(CWqprojection\fP structure supplied
by the calling routine for storing the pointer
to the query node, the attribute index, the attribute's
printed name, and the attribute's print width
upon successful lookup.
.TP
.I friendly
A boolean flag saying whether the printed name for the attribute
should be the attribute's friendly name or the actual name.
If true (non-zero) and there is a friendly name in the descriptor information,
the printed name will be the friendly name; otherwise, the printed name
will be the attribute's actual name.
.TP
.I attrlist
An array of \f(CWuattribute\fP structures containing attribute descriptor
information for a relation.
The
.I attrname
will be matched against the name in each \f(CWuattribute\fP structure.
.TP
.I attrcnt
The number of \f(CWuattribute\fP structures in
.IR attrlist .
.SH EXIT CODES          \" strongly recommended
.BR lookupattr ()
and
.BR lookupprojattr ()
both return true (non-zero) on successful look up and false (zero)
on failure.
The variable
.B uerror
is set to contain the error code.
.P
.BR findattr ()
returns the appropriate attribute index upon success and
minus one (-1) on failure.
The variable
.B uerror
is not set on failure.
.SH EXAMPLE             \" strongly recommended
Look up the \f(CWlogin\fP attribute in a list of query nodes
(i.e., relations).
.P
.RS .5i
.nf
\f(CW#include "uquery.h"
\f(CW#include "message.h"

struct qnode *nodelist;
int nodecnt;
struct attrref attrinfo;
 . . .
if ( ! lookupattr( "login", nodelist, nodecnt, &attrinfo ) ) {
	pruerror( );
	prmsg( MSG_ERROR, "cannot locate login attribute in relations" );
	/* return failure */
}\fR
.fi
.RE
.P
Look up the \f(CWlogin\fP attribute in a list of relations as
a projected attribute.
We want to use the friendly names and have a print width of 12.
.P
.RS .5i
.nf
\f(CW#include "uquery.h"
\f(CW#include "message.h"

struct qnode *nodelist;
int nodecnt;
struct qprojection projinfo;
 . . .
if ( ! lookupprojattr( "login:12", nodelist, nodecnt, &projinfo, TRUE ) ) {
	pruerror( );
	prmsg( MSG_ERROR, "cannot locate login attribute in relations" );
	/* return failure */
}\fR
.fi
.RE
.P
Look up the \f(CWlogin\fP attribute in a relation's attributes.
.P
.RS .5i
.nf
\f(CW#include "uquery.h"
\f(CW#include "message.h"

struct urelation *relptr;
int attrindex;
 . . .

attrindex = findattr( relptr->attrlist, relptr->attrcnt, "login" );
if ( attrindex < 0 ) {
	prmsg( MSG_ERROR, "cannot locate login attribute in relation '%s'"
		relptr->path );
	/* return failure */
}\fR
.fi
.RE
.SH EXTENDED DESCRIPTION    \" strongly recommended
.SS Attribute Projection Information Structure
While most applications need only reference the attribute values
for projected attributes,
there may occasionally be a need to access the projection information.
The structures to do this,
as defined in
.IR uquery.h ,
are shown below:
.P
.RS .5i
.nf
\f(CWstruct qnode {
	...
	struct urelation *rel;	/* relation info for node */
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
It gives the node (i.e., relation) the attribute is from,
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
\f(CWstruct qprojection *projptr;
 . . .
projptr->rel->rel->...\fP
.RE
.P
See the
.BR getrelinfo (3)
manual page for details on the \f(CWurelation\fP and
\f(CWuattribute\fP structures.
.SS Attribute Reference Structure
Attribute reference structures are used almost exclusively
internally to the query libraries;
however, there may occasionally be a need to use these structures for
other puposes.
The structures to do this,
as defined in
.IR uquery.h ,
are shown below:
.P
.RS .5i
.nf
\f(CWstruct qnode {
	...
	struct urelation *rel;	/* relation info for node */
	...
};

struct attrref {
	struct qnode *rel;		/* node for the attribute */
	short attr;			/* attribute number */
};\fP
.fi
.RE
.P
The \f(CWattrref\fP structure identifies an attribute
by giving the query node and
the attribute index withing the node's attribute list.
A query node is an instance of a relation;
the same relation may be used multiple times in one query.
Each query node points to the relation it represents, i.e.,
the \f(CWurelation\fP structure.
To access relation information about an attribute reference,
the following code will suffice:
.P
.RS .5i
.nf
\f(CWstruct attrref *refptr;
 . . .
refptr->rel->rel->...\fP
.fi
.RE
.P
See the
.BR getrelinfo (3)
manual page for details on the \f(CWurelation\fP and
\f(CWuattribute\fP structures.
.SH SEE ALSO            \" strongly recommended
cmpquery(1), mkquery(3), parsewhere(3), and retrieve(1).
