.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH validate "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
validate \- record validator
.SH SYNOPSIS
.nf
\fIvalidate\fP [-Itable] table1
.SH DESCRIPTION
\fIvalidate\fP is a table validator as described on the
\fIval\fP(UNITY) manual page.
The validator validates files in the form of UNITY tables:
this means that the records must contain a fixed number of fixed
width or terminator fields as described on the manual page for
\fItable\fP(UNITY).
.P
The default method for using \fIvalidate\fP is to name
the table to be validated as the only argument;
the names of three auxiliary files needed as described below are
determined from the simple file name of the specified table.
.P
If the -I option is used, the table name specified is used to determine
an alternate set of auxiliary files (described below).
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description and auxiliary files.
.P
If the input table specified is "-",
then input is taken from the standard input instead of a table.
The -I option must be used to specify the name of a table from
which the auxiliary files can be determined.
.P
Before \fIvalidate\fP can be invoked,
three additional files to be associated with the table must be created.
.TP 3
Description file:
This file contains a description of the layout of the fields within the
table as well as whether a field is a fixed length field (and its
associated length) or a terminator field (and its associated terminator).
Field names may be up to 31 characters in length, must begin with an
alphabetic character or underscore (_), and may contain
only alphanumeric characters or underscore.
The format is further described on the \fItable\fP(UNITY) manual page.
The name of this file must be D<table> for a table with simple
file name <table> and must reside in the current directory,
the same directory as the table,
or in one of the directories specified in the $UNITYDFILES path environment
variable
when \fIvalidate\fP is executed.
.TP 3
Validation script:
This file contains the validation criteria for the table.
The name of this file must be V<table> for a table with simple
file name <table> and must reside in the current directory,
the same directory as the table,
or in one of the directories specified in the $UNITYDFILES path environment
variable
when \fIvalidate\fP is executed.
The format of the validation table is as described on the
val(UNITY) manual page.
\fIvalcmp\fP produces the object file W<table> in the current directory
automatically when W<table> does not exist or its modification date
is older than V<table>.
.TP 3
Error Table:
This table defines error messages corresponding to the error codes
used in the validation table.
The table has two columns:
the first column contains the error code as used in the validation script table
and the second column (separated from the first by a tab) contains
the associated error message.
The name of this file must be E<table> for a table with simple
file name <table> and must reside in the current directory,
the same directory as the table,
or in one of the directories specified in the $UNITYDFILES path environment
variable when \fIvalidate\fP is executed.
If this table does not exist, the error codes are printed out
when an error occurs instead of the corresponding error message.
.P
As an example, suppose the /etc/passwd file is to be validated.
.br
The file Dpasswd could be as follows:
.sp
.nf
login  t:      8l       LOGIN NAME
passwd t:      14l      PASSWORD
uid    t:      5r       USER ID
gid    t:      5r       GROUP ID
acct   t:      20l      ACCOUTING INFORMATION
home   t:      15l      HOME DIRECTORY
shell  t\\n     12l      LOGIN SHELL
.fi
.sp
The file Vpasswd could be as follows:
.sp
.nf
login % "[A-Za-z][A-Za-z0-9]{1,6}"                          l1
passwd == ""
        shell == "/bin/rsh" | shell == "/bin/uname" | \\
                shell == "/bin/sync"                        p1
uid % "[0-9]{1,5}"                                          u1
gid % "[0-9]{1,5}"                                          g1
dept % "[0-9]{1,5}"                                         d1
home != ""                                                  h1
.fi
.sp
The file Epasswd could be as follows:
.sp
.nf
l1      login must be 1-6 alphanumeric characters
p1      entry must have a passwd
u1      user id must be 1-5 numeric characters
g1      group id must be 1-5 numeric characters
d1      department number must be 1-5 numeric characters
h1      entry must have a home specified
.fi
.sp
The /etc/passwd file could be validated by executing:
.nf
   validate /etc/passwd
or validate -Ipasswd < /etc/passwd
or validate -Ipasswd /etc/passwd
.fi
.P
Messages associated with error codes are written on the standard error output.
If a message cannot be found corresponding to the
error code, the error code itself is written out.
In either case, the
record in error is printed first followed by one or more lines with the
record number on which the error occurred followed by the error message.
Diagnostic messages from the machine are also printed on the standard error
output.
\fIvalidate\fP exits with a non-zero exit code if any error occurs
and 0 otherwise.
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
default search order for locating a description or an auxiliary file.
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
.SH BUGS
For upward compatibility with older versions of \fIvalidate\fP,
the command will also read from the standard input if no table
name is specified, rather than just a table name of "-".
Also, the syntax "-f table" may be used instead of "-Itable".
.SH FILES
.nf
unity/validate.c - the C source program
bin/validate - the executable program
D<table>,E<table>,V<table>,W<table> - the auxiliary files
.fi
.SH SEE ALSO
\fIIntro\fP(UNITY), \fItable\fP(UNITY), \fIvalcmp\fP(UNITY), \fIval\fP(UNITY)
