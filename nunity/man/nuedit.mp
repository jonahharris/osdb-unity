.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH nuedit 1 "" "New UNITY"
.SA 1
.SH NAME
nuedit \- edit records in a UNITY table
.SH SYNOPSIS
.nf
\fBnuedit\fR [-b\fI<blank_cnt>\fP] [-cdfiqvV] \e
        [-Q \fI[ErrorLimit]\fP] [-n\fI<newline_char>\fP] \e
        \fI<table>[=<alt_table>]\fP [with \fI<table>[=<alt_table>]\fP...] \e
        [where \fI<where-clause>\fP]
.fi
.SH DESCRIPTION
The \fBnuedit\fP command provides a mechanism
for editing specified records in a given UNITY table.
The table is locked for update during the entire duration
of the command so that other users cannot change it.
.P
The only mandatory argument is the name of the table on which changes
are to be made.
The standard input ("-") \fIcannot\fR be used for this table.
The \fIwhere\fP-clause is used to specify a subset of records
to edit.
It allows checking the values of attributes in each record against
constants and, if the \fIwith\fR parameter is given,
allows checking the values of attributes
against attributes of records in other tables (using a join).
(See the \fBretrieve\fR(1) manual page for more details on the
\fIwhere\fR-clause and cross-table matching.)
\fBnuedit\fP queries the specified table(s) and transforms records
satisfying the \fIwhere\fP-clause into ``packet'' format for editing
in an editor.
Those records that do not match the \fIwhere\fR-clause
will be left unchanged.
If no \fIwhere\fP-clause is specified, all the records in the given
table are converted to packet format for editing.
.P
In the packet format, each attribute's name and value are
put on a line separated by a tab.
If the \fB-u\fR option is given, then the ``user-friendly'' names
are used instead of the attribute names, if they exist in
the description file.
(See the \fBdescriptor\fP(4) manual page.)
If an attribute value contains a \fI<newline_char>\fR
(see the \fB-n\fR option below),
it will be replaced by a newline and a tab.
.P
Each record's packet is preceded by a line of the form:
.IP
#####<tab><packet_number>
.P
where \fI<packet_number>\fR is the sequence number of the packet
within the edited file.
This line is used only to mark the start of each record;
this line and the \fI<packet_number>\fR are ignored when converting
the edited file back to record format.
Blank lines are also ignored.
.P
After the matching records are put into the packet format,
the user is put into their editor to
make any necessary changes in the data.
The editor used is determined first by the ED environment
variable.
If this is not set or is empty, the EDITOR environment variable is
looked at.
If this is not set or is empty, then ``/bin/ed'' is used.
Standard editor commands should be used to do the editing.
When changes are complete the user should exit
the editor in the standard fashion.
.P
After exiting the editor, the data is converted back to
normal record format.
Records are checked so they do not corrupt the UNITY table.
Further validation may be done use the \fB-v\fR option.
.P
If an error occurs in the conversion, the user is notified and asked if
they would like to re-enter the editor to correct the problems.
If the user responds with 'y',
they are put back into the editor.
Otherwise, \fBnuedit\fR will notify the user where the packet file
is located and exit without making any changes.
If the packet-to-record conversion succeeds,
the user is prompted whether to make the changes to the table or not.
If the user responds with 'y', then
the updated records are
put back in the table and the original records are removed.
This last question is not asked if the \fB-f\fR or \fB-d\fR
options are given.
.SH PARAMETERS          \" required - if delete takes parameters - alphabetical order
.TP
\fB-b\fI<count>\fR
Insert \fI<count>\fR blank records in the edited file.
If \fI<count>\fR is not given, it is defaulted to 1.
This option is useful as an alternate mechanism for inserting
records into the table.
.IP
Note that blank records are \fInot\fR removed
from the edited file during packet-to-record conversion.
The user must remove any un-wanted blank records while in the editor.
.TP
.B -c
Check the new tuples after exiting the editor.
The \fBvalidate\fP(UNITY)
command is used to check the updated tuples.
If an error occurs on any record, 
the user will be prompted whether to re-enter the editor to
make the corrections, as if packet-to-record conversion had failed.
.IP
The V\fI<table>\fR and E\fI<table>\fR files should be present
before this option is used.
Otherwise, the \fBvalidate\fR command will always fail.
.IP
The \fBvalidate\fR command, like all other normal UNITY commands,
does not (yet) support reading the
description for a table from the table itself and not in a separate
descriptor file.
If the edited table has its description in the table itself,
a warning message will be printed and this option will be ignored.
.TP
.B -d
Demo or no-change mode.
Do not perform any updates, but act as if they were being done.
This option is used to see what
.B nuedit
would update, without making any updates.
.TP
.B -f
Force the updates to be done.
Once packet-to-record conversion has completed successfully,
the changes are made to the table without prompting the user
for the okay.
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
\fB-n\fI<newline_char>\fR
When generating the packet file for editing,
each time \fI<newline_char>\fR character is encountered in
an attribute value, it will be replaced by a newline
and a tab.
For example, if \fI<newline_char>\fR is '~' and an attribute ``junk''
had the value
.nf
.IP
	Value~for Junk
.fi
.IP
it would be shown in the packet file as
.nf
.IP
	junk	Value
		for Junk
.fi
.IP
The reverse action is taken when converting the packets back to records:
Any lines having tab as the first character are appended to the
previous attribute's value, minus the tab.
.IP
If this option is not given,
the default \fI<newline_char>\fR is tilde ('~').
If \fB-n\fR is given with no argument,
then the newline character is assumed to be null and
the newline conversion is effectively turned off.
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
Do not print a count of the converted records.
.TP
.B -v
Use the verbose or ``user-friendly'' names from the descriptor file
to label each attribute in the packet file.
If an attribute's user-friendly name is not present in
the descriptor file, the normal attribute name is used instead.
.TP
.B -V
Print the version of the command and exit.
No updates are done.
.TP
<\fItable1\fR>[\fB=\fR<\fIalt_table\fR>]
The table records are to be updated in.
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
.BI where " <where-clause>"
.so Where.u.h
.LE
.SH ENVIRONMENT         \" optional
.so Environ.u.h
.so Envunpack.u.h
.TP
ED, EDITOR
Editor used to edit the packet file.
First $ED is looked at; if it is not defined or empty $EDITOR is looked at.
If both variables are not defined or empty, /bin/ed is
used as the editor.
.SH EXIT CODES          \" strongly recommended
.so Exit.codes.h
.SH RESTRICTIONS        \" optional - includes caveats and bugs
.so Restrict.h
.P
There are no terminal restrictions from \fBnuedit\fR
other than those imposed by the particular editor used.
.P
If the input table is large, the packet format for the records
may be too large to be handled by the editor.
\fIIf the editor complains,
respond with 'n' when asked whether to make the updates.\fR
Otherwise, data may be corrupted or lost.
In this case, a where-clause may be used to update only
a portion of the file at a time.
.br
.ne 24
.SH EXAMPLES
.nf
$ ED=ed
$ export ED
$ nuedit SP where snumber leq s3
nuedit: 1 records selected from SP.
39
1,$p
#####   1
snumber s3
pnumber p1
qty     200

/qty/s/200/300/p
qty     300
w
39
q
nuedit: 1 records converted
nuedit: make these changes in SP (y or n)? y
nuedit: changes made, SP updated
.ne 17
.sp

BEFORE:            AFTER:

|------------|     |------------|
|snu|pnu| qty|     |snu|pnu| qty|
|mbe|mbe|    |     |mbe|mbe|    |
|r  |r  |    |     |r  |r  |    |
|------------|     |------------|
|s1 |p1 | 300|     |s1 |p1 | 300|
|s1 |p2 | 200|     |s1 |p2 | 200|
|s1 |p3 | 400|     |s1 |p3 | 400|
|s2 |p1 | 100|     |s2 |p1 | 100|
|s2 |p2 | 200|     |s2 |p2 | 200|
|s3 |p1 | 200|     |s3 |p1 | 300|
|------------|     |------------|
.fi
.SH FILES               \" optional 
.so Files.u.h
.TP
\fBpak$$\fR
\- Packet format file for editing.
If the updates are aborted for whatever reason, the
edited changes are contained in this file.
.SH "SEE ALSO"
alter(UNITY),
delete(UNITY),
insert(UNITY),
nalter(1),
ndelete(1),
ninsert(1),
retrieve(1),
setunpackenv(3),
ucheck(UNITY),
uedit(UNITY).
uenter(UNITY),
tuple(UNITY), and validate(UNITY).
