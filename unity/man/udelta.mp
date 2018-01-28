.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH udelta "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
udelta \- make a delta to a UNITY table stored under SCCS
.SH SYNOPSIS
.nf
\fIudelta\fP [-rSID] [-s] [-n] [-glist] [-m[mrlist]] [-y[comment]] \\
  [-p] file ...
.fi
.SH DESCRIPTION
\fIudelta\fP is used to introduce permanently changes in a UNITY
table stored under the Source Code Control System (SCCS).
\fIudelta\fP works the same as the SCCS command \fIdelta\fP(1)
with the following exceptions:
.P
.IP "-"
\fIudelta\fP deltas two files, the UNITY table
and the description file (Dtable) for each table
named.
.IP "-"
Only tables can be named.  \fIudelta\fP will not accept a
directory name or read from the standard input.
A table name of "-" (indicating take the standard input) is not valid.
.IP "-"
The records in the tables are sorted by the first
attribute (which should be a key attribute) and modified to
be compatible with SCCS.
Sorting the records by the first attribute reduces the
chance that a record will appear to be deleted and
inserted during a delta if it has only changed
positions in the table.
The sorted records are broken into multiple lines of 500 characters or less.
A carrot (^) is placed at the end of each record.
This will permit the records to be reconstructed when they
are unloaded from SCCS.
.P
All keyletter arguments function as described
for the SCCS \fIdelta\fP(1) command.
.SH EXAMPLE
The command
.sp
.nf
udelta -y"updated table" /usr/test/table
.fi
.sp
will used the files "table" and "Dtable" in the current directory
to update /usr/test/s.table.s and /usr/test/s.Dtable, respectively.
.SH FILES
\fIudelta\fP uses the same auxiliary files as \fIdelta\fP(1),
with one additional file which is the modified table.
.sp
.nf
unity/udelta.c, zuadmin.c - the C source program
bin/udelta - the executable program
.fi
.SH WARNING
A delta that changes only one record may cause
more then one line to be inserted and deleted,
since long records are split into more than one physical line.
.SH "SEE ALSO"
\fIuadmin\fP(UNITY), \fIuget\fP(UNITY), \fIadmin\fP(1),
\fIdelta\fP(1), \fIget\fP(1)
.br
Source Code Control User's Guide by
L. E. Bonanni and C. P. Salemi.
