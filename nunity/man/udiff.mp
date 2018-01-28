.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH udiff 1 "" "New UNITY"
.SA 1
.SH NAME
udiff \- Compare the tuples in two UNITY tables
.SH SYNOPSIS
.HP
.ll +0.5i
.B udiff
[-e[1|2]]
[-dqsvV]
[-Q]
\fI<key_attr1>\fR [to \fI<key_attr2>\fR] ...
.br
.ll -0.5i
[show \fI<attr1>\fR [to \fI<attr2>\fR] ...]
.br
[compare [only] \fI<attr1>\fR [to \fI<attr2>\fR] ...]
.br
[ignore \fI<attr>\fR ...]
.br
in \fI<table1>\fR[=\fI<alt_table>\fR] \fI<table2>\fR[=\fI<alt_table>\fR]
.br
[where \fI<where-clause>\fR]
.ad
.SH DESCRIPTION
.B udiff
compares the attribute values for tuples in two unity files based on the
given key attributes specified on the command line.
Two kinds of mis-matches are reported:
In the first type, the key attributes match between a tuple
in each table, but one or more non-key attributes do not
match.
In this case the record number and key attributes are displayed along with
the non-matching non-key attributes.
Other attributes may also be displayed by specifying them
in the ``show'' clause.
The second type of mis-match is an extra tuple in one of
the tables,
i.e., a tuple that does not match the key attributes of
any tuple in the other table.
In this case the record number and all attributes are shown
for the extra tuple.
Matching tuples are not expected to be in the same position
in both tables unless the ``rec#'' attribute is specified as
one of the key attributes.
All comparisons are done character-by-character; numeric, date,
and regular expression comparisons are not supported.
.P
After all mis-matching tuples are displayed a summary is
printed giving the number of matching tuples, the number of
extra tuples in each table, and the number of tuples
with non-key attribute mis-matches.
This summary may be suppressed with the
.B -q
flag.
.P
Unlike other UNITY commands, when locating the descriptor file
for each table, the table the directory resides in has
precedence over UNITYDFILES and the current directory.
This is done to allow different descriptor files for the
same table name in different directories.
.P
The ``show'' clause specifies attributes to display for all
mis-matching tuples.
Normally, only the key attributes and any mis-matching attributes
are displayed.
.P
The ``compare'' clause specifies which attributes to compare.
Normally, all attributes with the same name in each table
are compared.
Non-matching attribute names are ignored.
This clause is used mainly to specify different attribute names to
compare in each table.
Unless the ``only'' keyword is also given, any non-specified
attributes with matching names are still compared.
.P
The ``ignore'' clause specifies attributes for which differences
are to be ignored.
.P
The ``where-clause'' restricts the compared tuples in \fIboth\fR
tables to only those matching the where-clause.
Normally, all tuples in both tables are compared.
.SH PARAMETERS
Below are the command-line flags used to change
.BR udiff 's
operation:
.TP
.B -d
Do not display tuples where the keys match, but non-key
attributes do not match.
Only extra tuples in one table or the other will be displayed.
.TP
\fB-e\fR[\fB1\fR|\fB2\fR]
Do not display extra tuples in the first table (\fB-e1\fR),
in the second table (\fB-e2\fR), or in either table (\fB-e\fR).
Only tuples where the keys match, but non-key
attributes do not match will be displayed.
.TP
.B -s
Don't sort the tables on the given keys before comparing the tuples;
assume the tables are already sorted properly.
This flag is provided as an optimization if the tables are
already sorted into ascending order on the key values.
.TP
.B -Q
Gracefully continue after encountering any tuple syntax errors instead of quiting.
See the retrieve(1) manual page for more information.
.TP
.B -q
Operate in quiet mode.
Normally,
.B udiff
prints the number of matching and mis-matching tuples on standard error.
If this option is given, this message is suppressed.
.TP
.B -v
Use the verbose or ``user-friendly'' names
(if available) from the descriptor file for
labeling attributes in the output.
Normally, the attribute names are used for labels.
(See the
.BR table (UNITY)
or
.BR descriptor (4)
manual page for more details on the user-friendly names.)
.TP
.B -V
Print the version of the command and exit.
No comparisons are done.
.P
Below are the command-line clauses which describe
how to compare the two tables.
.br
.ne 6
.TP
\fI<key_attr1>\fR [to \fI<key_attr2>\fR] ...
Each named \fI<key_attr1>\fR is used to find matching
tuples in the first table given.
If \fI<key_attr2>\fR is also given, it is used as the
corresponding key attribute in the second table; otherwise,
\fI<key_attr1>\fR is used.
.IP
All attribute names should be given as an attribute name alone;
no table name or positional number should be prefixed to the name.
The ``\fIrec#\fR'' attribute can be used as a key;
however, the ``\fIseek#'' and``\fIall\fR'' attributes
cannot.
.TP
[show \fI<attr1>\fR [to \fI<attr2>\fR] ...]
In any mis-matching tuples, always display the given attributes.
Normally, only mis-matching attribute values are displayed.
.IP
All attribute names should be given as an attribute name alone;
no table name or positional number should be prefixed to the name.
The ``\fIrec#\fR'' attribute can be given here;
however, the ``\fIseek#'' and``\fIall\fR'' attributes
cannot.
.TP
[compare [only] \fI<attr1>\fR [to \fI<attr2>\fR] ...]
Compare the given non-key attributes in mis-matching tuples.
This clause is not normally needed unless compared attributes
have different names in each table or only certain attributes
are to be compared.
Normally, all non-key attributes with the same name in each table
are compared.
Non-matching attribute names are ignored.
This clause allows a different attribute name to be used in
the first table than the second.
If the ``only'' keyword is also given only the specified attributes
are compared.
Normally, all matching attribute names not given in this clause
are also compared.
.IP
All attribute names should be given as an attribute name alone;
no table name or positional number should be prefixed to the name.
The ``\fIrec#\fR'' attribute can also be given here;
however, the ``\fIseek#\fR'' and``\fIall\fR'' attributes
cannot.
.TP
[ignore \fI<attr>\fR ...]
Ignore differences in the given attributes when comparing tuples.
Normally, all non-key attributes with the same name in each table
are compared.
This clause allows a way of specifying attributes for which you don't
care about differences.
The attribute names given should be common between both compared
tables.
(Non-common attribute names are ignored automatically.)
Any unrecognized attributes in this list are ignored.
The ``compare'' clause overrides this clause;
i.e., any attribute mentioned
in both the compare and ignore clauses will still be compared.
.TP
in \fI<table1>\fR[=\fI<alt_table>\fR] \fI<table2>\fR[=\fI<alt_table>\fR]
The two tables to be compared.
The tables should be given as full or partial path names.
A table named ``\-'' refers to standard input; this can only appear once.
However, unlike other New Unity commands, if standard input is given
as a table name, the descriptor information can not be included with
the table itself, i.e., an \fI<alt_table>\fR must be given for
standard input.
.IP
If \fI<table2>\fR is a directory, then the file name from
\fI<table1>\fR is appended to it and used as the second table name.
.IP
By default, the descriptor file for a table is named \f(CWD\fI<table>\fR.
A descriptor file from another table can be specified by adding
``=\fI<alt_table>\fR'' after the table name.
The alternate descriptor file will be \f(CWD\fI<alt_table>\fR.
.TP
[where \fI<where-clause>\fR]
Specify a where-clause that will be used to select tuples out of
\fIboth\fR tables.
See the \fBretrieve\fR(1) manual page for more information about
the where-clause.
.IP
In the current implementation, no joins are available in the
where-clause.
Furthermore, all attributes referenced in the where-clause
must have a common name in both tables, since the same exact
where-clause is given to each query on the individual tables.
(If any needed attributes are not common, the only recourse is
to create new alternate descriptor files for the tables
in which the attributes do have the same name.)
.SH ENVIRONMENT
.so Environ.u.h
.so Envunpack.u.h
.SH EXIT CODES          \" strongly recommended
.so Exit.codes.h
.SH RESTRICTIONS        \" optional - includes caveats and bugs
There is no maximum tuple size limit, but each individual
attribute is limited to the size of an I/O block: 10K on mainframes,
6K on Vaxen and other mid-size machines, and 4K on other smaller machines.
.P
There is a limit on the number of attributes each table can have.
Currently, this is 300 on mainframes and 100 on all other machines.
.br
.ne 6
.SH EXAMPLES
Compare the table ``employee'' in the ``old'' and ``new''
directories.
The key attribute is the employee id, ``empid'', in both tables.
Compare the ``sal'' attribute in the old table to the ``salary''
attribute in the new table.
Always show the employee name for any mis-matching tuples.
.P
.RS .5i
.nf
\f(CWudiff empid show name compare sal to salary \e
	in old/employee new\fR
.fi
.P
Compare the table ``employee'' in the ``old'' and ``new''
directories, again.
This time, compare only those tuples where the employee's
title is ``manager''.
(This assumes that ``title'' is a common attribute in both tables.)
.P
.RS .5i
.nf
\f(CWudiff empid show name compare sal to salary \e
	in old/employee new \e
	where title = manager\fR
.fi
.RE
.SH FILES
.so Files.u.h
.SH "SEE ALSO"
descriptor(4), diff(1), retrieve(1), setunpackenv(3), and ucheck(UNITY).
