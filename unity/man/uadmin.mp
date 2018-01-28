.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH uadmin "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
uadmin \- store a UNITY table under SCCS
.SH SYNOPSIS
.nf
\fIuadmin\fP [-rrel] [-t[name]] [-fflag[flag-val]] [-alogin] \\
      [-m[mrlist]] [-y[comment]] table ...
.fi
.SH DESCRIPTION
\fIuadmin\fP
is used to place a UNITY table (table and description file)
initially under the Source Code Control System (SCCS).
\fIuadmin\fP works the same as the SCCS command
\fIadmin\fP(1) with the following exceptions:
.P
.IP "-"
\fIuadmin\fP is used only to initially place a UNITY table
under SCCS control.  Only keyletter arguments that
apply to the creation of new SCCS files are permitted
(as shown above).  To change the parameters of a
table already under SCCS control, the SCCS command
\fIadmin\fP(1) should be used manually on the table
and description (Dtable) files.
.IP "-"
The -i option is generated automatically, since the
command is only used to initially place UNITY tables
under SCCS control.
The table will be placed in the SCCS file s.table.s
and the description file (Dtable) in s.Dtable.
.IP "-"
Only tables can be named.  \fIuadmin\fP will not accept a
directory name or read from the standard input.  
A table name of "-" (indicating take the standard input) is not valid.
.IP "-"
Before the table is placed under SCCS, the
records are sorted by the first attribute
(which should be a key attribute).
Sorting the records by the first attribute will
reduce the chance that a record will incorrectly appear
to be deleted and inserted in subsequent deltas because of
a change in its position in the table.
The output is modified to make the table compatible with SCCS.
Tuples in a UNITY table may be up to 4096 characters per line in length.
SCCS cannot be used on lines this long.
The sorted records are broken into multiple lines of 500 characters or less.
A carrot (^) is placed at the end of each record.
This will permit the records to be reconstructed when they
are unloaded from SCCS.
Input containing the terminating character (^) will be flagged with an error.
.SH EXAMPLE
The command
.sp
.nf
uadmin -y"creating table" /usr/test/table
.fi
.sp
will take the files "table" and "Dtable" from the current directory
and create the SCCS files /usr/test/s.table.s and /usr/test/s.Dtable.
The table.s file has lines with less than 500 characters and terminated
by the UNITY record terminator character.
.SH FILES
\fIuadmin\fP may create the same auxiliary files
as \fIadmin\fP(1), plus an additional file which is the modified
table.
.P
For each UNITY table placed under SCCS
control, two files (s.table.s and s.Dtable)
will be permanently created.
.nf
.sp
unity/uadmin.c, zuadmin.c - the C source program
bin/uadmin - the executable program
.fi
.SH "SEE ALSO"
\fIudelta\fP(UNITY),
\fIuget\fP(UNITY),
\fIadmin\fP(1),
\fIdelta\fP(1),
\fIget\fP(1)
.br
Source Code Control System User's
Guide by L. E. Bonanni and C. A. Salem.
