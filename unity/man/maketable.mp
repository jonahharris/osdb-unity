.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH maketable "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
maketable \-  interactive UNITY table definition
.SH SYNOPSIS
\fImaketable\fP [-v] table
.SH DESCRIPTION
The \fImaketable\fP command allows the user to define a table by giving
the table name and interactively entering a list of attributes and
the associated information.
The table specified may exist in any directory
accessible to the user and the user may reference it by supplying
either a full, relative, or simple pathname
for the table (this is true for all UNITY commands).
The \fImaketable\fP command creates a description file for the
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
Attribute Type/Terminator:
The two supported data types for attributes
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
Print Information:
The print information consists of the column width and print justification
for the attribute to be used by the \fIuprint\fP(UNITY) program.
The column width is an integer width and may optionally be immediately
followed by a justification character.
The allowed justification characters are 'l' for left justified, 'r' for
right justified, and 'c' for centered output.
.TP
Verbose Name:
The verbose name for the attribute is
used by various UNITY programs to provide a more "user-friendly"
name when printing data or prompting for input.
The name may be any length and may not contain a tab.
Only 80 characters are stored internally
and, on output, some programs print only 30 characters.
.P
The attribute name and attribute type fields are required.
The print information and verbose name are optional.
Further details on the format of the description file
are given in the EXAMPLE section of this manual page.
.P
The \fIuenter\fP(UNITY) program is used for data entry.
It prompts for each field name and indicates the beginning of new
records (new attributes).
The user cannot enter non-printable characters, tilde, or carrot,
and continuation lines are not allowed
(the -c option is specified to \fIuenter\fP).
Input mode is terminated when control-d is entered.
At this point, the user is asked if the input should be updated
in the editor; a response of 'y' will place the user in the
editor (default editor is \fIed\fP but it can be changed by setting
the environment variable ED to another editor).
The final prompt is whether or not to update (in this case, create)
the description file.
.P
The \fImaketable\fP program, by default, does not do any validation
of the input.
Specifying the -v flag will cause the \fIvalidate\fP(UNITY) program
to be invoked with a validation script generated automatically
by the \fImaketable\fP command.
All input fields are validated.
The only validation that is not done is to ensure that, if the last
field is a terminator field, the terminator is a newline.
.P
When accessing a UNITY table, the default search sequence for
all UNITY programs is to first look for the
description file in the present working directory (name composed of the
simple file name of the table prefixed with a 'D').  If the file does not
exist, then the program looks for a file with the same name in the
directory in which the table is located.
If the description file is not found, it will then look at the environment
variable UNITYDFILES to see if a colon-separated list of
directories is specified, one of which contains the description file.
Thus, all description files can be stored in one or more common directories
by use of the UNITYDFILES environment variable.
When description files are created by a UNITY command
(e.g., \fIujoin\fP, \fIuselect\fP, etc.),
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
a description file must be created using \fImaketable\fP.
The description file, "Dpasswd," is created in the present working directory.
Note that a description file for any table
named "passwd" will be named "Dpasswd" so that multiple tables with
the same simple file name can be accessed using the same description file.
The command
.sp
.nf
maketable passwd
Terminate insertion with EOF (ctrl-d)

New Record
Attribute Name: login
Attribute Type/Delimiter:       t:
Print Format:   8l
Verbose Name:   LOGIN NAME

New Record
Attribute Name: passwd
Attribute Type/Delimiter:       t:
Print Format:   14l
Verbose Name:   PASSWORD

New Record
Attribute Name: uid
Attribute Type/Delimiter:       t:
Print Format:   5r
Verbose Name:   USER ID

New Record
Attribute Name: gid
Attribute Type/Delimiter:       t:
Print Format:   5r
Verbose Name:   GROUP ID

New Record
Attribute Name: acct
Attribute Type/Delimiter:       t:
Print Format:   20l
Verbose Name:   ACCOUNTING INFORMATION

New Record
Attribute Name: home
Attribute Type/Delimiter:       t:
Print Format:   15l
Verbose Name:   HOME DIRECTORY

New Record
Attribute Name: shell
Attribute Type/Delimiter:       t\\n
Print Format:   12l
Verbose Name:   LOGIN SHELL

New Record
Attribute Name: <control-d>
Enter editor to make changes (y or n) n
tuple: 7 records converted
Make these changes in Dpasswd (y or n) y
insert: 7 records inserted in Dpasswd.

would produce the following Dpasswd file in the current
directory:
.ne 7
login   t:      8l      LOGIN NAME
passwd  t:      14l     PASSWORD
uid     t:      5r      USER ID
gid     t:      5r      GROUP ID
acct    t:      20l     ACCOUNTING INFORMATION
home    t:      15l     HOME DIRECTORY
shell   t\\n     12l     LOGIN SHELL

.fi
.sp
Note that the sections of white space in the output description file
are tabs.
Thus, the description file is a four\-column, tab\-separated table.
The last two columns are optional but the print column must
appear if the user\-name column appears.
The description file may be created using a text editor, if desired,
instead of using the \fImaketable\fP command.
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
unity/maketable.c, zmaketable.c - the C source program
bin/maketable - the executable program
./D<table> - the description file created
./DD<table> - the description file for the description file
./VD<table> - the validation script for the description file
./ED<table> - the error message table for the description file
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIuenter\fP(UNITY), \fIuprint\fP(UNITY), \fItable\fP(UNITY),
\fIvalidate\fP(UNITY)
