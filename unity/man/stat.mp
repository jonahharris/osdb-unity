.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH stat "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
stat \- do elementary statistical analysis on numeric attributes
.SH SYNOPSIS
.nf
\fIstat [-c] [-Itable] [-Otable] [aname ... \fIin\fP ] table1 \\
 [\fIinto\fP table2] [where clause]
.fi
.SH DESCRIPTION
This command does an elementary statistical analysis of the specified numeric
attributes in the table.
If no attributes are specified, then an analysis is done on all attributes
in the table.
The \fIstat\fP command provides the user with the sum, count, average,
minimum, maximum, and standard deviation for each attribute specified
on the command line (the attribute values must be numeric).
Records in the table that contain invalid (non-numeric) attribute
values for any
of these attributes are omitted from the calculation.
However, all invalid entries are listed in the output as warning
messages to the user.
.P
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
If the \fIwhere\fP clause is specified, then the statistical analysis
is done only on the selected records (see the Intro manual page
for details on use of the \fIwhere\fP clause).
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
Attributes in the output table are \fItotal\fP, \fIcount\fP, \fIaverage\fP,
\fImin\fP, \fImax\fP, and \fIstd_dev\fP and this table can then be used by any
of the other UNITY commands.
The associated user\-friendly names are ATTRIBUTE NAME,
TOTAL OF VALUES, COUNT OF VALUES, AVERAGE OF VALUES, MINIMUM VALUE,
MAXIMUM VALUE, and STANDARD DEVIATION (used by several of the UNITY
commands on output).
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

.ne 6
$ stat qty price total in INV where pnumber leq p1

                    total  count     average       min       max   std dev
         qty       700.00      3      233.33    100.00    300.00     94.28
       price         0.72      3        0.24      0.23      0.25      0.01
       total       170.00      3       56.67     23.00     75.00     23.84

.ne 6
$ stat -c qty in INV into - | uselect -q total from -

.fi
This last command sequence would print the total value from the \fIstat\fP command.
There is no way to get the total value alone with just \fIstat\fP.
.SH FILES
.nf
unity/stat.c, zstat.c - the C source program
bin/stat - the executable program
/tmp/DnoI<PID> - temporary description file when no -Itable option
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIretrieve\fP(NewUNITY), \fIdist\fP(UNITY), \fIddist\fP(UNITY)
