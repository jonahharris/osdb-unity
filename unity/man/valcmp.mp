.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH valcmp "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
valcmp \- compile a validation table
.SH SYNOPSIS
\fIvalcmp\fP [-l] [-Itable] table [outfname]
.SH DESCRIPTION
The \fIvalcmp\fP program is used to compile a validation table that has
been created as described in the manual page for val(UNITY).  The filename
argument specified, table, must be the name of the UNITY table for which
the validation table has been created.  \fIvalcmp\fP assumes that the name
of the description file for the table is Dtable and that the name of
the validation table is Vtable.
If these files do not exist in the present working directory,
\fIvalcmp\fP looks for them in the directory in which the table resides,
and a directory named in the $UNITYDFILES environment variable path.
.P
The output is normally written to a
file named Wtable but may be written to another file by specifying another
file argument on the command line; this would normally be used by another
program executing \fIvalcmp\fP in such a way that if the program is
interrupted, Wtable does not exist in an invalid state of completion (i.e.,
the output is put into the temporary and then linked to Wtable).
.P
If the -I option is used, the table name specified is used to determine
an alternate description file and validation script name.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
A listing of the compiled output file is printed if the "-l" option is
specified.
The listing shows operations (arithmetic, logical, and
machine (such as PUSHS and HALT)), fields, and other elements in the
compiled output.  Regular expressions all appear as a '@'.
In general, it won't tell the casual user all that much.
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
.SH FILES
.nf
unity/valcmp.y - the yacc and C source program
bin/valcmp - the executable program
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIvalidate\fP(UNITY), \fIval\fP(UNITY)
