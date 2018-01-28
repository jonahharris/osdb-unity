.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH tuple UNITY "" "UNITY User's Manual"
.SA 1
.SH NAME
tuple \- converts data from packet to tuple format
.SH SYNOPSIS
.nf
\fItuple\fP [-c] [-q] [-Itable] [-Otable] [-n[<newline>]] [-u] table1 \\
 [\fIinto\fP table2]
.fi
.SH DESCRIPTION
\fItuple\fP converts data in stream packet format into
the tuples required for a UNITY table.
\fItuple\fP reads a packet stream from the input table,
matches the attribute names in the packet against the names in
the description file associated with the packet file,
and converts the attribute values into tuple format.
.P
Attributes in the packet must come in the order they are listed in the
description file.
Not all attributes in the table need be present in the packet; missing
attributes will be created.
Missing fixed width attributes will be blank filled.
Missing variable width attributes will have a null value.
All attributes in the packet, however, must be listed in the description file;
if not, an error is generated.
.P
If the "-u" option is specified, then the program expects
to find the "user\-friendly" names instead of the attribute names
in the packets, if they exist in the description file (see \fItable\fP(UNITY)).
If the "user\-friendly" names do not exist, then the program
matches against the attribute names.
This option is necessary to read packets created by a
\fIpacket\fP command run with the "-u" option.
.P
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
\fItuple\fP performs a limited amount of validation on the data when converting
it to tuple format.
The program will check the length of the fixed width attributes,
flagging attributes of incorrect length as errors.
The values of variable length or terminator type attributes
are checked to insure they do not contain the terminating character
(the appropriate attribute terminator will be added by \fItuple\fP)
Data containing unprintable characters or ones with special meaning
(the UNITY "newline" character as discussed below)
will be flagged with an error.
.P
The output is written to the standard output by default.
If an "into clause" is specified, the output is written into
the named table and a corresponding descrition file is created in the
current directory.
An error is printed if the output table already exists.
If \fIonto\fP is specified instead of \fIinto\fP, the output is
appended to the table if it already exists or the table created
and written if it does not exist.
.P
If the input table specified is "-", then input is taken from
the standard input instead of a table.
The -I option must be used to specify the name of a table from
which the description file name can be determined and used.
.P
If the output table name specified is "-", then the table output will
be written to the standard output.
Normally, the -O option must be used to specify the name of a table from
which the output description file name can be determined.
However, the -c option can be used instead to copy the description
to standard output starting with \fI%description\fP and ending with
\fI%enddescription\fP.
This allows the program to be used as an intermediate filter between
various UNITY or New Unity commands without having to create/cleanup
intermediate description files.
.P
When the -O option is used,
the description file will be created in the current directory
with one exception.  If the alternate table name
is the same as the output data table name,
then the description file will be created in the data directory.
If no table is given with the -O option then no output description
file is created.
.P
The -n[newline] option will cause the character "newline"
to be placed in the attribute value if the value for the attribute in packet
format extends more than a line (i.e., newline-tab sequence).
The default newline character is tilde (~).
If -n is specified with no argument,
then the newline character is assumed to be null and
no newline character is inserted (i.e., the newline option is effectively
turned off).
.P
If the "-q" option is specified, the program does not print out
the number of records that are converted to tuples.
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
.ne 12
.SH EXAMPLE
.nf
$ cat packets

#####   1
snumber s1
pnumber p1
qty     300

#####   2
snumber s1
pnumber p2
qty     200

.ne 11
$ display packets description
The table description is:
snumber	t|	3l
pnumber	t|	3l
qty	t\\n	4r

$ tuple packets
tuple: 2 records converted
s1|p1|300
s1|p2|200
.fi
.SH FILES
.nf
unity/tuple.c, ztuple.c - the C source program
bin/tuple - the executable program
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIretrieve\fP(NewUNITY),
\fIuedit\fP(UNITY), \fIuenter\fP(UNITY), \fIpacket\fP(UNITY)
