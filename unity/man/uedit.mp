.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH uedit UNITY "" "UNITY User's Manual"
.SA 1
.SH NAME
uedit \- edit records in a UNITY table
.SH SYNOPSIS
.nf
\fIuedit\fP [-q] [-Itable] [-v] [-n[nline]] [-u] table [where clause]
.fi
.SH DESCRIPTION
The \fIuedit\fP command provides a crash resistant means
for editing UNITY tables (the crash resistance is discussed more
fully on the Introduction manual page).
The table is locked for update during the entire update session
so that no other UNITY update command
can be used on the table (\fIalter\fP, \fIdelete\fP, \fIinsert\fP,
and \fIuenter\fP all use
the same locking facility and do not allow concurrent access).
.P
The only mandatory argument is the name of the table on which changes
are to be made.  The standard input ("-") CANNOT be used.
The \fIwhere\fP clause is used to divide the table selectively into a
group of records to be edited and those that are to be left unchanged.
\fIuedit\fP reads the specified table and transforms records
satisfying the \fIwhere\fP clause into stream packet format for editing
by the user.
Tuples that do not satisfy the criteria of the \fIwhere\fP clause are stored
in a second hold file for later combination with the updated records.
If a \fIwhere\fP clause is not specified,
then all the records are converted to packet form and are available for editing
(the hold file will be empty).
In a packet, the data is structured into field-name/field-value pairs
consisting of the name of the attribute and its
value separated by a tab (a convenient form for editing).
If the "-u" option is specified, then the "user\-friendly" names
are used instead of the attribute names, if they exist in
the description file (see \fItable\fP(UNITY).
If the "user\-friendly" names do not exist, then the attribute names
are appear in the packets.
.P
Each packet is preceded by a line of the form
.sp
#####<tab><packet number>
.sp
where <packet number> is the sequence number of the packet; it is used
for reference regarding the validation described below.
This line is thrown away when converting back to tuples and
thus the user may delete it without any problems (i.e., it is only
for the user's reference).
Blank lines are also ignored.
.P
The user is then put into the UNIX editor and
may move through the data,
making any changes using the standard editor commands.
\fIuedit\fP examines the environment variable ED and then EDITOR
to determine if an editor other than /bin/ed should be executed; this
makes it easy to use other available editors such as \fIvi\fP or
\fIemacs\fP ($ED <packet-file-name> is executed).
When changes are complete the user exits the editor in the standard fashion.
.P
At this point, the data will be converted back to record format.
If an error occurs in the conversion, the user is notified and asked if
the program should re-enter the editor for correction of the problem.
A response of 'y' will cause the user to be put back into the editor for
correction.  A response of 'n' will cause the file of edited data in
packet format to be saved in a temporary file whose name is given
in an error message to the user before program termination.
If the packet to record conversion succeeds, the updated records are
appended to the data in the hold file
(the records not satisfying the \fIwhere\fP clause).
The hold file now contains the updated version of the table.
The user is then prompted to choose whether really to
make these changes to the table or not.
If the user chooses to make these changes,
the original table is removed,
and the hold file is linked to the original table thus updating the table.
If not,
the hold file is removed and the original table is unchanged.
.P
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
\fIuedit\fP may be used from any terminal on any UNITY table
(i.e., there are no terminal restrictions other than those imposed
by the particular editor used).
Use of \fIuedit\fP for update alleviates problems caused by records
that are too long for the editor to handle.
The only restriction is that the combination
of the attribute name length and the length of the attribute value
does not exceed 512 characters (unless UNITY newline characters
are embedded in the value as discussed in the next paragraph).
.P
The -n[newline] option will cause a newline
and tab to be generated in the packet stream output
each time the "newline" character is encountered in the record.
The default newline character is tilde ('~').
The reverse action is taken when converting the packets back to records.
If -n is specified with no argument,
then the newline character is assumed to be null and
the newline option is effectively turned off.
.P
The records will normally be validated only to
the extent that they do not corrupt the UNITY table.
The validation that is done determines that
fixed width attributes are of the proper length
and no non-printable characters or characters
with special meaning (i.e.,'~') are present.
Further validation may be done by specifying the "-v" option on
the command line.
This causes the \fIvalidate\fP
command to be invoked to validate the updates.
Three auxiliary files are needed for validation: the description file,
the validation script, and the error message table.  These are
described in detail on the \fIvalidate\fP(UNITY) manual page.
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
If only a subset of the records are selected for update,
these records will appear at the end of the file after
all records which were not selected for update.
.P
If the input file is large, the packet format for the records
will be too large to be handled by the editor.
In this case, the editor will complain.
DO NOT ATTEMPT TO WRITE THE PACKET FILE AND WHEN ASKED WHETHER TO
MAKE THE UPDATES, DO NOT RESPOND WITH Y.
This may cause records to be lost.
In this case, a where clause must be specified to update only
a portion of the file.
The simplest approach is to update the first half of the file
(using rec# in the selection), then update the first half of
the file again (since the updated records are stored at the
end of the file).  This approach will work with splitting the
file into more than 2 selections as well.
.P
If the "-q" option is specified, the program does not print out
the number of records that are edited.
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
.ne 24
.SH EXAMPLES
.nf
$ ED=ed
$ export ED
$ uedit SP where snumber leq s3
uedit: 1 records selected from SP.
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
tuple: 1 records converted
Make these changes in SP (y or n) y
Changes made, SP updated
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
.SH FILES
.nf
unity/uedit.c, zuedit.c - the C Source program
bin/uedit - the executable program
tuple$$,Dtuple$$ - tuples to be updated and description file
pak$$,Dpak$$ - packet format and description file
save$$ - file where updates saved during abort
<directory of table>/L<table> - the lockfile
<directory of table>/tmp????? - the temporary update file
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fItuple\fP(UNITY), \fIvalidate\fP(UNITY), \fIed\fP(1)
