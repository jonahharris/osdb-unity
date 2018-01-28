.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH usort 1 "" "New UNITY"
.SA 1
.SH NAME
usort \- sort tuples in a UNITY table
.SH SYNOPSIS
.HP
.B usort
[-cnqsuEV] \e
.br
[-M {blanks|caseless}] [-Q [\fI<ErrorLimit>\fR]]\ \e
.br
[by \fI<attr>\fR:\fI<modifiers>\fR ...] in \fI<table>\fR[=\fI<alt_table>\fR]
.SH DESCRIPTION
.B usort
is a command for sorting tuples (records) in a UNITY table.
If the table to be sorted is standard input (-) then
the resulting output table is written to standard output.
Otherwise, the input table is updated with the
sorted output tuples.  If the unique (-u) option is given
then any duplicate tuples will be deleted from the table.
The -E option can be used to test if the table is already
sorted without modifying the input table.
.P
By default the table is lexically sorted by all attributes
in the order in which they appear in the table.
The "by" clause can be used to specify which attributes
and the order to be used for sorting.
The optional attribute modifiers can be used to change
the type of comparison (e.g., date, lexical, numeric, etc.)
as well as ascending or descending sort order for each
attribute.
The first attribute in the "by" clause is sorted first
(highest priority) followed by the next attribute
and so on.
.P
The special attributes ``\fIrec#\fR''
(the record number of the tuple in the table)
and ``\fIseek#\fR'' (the seek location of
the start of the tuple within the table)
can be used for sorting in addition to the
normal table attributes.
The special ``all'' attribute can be used
to indicate that all attributes, which
have not been specified previosly, should
also be sorted in the order in which they appear
in the table along with any modifiers given to
the special ``all'' attribute.
.P
Attributes can be sorted in more than one way (e.g.,
numeric and lexical) by including the attribute
more than once in the "by" clause.
It should be noted that uniqueness checks are
based on the sort comparison type(s).
If more than one type of comparison is specified
for a given attribute then uniqueness for that
attribute is based on each type of comparison specified.
.P
By default the descriptor file for a table is named \f(CWD\fI<table>\fR.
A descriptor file from another table can be specified by adding
``=\fI<alt_table>\fR'' after the table name.
The alternate descriptor file will be \f(CWD\fI<alt_table>\fR.
Note that if \fI<alt_table>\fR contains a path,
the given directory will be checked first for the alternate
descriptor file.
The \fIUNITYDFILES\fR and \fIUNITYDSEARCH\fR entries in the \fBENVIRONMENT\fR
section describe how the descriptor information for a table is located.
.SH PARAMETERS
.TP
.B -c
Create the description for the output at the beginning of the output itself.
This option is ignored when the table is not coming from standard input (-).
.TP
.B -E
Check if the table is already sorted without modifying the table.
An exit status of zero (0) is returned if the table is already sorted.
An exit status of four (4) is returned if the table is not already sorted.
.TP
.B -M { blanks | caseless }
Ignore blanks or character case, respectively, for all attributes
that are to be used for sorting as appropriate.
This is a short-hand way to avoid having to include the
"blanks" or "caseless" modifier with each individual attribute.
This modifer is not added to attributes which are not to be sorted
or are being sorted by a non-lexical type of comparison operator
(e.g., date, numeric, etc.).
.TP
.B -n
No-change mode.
This option is used to see what
.B usort
would do without making any updates to the table being sorted.
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
.B usort
prints the number of records sorted on standard error.
If this option is given, this message is suppressed.
.TP
.B -s
Sort the output.
This option is set by default.
.TP
.B -u
Only print unique tuples in the output.
Duplicate (identical) tuples are determined
by the type of sort comparison operator(s)
used for each individual attribute which
do not have to include a lexical
(character-by-character) comparison.
.TP
.B -V
Print the version of the command and exit.
No sorting is done.
.SH ATTRIBUTE MODIFIERS
.P
Attribute
.I <modifiers>
dictate special handling for the given attribute.
Any number of modifiers can be given for an attribute but
each must begin with a colon (``:'').
The modifiers can be separated by white space or
can immediately follow each other.
Most attribute modifiers can be abbreviated (truncated)
to the first few unique characters in the modifier keyword.
Below are the recognized attribute modifiers that impact sorting:
.RS .5i
.TP
.B :ascending
Sort the attribute's values in ascending order.
This is the default sort order.
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
.TP
.B :caseless
Use a character-by-character comparison on this attribute for sorting or uniqueness
but ignore the case of letters.
.TP
.B :date
Use a date comparison on this attribute for sorting or uniqueness.
.TP
.B :descending
Sort the attribute's values in descending order.
Normally, attribute values are sorted in ascending order.
.TP
.B :dictionary
Use a character-by-character comparison on this attribute
for sorting or uniqueness, but ignore characters
which are not letters, digits, or blanks (spaces or tabs).
The "caseless" modifier can be used with this option to
also ignore the case of letters.
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
.TP
.B :hexadecimal
Use \fIh\fPexadecimal (base 16) unsigned integer comparison on this attribute
for sorting or uniqueness.
This comparison type is not valid with
the special ``rec#'' and ``seek#'' attributes.
.TP
.B :numeric
Use a numeric comparison on this attribute for sorting or uniqueness.
This is the default (and only supported) comparison type for
the special ``rec#'' and ``seek#'' attributes.
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
.TP
.BI :split <string>
Use the given
.I <string>
as the delimiters for splitting the attribute into sub-fields
for sorting.
(See the
.B :field
modifier for more information on sub-fields.)
Sub-fields are
.I not
used when determining uniqueness.
Unlike most modifiers, this modifier
.I "must not"
be abbreviated.
Everything after ``:split'' is used as the
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
.SH ENVIRONMENT
.so Environ.u.h
.so Envunpack.u.h
.SH EXIT CODES          \" strongly recommended
.so Exit.codes.h
.SH RESTRICTIONS        \" optional - includes caveats and bugs
.so Restrict.h
.SH EXAMPLE
Sort password file by numeric user ID and login ID:
.PP
.RS
\(CWusort by uid:numeric login in /etc/passwd\fR
.RE
.SH FILES
.so Files.u.h
.br
.ne 3
.SH "SEE ALSO"
descriptor(4), retrieve(1), setunpackenv(3), and umerge(1).
