.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH format "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
format, uformat \- change format of a UNITY table
.SH SYNOPSIS
\fIformat\fP [-c] [-q] [-Itable] [-Otable] table1 \fIinto\fP table2
.SH DESCRIPTION
The \fIformat\fP program aids the user in changing the format
of a table from the description given in one description file
to that given in another.
The user must have previously created one description file
for the input table (table1 in the SYNOPSIS) and another description file
for the output table (table2 in the SYNOPSIS).
The description files are named Dtable1 and Dtable2 (the
user need not worry about the naming conventions if
description files are created using the \fItable\fP command).
The \fIformat\fP command outputs a new table by projection of those
fields in table1 that are specified in the description for table2 (in the
order specified) or creating new attributes for fields specified in
the description for table2 that do not appear in the description for table1.
.P
\fIuformat\fR is just another name for \fIformat\fR, to avoid name conflicts
on some systems.
.P
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input table.
If the -O option is used, the table name specified is used to determine
an alternate description file to be used in formating the output table.
When either the -I or -O option (or both) are used,
if the alternate table name is the same as the name of the data file or
if it contains a "/./" (full) or "././" (relative) path prefix,
the given data (description) directory will be searched first
for the description file.
.P
The \fIformat\fP command takes care of conversion between field
types (e.g., fixed width fields and terminator fields).
When converting between terminator fields (the simplest case), only the
terminator for the field is changed.
When converting from a fixed width field to a terminator field,
the new terminator is appended after the field value.
Conversions to a fixed width field are slightly more
complicated.
If the width of an output fixed width field is less than the width
of the input field (either fixed width or terminator),
the input attribute value is truncated on the right (without warning)
to produce the output value.
If the widths are equal then no conversion takes place (other than to
remove the terminator if the input field is a terminator field).
If the output field width is greater than the length of the input field,
the input value is padded with blanks to fill the entire
width of the output field.
If the print justification of the output field is 'l' or no
justification is given, the output value is
left justified and the value is padded with blanks on the right.
Otherwise, the attribute value is padded on the left with blanks and
the value is right justified.
There is no centering of attribute values.
.P
If a field is named in the output file description that does not appear
in the input file description, the user is warned that a new attribute
is being created in the output file.
The warning message(s) can be supressed via the -q (quiet) option
(see description of -q option below).
If the new attribute is a fixed width field, then the new attribute
values are spaces.
If it is a terminator field, then the new attribute values are
null (i.e., only the terminator is output for the field).
.P
The output is written into the table named in the "into clause";
an error is printed if the table already exists.
If \fIonto\fP is specified instead of \fIinto\fP, the output is
appended to the file if it already exists or the file is created
and written if it does not exist.
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
The -O option must be used to specify the name of a table from
which the output description file name can be determined
so that the output format may be determined.
When the output is to be written to standard output ("-"),
the -c option can be used to copy the description
to standard output starting with \fI%description\fP and ending with
\fI%enddescription\fP at the beginning of the output data stream.
This allows the program to be used as an intermediate filter between
various UNITY or New Unity commands.
.P
If the "-q" option is specified, the program does not print out
the number of records that are formatted.
If the "-q" option is specified more than once,
the program will not print any warning message(s)
when new attributes are created in the output table.
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
.sp
Desc file DSP:	        Desc file DINV:

snumber t|      3l      snumber t|      3l
pnumber t|      3l      pnumber t|      3l
qty     t\\n     4r      qty     t|      4r
                        price   t|      5r
                        total   t\\n     6r

.ne 20
$ format SP into INV
format: Warning - attribute price not in SP and is being created in INV.
format: Warning - attribute total not in SP and is being created in INV.
format: 6 records formatted from SP.

SP:                INV:

|------------|     |-------------------------|
|snu|pnu| qty|     |snu|pnu| qty|price| total|
|mbe|mbe|    |     |mbe|mbe|    |     |      |
|r  |r  |    |     |r  |r  |    |     |      |
|------------|     |-------------------------|
|s1 |p1 | 300|     |s1 |p1 | 300|     |      |
|s1 |p2 | 200|     |s1 |p2 | 200|     |      |
|s1 |p3 | 400|     |s1 |p3 | 400|     |      |
|s2 |p1 | 100|     |s2 |p1 | 100|     |      |
|s2 |p2 | 200|     |s2 |p2 | 200|     |      |
|s3 |p1 | 300|     |s3 |p1 | 300|     |      |
|------------|     |-------------------------|
.fi
.SH FILES
.nf
unity/format.c, zformat.c - the C source program
bin/format - the executable program
/tmp/DnoI<PID> - temporary description file when no -Itable option
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIretrieve\fP(NewUNITY)
