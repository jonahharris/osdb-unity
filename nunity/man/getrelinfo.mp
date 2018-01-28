.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH getrelinfo 3 "" "New UNITY"
.SH NAME                \" required
getrelinfo \- Locate and read in attribute information for a relation.
.br
freerelinfo \- Free memory associated with relation.
.SH SYNOPSIS            \" required
.nf
\fB#include "urelation.h"

struct urelation *
getrelinfo( relpath, relptr, savedpath )
char *relpath;
struct urelation *relptr;
int savedpath;

void
freerelinfo( relptr )
struct urelation *relptr;\fP
.fi
.SH DESCRIPTION         \" required - 1/2 page in length
The
.BR getrelinfo ()
routine returns the attribute information for
.I relpath
by locating and reading the descriptor file for \fIrelpath\fR.
The descriptor file's name will be \f(CWD\fIrelname\fR,
where
.I relname
is the file name portion of
.IR relpath .
The relation information is placed in the \f(CWurelation\fP structure
pointed to by
.IR relptr .
A pointer to the relation information structure is returned on success,
or NULL on failure.
.P
A pointer to the descriptor file path will be saved if
.I savedpath
is true (non-zero).
.P
The
.BR freerelinfo ()
routine frees all memory allocated with
.BR getrelinfo ().
.P
.so Compile.h
.SH PARAMETERS          \" required - if command takes parameters - alphabetical order
.TP
.I relpath
A path name to the relation which attribute information
is needed for.
This can be a full path, a partial path, or just the file name
of the relation.
The file need not exist, either, since only descriptor information
is read in.
The descriptor file is located using the UNITYDFILES
environment variable.
.IP
The
.I relpath
can also be of the form
.IP
.nf
	<table>=<alt_table>
.fi
.IP
In this case the description information is located based on
.IR alt_table ,
but the relation's data path is set to
.IR table .
If
.IR alt_table
is specified and it contains a "/./" (full) or "././" (relative)
path prefix, or
.IR alt_table
is the same as
.IR table,
then the directory given (if any) as a prefix to
.IR alt_table
is search first for the description information
before checking any other directories as specified
by the UNITYDSEARCH environment variable
(see ENVIRONMENT Section).
.TP
.I relptr
A pointer to a relation structure where attribute information
will be placed.
If this parameter is NULL, a structure will be allocated using
.BR malloc (3).
A pointer to this structure is returned on success.
.TP
.I savedpath
Flag saying whether to save the path to the descriptor file or not.
If true (non-zero) the path to the descriptor file will be saved
in the relation structure.
Space for the path name will be allocated using
.BR malloc (3).
.SH ENVIRONMENT         \" optional
.TP
UNITYDFILES
A colon-separated list of directories to search for the
relation descriptor file.
If this is not specified, or if the needed file is not found
in one of the directories, the current directory is
searched.
If that fails, the directory portion of
.I relpath
is searched.
.IP
Some previous UNITY implementations only accept one
directory in this variable.
Also, they use it only after looking in the current
directory and in the relation's directory.
Some commands do not use the variable at all.
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
A pointer to the \f(CWurelation\fP
structure is returned on success.
NULL is returned on failure and the external variable \f(CWuerror\fP
will contain the error code.
.SH EXAMPLE             \" strongly recommended
Read the relation information for the
.I /etc/passwd
file.
Allocate a structure for the information dynamically.
Do not store the path to the descriptor file.
.P
.RS .5i
.nf
\f(CW#include "urelation.h"

struct urelation *relptr;

relptr = getrelinfo( "/etc/passwd", NULL, FALSE );
if ( relptr == NULL )
{
	(void)pruerror( );
	/* print application message and return failure */
}\fP
.fi
.RE
.P
Read the relation information for the file
.IR tmppwd ,
which has the same description as
.IR /etc/passwd .
Use a statically allocated relation structure.
Do not store the path to the descriptor file.
.P
.RS .5i
.nf
\f(CW#include "urelation.h"

struct urelation pwdrel;

if ( getrelinfo( "tmppwd=passwd", &pwdrel, FALSE ) == NULL )
{
	(void)pruerror( );
	/* print application message and return failure */
}\fP
.fi
.RE
.SH EXTENDED DESCRIPTION    \" strongly recommended
.SS Relation and Attribute Structures
The \f(CWurelation\fP structure returned from
.BR getrelinfo ()
is defined in
.IR urelation.h .
.P
The descriptor information about each attribute is contained in the
\f(CWuattribute\fP structure defined in the same file.
.SH FILES               \" optional 
.TP
\fBD\fIrelname\fR
\- Descriptor file for \fIrelname\fR, where
.I relname
is the file name portion of the
.IR relpath
parameter .
This file lists the relation's
attributes, their delimiting-character or width,
printing information, and user-friendly names.
.SH SEE ALSO            \" strongly recommended
.nh
cmpquery(1), descriptor(4), and setunpackenv(3).
.hy
