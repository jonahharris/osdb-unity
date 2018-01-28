.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH nalter 1 "" "New UNITY"
.SH NAME                \" required
nalter \- change attribute values in a UNITY relation
.SH SYNOPSIS            \" required    
.na
.HP
.B nalter
[-adiopqrV] [-Q [\fIErrorLimit\fP]]
.ad
.br
.nf
<\fIattr\fP> [:\fImod\fP] [%\fIformat\fP] [f]<\fIop\fP> <\fIvalue/attr\fP> [:\fImod\fP]
[\fIattr\fP ...] in <\fItable\fP>[=<\fIalt_table\fP>]
[with <\fItable\fP>[=<\fIalt_table\fP>] ...]
[where <\fIwhere-clause\fP>]
.fi
.SH DESCRIPTION         \" required - 1/2 page in length
The \fBnalter\fR command allows the selective modification of one or more
attributes in a UNITY table.
The attributes can be set to new values or, if numeric, can have simple
arithmetic performed on them.
.PP
By default, all records in the table are changed, but the use of a \fIwhere\fR
clause allows restricting the changes to a subset of the records.
The \fIwhere\fR clause allows checking the values of attributes in each
record.
If the \fIwith\fR parameter is given, attributes can also be matched against
records in other tables (a join).
.PP
In normal operation, \fBnalter\fR prints out a count of the altered records.
It will exit if a change is ambiguous.
The problem of ambiguous alterations arises when using the \fIf\fP-prefixed
alter operations and a \fIwhere-clase\fP that does a join.
If the record being modified joins with mutiple records with different
values in the attribute containing the new value,
.B nalter
does not know which single new value to use.
.PP
While alterations are taking place, the table is locked against other users 
changing it.
.PP
The \fBretrieve\fR command manual page provides a more detailed explanation of
\fIwhere\fR clauses and cross-table matching (joins).
.SH PARAMETERS          \" required - if alter takes parameters - alphabetical order
.TP
.B -a
Print the altered (new) values of the \fI<attrs>\fR on standard output,
one record per line.
When both \fI-a\fR and \fI-o\fR are specified, the old value is printed
first, and separated from the new value with an equal sign (=).
See the \fBEXTENDED DESCRIPTION\fR for more information.
.TP
.B -d
Demo or no-change mode.
Do not perform any alterations, but act as if they were being done.
This option is used to see how
.B nalter
would act without actually doing the change.
.TP
.BI -delim c
Use character \fIc\fR as the delimiter between all attribute
values when printing old or new (altered) records
(see the \fI-a\fP and \fI-o\fP options).
.TP
.B -i
Ignore the case of letters in
.I all
the string and regular expression comparisons of the where-clause.
This option is a short hand notation to pre-pending the
.RB `` c ''
prefix to all the comparison operators in the where-clause.
With this option, the string ``this'' would match ``This'' or ``THIS''.
The regular expression ``this'' would match ``This is a test''
or ``THIS IS A TEST''.
(There is no need for grouping with square brackets.)
This option is analogous to the -i option of
.BR grep (1).
.TP
.B -o
Print the old values of the \fI<attrs>\fR on standard output, one record
per line.
This is useful, for example, when a record has a field
that must be accessed and incremented.
When both \fI-a\fR and \fI-o\fR are specified, the old value is printed
first, and separated from the new value with an equal sign (=).
See the \fBEXTENDED DESCRIPTION\fR for more information.
.TP
.B -p
Preserve record order.
Normally, modified records appear first in the updated relation
followed by the unaltered records.
When this option is specified,
the records in the updated relation will appear in the same order
as the original relation.
This option can also be specified by setting the
\fBprerec#\fR keyword in the \fIUNITY_ALTER\fR environment variable
(see \fBENVIRONMENT\fR Section).
.TP
.B -Q
Gracefully continue after encountering any tuple syntax errors instead of quiting.
An error limit can be specified with this option (i.e., "-Q \fIErrorLimit\fP")
to limit the number of tuple syntax error (warning) messages that would otherwise
be printed when there are many tuples that (might) have syntax errors.
An error limit of minus one (-1) can be used to indicate that
a warning message for each tuple error is to be printed.
If no error limit or an error limit of zero (0) is given,
only a summary message with the total number of tuples with tuple errors
for all relations that were queried will be printed.
The tuple error summary message is not printed if there were no tuple errors or
if the -q (quiet mode) option was specified.
.TP
.B -q
Quiet mode.
Do not print a count of the number of records altered.
.TP
.B -r
For each record that is changed, print its record number on standard
output, one record per line.
If either \fI-a\fR or \fI-o\fR is also specified, the record number is
followed by a colon (:) to separate it from the attribute value(s).
See the \fBEXTENDED DESCRIPTION\fR for more information.
.TP
.B -V
Print the version of the command and exit.
No changes are made to the table.
.TP
\fI<attr1>\fR [:\fImod1\fR] [%\fIformat\fR] [\fBf\fR]\fI<op>\fR \fI<value/attr2> [:mod2]\fR
Replace the value of attribute \fI<attr1>\fR using alter
operation \fI<op>\fR.
If \fI<op>\fR is not preceded by the optional \fBf\fR,
then \fI<value/attr2\fP> is taken to be a constant \fI<value>\fR;
if the \fBf\fR is present, then \fI<value/attr2\fP> is taken to
be an attribute name, \fI<attr2>\fR, whose value is to be used.
If \fI<attr2>\fR is not unique among the tables listed on the
command line, it must take the form of
\f(CWtable-name.attribute-name\fR.
.IP
The attribute/operation modifiers, :\fImod1\fP and :\fImod2\fP,
can be used to modify the normal operation of a given alter operator.
The modifier for the second attribute (:\fImod2\fP) is only allowed/valid
when \fI<value/attr2>\fP is an attribute.
.IP
%\fIformat\fP is an optional output format specification that is
passed to sprintf(3C) when printing the new value of the attribute
that has been modified.
The default format for string type alter operations is "%s" and
"%.11g" is the default format for numeric type alter operations.
.IP
See the \fBEXTENDED DESCRIPTION\fR for a list of the alter operations,
attribute/operation modifiers, and output format specifications.
.TP
.BI in " <table1>\fR[\fP" = <alt_table>\fR]\fP
The table whose values are to be altered.
.IP
By default, the descriptor file for the table is named \f(CWD\fI<table1>\fR.
A descriptor file from another table can be specified by adding
``=\fI<alt_table>\fR'' after the table name.
The alternate descriptor file will be \f(CWD\fI<alt_table>\fR.
.TP
.BI with " <table2>\fR[\fP" = <alt_table>\fR]...\fP
Extra table(s) containing records that are matched against
in the \fIwhere\fR clause.
A table named ``\-'' refers to standard input.
The standard input can only be used once.
.IP
If this option is used, attribute names appearing in more than one table
must be prefixed with the table name and a period (\f(CWtable.attribute\fR)
or the relation's positional number and a period (\f(CW2.attribute\fR).
See the
.BR retrieve (1)
manual page for more information on attribute name syntax.
.IP
By default, the descriptor file for the table is named \f(CWD\fI<table2>\fR.
A descriptor file from another table can be specified by adding
``=\fI<alt_table>\fR'' after the table name.
The alternate descriptor file will be \f(CWD\fI<alt_table>\fR.
.TP
.BI "where " <where-clause>
.so Where.u.h
.SH ENVIRONMENT         \" optional
.so Environ.u.h
.so Envunpack.u.h
.TP 10
UNITY_ALTER
This variable can be set to a comma separated list of keyword=value pairs
for some of the \fBnalter\fR command line parameters.
Command line parameter options always take precedence over 
any keyword=value pairs that are set.
The following keywords and values are recognized:
.ne 5
.IP
.nf
.ft CW
|-----------------------------------------------------------|
| KEYWORD |VALUE|DEFAULT|         DESCRIPTION               |
|-----------------------------------------------------------|
| prerec# | y/n |   n   | Preserve record order (-p option) |
|-----------------------------------------------------------|
.ft R
.fi
.IP
For example,
setting UNITY_ALTER="prerec#=y" is the same as using the "-p"
parameter option to preserve record order which is the way
alter(UNITY) works.
.SH EXIT CODES          \" strongly recommended
.so Exit.codes.h
.SH RESTRICTIONS        \" optional - includes caveats and bugs
.so Restrict.h
.SH EXAMPLES             \" strongly recommended
Alter the supervisor and department for a given owner in an ownership
data base:
.PP
.RS
.ft CW
nalter supv = 'J. Doe' dept = 12345 in owninfo \e
.br
    where login leq own1
.ft R
.RE
.PP
Increment a sequence number for a given prefix string, and retrieve
the old value:
.PP
.RS
.ft CW
.nf
OLDNUM=`nalter -o seqno += 1 in sequence \e
.br
        where string leq SSlmr`
.fi
.ft R
.RE
.PP
Look up owners in the \fI/etc/passwd\fR file and update their department
number if it is null in the \fIowninfo\fR relation.
(This does a join between the owninfo and passwd, and copies a field from
one to the other).
.PP
.RS
.ft CW
.nf
nalter owninfo.login f= /etc/passwd.login	\e
    in ownerinfo with /etc/passwd		\e
    where owninfo.dept = "" and			\e
          owninfo.login f= /etc/passwd.login
.fi
.ft R
.RE
.SH EXTENDED DESCRIPTION    \" strongly recommended
.SS Displaying Attribute Values
The \fI-a\fR, \fI-o\fR, and \fI-r\fR options can result in some or all of 
the following information begin printed:
.RS
\fI<record_number>\fR:\fI<old_value>\fR=\fI<new_value>...\fR
.RE
This format is chosen so that the output can be treated as a UNITY
relation if desired.
.PP
The colon after \fI<record_number>\fR is only printed if \fI-r\fR and
\fI-a\fR or \fI-o\fR is specified.
The equal sign after \fI<old_value>\fRis only printed if \fI-a\fR and
\fI-o\fR are both specified.
.PP
If more than one attribute is being altered, each attribute
(\fI<old_value>\fR, \fI<new_value>\fR, or
\fI<old_value>\fR=\fI<new_value>\fR), is displayed as it is in the
relation.
That is, it is terminated with the attribute terminator, or displayed as 
fixed width.
.PP
Occurrences of terminators in a value are escaped with a backslash.
(This includes the terminating equal sign when \fI-a\fR and \fI-o\fR are
both selected.)
.SS Alter Operations
In the following descriptions, the \fIold value\fR is the value of the
attribute on the left of the alter operation.
If the alter operation is prefixed with \fBf\fR, 
the \fInew value\fR is the value of the attribute on the right of the alter
operation; otherwise, the \fInew value\fR is the value on the right of
the alter operation.
.TP 5
.B =
Replace the old value with the new value.
To be compatible with \fBalter\fR(UNITY), the word ``\fBto\fR''
may also be used.
.TP
.B +=
Add the new value to the old value.
Both new and old values will be taken as numbers; if they are non-numeric,
they will be treated as zero.
.TP
.B \-=
Subtract the new value from the old value.
Both new and old values will be taken as numbers; if they are non-numeric,
they will be treated as zero.
.TP
.B *=
Multiply the old value by the new value.
Both new and old values will be taken as numbers; if they are non-numeric,
they will be treated as zero.
.TP
.B /=
Divide the old value by the new value.
Both new and old values will be taken as numbers; if they are non-numeric,
they will be treated as zero.
Divide by zero will cause the operation to be aborted.
.TP
.B %=
Modulo (remainder of dividing) the old value by the new value.
Both new and old values will be taken as integer numbers; if they are non-numeric,
they will be treated as zero.
Divide by zero will cause the operation to be aborted.
.SS Alter Attribute/Operation Modifiers
In the following descriptions,
the source attribute modifier refers to the modifier associated with the
right operand of one of the field (f-prefixed) operators and the
destination attribute modifier refers to the modifier associated with the
left operand or the attribute that is being modified.
In the case of an assignment operation ("=" or "f="),
the left operand modifier is treated as an operation modifier
instead of an attribute modifier since there is no need to
consider the data type of the old attribute value that is being modified.
.TP 8
\fInu\fPmeric
The operation is considered to be a numeric assignment when used with the
"=" or "f=" operators.
This applies to its use on either the left or right side of the alter operator.
With the "f<+|-|/|*|%>=" type operators, the associated source or destination
attribute values are treated as base 10 input numbers,
which is the default for all numeric operations.
The atof(3C) function is used to convert numeric (base 10) input values
to double precious floating point numbers before being used in a given
alter operation.
.TP
\fIbi\fPnary
The assiciated source or destination attribute values are treated
as base 2 input numbers.
The strtoul(3C) function is used to convert a base 2 input string
to unsigned integer and type-cast back to an integer before
being used in a given alter operation.
If it is used on the left of a non-field type operator, or if no
source attribute modifier is used with the "f=" operator,
then the source attribute or value is treated as base 2 input.
.TP
\fIo\fPctal
The assiciated source or destination attribute values are treated
as base 8 input numbers.
The strtoul(3C) function is used to convert a base 8 input string
to unsigned integer and type-cast back to an integer before
being used in a given alter operation.
If it is used on the left of a non-field type operator, or if no
source attribute modifier is used with the "f=" operator,
then the source attribute or value is treated as base 8 input.
.TP
\fIh\fPex
The assiciated source or destination attribute values are treated
as base 16 input numbers.
The strtoul(3C) function is used to convert a base 16 input string
to unsigned integer and type-cast back to an integer before
being used in a given alter operation.
If it is used on the left of a non-field type operator, or if no
source attribute modifier is used with the "f=" operator,
then the source attribute or value is treated as base 16 input.
.TP
\fIlen\fPgth
The string length of the source attribute value is used in the
given alter operation.  This modifier is not allowed to be used
as a destination attribute modifier with the "f<+|-|*|/|%>="
operators since the left operand is already considered to be
a number and not a string.
If used as an operation modifier on the left the "f=" operator,
the source attribute modifier must be null (not specified) or
a type of string (i.e., string, trim, ltrim, or rtrim).
.TP
\fIltr\fPim
Leading (left) blanks (' ' characters) are removed
from the attribute/input string value being assigned.
This modifier is not allowed to be used with the "[f]<+|-|*|/|%>="
operators since the both operands are considered/required
to be some type of number.
.TP
\fIrtr\fPim
Trailing (right) blanks (' ' characters) are removed
from the attribute/input string value being assigned.
This modifier is not allowed to be used with the "[f]<+|-|*|/|%>="
operators since the both operands are considered/required
to be some type of number.
.TP
\fIstr\fPing
The operation is considered to be a string assignment, which is the default,
when used with the "=" or "f=" operators.
This applies to its use on either the left or right side of the alter operator.
This modifier is not allowed to be used with the "[f]<+|-|*|/|%>="
operators since the both operands are considered/required to be some type of number.
.TP
\fItol\fPower
Upper case characters are converted to lower case
in the attribute value being assigned.
This modifier is not allowed to be used with the "[f]<+|-|*|/|%>="
operators since the both operands are considered/required
to be some type of number.
If using the field assignment (f=) operator,
the [l|r]trim modifier can be used as the other
(source) attribute modifier to trim leading/trailing
blanks and convert all characters to lower case.
.TP
\fItou\fPpper
Lower case characters are converted to upper case
in the attribute value being assigned.
This modifier is not allowed to be used with the "[f]<+|-|*|/|%>="
operators since the both operands are considered/required
to be some type of number.
If using the field assignment (f=) operator,
the [l|r]trim modifier can be used as the other
(source) attribute modifier to trim leading/trailing
blanks and convert all characters to upper case.
.TP
\fItr\fPim
Leading (left) and trailing (right) blanks (' ' characters) are removed
from the attribute/input string value being assigned.
This modifier is not allowed to be used with the "[f]<+|-|*|/|%>="
operators since the both operands are considered/required
to be some type of number.
.SS Alter Output Format Specification
The format specification starts with the percent (``%'')
format conversion character followed by an optional justification
character, optional field width and precision, and ends with the
type of conversion character.  The following regular expression
defines the \fI%format\fP specifications that are allowed:
.IP
.nf
.ce
[+ -#]*[0-9]*\.{0,1}[0-9]*[bdiouxXeEfgGs]
.fi
.P
The 'b' conversion character is used for unsigned binary output
that is performed by a special low level UNITY library function,
sprintfb(NewUNITY),
that works similar to the way sprintf(3C) works for the 'x' (hexadecimal)
or 'o' (octal) format conversion characters.
.SH FILES               \" optional 
.so Files.u.h
.br
.ne 4
.SH SEE ALSO            \" strongly recommended
alter(UNITY), ndelete(1), ninsert(1), nprintf(UNITY),
nuedit(1), retrieve(1), setunpackenv(3), ucheck(UNITY), and uedit(UNITY).
