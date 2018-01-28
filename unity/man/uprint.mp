.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH "uprint,print" "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
uprint \- formatted presentation of a UNITY table
.SH SYNOPSIS
.nf
\fIuprint\fP  [options] [aname ... \fIin\fP ] table1 [ into table2 ] \\
 [where clause]
.fi
.SH DESCRIPTION
The \fIuprint\fP program provides a printout of a UNITY table in
tabular form for those records that satisfy the \fIwhere\fP condition.
\fIuprint\fP automatically handles wrapping header information
(i.e., attribute names) and attribute values onto more than one line
of a particular column if it encounters a lengthy value
(because of UNITY's variable-length fields).
.P
\fIuprint\fP can also be used to output only some of the attributes of a table,
thus incorporating the effect of a project function.
If no attribute names are specified, then all attributes of the table
which do not have a "nodisplay" print justification are printed.
An attribute with an upper case ('C', 'L', or 'R') print justification
is not printed by default when no attributes are listed.
If the print width of an attribute is zero (0)
and the enviroment variable
UNITY_PRT contains the keyword=value of "pr0width=n"
then the attribute is not printed by default either.
.P
If a list of attributes appears, then the table will
contain only those fields listed in the order that they appear.
The special attribute ``all''
can be used to print all attributes in the table including the
ones which are to be treated as "nodisplay" by default.
The ``all'' attribute can be specified as
"all:nodisplay=[attrlist]" where [attrlist] is an optional comma separated
list of attributes not to be displayed with all of the other attributes
in the table.
Duplicate attribute names are allowed and the number of columns is
limited by the number of attributes allowed in a description file
(currently 300 attributes).
Additionally, if the number of attributes is sufficiently large so that
all attribute columns will not fit across a line,  multiple tables or
sections are printed.
.P
If the "-Itable" option is used, the table name specified is used to determine
an alternate description file to be used in checking the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
If the input table specified is "-", then input is taken from
the standard input instead of a table.
Normally, the -I option must be used to specify the name of a table from
which the description file name can be determined and used.
If the -I option is not given, the standard input stream is checked
for the description that begins with "\fI%description\fP" and ends
with "\fI%enddescription\fP".  This allows \fIuprint\fP to interface directly
with the output of New Unity commands, such as \fIretrieve(1)\fP,
that do not create intermediate description files.
.P
The output is written to the standard output by default.
If an "into clause" is specified, the output is written into
the named table.
An output description file is not created since the output of \fIuprint\fP
is not in the format of a UNITY table.
An error is printed if the output table already exists.
If \fIonto\fP is specified instead of \fIinto\fP, the output is
appended to the table if it already exists or the table created
and written if it does not exist.
.P
If the output table name specified is "-", then the table output will
be written to the standard output.
.P
The \fIuprint\fP program leaves two lines at the top of the page and
prints a header consisting of a dashed line, the attribute
names for the columns and then another dashed line (this default can be
modified by using the "-a", "-h" and "-t" options discussed below).
If the printout is paginated (see the "-l" option below),
the header (i.e., attribute list) is printed at the top of each page,
four blank lines are printed at the bottom of each page
(see the "-f" option for printing a footer), and
the last page is filled with blank lines (i.e., newline characters).
.P
Several command line options, available for the \fIpr\fP(1) command,
are provided with the \fIuprint\fP command.
Print information specified in the description file allows
the user to specify a print width for each attribute as well as
how the attribute value is to be positioned within the attribute column.
.P
The command line options are as follows:
.IP "-d"
The output is double spaced.
If the "-d" is followed by a character, than that character is used
to print the horizontal separator line.
The default is to print spaces (except where the column separators appear).
Thus, to get separators consisting of hyphens, use "-d-".
.IP "-e[#]"
Record numbers are printed on the left of the table (enumeration).
If "#" is appended to the "-e" option
then the actual database record number is printed.
Otherwise, records which are in the database but not printed
are not included in the count.
The number is printed to the left of the first line of output
for each record.  The table is shifted five characters to the right
and the table width is decreased by five characters.
Numbering will continue until 9999 before wrapping-around to 0000.
Any header, footer, or attribute list specified with the -h, -f, or
-a print options are NOT shifted to the right.  It was decided that
the user may want a header or footer over the record numbers.
.IP "-v"
Instead of printing the attribute names as headers for each column,
the "user\-friendly" name is printed, if they are available
in the description file (see \fItable\fP for more details).
.IP "-w\fIk\fP[-\fIs\fP]"
The width of the line is set to \fIk\fP character positions.
The default width is 79 characters per line;
the default was chosen so tables will be printed nicely on video terminals.
If "-w" is specified without a width, the width defaults to 120
characters per line.
If \fIk\fP is followed by a minus sign ('-') and a digit string
then the output will be truncated to the number of sections (\fIs\fP).
If \fIk\fP is terminated by a minus sign without a digit string
then the output will be truncated to the number of fields that
fit on one line just as if \fIs\fP was set to one (1).
If \fIs\fP is set to zero (0) then no limit is put on the number
of output sections, which is the default, except when the table
comes from standard input when \fIs\fP is one (1).
.IP "-o\fIk\fP"
Each line is offset by \fIk\fP character positions (the default is 0).
The number of character positions per line is the sum of the width and
the offset.
.IP "-l\fIk\fP"
The length of a page is set to \fIk\fP lines.
The default is no pagination.
If "-l" is specified with no page length, the default is the standard
UNIX default of 66 lines per page.  The minimum allowable page length is 10.
.IP "-h"
The next argument is used as a header to be printed at the top
of each page in addition to the attribute list.
Two blank lines will be printed at the top of the page, followed
by the header line, followed by two blank lines, the attribute
list, and the table.
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
.IP "-a"
The next argument is used as the line to be printed instead of the
attribute names.
The line will be printed immediately before the table and the
attribute name line will not appear.
The argument may consist of multiple lines but it is the
user's responsibility to account for the right number of lines appearing
on the page (e.g., if the header consists of two lines, the user should
specify -l65 to get the right number of data lines output for a default
page length of 66 lines).  Note that '\\n' is \fInot\fP expanded to
a newline.  The only expansion that is done is that "%d" expands to
the current page number.  A side effect of this is that the user must
specify "%%" to get a single "%" printed (problems may occur if
a "%" is used in any other way).
.IP "-f"
The next argument is used as a footer to be printed at the bottom
of each page.
This option will work only if the "-l" option is specified.
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
.IP "-r"
No error messages are printed about failure to open files.
.IP "-t"
The five-line identifying header and the five-line trailer normally supplied
for each page are not printed.  The program also quits printing after
the last section without spacing to the end of the page.
.IP "-s\fIc\fP"
Columns are separated by the single character '\fIc\fP' instead of vertical
bars ('|').  The default for \fIc\fP is '|'.
.IP "-u"
This option causes the output to be printed "un-boxed" (i.e., blanks are
printed instead of the separator lines).
.IP "-n\fIc\fP"
This option causes all occurrences of '\fIc\fP'
to be translated into a skip to the next row within the attribute column.
Thus, the user can explicitly control where breaks occur in
attribute values even if the value would not fill the column.
The newline character is not printed.
For example, a description field in a data catalog could
be stored as a single UNITY field with tildes representing newlines.  When
printing the fields, the tildes could be translated into new rows to
correctly format the description.  Note that all strings
between the newline delimiters must be less than or equal to the column width
for the attribute so that the formatting will work correctly.
The default for \fIc\fP is tilde (~).
.IP "-b"
This option will cause all \fImulti-line\fP fields to be split at the
last space, comma, semi-colon, colon, or if not found,
the last non-alphanumeric character that appears on a row
for the attributes.
Blanks that would appear at the beginning of the next line are ignored.
If the row does not have a non-alphanumeric character,
the program will split the value at the column end.
For example, the attribute value "hello, world" being put in a
9-character column will be split as "hello," and "world"
using the "-b" option, and as "hello, wo" and "rld" otherwise.
If the fields are not multi-line, this option has no effect on the output.
.sp
The "-n" option takes priority over the "-b" option.
Thus if the "-n" and "-b" options are both given,
the program will first scan for a newline character.  Failing to find one,
it scans backwards trying to find a break character \fIonly\fP
if the entire attribute value will not fit into the column width specified for
the field.  Failing to find a break character, it splits
the attribute value exactly at the field width.
Note that the "-n" has an effect even if the attribute value will fit on
the row while the "-b" option takes effect only on multi-line attribute values.
.IP "-p"
The "-p" (paging) option indicates that records should not be printed across
pages when possible.
The number of lines needed to print a record is
calculated before printing begins and if the record will not fit
on the current page, printing will start on a new page.
The only time that a record will print across more than one page is
if it will not fit on a single page.
This option is ignored if the -l option is not specified.
.IP "-R"
This option suppresses the printing of repeated attribute values
from record to record
when they appear on the left-hand side of the table (see example
in EXAMPLES section).
All attribute values are printed for the first record on each page.
.IP "-I"
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
The print information that is stored in the description
file is handled as follows.
The user may specify print information for any attribute field in the
description file by following the attribute type with a tab and then
the print width appended with the positioning character.
The print width may be any integer value (within the limits of an
\fIint\fP for the machine);
the \fIuprint\fP program checks for a reasonable print width as explained below.
The positioning character may be 'l', 'r',
or 'c' for left justification, right justification, or centering
respectively.
The default positioning is left justification.
Note that both the print width and justification can be changed
by inputing them directly on the command line immediately following the
attribute name (see below).
.P
The user may enter the printing information directly with 
the \fItable\fP command by inputing the attribute type and the
print information as a single parameter
(quotes are
necessary so that it will be taken as a single parameter).
The format for specifying an attribute type is
.sp
.nf
{w<length> | t<terminator>}[<tab>[print width][position]]
.fi
.sp
The following is
an example of a definition for the /etc/passwd file with print
information specified.
.sp
.nf
login  t:      8l
passwd t:      14l
uid    t:      5r
gid    t:      5r
acct   t:      20l
home   t:      15l
shell  t\\n     12l
.fi
.fi
.sp
Note, in the example, that the "login" attribute
is left justified whereas the "uid" and "gid" attributes
are right justified.
.P
Column width for an attribute is determined as follows:
.IP 1.
If an attribute list is specified on the command line and
an attribute name is specified with a colon (:) and an
integer value appended, then that integer value is taken
to be the field width and over-rides
all other considerations (e.g., uprint login:12 in /etc/passwd
would print the login field with a field width of 12 characters).
The integer value may be followed by a 'l', 'c', or 'r' which
is taken as a justification character (left, center, or right, respectively).
.IP 2.
If a print width is given in the description file, it over-rides all
considerations other than if a width is specified on the
command line.  However, if it is less than 1, it is ignored.
Note that if the print width is less than the length of the
attribute name, the attribute name will be wrapped around in the
header.
.IP 3.
If the field is a fixed width field, the column width is the maximum
of the attribute name length and the attribute width.
.IP 4.
If the field is a terminator field, the column width is set to a
fixed default.  The default is currently 12 and may be changed by
re-defining COLW in \fIzuprint.c\fP and compiling/linking
the program again.
.IP 5.
If the resulting column width as determined from steps 1, 2, 3, and 4 above
is greater than the line width minus two
(since two character positions are reserved for the beginning and ending "|" of
the table), the value is reset to line width minus two.
.P
The \fIuprint\fP program has no limitations regarding the amount
of data that can be printed.
The program prints lines as they are generated rather than setting them
up in arrays.
The resulting program has no data limitations.
.SH ENVIRONMENT
.TP
UNITY_PRT
This variable can be set to a comma separated list of keyword=value pairs
to alter the default \fItop\fR and \fIbottom\fR of page spacing
as well as the default two line spacing that appears after
an optional table header when the table is not to be printed across
multiple pages (i.e., -l, -p, and -t options have not been used).
If the table does not fit into a single section on the page
then all keyword=value options are ignored if the "msecgap"
(multiple section gap) keyword has not been specified.
In addition, \fIcheaders\fR can be set to "y" to indicate
that the attribute name (column) headers are to be centered
and \fIcescapes\fR can be set to "y" to indicate that
any Standard C backslash excape sequences in header
or footer lines are to be converted/replaced
by the equivalent character value.
Note that the "-f <footer>" can be used to add a footer
to the end of the table as long as the \fIbottom\fR keywork
has been set to a valid value.
The following keywords and values are recognized:
.ne 9
.IP
.nf
.ft CW
|-----------------------------------------------------------|
| KEYWORD|VALUE|DEFAULT|         DESCRIPTION                |
|-----------------------------------------------------------|
| top    | 0-8 |   2   |Spacing at top of page.             |
| head   | 0-4 |   2   |Spacing after optional header.      |
| bottom | 0-8 |   0   |Spacing after table and/or footer.  |
| msecgap| 0-4 |   2   |Spacing (gap) between sections.     |
|cheaders| y/n |   n   |Center attribute name in headers.   |
|cescapes| y/n |   n   |Convert Standard C backslash escape |
|        |     |       |sequences in header or footer lines.|
|pr0width| y/n |   y   |Print zero (0) width attributes.    |
|-----------------------------------------------------------|
.ft R
.fi
.IP
For example,
setting UNITY_PRT="cheaders=y,bottom=4" would
center all attribute names in the output table column headers
and add 4 blank lines at the end of the table
which is the way the New Unity prtable(1) command works.
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
.SH EXAMPLES
.nf
.in 0
.ne 19
$ uprint -w65 /etc/passwd    # simple uprint command


|--------------------------------------------------------------|
|login   |passwd        |  uid|  gid|acct                      |
|--------------------------------------------------------------|
|root    |WrO2xgZ/9QZIc |    0|    1|3784,Admin                |
|sync    |              |   20|    1|3784,Admin                |
|--------------------------------------------------------------|


|----------------------------|
|home           |shell       |
|----------------------------|
|/              |            |
|/              |/bin/sync   |
|----------------------------|

.ne 10
$ uprint -v -b login uid in /etc/passwd      # uprint with projection


|--------------|
|LOGIN   | USER|
|NAME    |   ID|
|--------------|
|root    |    0|
|sync    |   20|
|--------------|


.ne 34
$ uprint -l15 -h "`date '+%m/%d/%y'`    GENERAL HEADING        Page %d
Another line of heading" login gid in /etc/passwd


02/08/82    GENERAL HEADING        Page 1
Another line of heading


|--------------|
|login   |  gid|
|--------------|
|last    |   11|
|root    |    1|
|--------------|






02/08/82    GENERAL HEADING        Page 2
Another line of heading


|--------------|
|login   |  gid|
|--------------|
|uname   |    1|
|daemon  |    1|
|--------------|

.ne 34
$ uprint -l15 -h "`date '+%m/%d/%y'`    GENERAL HEADING        Page %d
Another line of heading" -a "|attribute list|" login gid in /etc/passwd


02/08/82    GENERAL HEADING        Page 1
Another line of heading


|attribute list|
|--------------|
|last    |   11|
|root    |    1|
|uname   |    1|
|--------------|






02/08/82    GENERAL HEADING        Page 2
Another line of heading


|attribute list|
|--------------|
|daemon  |    1|
|bin     |    2|
|adm     |    4|
|--------------|
.sp
.ne 14
$ uprint table                   $ uprint -R table
                                
                                
|--------------------------|     |--------------------------|
|a1      |a2      |a3      |     |a1      |a2      |a3      |
|--------------------------|     |--------------------------|
|abc     |def     |ghi     |     |abc     |def     |ghi     |
|abc     |def     |ghj     |     |        |        |ghj     |
|abc     |deg     |ghj     |     |        |deg     |ghj     |
|abd     |deg     |ghj     |     |abd     |deg     |ghj     |
|--------------------------|     |--------------------------|
.fi
.in
.fi
.SH BUGS
When output is too wide for the page, \fIuprint\fP
separates it into sections (separate tables), and
rereads the relation for each one.
When input is coming from a pipe this doesn't work,
since a pipe can't be rewound and reread.  In this
case, only the first section is printed.
.P
To remedy this, put the data into a temporary file and
run \fIuprint\fP against that, or use the
\fIretrieve -T\fP NUNITY command, which doesn't have
that problem.
.SH FILES
.nf
unity/uprint.c, zuprint.c - the C source program
bin/uprint - the executable program
/tmp/DnoI<PID> - temporary description file when no -Itable option
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIcatalog\fP, \fIuprintf\fP, \fItable\fP(UNITY), \fIpr\fP(1),
\fIretrieve\fP(NewUNITY)
