.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH descriptor 4 "" "New UNITY"
.SH NAME                \" required
descriptor \- File containing attribute information for a UNITY relation.
.SH SYNOPSIS            \" required
.nf
\fIattrname\fP    [\fBt\fIdelim\fR|\fBw\fInumber\fR    [\fBl\fP|\fBr\fP|\fBc\fInumber\fR    [\fIfriendly_name\fR]]]
.fi
.SH DESCRIPTION         \" required - 1/2 page in length
The descriptor file lists the attributes within a corresponding UNITY relation,
along with how the attribute is recognized (by a terminating character or
a fixed width), the justification and print width,
and a user-friendly name describing the attribute.
One attribute's information is listed per line.
The order the attributes appear in the descriptor
file is the order they must appear
in the UNITY relation.
.P
There are some significant differences between the requirements
on New UNITY descriptor files and previous UNITY descriptor files.
The New UNITY implementation will work with any previous version
descriptor file, but not vice versa.
.P
A descriptor file is always named \f(CWD\fIrelname\fR, where
.I relname
is the file name of the UNITY relation it describes.
The routine
.BR getrelinfo (3)
is used to look up and read the descriptor information for relation.
.P
In the New UNITY implementation, the descriptor file for a relation
is located by looking in the directories listed in the
UNITYDFILES environment variable.
If this variable is not set or the descriptor file is not located there,
the current directory is searched.
Failing this, the directory where the relation is located is
searched.
Finally, the data table itself is searched for the description.
(More on this below.)
The default search sequence/order of first checking UNITYDFILES,
then the current directory, and finally the data directory
before checking the data file itself can be modifed via the
UNITYDSEACH environment variable.
This method of searching gives applications exact control over
where descriptor files will be located.
Details on using UNITYDSEARCH are covered in the ENVIRONMENT Section.
.P
In previous UNITY implementations,
the current directory was searched first for the descriptor file,
followed by the relation's directory.
Finally, UNITYDFILES was used.
Additionally, some commands only allowed one directory in this
variable, while others did not use UNITYDFILES at all.
.P
In the New UNITY implementation,
all information in the descriptor file is optional
except the attribute name itself (for obvious reasons).
Each piece of information is separated from the next by a single tab.
Spaces on either side of the tab are ignored for backward
compatability with previous UNITY implementations.
Additionally, in previous UNITY implementations, the
termination or width information is required, not optional.
.P
A table description can be place directly in the data itself.
The first line of the table must be exactly ``%description''.
After this line comes the table description as it
would appear in a description file, including comments.
A line containing only ``%enddescription'' signals the
end of the description and the start of the data.
.P
The data table is the lowest priority for locating the description, so
this description can still be over-ridden.
If it is overridden any description in the data file is ignored
and not treated as data.
\fINote, however, that normal UNITY commands will not ignore
the description, but will treat it like data.\fR
.SH FILE CONTENTS
Each part of an attribute description is explained below.
The New UNITY requirements are listed first, followed by any
differences with previous versions:
.TP
.I attrname
The name of the attribute.
The name must start with an alphabetic character or an underscore (``_''),
followed by alphanumeric characters or underscore.
The name may be up to 31 characters long; longer names
will be truncated.
.IP
In previous UNITY implementations, the attribtue name
cuold only be 14 characters long and could not contain
underscores.
.TP
\fBt\fIdelim\fR|\fBw\fInumber\fR
The termination character (delimitor) or width of the attribute.
If the character is ``t'', the attribute is a terminated
(i.e., delimited) attribute.
The attribute value continues until the
.I delim
character is seen in the relation.
However, if a new-line is seen in the input before
.IR delim ,
the current record is ignored and
the record parsing starts over after the new-line.
.IP
There are several special strings allowed as delimiters:
.RS .5i
.TP 4
.B \en
The delimiter is a new-line character.
.TP 4
.B \et
The delimiter is a (horizontal) tab.
.TP 4
.B \ea
The delimiter is an alert (bell) character.
.TP 4
.B \eb
The delimiter is a backspace character.
.TP 4
.B \ef
The delimiter is a formfeed character.
.TP 4
.B \er
The delimiter is a carrige return character.
.TP 4
.B \ev
The delimiter is a vertical tab character.
.TP 4
.B \e\e
The delimiter is a backslash character.
In this case, no escaping of the field termination
(backslash) character is supported since that would
make record parsing ambiguous.
.TP 4
.B \eooo
The delimiter is the character represented by the given octal number.
.TP 4
.BI \e c
The character
.I c
is the delimiter.
.RE
.IP
If the last attribute in a relation has a delimiter, the delimiter must be
a new-line, i.e., the termination code must be ``\f(CWt\en\fP''.
.IP
If a backslash is encounterd in a terminated attribute value,
followed by the attribute delimiter, the
delimiter is taken as part of the attribute value and the
backslash is removed (except when backslash is the attribute delimiter).
This allows the attribute delimiter to be embedded in the attribute
value.
.IP
If the character is ``w'', the attribute is a fixed width string.
Exactly
.I number
characters are read from the relation to form this attribute's value.
However, the attribute value must not cross a new-line unless
the attribute value contains a null ('\e0') character (anywhere)
before the new-line ('\en') character.
No escaping of characters is supported.
It should be noted that
New Unity does not provide any support to update or write new
tuples (records) that contain binary data although Old (original)
Unity commands do support the both reading and writing fixed
width attribute values that contain binary data (null characters).
.IP
If no termination code is given for an attribute
(i.e., the line ends directly after the attribute name),
the attribute is assumed to be terminated by a colon (:) delimiter.
Previous UNITY implementation require the termination code to
be present.
.TP
\fBl\fP|\fBr\fP|\fBc\fInumber\fR
The justification and print width of the attribute.
This information tells formatting programs such as
.BR prtable (1)
and
.BR uprint (UNITY)
how to justify the attribute's values within a column
and to allow
.I number
characters in each column.
The recognized justification codes are:
.P
.RS .5i
.TP 4
.B c
Center the attribute value in the column.
.TP 4
.B l
Left justify the attribute value in the column.
.TP 4
.B r
Right justify the attribute value in the column.
.RE
.IP
If
.I number
is zero or the justification character is upper case (\fBC\fR, \fBL\fR, or \fBR\fR),
the attribute will not be projected by
.BR retrieve (1)
and
.BR fmkquery (3)
unless the attribute is explicitly named or the special ``all''
attribute is given.
This is used for hiding attributes that are reserved for future use.
This feature is not provided (or only part of it) in previous UNITY implementations.
.IP
If no justification code is present, the attribute value
will be left justified in a 12-character column.
.TP
.I friendly_name
The user-friendly name of an attribute.
Because of the limited length of attribute names,
they are often abbreviations of the real content of an attribute.
The
.I friendly_name
is a more verbose name that can be used in formatting the
relation to make the attribute's content more clear.
The
.I friendly_name
contains all characters up to a new-line.
It may contain any type or number of characters.
If no friendly name is present the attribute name itself is
used when labeling the attribute.
.P
Any line in the description starting with a pound sign (#)
as the first character
will be treated as a comment and ignored.
This is consistent with normal UNITY commands.
.SH ENVIRONMENT         \" optional
.TP
UNITYDFILES
A colon-separated list of directories to search for
relation descriptor files.
If this is not specified, or if a needed file is not found
in one of the directories, the current directory is
searched.
If that fails, the directory where the associated relation is located
(based on the relation path) is searched.
.IP
Some previous UNITY implementations only accept one
directory in this variable.
Also, they use it only after looking in the current
directory and in the relation's directory.
Some UNITY commands do not use this variable at all.
.TP
UNITYDSEARCH
Any combination of lower or upper case 'c', 'd', and/or 'u' characters
which stand for "current directory", "data directory", and "UNITYDFILES",
respectively.  If UNITYDSEARCH is not set, then "ucd" is used as the
default search order for locating a descriptor file before checking
the actual data file itself.
Note that preference can be given to search the data directory first
(via the command line) when getrelinfo(3) is called with an alternate
table name that has a "/./" (full) or "././" (relative) path prefix or
when the alternate description and data file (table) names are the same.
.SH RESTRICTIONS        \" optional - includes caveats and bugs
There is a limit on the number of attributes each table can have.
Currently, this is 300.
.P
There is no maximum record size limit,
but each individual attribute is limited to the size of
an I/O block: 10K on mainframes, 6K on Vaxen and other
mid-size machines, and 4K on other smaller machines.
.P
Old UNITY commands have a maximum record size of 4K.
.SH EXAMPLE             \" strongly recommended
Below is the descriptor file for the
.I /etc/passwd
file.
Notice that a new-line delimiter is always given on the last attribute
(unless it is a fixed width attribute).
.P
.RS .5i
.nf
\f(CW#
# Unity description file for /etc/passwd
#
login	t:	8l	Login Name
passwd	t:	14l	Password
uid	t:	5r	User-Id
gid	t:	5r	Group-Id
dept	t-	5l	Department
name	t(	20l	User's Name
acct	t)	4l	Account Number
bin	t:	5l	Output Bin
home	t:	15l	Home Directory
shell	t\en	12l	Login Shell\fP
.fi
.RE
.P
.SH FILES               \" optional
.TP
\fBD\fI<relname>\fR
\- Descriptor file for relations named \fI<relname>\fR.
It lists the relation's
attributes, their delimiting-character or width,
printing information, and user-friendly names.
.TP
\fBL\fI<relname>\fR
\- Lock file for relations named \fI<relname>\fR on Sun
machines and Intel machines running SVR4.
Other machines use a lock file in /tmp based on the
inode number, but this doesn't work for NFS-mounted
relations, or on machines with large inode numbers.
.SH SEE ALSO            \" strongly recommended
getrelinfo(3).
