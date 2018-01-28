.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH "uselect" "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
uselect \- do selection and projection on a UNITY table
.SH SYNOPSIS
.nf
\fIuselect\fP [-c] [-q] [-Itable] [-Otable] [aname [as aname2] ...] \\
 \fIfrom\fP table1 [\fIinto\fP table2] [where clause]
.fi
.SH DESCRIPTION
\fIuselect\fP builds a new table from
a subset of an existing table by projection and selection on an
individual table.
If attributes are specified on the command line (before the "\fIfrom\fP"),
then only these fields will be contained in the new table in the
order specified (i.e., a projection is done on these fields).
Otherwise, all fields will appear in the output table in the order of the
original table.
The optional "\fIas\fP" feature allows the user to
rename attributes during the process (i.e., uselect aname1 as aname2 ...
will rename the "aname1" attribute "aname2" in the description file for the
new table).
Finally, the \fIwhere\fP clause selects only a subset of
the records based on some condition on the values of certain
attributes in the the table.
.P
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
The output is written to the standard output by default.
If an "into clause" is specified, the output is written into
the named table and a corresponding descrition file is created in the
current directory.
An error is printed if the output table already exists.
If \fIonto\fP is specified instead of \fIinto\fP, the output is
appended to the table if it already exists or the table created
and written if it does not exist.
.P
If the input table specified is "-", then input is taken from
the standard input instead of a table.
Normally, the -I option must be used to specify the name of a table from
which the description file name can be determined and used.
If the -I option is not given, the standard input stream is checked
for the description beginning with a \fI%description\fP and ending
with a \fI%enddescription\fP.  This allows the program to interface directly
with the output of New Unity commands, such as \fIretrieve(1)\fP,
that do not create intermediate description files.
.P
If the output table name specified is "-", then the table output will
be written to the standard output.
Normally, the -O option must be used to specify the name of a table from
which the output description file name can be determined.
However, the -c option can be used instead to copy the description
to standard output starting with \fI%description\fP and ending with
\fI%enddescription\fP.
This allows the program to be used as an intermediate filter between
various UNITY or New Unity commands without having to create/cleanup
intermediate description files.
.P
When the -O option is used,
the description file will be created in the current directory
with one exception.  If the alternate table name
is the same as the output data table name,
then the description file will be created in the data directory.
The -O option can also be used by itself without a table name
in conjunction with the "\fIinto\fP table" clause to indicate
that no output description file is to be created.
However, the use of the -O option in this way only applies
when the output table is not going to standard output
or if the output table format is the same as the input table format
(i.e., no attribute names were specified on the command line).
.P
The processing time of the \fIuselect\fP command can be decreased
using an index created by \fIindex\fP.
If an index exists for the
\fIfirst\fP attribute appearing in the where clause and an 'or' operator
does not appear at the same logical level as that attribute or above,
then the index provides direct access to the table
and thus speeds up the selection process.
However, an index cannot be used if the input is from the standard input.
.P
If the "-q" option is specified, the program does not print out
the number of records that are selected.
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
.SH EXAMPLES
The following examples, based on the description file listed for the
/etc/passwd file on the \fItable\fP(UNITY) manual page, show some
of the features of the \fIuselect\fP command.
.sp
.nf
Ex. 1:  uselect -Ipasswd from - where gid leq 11 </etc/passwd
.fi
.sp
This will print out at the terminal, all records in the passwd file that
have a group identification number of 11.
.sp
.nf
Ex. 2:  uselect from /etc/passwd into group11 where gid leq 11
.fi
.sp
This will select the same records as in example 1, but the results
will be written into the group11 table (and the description file Dgroup11
will also be created as a duplicate of the Dpasswd description file).
.sp
.nf
Ex. 3:  uselect login uid gid from /etc/passwd \\
           where home req "^/r1"
.fi
.sp
This command will print out the login names, and user and group identification
numbers for all entries in the /etc/passwd file that have "/r1" as
the beginning part of their login directory
(a regular expression equality is done
on the entire "home" attribute looking for those values
that begin with the string "/r1").
.sp
.nf
Ex. 4:  uselect login as programmer uid from /etc/passwd into tmp
.fi
.sp
This command will select all records in the /etc/passwd file and
for each record will output the login and user id.  The login attribute
will be renamed programmer in the description file for tmp.
.SH FILES
.nf
unity/uselect.c, zuselect.c - the C source program
bin/uselect - the executable program
A<table>.<attribute>, B<table>.<attribute> - the index files
/tmp/DnoI<PID> - temporary description file when no -Itable option
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIindex\fP(UNITY), \fIretrieve\fP(NewUNITY)
