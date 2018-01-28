.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH "prblock" 1 "" "New UNITY"
.SA 1
.SH NAME
prblock \- Format input lines vertically in a block.
.SH SYNOPSIS
.HP
.B prblock
[-abepP]
.RI [-d c ]
.RI [-D c ]
.RI [-F n ]
.RI [-h <pg_header> ]
.RI [-f <pg_footer> ]
.RI [-i <file> ]
.RI [-L n ]
.RI [-m n ]
.RI [-n c ]
.RI [-o n ]
.RI [-W n ]
.IR  <field_names> ...
.fi
.SH DESCRIPTION
For each input line, the
.B prblock
command prints a block structured layout of the fields in the line.
For each field in the line,
the field name is printed in the left margin and then the field
value is printed.
.P
.B prblock
gracefully handles field names and
values that wrap onto more than
one line.
By default, up to 34 characters are printed for the field name
in the left margin;
space characters fill up to column 35;
and up to 44 bytes of the field value make up the rest of the line.
Continuing field names begin at column 1, while
continuing field values begin at column 35.
.P
If a field is too wide for one line, it is broken
according to the following rules:
.RS .5i
.IP 1. .25i
at the last white-space character that will fit on the line, if any;
.IP 2. .25i
else at the last punctuation or
non-alphanumeric character that will fit on the line, if any;
.IP 3. .25i
else at the width of the line.
.RE
.P
If a value is broken across lines,
blanks that would appear at the beginning of the next line are not printed.
.P
By default, two lines of hyphens (``\-'') are printed between each record.
.SH PARAMETERS          \" required - if command takes parameters - alphabetical order
.TP
.B -a
Automatically determine how wide the left margin for field names should be,
based on the length of the longest field name.
The margine is set to allow a minimum of two spaces between each field
name and its field value.
The space allowed for field values is also adjusted to fit within
the specified page width.
.IP
This option overrides the
.B -m
option.
.TP
.B -B
Suppress the printing of blank fields.
If a field value is null,
the line \- which would consist only of the field name \- is
not printed.
.TP
.BI -d c
The character
.RI `` c ''
is the field delimiter character for dividing up each input line.
The default delimiter is a colon (``:'').
.TP
.BI -D c
Double space the output.
Normally, a blank line is printed between each field value.
However, if the character
.RI `` c ''
is given, a line consisting of only
.IR c 's
for the entire page width will be printed.
.TP
.B -e[n]
Enumerate the records.
If the
.B -P
option is specified, then a separate line is printed at
the beginning of each record in the form
.P
.ce
\f(CW--- Record \fI<record number>\fP ---\fR
.IP
If the
.B -P
option is not specified, then the record number is
printed on the second of the dashed separator lines.
If ``n'' is specified then record numbering
starts with ``n'' instead of ``1''.
.TP
.BI -F n
The number of fields in each record
.I must
be
.IR n .
The number
.I n
must also be greater than or equal to the number of field names.
If
.I n
is greater than the number of field names,
the extra fields in the input are never printed.
.TP
.BI -f <pg_footer>
The next argument is used as a footer to be printed at the bottom
of each page.
The bottom of the page output will consist of
two blank lines, followed by the footer line,
and followed by two more blank lines.
The page footer
may consist of multiple lines, but it is the
user's responsibility to account for the right number of lines appearing
on the page.
For example, if the footer consists of two lines, the user should
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
This option implies the
.B -p
option.
.TP
.BI -h <pg_header>
The next argument is used as a header to be printed at the top
of each page.
Four blank lines will be printed at the top of the page, followed
by the header line, followed by two blank lines, and the data.
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
This option implies the
.B -p
option.
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
This option implies the
.B -p
option.
.TP
.BI -m n
The width of the left field-name margin is set to 
.I n
characters.
By default, the left margin, in which the field names are printed,
is 35 characters.  This means that up to 34 characters of field
name are printed and spaces are used to fill up to column 35.
Field values are printed in columns 36 up to the width of the
page (default 79 characters).
The print width for field values is equal to the page width
minus the margin width.
Thus, increasing the margin size without increasing the page width
will cause the print width for field values to decrease.
If
.I n
is "-" or a negative number,
then any leading white space (blanks) in the field names
is deleted so that the field name is left-justifed.
If
.I n
is simply "-" then the current (default) left margin width is not changed.
.IP
This option is ignored if the
.B -a
option is given (except for the left-justify operation, if requested).
.TP
.BI -n c
This option causes all occurrences of
.RI `` c ''
to be translated into a skip to the next line.
Thus, the user can explicitly control where breaks occur in
field values even if the value would not fill the line.
The default for 
.I c
is tilde (``\~'').
.IP
The
.B -n
option takes priority over normal line splitting.
The command will first scan for a newline character.
Failing to find one,
it scans backwards trying to find a break character
.I only
if the entire field value will not fit on the current line.
Failing to find a break character, it splits
the field value exactly at the width of the line.
The
.B -n
option has an effect even if the field value will fit on
the line, while normal line splitting
takes effect only on multi-line field values.
.TP
.BI -o n
The page offset is set to
.I n
characters.
By default, the page offset is zero (0).
If
.I n
is a negative number,
then '-' characters will be used in place of space characters
to fill the "offset" region of the two default lines of hyphens
(if/when printed).
If
.I n
is simply "-" then the two default lines of hyphens that normally
print between records will not be printed.
Note that this option extends the total output line length so
the user may also want/need to use the page width (-w) option to
shorten the page width if they do not want the total output line
length (limit) to change.
.TP
.B -P
Paginate the output and place each record on a new page.
The dashed separator lines are suppressed.
This option overrides the
.B -p
option.
.TP
.B -p
Paginate the output.
When paginating, records are not printed across
pages whenever possible.
The number of lines needed to print a record is
calculated before printing begins and if the record will not fit
on the current page, printing will start on a new page.
The only time that a record will print across more than one page is
if it will not fit on a single page.
.TP
.BI -W n
The width of lines is set to 
.I n
characters.
By default, the print width is 79 characters per line; the
default was chosen so that the output will
print nicely on video terminals.
If
.I n
is not given, it defaults to 120 characters per line.
The minimum width is 10 characters.
This width also controls the width of the separator lines.
.IP
If the page width is less than the left margin for field names,
the page width is divided equally between field names and values.
.SH ENVIRONMENT
.TP 10
UNITY_PRB
This variable can be set to a comma separated list of keyword=value pairs
to alter the default output format.
The following keywords and values are recognized:
.ne 8
.IP
.nf
.ft CW
|-----------------------------------------------------------|
| KEYWORD| VALUE |      DEFAULT      |     DESCRIPTION      |
|-----------------------------------------------------------|
| alignlm|  y/n  |        n          | Align left margin    |
|        |       |                   |(same as "-m-" option)|
| enumfmt| string|"--- Record %d ---"| Format of enumerated |
|        |       |                   | record numbers.      |
| enumgap|  0-2  |        1          | Blank lines to print |
|        |       |                   | after "enumfmt".     |
|-----------------------------------------------------------|
.ft R
.fi
.P
For example,
setting UNITY_PRB="alignlm=y,enumgap=0,enumfmt=--- %d ---"
would remove leading whitspace (blanks) from friendly-user
(i.e., verbose) attribute names and change the default
enumerated record number format ("--- Record %d ---")
to "--- %d ---", and remove
the blank line that follows when either
the "-P" or "-o-" option is specified.
.SH EXIT CODES          \" strongly recommended
.so Exit.codes.h
.SH RESTRICTIONS        \" optional - includes caveats and bugs
There is no maximum record size limit, but each individual
attribute is limited to the size of an I/O block: 10K on mainframes,
6K on Vaxen and other mid-size machines, and 4K on other smaller machines.
.P
There is a limit on the number of columns that can be printed.
Currently, this is 300 on mainframes and 100 on all other machines.
.SH EXAMPLES
To print three fields (a supplier name, number, and location)
on each line:
.IP
\f(CWprblock -W50 -d':' "Supplier Name" "Supplier Number" Location\fP
.P
Given the input
.P
.RS .5i
.nf
\f(CWSmith:5398:London
Jones:769:Nice
Blake:245:Paris\fP
.fi
.RE
.P
using the above command, the output would be:
.P
.RS .5i
.nf
\f(CW--------------------------------------------------
--------------------------------------------------

Supplier Name                      Smith
Supplier Number                    5398
Location                           London

--------------------------------------------------
--------------------------------------------------

Supplier Name                      Jones
Supplier Number                    769
Location                           Nice

--------------------------------------------------
--------------------------------------------------

Supplier Name                      Blake
Supplier Number                    245
Location                           Paris\fP
.fi
.RE
.SH "SEE ALSO"
catalog(UNITY), prtable(1), qprintf(1), and retrieve(1).
