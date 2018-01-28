.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH setdiff "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
setdiff \- find set difference of attributes on two UNITY tables
.SH SYNOPSIS
.nf
\fIsetdiff\fP [-c] [-q] [-Itable1] [-Itable2] a1a[,a1b,...] a2a[,a2b,...] \\
 \fIfrom\fP table1 table2 [\fIinto\fP table3]
.fi
.SH DESCRIPTION
The \fIsetdiff\fP command allows the user to build a new table by
finding the difference of two sets of attribute values, one from each of two
existing tables (table1 and table2).
The set difference of tables table1 and table2 is made up of those records
from table1 that will not appear in the output of a \fIujoin\fP(UNITY) command
with the same arguments.
The arguments may be reversed to get those records in table2 that will
not appear in the output of a \fIujoin\fP(UNITY) command with
the same arguments.
.P
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the first input table.
If the -I option is used twice, the second occurrance table name
specified in the second occurrance is used to determine
an alternate description file to be used in reading the second input table.
Note that this makes the -I option positional.
Even if an alternate
description file is not desired for the first table, it must be entered
if an alternate description file is desired for the second table.
In this case, the table name entered in the first occurrance of the -I
option should be the same as table1.
.P
The following should be kept in mind when using the -I option.
If the alternate table name is the same as the name of the corresponding
data file or if it contains a "/./" (full) or "././" (relative) path prefix,
the given data (description) directory will be searched first
for the description file.
If the -I option has to be used because of the positional
parameter requirement noted above, it may be necessary to
add an extra "/" somewhere in the table name for either the
-I option or the data file so that the table names are unique
which will then avoid making the data directory the first
place to be checked for the needed description file(s).
.P
The output is written to the standard output by default.
If an "into clause" is given, the output is written into
the named table and a corresponding description file is created in the
current directory.
An error is printed if the output table already exists.
If \fIonto\fP is specified instead of \fIinto\fP, the output is
appended to the table if it already exists or the table created
and written if it does not exist.
.P
If an input table specified is "-", then input is taken from
the standard input instead of a table.
Normally, the -I option must be used to specify the name of a table from
which the description file name can be determined and used.
If the -I option is not given, the standard input stream is checked
for the description beginning with a \fI%description\fP and ending
with a \fI%enddescription\fP.  This allows the program to interface directly
with the output of New Unity commands, such as \fIretrieve(1)\fP,
that do not create intermediate description files.
Note that both table names can not be "-";
one of them must be specified.
The indexes, as described below, cannot be used on a table
that is read from the standard input.
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
The -O option can also be used by itself without a table name
in conjunction with the "\fIinto\fP table" clause to indicate
that no output description file is to be created.
However, the use of the -O option in this way only applies
when the output table is not going to standard output.
.P
To perform the setdiff operation \fIefficiently\fP, table2 must
have an index on the first attribute on which the join is done (a2a).
.P
The \fIsetdiff\fP command is used when it is important to find out
which records from a table will be lost when a \fIujoin\fP(UNITY) is done with
another table.
.P
If the "-q" option is specified, the program does not print out
the number of records that are in the set difference
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
.ne 16
.SH EXAMPLE
.nf

Table S:                        Table SP:

|-------------------------|     |------------|
|snu|sname     |city      |     |snu|pnu| qty|
|mbe|          |          |     |mbe|mbe|    |
|r  |          |          |     |r  |r  |    |
|-------------------------|     |------------|
|s1 |smith     |london    |     |s1 |p1 | 300|
|s2 |jones     |paris     |     |s1 |p2 | 200|
|s3 |blake     |paris     |     |s1 |p3 | 400|
|-------------------------|     |s2 |p1 | 100|
                                |s2 |p2 | 200|
                                |------------|

.ne 13
$ setdiff snumber snumber from S SP into tmp2
setdiff: 1 records difference from S and SP.

Table tmp2:

|-------------------------|
|snu|sname     |city      |
|mbe|          |          |
|r  |          |          |
|-------------------------|
|s3 |blake     |paris     |
|-------------------------|
.sp
.fi
As another example, suppose I want to alter a field, say field1,
of a table, say table1, for a selected portion of the table that does not
fit into any groupings using the \fIwhere\fP clause.
It can be accomplished by putting the keys of those records to be altered
into a file, say table2 (and creating the appropriate
description file with a single field, say key),
and executing the following commands:
.sp
.nf
$ ujoin key key from table1 table2 into tmp
$ alter field1 to "newvalue" in tmp
$ setdiff key key from table1 table2 >> tmp
$ mv tmp table1
.fi
.sp
The table table1 has been modified so that all records whose key appears
in table2 have the field1 attribute set to "newvalue"; all other
records appear the same as before.
.sp
Another use for this command is to compare \fIall\fP attributes of two different
tables with the same format (e.g., an old and new version of the same table).
The output will be those lines that are different between the tables.
.SH FILES
.nf
unity/setdiff.c, zsetdiff.c - the C source program
bin/setdiff - the executable program
A<table>.<attribute>, B<table>.<attribute> - the index files
/tmp/DnoI<PID> - temporary description file when no -Itable option
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIujoin\fP, \fIregjoin\fP,
\fIouterjoin\fP, \fIretrieve\fP(NewUNITY), \fIindex\fP(UNITY)
