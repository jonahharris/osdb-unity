.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH nprintf "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
.nf
nprintf \- numeric-formatted print on UNITY tables
.fi
.SH SYNOPSIS
.nf
\fInprintf\fP [-a] [-c] [-n] [-q] [-Itable] [-Otable] \\
 [-C <CenturyDigits>] [-S] [-W <DateWarningLimit>] \\
 aname1 [:modifier] [%format] [anameN ...] \\
 \fIin\fP table1 [\fIinto\fP table2] [where clause]
.fi
.SH DESCRIPTION
The \fInprintf\fP command converts between various numeric or string
data formats for any number of specified attributes in a UNITY table.
All attributes in the UNITY table that are not specified on the command line
are copied without change.  Thus, the output record format is always the same
as the input record format.
.P
An optional attribute \fImodifier\fP can be specified (as described below)
to perform input base conversion on numeric attribute values, to trim blanks
(space characters), to convert attribute values to upper or lower case,
and/or to convert date strings from one format to another.
For all attribute modifiers except date,
the \fIsprintf(3C)\fP library function is then called with
the given \fI%format\fP specification to print the numeric result or
new string value of the attribute into the new format specification that
replaces the original attribute value in the output table.
.P
The format specifications starts with the percent (``%'')
format conversion character followed by an optional justification
character, optional field width and precision, and ends with the
type of conversion character.  The following regular expression
defines the \fI%format\fP specifications that are allowed:
.P
.nf
.ce
[+ -#]*[0-9]*\.{0,1}[0-9]*[bdiouxXeEfgGs]
.fi
.P
The 'b' conversion character is used for unsigned binary output
that is performed by a special low level UNITY library function,
sprintfb(UNITY),
that works similar to the way sprintf(3C) works for the 'x' (hexadecimal)
or 'o' (octal) format conversion characters.
If \fInprintf\fP has been compiled with the \fIINTCOMP\fP flag
then floating point type input and output conversions
are not supported.
.P
The format specification for dates are identifed by
an upper case ``D'' as the conversion character after
the percent (``%'').
The ``D'' may be preceded by an optional
``/'', ``-'', ``.'', or ``#'' to specify
a particular type of date format.
The "%/D" or default ("%D") date format is "<month>/<day>/<year>".
The "%-D" date format is "<month>-<day>-<year>".
The "%.D" date format is "<day>.<month>.<year>".
The "%#D" date format is "YYMMDD" or "YYYYMMDD"
where month (MM) and day (DD) are 2-digits and
year is either 2-digits (YY) or 4-digits (YYYY),
respectively.
The -C option can be used to control whether year is
printed as 2-digits or 4-digits (see PARAMETERS Section below).
.P
A date specifications can also include time which
is identifed by the ``T'' conversion character (e.g., "%D%T").
The ``T'' may be preceded by an optional
``:'', ``.'', or ``#'' to specify a particular
type of format for the time portion of the date.
The "%:T" time format is "HH:MM:SS" where hours (HH),
minutes (MM), and seconds (SS) are all 2-digits,
which is the default when date is specified as
"%D", "%/D", "%-D".
The "%.T" time format is "HH.MM.SS",
which is the default when date is specified as "%.D"
(i.e., "%.D%T").
The "%#T" time format is "HHMMSS",
which is the default when date is specified as "%#D"
(i.e., "%#D%T").
The -S option can be used to print time without
seconds when ``:'' or ``.'' is used
as the sub-field separator (see PARAMETERS Section below).
.P
Normally a space character is used as the separator
between the date and time except for "%#D%T" or "%#D%#T"
which default to no separator (i.e., "YYMMDDHHMMSS" or
"YYYYMMDDHHMMSS").
This can be modified by adding a literal space, literal tab,
or "%t" (which will be converted to a tab character) between
the date (%D) and time (%T) in the format specification
(e.g., "%D%t%T").
.P
By default, null attribute values are not converted or reformatted
regardless of which records have been selected via the where-clause.
The \fB-n\fP (null) option can be used to indicate that
null attribute values should also be formatted using
a numeric value of zero (0) or the null ("") string, as appropriate.
.P
An update of the input table (table1) is performed when no
\fI"into table2"\fP clause has been
given and table1 is not standard input (-).
During a table update, only the records that match
the \fIwhere-clause\fP are modified.
All other records are copied without change.
.P
When an \fI"into table2"\fP clause is given
or table1 is standard input,
only the records that match the \fIwhere-clause\fP
are selected and printed into/onto the given output table.
In this case, the \fB-a\fP (all) option can be used
if the user wants all of the input records to be printed,
not just the ones that have been selected
to be numerically reformatted and printed.
If \fIonto table2\fP is specified instead of \fIinto table2\fP,
the output is appended to table2 if it already exists or table2 is created
if it does not exist.
.P
When the \fI"into table2"\fP clause is used,
the input table description is copied to the
same directory as the output data file when
the output description cannot be found via the
\fBUNITYDFILES\fP environment variable.
This is different than most (all) other original
UNITY commands that normally create the output
description file in the current directory
and is intended to provide greater compatability
with the New UNITY commands.
An error is printed and the program terminates
if the output description already exists in either
the current or data directories.
If \fIonto\fP is specified instead of \fIinto\fP
then the output description will be created
in the data directory if it does not already
exist in the current or data directory
(and cannot be found via UNITYDFILES).
.P
If the input table specified is "-", then input is taken from
the standard input instead of a table.
Normally, the -I option must be used to specify the name of a table from
which the description file name can be determined and used.
If the -I option is not given, the standard input stream is checked
for the description that begins with "\fI%description\fP" and ends
with "\fI%enddescription\fP".  This allows \fInprintf\fP to interface directly
with the output of New Unity commands, such as \fIretrieve(1)\fP,
that do not create intermediate description files.
.P
If the output table name specified is "-", then the table output will
be written to the standard output.
Normally, the -O option must be used to specify the name of a table from
which the output description file name can be determined.
However, the -c option can be used instead to copy the description
to standard output starting with "\fI%description\fP" and ending with
"\fI%enddescription\fP".
This allows \fInprintf\fP to be used as a filter between
various UNITY or New UNITY commands without having
to create/cleanup intermediate description files.
.P
When the -O option is used,
the description file will be created in the current directory
with one exception.  If the alternate table name
is the same as the output data table name,
then the description file will be created in the data directory.
If no table is given with the -O option then no output description
file is created.
.SH PARAMETERS
.TP
.B -C<CenturyDigits>
This option is used to control the expansion of
2-digit years (YY) in date type attributes that are being
reformatted (i.e., "%D" type format specifications)
to be have the given century digits which can range
from ``01'' to ``99''.
The default (0) is to use the current year minus 60 years
as the start of a 100 year "window" that is used
to calculate and add the missing century digits
to date type attributes that only have 2-digits for the year.
If "-" is given instead of a number, then
all date type attributes that are being reformatted
(i.e., "%D" type format specifications)
will be updated (printed) with 2-digit years
and any century digits will be removed.
.TP
.B -Itable
When the "-Itable" option is used, the table name specified is used to determine
an alternate description file to be used in reading the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.TP
.B -Otable
When the "-Otable" option is used, the table name specified is used to create an
an alternate description file for the output table and will always be
created in the current directory with one exception.
If the alternate table name is the same
as the output data table name,
then the description file will be created in the data directory.
If no table is given with the -O option then no output description
file is created.
.TP
.B -S
Do not include seconds in date type attributes that are being
reformatted (i.e., "%D%T" type format specifications).
In other words, the time portion of date (and time) type attributes
will be formated as "HH:MM" (or "HH.MM") instead of "HH:MM:SS"
(or "HH.MM.SS").
This option does not change the format when "%#T" is specified
(i.e., "HHMMSS").
.TP
.B -W<DateWarningLimit>
This option is used to over-ride the default limit of 10
date error warnings that are reported when "%s" is used
as the format specification with the ":date" modifier
and the value of any date type attribute is not a valid date
(see the ":date" modifier in the attribute modifier section below).
.TP
.B -a
Print all records \(em not just the ones that match the where-clause
(if specified).
.TP
.B -c
Create a copy of the output descriptor on the output data stream.
This is only valid when the output table is going to standard output
and the \fI-Otable\fP option has not been used.
.TP
.B -n
Null attribute values are also to be numerically-formatted
using a value or zero (0) or the null string (""), as appropriate.
Normally, null attribute values are copied without change
(i.e., as null values) to the output table.
.TP
.B -q
Operate in quiet mode.
Normally, the number of records updated and/or printed
is reported (printed) on standard error.
.P
An attribute modifier is used to indicate
special handling for the given attribute.
Only one modifier is allowed per attribute and
must begin with a colon (``:'').
The modifiers can be separated by white space or
can immediately follow the attribute name and may be
truncated to any number of characters as long as
the remaining part of the modifier is unique.
The following attribute modifiers are recognized:
.br
.ne 4
.TP
:\fIb\fPinary
The initial value of the attribute is to be treated as
an unsigned binary integer.
.TP
:\fIda\fPte
The attribute is to be interpreted as a date (or date and time).
If no format specification or "%s" is given,
than a warning message will be printed for any attribute
value that cannot be interpreted (parsed) as a valid date
or date and time value.
It should be noted that zero is not a valid year
and any date with year of zero that is more than
2-digits will be flagged as an error (warning).
If a valid date (%D) or date and time (%D%T) type
format specification is given,
then any date errors will cause nprintf(1) to
terminate with an error.
.TP
:\fIh\fPex
The initial value of the attribute is to be treated as
an unsigned hexadecimal integer.
.TP
:\fIltr\fPim
Leading (left) space characters are removed before
sprintf(3C) is called to print the new attribute value.
.TP
:\fIltrl\fPower
Leading (left) space characters are removed and upper case characters
are converted to lower case before
sprintf(3C) is called to print the new attribute value.
.TP
:\fIltru\fPpper
Leading (left) space characters are removed and lower case characters
are converted to upper case before
sprintf(3C) is called to print the new attribute value.
.TP
:\fIn\fPumeric
The initial value of the attribute is to be treated as numeric or
or double precision floating point number.
When compiled with the \fIINTCOMP\fP flag,
all numeric input values are converted to integer type numbers
instead of double type numbers.
.TP
:\fIo\fPctal
The initial value of the attribute is to be treated as
an unsigned octal integer.
.TP
:\fIrtr\fPim
Trailing (right) space characters are removed before
sprintf(3C) is called to print the new attribute value.
.TP
:\fIrtrl\fPower
Trailing (right) space characters are removed and upper case characters
are converted to lower case before
sprintf(3C) is called to print the new attribute value.
.TP
:\fIrtru\fPpper
Trailing (right) space characters are removed and lower case characters
are converted to upper case before
sprintf(3C) is called to print the new attribute value.
.TP
:\fIs\fPtring
The initial value of the attribute is to be treated as a string.
This is the default treatment when no attribute modifier
and no \fI%format\fP specification has been given.
The use of \fIa\fPscii in place of \fIs\fPtring
is allowed for backwards compatibility but
is discouraged.
.TP
:\fItr\fPim
Both leading and trailing space characters are removed before
sprintf(3C) is called to print the new attribute value.
.TP
:\fItrl\fPower
Both leading and trailing space characters are removed
and upper case characters are converted to lower case before
sprintf(3C) is called to print the new attribute value.
.TP
:\fItru\fPpper
Both leading and trailing space characters are removed
and lower case characters are converted to upper case before
sprintf(3C) is called to print the new attribute value.
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
.SH CAVEATS
The user is not allowed to modify fixed width attributes.
However, no error is reported/generated if a fixed
width attribute is given with the default string copy ("%s")
format specification since it does not alter
the attribute value(s).
.br
.ne 5
.SH EXIT CODES
.TP
.B 0
Successful termination.
.TP
.B 1
Invocation error or unsuccessful termination.
.br
.ne 6
.SH EXAMPLE
.nf
.ft CW
$ retrieve -qcd"    " rec# as a1 rec# as a2 rec# as a3 from /etc/passwd |
> nprintf a1 %05s a2 :binary%d a3:hex%1.3f in - where rec# = 10 or rec# = 11
00010   2       16.000
00011   3       17.000
nprintf: 2 records printed in -
.ft
.fi
.SH FILES
.nf
unity/nprintf.c, znprintf.c - the C source program
bin/nprintf - the executable program
/tmp/DnoI<PID> - temporary description file when no -Itable option
<directory of table>/L<table> - the lockfile
<directory of table>/ctmp?????? - the temporary update file
.fi
.SH SEE ALSO
\fIIntro\fP(UNITY), \fIcompute\fP(UNITY), \fInalter\fP(NewUNITY),
\fIretrieve\fP(NewUNITY), \fIuprintf\fP(UNITY)
