.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH load "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
load \- load records between a UNIX file and a UNITY table
.SH SYNOPSIS
\fIload\fP [-q] [-rErrorLimit] [-Itable] data-file \fIto\fP table
.SH DESCRIPTION
The \fIload\fP command allows the user to load any UNIX file into a UNITY
table.
To execute the \fIload\fP command, the records in data-file
must have already been carefully set up to conform to the definition of
the table.
.P
If the "-Itable" option is used, the table name specified is used to determine
an alternate description file to be used in checking the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
The file is checked for conformity to the description file before
loading the table.
Due to the lack of numerical data types, the load command checks
only for the right number of attributes in each record.
Records are checked by counting past the number of characters specified
for a fixed width field and skipping to the termination symbol on a
terminator field.
If less than w characters are found for a fixed width field or
a terminator symbol is not found before a newline is encountered,
then an error message is printed indicating that the record is too short.
If additional characters are found before a newline is encountered,
then an error message is printed indicating that the record is too long.
All input records are checked before \fIload\fP terminates;
if any errors are detected, all records are left untouched in the input file.
.P
The "-rErrorLimit" option is used to change
the limit on the number of individual error messages
that are printed when errors are found in the input records.
If "ErrorLimit" is set to zero (0) then
and error summary message is printed without
printing any individual (per record) error messages.
The default value for "ErrorLimit" is one (1).
.P
This command will not read the standard input nor will it write
to the standard output (i.e., table names of "-" are not valid).
.P
If the "-q" option is specified, the program does not print out
the number of records that are loaded.
.br
.ne 5
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
unity/load.c, zload.c - the C source program
bin/load - the executable program
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fItable\fP(UNITY), \fIunload\fP(UNITY)
