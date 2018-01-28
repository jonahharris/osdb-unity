.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH uprintf "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
.nf
uprintf \- user-formatted print on UNITY tables
.fi
.SH SYNOPSIS
.nf
\fIuprintf\fP [-c] [-q] [-Itable] [-Otable] [-fformat] [-nc] aname \\
 [aname ...] [ { \fIresult\fP | \fIresult\fP:format } aname1] \\
 [aname ... { \fIresult\fP | \fIresult\fP:format } anameN] \\
 \fIin\fP table1 [\fIinto\fP table2] [where clause]
.fi
.SH DESCRIPTION
The \fIuprintf\fP command generates a string value based
on the combination of one or more specified attributes for each record.
.P
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
The "-f" option may be used to specify the output format.
The output format must immediately
follow the -f option on the command line and must be surrounded by
quotes if it contains white space or other characters recognized by the shell.
The output format may be anything consisting
of string output values (i.e., any combination of characters and %s formats).
For example, -f%-.20s would print results
as twenty-character left-justified string values and "-f%s %s" would print
the concatenation of two attribute values separated by a space.
The valid formats are discussed in detail on the \fIprintf\fP(3S)
manual page but only the "s" conversion character is allowed.
All character constant escape sequences recognized in the C language
(newline, horizontal tab, backspace, carriage return, form feed, backslash,
single quote, and bit pattern) are recognize as part of the output format
(e.g., the strings "\\t" and "\\n" are recognized as tab and
newline characters, respectively).
The number of variables to be printed (%s occurrences in the output format)
must be less than or equal to the number of attributes specified to be
printed; otherwise, the program returns an error.
.P
If a resultant field (\fIresult\fP clause) is not specified,
only the fields specified as part of the output format are printed
(projection is done on the specified fields)
and only selected records (based on the \fIwhere\fP clause) are printed.
The default format is "%s\\n"
which means that the values of first attribute specified
for the command will be printed, one per line.
In this way, the \fIuprintf\fP command can be used
as a simple report generator.
The input table is not updated.
The output is written to the standard output by default.
If an "\fIinto\fP clause" is specified, the output is written into
the named table (no description file is created);
an error is printed if the table already exists.
If \fIonto\fP is specified instead of \fIinto\fP, the output is
appended to the file if it already exists or the file is created
and written if it does not exist.
The -n\fIc\fP option causes all occurrences of the specified character \fIc\fP
to be translated into a a newline during printing.
Thus, the user can explicitly control where breaks occur in
attribute values.
The newline character is not printed.
The default newline indicator character is tilde (\~).
.P
If one or more result fields are specified, then the output is a UNITY table
with the same fields as the input table.
Each result field must be an existing terminator field.
The default format is "%s" which means
that the result will be the string value of the first attribute
specified on the command line.
The -f option may be used to change the default output format
that is used when an output format has been been specified
for a given result by appending the format to the \fIresult\fP
keyword with a leading colon as shown in the synopsis.
Note that "\fIresult\fP:format" is (must be) one command line
parameter \(em not two.
Each result attribute must be unique and all results are based
on the original attribute values.
The number of results is limited by the maximum number of attributes
that can be defined in a UNITY table.
In addition, the total number of attributes used as input to all
result format strings which get passed to sprintf(3C),
is limited to the maximum number of attributes
that can be defined in a UNITY table.
In other words, the limit is reached by,
1) printing all attributes into a single result,
2) generating a result for every attribute and
using only one input attribute for each result, or
3) a combination of printing multiple results with some or all
results being generated from more than one input attribute.
.P
When one or more results are given without an \fIinto\fP clause,
the input table is updated.
A temporary file is updated (i.e., the records satisfying the \fIwhere\fP
clause are altered), the original file is removed, and the temporary
file is linked to the original table, thus updating the table.
Only one user may update the table at a time but any number of
users may read the table.
The user must have write permission in the directory containing
the table, but needs only read permission on the table itself.
The group and user identification, and mode of the table are preserved.
If an "into clause" is specified, the output is written into
the named table and a corresponding description file is created;
an error is printed if the table or corresponding description file already
exists.
If \fIonto\fP is specified instead of \fIinto\fP, the output is
appended to the file if it already exists or the file is created
and written if it does not exist; the description file is
created if it does not exist.
The -n option is ignored if a resultant field is specified.
.P
If the input table specified is "-", then input is taken from
the standard input instead of a table.
Normally, the -I option must be used to specify the name of a table from
which the description file name can be determined and used.
If the -I option is not given, the standard input stream is checked
for the description beginning with a \fI%description\fP and ending
with a \fI%enddescription\fP.  This allows \fIuprintf\fP to interface directly
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
This allows \fIuprintf\fP to be used as an intermediate filter between
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
the number of output records.
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
.ne 14
.SH EXAMPLE
.nf
$ uprint S

|-------------------------|
|snu|sname     |city      |
|mbe|          |          |
|r  |          |          |
|-------------------------|
|s1 |Smith     |London    |
|s2 |Jones     |Nice      |
|s3 |Blake     |Paris     |
|-------------------------|

.ne 6
$ uprintf -q -f"Supplier %s, supplier number %s, is located in %s.\\n" \\
  sname snumber city in S

Supplier Smith, supplier number s1, is located in London.
Supplier Jones, supplier number s2, is located in Nice.
Supplier Blake, supplier number s3, is located in Paris.

.ne 5
$ uprintf -q rec# result snumber sname "result:Mr. %s" sname in S into -
1       Mr. smith       London
2       Mr. jones       Nice
3       Mr. blake       Paris

.fi
.SH FILES
.nf
unity/uprintf.c, zuprintf.c - the C source program
bin/uprintf - the executable program
/tmp/DnoI<PID> - temporary description file when no -Itable option
<directory of table>/L<table> - the lockfile
<directory of table>/ctmp?????? - the temporary update file
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIcompute\fP(UNITY),
\fInprintf\fP(UNITY), \fIretrieve\fP(NewUNITY)
