.\"
.\" Copyright (C) 2002 by Lucent Technologies
.\"
Specify a subset of records to be considered.
If no \fIwhere-clause\fR is given, all records will be considered.
.IP
The \fIwhere-clause\fR is made up of expressions comparing the value
of an attribute to a constant or to the value of another attribute.
The expressions can be combined using boolean operators and parenthesis.
.IP
The comparison operators are:
.BR lt " (less than), "
.BR le " (less than or equal to), "
.BR gt " (greater than), "
.BR ge " (greater than or equal to), "
.BR eq " (equal to), and "
.BR ne " (not equal to).  "
If they appear as is, they are numeric comparisons.
They can also have the following prefixes:
.BR l " (lexical comparison), "
.BR c " (lexical comparison, but ignore the case of letters), "
.BR n " (numeric comparison), "
.BR r " (regular expression comparison \- \fBeq\fP and \fBne\fP only), "
.BR d " (date and time comparison), and "
.BR f " (field-to-field comparison \- may also be combined with \fBl\fP, \fBc\fR, \fBn\fR, and \fBd\fP prefixes).  "
.IP
The boolean operations are:
.BR ! " (negation), "
.BR and " (logical conjunction), "
.BR or " (logical disjunction), "
.BR ( " and " ) " (grouping), and "
.BR else " (selection precedence).  "
.IP
Attributes used with any of the six basic numeric comparison operators
can be converted from various integer input bases before being converted
to a double float value by adding a ":modifier" string
immediately after the attribute name.
The valid numeric attribute modifiers are
\fIb\fPinary, \fIh\fPexadecimal,
\fIo\fPctal, \fIn\fPumeric, and \fIlen\fPght.
The generic "numeric" modifier is there for completeness
and does not change the normal operation of converting
the input string value directly to a double float value.
The "length" modifier uses strlen(3C) to get the string length
of the attribute value which is then used in the comparison.
The other modifiers use strtoul(3C) to convert the input value
from the requested input base before being type-cast to
integer and then converted to a double float value for comparison.
The (numeric) attribute modifiers cannot be used with the set (in) operator
and they cannot be used with the special "rec#" or "seek#" attributes.
.IP
Note that an attribute value in the where-clause
that begins will a leading "0x" or "0X" will be taken
as hexadecimal input when used in a basic numeric comparison.
This does not include set (in) comparisons.
.IP
See the
.BR retrieve (1)
manual page for more information.
