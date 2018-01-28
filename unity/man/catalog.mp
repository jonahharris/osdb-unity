.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH "catalog" "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
catalog \- vertically formatted presentation of a UNITY table
.SH SYNOPSIS
.nf
\fIcatalog\fP [-llines] [-mmargin] [-ooffset] [-wwidth] [-n]  \\
  [-b] [-e[#]] [-v[!]] [-d] [-Itable] [-h header] [-f footer]  \\
  [ aname ... \fIin\fP ] table1 [\fIinto\fP table2] [where clause]
.fi
.SH DESCRIPTION
The \fIcatalog\fP program provides a printout of a UNITY table in vertical
(rather than tabular form)
for those records that satisfy the \fIwhere\fP condition.
The attribute name is printed in the left margin and then the attribute
value is printed.
.P
\fIcatalog\fP automatically handles wrapping attribute names and
values onto more than
one line if it encounters a lengthy value.
By default, up to 34 characters are printed for the attribute name
in the left margin,
space characters fill up to column 35, and up to
44 bytes of the attribute value are printed on a single line.
Continuing attribute names begin at column 1 and
continuing data values begin at column 35.
An attribute name and attribute value is broken at the last
space, comma, semi-colon, colon, or if not found,
the last non-alphanumeric character that can fit on a line
for the attribute.
Blanks that would appear at the beginning of the next line are ignored.
If the value does not have a non-alphanumeric character that
fits on the line,
the program will split the value at the end of the line.
.P
Two lines of dashed lines are printed between each record or tuple.
.P
\fIcatalog\fP can also be used to output only some of the attributes
of a table, thus incorporating the effect of a projection function.
If no attribute names are specified, then all attributes of the table
which do not have a "nodisplay" print justification are printed.
An attribute with an upper case ('C', 'L', or 'R') print justification
is not printed by default when no attributes are listed.
If the print width of an attribute is zero (0)
and the enviroment variable
UNITY_PRB contains the keyword=value of "pr0width=n"
then the attribute is not printed by default either.
.P
If a list of attributes appears, then the output will
contain only those fields listed in the order that they appear.
The special attribute ``all''
can be used to print all attributes in the table including the
ones which are to be treated as "nodisplay" by default.
The ``all'' attribute can be specified as
"all:nodisplay=[attrlist]" where [attrlist] is an optional comma separated
list of attributes not to be displayed with all of the other attributes
in the table.
Duplicate attribute names are allowed and the number of attributes specified is
limited by the number of attributes allowed in a description file
(currently 300 attributes).
.P
The "user\-friendly" names are printed instead of the attribute names
if they are stored in the description file
(see \fItable\fP for details).
.P
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
The command line options are as follows:
.IP "-w\fIk\fP"
The width of lines is set to \fIk\fP character positions.
By default, the print width is 79 characters per line; the
default was chosen so that tables will
be printed nicely on video terminals.
The minimum width that may be specified is 10 characters.
This width also controls the width of the separator lines.
.IP "-d"
A blank line is printed between each attribute value (double-spacing).
.IP "-m\fIk\fP"
The width of the left margin is set to \fIk\fP character positions.
By default, the left margin, in which the attribute names are printed,
is 35 characters.  This means that up to 34 characters of attribute
name are printed and spaces are used to fill up to column 35.
Attribute values are printed in columns 36 up to the width of the
page (default 79 characters).
The print width for attribute values is equal to the page width
minus the margin width.
Thus, increasing the margin size without increasing the page width
will cause the print width for attribute values to decrease.
If \fIk\fP starts with a minus ('-') then the attribute names
are left-justified by removing any leading white space (blanks)
from the friendly-user attribute names.
.IP "-o\fIk\fP"
The page offset is set to \fIk\fP characters.
By default, the page offset is zero (0).
If \fIk\fP is a negative number,
then '-' will be used in place of space characters to fill the
"offset" region of the the two default lines of hyphens (if/when printed).
If \fIk\fP is simply "-" then the two default lines of hyphens that normally
print between records will not be printed.
Note that this option extends the totol output line length
so the user may also want/need to use the page width (-w) option
to shorten the the page width if they do not want the total
output line length to change.
.IP "-l\fIk\fP"
The length of a page is set to \fIk\fP lines.
If "-l" is specified with no page length, the default is the standard
UNIX default of 66 lines per page.  The minimum allowable page length is 10.
Each record begins printing on a new page with this option and the printing
of the dashed separator lines is suppressed.
.IP "-n\fIc\fP"
This option causes all occurrences of '\fIc\fP'
to be translated into a skip to the next line.
Thus, the user can explicitly control where breaks occur in
attribute values even if the value would not fill the line.
The newline character is not printed.
This option takes priority of normal line splitting.
The program will first scan for a newline character.  Failing to find one,
it scans backwards trying to find a break character \fIonly\fP
if the entire attribute value will not fit on the current line.
Failing to find a break character, it splits
the attribute value exactly at the width of the line.
Note that all strings between the newline delimiters must be less than
or equal to the 40 characters per line.
The default for \fIc\fP is tilde (\~).
.IP "-b"
This option will suppress the printing of blank attributes.
That is, if a value does not exist for an attribute,
the line (which would consist only of the attribute name) is
not printed.
.IP "-e[#]"
Record numbers are printed at the beginning of each record (enumeration).
If the -l option is specified, then a separate line is printed at
the beginning of each record in the form "--- # ---", where "#"
is the number of the record being printed.
If an "#" is appended to the "-e" option then the actual database
record number is printed.
Otherwise, records which are in the database file but not printed
are not included in the count.
If the -l option is not specified, then the record number is
printed on the second of the dashed separator lines.
.IP "-h"
The next argument is used as a header to be printed at the top
of each page.
Four blank lines will be printed at the top of the page, followed
by the header line, followed by two blank lines, and the data.
The header argument may consist of multiple lines but it is the
user's responsibility to account for the right number of lines appearing
on the page (e.g., if the header consists of two lines, the user should
specify -l65 to get the right number of data lines output for a default
page length of 66 lines).  Note that '\\n' is \fInot\fP expanded to
a newline.  The only expansion that is done is that "%d" expands to
the current page number.  A side effect of this is that the user must
specify "%%" to get a single "%" printed (problems may occur if
a "%" is used in any other way).
This option is ignored if the -l option is not specified.
.IP "-f"
The next argument is used as a footer to be printed at the bottom
of each page.
The bottom of the page output will consist of
two blank lines, followed by the footer line,
and followed by two more blank lines.
The footer argument may consist of multiple lines but it is the
user's responsibility to account for the right number of lines appearing
on the page (e.g., if the footer consists of two lines, the user should
specify -l65 to get the right number of data lines output for a default
page length of 66 lines).
Note that '\\n' is \fInot\fP expanded to a newline; use
an actual newline as part of the value within quotes.
The only expansion that is done is that "%d" expands to
the current page number.
A side effect of this is that the user must specify "%%"
to get a single "%" printed
(problems may occur if a "%" is used in any other way).
This option is ignored if the -l option is not specified.
.IP "-v[!]"
Print verbose (friendly-user) attribute names, which is the default
if/when available.
If "-v" is followed by the "!" character then the normal
(not verbose) attribute names will be printed in the output.
.P
The \fIcatalog\fP program has no limitations regarding the amount
of data that can be printed.
The program prints lines as they are generated rather than setting them
up in arrays.
The resulting program has no data limitations.
.P
The output is written to the standard output by default.
If an "into clause" is specified, the output is written into
the named table.  An output description file is not created since
the output of the catalog command is not a UNITY table.
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
with a \fI%enddescription\fP.  This allows the program to interface directly
with the output of New Unity commands, such as \fIretrieve(1)\fP,
that do not create intermediate description files.
.P
If the output table name specified is "-", then the table output will
be written to the standard output.
.SH ENVIRONMENT
.TP
.B UNITY_PRB
This variable can be set to a comma separated list of keyword=value pairs
to alter the default output format.
The following keywords and values are recognized:
.ne 16
.IP
.nf
.ft CW
|-----------------------------------------------------------|
| KEYWORD| VALUE |   DEFAULT    |      DESCRIPTION          |
|-----------------------------------------------------------|
| alignlm|  y/n  |      n       |Align left margin (same as |
|        |       |              |using the "-m-" option).   |
|cescapes|  y/n  |      n       |Convert standard C back-   |
|        |       |              |slash escape sequence(s) in|
|        |       |              |header and/or footer lines.|
| verbose|  y/n  |      y       |Print verbose attribute    |
|        |       |              |names when available.      |
| enumfmt| string| "--- %d ---" |Format of enumerated record|
|        |       |              |numbers.                   |
| enumgap|  0-2  |      0       |Blank lines to print after |
|        |       |              |"enumfmt".                 |
|pr0width|  y/n  |      y       |Print zero (0) width attrs.|
|-----------------------------------------------------------|
.ft R
.fi
.IP
For example,
setting UNITY_PRB="enumgap=1,enumfmt=--- Record %d ---"
would change the default
enumerated record number format ("--- %d ---")
to "--- Record %d ---" and add one
blank line when either the "-l" or "-o-"
option is specified and make the output look
the same is the New Unity prblock(1) command.
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
.br
.ne 30
.SH EXAMPLE
.nf
$ catalog -m35 -w75 S

---------------------------------------------------------------------------
---------------------------------------------------------------------------

SUPPLIER NUMBER                    s1
SUPPLIER NAME                      smith
SUPPLIER'S CITY                    london

---------------------------------------------------------------------------
---------------------------------------------------------------------------

SUPPLIER NUMBER                    s2
SUPPLIER NAME                      jones
SUPPLIER'S CITY                    nice

---------------------------------------------------------------------------
---------------------------------------------------------------------------

SUPPLIER NUMBER                    s3
SUPPLIER NAME                      blake
SUPPLIER'S CITY                    paris
.fi
.SH CAVEAT
The command name \fIcatalog\fP comes from the command's original
use which was for printing a data catalog.
.SH FILES
.nf
unity/catalog.c, zcatalog.c - the C source program
bin/catalog - the executable programs (linked)
/tmp/DnoI<PID> - temporary description file when no -Itable option
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIretrieve\fP(NewUNITY), \fItable\fP(UNITY), \fIuprint\fP(UNITY)
