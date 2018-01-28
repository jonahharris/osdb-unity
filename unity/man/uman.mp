.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH uman "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
uman \- print man pages for UNITY commands
.SH SYNOPSIS
uman [ command ... ]
.SH DESCRIPTION
\fIuman\fP prints the documentation
for each of the specified UNITY \fIcommands\fP.
If no command is specified,
a manual consisting of all the manual pages is produced.
.SH EXAMPLES
uman asort
.br
will produce the manual page for the asort command.
.sp
uman | opr -ttx &
.br
will produce,
in the background,
a complete manual on the high\-speed printer.
.SH FILES
.nf
unity/uman.sh - the shell source
bin/uman - the executable command
*.mp - the man pages
.fi
.SH CAVEAT
The \fIuman\fP command originally assumes that the manual pages
are in the directory with the source files.
If UNITY is installed (`make install` is executed), the
installation procedure changes uman to point to the MAN directory
(the directory in which the manual pages are installed).
If the manual pages are installed in another directory without
using the Makefile, the \fIuman\fP command should be updated by
changing the line near the beginning of the shell which
changes directory to
to the directory in which the .mp manual page source files are stored.
