.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH packet "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
packet \- convert records to stream packet format
.SH SYNOPSIS
.nf
packet [-q] [-Itable] [-Otable] [-n[<newline>]] [-u] table1 \\
  [\fIinto\fP table2 ]
.fi
.SH DESCRIPTION
The \fIpacket\fP command transforms records in UNITY tuple format to
stream packet format.
In a packet, the data is structured into field-name/field-value pairs
consisting of the name of the attribute and its value separated
by a tab (a convenient form for editing).
Each packet is preceded by a line of the form
.sp
#####<tab><packet number>
.sp
where <packet number> is the sequence number of the packet; it is provided
for reference purposes by the user.
.P
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
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
Normally, the -I option must be used to specify the name of a table from
which the description file name can be determined and used.
If the -I option is not given, the standard input stream is checked
for the description beginning with a \fI%description\fP and ending
with a \fI%enddescription\fP.  This allows the command to interface directly
with the output of New Unity commands, such as \fIretrieve(1)\fP,
that do not create intermediate description files.
.P
If the output table name specified is "-", then the table output will
be written to the standard output.
The -O option must be used to specify the name of a table from
which the output description file name can be determined.
.P
When the -O option is used,
the description file will be created in the current directory
with one exception.  If the alternate table name
is the same as the output data table name,
then the description file will be created in the data directory.
If no table is given with the -O option then no output description
file is created.
.P
In converting terminator fields to packet format, all occurrences of
the UNITY "newline" character are translated into a newline-tab sequence.
The default "newline" character is tilde (~).
The default may be changed to \fIc\fP by specifying the
-n\fIc\fP option on the command line.
If -n is specified with no newline character, then newline translation
is effectively turned off.
.P
If the "-u" option is specified, then the "user\-friendly" names
are printed instead of the attribute names, if they exist in
the description file (see \fItable\fP(UNITY)).
If the "user\-friendly" names do not exist, then the attribute names
are printed.
.P
If the "-q" option is specified, the program does not print out
the number of records that are packeted.
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
.br
.ne 14
.SH EXAMPLE
.nf
$ uprint -q SP

|------------|
|snu|pnu| qty|
|mbe|mbe|    |
|r  |r  |    |
|------------|
|s1 |p1 | 300|
|s1 |p2 | 200|
|------------|

.ne 13
$ packet -q SP

#####   1
snumber s1
pnumber p1
qty     300

#####   2
snumber s1
pnumber p2
qty     200

.fi
.SH FILES
.nf
unity/packet.c, zpacket.c - the C source program
bin/packet - the executable program
/tmp/DnoI<PID> - temporary description file when no -Itable option
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIretrieve\fP(NewUNITY), \fItuple\fP(UNITY)
