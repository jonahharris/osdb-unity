.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH merge UNITY "" "UNITY User's Manual"
.SA 1
.SH NAME
merge, unmerge \- join/split attribute values for given attribute
.SH SYNOPSIS
.nf
\fImerge\fP [-c] [-q] [-s<separator>] [-Itable] [-Otable] aname1 \fIin\fP table1 \\
 [\fIinto\fP table2]
\fIunmerge\fP [-c] [-q] [-Itable] [-Otable] [-s<separator>] aname1 \fIin\fP table1 \\
 [\fIinto\fP table2]
.fi
.SH DESCRIPTION
\fImerge\fP will merge several records together to form one record
where all attribute values of each record merged are the same
except for the attribute on which the merge is being done.
The resulting merged attribute
is composed of the values of the merged attribute for each of the
records merged separated by a separator character.
The default separator character is comma(,) and may be changed
using the \fI-s\fPc option.
The merged attribute must be a terminator field.
\fImerge\fP assumes that the records are in sorted order by all attributes
other than the attribute on which the merge is being done
(with the first attribute not being merged being the most significant
sort key, the second attribute not being merged being the second
most significant sort key, etc.).
\fImerge\fP prints an error message
and the record that is out of order and exits if the records
are not in sorted order.
.P
\fIunmerge\fP is the reciprocal function of \fImerge\fP and breaks
a single record into multiple records where each of the new records
has the same attribute values for all attributes except
the attribute on which the unmerge is being done.
That attribute will have the first part of the original value
(all characters up to the separator character) on the first output record, etc.
.P
For either command,
if the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
For both commands,
the output is written to the standard output by default.
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
with a \fI%enddescription\fP.  This allows either command to interface directly
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
This allows either command to be used as an intermediate filter between
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
the number of records that are merged (or unmerged).
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
.ne 13
.SH EXAMPLE
.nf
$ display children
The table description is:
parent	t\\t
child	t\\n
The data for the table is:
A. Parent        Child 1
A. Parent        Child 2
A. Parent        Child 3
B. Parent        Child 1
B. Parent        Child 2
.ne 6
$ merge child in children into out
merge: 2 records merged from children.
$ dis out data
The data for the table is:
A. Parent        Child 1,Child 2,Child 3
B. Parent        Child 1,Child 2
.ne 9
.sp
$ unmerge child in out
unmerge: 5 records unmerged.
A. Parent        Child 1
A. Parent        Child 2
A. Parent        Child 3
B. Parent        Child 1
B. Parent        Child 2
.fi
.SH FILES
.nf
unity/merge.c, unmerge.c, zmerge.c, zunmerge.c - the C source programs
bin/merge, unmerge - the executable programs
.fi
.SH SEE ALSO
\fIIntro\fP(UNITY), \fIretrieve\fP(NewUNITY)
