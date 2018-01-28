.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH udelete "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
udelete, delete \- delete one or more records from a UNITY table
.SH SYNOPSIS
.nf
\fIudelete\fP [-c] [-q] [-Itable] [-Otable] \fIfrom\fP table1 [\fIinto\fP table2] \\
 where clause
.fi
.SH DESCRIPTION
The \fIudelete\fP command allows the user to delete a set of records from an
existing table according to some criteria on the values of
certain attributes (i.e., records satisfying the \fIwhere\fP condition).
The user is not allowed to explicitly delete all records; a where
clause must be specified.
However, a where clause may be specified which will implicitly delete
all records.
.P
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
This command is considered an update command if a table name is given
(not "-") and the "into clause" is not specified.
In this case,
a temporary file is updated (i.e., the records satisfying the \fIwhere\fP
clause are not written out to the temporary file),
the original file is removed, and the temporary
file is linked to the original table, thus updating the table.
Only one user may update the table at a time but any number of
users may read the table.
The user must have write permission in the directory containing
the table, but needs only read permission on the table itself.
The group and user identification, and mode of the table are preserved.
.P
If an "into clause" is specified, the output is written into
the named table and a corresponding descrition file is created in the
current directory.
An error is printed if the output table already exists.
The input table is not locked during the update.
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
with a \fI%enddescription\fP.  This allows \fIudelete\fP to interface directly
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
This allows \fIudelete\fP to be used as an intermediate filter between
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
the number of records that are deleted.
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
.br
.ne 18
.SH EXAMPLE
.nf
$ udelete from SP where snumber leq s3 and pnumber leq p1
udelete: 1 records deleted in SP.

BEFORE:            AFTER:

|------------|     |------------|
|snu|pnu| qty|     |snu|pnu| qty|
|mbe|mbe|    |     |mbe|mbe|    |
|r  |r  |    |     |r  |r  |    |
|------------|     |------------|
|s1 |p1 | 300|     |s1 |p1 | 300|
|s1 |p2 | 200|     |s1 |p2 | 200|
|s1 |p3 | 400|     |s1 |p3 | 400|
|s2 |p1 | 100|     |s2 |p1 | 100|
|s2 |p2 | 200|     |s2 |p2 | 200|
|s3 |p1 | 400|     |------------|
|------------|     
.fi
.SH FILES
.nf
unity/delete.c, zdelete.c - the C source program
bin/delete, bin/udelete - the executable programs, linked together
/tmp/DnoI<PID> - temporary description file when no -Itable option
<directory of table>/L<table> - the lockfile
<directory of table>/dtmp?????? - the temporary update file
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIalter\fP(UNITY), \fIinsert\fP(UNITY),
\fIuedit\fP(UNITY), \fIuenter\fP(UNITY), \fIretrieve\fP(NewUNITY)
