.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH outerjoin "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
outerjoin \- outerjoin two existing UNITY tables
.SH SYNOPSIS
.nf
\fIouterjoin\fP [-c] [-q] [-Itable1] [-Itable2] [-Otable] a1a[,a1b,...] \\
 a2a[,a2b,...] \fIfrom\fP table1 table2 [\fIinto\fP table3]
.fi
.SH DESCRIPTION
The \fIouterjoin\fP command allows the user to build a new table by doing
an outerjoin on two existing tables (table1 and table2).
The first two arguments specify the attributes on which tables table1 and
table2 are to be joined; the arguments are composed of one or more
attributes in the respective tables, separated by commas
(no white space is allowed in the attribute lists).
The outerjoin of tables table1 and table2 is composed of those records
whose attribute values from table1 equal the attribute values from table2
for those fields on which the outerjoin is being done PLUS all records
from table1 which do not have a matching record (i.e., a record with
equal attribute values) on table2.
The records are concatenated together with the attributes from
table1 appearing first.
The attributes in table2 on which the outerjoin is done do
not appear in the output since they are a duplicate of the
attribute values in table1.
If the last attribute of table1 is a terminator field (the terminator must
be newline), then the terminator is changed to tab so that each
joined record is a single line on output.
For table1 records for which there is no matching table2 record, the table2
attributes on output are null values;
terminator fields consist only of the terminator
and fixed with fields are padded with spaces.
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
If an "into clause" is specified, the output is written into
the named table and a corresponding descrition file is created in the
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
Note that the second input table is never allowed to be "-" since
it has to be read multiple times (once for each record in table1).
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
.ne 2
To perform the outerjoin operation efficiently,
table2 must have an index on the first join attribute specified
for table2.
.P
If an attribute in table1 other than an attribute on which the outerjoin
is being done has the same name as an attribute in table2,
then the resulting description file will have two attributes with the
same name; this causes strange results when used by several of the UNITY
programs.
The program attempts to solve the problem
by renaming the duplicate attribute name in
the second description file to <attribute>1
(i.e., a "1" is appended to the attribute name for all duplicate
pairs other than the joined attributes)
and warning the user about the name change.
.P
If the "-q" option is specified, the program does not print out
the number of records that are outerjoined.
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
.ne 16
$ outerjoin snumber snumber from S SP into tmp1
outerjoin: 6 records joined from S and SP.
$ uprint tmp1
|----------------------------------|
|snu|sname     |city      |pnu| qty|
|mbe|          |          |mbe|    |
|r  |          |          |r  |    |
|----------------------------------|
|s1 |smith     |london    |p1 | 300|
|s1 |smith     |london    |p2 | 200|
|s1 |smith     |london    |p3 | 400|
|s2 |jones     |paris     |p1 | 100|
|s2 |jones     |paris     |p2 | 200|
|s3 |blake     |paris     |   |    |
|----------------------------------|
.fi
.SH "CAVEAT"
The \fIouterjoin\fP command is implemented using the software
\fIujoin\fP and some conditional compilation based on
whether OUTERJOIN is defined or not.
.SH FILES
.nf
unity/ujoin.c, zujoin.c - the C source program
bin/outerjoin - the executable programs
A<table>.<attribute>, B<table>.<attribute> - the index files
/tmp/DnoI<PID> - temporary description file when no -Itable option
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIujoin\fP, \fIregjoin\fP, \fIsetdiff\fP, \fIindex\fP(UNITY),
\fIretrieve\fP(NewUNITY)
