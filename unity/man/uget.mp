.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH uget "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
uget \- get a version of a UNITY table stored under SCCS
.SH SYNOPSIS
.nf
\fIuget\fP [-rSID] [-ccutoff] [-ilist] [-x[list] [-aseq-no.] [-k] [-e] \\
       [-s] [-b] [-g] [-t] file ...
.fi
.SH DESCRIPTION
\fIuget\fP generates a UNITY table stored under
the Source Code Control System (SCCS) according to the specifications given
by its letter arguments.
\fIuget\fP works the same as the SCCS command \fIget\fP(1)
with the following exceptions:
.P
.IP "-"
\fIuget\fP generates two files, the UNITY table
and the description file (Dtable).
.IP "-"
Only tables can be named.  \fIuget\fP will not accept
a directory name or read from the standard input.
A table name of "-" (indicating standard input) is not valid.
.IP "-"
The keyletter arguments -p, -l, -m, -n, and -w are not permitted.
They would incorrectly alter or print the UNITY table and
description files.
.IP "-"
The records in the table are automaticly
reconstructed, joining lines that were longer than 500 characters
and removing the record terminator characters.
.P
All the keyletter arguments function as described for \fIget\fP(1).
.SH EXAMPLE
The command
.sp
.nf
uget -e /usr/test/table
.fi
.sp
will get the latest copy of /usr/test/s.table.s and /usr/test/s.Dtable
and create the files "table" and "Dtable" in the current directory
for edit.
.SH FILES
\fIuget\fP may create the same auxiliary files as \fIget\fP(1),
with one additional type used for the modified table.
This file name is formed by appending a ".s" to the existing file name.
The ".s" indicates that the records in this file have
passed through modification making them compatible with SCCS.
.sp
.nf
unity/uget.c, zuget.c - the C source program
bin/uget - the executable program
.fi
.SH "SEE ALSO"
\fIuadmin\fP(UNITY), \fIudelta\fP(UNITY), \fIadmin\fP(1),
\fIget\fP(1), \fIdelta\fP(1)
.br
Source Code Control System User's Guide
by L. E. Bonanni and C. A. Salemi.
