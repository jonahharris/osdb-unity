.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH ninsert 1 "" "New UNITY"
.SH NAME                \" required
ninsert \- insert records in a UNITY relation
.SH SYNOPSIS            \" required    
.na
.HP
.B ninsert
[-cCpqVz]
.RI [-d <delim-char> ]
.RI [-Q [ErrorLimit]]
.br
.RI [-f <rec#_precision> ]
.RI [-F <REC#_precision> ]
.br
.RI [-m|-r <record_cnt> ]
.RI [-n <newline_char> ]
.RI [[ <attr> " as]"
.IR <attr> [= <value> ]]...
.RI into|onto " <table1>" [ =<alt_table> ]
.br
.RI [from " <table2>" [= <alt_table> ]]
.ad
.SH DESCRIPTION         \" required - 1/2 page in length
The \fBninsert\fR command allows new records to be inserted into a UNITY table.
Records can be taken from standard input or from another table.
Selected attributes of the new records can be given constant values on the command
line so that the value doesn't have to be repeated for each record.
When input is from another table, attributes can be renamed
including the special attribute ``rec#'' which is the
input table record number.
In addition, the special attribute ``REC#'' (in all capital letters),
which also has to be renamed, can be used to obtain the record
number of the tuple being inserted into the output table.
.PP
If the table being inserted into doesn't exist, \fBninsert\fR can create it
and, optionally, its descriptor file.
\fBninsert\fR can also prompt the user for attribute values when input is from
standard input and no from-clause is given.
.PP
While insertions are taking place, the table is locked against other users 
changing it.
.SH PARAMETERS          \" required - if insert takes parameters - alphabetical order
.TP 10
.B -c
If the output table, \fI<table1>\fR, does not exist, create a
description file for it based on the attribute
names appearing on the command line.
The attributes are normally separated by the attribute
delimiter from the input table or a colon (:) if there is not an input table.
.TP
.B -C
Create the description for the output table, like the
.BR -c
option, but put the description with the data table itself, not in a
descriptor file.
.TP
.BI -d <delim-char>
Within the input lines attributes will be delimited by \fI<delim_char>\fP.
The default delimiter character is a newline.
This only applies if the input is from standard input and the
user is not prompted for values (\fB-p\fP option).
.TP
.BI -f <rec#_precision>
Format 'rec#' to a precision of \fI<rec#_precision>\fP digits
which must be between 1 and 10.
The default is not to pad 'rec#' (input table record number)
with any leading zeros.
.TP
.BI -F <REC#_precision>
Format 'REC#' to a precision of \fI<REC#_precision>\fP digits
which must be between 1 and 10.
The default is not to pad 'REC#' (output table record number)
with any leading zeros.
This option can also be used when the are no references to 'REC#'
so that ninsert will validate the format of existing records
in the output (into/onto) table.
Normally, no validation of existing records in the output table
is performed when no reference has been made to 'REC#'.
.TP
.BI -m|-r <record_cnt>
At most
.RB ( -m )
or exactly
.RB ( -r )
.I <record_cnt>
records will be inserted in the destination relation.
If
.B -r
is used, but
.I <record_cnt>
records are not available from the input source,
then an error message will be
printed, NO insertions will be done, and
.B ninsert
will exit with a failure status of 4.
These options are ignored if all attribute values are given
on the command line, i.e., exactly one record is inserted.
.TP
\fB-n\fI<newline_char>\fR
When prompting for input interactively, any attribute
value may be continued on the next line by ending the line
with a backslash as the last character.
When this is occurs, the \fI<newline_char>\fR replaces the
backslash and newline in the prompted input.
.IP
If this option is not given,
the default \fI<newline_char>\fR is tilde ('~').
If \fB-n\fR is given with no argument,
then the newline character is assumed to be null and
any backslash-newline in the prompted input will be removed.
.TP
.B -p
Prompt the user for new tuple values; not applicable if the \fIfrom\fR
option is given.
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
Do not print a count of the number of records inserted.
.TP
.B -V
Print the version of the command and exit.
No insertions are done.
.TP
.B -z
Do not include zero-width (print) attributes
unless explicitly referenced.
The default is that values for all attributes
in the output table are expected from the input table
when no attributes are listed on the command line.
This option also affects the expansion of the
special ``all'' attribute as described below.
.TP
[\fI<attr>\fB as\fR ]\fI<attr>\fR[\fB=\fI<value>\fR]
Specify the attributes to insert into the new table.
If no form of this parameter appears, all attributes for
the output table
will be expected in the input, in the order they appear
in the descriptor file for the table.
.IP
If \fI<attr>\fR appears alone, it names an attribute that will receive
values to insert; attributes not specified will be null.
If attributes appear alone on the command line, only values for those attributes
will be expected from the standard input.
.IP
The \fI<attr>\fB=\fI<value>\fR form is used to give selected attributes
of the inserted records constant values.
Without this, unspecified attributes will have null values.
.IP
The \fI<attr>\fR\fB as \fI<attr>\fR form is used to rename attributes when
input is taken from another table using the \fIfrom\fR option.
.IP
The special ``all'' attribute can be used to refer to all attributes.
If the -z option as been given then any zero-width attributes will
not be included.
In addition, ``all'' can be used with the ``:nodisplay=<attrlist>'' modifier
to indicate all attributes except for the comma separated attribute list
(e.g., all:nodisplay=seqno).
No space is allowed between the ``all'' and the ``:nodisplay=<attrlist>''.
.TP
.BI into\fR|\fPonto " <table1>\fR[\fP" = <alt_table>\fR]\fP
The table where the records are to be inserted; it will be created if it
does not exist, and a description file exists or the \fI-c\fR option was given.
.IP
If ``into'' is specified, the table must not previously exist.
.IP
By default, the descriptor file for the table is named \f(CWD\fI<table1>\fR.
A descriptor file from another table can be specified by adding
``=\fI<alt_table>\fR'' after the table name.
The alternate descriptor file will be \f(CWD\fI<alt_table>\fR.
.TP
.BI from " <table2>\fR[\fP" = <alt_table>\fR]\fP
The table where records are to be read from.
The \fIas\fR parameter can also be used to rename the attributes in
.IR <table2> .
A table named ``\-'' refers to standard input.
.IP
If no attribute names are given on the command line, \fBninsert\fR
will match the attributes from the source table to the destination
table.
Any missing attributes will be blank in the new records.
If any attribute names are given on the command line, then if any
of the named attributes are missing in the source table, an error
message will be printed and no insertions will be done.
.IP
By default, the descriptor file for the table is named \f(CWD\fI<table2>\fR.
A descriptor file from another table can be specified by adding
``=\fI<alt_table>\fR'' after the table name.
The alternate descriptor file will be \f(CWD\fI<alt_table>\fR.
.SH ENVIRONMENT         \" optional
.so Environ.u.h
.so Envunpack.u.h
.SH EXIT CODES          \" strongly recommended
.so Exit.codes.h
.TP
4
The required number of records specified with the
.B -r
option were not available in the input.
.SH RESTRICTIONS        \" optional - includes caveats and bugs
There is no maximum record size limit, but each individual
attribute is limited to the size of an I/O block: 10K on mainframes,
6K on Vaxen and other mid-size machines, and 4K on other smaller machines.
.P
There is a limit on the number of attributes each table can have.
Currently, this is 300 on mainframes and 100 on all other machines.
.SH EXAMPLES            \" strongly recommended
Insert owner information records into an ownership data base
(appending them), prompting the user for input:
.PP
.RS
.ft CW
ninsert -p onto owninfo
.ft R
.RE
.PP
Insert owner information records into an ownership data base
(which must not previously exist), without prompting the user for input:
.PP
.RS
.ft CW
ninsert into owninfo
.ft R
.RE
.PP
Add owners of a particular file into the \fIowner\fR relation.
The user will only supply the owners' logins and ownership types.
The file and loadline are given constant values on the command line; all
other fields will be null.
.PP
.RS
.ft CW
ninsert -p login owntype file=dir/filename \e
.br
    ldline=%all onto owner
.ft R
.RE
.PP
Add people to the
.I owninfo
relation from the \fI/etc/passwd\fR file.
Here we assume that the department number attribute in
.I /etc/passwd
is called ``deptno'', but in
.I owninfo
it is called ``dept''.
.PP
.nf
.RS
.ft CW
ninsert -p login name deptno as dept onto owninfo	\e
	from /etc/passwd
.ft R
.RE
.fi
.PP
Add all records from an input table onto the output table
along with the input table record number which will
replace ``id'' and include the output table record number
as ``seqno'' (sequence number) in the records being
inserted.
.PP
.nf
.RS
.ft CW
ninsert REC# as seqno rec# as id all:nodisplay=id	\e
	onto output_table from input_table
.ft R
.RE
.fi
.SH FILES               \" optional 
.so Files.u.h
.SH SEE ALSO            \" strongly recommended
insert(UNITY), nalter(1), ndelete(1), nuedit(1), retrieve(1),
setunpackenv(3), ucheck(UNITY), and uedit(UNITY).
