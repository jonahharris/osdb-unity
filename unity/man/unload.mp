.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH unload "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
unload \- unload records between a UNITY table and a UNIX file
.SH SYNOPSIS
\fIunload\fP table1 [ \fIto\fP table2 ]
.SH DESCRIPTION
The \fIunload\fP command allows the user to unload records between a
UNITY table and a UNIX file.
The contents of table1 are transferred to the UNIX file
table2 (if the \fIto\fP clause is specified).
The table table1 is removed but the description file is left untouched.
.P
This command will not read the standard input nor will it write
to the standard output (i.e., table names of "-" are not valid).
.SH FILES
.nf
unity/unload.c, zunload.c - the C source program
bin/unload - the executable program
.SH "SEE ALSO"
\fIload\fP(UNITY)
