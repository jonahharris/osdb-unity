.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH erase-index "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
erase-index, ei \- erase UNITY index files
.SH SYNOPSIS
\fIerase-index\fP aname \fIfrom\fP table
.SH DESCRIPTION
The \fIerase-index\fP command allows the user to erase index files created
for a UNITY table by the \fIindex\fP command.
It effectively hides the naming convention for UNITY index files
from the user.
\fIaname\fP and \fItable\fP are the attribute name and UNITY table name,
respectively, for which the index files are to be deleted.
.P
\fIei\fP is an abbreviation for \fIerase-index\fP to save on key-strokes.
.P
Index files would normally be deleted when the user feels that they do
not speed up use of the \fIuselect\fP, \fIujoin\fP
and \fIsetdiff\fP commands enough to incur the
overhead of updating the indexes during the use of these
commands (assuming the table is updated )
and also the disk space that is used.
.SH FILES
.nf
unity/ei.c, zei.c - the C source program
bin/erase-index,ei - the executable programs (linked)
A<table>.<attribute>, B<table>.<attribute> - the index files
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY),
\fIindex\fP(UNITY), \fIuselect\fP(UNITY),
\fIsetdiff\fP(UNITY), \fIujoin\fP(UNITY)
