.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH qprintf 1 "" "New UNITY"
.SA 1
.SH NAME
qprintf \- Format lines read from standard input according to a user-specified format.
.SH SYNOPSIS
.HP
.B qprintf
.RI [-bR]
.RI [-d c ]
.RI [-F n ]
.RI [-f <footer_string> ]
.br
.RI [-h <header_string> ]
.RI [-i <file> ]
.RI [-m <middle_string> ]
.RI [-n c ]
.RI [ <format> ]
.RI [ <field_names> ...]
.SH DESCRIPTION
The
.B qprintf
command generates strings on the output according to the
format string given on the command line.
For each line read from standard input,
.B qprintf
divides it into fields according to a field delimiter
and prints the fields on the standard output using
.I <format>
as the first argument to
.BR printf (3)
and the field values or \fI<field_names>\fR as the other arguments.
.P
Within the format string, all conversions start with a percent
sign (``%''), followed by optional justification,
width and precision information recognized by
.BR printf (3),
and finally a conversion character.
The following conversion characters are recognized in the format string:
.TP
\fBs\fR
Output a field value.
The field output is the \fIN\fRth field as this is the \fIN\fRth ``%s''
in the format string.
.TP
\fBd\fR, \fBo\fR, or \fBx\fR
Print the record number being printed.
.TP
\fBn\fR
Print the corresponding \fI<field_name>\fR given on the command line.
The field name output is the \fIN\fRth field as this is the \fIN\fRth ``%n''
in the format string.
This conversion is most useful when the format string
is applied repeatedly with the \fB-R\fR option.
Note, that unlike the other conversions, this conversion is not
a standard conversion supported by
.BR printf (3).
It is converted to a \fB%s\fR and the field name is supplied as an
argument.
.P
As an example, the format ``%-.20s'' would print a field
as a twenty-character left-justified value.
The format ``%d %n=%s\en'' would print
the record number, a space, the field name, an equal sign,
the field value, and a newline.
.P
Note that a newline is
.I not
output with each record unless included in the format string.
.P
All character constant escape sequences recognized in the C language
are recognized as part of the format string:
.P
.RS .5i
.PD 0
.TP 9
.B \en
newline
.TP 9
.B \et
horizontal tab
.TP 9
.B \eb
backspace
.TP 9
.B \er
carriage return
.TP 9
.B \ef
form feed
.TP 9
.B \e\e
backslash
.TP 9
.B \e"
double quote
.TP 9
.BI \e number
bit pattern
.PD
.RE
.P
The format string should be surrounded by
quotes if it contains white space or other characters recognized by the shell.
.P
If no
.I <format>
string is given, then each field value is printed, followed by a
new-line, i.e., a format of ``%s\en'' for each field is used.
Without the
.I <format>
argument, the
.B -F
option must be given to know how many fields are on each line.
Normally, if there are not as many ``%s'' constructs in
.I <format>
as the field count given with the
.B -F
option, then after the format string is printed,
each extra field value is printed followed by a new-line
i.e., one ``%s\en'' conversion is appended to
.I <format>
for each extra field.
This can be changed with the \fB-R\fR option.
.SH PARAMETERS
.TP
.B -b
Remove trailing blanks from field values before they are printed.
.TP
.BI -d c
The character
.RI `` c ''
is the field delimiter character for dividing up each input line.
The default delimiter is ``^G'' (Control-G).
.TP
.BI -F n
The number of fields in each record must be
.IR n .
The number
.I n
must also be greater than or equal to the number of ``%s''
conversions in the format string.
.IP
If no
.I <format>
string is given, this parameter is required.
.TP
.BI -f " <footer_string>"
The next argument is used as a string to be printed at the end
of the output, after all tuples have been printed.
The string may consist of any number of lines.
Standard C escape conventions (e.g. ``\en'') are recognized and translated
to the corresponding value.
In this case the string should be quoted to avoid backslash interpretation
by the shell.
.TP
.BI -h " <header_string>"
The next argument is used as a header to be printed at the beginning
of the output, before any tuples are printed.
The string may consist of any number of lines.
Standard C escape conventions (e.g. ``\en'') are recognized and translated
to the corresponding value.
In this case the string should be quoted to avoid backslash interpretation
by the shell.
.TP
.BI -i <file>
The data to be formatted is taken from the given
.I file.
Without this option, the data is taken from standard input.
.TP
.BI -m " <middle_string>"
The next argument is used as a string to be printed
in between each tuple of output.
The string may consist of any number of lines.
Standard C escape conventions (e.g. ``\en'') are recognized and translated
to the corresponding value.
In this case the string should be quoted to avoid backslash interpretation
by the shell.
.TP
.BI -n c
All occurrences of the specified character
.RI `` c ''
are translated into a a newline during printing.
Thus, the user can explicitly control where breaks occur in
field values.
The default newline indicator character is tilde (\~).
.TP
\fB-R\fR
Use the \fI<format>\fR string repeatedly as needed for all the
fields.
This option is most useful when the format describes how a single
field (or set of fields) should be printed.
If the number of fields is not an exact multiple of the
number %s conversion constructs in the format string,
the extra conversions are ignored, but the rest of the format
string is still printed.
This option is ignored if there is no \fI<format>\fR string
or the \fB-F\fR option is not given.
.SH EXIT CODES          \" strongly recommended
.so Exit.codes.h
.SH RESTRICTIONS        \" optional - includes caveats and bugs
There is no maximum record size limit, but each individual
attribute is limited to the size of an I/O block: 10K on mainframes,
6K on Vaxen and other mid-size machines, and 4K on other smaller machines.
.P
There is a limit on the number of fields.
Currently, this is 300 on mainframes and 100 on all other machines.
.SH EXAMPLE
To print three fields (a supplier name, number, and location)
on each line:
.P
.RS .5i
.nf
\f(CWqprintf -d':' "Supplier %s, number %s, is located in %s.\en"\fR
.fi
.RE
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
using the above command, the output would be
.P
.RS .5i
.nf
\f(CWSupplier Smith, number 5398, is located in London.
Supplier Jones, number 769, is located in Nice.
Supplier Blake, number 245, is located in Paris.\fR
.fi
.RE
.P
To create C structure initialization code out of the same input
as above, the following command could be used:
.P
.RS .5i
.nf
\f(CWqprintf -d':' -h"struct xyz var[] = {\en" -m",\en" -f"\en};\en"	\e
	"\et{ \e"%s\e", %s, \e"%s\e" }"
.fi
.RE
.P
Using the above command, the output would be
.P
.RS .5i
.nf
\f(CWstruct xyz var[] = {
	{ "Smith", 5398, "London" },
	{ "Jones", 769, "Nice" },
	{ "Blake", 245, "Paris" }
};\fR
.fi
.RE
.P
Create shell-variable assignments out of the input for use in
a while-loop:
.P
.RS .5i
.nf
\f(CWqprintf -d':' -R -F3 "%n='%s'; " -m"\en" -f"\en" NAME ID LOC\fR
.fi
.RE
.P
Using the above command, the output would be
.P
.RS .5i
.nf
\f(CWNAME='Smith'; ID='5398'; LOC='London';
NAME='Jones'; ID='769'; LOC='Nice';
NAME='Blake'; ID='245'; LOC='Paris';\fR
.fi
.RE
.SH "SEE ALSO"
prblock(1), prtable(1), retrieve(1), and uprintf(UNITY).
