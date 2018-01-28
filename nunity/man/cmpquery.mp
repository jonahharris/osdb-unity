.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH cmpquery 1 "" "New UNITY"
.SH NAME
cmpquery \- compile a \fBretrieve\fR query to embed in a C program
.SH SYNOPSIS
.na
.HP 10
.B cmpquery
[-chsVu]
.RI [-i <include_dir> ]
.RI [-o <output_file> ]
.I <query_name>
on
.br
.RI "[" <attr>\fR:\fP<modifiers> "... [as " <new_name> "] ...]"
.br
[sorted [by \fI<attr>\fR:\fI<modifiers>\fR ...]] [unique]
.br
from
.IR <tables> ...
.RI "[where " <where-clause> ]
.ad
.SH DESCRIPTION         \" required - 1/2 page in length
.B cmpquery
is a query compiler for translating a query suitable for use with the
.BR retrieve (1)
command into C code so it can be included directly in an application.
A query name is specified on the command line along with a query
expression that is translated.
The query expression includes the attributes to retrieve,
any modifiers for the attributes to specify sorting or uniqueness,
the file names of relations used in the query,
and a \fIwhere-clause\fP which stipulates the conditions of the query.
See the
.BR retrieve (1)
manual page for a complete description of each part of the query.
.P
The generated query is composed of two files:
a header file containing definitions for
all retrieved (i.e., projected) fields,
and a C source file containing data structures describing the query
and an initialization function for binding the query to the
relations at run-time.
It is possible to generate either the header file or the source file.
The default is to do both.
.SH PARAMETERS          \" required - if command takes parameters - alphabetical order
.TP
.B -c
Generate a source file for the query.
The source file declares the data structures that represent the
query and provides an initialization function for binding the query to
the relations at run time.
.IP
If neither the -c nor the -h parameter is specified, both a source file
and a header file will be generated.
.TP
.B -h
Generate a header file for the query.
The header has \f(CW#define\fRs for the attributes referenced in
the query, and an \f(CWextern\fR for the query data structure.
Each attribute name takes the
form: \fI<QUERY_NAME>_<ATTRIBUTE_NAME>\fR.
.IP
The symbol \fI<QUERY_NAME>\f(CW_MAXATT\fR is defined to be the
maximum attribute number.
.IP
If neither the -c nor the -h parameter is specified, both a source file
and a header file will be generated.
.TP
.B -i
The given
.I <include_dir>
is used as a prefix to included files in the generated output files,
i.e., ``#include "\fI<include_dir>\fR/uquery.h".
If
.I <include_dir>
is ``-'', then no #include will be generated in the output files.
This is useful when the generated files have access to
\fIuquery.h\fR through some other means (e.g. by being #include'd
in some other file).
.IP
Note that
.I <include_dir>
must immediately follow the
.BR -i ,
without any space in between.
.TP
.BI -o <output_file>
Place the source to the query in \fI<output_file>\fR (standard
output if a ``-'') rather than in \fI<query_name>\fR.c.
.TP
.B -s
The retrieved records should be sorted.
Normally, the output is not sorted and there may be multiple,
identical entries.
The sort order is normally left to right and is done
in ascending order using a character-by-character comparison.
(See
.B "sort by"
and attribute
.I <modifiers>
sections in the
.BR retrieve (1)
manual page
for how to modify the sort criteria.)
This option is the same as the
.B sorted
keyword.
.TP
.B -u
Only unique tuples should be retrieved in the query.
Normally multiple, identical entries may be retrieved.
Identical entries are records
where all the
.I displayed
attribute values are the same.
Normally, a character-by-character
comparison is used.
(See the attribute
.I <modifiers>
section in the
.BR retrieve (1)
manual page
for how to modify the comparison type.)
This option is the same as the
.B unique
keyword.
.IP
Note:  The output is not necessarily sorted with this option.
If both sorting and uniqueness are desired, use both \fB-s\fR
and \fB-u\fR.
.TP
.B -V
Print the version of the command and exit.
No compiled query is produced.
.TP
.I <query_name>
The name of the query.
This will be used as the base part of the source or header files
generated.
An upper-case version of the name is prefixed on the query's
attribute index definitions, as described above.
.IP
Query names must begin with a letter or an underscore, and can only
contain letters, digits, and underscores.
If the query name is used a a prefix to the output files, it should
be twelve or fewer characters long.
.TP
.BI "on \fR[\fP" "<attr>\fB:\fP<modifiers>\fR...\fP " [\fPas " <new_name>\fR]]...\fP
A projection list of attributes.
Only the given attributes will be returned in the query results.
If no attributes are given,
all attributes in all queried tables
which don't have an explicit zero print length are returned.
.IP
If a null attribute is given (that is, ""), the resulting query
will just report on the existence of a tuple, but won't return any value.
(See the \fBretrieve\fR(1) manual page for attribute name syntax.)
.IP
The \fI<modifiers>\fP are used to specify sorting criteria for
the \fI<attr>\fP.  Multiple modifiers may be given.
(See the \fBretrieve\fR(1) manual page for a complete description of
the supported modifiers.)
.IP
The \fI<new_name>\fP serves as a way to rename the
attribute index definition within the header file
to \fI<QUERY_NAME>_<NEW_NAME>\fP.
This is needed whenever multiple projected attributes
have the same name to disambiguate the \f(CW#define\fPs.
.TP
[\fBsorted\fR [\fBby\fR \fI<attr>\fR:\fI<modifiers>\fR ...]]
The retrieved records should be sorted on the given attributes.
Normally, the output is not sorted and there may be multiple,
identical entries.
If no sort attributes are given, the sort order is
left to right and is done
in ascending order using a character-by-character comparison.
The attribute
.I <modifiers>
can be used to modify the sort criteria.
(See the attribute
.I <modifiers>
section in the
.BR retrieve (1)
manual page
for more details.)
This option is the same as the
.B -s
option.
It is provided as an alternate syntax for use
when more detail is needed for the sort attributes or
using the flag is not possible or inappropriate, e.g., in makefiles.
.TP
.RB [ unique ]
Only unique tuples should be retrieved in the query.
Normally, multiple, identical entries may be retrieved.
Identical entries are records
where all the
.I displayed
attribute values are the same.
Normally, a character-by-character
comparison is used.
(See the attribute
.I <modifiers>
section in the
.BR retrieve (1)
manual page
for how to modify the comparison type.)
This option is the same as the
.B -u
option.
It is provided as an alternate syntax for use
when using the flag is not possible or inappropriate, e.g., in makefiles.
.IP
Note:  The output is not necessarily sorted with this option.
If both sorting and uniqueness are desired, use both \fBsorted\fR
and \fBunique\fR.
.TP
.RI  "\fBfrom\fR " <tables> ...
The file names of relations to be used in the query.
The
.B from
keyword is only required if projected attributes are specified.
The files need not exist at the time the query is compiled,
but the descriptor files must be readable.
.TP
.BI "[where " <where-clause> ]
.so Where.u.h
.IP
Within the \fIwhere-clause\fP any comparison value that starts with
a dollar sign (``$'') is considered to be a variable for the
query.
The ``$'' should be quoted to avoid interpretation by the shell.
These variables are used to have dynamic comparison
values used in the query.
Normally query variables are defined as external character arrays
within the query source file.
They must also be declared as global data and allocated space
within the application using the query.
A value must be copied into the array before the
query is evaluated.
(See the \fBEXAMPLE\fP for more information.)
.P
A special case for query variables is the set operator.
When only one comparison value is given for a set operator
and it is a query variable, this query variable is assumed
to be an array of character pointers, rather than a single
array of characters.
This is so all the comparison values for the set operation
can be assigned dynamically.
The application must still allocate space for the pointer array,
as with other query variables.
The list of comparison values must be terminated by a NULL pointer.
The application must also make sure the comparison values
line up appropriately when multiple attributes are given with
the set operator.
.P
A single query variable cannot be used with the set operator
when the comparison attributes come from different relations.
This is an implementation restriction, due to the way such
set operators are evaluated.
.B cmpquery
will report an error and fail if this occurs.
.SH ENVIRONMENT         \" optional
.so Environ.u.h
.SH EXIT CODES          \" strongly recommended
.so Exit.codes.h
.SH RESTRICTIONS        \" optional - includes caveats and bugs
.so Restrict.h
.P
The attribute limit for individual tables applies also
to the the total number of projected attributes.
Any attributes beyond this limit are ignored.
.P
Care should be taken that query variables are unique
across all queries used in an application, since
they must be global to the application.
.SH EXAMPLE             \" strongly recommended
Compile just the query source file for a query to list the
names of all employees who
work in the toy department.
The query name should be ``toydept.''
.P
.RS .5i
.nf
\f(CWcmpquery -c toydept on empname from employee	\e
	where dept = toy\fP
.fi
.RE
.P
If the header file for this query had been generated, it would contain:
.P
.RS .5i
.nf
\f(CW#define TOYDEPT_EMPNAME	0

\f(CW#define TOYDEPT_MAXATT	1

extern struct uquery toydept;\fR
.fi
.RE
.P
The generated source file would contain the following:
.P
.RS .5i
.nf
\f(CW#include "uquery.h"

struct uquery toydept = {
	. . .
};

int
inittoydept( r1_employee )
struct urelation *r1_employee;
{
	. . .
};\fP
.fi
.RE
.P
Compile the header and source file for a query to get the name,
department, and salary
of employees that work for a particular manager.
The manager's name will be a variable.
The name of the query should be ``manager''.
.P
.RS .5i
.nf
\f(CWcmpquery manager on 1.empname 1.dept 1.salary	\e
	from employee employee department		\e
	where 2.name req '$q_mngrname'			\e
		and 2.title = manager			\e
		and 2.empid f= department.mngrid	\e
		and 1.dept f= department.deptno\fP
.fi
.RE
.P
The generated header file for this query would contain:
.P
.RS .5i
.nf
\f(CW#define MANAGER_EMPNAME	0
\f(CW#define MANAGER_DEPT	1
\f(CW#define MANAGER_SALARY	2

\f(CW#define MANAGER_MAXATT	3

extern struct uquery manager;\fR
.fi
.RE
.P
The generated source file would contain the following:
.P
.RS .5i
.nf
\f(CW#include "uquery.h"

struct uquery manager = {
	. . .
};

extern char q_mngrname[];

int
initmanager( r1_employee, r2_employee, r3_department )
struct urelation *r1_employee;
struct urelation *r2_employee;
struct urelation *r3_department;
{
	. . .
};\fP
.fi
.RE
.P
Below is a code segment showing a sample use of this last query within
an application.
(See the \fBEXTENDED DESCRIPTION\fP section and the
references in the \fBSEE ALSO\fP section
for more information.)
.P
.RS .5i
.nf
\f(CW#include "uquery.h"
\f(CW#include "manager.h"

char q_mngrname[40];		/* query variable for manager query */

struct urelation r_employee;
struct urelation r_department;
struct qresult result;
char *attrvals[MANAGER_MAXATT];
 . . .

if ( ! getrelinfo( "employee", &r_employee, FALSE ) ) {
	pruerror( );
	return( FALSE );
}
if ( ! getrelinfo( "department", &r_department, FALSE ) ) {
	pruerror( );
	return( FALSE );
}
if ( ! initmanager( &r_employee, &r_employee, &r_department ) ) {
	pruerror( );
	prmsg( MSG_INTERNAL, "cannot initialize manager query" );
	return( FALSE );
}
 . . .

strcpy( q_mngrname, "Jones" );
if ( ! queryeval( &manager, &result ) ) {
	if ( uerror )		/* something went wrong */
		pruerror( );
	return( FALSE );
}
if ( ! initresult( &result ) ) {
	pruerror();
	return( FALSE );
}
while( nexttuple( &manager, &result, attrvals ) ) {
	/*
	 * Do something with the attribute values:
	 *	empname = attrvals[MANAGER_EMPNAME]
	 *	dept = attrvals[MANAGER_DEPT]
	 *	salary = attrvals[MANAGER_SALARY]
	 */
}
freeresult( &result );\fR
.fi
.RE
.SH EXTENDED DESCRIPTION    \" strongly recommended
.SS Generated Source File Contents
The generated C-code for a query is composed of two parts within the
source file.
The first is the data structure for the query itself.
The second is an initialization function for the query.
This initialization function serves to bind the query to
the relations used in the query at run time and to initialize
data within the query itself that cannot be done statically
(i.e., union information).
.P
The initialization function takes as arguments pointers to
the relation structures for each relation used in the query
in the same order as mentioned on the \fBcmpquery\fP
command line.
The initialization function must be called once, before
the query is used.
If it is called multiple times, however, no harm will be done.
.SS Building Commands Which Use the New UNITY
In order to compile a generated query,
the header files
.IR uquery.h
and
.IR urelation.h
must be accessible to the C compiler.
Normally, these files are in the
.I $TOOLS/lib/nunity/hdr
directory.
So, to compile the generated query, the following command
line can be used:
.nf
.IP
\f(CWcc -c -I$TOOLS/lib/nunity/hdr \fI<query_name>\fP.c\fR
.fi
.P
In order to link a command which uses any of the new UNITY
functionality, three libraries are required:
.IR $TOOLS/lib/nunity/lib/libunity.a ,
.IR $TOOLS/lib/nunity/lib/libmisc.a ,
and the Programmers' Workbench library (\f(CW-lPW\fR).
The \fBmalloc\fR(3X) library is also recommended (\f(CW-lmalloc\fR),
but not required.
The
.I libunity.a
library contains the new UNITY functions for querying and
altering a database.
The
.I libmisc.a
library contains miscellaneous functions used by
.IR libunity.a ,
but which are not directly related to databases
(e.g.,
printing messages,
checking file permissions,
and
stripping directory and file names from path names).
So, to link a command which uses the new UNITY, the following
command line can be used:
.nf
.IP
\f(CWcc -o\fI<command>\fP \fI<query_name>\fP.o \fI<other_objs>\fP...  \e
	$TOOLS/lib/nunity/lib/libunity.a  \e
	$TOOLS/lib/nunity/lib/libmisc.a  \e
	\fI<other_libs>\fP... -lPW -lmalloc\fR
.fi
.SH FILES               \" optional 
.TP 10
.IR <query_name> .c
The default name for the source file generated containing the query
data.
.TP
.IR <query_name> .h
The header file generated containing query-related \f(CW#define\fRs
and \f(CWextern\fRs.
.SH SEE ALSO            \" strongly recommended
getrelinfo(3), queryeval(3), nexttuple(3), set_tplfunc(3), retrieve(1)
and
.RI `` "An Improved and Extended Implementation of the UNITY Data Base Management System" '',
E. D. Scott,
July 25, 1989,
55634-072589-01TM.
