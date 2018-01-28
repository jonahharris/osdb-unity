.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH set_attralloc 3 "" "New UNITY"
.SH NAME                \" required
set_attralloc \- Set maximum tuple size in application.
.SH SYNOPSIS            \" required
.nf
\fBint
set_attralloc( maxattrcnt )
int maxattrcnt;\fP
.SH DESCRIPTION         \" required - 1/2 page in length
The
.BR set_attralloc ()
routine establishes the maximum number of attributes any
relation used in an application will use.
This routine does not affect the functionality of query evaluation,
but instead is a way of optimizing memory usage.
All tuple structures allocated during query evaluation have space for the
same number of attributes.
By default, this is \f(CWMAXATT\fP attributes (defined in
.I urelation.h
as 300) so that any relation can be used.
However, most relations have a lot less than 300 attributes.
This can be a significant waste of space.
.P
This routine may be called as often as needed as long as no
queries are still outstanding at the time of the call, i.e.,
all previously evaluated queries must be freed using
.BR freeresult (3).
.P
.so Compile.h
.SH PARAMETERS          \" required - if command takes parameters - alphabetical order
.TP
.I maxattrcnt
The maximum number of attributes any relation used in a subsequent query
will use.
If this number is less than or equal to zero, or greater than 300,
the maximum will be set to 300 instead.
.IP
If any relation passed to
.BR queryeval (3)
has more attributes than
.IR maxattrcnt ,
.BR queryeval (3)
will fail.
.SH EXIT CODES          \" strongly recommended
True (non-zero) is returned on success.
False (zero) is returned if there is still an outstanding query.
.SH RESTRICTIONS        \" optional - includes caveats and bugs
The maximum number of attributes for any tuple is \f(CWMAXATT\fP.
This is currently defined in
.I urelation.h
as 300 for main frames and 100 for other machines.
.P
This routine can only be called while no queries are being
evaluated or used, i.e.,
all previously evaluated queries must be freed using
.BR freeresult (3).
(See the
.BR nexttuple (3)
manual page.)
Within these limits, the routine may be called as often
as needed to tailor the tuple size.
.SH EXAMPLE             \" strongly recommended
Given a query, set the allocation
size to the largest of the relations used in the query.
.P
.RS .5i
.nf
\f(CWstruct urelation *rellist;
int relcnt, maxatt, i;
 . . .
maxatt = 0;
for( i = 0; i < relcnt; i++ ) {
	if ( rellist[i].attrcnt > maxatt )
		maxatt = rellist[i].attrcnt;
}
set_attralloc( maxatt );
 . . .
queryeval( . . . );\fP
.fi
.RE
.SH SEE ALSO            \" strongly recommended
cmpquery(1), nexttuple(3) and queryeval(3).
