.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH ndelete 1 "" "New UNITY"
.SH NAME                \" required
ndelete \- delete records from a UNITY relation
.SH SYNOPSIS            \" required    
.na
.HP
.B ndelete
[-dioqrV] [-Q [\fIErrorLimit\fP]] [from]
\fI<table1>[=<alt_table>]\fP
[with \fI<table2>[=<alt_table>]...]\fP
.br
.RI [where " <where-clause>"]
.ad
.SH DESCRIPTION         \" required - 1/2 page in length
The \fBndelete\fR command allows the selective deletion of records from
a UNITY table.
The \fIwhere-clause\fR specifies matching conditions that the records being
deleted will meet.
It allows checking the values of attributes in each record against constants and,
if the \fIwith\fR parameter is given, allows checking the values of attributes
against attributes of records in other tables (using a join).
.PP
In normal operation, \fBndelete\fR prints out a count of the deleted records.
This behavior can be changed by command line parameters.
.PP
While deletions are taking place, the table is locked against other users 
changing it.
.PP
The \fBretrieve\fR command manual page provides a more detailed explanation of
\fIwhere-clause\fRs and cross-table matching (joins).
.SH PARAMETERS          \" required - if delete takes parameters - alphabetical order
.TP
.B -d
Demo or no-change mode.
Do not perform any deletions, but act as if they were being done.
This option is used to see what
.B ndelete
would delete, without making any deletions.
.TP
.BI -delim c
Use character \fIc\fR as the delimiter between all attribute
values when printing old (deleted) records (see the \fI-o\fP option).
.TP
.B -i
Ignore the case of letters in
.I all
the string and regular expression comparisons of the where-clause.
This option is a short hand notation to pre-pending the
.RB `` c ''
prefix to all the comparison operators in the where-clause.
With this option, the string ``this'' would match ``This'' or ``THIS''.
The regular expression ``this'' would match ``This is a test''
or ``THIS IS A TEST''.
(There is no need for grouping with square brackets.)
This option is analogous to the -i option of
.BR grep (1).
.TP
.B -o
Print out the (old) records being deleted.
If both \fI-r\fR and \fI-o\fR are specified, the record number is
separated from the record text with a colon.
.TP
.B -Q
Gracefully continue after encountering any tuple syntax errors instead of quiting.
An error limit can be specified with this option (i.e., "-Q \fIErrorLimit\fP")
to limit the number of tuple syntax error (warning) messages that would otherwise
be printed when there are many tuples that (might) have syntax errors.
An error limit of minus one (-1) can be used to indicate that
a warning message for each tuple error is to be printed.
If no error limit or an error limit of zero (0) is given,
only a summary message with the total number of tuples with tuple errors
for all relations that were queried will be printed.
The tuple error summary message is not printed if there were no tuple errors or
if the -q (quiet mode) option was specified.
.TP
.B -q
Quiet mode.
Do not print a count of the records deleted.
.TP
.B -r
Print out the record numbers of the records being deleted.
If both \fI-r\fR and \fI-o\fR are specified, the record number is
separated from the record text with a colon.
.TP
.B -V
Print the version of the command and exit.
No deletions are done.
.TP
.RI [\fBfrom\fR] <table1> [\fB=\fR <alt_table> ]
The table records are to be deleted from.
The word ``from'' may be omitted.
.IP
By default, the descriptor file for the table is named \f(CWD\fI<table1>\fR.
A descriptor file from another table can be specified by adding
``=\fI<alt_table>\fR'' after the table name.
The alternate descriptor file will be \f(CWD\fI<alt_table>\fR.
.TP
.BI with " <table2>\fR[\fP" = <alt_table>\fR]...\fP
Extra table(s) containing records that are matched against
in the \fIwhere\fR clause.
A table named ``\-'' refers to standard input.
The standard input can only be used once.
.IP
If this option is used, attribute names appearing in more than one table
must be prefixed with the table name and a period (\f(CWtable.attribute\fR)
or the relation's positional number and a period (\f(CW2.attribute\fR).
See the
.BR retrieve (1)
manual page for more information on attribute name syntax.
.IP
By default, the descriptor file for the table is named \f(CWD\fI<table2>\fR.
A descriptor file from another table can be specified by adding
``=\fI<alt_table>\fR'' after the table name.
The alternate descriptor file will be \f(CWD\fI<alt_table>\fR.
.br
.ne 6
.TP
.BI where " <where-clause>"
.so Where.u.h
.LE
.br
.ne 6
.SH ENVIRONMENT         \" optional
.so Environ.u.h
.so Envunpack.u.h
.SH EXIT CODES          \" strongly recommended
.so Exit.codes.h
.SH RESTRICTIONS        \" optional - includes caveats and bugs
.so Restrict.h
.SH EXAMPLE             \" strongly recommended
Delete all the signoffs for load k8.0:
.PP
.RS
\f(CWndelete from signoff where load leq k8.0\fR
.RE
.PP
To list the records that would be deleted without changing the data base:
.PP
.RS
\f(CWndelete -do from signoff where load leq k8.0\fR
.RE
.PP
Delete all the signoffs that apply to LMRs which are in the
\fIsubmitted\fR state.
(Look up the MR in the \fIsubmitmr\fR relation, and look up the
corresponding LMR in the \fIlmrinfo\fR relation and check the LMR's state.)
.PP
.RS
.ft CW
.nf
ndelete from signoff with submitmr lmrinfo		\e
	where signoff.MR fleq submitmr.submitmr		\e
		and submitmr.lmr fleq lmrinfo.lmr	\e
		and lmrinfo.state leq submitted
.RE
.SH FILES               \" optional 
.so Files.u.h
.SH SEE ALSO            \" strongly recommended
delete(UNITY), nalter(1), ninsert(1), nuedit(1), retrieve(1),
setunpackenv(3), ucheck(UNITY), and uedit(UNITY).
