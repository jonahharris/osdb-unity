.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH "prtable" 1 "" "New UNITY"
.SA 1
.SH NAME
prtable \- Format lines read from standard input into a table.
.SH SYNOPSIS
.HP
.B prtable
[-beNpRtU]
.RI [-d c ]
.RI [-D c ]
.RI [-F n ]
.RI [-f <pg_footer> ]
.RI [-h <pg_header> ]
.RI [-i <file> ]
.RI [-L n ]
.RI [-n c ]
.RI [-o n ]
.br
.RI [-W n[-s] ]
.RI [-c n1 , n2 ,...]
.RI [-l n1 , n2 ,...]
.RI [-r n1 , n2 ,...]
.br
.RI -C " <column_heading>"
.B |
.IR <column_names> ...
.SH DESCRIPTION
The
.B prtable
command formats each line of input into a table with headings for each
column.
.B prtable
automatically handles wrapping column headings
and field values onto more than one line
of a particular column, if it encounters a lengthy value.
Additionally, if the number of fields is sufficiently large so that
all columns will not fit in one line, multiple sections of the
table are printed.
.P
The
.B prtable
command leaves two lines at the top of the page and
prints a header consisting of a dashed line, the column headings,
and then another dashed line.
(This default can be
modified by using the
.BR -C ,
.BR -h ,
and
.BR -t
options discussed below).
If the output is paginated (see the
.BR -p
option),
the page header, if any, and the column headings
are printed at the top of each page.
At the bottom of each page
four blank lines are printed or a footer is printed
(see the
.BR -f
option for printing a footer).
The last page is filled to capacity with blank lines (i.e., newline characters).
The two blank lines at the top of the page and the four blanks lines
after the optional footer can be supressed and/or changed via the
\fIUNITY_PRT\fR environment variable for tables that are/can be printed
in one section (see \fBENVIRONMENT\fR Section).
.P
Command line options allow
the user to specify a print width for each column as well as
how each value should be positioned within the column.
.SH PARAMETERS
.TP 10
.B -b
This option causes
.I multi-line
columns to be split at the
last space, punctuation mark, or
non-alphanumeric character that appears on a line
for each column.
Blanks that would appear at the beginning of the next line are ignored.
If the row does not have a space or non-alphanumeric character,
the program will split the value at the column width.
For example, the field value ``\f(CWhello, world\fP''
.R
being put in a
9-character column will be split as ``\f(CWhello,\fP'' and ``\f(CWworld\fP''
.R
using this option.
Otherwise, it would be split as ``\f(CWhello, wo\fP'' and ``\f(CWrld\fP''.
.R
If the field values are not multi-line, this option has no effect
on the output.
.IP
The
.B -n
option (see below) takes priority over the
.B -b
option.
Thus if both options are given,
the program will first scan for a newline character.
Failing to find one,
it scans backwards trying to find a break character
.I only
if the entire field value will not fit into the column width specified for
the field.
Failing to find a break character, it splits
the field value exactly at the column width.
The
.B -n
option has an effect even if the field value will fit on
the line, while the
.B -b
option takes effect only on multi-line field values.
.TP
.BI -C " <column_heading>"
The next argument is used as the column heading line to be printed instead of the
column names.
The line will be printed immediately before the table and the
normal column name line will not appear.
The column heading may consist of multiple lines, but it is the
user's responsibility to account for the right number of lines appearing
on the page.
For example, if the column heading consists of two lines, the user should
specify
.B -L65
to get the right number of data lines output for a default
page length of 66 lines.
Note that ``\en'' is
.I not
expanded to a newline.
.IP
One caveat of this option is that it only works properly when
the total width of a table is less than the page width.
Otherwise, the table must be broken into multiple sections and
the same column header is used for each section.
.P
.ne 5
.nf
\fB-c\fIn1\fR,\fIn2\fR,...
\fB-l\fIn1\fR,\fIn2\fR,...
.fi
.PD 0
.TP
\fB-r\fIn1\fR,\fIn2\fR,...
Specify the justification and width of columns.
The columns are centered
.RB ( -c ),
left justified
.RB ( -l ),
or right justified
.RB ( -r ).
The column widths are set to
.IR n1 ,
.IR n2 ,...
If no numbers are given, one column of width 12 is assumed.
There may be any number of occurrances of these arguments on the command
line.
If more column names are given than justification arguments,
the remaining columns are left-justified of width 12.
Any extra justification arguments are ignored.
.PD
.TP
.BI -D c
Double space the output.
An extra line is printed between each input record in the output.
An extra line is not printed between output lines belonging to
the same input record.
If
.RI `` c ''
is given, it is used
to print the separator line.
The default is to print spaces except where the column separators appear.
Thus, to get separator lines consisting of hyphens, use ``-d-''.
.TP
.BI -d c
The character
.RI `` c ''
is the field delimiter character for dividing up each input line.
The default delimiter is ``:'' (colon).
.TP
.B -e[n]
Enumerate the records.
The record number is printed to the left of the first line of output
for each record.  If ``n'' is specified then record numbering
starts with ``n'' instead of ``1''.
The table is also shifted five characters to the right
and the table width is decreased by five characters to accommodate the numbers.
Numbering will continue until ``9999'' and then wrap-around to ``0000''.
If ``n'' is specified and starts with a leading ``0'' or if ``n''
is greater than ``9999'', then all record numbers will be output
as 4-digit (zero-padded) numbers.
Any column header, footer, or page header line specified with the
.BR -C ,
.BR -f ,
or
.BR -h
options are
.I not
shifted to the right;
the user may want a header or footer over the record numbers.
.TP
.BI -F n
The number of fields in each record
.I must
be
.IR n .
The number
.I n
must also be greater than or equal to the number of column names,
if the
.B -C
option is not given.
Any extra columns in the input are never printed.
.TP
.BI -f " <pg_footer>"
The next argument is used as a footer to be printed at the bottom
of each page.
The bottom of the page output will consist of
two blank lines, followed by the footer line,
and followed by two more blank lines.
The page footer
may consist of multiple lines, but it is the
user's responsibility to account for the right number of lines appearing
on the page.
For example, if the page footer consists of two lines, the user should
specify
.B -L65
to get the right number of data lines output for a default
page length of 66 lines.
Standard C escape conventions (e.g. ``\en'') are recognized and translated
to the corresponding value.
In this case the footer should be quoted to avoid backslash interpretation
by the shell.
The footer may also contain a single ``%d'' which is expanded to
the current page number.
(See the manual page for
.BR printf (3)
for more details.)
This option implies pagination of the output.
.TP
.BI -h " <pg_header>"
The next argument is used as a header to be printed at the top
of each page in addition to the column header line.
Two blank lines will be printed at the top of the page, followed
by the header line, two blank lines, and the column headers,
and the table.
The header argument may consist of multiple lines, but it is the
user's responsibility to account for the right number of lines appearing
on the page.
For example, if the header consists of two lines, the user should
specify
.B -L65
to get the right number of data lines output for a default
page length of 66 lines.
Standard C escape conventions (e.g. ``\en'') are recognized and translated
to the corresponding value.
In this case the header should be quoted to avoid backslash interpretation
by the shell.
The header may also contain a single ``%d'' which is expanded to
the current page number.
(See the manual page for
.BR printf (3)
for more details.)
This option implies pagination of the output.
.TP
.BI -i <file>
The data to be formatted is taken from the given
.I file.
Without this option, the data is taken from standard input.
.TP
.BI -L n
The length of a page is set to
.I n
lines.
If no page length is given, the default is 66 lines per page.
The minimum allowable page length is 10.
.TP
.B -N
Do
.I not
paginate the output.
By default, output is normally paginated.
Pagination helps readability, especially when the table
is too wide to fit on the screen.
This option turns pagination off, independent of the width of the table.
(See the
.B -p
option below.)
.IP
This option is ignored if any of the
.BR -f ,
.BR -h ,
or
.BR -L
options are also given.
.TP
.BI -n c
This option causes all occurrences of
.RI `` c ''
to be translated into a skip to the next row within a column.
Thus, the user can explicitly control where breaks occur in
field values even if the value would not fill the column.
For example, a description field in a data dictionary could
be a single field with tildes representing newlines.
When
printing the field, the tildes would be translated into newlines to
correctly format the description.
Note, however, that all strings
between the newline delimiters must be less than or equal to the column width
for the formatting to work correctly.
If
.RI `` c ''
is not given, the default is tilde (``~''),
.IP
The
.B -n
option takes priority over the
.B -b
option (see above).
Thus if both options are given,
the program will first scan for a newline character.
Failing to find one,
it scans backwards through the field value trying to find a break character
.I only
if the entire field value will not fit into the column width specified for
the field.
Failing to find a break character, it splits
the field value exactly at the column width.
The
.B -n
option has an effect even if the field value will fit on
the line, while the
.B -b
option takes effect only on multi-line field values.
.TP
.BI -o n
Each line is shifted to the right by
.I n
character positions.
The default for
.I n
is 0.
The number of characters printed per line is the sum of the page width
(see the
.B -W
option below)
and
the offset.
.TP
.B -p
Paginate the output.
By default, output is normally paginated;
this option is provided to override the \fB-N\fP option.
Pagination helps readability, especially when the table
is too wide to fit on the screen.
.IP
When paginating, records are not printed across
pages whenever possible.
The number of lines needed to print a record is
calculated before printing begins and if the record will not fit
on the current page, printing will start on a new page.
The only time that a record will print across more than one page is
if it will not fit on a single page.
.TP
.B -R
This option suppresses the printing of repeated field values
from record to record
when they appear on the left-hand side of the table.
All field values are printed for the first record on each page, however.
The input should be sorted in order for this option to be very useful.
.IP
When the double space option (\fB-D\fR) is also given, the redundant
field values are separated from the next row by spaces,
not the row delimiter given with the \fB-D\fR option.
This gives a more heirarchical appearance to the table.
.TP
.BI -s c
Columns in the table are separated by the single character
.RI `` c ''
instead of a vertical bar (``|'').
If
.I c
is not given, columns will remain separated by a vertical bar (``|'').
.TP
.B -t
The five-line identifying header and the five-line trailer normally supplied
for each page are not printed.
The command also quits printing after
the last section without spacing to the end of the page.
.TP
.B "-U"
Print the output ``un-boxed''.
Instead of printing hyphens (``-'') and vertical bars (``|'')
around the table, blanks are printed.
.TP
.BI -W n[-s]
The width of the output line is set to
.I n
characters.
If
.I n
is a followed by a minus sign ('-') and a digit string
then the output will be truncated to
the number of fields that fit in the given number of sections (\fIs\fP).
If
.I n
is terminated by a minus sign without a digit string
then the output will be truncated to
the number of fields that fit on one output line just as if
.I s
was set to one (1).
If
.I s
is set to zero (0)
then no limit is put on the number of output sections, which is the default
and the same as simply specifying "-W n".
.IP
The default width is 79 characters per line;
the default was chosen so tables will be printed nicely on video terminals.
If
.I n
is not given, the width defaults to 120
characters per line.
The minimum allowable page width is 10.
The actual number of characters per line is the sum of the width and
the offset.
(See the
.B -o
option above.)
.SH ENVIRONMENT
.TP 10
UNITY_PRT
This variable can be set to a comma separated list of keyword=value pairs
to alter the default \fItop\fR and \fIbottom\fR of page spacing
as well as the default two line spacing that appears after
an optional table header when the table is not to be printed across
multiple pages (i.e., -L, -p, and -t options have not been used).
If the table does not fit into a single section on the page
then all keyword=value options are ignored if the "msecgap"
(multiple section gap) keyword has not been specified.
The following keywords and values are recognized:
.ne 8
.IP
.nf
.ft CW
|-----------------------------------------------------------|
| KEYWORD|VALUE|DEFAULT|         DESCRIPTION                |
|-----------------------------------------------------------|
| top    | 0-8 |   2   | Spacing at top of page.            |
| head   | 0-4 |   2   | Spacing after optional header.     |
| bottom | 0-8 |   4   | Spacing after table and/or footer. |
| msecgap| 0-4 |   2   | Spacing (gap) between sections.    |
|-----------------------------------------------------------|
.ft R
.fi
.P
For example,
setting UNITY_PRT="bottom=0" would supress the 4 blank lines
that appear at the end of the table which is the way uprint(1) works.
.SH EXIT CODES          \" strongly recommended
.so Exit.codes.h
.SH RESTRICTIONS        \" optional - includes caveats and bugs
There is no maximum record size limit, but each individual
attribute is limited to the size of an I/O block: 10K on mainframes,
6K on Vaxen and other mid-size machines, and 4K on other smaller machines.
.P
There is a limit on the number of columns.
Currently, this is 300 on mainframes and 100 on all other machines.
.SH EXAMPLES
Print
.I /etc/passwd
as a table with enumeration and default column justifications and widths.
The delimiter will be a colon (``:'').
.IP
\f(CWprtable -eD':' -W65 login passwd uid gid acct home shell\fP
.P
Some example output for the above command would be:
.nf
.ne 15
.IP
\f(CW     |----------------------------------------------------------------|
     |login       |passwd      |uid         |gid         |acct        |
     |----------------------------------------------------------------|
   1 |root        |WrO2xgZ/9QZI|0           |1           |3784,Admin  |
     |            |c           |            |            |            |
   2 |sync        |            |20          |1           |3784,Admin  |
     |----------------------------------------------------------------|


     |-------------------------|
     |home        |shell       |
     |-------------------------|
   1 |/           |            |
   2 |/           |/bin/sync   |
     |-------------------------|\fP
.fi
.P
Print
.I /etc/passwd
again, but with different justifications and widths, but no enumeration:
.nf
.ne 2
.IP
\f(CWprtable -d':' -W65 -l8,15 login passwd -r5,5 uid gid	\e
	-l26,16,12 acct home shell\fP
.fi
.P
Some example output for the above command would be:
.nf
.ne 15
.IP
\f(CW|--------------------------------------------------------------|
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
|----------------------------|\fP
.fi
.P
Print out two columns
with a fancy page heading and column heading.
Paginate the output with 15 lines per page:
.RS .5i
.P
.nf
\f(CWprtable -L15 -l12 -r5 -C "| col. heading |"	\e
	-h "`date '+%m/%d/%y'`    GENERAL HEADING        Page %d
Another line of heading"\fP
.fi
.RE
.P
Some example output of the above command might be:
.RS .5i
.P
.nf
.ne 15
\f(CW02/08/82    GENERAL HEADING        Page 1
Another line of heading


| col. heading |
|--------------|
|last    |   11|
|root    |    1|
|uname   |    1|
|5esslmr |  246|
|--------------|




.ne 15
02/08/82    GENERAL HEADING        Page 2
Another line of heading


| col. heading |
|--------------|
|daemon  |    1|
|bin     |    2|
|adm     |    4|
|--------------|\fP





.fi
.RE
.P
Print out three columns, once normally and once with duplicates
removed.
For the normal output, the command would be
.IP
\f(CWprtable -l6,6,6 a1 a2 a3\fP
.P
The normal output would be:
.nf
.ne 8
.IP
\f(CW|--------------------------|
|a1      |a2      |a3      |
|--------------------------|
|abc     |def     |ghi     |
|abc     |def     |ghj     |
|abc     |deg     |ghj     |
|abd     |deg     |ghj     |
|--------------------------|\fP
.fi
.P
The same information printed with duplicates removed
.IP
\f(CWprtable -R -l6,6,6 a1 a2 a3\fP
.P
would be:
.nf
.ne 8
.IP
\f(CW|--------------------------|
|a1      |a2      |a3      |
|--------------------------|
|abc     |def     |ghi     |
|        |        |ghj     |
|        |deg     |ghj     |
|abd     |deg     |ghj     |
|--------------------------|\fP
.fi
.SH "SEE ALSO"
prblock(1), qprintf(1), retrieve(1), and uprint(UNITY).

