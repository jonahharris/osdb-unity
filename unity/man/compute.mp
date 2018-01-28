.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH compute "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
.nf
compute \- compute arithmetic expressions on UNITY tables
.fi
.SH SYNOPSIS
.nf
\fIcompute\fP [-c] [-q] [-Itable] [-Otable] [-f%format] aname [op aname ...] \\
 \fIresult\fP [%format] aname1 [ aname ... result [%format] anameN] \\
 \fIin\fP table1 [\fIinto\fP table2] [where clause]
.fi
.SH DESCRIPTION
The \fIcompute\fP command changes the value of the resultant
attribute (aname1 in the SYNOPSIS) to the value computed by the expression
specified before \fIresult\fP for those records
that satisfy the \fIwhere\fP condition in the specified table.
More than one result can be computed but a unique attribute name
must be given for each result and all
results are based on the original attribute values.
The expressions accepted may contain attribute names from the
specified table, numeric constants without signs,
unary + and - (these must appear as separate arguments to the command),
+, -, *, /, %, parentheses (to over-ride normal precedence rules),
and the following functions:
.sp
.nf
.ft CW
	abs(attribute) \(em returns absolute value of attribute.
	bin(attribute) \(em returns value of binary numbered attribute.
	hex(attribute) \(em returns value of hexidecimal numbered attribute.
	oct(attribute) \(em returns value of octal numbered attribute.
	len(attribute) \(em returns string length of attribute value.
.ft
.fi
.P
The precedence from highest to lowest is
.IP "1."
abs(attribute), bin(attribute), hex(attribute), oct(attribute), and len(attribute),
.IP "2."
unary + and - with right associativity,
.IP "3."
(),
.IP "4."
*, /, and % with left associativity (evaluated from left to right),
.IP "5."
+ and - with left associativity.
.P
.nf
Thus, with attributes a1, a2, a3, and a4,
.sp
a1 + a2 \\* a3 + - a4   evaluates as ( (a1 + (a2 * a3) ) + (-a4) )
.sp
and
.sp
\\( abs\\(a1\\) + - a2 \\) \\* a3 evaluates as ( ((abs(a1))+(-a2)) * a3)
.fi
.P
The precedence is formally defined as
.sp
.nf
.ft CW
<expression>     ::=  abs(<unity field>) | bin(<unity field>) |
                      hex(<unity field>) | oct(<unity field>) |
                      len(<unity field>) | <expr1>
<expr1>          ::=  <expr2> | - <expr2> | + <expr2>
<expr2>          ::=  <expr3> | ( <expr3> )
<expr3>          ::=  <expr4> | <expr4> [/*%] <expr4>
<expr4>          ::=  <identifier> | <identifier> [+-] <identifier>
<identifier>     ::=  <unity field> | <constant>
<unity field>    ::=  any field in the description file or rec#
<constant>       ::=  numeric floating-point constant (without sign)
.ft
.fi
.P
A value is computed for each record that matches the selection
criteria.
The resultant field must be a terminator field.
.P
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
This command is considered an update command if a table name is
given (not "-") and the "into clause" is not specified.
In this case,
a temporary file is updated (i.e., the records satisfying the \fIwhere\fP
clause are altered), the original file is removed, and the temporary
file is linked to the original table, thus updating the table.
Only one user may update the table at a time but any number of
users may read the table.
The user must have write permission in the directory containing
the table, but needs only read permission on the table itself.
The group and user identification, and mode of the table are preserved.
.P
If an "into clause" is specified, the output is written into
the named table and a corresponding description file is created in the
current directory.
An error is printed if the output table already exists.
If \fIonto\fP is specified instead of \fIinto\fP, the output is
appended to the table if it already exists or the table created
and written if it does not exist.
The input table is not locked during the update.
.P
If the input table specified is "-", then input is taken from
the standard input instead of a table.
Normally, the -I option must be used to specify the name of a table from
which the description file name can be determined and used.
If the -I option is not given, the standard input stream is checked
for the description beginning with a \fI%description\fP and ending
with a \fI%enddescription\fP.  This allows \fIcompute\fP to interface directly
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
This allows \fIcompute\fP to be used as an intermediate filter between
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
The "-f" option may be used to change the default output format of the
resultant field.
The default format is "%.3f" which means
that the result will be printed as a floating point number
with minimally three decimal places.
The user may change the output format to any valid floating
point or integer format by specifying the output format immediately
following the -f option on the command line with the
desired format.
For example, -f%.0f would print results
as integer values (no decimal point) and -f%.3e would
print values in scientific notation.
The valid formats are discussed in detail on the \fIprintf\fP(3S)
manual page.
In addition, "%b" can be used to request unsigned binary number
output format in the same way that "%o" or "%x" can be used for
octal and hexidecimal format, respectively.
The regular expression that shows all acceptable formats is
.nf
.sp
   %[+ -#]*[0-9]*\\.\\{0,1\\}[0-9]*[bdiouxXeEfgG]
.sp
.fi
The user may over-ride the default output format for individual
results by appending a "\fI%format\fP" string directly to the
name of the result attribute (no white space) or
as a separate parameter (without the "-f" prefix) immediately
after the name of the result attribute.
.P
If the "-q" option is specified, the program does not print out
the number of records for which a computation is done.
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
UNITYUNPACK
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
$ compute -f%0.2f qty \\* price result total in INV
compute: 6 records computed in INV.

BEFORE:                         AFTER:

|-------------------------|     |-------------------------|
|snu|pnu| qty|price| total|     |snu|pnu| qty|price| total|
|mbe|mbe|    |     |      |     |mbe|mbe|    |     |      |
|r  |r  |    |     |      |     |r  |r  |    |     |      |
|-------------------------|     |-------------------------|
|s1 |p1 | 300|  .25|      |     |s1 |p1 | 300|  .25| 75.00|
|s1 |p2 | 200|  .30|      |     |s1 |p2 | 200|  .30| 60.00|
|s1 |p3 | 400|  .35|      |     |s1 |p3 | 400|  .35|140.00|
|s2 |p1 | 100|  .23|      |     |s2 |p1 | 100|  .23| 23.00|
|s2 |p2 | 200|  .32|      |     |s2 |p2 | 200|  .32| 64.00|
|s3 |p1 | 300|  .24|      |     |s3 |p1 | 300|  .24| 72.00|
|-------------------------|     |-------------------------|
.SH CAVEAT
The user must remember to escape those characters that are
recognized by the shell, namely (, ), and *.
The unary + and - signs must appear as a separate argument
to the \fIcompute\fP command; otherwise, an error
message is given.
Thus, minus two is entered as - 2 instead of -2.
\fIcompute\fP does all arithmetic in single precision float which
may be slow on machines that do not have floating-point hardware.
If the compute.c and uexpr.c programs are compiled with -DINTCOMP
(i.e., INTCOMP defined to the pre-processor),
then all arithmetic will be done with integers.
The output format must then be %d type or one of the other
(unsigned) integer types instead of any of the floating
output formats (i.e., e, E, f, g, G).
.SH FILES
.nf
unity/compute.c, zcompute.c - the C source program
bin/compute - the executable program
/tmp/DnoI<PID> - temporary description file when no -Itable option
<directory of table>/L<table> - the lockfile
<directory of table>/ctmp?????? - the temporary update file
.fi
.SH SEE ALSO
\fIIntro\fP(UNITY), \fInprintf\fP(UNITY),
\fIretrieve\fP(NewUNITY), \fIuprintf\fP(UNITY)
