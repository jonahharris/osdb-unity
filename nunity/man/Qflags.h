.\"
.\" Copyright (C) 2002 by Lucent Technologies
.\"
.TP
.I flags
Alter the default operation of the query.
Any combination of the following flags may be OR'ed together.
.IP
.B Q_SORT
\- Sort the output of the query.
By default the output of a query is not sorted,
unless one or more sorted attributes are given.
This flag dictates that the output should be sorted,
regardless or whether sorted attribute were given or not.
Normally attributes are sorted in the order given
and the attribute values are put in ascending order.
Modifiers may be attached to the sorted attribute
names change the sort criteria.
See the
.BR retrieve (1)
manual page for more details about attribute modifiers.
.IP
.B Q_UNIQUE
\- Each projected tuple will be unique.
By default, duplicate tuples may be retrieved by the query.
This flag turns on checking to make sure each
combination of projected attributes is unique.
Normally, a character-by-character comparison is used
to compare attribute values for uniqueness.
Modifiers may be attached to the projected attribute
names to change the type of comparison used to determine
uniqueness, such as a numeric comparison.
See the
.BR retrieve (1)
manual page for more details about attribute modifiers.
.IP
.B Q_FRIENDLY
\- The friendly names (if available) are used as the print names
for projected attributes, rather than the attribute names
themselves.
.IP
.B Q_NOEXPAND
\- Do not expand any instances of the ``all'' attribute.
If this flag is present,
any references to the ``all'' attribute will
.I not
be expanded to all attributes in the given relation.
The query will also be marked as un-initialized,
whether it contains references to the ``all''
attribute or not.
If
.BR queryeval (3)
called with the un-initialized query, it will always fail.
The routine
.BR exprojlist (3)
can be called to expand the attributes and finish initializing
the query.
This flag should only be used when this expansion needs to
be delayed for some reason (e.g., when compiling a query into C code as
.BR cmpquery (1)
does).
.IP
.B Q_NOCASECMP
\- All string and regular expression comparisons in the
where-clause should ignore the case of letters.
