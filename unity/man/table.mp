.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH table "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
table \-  define table name, attributes, and data types
.SH SYNOPSIS
\fItable\fP table aname1 atype1 [...] 
.SH DESCRIPTION
The \fItable\fP command allows the user to define a table by giving
the table name and a list of attributes (aname1, aname2,...) with their
respective data types (atype1, atype2,...).
The table specified may exist in any directory
accessible to the user and the user may reference it by supplying
either a full, relative, or simple pathname
for the table (this is true for all UNITY commands).
The \fItable\fP command creates a description file for the
defined table with a name composed by prefixing the simple
file name of the table with a 'D'.
The description file is created in the present working directory
and must be present for execution of all other UNITY commands.
.TP
Attribute Names:
Attribute names may be a maximum of 31 characters in length; they
must begin with an alphabetic character or underscore ("_"), and may
contain only alphanumeric characters and "_" (i.e., A-Z, a-z, 0-9, _).
.TP
Attribute Types:
The two currently supported data types for attributes
are fixed-length character strings
and variable-length character strings ended with a terminator.
A fixed length string is indicated by a type of "w" (width) followed by
the width of the attribute.  A variable-length string is indicated by a
type of "t" (terminator) followed by the terminator for the attribute.
If the last attribute for a table is a terminator field, then its terminator
\fImust\fP be a newline (all records must end with a newline).
All character constant escape sequences recognized in the C language
(newline, horizontal tab, backspace, carriage return, form feed, backslash,
single quote, and bit pattern) are recognized for terminator characters
(e.g., the strings "\\t" and "\\n" are recognized as tab and
newline characters, respectively, for terminator characters).
.TP
Optional Print Information:
The user may optionally enter the print information
(i.e., the column width and print justification) for the attribute
to be used by the \fIuprint\fP(UNITY) program directly with the \fItable\fP
command by inputing the attribute type and the print information
separated by a tab as a single parameter for the \fIatype\fP of an attribute
(quotes are necessary so that it will be taken as a single parameter).
Further information about the use of the print information can be found
on the \fIuprint\fP(UNITY) manual page.
.TP
Optional User-Friendly Name:
The user may optionally enter a more "user\-friendly" name for
the attribute to be used by printed by various
UNITY programs.
If entered, the print information is NOT optional; it is needed
as a positional parameter.
Thus, the user name is treated as a third part of the attribute type
and must be separated by a tab from the print information
(quotes are necessary so that it will be taken as a single parameter).
The name may be any length and may not contain a tab.
Only 80 characters are stored internally
and, on output, some programs print only 30 characters.
.P
The format for specifying an attribute type is then
.sp
.nf
.ce
{w<len> | t<term>}[<tab>[width][just][<tab>username]]
.sp
.fi
For example, attribute type
.sp
.nf
  "t|        10l     Friendly name"

.fi
where the sections of white space are tabs would specify a terminator
field (terminated with |), print width 10 and left justification
with user name "Friendly name".
Both the print information and user\-friendly name are optional.
.P
In some situations, it is necessary to allow users to specify their own
description by which records in the table are interpreted.
A simple example of a case where this is useful
is the specification of a date:
one user may want to access a date as three two-character fields (e.g., month,
date, and year) while another user may reference it as a single six-character
quantity.
In other situations, it is advantageous to have one single description file
that is accessed by all users (i.e., for a single, centrally-located table).
.P
When accessing a UNITY table, the default search sequence for all
UNITY programs is to first look for the
description file in the present working directory (name composed of the
simple file name of the table prefixed with a 'D').  If the file does not
exist, then the program looks for a file with the same name in the
directory in which the table is located.
If the description file is not found, it will then look at the environment
variable UNITYDFILES to see if a colon-separated list of
directories is specified, one of which contains the description file.
Thus, all description files can be stored in one or more common directories
by use of the UNITYDFILES environment variable.
When description files are
created by a UNITY command (e.g., \fIujoin\fP, \fIuselect\fP, etc.),
they are created in the present working directory.
This default search sequence can be modified via the UNITYDSEARCH
environment variable (see ENVIRONMENT section below).
.P
Lines in the description file beginning with a pound sign (#) are ignored.
They may be added using any available text editor and used as
comment lines (e.g., to hold SCCS what strings).
.SH ENVIRONMENT
.TP
.B UNITYDFILES
A colon-separated list of directories to search for
table description files after checking the
current directory and the data file directory.
.TP
.B UNITYDSEARCH
Any combination of lower or upper case 'c', 'd', and/or 'u' characters
which stand for "current directory", "data directory", and "UNITYDFILES",
respectively.  If UNITYDSEARCH is not set, then "cdu" is used as the
default search order for locating a description file.
Note that preference can be given to search the data directory first
(via the command line) when an alternate table name is given
that has a "/./" (full) or "././" (relative) path prefix or
when the alternate description and data file (table) names are the same.
.SH EXAMPLE
To use the /etc/passwd file as a UNITY table,
a description file must be created using \fItable\fP.
The description file, "Dpasswd," is created in the present working directory.
Note that a description file for any table
named "passwd" will be named "Dpasswd" so that multiple tables with
the same simple file name can be accessed using the same description file.
The command
.sp
.nf
table /etc/passwd login  "t:      8l    LOGIN NAME" \\
        passwd "t:      14l     PASSWORD" \\
        uid    "t:      5r      USER ID" \\
        gid    "t:      5r      GROUP ID" \\
        acct   "t:      20l     ACCOUNTING INFORMATION" \\
        home   "t:      15l     HOME DIRECTION" \\
        shell  "t\\n     12l    LOGIN SHELL" \\

would produce the following Dpasswd file in the current
directory:

login  t:      8l       LOGIN NAME
passwd t:      14l      PASSWORD
uid    t:      5r       USER ID
gid    t:      5r       GROUP ID
acct   t:      20l      ACCOUNTING INFORMATION
home   t:      15l      HOME DIRECTORY
shell  t\\n     12l     LOGIN SHELL
.fi
.sp
Note that the sections of white space in the output description file
are tabs.
Thus, the description file is a four\-column, tab\-separated table.
The last two columns are optional but the print column must
appear if the user\-name column appears.
The description file may be created using a text editor, if desired,
instead of using the \fItable\fP command.
All characters following the user\-friendly name (i.e., if a tab
follows the name) are ignored.
.SH CAVEAT
Most UNITY programs do NOT use the "user\-friendly" names
and thus do not keep the names.
Instead they are thrown away as they are read.
This cuts down on the amount of space needed by each program.
As a result, some UNITY programs that create new description
files on output DO NOT write out the user\-friendly names
as part of the new description.
In particular,
any "user\-friendly" names are LOST when a new description file is created
(when an "into" clause is specified)
by \fIuselect\fP when projection is done,
and by \fIujoin\fP, \fIouterjoin\fP, and \fIregjoin\fP.
They can be added to the description file later if needed by the user with a
text editor.
.SH FILES
.nf
unity/table.c, ztable.c - the C source program
bin/table - the executable program
./D<table> - the description file created
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIuprint\fP(UNITY)
