.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH asort "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
asort \- sort a table on a specified attribute
.SH SYNOPSIS
.nf
\fIasort\fP [-c] [-q] [sort flags] [-Itable] [-Otable] aname \fIin\fP table1 \\
 [\fIinto\fP table2]
.fi
.SH DESCRIPTION
The \fIasort\fP command allows the user to sort a table
on a given attribute easily.
Note that this is \fInot\fP an update command and the original table is
left untouched.
Any attribute named in the description for the table may be given
as the attribute on which the table is to be sorted;
the only restriction is that the attribute values must be less
than 500 characters in length.
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
An error is printed if the table already exists.
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
with a \fI%enddescription\fP.  This allows \fIasort\fP to interface directly
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
This allows \fIasort\fP to be used as an intermediate filter between
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
the number of records that are sorted.
.P
Sort options may be given as the initial argument(s) on the command line.
Valid flags for the \fIsort\fP program are as follows:
.IP "\fB-b\fP"
Ignore leading blanks.
.IP "\fB-d\fP"
Dictionary sort (only letters, digits, and blanks are considered).
.IP "\fB-f\fP"
Fold upper to lower case.
.IP "\fB-i\fP"
Ignore characters outside Ascii range 040-0176.
.IP "\fB-n\fP"
Numeric sort.
.IP "\fB-r\fP"
Reverse sort order.
.IP "\fB-t\fP\fIx\fP"
Specify "tab character" separating sub-fields.
.IP "+\fIpos1\fP -\fIpos2\fP"
This notation restricts a sort key to a sub-field of the specified
UNITY field beginning at \fIpos1\fP and ending just before \fIpos2\fP.
\fIPos1\fP and \fIpos2\fP each have the form \fIm.n\fP
where \fIm\fP tells a number of fields to skip from the beginning
of the UNITY field and \fIn\fP tells a number of characters to skip further.
If any flags are present they override all the global ordering options for
the field.
A missing .\fIn\fP means .0; a missing \fI-pos\fP
means the end of the UNITY field.
Under the \fB-t\fP\fIx\fP option,
sub-fields are strings separated by \fIx\fP; otherwise sub-fields
are non-empty non-blank strings separated by blanks.
.IP "\fB-u\fP"
This option will suppress all records but one in each set of
records that has a given value for the specified attribute.
The record selected from the set is unpredictable
(i.e., the first one is not always chosen).
The user can presort the file in the desired sort order and
then run \fIasort\fP with the -u \fIand\fP -m flags
which causes the first instance of each unique value to
be selected (thus, the selected lines are predictable).
The -u option \fIcannot\fP be used with the -D, -t, +pos, or -pos
options (these four options are necessary to get the -u option to work).
.IP "\fB-D\fP"
This option causes the attribute specified to be treated as a
date is mm/dd/yy format so that the values are sorted by yy, then mm,
then dd.
This option \fIcannot\fP be used with the -u, -t, +pos, or -pos
options (these four options are necessary to get the -u option to work).
.P
The default is an alphanumeric sort on the entire attribute value
in ascending order.
.P
The \fIasort\fP command provides several advantages for the user over
using the system \fIsort\fP(1) command directly on the table.
First, the line length limitation of \fIsort\fP of 512 characters
may cause long records to be silently truncated whereas \fIasort\fP
sorts only on the specified attribute.
Second, the command to sort on a given attribute is independent of
how the attributes are stored since this information is hidden from the user
in the description file.
Thus, a table structure may change (along with the
corresponding description file) without necessitating
a change to the command to sort on an attribute.
A third advantage of this command is the reduced processing time required
for large tables.
When the table to be sorted exceeds 32 kilobytes,
the system \fIsort\fP program creates sorted temporary files and
then merges the temporaries together; this process is slow.
Since \fIasort\fP sorts only the specified attribute and an
associated seek location, there is a much greater chance that the
sort will be completed without the merging process.
.P
The user can specify only a single sort field in the \fIasort\fP command.
However, a table may be sorted on several attributes by sorting
first on the least significant attribute, then on the next least
significant attribute and so on, sorting the most significant
attribute last.
Although the sorting algorithm used by \fIsort\fP(1) is unstable,
the \fIasort\fP command makes each sorting pass stable
(i.e., records with identical attribute values are output in the order
that they were input which is necessary for the multi-key sort
to work properly)
by using the seek location of the record as part of the sorting key.
The seek location is padded with zeros on the left to ensure proper
sorting whether a numeric or alphanumeric sort is done.
A user specifying reverse sort order (the "-r" sort option) should be
aware that this will also reverse the order of the seek values so that
multi-key sorts in reverse order will probably not work as desired.
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
.SH EXAMPLE
.nf
$ asort sname in S into tmp3
asort: 3 records sorted from S.

BEFORE:                         AFTER:

|-------------------------|     |-------------------------|
|snu|sname     |city      |     |snu|sname     |city      |
|mbe|          |          |     |mbe|          |          |
|r  |          |          |     |r  |          |          |
|-------------------------|     |-------------------------|
|s1 |smith     |london    |     |s3 |blake     |paris     |
|s2 |jones     |nice      |     |s2 |jones     |nice      |
|s3 |blake     |paris     |     |s1 |smith     |london    |
|-------------------------|     |-------------------------|
.SH CAVEAT
.P
The actual value that is sorted consists of
.sp
<UNITY field value>\\01<8-digit seek value>
.sp
This allows the sort to be stable since earlier seek values
will appear first for lines with duplicate attribute values.
The attribute value is terminated with a '\\01' (control-A)
instead of a '\\0'
because if an attribute value is null, the output record
would begin with '\\0' and the sort(1) program silently throws these
records away.
\fIindex2\fP is used to generate the records on which to sort.
.P
In order to do a unique sort (option -u), the program specifies
that the tab character is control-A which is the character
that separates the attribute value from the seek location.
A unique sort is done on each line for the first field (i.e., the
attribute) only so that the resulting list will be unique on
attribute value.
This means that the -t, -pos, and +pos options cannot be
used with the -u option.
.P
In order to sort a table on a date field stored in mm/dd/yy format,
the user would execute
.sp
asort -t/ -n +2.0 -2.3 +0.0 -1.0 +1.0 -2.0 +2.3 -3.0 date in table 
.sp
Note that the last sub-field in the date field is split up into two parts
(i.e., +2.0 -2.3 and +2.4 -3.0).
The +2.0 -2.3 positions specify to sort on the year part of the date
only (excluding the seek value) and the +2.4 -3.0 positions as the
least significant sort sub-field ensure that the sort is stable.
These values are added to the sort options when the user specifies
the -I option.
It was necessary to pick a separator character for the values and virgule (/)
was chosen.
.SH FILES
.nf
unity/asort.c, zasort.c - the C source program
bin/asort, index2 - the executable programs
/tmp/DnoI<PID> - temporary description file when no -Itable option
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIsort\fP(1), \fIindex\fP(UNITY)
