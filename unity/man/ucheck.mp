.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH ucheck UNITY "" "UNITY User's Manual"
.SA 1
.SH NAME
ucheck \- check UNITY table attributes
.SH SYNOPSIS
\fIucheck\fP [-q] [-rErrorLimit] [-Itable] table
.SH DESCRIPTION
\fIucheck\fP checks to see that the proper number of attributes exist for each
record in a UNITY table using the associated description file.
Records are checked by counting past the number of characters specified
for a fixed width field and skipping to the terminator symbol on a
terminator field.
If less than w characters are found for a fixed width field or
a terminator symbol is not found before a newline is encountered,
then an error message is printed indicating that the record is too short.
If additional characters are found before a newline is encountered,
then an error message is printed indicating that the record is too long.
If a newline is encountered in a fixed width field before encountering
a null ('\\0') byte (i.e., the field conatins binary data),
an error message is printed indicating that a fixed width attribute
in the record contaions an embedded newline.
If the length of a field is greater than or equal to "DBBLKSIZE"
(defined in db.h) or the total record length is greater than
"MAXREC" (defined in db.h) then an error message is printed
along with the corresponding maximum attribute or record size limit.
All input records are checked before \fIucheck\fP terminates.
The identical checks are done when records are loaded or inserted into
a UNITY table.
.P
The "-rErrorLimit" option is used to request
an error summary report and/or to limit the
number of individual error messages.
If "ErrorLimit" is set to zero (0) then
an error summary message is printed without
printing any individual (per record) error messages.
If no \fIErrorLimit\fR is given with the -r option,
then ErrorLimit is set to zero (0), if it was not set by a
previous "-rErrorLimit" parameter, and
an error summary report will be printed
if there were any errors
regardless of whether or not the "-q"
(quiet) option was used (see below).
Otherwise, no error summary report is printed when
the "-q" option is used.
.P
If the "-Itable" option is used, the table name specified is used to determine
an alternate description file to be used in checking the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
If the input table specified is "-", then input is taken from
the standard input instead of a table.
Normally, the -I option must be used to specify the name of a table from
which the description file name can be determined and used.
If the -I option is not given, the standard input stream is checked
for the description that begins with "\fI%description\fP" and ends
with "\fI%enddescription\fP".  This allows \fIucheck\fP to interface directly
with the output of New Unity commands, such as \fIretrieve(1)\fP,
that do not create intermediate description files.
.P
If the "-q" option is specified, the program does not print out
the number of records that are checked.
Note that if there were any record errors then no message is
printed to indicate how many records were checked unless
the "-r" option is used.
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
.TP
UNITYUNPACK
A string of the form "command.suffix" where "command" is the the name of
a command that can be used to read (cat) the contents of a packed relation (file)
and "suffix" is the 1-3 character suffix (without the '.') that is appended to
the filename when the file has been packed (compressed).
Typical values for UNITYUNPACK are "pcat.z", "zcat.Z", and "gzcat.gz".
.IP
It should be noted that packed relations are not allowed as the target
of any type of insert, update, or delete operation.
In addition, using packed relations prevents the use of attribute indexes
and also prevents any packed tuples from being validated, see validate(UNITY).
See the New Unity setunpackenv(3) manual page for more information.
.SH EXAMPLES
.nf
ucheck /etc/passwd
or
ucheck -Ipasswd - </etc/passwd
.fi
.P
would check the passwd file assuming a valid description file exists
in the current directory, in /etc, or in a directory named in $UNITYDFILES.
.P
.nf
ucheck -r /etc/passwd
.fi
.P
would also check the passwd file and print an error summary
(or ok) message without reporting individual record errors.
.SH FILES
.nf
unity/ucheck.c, zucheck.c - the C source program
bin/ucheck - the executable program
/tmp/DnoI<PID> - temporary description file when no -Itable option
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIinsert\fP(UNITY), \fIload\fP(UNITY), \fIretrieve\fP(NewUNITY)
