.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH uenter UNITY "" "UNITY User's Manual"
.SA 1
.SH NAME
uenter \- enter records into a UNITY table
.SH SYNOPSIS
\fIuenter\fP [-q] [-Itable] [-v] [-n[nline]] [-u] [-c] \fIin\fP table
.SH DESCRIPTION
The \fIuenter\fP command provides a crash resistant means
for entering records into a UNITY table
(the crash resistance is discussed more fully on the Introduction
manual page).
The user is prompted for all attribute values.
When entry is complete and the records are ready to be
inserted in the table, the \fIinsert\fP
command is used so that no other UNITY update command
can be used on the table (\fIalter\fP, \fIdelete\fP, \fIinsert\fP,
\fIcompute\fP, \fInprintf\fP, \fIuprintf\fP,
and \fIuedit\fP all use
the same locking facility and do not allow concurrent access).
.P
The prompting sequence may be used from any
terminal for any table (i.e., there are no terminal dependencies).
The user is prompted with the name of each attribute in the table.
If the "-u" option is specified, then prompts are the "user\-friendly" names
instead of the attribute names, if they exist in the description file
(see \fItable\fP(UNITY).
If the "user\-friendly" names do not exist, then the attribute names
are used for prompting.
The user can then enter the desired value for each attribute.
Attribute values may extend over multiple lines by entering a backslash ('\\')
as the last character in the line.
The entry mode is exited by typing a EOF (control-d).
.P
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input
(attribute or user-friendly names are taken from the alternate description
file as well).
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
The entered records are converted to stream packet format and output
into a hold file.
In a packet format, the data is structured into field-name/field-value pairs
consisting of the name of the attribute and its
value separated by a tab (a convenient form for editing).
If the "-u" option is specified, then the "user\-friendly" names
are printed instead of the attribute names.
.P
Each packet is preceded by a line of the form
.sp
#####<tab><packet number>
.sp
where <packet number> is the sequence number of the packet; it is used
for reference with reguard to the validation described below.
This line is thrown away when converting back to tuples and
thus the user may delete it without any problems (i.e., it is only
for the user's reference).
Blank lines are also ignored.
.P
The user is asked if editing of the new input is desired.
A response of 'y' will cause the user to be put into the editor
for making any changes using the standard editor commands.
\fIuenter\fP examines the environment variable ED and then EDITOR
to determine if an editor other than /bin/ed should be executed; this
makes it easy to use other available editors such as \fIvi\fP or
\fIemacs\fP ($ED <packet-file-name> is executed).
When changes are complete the user exits the editor in the standard fashion.
At this point, the data will be converted to record format.
If an error occurs in the conversion, the user is notified and asked if
the program should re-enter the editor for correction of the problem.
A response of 'y' will cause the user to be put back into the editor for
correction.  A response of 'n' will cause the file of edited data in
packet format to be saved in a temporary file whose name is given
in an error message to the user before program termination.
If the packet to record conversion succeeds, 
the user is then prompted to choose whether to insert the new records.
A response of 'y' will cause the new records to be inserted into the table
using the \fIinsert\fP command.
An information message shows how many records are inserted.
A response of 'n' will leave the original table unchanged.
.P
A newline and tab are generated in the packet stream output
each time a line ends with a backslash (\\).
When the packets are converted to records, this newline/tab sequence is
converted to a special character call the "newline" character.
The default newline character is tilde ('~').
The "newline" character may be changed by using the -n[nline] option.
If -n is specified with no argument,
then the newline character is assumed to be null and
no newline character is inserted in the conversion to records
(i.e., the newline option is effectively turned off).
.P
If an input line ends with a vertical bar (|), then the next line of
input will be concatenated on to the end of the previous line of
input with a space separating the two lines.
Thus, vertical bar is a continuation indicator with no newline character
whereas the backslash is a continuation indicator with newline
character added.
.P
To enter a vertical bar or backslash at the end of an input line
without having the character interpreted as a continuation
line, the user may precede it by a backslash.
Two backslashes entered at the end of the line are interpreted
as a single backslash, and a backslash and vertical bar entered at
the end of a line are interpreted as a vertical bar.
The -c option may be used to turn off all continuation line processing.
A backslash or vertical bar at the end of an input line are treated
as normal characters when the -c option is specified.
.P
The records will normally be validated only to
the extent that they do not corrupt the UNITY table.
The validation that is done determines that
fixed width attributes are of the proper length,
variable length attributes do not contain the terminating character
in their value,
and no non-printable characters or characters
with special meaning (i.e.,'~') are present.
Further validation may be done by specifying the "-v" option on
the command line.
This causes the \fIvalidate\fP command to be invoked to validate the updates.
Three auxiliary files are needed for validation: the description file,
the validation script, and the error message table.
These are described in detail on the \fIvalidate\fP(UNITY) manual page.
These files must exist for the table being edited
(i.e., with names D<table>, V<table> and E<table> in either the
present working directory, the directory in which the table resides,
or in one of the directories specified in the $UNITYDFILES path environment
variable)
before editing begins.
\fIvalidate\fP writes its error messages to the standard error output.
If an error occurs on any record, then the record number, 
the error that occurred, and the record in error will be printed.
The user can then re-enter the editor to make the corrections.
.P
If the "-q" option is specified, the program does not print out
the number of records that are entered.
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
.br
.ne 35
.SH EXAMPLE
.sp 2
.nf
$ ED=ed
$ export ED
$ uenter in SP
Terminate insertion with EOF (ctrl-d)

New Record
snumber:        s3
pnumber:        p1
qty:    400

New Record
snumber:
Enter editor to make changes (y or n) y
39
1,$p
#####   1
snumber s3
pnumber p1
qty     400

/qty/s/400/200/
w
60
q
tuple: 1 records converted
Make these changes in SP (y or n) y
insert: 1 records inserted in SP.
.sp
This example inserts one record into the SP file.
.fi
.SH FILES
.nf
unity/uenter.c, zuenter.c - the C source program
bin/uenter - the executable program
tuple$$,Dtuple$$ - tuples to be entered and description file
pak$$,Dpak$$ - packet format and description file
save$$ - file where entry saved during abort
<directory of table>/L<table> - the lockfile
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fItuple\fP(UNITY), \fIuedit\fP(UNITY), \fIinsert\fP(UNITY), \fPed\fP(1)
