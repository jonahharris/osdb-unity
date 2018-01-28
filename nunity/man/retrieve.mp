.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH retrieve 1 "" "New UNITY"
.SA 1
.SH NAME
retrieve \- retrieve information from UNITY tables
.SH SYNOPSIS
.HP
.B retrieve
[-cEiqsuvV]
.RI [-d " c]"
.br
.RI "[-Q" " [<ErrorLimit>]]" " [-r" "<RecordLimit>]"
.br
[-A\fB|\fP-B\fB|\fP-F\fI<format>\fP|\fP-T\fB]
.RI [ <print_options> ]
.br
[\fI<attribute>\fR[:\fI<modifiers>\fR]... [as \fI<newattr>\fR] ...]
[sorted [by \fI<attr>\fR:\fI<modifiers>\fR ...]] [unique]
.br
from
.IR <table> [ =<alt_table> ]...
.br
.RB [into | onto
.IR <new_table> [ =<alt_table> ]]
.br
[where
.IR <where_clause> ]
.SH DESCRIPTION
.B retrieve
is a generalized query command for retrieving
information from a collection of UNITY tables.
It querys information from the given tables by a process of
projections, selections, and/or joins on the tables.
.P
.B retrieve
differs from the other UNITY commands (such as
.BR uselect (1),
.BR uprint (1),
and
.BR uprintf "(1) )"
in several ways.
First, and foremost,
.B retrieve
can query information from several (up to eight) different
tables at once, rather than just a single table.
Any joins are done as part of the query, without the need of
intermediate files.
Secondly, the formatting of the selected information is handled
separately from the query itself.
.B retrieve
makes use of several formatter commands
.RB ( prtable (1),
.BR prblock (1),
and
.BR qprintf "(1) )"
to format the selected information into a form dictated on the
.B retrieve
command line.
By default no formatting is done;
all information is printed as it appears in
the original tables.
.P
The command line for
.B retrieve
can be divided into four parts:
(1) option flags which specify how to format the retrieved information,
(2) the attributes to retrieve from the various tables
including specifications for sorting or uniqueness,
(3) the tables on which the query is performed,
and
(4) the where-clause, which stipulates what records
should be selected from the given tables.
These are all described below; see the \fBEXTENDED DESCRIPTION\fR
section for a detailed explanation of the where-clause.
.SH PARAMETERS
Besides selecting arbitrary records and attributes from the tables,
.B retrieve
also provides facilities for formatting the retrieved information.
These facilities are provided by piping the retrieved information
to a back-end formatting program.
There are currently four different output formats, indicated by command-line
flags.
Flags for the individual formatting command are also accepted on
the command line.
.TP
.BI -A
Print the output unformatted, as it exists in the individual tables.
The exception to this is if an attribute has a delimiter
and is the last attribute in a table, but not the
last in the attribute list for
.BR retrieve ,
then it's terminator (normally a newline)
is converted to a tab.
(See the
.B -d
option to override this replacement.)
.TP
.B -B
Print each record in the output as a block,
with a label to the left of each
attribute value.
(This formatting is done by the
.BR prblock (1)
command.)
.TP
.BI -F format
Print the output according to the format string
.IR format .
The format string is like that used by
.BR printf (3),
except that only ``%s'' is supported for attribute values.
(This formatting is done by the
.BR qprintf (1)
command.)
.TP
.B -T
Print the output as a table with column headings.
(This formatting is done by the
.BR prtable (1)
command.)
.TP
.I <print_options>
Flags recognized by the individual formatting commands are
acceptable on the
.B retrieve
command line and are passed on to the appropriate formatting command.
More details on each formatting command are available by consulting their
individual manual pages.
(See
.BR prblock (1),
.BR prtable (1),
and
.BR qprintf (1).)
.IP
If no particular formatter is indicated on the command line, but there
are print flags given for the formatter, the table
.RB ( -T )
format is used.
On the other hand, if no formatter is indicated
and no other print flags are given either, then
the output is printed unformatted
.RB ( -A ).
.P
Below are the miscellaneous command line flags recognized directly by
.BR retrieve :
.TP
.B -c
Create the description for the output at the beginning of the output itself.
The description is consistent with normal descriptions contained in
the data table, as described in the
.BR descriptor (4)
manual page.
This option applies whether an
.IR into -clause
is given or the output is just sent to stdout.
This option is ignored, however, if the output is formated.
A major use of this option is for piping the output of one
.B retrieve
command to another New Unity command, such as
.BR nalter (1).
.br
.ne 3
.TP
.BI -d c [!]\ [-d o ]
Use the character \fIc\fR as the delimiter between all attribute
values in the output or in place of the old delimiter character(s) \fIo\fR.
To change fixed width attributes to be terminated by the character \fIc\fR,
the (second) -d option can be used without the \fIo\fR character(s).
Any number of -d\fIo\fR options can be given and/or a string of \fIo\fR characters
given.  Any standard C backslash escape sequences will be converted to
their character equivalents (i.e., "\e0" == '\e0').
If an old delimiter character \fIo\fR is specified and the new delimeter
character \fIc\fR is followed by the '!' (not) character,
then the character \fIc\fR will be used as the new delimiter character
where \fIo\fR is not the current delimiter (or fixed width attribute).
.TP
.B -E
Check if a tuple matching the given where-clause and projection
criteria exists,
but don't print it.
If such a tuple combination does not exist, \fBretrieve\fR will exit with
an unsuccessful status.
All options involving sorting, uniqueness, and formatting are
ignored with this option.
.IP
Beware of using this option without any explicitly projected attributes,
especially when using set difference in the where-clause.
(In this case, a cross product of all tuples in
all relations is projected.)
Without any projected attributes, an unsuccessful exit code may
result (when one relation has no matching tuples),
while with a projected attribute a successful exit code would result.
While this may seem confusing, the command is operating correctly.
.IP
This option is the same as projecting a single null attribute,
described below.
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
.BI -Q " [<ErrorLimit>]"
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
Operate in quiet mode.
Normally,
.B retrieve
prints the number of records retrieved on standard error.
If this option is given, this message is suppressed.
.TP
.BI -r " <RecordLimit>"
Limit the number of records retrieved to \fIRecordLimit\fP.
If \fIRecordLimit\fP is zero (0) then all records that are
queried will be output (printed) which is the same as not
requesting this option.
.TP
.B -s
Sort the output, but don't remove duplicate lines.
Normally, the output is not sorted and there may be multiple,
identical entries.
The sort order is normally left to right and is done
in ascending order using a character-by-character comparison.
(See the
.B "sort by"
and attribute
.I <modifiers>
sections below for how to modify the sort criteria.)
.TP
.B -u
Print only unique entries in the output.
Normally, there may be multiple, identical entries in the output.
Identical entries are records
where all the
.I displayed
attribute values are the same.
Normally, a character-by-character
comparison is used.
(See the attribute
.I <modifiers>
section below for how to modify the comparison type.)
.IP
Note:  The output is not necessarily sorted with this option.
If both sorting and uniqueness are desired, use both \fB-s\fR and \fB-u\fR.
.TP
.B -v
Use the verbose or ``user-friendly'' names
(if available) from the descriptor file for
labeling attributes in the formatted output.
Normally, the attribute names are used for labels.
(See the
.BR table (UNITY)
or
.BR descriptor (4)
manual page for more details on the user-friendly names.)
This option implies that the output should be formatted.
.TP
.B -V
Print the version of the command and exit.
No query is done.
.P
Below are the command-line clauses which describe the query
executed by
.BR retrieve .
The clauses must all follow the command-line flags described above.
The clauses must also be specified in the order listed
in the usage statement or a syntax error will result.
.TP
\fI<attribute>\fR[\fB:\fI<modifiers>\fR...] [\fBas \fI<new_attr>\fR] ...
Each named \fI<attribute>\fR is printed in the output
(i.e., a projection),
in the given order.
If no attributes are specified, all attributes
without an explicit zero print width
in the descriptor files from all the tables
will appear in the output in the order of the
original tables.
.IP
The special attribute ``\fIall\fR'' refers to all attributes in a table
when "all" is not a normal attribute in the table or it has been qualified
as ``all:nodisplay=''.
This attribute cannot be used in the where-clause, however.
The special attribute ``\fIrec#\fR'' refers to the record number within
a table.
The special attribute ``\fIseek#\fR'' refers to the seek location of
the start of the tuple within its table.
The
.I rec#
and
.I seek#
attributes can be used in the where-clause or as a projected attribute.
The
.I all
attribute can only be used as a projected attribute.
.IP
If a single null attribute is given (i.e., ""),
no output will be generated.
If no tuple is found from each relation matching
the where-clause,
.B retrieve
will exit with an unsuccessful (non-zero) exit status.
All options involving sorting, uniqueness, and formatting are
ignored when the null attribute is given.
The null attribute is the same as using \fB-E\fR option, described above,
except that all attributes from all relations are assumed to be projected.
With the \fB-E\fR option, there is explicit control of which
attributes are projected.
.IP
The syntax for specifying attribute names differs slightly from other
UNITY commands.
Whenever an attribute name is not unique among the queried tables,
the attribute name must be prefixed either with the table's file name
(\fInot the path name\fR) and a period (``.''),
or a number representing the position of the table
among all tables on the command line
and a period.
For example, if the tables used in the query were, in order,
.IR dir/class ,
.IR dir/owner ,
and
.IR dir/owninfo ,
then an attribute
.I login
from the
.I dir/owner
table could be referenced as
.RS .5i
.IP
\f(CW2.login\fR
.RE
.IP
or, equivalently,
.RS .5i
.IP
\f(CWowner.login\fR
.RE
.IP
These two forms can be arbitrarily intermixed on the command line.
However, the second form has the advantage that the attribute references are
independent of the order the tables appear on the command line.
Whenever multiple tables with the same filename are used in
a query, the first form must be used.
.IP
If an attribute name is unique among all queried tables,
then no prefix is needed on the attribute (although it is still allowed).
The syntax is then identical to other previous UNITY commands.
.IP
\fINote that this same syntax for the attribute names
also applies to attribute references within the where-clause.\fP
.IP
The optional "\fIas\fP" feature allows attributes to be renamed
in the output or in any created descriptor file.
For example, ``\f(CWretrieve aname1 as aname2 ...\fR''
will rename the ``aname1'' attribute to ``aname2''
in any formatted output
or in any descriptor file created for a new table.
.IP
The attribute
.I <modifiers>
dictate special handling for the given attribute,
such as a column print width and sorting criteria.
Any number of modifiers can be given for an attribute;
each must begin with a colon (``:'').
The modifiers can be separated by white space or
can immediately follow each other.
Below are the recognized attribute modifiers:
.RS .5i
.TP
.B :ascending
Sort the attribute's values in ascending order.
This is the default sort order.
This option implies that the output will be sorted.
.TP
.B :binary
Use binary (base 2) unsigned integer comparison on this attribute
for sorting or uniqueness.
This comparison type is not valid with
the special ``rec#'' and ``seek#'' attributes.
.TP
.B :blanks
Remove all white space from the beginning of the attribute
and any sub-fields before doing the sort or determining uniqueness.
If uniqueness is not specified on the command line,
this option implies that the output will be sorted.
.P
.PD 0
.TP
.BI : <prwidth>
.TP
.BI :c <prwidth>
.TP
.BI :l <prwidth>
.TP
.BI :r <prwidth>
.PD
Print the attribute in a column
.I <prwidth>
characters wide.
Also optionally force the
attribute to be left justified ('l'), right justified ('r'),
or centered ('c') in the column.
If an upper case justification character (C, L, or R) is given
the attribute is treated like an attribute with <prwidth> set to zero (0) \(em
the attribute is not projected (displayed) unless explicitly specified
or specified by the special ``all'' attribute.
Otherwise, this modifier is only used for table formatted output.
If an attribute value is wider than the print width, it will wrap around
onto the next line.
(See the \fBprtable\fR(1) command for more information on table formatting.)
.IP
The justification and \fI<prwidth>\fR override any print width
and justification given for the attribute in the descriptor file
for the corresponding table.
(See the
.BR table (UNITY)
or
.BR descriptor (4)
manual pages.)
If no justification is given anywhere, the attribute will be left
justified.
If no print width is given anywhere, and the attribute is a terminated
attribute, the column will be 12 characters wide.
If the attribute is a fixed width attribute, the column will be as
many characters wide as the attribute is.
.TP
.B :caseless
Use a character-by-character comparison on this attribute for sorting or uniqueness,
but ignore the case of letters.
If uniqueness is not specified on the command line,
this option implies that the output will be sorted.
.TP
.B :date
Use a date comparison on this attribute for sorting or uniqueness.
If uniqueness is not specified on the command line,
this option implies that the output will be sorted.
.TP
.BI :delim <char>
If the output is unformatted,
use the given
.RI < char >
as the delimiter for this attribute.
If the output is formatted (i.e., the
.BR -T ,
.BR -B ,
or
.BR -F
options were given) this modifier is ignored.
The normal C escape sequences are allowed in
.RI < char >,
such as ``\en''.
If all attributes should have the same delimiter, the
.B -d
option can be used as a short-hand notation.
Unlike most modifiers, this modifier
.I "must not"
be abbreviated; everything after ``:delim'' is used to get
the attribute delimiter.
.TP
.B :descending
Sort the attribute's values in descending order.
Normally, attribute values are sorted in ascending order.
This option implies that the output will be sorted.
.TP
.B :dictionary
Use a character-by-character comparison on this attribute
for sorting or uniqueness, but ignore characters
which are not letters, digits, or blanks (spaces or tabs).
The "caseless" modifier can be used with this option to
also ignore the case of letters.
If uniqueness is not specified on the command line,
this option implies that the output will be sorted.
.TP
.BI :field <field#>\fR[.\fP<char#>\fR][\fP - <field#>\fR[.\fP<char#>\fR],...\fP
Use only the given sub-fields and/or character
positions in the attribute for sorting.
This option implies that records will be sorted;
sub-fields are not used when determining uniqueness.
Up to four sub-field ranges may be specified.
Any additional ranges are ignored.
Sub-field and character positions start at 1.
The ending field and character positions are inclusive.
The sub-fields are determined by the split string given
with the
.B :split
modifier.
The default split string is a comma.
.IP
If different sub-fields need different types of comparisons
(e.g., a number appended to a string),
the attribute and sub-fields can be listed multiple times,
each with the different comparison specification.
All the attributes but one should have the
.B :nodisplay
modifier.
.TP
.B :hexadecimal
Use \fIh\fPexadecimal (base 16) unsigned integer comparison on this attribute
for sorting or uniqueness.
This comparison type is not valid with
the special ``rec#'' and ``seek#'' attributes.
.TP
.B :nodisplay
Do not display this attribute in the output.
This option is useful when the sorted attribute is not displayed
(e.g., the ``rec#'' attribute),
or when an attribute's sub-fields need different types of comparisons.
Note, however, that this modifier does
.I not
imply sorting.
Furthermore, uniqueness does not apply to non-displayed attributes.
If used with the special ``all'' attribute, then a comma separated
list of attribute names can be given as "nodisplay=attr1[,attr2,...]"
so that all attributes in the table are displayed except for the
given list of attributes.
In this case, nodisplay cannot be abreviated
and must be followed by the equal ("=") sign and a valid list of
attributes.
Otherwise, all attributes (when expanded) will be marked as nodisplay.
.TP
.B :noverbose
Do not copy verbose (friendly) attribute name from the input
attribute's relation descriptor when creating new output descriptor.
.TP
.B :numeric
Use a numeric comparison on this attribute for sorting or uniqueness.
This is the default (and only supported) comparison type for
the special ``rec#'' and ``seek#'' attributes.
.ig
.TP
.BI :sort <priority>
Sort on this attribute.
Obviously, this option implies the output will be sorted.
The optional
.I <priority>
is an integer value indicating what order the attribute
should be used in the sort, relative to other attributes.
The lower the integer, the higher priority.
The highest priority is 1.
If no priority is given, the lowest possible priority is assigned to
the attribute.
When sorting records, attributes are compared in priority order.
If two attributes have the same priority, the
one listed first on the command line will be used first.
.IP
Only those attributes which have this modifier
(or some other modifier which implies sorting, e.g.,
.BR :ascending ,
.BR :binary ,
.BR :caseless ,
.BR :date ,
.BR :descending ,
.BR :field ,
.BR :hex ,
.BR :nodisplay ,
.BR :numeric ,
.BR :octal ,
.BR :split ,
or
.BR :string ),
will be used in the sorting the tuples.
The order of non-sorted attributes is completely arbitrary.
Additionally, the order of tuples in the relation is not preserved.
(To preserve the order of tuples, sort on the ``rec#'' attribute.)
If the
.B -s
or
.B sorted
options are given, but no sort modifier is used, all retrieved attributes
will participate in the sort.
..
.TP
.B :octal
Use octal (base 8) unsigned integer comparison on this attribute
for sorting or uniqueness.
This comparison type is not valid with
the special ``rec#'' and ``seek#'' attributes.
.TP
.B :printable
Use a character-by-character comparison on this attribute
for sorting or uniqueness, but ignore characters
which are not printable (i.e., outside the ASCII range of 040-0176).
The "caseless" modifier can be used with this option to
also ignore the case of letters.
If uniqueness is not specified on the command line,
this option implies that the output will be sorted.
.TP
.BI :split <string>
Use the given
.I <string>
as the delimiters for splitting the attribute into sub-fields
for sorting.
(See the
.B :field
modifier for more information on sub-fields.)
This option implies that records will be sorted;
sub-fields are
.I not
used when determining uniqueness.
Unlike most modifiers, this modifier
.I "must not"
be abbreviated; everything after ``:split'' is used as the
split string.
The default split string is ``,''.
Each character in the split string is used to locate
the end of the respective sub-field in the attribute.
The last character in the split string is repeated as needed.
Thus, if five fields are separated by commas, ``:split,'' and ``:split,,,,''
are equivalent specifications.
If the first field were separated by a forward slash
and the rest by commas, the modifier would be ``:split/,''.
If a colon (``:'') is needed as one of the split characters,
it must be preceded by a backslash (``\e'') in order to not be
interpreted as the start of a new modifier.
Normal C-like escape sequences are also recognized in the
split string.
.TP
.B :string
Use a character-by-character
comparison on this attribute for sorting or uniqueness.
This is the default comparison type except the
special ``rec#'' and ``seek#'' attributes, which are always numeric.
If uniqueness is not specified on the command line,
this option implies that the output will be sorted.
.TP
.B :verbose=<\fIfriendly\fP>
Use <friendly> for the friendly (verbose) attribute name
when creating new output descriptor.
The value of <friendly> is terminated by the null character,
so any additional attribute modifiers must be given as
subsequent command line arguments, and cannot include
a new line character.
No backslash escape processing is performed on the value
given as <friendly>.
.TP
.B :value=<\fIattrvalue\fP>
Print <attrvalue> as the projected attribute value
in place of the original attribute value from the query.
This modifier provides a very basic type of alter
operation (e.g., fixed value assignment) for projected
attributes.
Standard C style backslash escape character processing
is performed on the value given as <attrvalue>.
The value of <attrvalue> cannot include a new line character
and is terminated by the null character.
Therefore, any additional attribute modifiers must be given as
subsequent command line arguments.
It should be noted that no sorting or uniqueness checks
are performed on attributes with the "value=" modifier.
It should also be noted that, because of internal memory 
constraints, the "value=" and "verbose=" modifiers are
mutually exclusive and cannot be used with
the special ``all'' attribute.
.TP
.B :width<\fIn\fP>
If the output is unformatted, make the given attribute
a fixed width field of length \fIn\fR.
Unlike most attribute modifiers,
this modifier \fIcannot\fR be abbreviated
and must be followed by a valid number (digit string).
.RE
.TP
[\fBsorted\fR [\fBby\fR \fI<attr>\fR:\fI<modifiers>\fR ...]]
The retrieved records should be sorted on the given attributes.
Normally, the output is not sorted and there may be multiple,
identical entries.
If no sort attributes are given, the sort order is
left to right and is done
in ascending order using a character-by-character comparison.
The attribute
.I <modifiers>
can be used to modify the sort criteria.
(See the attribute
.I <modifiers>
section above for more details.)
This option is the same as the
.B -s
option.
It is provided as an alternate syntax for use
when more detail is needed for the sort attributes or
using the flag is not possible or inappropriate, e.g., in makefiles.
.TP
.RB [ unique ]
Print only unique entries in the output.
Normally, multiple, identical entries may be retrieved.
Identical entries are records
where all the
.I displayed
attribute values are the same.
Normally, a character-by-character
comparison is used.
(See the attribute
.I <modifiers>
section above for how to modify the comparison type.)
This option is the same as the
.B -u
option.
It is provided as an alternate syntax for use
when using the flag is not possible or inappropriate, e.g., in makefiles.
.IP
Note:  The output is not necessarily sorted with this option.
If both sorting and uniqueness are desired, use both \fBsorted\fR
and \fBunique\fR.
.TP
\fBfrom\fR \fI<table>\fR[\fB=\fI<alt_table>\fR]...
The list of tables needed in the query.
The tables should be given as full or partial path names.
A table named ``\-'' refers to standard input; this can only appear once.
The keyword
.B from
is only needed if projected attributes are specified.
.IP
By default, the descriptor file for a table is named \f(CWD\fI<table>\fR.
A descriptor file from another table can be specified by adding
``=\fI<alt_table>\fR'' after the table name.
The alternate descriptor file will be \f(CWD\fI<alt_table>\fR.
Note that if \fI<alt_table>\fR contains a path,
the given directory will be checked first for the alternate
descriptor file.
.IP
The \fIUNITYDFILES\fR and \fIUNITYDSEARCH\fR entries in the \fBENVIRONMENT\fR
section describe how the descriptor information for a table is located.
.TP
\fBinto\fR|\fPonto \fI<new_table>\fR[\fB=<alt_table>\fR]...
Normally, all output from
.B retrieve
is written to the standard output.
If an \fBinto\fR clause is given, the named file is created
(which must not previously exist).
If \fBonto\fR is used instead of \fBinto\fR,
the output is appended to the specified file.
The file will still be created if it does not exist.
.IP
If the output is unformatted, \fI<new_table>\fR will be a UNITY table.
If a descriptor cannot be located,
it will also be created in the current directory.
By default, the descriptor file for the table is named \f(CWD\fI<new_table>\fR.
A descriptor file from another table can be specified by adding
``=\fI<alt_table>\fR'' after the table name.
The alternate descriptor file will be \f(CWD\fI<alt_table>\fR.
Note that if \fI<alt_table>\fR contains a path,
the given directory will be checked first for the alternate
descriptor file.
.IP
The \fIUNITYDFILES\fR and \fIUNITYDSEARCH\fR entries in the \fBENVIRONMENT\fR
section describe how the descriptor information for a table is located.
.TP
.BI where " <where_clause>"
A set of comparisons which control which records will be selected.
If omitted, a cross product of all records in all tables is selected.
See the \fBEXTENDED DESCRIPTION\fR section for details on the syntax
of the where-clause.
.SH ENVIRONMENT
.so Environ.u.h
.so Envunpack.u.h
.SH EXIT CODES          \" strongly recommended
.so Exit.codes.h
.TP
4
No tuples were found when the -E flag was specified.
.TP
5
Formatting command (prtable, prblock, qprintf) failed.
.SH RESTRICTIONS        \" optional - includes caveats and bugs
.so Restrict.h
.P
The attribute limit for individual tables applies also
to the the total number of attributes printed out,
whether the output is formatted or not.
Any attributes beyond this limit are ignored.
.SH EXAMPLES
The following examples are based on three tables, described below with
their description files.
.P
The
.I class
table groups files together; owners then own the classes, not the
individual files:
.P
.RS .5i
.nf
\f(CWdir	t:	32l	File's Directory Path
file	t:	14l	File Name
class	t\en	32l	Grouping File Belongs To\fP
.fi
.RE
.P
The
.I owner
table lists the owners of each class:
.P
.RS .5i
.nf
\f(CWclass	t:	32l	Grouping File Belongs To
ownid	t\en	6r	Owner's ID Number\fP
.fi
.RE
.P
The
.I owninfo
table gives the information about each owner:
.P
.RS .5i
.nf
\f(CWownid	t:	6r	Owner's ID Number
mach	t:	10l	Machine Where Owner Resides
login	t:	8l	Owner's Login on Machine
name	t:	32l	Owner's Name
supv	t:	32l	Owner's Supervisor
dept	t\en	10l	Owner's Department\fP
.fi
.RE
.P
Retrieve the name, supervisor and machine for all occurrences of
the login
.RI `` jqpublic ''.
Print the information out unformatted.
The records should be sorted by name, supervisor, and record number.
The record number should not be displayed, however.
.P
.RS .5i
.nf
\f(CWretrieve name supv mach		\e
	sorted by name supv rec#	\e
	from owninfo			\e
	where login leq jqpublic\fP
.fi
.RE
.P
Retrieve the owners' name, machine, and login for the file
.I MGblock.L
in the directory
.IR pc/hdr .
Print the information out as a table.
Use algebraic comparison symbols instead of the two-character
nmeumonics:
.P
.RS .5i
.nf
\f(CWretrieve -T owninfo.name owninfo.mach owninfo.login	\e
	from class owner owninfo					\e
	where class.dir = pc/hdr and				\e
		class.file = MGblock.L and			\e
		class.class f= owner.class and		\e
		owner.ownid fn= owninfo.ownid\fP
.fi
.RE
.P
Continuing with the previous example,
suppose there can be owners in
our data base for individual files or there can be ``default'' owners
for all files in a directory, i.e., for any file not explicitly
listed in the data base.
(In this case the
.I file
attribute would be ``\f(CW%default\fR''.)
Under these assumptions we need to use an
.BR else -operator
to get the proper information from the data base.
The previous query is repeated below to handle these new assumptions.
The changes are shown in italic:
.P
.RS .5i
.nf
\f(CWretrieve -T owninfo.name owninfo.mach owninfo.login	\e
	from class owner owninfo					\e
	where \e( class.dir leq pc/hdr and			\e
		class.file leq MGblock.L \e) \fIelse\fP		\e
		\fI\e( class.dir leq pc/hdr and\fP			\e
		\fIclass.file leq %default \e)\fP and		\e
		class.class fleq owner.class and		\e
		owner.ownid feq owninfo.ownid\fR
.fi
.RE
.P
Retrieve information about files that are owned
by people in J. Q. Public's group,
i.e. files whose owner's supervisor resembles
``J. Q. Public''.
Print the information out in the form
.IP
\f(CW\fI<directory>\fP/\fI<file>\fP is owned by \fI<owner's name>\fP\en\fR
.P
The command for this is:
.P
.RS .5i
.nf
\f(CWretrieve -F"%s/%s is owned by %s\en"			\e
		 class.dir class.file owninfo.name		\e
	from class owner owninfo					\e
	where owninfo.supv req "J.* Q.* Public" and	\e
		class.class fleq owner.class and		\e
		owner.ownid feq owninfo.ownid\fP
.fi
.RE
.P
Note that a regular-expression comparison is used to
catch the various versions of the supervisor name that might exist.
.P
Retrieve information about the files \fIMGblock1.L\fR, \fIMGblk2.L\fR,
and \fIMGblk3.L\fR from the directory \fIpc/hdr\fR.
Use the set membership operator to specify the comparisons on
the file name:
.P
.RS .5i
.nf
\f(CWretrieve -T owninfo.name owninfo.mach owninfo.login		\e
	from class owner owninfo					\e
	where class.dir = pc/hdr and					\e
		class.file in MGblock1.L,MGblk2.L,MGblk3.L and	\e
		class.class f= owner.class and				\e
		owner.ownid fn= owninfo.ownid\fP
.fi
.RE
.SH EXTENDED DESCRIPTION
.SS "The Where-Clause"
The where-clause specifies a subset of
the records in the tables based on comparisons of the values of certain
attributes in the tables.
The comparisons can also be combined with boolean operators and
grouped arbitrarily with parentheses.
.P
Each comparison in the where-clause takes one of two forms.
It either compares an attribute to a constant value:
.P
.nf
	\fI<attribute> <cmp_type><cmp_oper> <value>\fR
.fi
.P
or compares two attributes:
.P
.nf
	\fI<attribute>\fR \fBf\fR[\fBo\fR]\fI<cmp_type><cmp_oper>\fR \fI<attribute>\fR
.fi
.P
The \fI<cmp_oper>\fR tells what kind of comparison operation should be done.
It can be expressed as a two-character mnemonic or as an algebraic symbol.
The \fI<cmp_type>\fR tells the data type of the comparison operands,
e.g., numbers, strings, or regular expressions.
If there is no \fI<cmp_type>\fR and a two-character mnemonic symbol
is used for \fI<cmp_oper>\fR, a numeric comparison will be done.
If there is no \fI<cmp_type>\fR and an algebraic symbol
is used for \fI<cmp_oper>\fR, a normal string comparison will be done.
.P
Attributes used in numeric comparisons can include a numeric attribute
modifier (<attriubute>:<modifier>) to indicate an alternate input base
is to be used to converting the input string to a number for comparison.
The numeric attribute modifiers include \fIb\fPinary (base 2),
\fIo\fPctal (base 8), \fIh\fPexadecimal (base 16),
\fIn\fPumeric (base 10 \(em the default),
and \fIlen\fPgth.
The "length" modifier uses strlen(3C) to get the string length of
the attribute value which is then used in the numeric comparison.
Attribute modifiers cannot be used with the set (in) operator \(em
only the six basic numeric operators (two-character mnemonics)
which are discussed below.
In addition, "length" cannot be used as a projected attribute modifier
outside of the where-clause.
.P
The two-character mnemonics for \fI<cmp_oper>\fR
and their meanings are shown below:
.P
.ne 6
.PD 0
.RS .5i
.TP
.B eq
equal to
.TP
.B ne
not equal to
.TP
.B lt
less than
.TP
.B le
less than or equal to
.TP
.B ge
greater than or equal to
.TP
.B gt
greater than
.RE
.PD
.P
The algebraic symbols for \fI<cmp_oper>\fR
and their meanings are shown below:
.P
.PD 0
.RS .5i
.TP
.B =
equal to
.TP
.B !=
not equal to
.TP
.B <
less than
.TP
.B <=
less than or equal to
.TP
.B >=
greater than or equal to
.TP
.B >
greater than
.RE
.PD
.P
NOTE:  When using these symbols, the '<' and '>' characters should
be quoted (using a backslash '\e', or double or single quotes)
to avoid interpretation by the shell.
.P
The comparison operator, \fI<cmp_oper>\fR may be prefixed
with another character, the \fI<cmp_type>\fR,
to specialize the comparison as follows:
.TP
.B n
Do a numerical comparison.
Both values are converted to double float values
and compared.
Thus, values like ``1.0'' and ``1'', which would fail an equality test
based on a character-by-character comparison, would match using
a numerical equality comparison (i.e.,
.RI `` eq '').
.IP
This is the default comparison type when a two-character mnemonic is used
to indicate the comparison operation.
.TP
.B l
Do a lexical, or character-by-character, comparison.
The two values are compared as character strings
using the standard ASCII character set.
Thus, ``1.0'' would be greater than ``1''; ``tim''
would be less than ``time'';
and ``test'' would equal ``test''.
.IP
This is the default comparison type when an algebraic symbol is used
to indicate the comparison operation.
.TP
.B c
Do a character-by-character comparison, but ignore
the case of letters.
The two values are compared as character strings
using the standard ASCII character set,
except that upper- and lower-case characters are considered equivalent.
Thus, ``1.0'' would be greater than ``1''; ``tim''
would be less than ``time'';
and ``test'' would equal ``TEST''.
This option may also follow the regular-expression
comparison (i.e., ``rc'') to do caseless regular expression matching,
without the need for groupings with square brackets.
.TP
.B r
Do a regular-expression comparison.
(This type of comparison can only be used with the
\fIeq\fR, \fI=\fR, \fIne\fR, and \fI!=\fR
comparison operators.)
The second value is taken as a regular expression against
which to compare the first value.
The
.BR regex (3)
regular-expression matching routines are used to
compile regular expressions and make comparisons.
The manual page for
.BR regex (3)
gives, in detail, the
regular expression syntax that is used.
(It is an enhanced version of that used by
.BR ed (1),
and
.BR grep (1).)
Regular expressions should normally be surrounded
by single quotes to avoid expansion by the shell.
.IP
This option may also be followed by the caseless
comparison (i.e., ``rc'') to do caseless regular expression matching,
without the need for groupings with square brackets.
.IP
As a shorthand notation, ``~'' is equivalent to ``req''
and ``!~'' is equivalent to ``rne'' as comparison operators.
When using
.BR ksh (1),
the tilda (~) should be quoted to avoid expansion by the shell.
.IP
Regular expression comparisons cannot be used in field-to-field
comparisons.
.TP
.B d
Do a date and time comparison.
Date comparisons treat the values as dates in one of the following forms:
.P
.ne 5
.nf
.in +1.0i
<month>/<day>/<year> [<hour>:<min>[:<sec>]]
<month>-<day>-<year> [<hour>:<min>[:<sec>]]
<day>.<month>.<year> [<hour>.<min>[.<sec>]]
YYYYMMDD[HHMMSS]
YYMMDD[HHMMSS]
.in -1.0i
.fi
.IP
where
.I month
is between 1 and 12 inclusive;
.I day
is between 1 and 31
inclusive, and
.I year
is between 0 and 9999 inclusive.
Note that the year can be in either 2-digit or 4-digit format.
A year that is only two digits (i.e., does not include the
century digits) will be based on the current year minus 60 years
as the start of a 100 year "window" that is used to calculate and
add the missing century digits when doing the date comparison.
Note that numbers less than 10 may appear as either two digit or
single digit numbers with or without leading white space when
they are separated by a forware spash (/), hyphen (-), or dot (.).
.IP
The time-of-day is optional, but if included, must follow the
conventions of a 24-hour clock.
If the time-of-day is omitted, it is considered to be midnight, ``00:00:00''.
Unlike the date (month/day/year) fields, no white space can appear
in the hour, minute, or second fields.
If both the date  and the time-of-day are in packed computer form
then a space or tab character does not need to appear between
the date and the time-of-day (i.e., YYMMDDHHMMSS or YYYYMMDDHHMMSS).
The seconds are optional for time-of-day when not
expressed in packed computer form.
.TP
.B D
Do a date comparison ignoring the optional time-of-day.
This comparison is the same as the regular date and time
comparison except that time-of-day is always treated
as midnight (``00:00:00'') regardless what value
was given, if any,
.P
The 'f' before \fI<cmp_type>\fR indicates a ``field-to-field''
(i.e., attribute-to-attribute) comparison.
The operand following \fI<cmp_oper>\fR is expected to be an attribute name
rather than a constant value.
A field-to-field, regular-expression comparison (i.e, 
.RB `` freq ''
or
.RB `` frne '')
is
.I not
allowed.
This prefix is primarily used to stipulate
conditions of a join between two tables.
However, the attributes can also be from the same table.
.P
If ``fo'' is used in a field-to-field comparison, an outer join
will be done between the given attributes, instead of a normal join.
In an outer join, any record
in one table which does not join with any record in the other table
is joined with the ``null tuple''.
The null tuple is a tuple in which all attributes values are empty strings.
.P
If ``fO'' (capital-O) is used in a field-to-field comparison, an outer
join will also be done between the attributes; however, only tuples
from the first attribute's relation which don't join with a tuple in
the second relation will be joined with the ``null tuple''.
\fINon-joining tuples from the second attribute's relation are
treated like a normal-join.\fR
.P
There is also a set-membership operator ``in''.
It is used to match a list of attributes with any one of a set of values.
The general form for this operator is
.P
.nf
	<\fIattA\fR>[\fB,\fR<\fIattB\fR>...] <\fIcmptype\fR>\fBin	\e
		<\fIval1A\fR>[\fB,\fR<\fIval1B\fR>...]\fB,\fR<\fIval2A\fR>[\fB,\fR<\fIval2B\fR>...]
.fi
.P
This operator is a short hand notation for
.P
.nf
	\fB( \fR<\fIattA\fR> \fI<cmptype>\fB=\fR <\fIval1A\fR> [\fBand \fR<\fIattB\fR> \fI<cmptype>\fB=\fR <\fIval1B\fR> ...] \fB) or	\e
	( \fR<\fIattA\fR> \fI<cmptype>\fB=\fR <\fIval2A\fR> [\fBand\fR <\fIattB\fR> \fI<cmptype>\fB=\fR <\fIval2B\fR> ...] \fB)\fR ...
.fi
.P
Unless a space or tab is part of the comparison value itself,
it should not be embedded in the comparison values.
(For example, no space should be placed after the commas.)
Note also that any un-escaped spaces or tabs will cause
a syntax error.
.P
The following values are recognized for <\fIcmptype\fP> to
dictate the type of comparison used.
Each type of comparison works as described above for the
normal comparison operators:
.TP
.B l
Do a string comparison.
This is the default if no \fI<cmptype>\fR is specified.
.TP
.B c
Do a string comparison, but ignore the case of letters.
.TP
.B d
Do a date and time comparison.
.TP
.B D
Do a date comparison ignoring any time-of-day that may have been included.
.TP
.B n
Do a numeric comparison.
.TP
.B r
Do a regular expresion comparison.
.P
The ``f'' prefix (field-to-field comparison) is not allowed
with regular expression comparisons.
.P
When multiple attributes are used with the set-membership operator,
the comparison values may be grouped together by enclosing them
in parenthesis.
If there are less values in the parenthesis than there are attributes listed,
the remaining comparison values are assumed to be
zero for numeric comparisons and
empty strings for string, regular expression, and date comparisons.
If more values are given in the parenthesis than attributes, the
extra values are assumed to be part of a new comparison combination.
.P
To embed a comma or parenthesis in a comparison value,
precede it with a backslash (``\e'').
.P
The comparison and set-membership operators above can be combined with
boolean operators, selection operators and/or parenthesis
to form a more complex where-clause:
.TP
.B !
.I "Boolean negation."
This operator reverses the logic of the clause to the immediate right.
.TP
.B and
.I "Boolean conjunction."
The clause on the left and the clause on the right of this operator
must both be true in order for records to be selected.
.TP
.B or
.I "Boolean disjunction."
Either the clause on the left or the clause on the right of this operator
must be true in order for records to be selected.
.TP
``\fB\-\fP'' \fIor\fP ``\fBminus\fP''
.I "Set difference operator."
Select those records which do
.I not
match the clause to the immediate right.
In field-to-field comparsisons across different tables (i.e., joins), this
operator indicates to select those records which do
.I not
join with any record in the other table,
according to the given comparison condition.
For selection operations this operator is equivalent to
boolean negation.
.IP
As an abbreviation, a minus sign can be given after the
initial ``f'' in a field-to-field comparison to indicate set difference.
For example, the clause
.IP
.nf
		- ( 1.login f= 2.login )
.IP
is equivalent to
.IP
		1.login f-= 2.login
.fi
.IP
Note that unlike most other operators, set-difference is
evaluation-order dependent.
For example, in the expression
.IP
.nf
	1.name req Smith and 1.login f-= 2.login
.fi
.IP
what records are output depends on
whether the set difference or the
regular expression is evaluated first.
Within the New Unity query evaluator,
all comparisons involving a single relation are
evaluated before any comparisons across relations.
So, in the above example, the regular expression would
be evaluated first, followed by the set difference.
If a different operation ordering is needed (e.g., a set difference
and then a single field comparison), the only recourse is
to use two
.B retrieve
commands and pipe the output of one into the other using the
.B -c
option.
With regard to comparisons across relations, normal joins are
done first, followed by outer joins, followed by set differences.
.IP
Currently, the set difference operator is allowed only on
individual comparisons, not on arbitrary expressions.
This is due to a bug in the current implementation on how
set difference on arbitrary expressions is handled.
(It almost always produces the wrong result.)
When the bug is fixed, this restriction will be lifted.
If you want the set difference of an expression, you must
manually distribute the operator to all the individual comparisons.
.TP 
``\fB(\fP'' \fIand\fP ``\fB)\fP''
.I "Grouping operators."
The parenthesis are used to group parts of the where-clause
together to dictate the order of evaluation.
They are used primarily to override the normal precedence of the
the boolean operations.
(See the discussion of precedence below.)
Each parenthesis must appear as a separate argument on the command line.
They should also be escaped to avoid interpretation by the shell.
.TP
.B else
.I "Selection-precedence operator."
The clause to the left is first used to match records,
but if none are found, the clause to the right
is used to match records.
It is analogous to an ``ordered OR''
operation (also known as a ``short-circuited OR''),
where the first expression, evaluating left to right, that
is true causes an exit from the evaluation.
This operator is used to apply a precedence to the possible
values of an attribute.
It can also be used to provide the notion of a default value for
an attribute.
See the EXAMPLES section for a sample use of this operator.
.IP
It is important to note that the negation of an else-operator
yields an and-operator.
Double negation of an else-operator yields an or-operator.
.IP
Currently, there are a few restrictions for the else-operator:
.RS .5i
.IP 1. .25i
The left and right clauses must not contain field-to-field
comparisons from different tables.
.IP 2. .25i
The tables referenced in the left and right clauses
must be the same.
.RE
.IP
Unlike most other operators, the
.B else
operator is execution-order dependent.
For example, in the expression
.IP
.nf
	login = sam and name = Sam else name = SAM
.fi
.IP
what records are output will depend on whether the match on
.I login
is done first, or whether the
.BR else- clause
is done first.
Within New Unity's query evaluator, all
.RB non- else
comparisons are done before the
.BR else -clause.
To specify that the
.I login
match should be done after the
.BR else -clause,
it should be included directly in each side of the
.BR else -clause:
.IP
.nf
	\e( login = sam and name = Sam \e) \e
	else \e( login = sam and name = SAM \e)
.fi
.IP
Alternately, multiple
.B retrieve
commands can be used where the output of one
is piped into the other using the
.B -c
option.
.P
The operator precedence for the where-clause,
from highest to lowest, is:
the comparison operators,
the
.BR else
operator,
the negation operator (``!'')
and set-difference operator (``-''),
the
.BR and
operator,
and finally,
the
.BR or
operator.
Parentheses may be used to group conditions to
override precedence.
.SS Miscellaneous
The processing time of the
.B retrieve
command can be decreased
using an index created by the
.BR index (UNITY)
command on any or all attributes used in the where-clause.
Note, however, that indexes can only be used for normal,
character-to-character (lexical) comparisons.
Indexes are ignored for all other types of comparisons.
.SH FILES
.so Files.u.h
.br
.ne 6
.SH "SEE ALSO"
catalog(UNITY), cmpquery(1), descriptor(4), fmkquery(3), setunpackenv(3),
index(UNITY), nprintf(UNITY), prtable(1), prblock(1), qprintf(1), table(UNITY),
ucheck(UNITY), umerge(1), uprint(UNITY), uprintf(UNITY), uselect(UNITY), and usort(1).
