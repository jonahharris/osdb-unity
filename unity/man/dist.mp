.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH dist "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
dist, ddist \- single and double distributions on a UNITY table
.SH SYNOPSIS
.nf
\fIdist\fP [-c] [-n] [-Itable] [-Otable] {aname1|count} \fIby\fP aname2 \fIin\fP table1 \\
 [\fIinto\fP table2] [where clause]
\fIddist\fP [-c] [-n] [-Itable] [-Otable] {aname1|count} \fIby\fP aname2 aname3 \\
 \fIin\fP table1 [\fIinto\fP table2] [where clause]
\fItally\fP [-c] [-n] [-Itable] [-Otable] aname \fIin\fP table1 \\
 [\fIinto\fP table2] [where clause]
\fIdtally\fP [-c] [-n] [-Itable] [-Otable] aname1 aname2 \fIin\fP table1 \\
 [\fIinto\fP table2] [where clause]
.fi
.SH DESCRIPTION
The \fIdist\fP command does a "single distribution," tabulating
the sum of the values of a numeric attribute (aname1 in the SYNOPSIS)
corresponding to an alphanumeric attribute (aname2) for the specified table.
The user is given the option of producing sorted output (the default case)
or unsorted output (i.e., in retrieval order) by specifying
the \fI-n\fP option.
.P
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
The output is a three-column table:
column one contains distinct values for aname2, column two contains the summed
numeric values for aname1 corresponding to each distinct value of aname2, and
column three contains counts indicating the number of entries summed for
each distinct aname2 value.  Records in the table that contain invalid
(non-numeric) attribute values for aname1 are omitted.
However, all invalid entries
are listed in the output as warning messages.
If the keyword \fIcount\fP
is used instead of a legal attribute name from table, the output will
contain only two columns: the distinct values for aname2 and the count.
The default command allows up to 1000 rows of output.
.P
The \fIwhere\fP clause can be used to limit the distribution to
only the selected records (see the Intro manual page for a detailed
description on use of the where clause).
.P
The output is written to the standard output by default and is NOT
in a valid format for a UNITY table.
If an "into clause" is specified, the output is written into
the named table (in table format)
and a corresponding descrition file is created in the current directory.
An error is printed if the output table already exists.
If \fIonto\fP is specified instead of \fIinto\fP, the output is
appended to the file if it already exists or the file is created
and written if it does not exist; the description file is
created if it does not exist.
The output description file attributes are \fIname\fP, \fItotal\fP and
\fIcount\fP or just \fIname\fP and \fItotal\fP depending on whether the
\fIcount\fP keyword is used.
The associated user\-friendly names are
FIELD VALUE, TOTAL OF VALUE, and COUNT OF VALUES.
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
The \fIddist\fP command is similar to \fIdist\fP, except that it
does a "double distribution" of a numeric
attribute (aname1) over \fItwo\fP alphanumeric attributes (aname2 and
aname3).  Hence the summed values for aname1 and count are based on unique
pairs of aname2 and aname3 values.
The number of rows initially allocated is 20 and this number grows
dynamically as more rows are needed
(limited by the amount of space that can be allocated dynamically).
The maximum number of columns is fixed (default 100 columns).
The \fIwhere\fP clause can be used to limit the distribution to
only the selected records.
.P
Input and output are similiar to that of \fIdist\fP.
The number of columns in the resulting table (attributes) must
not exceed the maximum number of attributes allowed in a table
by UNITY (currently 300).  Since the attribute names of this table
are actual values, they may not follow the restrictions placed
on attribute names in UNITY.  Attribute names beginning with a
non-alphabetic character will have a 'v' prefix added and
non-alphanumeric characters within the value will be converted to
an underscore.
Use of the \fIonto\fP clause is strongly discouraged unless
the user is certain that the number of columns output will
be exactly the same.
.P
\fItally\fP and \fIdtally\fP are provided as a short hand notation
for "dist count by" and "ddist count by," respectively (see synopsis for
\fIdist\fP and \fIddist\fP).
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
.ne 15
.SH EXAMPLES
.nf
$ uprint INV
|-------------------------|
|snu|pnu| qty|price| total|
|mbe|mbe|    |     |      |
|r  |r  |    |     |      |
|-------------------------|
|s1 |p1 | 300|  .25| 75.00|
|s1 |p2 | 200|  .30| 60.00|
|s1 |p3 | 400|  .35|140.00|
|s2 |p1 | 100|  .23| 23.00|
|s2 |p2 | 200|  .32| 64.00|
|s3 |p1 | 300|  .24| 72.00|
|-------------------------|
.ne 9
$ tally pnumber in INV

NAME           COUNT

p1                 3
p2                 2
p3                 1
              ------
TOTAL              6
.ne 8
$ dist qty by pnumber in INV

NAME                TOTAL  COUNT

p1                    700      3
p2                    400      2
p3                    400      1
.sp
.ne 9
$ dtally pnumber snumber in INV
                       s1           s2           s3

p1                      1            1            1
p2                      1            1            0
p3                      1            0            0
                    -----        -----        -----
TOTAL                   3            2            1       

.ne 7
$ ddist qty by pnumber snumber in INV
                       s1           s2           s3

p1                    300          100          300
p2                    200          200            -
p3                    400            -            -

.SH FILES
.nf
unity/dist.c, ddist.c, zdist.c, zddist.c - the C source programs
bin/dist, tally - the executable programs (linked)
bin/ddist, dtally - the executable programs (linked)
/tmp/DnoI<PID> - temporary description file when no -Itable option
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIretrieve\fP(NewUNITY), \fIstat\fP(UNITY)
