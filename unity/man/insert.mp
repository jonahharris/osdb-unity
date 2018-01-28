.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH insert "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
insert \- insert new records into a UNITY table
.SH SYNOPSIS
.nf
\fIinsert\fP [-c] [-q] [-rErrorLimit] [-Itable] [-n[newline]] \\
 \fIin\fP table1 [\fIinto\fP table2] [ {\fIprompting\fP | \fIfrom\fP table3 } ]
.fi
.SH DESCRIPTION
The \fIinsert\fP command allows the user to add new records to the
end of an existing UNITY table (table1 in the SYNOPSIS).
There are three different insertion formats:
.TP
From a UNIX file:
Records are inserted into the table from the UNIX file table2 specified
in the \fIfrom\fP clause as in
.sp
\fIinsert\fP \fIin\fP table1 \fIfrom\fP table2
.sp
The records in the file must be prepared according to the table description of
the table table1.
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input table (table2).
Note that if the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.TP
Interactive record insertion:
Records are interactively inserted by entering them from the standard input.
The program reads an entire record at a time and then prompts for a new record
with the phrase "New record:".
This option is specified by executing
.sp
\fIinsert\fP \fIin\fP table1
.sp
The user must type the list of attribute values of a record, including
terminators, followed by a carriage return.
The user can enter as many new records as desired and input is
terminated with an EOF (control-d).
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input.
.TP
Interactive prompting for individual attributes:
The \fIinsert\fP program allows the user the option of
being prompted for every attribute of records to be inserted in the table.
Normally, the \fIuenter\fP command would be used instead
(this option is now obsolete but remains for upward compatibility).
This option is invoked by specifying the "\fIprompting\fP" option on
the command line as in
.sp
\fIinsert in\fP table1 \fIprompting\fP
.sp
The program informs the user that input is terminated by a control-d,
prints "New record:" and then begins prompting for attribute values
(one per line).
When an entire record has been input, the program asks the user if
the record is to be inserted.
A response beginning with 'N' or 'n' will cause the record to be discarded;
otherwise, the record is written out to a temporary file.
"New record:" is printed to let the user know that
a new record is being started and the program continues prompting for
attribute value input.
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input and
for what attribute values to prompt.
.sp
The prompting mode also allows the user to easily enter multi-line attributes.
If a line is entered whose last character is a '\\',
then it is taken to mean that the field is continued on the following line.
The '\\' is replaced by a "newline" character (tilde being the default) and
the program reads the next input line as part of the current field.
This continues until a line not ending in '\\' is received.
The "-n\fIc\fP" option can be used to change the default "newline"
character to \fIc\fP.
If "-n" is specified without a following character, then the multi-line
input is directly concatenated together without any "newline" character
inserted.
.sp
When the last record has been input, the user enters control-d
when prompted for another attribute.
The program ensures that only complete records are output so that a
user may terminate entry while being prompted for any attribute.
.sp
If either interactive mode of inserting records is being used,
the program then informs the user of the temporary file name in
which the new records are stored before continuing
so that if the program is interrupted or if an error occurs in the
table, the input can be retrieved from the temporary file for further use.
The program asks if the records are to be inserted into the table.
A response beginning with 'N' or 'n' will cause the program to exit leaving
the records in the temporary file (the user may also break the program
at this point causing the same results).
The input may then be edited if a mistake has been made and
then inserted using the "\fIfrom\fP" clause with the temporary file name.
Otherwise, the records are checked and then inserted at the end of
the table if no errors are detected.
The table is locked for update only after the records have been
checked and found valid.
.P
The "-rErrorLimit" option is used to change
the limit on the number of individual error messages
that are printed when errors are found in the input records.
If "ErrorLimit" is set to zero (0) then
and error summary message is printed without
printing any individual (per record) error messages.
The default value for "ErrorLimit" is one (1).
.P
The command is considered an update command if a table name is given
(not "-") and the "into clause" is not specified.
In this case,
the \fIinsert\fP program copies the original table to a temporary file,
appends the new records to the end of the file,
and then links the temporary file into the table.
Although increasing the update time, it makes this update program
crash-resistant.
Only one user may update the table at a time but any number of
users may read the table.
The user must have write permission in the directory containing
the table, but needs only read permission on the table itself.
The group and user identification, and mode of the table are preserved.
.P
If an "into clause" is specified, the output is written into
the named table and a corresponding descrition file is created in the
current directory.
The data in the input table (table1) is first copied to the output file
followed by the records to be inserted.
An error is printed if the output table already exists.
If \fIonto\fP is specified instead of \fIinto\fP, the output is
appended to that table if it already exists or the table created
and written if it does not exist.
.P
If the input table specified is "-", then input is taken from
the standard input instead of a table.
Normally, the -I option must be used to specify the name of a table from
which the description file name can be determined and used.
If "\fIfrom\fP -" (from standard input) was specified and
the -I option was not given,
then the standard input stream is checked
for the description beginning with a \fI%description\fP and ending
with a \fI%enddescription\fP.  This allows \fIinsert\fP to interface directly
with the output of New Unity commands, such as \fIretrieve(1)\fP,
that do not create intermediate description files.
Otherwise, the -I option must be used to specify the name of a table from
which the description file name can be determined and used.
The standard input cannot be specified for either interactive
input options (either record or attribute prompting).
.P
If the output table name specified is "-", then the table output will
be written to the standard output.
Normally, the -O option must be used to specify the name of a table from
which the output description file name can be determined.
However, the -c option can be used instead to copy the description
to standard output starting with \fI%description\fP and ending with
\fI%enddescription\fP.
This allows \fIinsert\fP to be used as an intermediate filter between
various UNITY or New Unity commands without having to create/cleanup
intermediate description files.
.P
When the -O option is used,
the description file will be created in the current directory
with one exception.  If the alternate table name
is the same as the output data table name,
then the description file will be created in the data directory.
If no table is given with the -O option then no output description
file is created.
.P
If the "-q" option is specified, the program does not print out
the number of records that are inserted.
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
.SH FILES
.nf
unity/insert.c, zinsert.c - the C source program
bin/insert - the executable program
/tmp/DnoI<PID> - temporary description file when no -Itable option
itmp?????? - temporary hold file for interactive input
<directory of table>/L<table> - the lockfile
<directory of table>/itmp2?????? - the temporary update file
.fi
.SH "SEE ALSO"
Intro(UNITY), \fIalter\fP(UNITY), \fIdelete\fP(UNITY),
\fIuedit\fP(UNITY), \fIuenter\fP(UNITY), \fIretrieve\fP(NewUNITY)
