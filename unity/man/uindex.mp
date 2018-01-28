.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH uindex "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
uindex \- produce an index on an attribute for a UNITY table
.SH SYNOPSIS
\fIuindex\fP [-Itable] aname in table
.SH DESCRIPTION
After a table is defined and loaded, a user can
immediately retrieve and analyze the data.
The user has the option to declare a particular attribute to be
indexed by using the \fIuindex\fP command.
Indexes can be used in UNITY to reduce processing time for the
\fIuselect\fP, \fIujoin\fP, \fIouterjoin\fP, and \fIsetdiff\fP
commands when searching
for records that have certain values for an indexed attribute.
An index can be created for any attribute, aname, 
in the description file for the specified table using the \fIuindex\fP command.
The index is built in alphanumeric order on the values of the attribute
being indexed (thus indexes cannot be used for queries involving
numeric comparisons).
The index files are then used to find applicable records when selecting
records using the \fIuselect\fP command, or
joining or finding the set difference of two tables on attributes
for which one has an existing index.
Indexes are automatically updated when they are used so the user
need not worry that the index being used is obsoleted by an updated
table (this is discussed in further detail below).
.P
If the -I option is used, the table name specified is used to determine
an alternate description file to be used in reading the input table.
If the name is the same as the name of the data file or if it contains
a "/./" (full) or "././" (relative) path prefix, the given data (description)
directory will be searched first for the description file.
.P
Reading of the standard input (input table name "-") is not supported.
.SH EXAMPLE
If, for example, the /etc/passwd file is frequently searched by login value
then the user can execute the command
.sp
index login in /etc/passwd
.sp
to build an index for the attribute login
(assuming an appropriate description file for the /etc/passwd file
exists in the present working directory or in /etc).
.P
The remainder of this manual page describes in detail the creation and
use of the index files and may be omitted by the casual user.
.br
.ne 4
.SH "DESCRIPTION OF INDEX UPDATE"
.P
One problem associated with the use of index files is that of updating
them when the table changes.
Any design of indexes that handles the automatic update problem
must also be capable of dealing with a multi-user environment
in which potentially many index files exist in different directories.
(i.e., it may be possible to update the index files in the current directory
associated with an updated table but not be able to update
obsolete copies in other directories).
The solution chosen was to make the routines
that access the index files
smart enough to know when to update the index files before using them.
.P
These programs look at the modification dates of the index files and
the table; if the table has been updated since
the index files were created, then the index files are re-created
using the \fIuindex\fP command.
Only an index needed for a query is updated.
This has two advantages over other techniques.
First, this approach handles the problem of updating multiple copies of the
index files in many different directories.
Second, there is less overhead with this approach since the index
files are updated only when they are needed and may be updated only once even
though many table updates have occurred.
A user who does not rely on indexes incurs no extra overhead
from the index update code since index files are not updated
unless they exist originally.
.SH "DETAILED DESCRIPTION OF INDEX STRUCTURE"
An index may be created for any attribute in a UNITY table.  The use
of an index reduces the processing time during a \fIuselect\fP,
\fIujoin\fP or \fIsetdiff\fP command.
The time is reduced by not requiring that every record in the
table be read but instead using the index to directly access only the
desired records.
A UNITY index is composed of two files.
One file (hereafter referred to as the Bfile since its name is formed by the
concatenation B<table-name>.<attribute-name>) contains a B-tree
structure that is used to find a particular attribute value.
Along with the value is a pointer into the other file (hereafter referred
to as the Afile).
Starting at that position in the Afile is the number of records in the
table which have the attribute value found in the Bfile and the
corresponding pointers into the table (the accession list).
The pointers that are used are seek locations within the files.
.P
The index files are created using the \fIindex2\fP and \fIuindex\fP
programs.
The table name and the attribute name for which the index is to be
created are used by \fIindex2\fP.
The \fIindex2\fP program is general purpose program that
reads the table and writes the attribute value
(terminated with a '\\01') and the seek location (an 8-digit
decimal value) of the beginning of the record to the standard
output for each record (one line per record) in the table.
This program is also used by \fIasort\fP(UNITY).
.P
The UNIX \fIsort\fP(1) program sorts the output leaving
the output lines in attribute value order.
The attribute value is terminated with a '\\01' instead of a '\\0'
because if an attribute value is null, the output record
would begin with '\\0' and the sort(1) program silently throws these
records away.
.P
The \fIuindex\fP program executes a shell command,
where the \fIindex2\fP program pipes its output to \fIsort\fP,
and reads the input (using \fIpopen\fP(3)).
\fIuindex\fP creates the Afile from this sorted output by
reading lines and keeping track of whether the new value
is the same as the previous value read in.
When a new attribute value is read, the negative of the number
of records having the old value and the seek values for the start
of those records are written to the Afile.
.P
The key value and the seek offset where the list for that value
begins in the Afile then used to build the B-tree.
The tree is composed of "nodes" which have a header followed by a
list of attribute values and corresponding seek locations as
described below.
The header contains the level of the node in the tree, the seek location at
which the node is written into the Bfile, and a checksum value.
The level numbering is the reverse of normal tree level numbering with
all leaf nodes at level-0 to make numbering easier since the
tree is built bottom up.
The tree has a maximum of 13 levels (as determined by the
definition MAXLVL in \fIuindex.c\fP);
13 is the maximum number of levels needed to index 4096
values (the current maximum number of allowable index values)
all of which are the maximum length in value of 128 characters.
Lines are read from the input and written out to an in-core copy of a
level-0 node.
When the level-0 node is full (e.g., the attribute value/location
information runs over the node length), the node is completed and written
out to the Bfile.
Completion includes filling in the last entry of the node with
the maximum Ascii character as a node terminator and the seek
location of the last attribute for the node, and filling in
the header with the seek location at which this node will be written in
the Bfile and the checksum for the node.
Besides writing out the node to the Bfile, an entry containing
the largest key for this node and the seek location where this
node was written out in the Bfile is written to the next higher
level node.
As a higher level node is filled, the same procedure occurs
with an entry written to the next higher level node.
When the last input line has been handled, all partially filled nodes
are written out to the Bfile similarly (i.e., entries for
the largest key in a node are written to the next higher level node).
The highest level node that has entries is considered to be the root and
is written out as the first node in the Bfile (space is allocated for
it before the first level-0 node is written out).
.P
Several C-functions are involved in reading the index files.  \fIbopen()\fP
opens the Bfile (the B-tree structure) and reads the root node,
makes a copy of the node for future searches, and initializes two read
position pointers.  \fIndread()\fP reads nodes and checks that
the seek value stored in the header of the node matches the seek
location of the node.
\fIndcheck()\fP validates the checksum stored in the node.
\fIrdindexed()\fP finds a particular key value.  The root is searched
(using the \fIscan()\fP function) for the first entry whose attribute value is
equal to or higher than the value being searched for.  The associated
seek value is used as a seek location in the Bfile from which the next
node is read.  This process continues until a level-0 node has been read.
This node is scanned for the specified key.  \fIrdindexed()\fP returns
whether the key is found, not found, or if the node end is found, as
well as pointers to the key value and associated seek location of the key found.
\fIrdnext()\fP gets the next index value, automatically handling
node end and finding the next level-0 node (via a call to \fIrdindexed()\fP
with a key value equal to the current key appended with a '\\01'
to get the next possible key value).  The seek value returned with the key
value by \fIrdindexed()\fP or \fIrdnext()\fP is used to do a seek into the
Afile.
Starting at that location, the number of records and their starting locations
are read from the Afile.
Records are accessed directly from the table using these
seek locations.
.SH ENVIRONMENT
.TP
.B UNITYDFILES
A colon-separated list of directories to search for
table description files after checking the
current directory and the data file directory.
.TP
.B UNITYDSEARCH
Any combination of lower or upper case 'c', 'd', and/or 'u' characters
which stand for "current directory", "data directory", and "UNITYDFILES",
respectively.  If UNITYDSEARCH is not set, then "cdu" is used as the
default search order for locating a description file.
Note that preference can be given to search the data directory first
(via the command line) when an alternate table name is given
that has a "/./" (full) or "././" (relative) path prefix or
when the alternate description and data file (table) names are the same.
.br
.ne 5
.SH FILES
.nf
unity/uindex.c, index2.c, zindex.c, zindex2.c - the C source files
bin/uindex, index2 - the executable programs
A<table>.<attribute>, B<table>.<attribute> - the index files
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY),
\fIsort\fP(1), \fIuselect\fP(UNITY), \fIujoin\fP(UNITY), \fIsetdiff\fP(UNITY),
\fIouterjoin\fP(UNITY)
