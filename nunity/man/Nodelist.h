.\"
.\" Copyright (C) 2002 by Lucent Technologies
.\"
.TP
.I nodelist
The array of query nodes (i.e., relations) used in the query.
A query node is an instance of a relation; the same relation may be referenced
multiple times in
.IR nodelist .
These structures should be initialized by setting the
\f(CWrel\fP element in each \f(CWqnode\fP structure to point
to the corresponding \f(CWurelation\fP structure obtained
from
.BR getrelinfo (3),
as in
.P
.RS 1i
.nf
\f(CWrelptr = getrelinfo( relpath[i], NULL, FALSE );
nodelist[i].rel = relptr;\fP
.fi
.RE
.TP
.I nodecnt
The number of query node structures in the
.I nodelist
array.
At most eight relations may be queried at any one time.
