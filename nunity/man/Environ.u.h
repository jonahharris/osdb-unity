.\"
.\" Copyright (C) 2002 by Lucent Technologies
.\"
.TP 10
UNITYDFILES
A colon-separated list of directories to search for
table descriptor files.
If this is not specified, or if a needed file is not found
in one of the directories, the current directory is
searched.
If that fails, the directory where the associated table is located
(based on the table name on the command line) is
searched.
.IP
NOTE: UNITY commands in \fIexptools\fR only accept one
directory in this variable.
Also, they use it only after looking in the current
directory and in the table's directory; some commands do
not use it at all.
.TP
UNITYDSEARCH
Any combination of lower or upper case 'c', 'd', and/or 'u' characters
which stand for "current directory", "data directory", and "UNITYDFILES",
respectively.  If UNITYDSEARCH is not set, then "ucd" is used as the
default search order for locating a descriptor file before checking
the actual data file itself.
Note that preference can be given to search the data directory first
(via the command line) when an alternate table name is given
that has a "/./" (full) or "././" (relative) path prefix or
when the alternate description and data file (table) names are the same.
