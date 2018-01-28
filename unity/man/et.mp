.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH erase-table "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
erase-table, et \- erase UNITY table and auxiliary files
.SH SYNOPSIS
\fIerase-table\fP table [table ...]
.SH DESCRIPTION
The \fIerase-table\fP command can be used to remove both
the table and the auxiliary files (description file, index files,
validation script, and error table) for one or more UNITY tables.
This command effectively hides the naming conventions
for the auxiliary files.
This command is executed when the UNITY table is no longer to be used;
otherwise the \fIunload\fP command would be used.
.P
\fIet\fP is an abbreviation for \fIerase-table\fP to save on key-strokes.
.SH CAVEAT
This command does a fork() and exec() in the process of removing all
associated index files and may be slow on some machines.
.SH FILES
.nf
unity/et.c, zet.c - the C source program
bin/erase-table, et - the executable programs (linked)
D<table> - the description file removed
[AB]<table>.* - the index files removed
V<table> - the validation script removed
E<table> - the error table removed
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIunload\fP(UNITY)
