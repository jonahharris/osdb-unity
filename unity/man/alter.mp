.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH alter "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
alter \- alter the value of an attribute in one or more records
.SH SYNOPSIS
.nf
\fIalter\fP [-c] [-q] [-Itable] [-Otable] aname1 \fI=\fP value [anameN f= aname] \\
 \fIin\fP table1 [\fIinto\fP table2] [where clause]
.fi
.SH DESCRIPTION
The \fIalter\fP command changes the value of one or more attributes
to a particular value (value) using the "=" (or "to") operater or
to the value of another attribute (aname) using the "f=" operator
for those records that satisfy the \fIwhere\fP
condition in the specified table.
.P
When assigning a value ("=" or "to" operator) to a field,
if the field to be altered is a fixed width field and the new value to
be substituted is of shorter length, the value is padded on the right
with blanks.
Attempting to assign a new value of longer length than the field width
is an error and will terminate the program.
.P
When assigning the value of another field ("f=" operator) to a field,
the field to be altered must be a terminator field
or both fields must be fixed width and have the same width.
Otherwise, it is an error and the program exits.
.P
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
This command is considered an update command if a table name is given
and the "into clause" is not specified.
In this case, a temporary file is updated
(i.e., the records satisfying the \fIwhere\fP
clause are altered), the original file is removed, and the temporary
file is linked to the original table, thus updating the table.
Only one user may update the table at a time (a lock file is used
to enforce this) but any number of users may read the table.
The user must have write permission in the directory containing
the table, but needs only read permission on the table itself.
The group and user identification, and mode of the table are preserved.
.P
If an "into clause" is specified, the altered table is written into
the named table and a corresponding descrition file is created in the
current directory.
The input table is not locked during the update.
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
with a \fI%enddescription\fP.  This allows \fIalter\fP to interface directly
with the output of New Unity commands, such as \fIretrieve(1)\fP,
that do not create intermediate description files.
.P
If the output table name specified is "-", then the table output will
be written to the standard output.
Normally, the -O option must be used to specify the name of a table from
which the output description file name can be determined.
However, the -c option can be used instead to copy the description
to standard output starting with \fI%description\fP and ending with
\fI%enddescription\fP.
This allows \fIalter\fP to be used as an intermediate filter between
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
If the "-q" option is specified, the program does not print out
the number of records that are altered.
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
.B UNITYUNPACK
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
.ne 15
$ alter city to nice in S where sname leq jones
alter: 1 records altered in S.

BEFORE:                        AFTER:

|-------------------------|    |-------------------------|
|snu|sname     |city      |    |snu|sname     |city      |
|mbe|          |          |    |mbe|          |          |
|r  |          |          |    |r  |          |          |
|-------------------------|    |-------------------------|
|s1 |smith     |london    |    |s1 |smith     |london    |
|s2 |jones     |paris     |    |s2 |jones     |nice      |
|s3 |blake     |paris     |    |s3 |blake     |paris     |
|-------------------------|    |-------------------------|

.ne 11
$ alter -c -q snumber f= rec# in S into - | uprint -

|-------------------------|
|snu|sname     |city      |
|mbe|          |          |
|r  |          |          |
|-------------------------|
|1  |smith     |london    |
|2  |jones     |nice      |
|3  |blake     |paris     |
|-------------------------|
.fi
.SH FILES
.nf
unity/alter.c, zalter.c - the C source program
bin/alter - the executable program
/tmp/DnoI<PID> - temporary description file when no -Itable option
<directory of table>/L<table> - the lockfile
<directory of table>/atmp?????? - the temporary update file
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIcompute\fP(UNITY), \fIdelete\fP(UNITY),
\fIinsert\fP(UNITY), \fInprintf\fP(UNITY),
\fIuedit\fP(UNITY), \fIuenter\fP(UNITY),
\fIuprintf\fP(UNITY), \fIretrieve\fP(NewUNITY)
