.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH udisplay "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
udisplay, udis \- print description and/or data for a UNITY table
.SH SYNOPSIS
.nf
\fIudisplay\fP [-n] [-q] [-Itable] table [{\fID\fPpath|\fII\fPtable|\fIdes\fP|\fIdata\fP}]
.fi
.SH DESCRIPTION
\fIudisplay\fP is used to print (display) various information
about a UNITY table and its description.
The \fIudisplay\fP command hides the naming
conventions for tables and their associated description files.
.P
The default, when only the table name has been specified,
is to first print the line "The table description is:"
followed by the table description and then print the line
"The data for the table is:" followed by the data in the table.
If the -q option is used then "%description" is printed
in place of "The table description is:" and "%enddescription"
is printed in place of "The data for the table is:".
This allows the output of \fIudisplay\fP to be piped as input
to another UNITY command or simply viewed as text by the user
(as originally designed).
The -q option, in general, is used to supress printing of the
"The table description is:" and "The data for the table is:"
lines which, otherwise, make the output description and/or data
invalid for use by other UNITY commands.
.P
If \fID\fPpath is given as the second command line parameter,
then the path to the table description is printed.
If \fII\fPtable is given as the second command line parameter,
then the path to the table descrption is printed
with either a special "/./" (full) or "././" (relative)
path prefix and without the 'D' in the base filename.
This ouput, when using the "\fII\fPtable" parameter,
can be used (without modification) in the
"-Itable" option of any UNITY command
to specify that the given alternate table
be used to obtain the needed description file.
A "/./" or "././" prefix in the "-Itable"
option of any UNITY command indicates that the
specified data (description) directory be checked
first when searching for the description file.
.P
If "\fIdes\fPcription" is given as the second command line parameter,
then the contents of the description file is printed
without the initial "The table description is:" line
and without the pair of "%description"/"%enddescription" lines.
If \fIdata\fP is given as the second parameter,
the contents of the table is printed without the initial
"The data for the table is:" line.
.P
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
If the -n option is used, then the New Unity default search sequence
for locating the needed table description will be used.
New UNITY first checks any directories in the UNITYDFILES environment variable
before checking the current directory and finally the data directory.
Note that when UNITYDSEARCH has been set then this option is ignored
since both UNITY and New UNITY check if this environemnt variable
has been set for a specific search order to be used in finding
table description files.
.P
If the input table specified is "-", then input is taken from
the standard input instead of a table.
Normally, the -I option must be used to specify the name of a table from
which the description file name can be determined and used.
If the -I option is not given, the standard input stream is checked
for the description beginning with a \fI%description\fP and ending
with a \fI%enddescription\fP.  This allows the program to interface directly
with the output of New Unity commands, such as \fIretrieve(1)\fP,
that do not create intermediate description files.
Note that the -I option is not needed when \fIdata\fP has been
specified as the second command line parameter since,
in this case, the description file is not needed
(the data file is copied directly to standard output without
checking record content or format).
.P
\fIudis\fP is an abbreviation for \fIudisplay\fP to save on key-strokes.
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
.TP
UNITYUNPACK
A string of the form "command.suffix" where "command" is the the name of
a command that can be used to read (cat) the contents of a packed relation (file)
and "suffix" is the 1-3 character suffix (without the '.') that is appended to
the filename when the file has been packed (compressed).
Typical values for UNITYUNPACK are "pcat.z", "zcat.Z", and "gzcat.gz".
.IP
It should be noted that packed relations are not allowed as the target
of any type of insert, update, or delete operation.
In addition, using packed relations prevents the use of attribute indexes
and also prevents any packed tuples from being validated, see validate(UNITY).
See the New Unity setunpackenv(3) manual page for more information.
.SH EXAMPLE
.nf
.ne 12
$ udisplay SP
The table description is:
snumber t|      3l      SUPPLIER NUMBER
pnumber t|      3l      PART NUMBER
qty     t\\n    4r      QUANTITY
The data for the table is:
s1|p1|300
s1|p2|200
s1|p3|400
s2|p1|100
s2|p2|200
s3|p1|300

.ne 12
$ udisplay -q SP
%description
snumber t|      3l      SUPPLIER NUMBER
pnumber t|      3l      PART NUMBER
qty     t\\n    4r      QUANTITY
%enddescription
s1|p1|300
s1|p2|200
s1|p3|400
s2|p1|100
s2|p2|200
s3|p1|300

.ne 5
$ udisplay SP Dpath
DSP

$ udisplay SP Itable
\&././SP
.fi
.SH FILES
.nf
unity/udis.c, zdis.c - the C source program
bin/udisplay, udis - the executable programs (linked)
/tmp/DnoI<PID> - temporary description file when no -Itable option
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIretrieve\fP(NewUNITY), \fIuprint\fP(UNITY)
