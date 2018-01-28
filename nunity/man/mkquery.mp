.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH mkquery 3 "" "New UNITY"
.SH NAME                \" required
.HP
mkquery, fmkquery, smkquery, vmkquery, vsmkquery \-
Make a query from relation names, attribute names, and a where-clause.
.SH SYNOPSIS            \" required
.nf
\fB#include "uquery.h"

struct uquery *
fmkquery( flags, relpaths, relcnt, attrnames, attrcnt,
	sortattrs, sortcnt, wherelist, wherecnt )
int flags;
char **relpaths;
int relcnt;
char **attrnames;
int attrcnt;
char **sortattrs;
int sortcnt;
char **wherelist;
int wherecnt;

struct uquery *
mkquery( relpaths, relcnt, attrnames, attrcnt, wherelist, wherecnt )
char **relpaths;
int relcnt;
char **attrnames;
int attrcnt;
char **wherelist;
int wherecnt;

struct uquery *
smkquery( flags, relpathstr, attrnamestr, sortstr, wherestr )
int flags;
char *relpathstr;
char *attrnamestr;
char *sortstr;
char *wherestr;

struct uquery *
vmkquery( flags, relcnt, attrcnt, sortcnt, wherecnt,
		relpath, ..., attrname, ..., sortattr, ..., wherearg, ... )
int flags;
int relcnt;
int attrcnt;
int sortcnt;
int wherecnt;
char *relpath, ...;
char *attrname, ...;
char *sortattr, ...;
char *wherearg, ...;

struct uquery *
vsmkquery( flags, relcnt, attrcnt, sortcnt,
		relpath, ..., attrname, ..., sortattr, ..., wherestr )
int flags;
int relcnt;
int attrcnt;
int sortcnt;
char *relpath, ...;
char *attrname, ...;
char *sortattr, ...;
char *wherestr;

void
unmkquery( query )
struct uquery *query;\fP
.fi
.SH DESCRIPTION         \" required - 1/2 page in length
The
.BR fmkquery ()
routine builds a query data structure suitable for use with
.BR queryeval (3),
based on the list of relation names,
projected and sorted attribute names, and
where-clause passed in.
The
.BR unmkquery ()
routine frees all the dynamically allocated memory
associated with the given query structure.
The additional routines
.BR mkquery (),
.BR smkquery (),
.BR vmkquery (),
and
.BR vsmkquery (),
are alternate interfaces to
.BR fmkquery ().
The
.BR mkquery ()
routine is provided for backward compatibilty.
It does not allow flags to specify additional query options
such as sorting and uniqueness of retrieved tuples,
and does not allow the specification of sort attributes.
With
.BR smkquery ()
the relation list, attribute lists, and where-clause
are given as single strings,
rather than as a list of individual arguments.
Each string is broken into individual arguments delimited by white space
and passed to
.BR fmkquery ().
With
.BR vmkquery ()
arguments are given directly in the function call, rather than in
character pointer arrays.
This interface uses the
.BR varargs (5)
facilities to assemble parameters into arrays for
.BR fmkquery ().
With
.BR vsmkquery ()
the relation list and attribute lists are given as individual
arguments, like
.BR vmkquery (),
while the where clause is given as a single string, like
.BR smkquery ().
.P
All of these functions return a pointer to a query structure on success
and NULL on failure.
Unlike other New UNITY commands,
.BR fmkquery ()
will print error messages using
.BR prmsg (3),
rather than setting an error code in the
.BR uerror
variable.
This is done so context information can be given in the error messages.
.P
.BR fmkquery ()
is a higher level interface to the
.BR getrelinfo (3),
.BR lookupattr (3),
and
.BR bldquery (3)
routines.
It is provided to avoid the necessity of knowing anything
about individual data structures in straight forward situations.
.P
.so Compile.h
.SH PARAMETERS          \" required - if command takes parameters - alphabetical order
.so Qflags.h
.P
.I relpaths
.PD 0
.TP
.I "relpath, ..."
A list of full or partial paths to the relations used in the query.
The list is either given as an array of character pointers
.RB ( fmkquery "() )"
or as individual paths given directly in the function call
.RB ( vmkquery ()
and
.BR vsmkquery "() )."
The number of relations is given in
.IR relcnt .
The relations need to exist and be readable.
.PD
.IP
An alternate relation description can be given for each relation
by appending an equals sign (=) and the alternate relation name
to the original relation path.
For example to interpret
.I tmpfile
like
.IR /etc/passwd ,
a relation name of ``\f(CWtmpfile=passwd\fP'' would be used.
.IP
The special relation name ``\-'' is interpreted as the standard input.
It may be given at most once in the relation names.
It must always have an alternate descriptor file associated with it.
For example, if the standard input is coming from
.IR /etc/passwd ,
the relation name ``\f(CW\-=passwd\fP'' would be used.
.TP
.I relcnt
The number of relations listed in the array
.I relpaths
or given directly as part of the function call in
.RI `` "relpath, ..." ''.
.TP
.I relpathstr
A string containing the white-space separated
path names to the relations used in the query.
The individual path names are interpreted the same as for the
.I relpaths
parameter above.
.IP
The
.I relpathstr
string will be altered by
.BR smkquery ()
to null terminate each relation path.
.P
.I attrnames
.PD 0
.TP
.I "attrname, ..."
The list of projected attributes for the query.
The list is either given as an array of character pointers
.RB ( fmkquery "() )"
or as individual attribute names given directly in the function call
.RB ( vmkquery ()
and
.BR vsmkquery "() )."
The number of projected attributes is given in
.IR attrcnt .
.PD
.IP
If no attributes are are given, then all attributes from all
relations
which don't have an explicit zero print width in the descriptor file
are retrieved.
If a single null attribute is given
(as either "" or NULL),
then no tuples will be retrieved,
but the existence of a tuple matching the given where-clause will be
indicated by whether
.BR queryeval (3)
returns true or false.
.IP
If an attribute is not unique among the queried relations, it must be
prefixed by the file name of the relation and a period (e.g.,
\f(CWpasswd.login\fP)
or by the positional number of the relation in
.IR relnames
and a period (e.g., \f(CW1.login\fP).
The positional numbering starts at one and goes through
.IR relcnt .
.IP
This same naming convention also applies to attribute references in the
where-clause.
.IP
Several special attributes are defined for any relation.
The ``\f(CWrec#\fP'' attribute refers to each tuple's positional number
within the relation.
The ``\f(CWseek#\fP'' attribute refers to file location where
each tuple starts.
The ``\f(CWall\fP'' attribute can be used in the projection list
as an abbreviation for all attributes of a relation.
.IP
Attribute names may have optional modifiers appended to
them to print width and justification.
Each modifier starts with a colon (``:'') followed by a
key word.
For example, to specify a ``count'' field with a print width of 15,
``\f(CWcount:15\fP'' would be used.
See the
.BR retrieve (1)
manual page for complete details on attribute modifiers.
It should be noted that access to the special attribute modifier
``value='' is controlled by a global variable ``uqpnvalue''
which is type integer (int) and must be set to ``QP_NEWVALUE''
in order to use (reference) it.
.IP
Attribute names may also be renamed in the qprojection
structure by appending ``as'' and the new name as the
next two entries in the array (or arguments)
after the attribute name.
For example, to rename the ``login'' field,
``login'', ``as'', ``Login Name'' would be placed in the
.I attrnames
array or as individual function arguments.
.TP
.I attrcnt
The number of projected attributes listed in the array
.I attrnames
or given directly as part of the function call in
.RI `` "attrname, ..." ''.
.P
.TP
.I attrnamestr
A string containing a list of the projected attributes, separated
by white space.
The individual attribute names are interpreted the same as for the
.I attrnames
parameter above.
The
.I attrnamestr
string will be altered by
.BR smkquery ()
to null terminate each attribute name.
.P
.I sortattrs
.PD 0
.TP
.I "sortattr, ..."
The list of attributes on which to sort the output of the query.
The list is either given as an array of character pointers
.RB ( fmkquery "() )"
or as individual attribute names given directly in the function call
.RB ( vmkquery ()
and
.BR vsmkquery "() )."
The number of sort attributes is given in
.IR sortcnt .
The individual attribute names are interpreted the same as for the
.I attrnames
parameter above.
.PD
.IP
If 
.B Q_SORT
is specified in
.I flags
but no sort attributes are are given,
then all projected attributes will be sorted on in the order
they were given.
.IP
Attribute names may have optional modifiers appended to
them to control the type of comparisons and sorting order.
Each modifier starts with a colon (``:'') followed by a
key word.
For example, to specify a ``count'' field as sorted numerically,
``\f(CWcount:numeric\fP'' would be used.
Sorting criteria may also be specified on the projected
attributes, but any sort attribute modifiers take precedence.
See the
.BR retrieve (1)
manual page for complete details on attribute modifiers.
.TP
.I sortcnt
The number of sort attributes listed in the array
.I sortattrs
or given directly as part of the function call in
.RI `` "sortattr, ..." ''.
.P
.TP
.I sortstr
A string containing a list of the attributes to sort on, separated
by white space.
The individual attribute names are interpreted the same as for the
.I attrnames
parameter above.
The
.I sortstr
string will be altered by
.BR smkquery ()
to null terminate each attribute name.
.P
.I wherelist
.PD 0
.TP
.I "wherearg, ..."
A list of strings representing the where-clause of the query.
Each string should be one argument as if typed on a command line
to the shell.
The list is either given as an array of character pointers
.RB ( fmkquery "() )
or as individual arguments given directly in the function call
.BR vmkquery "() )."
The number of arguments in the where-clause is given in
.IR wherecnt .
.PD
.IP
If any argument of the where-clause must contain white space, this
interface must be used to avoid mis-interpretation of the white space
as an argument delimiter in the where-clause.
.IP
.so Where3.u.h
.TP
.I wherecnt
The number of arguments in the where-clause given in the array
.I wherelist
or directly as part of the function call in
.RI `` "wherearg, ..." ''.
.IP
If
.I wherecnt
is less than or equal to zero,
no restriction is applied to the query and a cross product of all tuples in
all relations is retrieved.
.TP
.I wherestr
A string representing the where-clause of the query.
The individual arguments of the where-clause are delimited by white-space.
If any argument of the where-clause must contain white space,
.BR smkquery ()
and
.BR vsmkquery ()
cannot be used.
Instead, the where-clause must be given as an array or as individual arguments
to
.BR fmkquery ()
or
.BR vmkquery (),
respectively.
.IP
The
.I wherestr
string will be altered by
.BR smkquery ()
or
.BR vsmkquery ()
to null terminate each where-clause argument.
.IP
If
.I wherestr
is NULL or an empty string,
no restriction is applied to the query and a cross product of all tuples in
all relations is retrieved.
.IP
The syntax of
.I wherestr
is the same as for the
.I wherelist
described above.
.SH ENVIRONMENT         \" optional
.TP
UNITYDFILES
A colon-separated list of directories to search for the
relation descriptor file.
If this is not specified, or if the needed file is not found
in one of the directories, the current directory is
searched.
If that fails, the directory portion of each individual
.I relpath
is searched.
.IP
Some previous UNITY implementations only accept one
directory in this variable.
Also, they use it only after looking in the current
directory and in the relation's directory.
Some commands do
not use this variable at all.
.TP
UNITYDSEARCH
Any combination of lower or upper case 'c', 'd', and/or 'u' characters
which stand for "current directory", "data directory", and "UNITYDFILES",
respectively.  If UNITYDSEARCH is not set, then "ucd" is used as the
default search order for locating a descriptor file before checking
the actual data file itself.
Note that preference can be given to search the data directory first
(via the command line) when an alternate table name is given
that has a "/./" (full) or "././" (relative) path prefix or
when the alternate description and data file (table) names are the same.
.SH EXIT CODES          \" strongly recommended
A pointer to a query data structure is returned on success;
NULL is returned on failure and an explanatory error message will
have been printed using
.BR prmsg (3).
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
All relations must have less than \f(CWMAXATT\fP attributes, also.
.SH EXAMPLE             \" strongly recommended
Build a query that shows an employee, ``Jones'',
and the name of the department
he/she works in.
(We're assuming here that the
.I employee
relation does not give the department name directly,
but gives a department id.
The department id must be joined with the 
.I department
relation to get the department name.)
Sort the output uniquely on the department id.
First, do the query using
.BR fmkquery (3).
.P
.RS .5i
.nf
\f(CWchar *relnames[2];
char *attrnames[2];
char *sortattr[1];
char *where[10];
int relcnt, attrcnt, sortcnt, wherecnt;
struct uquery *query;

relnames[0] = "../employee";		/* relation 1 */
relnames[1] = "../department";		/* relation 2 */
relcnt = 2;
/*
 * The "name" attribute is not unique.  So we must include the relation
 * name or positional number in the attribute name.  For illustrative
 * puposes, we'll do it both ways.
 */
attrnames[0] = "employee.name";	
attrnames[1] = "2.name";		/* department is relation 2 */
attrcnt = 2;
sortattr[0] = "1.deptid";
sortcnt = 1;
where[0] = "employee.name";
where[1] = "=";
where[2] = "Jones";
where[3] = "and";
where[4] = "1.deptid"		/* employee is relation 1 */
where[5] = "f="		/* ``f'' => field-to-field compare, not fld-to-value */
where[6] = "department.deptid";
wherecnt = 7;

/*
 * Since we're supplying sort attributes, the query output
 * will be sorted even without the Q_SORT passed in in the
 * flags.  We're just including it here to be thorough.
 * However, since we want every tuple to be unique, Q_UNIQUE
 * must be passed in.
 */
query = fmkquery( Q_SORT|Q_UNIQUE, relnames, relcnt, attrnames, attrcnt,
		sortattr, sortcnt, where, wherecnt );
if ( query == NULL ) {
	/*
	 * Return failure.  An error message has already been printed
	 * using prmsg(3).
	 */
}\fP
.fi
.RE
.P
Now build the same query using
.BR smkquery ()
giving each part as a single string.
.P
.RS .5i
.nf
\f(CWstruct uquery *query;

query = smkquery( Q_SORT|Q_UNIQUE,	/* output sorted uniquely */
		"../employee ../department",	/* relations */
		"employee.name 2.name",		/* projected attributes */
		"1.deptid",			/* sort attributes */
						/* where-clause */
		"employee.name = Jones and 1.deptid f= department.deptid" );
if ( query == NULL ) {
	/*
	 * Return failure.  An error message has already been printed
	 * using prmsg(3).
	 */
}\fP
.fi
.RE
.P
Now build the query again using
.BR vmkquery ()
giving the individual arguments as part of the function
call itself.
.P
.RS .5i
.nf
\f(CWstruct uquery *query;

query = vmkquery( Q_SORT|Q_UNIQUE,	/* output sorted uniquely */
		2, 2, 1, 7,	/* relation, attribute, and where counts */
		"../employee", "../department",		/* 2 relations */
		"employee.name", "2.name",	/* 2 projected attributes */
		"1.deptid",			/* 1 sort attribute */
		"employee.name", "=", "Jones", "and",	/* 7 where args */
		"1.deptid", "f=", "department.deptid" );
if ( query == NULL ) {
	/*
	 * Return failure.  An error message has already been printed
	 * using prmsg(3).
	 */
}\fP
.fi
.RE
.P
Finally, use
.BR vsmkquery ()
to build the query
giving the where-clause as a single string.
.P
.RS .5i
.nf
\f(CWstruct uquery *query;

query = vsmkquery( 2, 2, 1,	/* relation and attribute counts */
		"../employee", "../department",		/* 2 relations */
		"employee.name", "2.name",	/* 2 projected attributes */
		"1.deptid",			/* 1 sort attribute */
		"employee.name = Jones and 1.deptid f= department.deptid" );
if ( query == NULL ) {
	/*
	 * Return failure.  An error message has already been printed
	 * using prmsg(3).
	 */
}\fP
.fi
.RE
.SH SEE ALSO            \" strongly recommended
.nh
bldquery(3), cmpquery(1), getrelinfo(3), lookupattr(3), prmsg(3),
queryeval(3), retrieve(1), setunpackenv(3), and uerror(3).
.hy
