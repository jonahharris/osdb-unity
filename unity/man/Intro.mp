.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH Introduction UNITY "" "UNITY User's Manual"
.SA 1
.SH NAME
Intro \- introduction to UNITY commands and application programs
.SH DESCRIPTION
.P
UNITY is a small relational data base management system (DBMS)
that runs under the UNIX(TM) operating system.
UNITY is very user-oriented and provides many powerful command capabilities.
Emphasis is placed on ease of use and allowing a multi-user
environment while keeping UNITY small and simple to understand and maintain.
.P
Since UNITY is based on the relational data model
it benefits from an easily understood data structure.
Data is thought of in terms of \fItables\fP (files)
of \fItuples\fP (records) consisting of several \fIattributes\fP (fields).
With this model, a \fIdatabase\fP is a collection of \fItables\fP.
UNITY does not currently support the concept of the \fIkey\fP
(i.e., an attribute which is unique for every record in the data file)
in the relational model so that identical tuples are allowed in a table.
However, attributes may be specified to be indexed to decrease access time.
In UNITY, attribute values are Ascii character strings that are
either fixed in length, or variable in length with a
specified terminator.
Description files, which provide an
interpretation for the records in the tables, must be defined by the user.
The command language is simple and yet powerful.
It is based on relational algebra (e.g., joining of tables, selection
and projection of attributes) implemented as
executable C-programs within the UNIX environment.
The commands allow the user ease in creation, maintenance and update,
and query of the tables.
These commands may be executed singly in sequence with output into a file
to serve as input to the next command, or in a procedure made up of
a sequence of commands to be executed as if a single
command had been given (shell procedure).
.P
UNITY does \fInot\fP currently provide a self-contained user language
or command interpretor.
Instead, it offers the full capability of the UNIX shell language to build up
complicated queries from simple individual commands (i.e., the UNITY
programs function as tools).  In effect, a new data type called TABLE
and a set of functions such as \fIuselect\fP and \fIujoin\fP have been
defined.
Each UNITY command is designed to be small and modular.
The UNITY software currently consists of about four dozen C-language
programs (about 15,000 lines of code).
Each command
is a stand-alone module and most of the commands
occupying less than 50K bytes of user memory
(with the exception of the \fIregjoin\fP, \fIumenu\fP and
\fIvalidate\fP programs).
.P
Users may write their own programs using the UNITY functions at
two levels.
The lower level UNITY functions
can be called from a user program to read, manipulate,
and write UNITY tables; these functions are described in \fIUintll\fP(UNITY).
The user also has access to the high-level functions which
execute actual UNITY commands.
The interface is argc\-argv format just as the arguments would
be passed to the unity main programs; these functions are described in
\fIUinthl\fP(UNITY).
All of the UNITY functions are available in the dblib.a archive file.
.SS "Operation Environment"
.P
UNITY was designed with the UNIX shell language as its user
interface (host language).
This architecture makes it easy to improve UNITY's capabilities
by adding additional modules
to provide new functions.
The shell provides another level of flexibility,
permitting complex query-transactions to be built up from elementary
commands, and allowing file transfers for processing UNITY queries in
a distributed environment.
.P
A simple query may be answered by executing one of the UNITY commands.
However, there are frequently queries that require a sequence of these
commands.
It is not convenient for each user to execute these commands one by
one, and the shell language provides the means to build a query or transaction
that is made up of several UNITY commands.
These transactions can be built in a "parametric"
way so that different values may be substituted into the same
sequence of commands.
Such parametric transactions become part of the UNITY command structure
and can easily be employed by untrained users.
.SS "Getting Started"
.P
The first step in setting up UNITY tables is to create the description
files using the \fImaketable\fP or \fItable\fP command.
Data may then be loaded into the tables using \fIuenter\fP
(\fIload\fP or \fIinsert\fP may also be used).
The tables may be displayed by using the \fIdisplay\fP and
\fIuprint\fP commands.
The \fIcompute\fP, \fIinsert\fP, \fIalter\fP, \fIudelete\fP,
\fInprintf\fP, \fIuenter\fP,
and \fIuedit\fP commands can be used to update the tables.
The records may be manipulated with the \fIuselect\fP, \fIujoin\fP,
\fIouterjoin\fP, \fIsetdiff\fP, \fIuformat\fP, \fIasort\fP, \fImerge\fP,
\fIunmerge\fP, and \fIuprintf\fP commands to create new tables.
The \fIvalidate\fP command is available for verifying that records
are valid against a pre-defined validation script for the table.
An interface to the Source Code Control System (SCCS) is provided
by \fIuadmin\fP, \fIudelta\fP, and \fIuget\fP.
Finally, \fIstat\fP, \fIdist\fP (\fItally\fP), and
\fIddist\fP (\fIdtally\fP) are available for data analysis.
Manual pages more fully describe each of the commands listed above.
.SS "UNITY Features"
The following are some of the features of UNITY:
.IP 1.
Mutual exclusion of update transactions to
maintain consistency during concurrent updates by multiple users;
.IP 2.
A powerful selection capability including the use of regular
expression pattern matching and arbitrary nesting of relational operators;
.IP 3.
Ability to create index files to reduce processing time and
automatic updating of index files;
.IP 4.
The \fIuprint\fP and \fIcatalog\fP programs to allow the user a flexible
way to display the records without having to know and use
awk(I) or other such languages;
.IP 5.
Interactive insertion and updating of tables;
.IP 6.
Programs to aid the user in reformatting and sorting tables;
.IP 7.
A validation module that allows the user flexibility in validating
input, updates, and currently existing tables;
.IP 8.
Ease in building UNITY and porting the code to other machines (it has been
installed and tested on many computers).
.SS "Building UNITY"
.P
When building UNITY for the first time on a machine, the UNITY
administrator must change present working directory to the directory
containing the UNITY source code and execute
.sp
.ce
nohup make &
.sp
This will run the build process in the background and
create the archive file (named "dblib.a"),
compile the functions and put the object files into the archive, and
compile/link the programs.
If this command is executed after UNITY is already built,
any object modules in the archive file that are outdated will be updated
and programs will be re-compiled/linked if necessary.
The command will print
.br
  `all' is up to date.
.br
if no re-compilation is necessary.
A single program can be updated by executing
.sp
.ce
make "program"
.sp
If the executable commands are to be moved into a "bin"
directory, and the archive and header files are to be moved into a
library directory, and the manual pages installed, the user should execute
.sp
.ce
make LIB=<library dir> BIN=<bin dir> MAN=<man dir> install
.sp
If LIB, BIN, and MAN are not set,
the make(1) program will use ../lib, ../bin, and ../man,
respectively (these can be changed in the Makefile).
The files are linked into the respective directories
(if the LIB, BIN, or MAN directories are not on the same file system,
the Makefile must be changed to do a copy instead of a link).
The \fIuman\fP command is updated to look the manual pages
in the MAN directory.
.P
When building UNITY on an versions of UNIX without
floating point (e.g., the AT&T 3B2/3B5), "-f" must be added to the
CFLAGS in the Makefile (to include the floating point software when
loading the executable programs).
When building UNITY on UNIX/370, "-b1,1" must be added to
the CFLAGS in the Makefile (to include additional base registers).
When building UNITY on a 16-bit machine
(e.g., the DEC PDP 11/70), IFLAG must be set to "-i"
so that when the \fIumenu\fP program is executed, the program text
and data areas will live in separate address spaces.
.P
On some versions of UNIX, libPW.a is not available.
For these versions, PW in the Makefile should be set to null
(by default, it is set to -lPW).
PW1 must be set to regcmp.o and PW2 must be set to regex.o
in the Makefile.
The source files for regcmp.c and regex.c are provided for those
systems that need it (the version in libPW.a is used otherwise).
.P
Some versions of \fImake\fP(1) will not interpret the Makefile
correctly.
Older version of \fImake\fP don't understand archive library dependencies
and some versions don't recognize macros added in later versions
of \fImake\fP (e.g., BSD does not recognize the $$@ dependency).
This will cause the software to build incorrectly or not to build at all.
A shell script, \fImakeunity\fP is provided to build the UNITY software.
It assumes that the software is to be installed.
CC, CFLAGS, IFLAG, LIB, MAN, BIN, PW, PW1, and PW2 are all shell
variables that can be changed within \fImakeunity\fP before executing
the command.  Note that executing this shell will always re-build
UNITY as if it had never been compiled before, even if only one
file has been updated (i.e., the shell does not look at modification
times to do only restricted re-building).
.SS "The Description File"
A description file is associated with each UNITY table which lists
the fields on each record, the field type (fixed width or terminator field)
and, optionally, print information and "user\-friendly" field names.
In some situations, it is necessary to allow users to specify their own
description by which records in the table are interpreted.
A simple example of a case where this is useful
is the specification of a date:
one user may want to access a date as three two-character fields (e.g., month,
date, and year) while another user may reference it as a single six-character
quantity.
In other situations, it is advantageous to have one single description file
that is accessed by all users (i.e., for a single, centrally-located table).
.P
When accessing a UNITY table, the default is for UNITY programs
to first look for the description file in the present working directory
(name composed of the simple file name of the table prefixed with a 'D').
If the file does not exist, then the program looks for a file with the
same name in the directory in which the table is located.
If the description file is not found, it will then look at the environment
variable UNITYDFILES to see if a colon-separated list of
directories is specified, one of which contains the description file.
Thus, all description files can be stored in one or more common directories
by use of the UNITYDFILES environment variable.
.P
The UNITYDSEARCH environment variable can be used to change the default
search order/sequence for finding the description file by setting it to
any combination of lower or upper case 'c', 'd', and/or 'u' characters
which stand for "current directory", "data directory", and
"UNITYDFILES", respectively.  If UNITYDSEARCH is not set,
then "cdu" is used as the default search order for locating a
description file.
.P
The "-Itable" option can be used to indicate that the
description file is to be obtained using the alternate
table name.
Preference can be given to search the data directory first
when the alternate table name
has a "/./" (full) or "././" (relative) path prefix
or when the alternate description and data file (table)
names are the same.
The same is also true for the "-Otable" option but only when it is
needed/used for reading a table or description file
(e.g., uformat).
.P
If the "-Itable" option has not been given and the
input table is being read from stardard input ("-"),
then most of the UNITY commands will now look the
description at the beginning of the data stream
between a pair of "%description" and "%enddescription"
lines the same way that NewUNITY commands work.
The obvious exceptions to this are the interactive
data input and the data validation commands that
require an existing description and/or any associated
validation files.
.P
WHEN DESCRIPTION FILES AND VALIDATION OBJECT FILES ARE
CREATED BY A UNITY COMMAND (E.G., \fIujoin\fP, \fIuselect\fP,
\fIvalidate\fP, ETC.),
THEY ARE CREATED IN THE PRESENT WORKING DIRECTORY.
Any "user\-friendly" names are LOST when a new description file is created
(when an "into" clause is specified)
by \fIuselect\fP when projection is done,
and by \fIujoin\fP, \fIouterjoin\fP, and \fIregjoin\fP.
.P
Description files that are created using the "-Otable"
option are normally created in the current working directory.
However, the user can indicate that the description is
to be created in the same directory as the data table
by giving the same table name in the "-Otable" option
as the name of the output data table (i.e., "into table"
or "onto table").
In addition, the output description file that is normally created
as a result of using an "into table" or "onto table" clause can be
supressed by giving the "-Otable" option without
an alternate table name (i.e., "-O") for most UNITY commands.
The exceptions are some of the commands that do projection.
Otherwise, any path prefix given in the "-Otable" option
is ignored and the description file is created in the
current working directory.
.P
Most UNITY commands now support a "-c" option that is
used to create the description information at the beginning
of the output data stream between a pair of "%description"
and "%enddescription" lines similar to what the NewUNITY
retrieve(1) command does.
However, this option is only valid when the output table
is being written to standard output (stdout) and when
the "-O" or "-Otable" option has not been given.
Otherwise, the "-c" option will be silently ignored.
.P
The \fImaketable\fP and \fItable\fP
commands are provided to create the original description file for each
table or the description file may be created using a text editor.
The format is given on the manual page for \fItable\fP.
Comment lines may be added to the description file; all lines beginning
with a pound sign (#) in column 1 are ignored.
.SS "Selection criteria - the where clause"
Many of the UNITY programs
allow the user to operate on a subset of the records in the
table based on selection criteria specified in a \fIwhere\fP clause.
The \fIwhere\fP clause is composed of the conjunction (keyword \fIand\fP) and
disjunction (keyword \fIor\fP) of conditions specified as a comparison between
a UNITY field and a user-specified value,
or another UNITY field in the same table.
.sp
There are six basic comparison operators as indicated by six suffixes
used for the comparison operators.
They are:
.sp
.ne 6
.nf
.in +1.0i
lt  - less than
le  - less than or equal to
eq  - equal to
ne  - not equal to
ge  - greater than or equal to
gt  - greater than
.in -1.0i
.fi
.sp
These may be prefixed to specialize the comparison as follows:
.IP none
If no prefix is given, then a numerical comparison is done.
This means that both values are converted to double float values
and compared.
Thus, values like 1.0 and 1 which would fail an equality test
based on a character-by-character comparison would match using
a numerical equality comparison (i.e., operator \fIeq\fP).
.sp
Attributes used in numeric comparisons can be converted
from various integer input bases before being converted
to a double float value by using a ":modifier" string
immediately after the attribute name.
The valid numeric attribute modifiers are
\fIb\fPinary, \fIh\fPexadecimal,
\fIo\fPctal, \fIn\fPumeric, and \fIlen\fPgth.
The generic "numeric" modifier is there for completeness
and does not change the normal operation of converting
the input string value directly to a double float value.
The "length" modifier uses strlen(3C) to get the string length
of the attribute value which is then used in the comparison.
The other modifiers use strtoul(3C) to convert the input value
from the requested input base before being type-cast to
integer and then converted to a double float value for comparison.
.sp
Note that an attribute value in the where-clause
that begins will a leading "0x" or "0X" will be taken
as hexadecimal input when used in a numeric comparison.
.IP n
If the operator is prefixed with an 'n', then a numerical comparison
is done.
This is the same as if no prefix was given and is needed
for upward compatibility with New UNITY.
.IP l
If the operator is prefixed with an 'l', then a lexical comparison
is done.
The two values are compared as character strings, a character at a time,
using the standard Ascii character set
(see \fIascii\fP(5) in the UNIX User's Manual).
Thus, "1.0" would not be considered equal to "1", "tim" would be
considered less than "time", and "test" would equal "test".
.IP c
If the operator is prefixed with a 'c',
then a caseless lexical comparison is done.
The two values are compared as character strings
using the standard Ascii character set,
except that upper and lower case characters
are considered equivalent.
Thus, "1.0" would be greater than "1",
"tim" would be less than "time",
and "test" would equal "TEST".
.IP r
If the \fIeq\fP or \fIne\fP operators, \fIonly\fP, are prefixed with an 'r',
then a regular expression comparison is done.
The second value is taken as a regular expression against
which to compare the field value.
The \fIregexp\fP(7) regular expression matching routines
(the same routines used in \fIed\fP(1), \fIsed\fP(1), etc.)
compile the regular expressions and make the comparisons.
The manual page for \fIed\fP(1) specifies in detail the
regular expressions that are accepted.
The regular expression takes the place of the attribute value
in the "\fIwhere\fP" clause.
It should not be surrounded by "/" or "?" since there is no notion of
current line in the selection process and should normally be surrounded
by single quotes to avoid expansion by the shell.
For upward compatibility with New UNITY,
"~" and "!~" can be used as regular expression operators
in place of "req" and "rne", respectively.
.sp
This option may also be followed by the caseless comparison
(i.e., "rc..") to do caseless regular expression matching without
the need for groupings with square brackets.
.IP d
If an operator is prefixed with a 'd', then a date
and time comparison is done.
If an operator is prefixed with an upper case 'D',
then the optional time part of a date is ignored
when comparing dates.
Date comparisons treat the values as dates in one
of the following forms:
.sp
.ne 5
.nf
.in +1.0i
<month>/<day>/<year> [<hour>:<min>[:<sec>]]
<month>-<day>-<year> [<hour>:<min>[:<sec>]]
<day>.<month>.<year> [<hour>.<min>[.<sec>]]
YYYYMMDD [HHMMSS]
YYMMDD [HHMMSS]
.in -1.0i
.fi
.sp
where month is between 1 and 12 inclusive, day is between 1 and 31
inclusive, and year is between 0 and 9999 inclusive.
This allows dates to be in either two or four digit year format.
A year that is only two digits (i.e., does not include the
century digits) will be based on the current year minus 60 years
as the start of a 100 year "window" that is used to calculate and
add the missing century digits when doing the date comparison.
Note that numbers less than 10 may appear as either two digit or
single digit numbers with or without leading white space when
they are separated by a forward slash (/), hyphen (-), or dot (.).
.IP
The time-of-day is optional, but if included, must follow
the conventions of a 24-hour clock.
If the time-of-day is omitted, it is considered to be midnight,
``00:00:00''.
Unlike the date (month/day/year) fields, no white space can appear
in the hour, minute, or second fields.
If both the date and the time-of-day are in packed computer form
then a space or tab character does not need to appear between
the date and time-of-day (i.e., YYMMDDHHMMSS or YYYYMMDDHHMMSS).
.IP f
If an operator is prefixed with an 'f', then a field-to-field comparison
is done.
Note that an operator can be prefixed with an 'fd' for a field-to-field
date comparison, an 'fl' for a field-to-field
lexical comparison, an 'fc' for a field-to-field
caseless lexical comparison,
or an 'fn' for a field-to-field
numerical comparison (the default with just an 'f' prefix is a
field-to-field numerical comparison).
An 'fr' prefix is \fInot\fP valid (i.e., a field-to-field regular expression
comparison is not allowed).
.P
Parentheses may be used to group conditions arbitrarily together
(note that the parenthesis must appear as a separate argument to
the command and that it must be escaped so that the shell does not
interpret it).
The '!' operator may be used to negate any condition.
The operator precedence from highest to lowest precedence is
comparison operators, the ! (not) unary operator, \fIand\fP,
and \fIor\fP.
Parentheses may be used to override precedence.
The where clause is formally defined as
.sp
.nf
<where clause>       ::=  \fIwhere\fP <expression>
<expression>         ::=  <expr1> | <expr1> \fIor\fP <expr1>
<expr1>              ::=  <expr2> | <expr2> \fIand\fP <expr2>
<expr2>              ::=  <expr3> | ! <expr3>
<expr3>              ::=  <field> <relop> <value> |
                          <unity field> <frelop> <unity field> |
                          ( <expression> )
<field>              ::=  \fIrec#\fP | <unity field> |
                          <unity field>:<numeric modifier>
<numeric modifier>   ::=  \fIb\fPinary | \fIh\fPex | \fIo\fPctal |
                          \fIn\fPumeric | \fIlen\fPgth
<frelop>             ::=  \fIf\fP<numerical operator> |
                          \fIf\fP<lexical operator> |
                          \fIf\fP<caseless operator> |
                          \fIf\fP<date operator>
<relop>              ::=  <numerical operator> |
                          <lexical operator> |
                          <caseless operator> |
                          <date operator> |
                          <reg. expr. operator>
<numerical operator> ::=  \fIeq\fP | \fIlt\fP | \fIgt\fP | \fIle\fP | \fIge\fP | \fIne\fP |
                          \fIneq\fP | \fInlt\fP | \fIngt\fP | \fInle\fP | \fInge\fP | \fInne\fP
<lexical operator>   ::=  \fIleq\fP | \fIllt\fP | \fIlgt\fP | \fIlle\fP | \fIlge\fP | \fIlne\fP
<caseless operator>  ::=  \fIceq\fP | \fIclt\fP | \fIcgt\fP | \fIcle\fP | \fIcge\fP | \fIcne\fP
<date operator>      ::=  \fIdeq\fP | \fIdlt\fP | \fIdgt\fP | \fIdle\fP | \fIdge\fP | \fIdne\fP
<reg. expr. operator>::=  \fIreq\fP | \fIrne\fP
<unity field>        ::=  any field in the description file
<value>              ::=  string

.fi
.P
\fIrec#\fP above is replaced by the sequence number of a record
in the table.
.P
As an example, the /etc/passwd file is a valid UNITY table.
Based on the standard format of
.sp
login:passwd:uid:gid:acct:home:shell
.sp
the description file could be
.sp
.nf
login  t:      8l
passwd t:      14l
uid    t:      5r
gid    t:      5r
acct   t:      20l
home   t:      15l
shell  t\\n     12l
.fi
.sp
A valid query might be
.sp
uselect from /etc/passwd where name req '^J[^ohn]*Smith$'
.sp
that would select all entries in the passwd file with a last name
of Smith and a first initial of 'J' but not a first name of 'John'
(e.g., 'Jim' or 'James' would be acceptable).
.SS "Crash Resistant Update"
UNITY is careful when updating a table that data is not lost.
When UNITY is compiled with a UNIX Standard C compiler
such that "__STDC__" has been defined, then
an update is done by doing the update to a temporary file
and using the rename(2) system call to rename the temporary
file to the original table name and unlink the original file
in one atomic operation.
Otherwise, the old (traditional) UNITY update method is that
when an update is done, a temporary file is updated,
the original file is removed, and the temporary
file is linked to the original table, thus updating the table.
This form of updating a temporary file and then replacing
the original is crash-resistant (if a crash occurs, no data should
be lost) and assures that a reader of the table
will get a consistent view.
When UNITY has been been built using the rename(2) system call,
the critical section comes during the removal of the original table
and the linking in of the updated table.
However, if the system crashes during an update, the table will
be in a consistent state.
It will either be unchanged if the table still exists or
changed and residing in the temporary file if the original table
has been removed (i.e., if the crash occurs after removing
the original table but before the link of the temporary file has taken place).
.P
The temporary file is always created in the directory of the table
so that there is no problem with the rename or link 
(i.e., one cannot link across file systems).
Note that this assumes that the user has write permission in the directory
containing the table but not necessarily write permission on
the table itself.  Thus, the user may turn off the write permissions of the
table to avoid accidental removal of the file (i.e., only read permission
is necessary for a table).
.SS "Handling of Concurrent Updates"
UNITY has a locking facility to limit update of a table
to one user at a time (sequential processing) to prevent inconsistencies
caused by concurrent updates.
A lock file is created for the table at the beginning of
an update and removed at the end.
Thus, the entire table is locked during the update.
If another user attempts an update while someone already
has the file locked, the user is notified and must
execute the command again.
The test for the existence of a locked table is done only in the
update programs \fIalter\fP, \fIcompute\fP, \fIdelete\fP, \fIinsert\fP,
\fIuedit\fP, \fInprintf\fP, and \fIuprintf\fP
(\fIuenter\fP uses \fIinsert\fP to do the final insertion) so that a user
making queries or reports on the table is not affected.
Thus, multiple readers and one writer are allowed for a table.
The readers will always get a consistent
view of the records (i.e., one can never look at a partially updated table),
due to the way updates are done.
.P
The locking mechanism is implemented as follows.
A \fIstat\fP(2) is done on the table to determine the device
and inode numbers for the table; these two values uniquely identify
the file on a given UNIX system.
These values are converted to Ascii strings and concatenated to form the
lock file name.
Ten attempts are made to create the lock file in /tmp; 
this should be enough time for another update to complete and remove
the lock file (unless the update is a \fIuedit\fP).
If all attempts to create the lock file fail,
the user is notified that the lock file already exists and the program exits.
The lock file name is given so that the user can examine the lock file
using \fIls\fP(1) (the long format will list the owner of the lock file)
to determine if the table is really being used or if the
lock file was accidentally left around from a previous update of the table
(i.e., the update was killed before the lock file could be removed such
as during a system crash).
If necessary, the lock file may be removed since the /tmp directory is
writable by anyone.
If the \fIcreat\fP(2) is successful, the table
is updated and then the lock file is removed.
All update programs catch interrupt, quit,
hang-up, and software termination signals and remove the lock file
before termination.
.P
Note that the locking mechanism is useless if a user edits the table
using programs other than the UNITY update programs.
If several users are to be accessing the same table, it may be necessary to
appoint an administrator who will own the file.
The table permissions may be set such that the table is
read-only for other users.
In this manner, a user may not directly edit the table but may
update it using the UNITY update programs.
.SS "Pipe-lining of UNITY Commands"
Most UNITY commands have the capability to read from the standard input
and/or write to the standard output.
This is indicated by specifying a table name of "-" for either the
input table name or in the "into clause" for an output table.
.P
However, not only is the table needed but also the associated description file.
For reading from the standard input,
the -I option is used to specify the name of a table from
which the input description file name can be determined
if/when the "-c" option was not used on the preceding UNITY command
to create a description at the beginning of the output data stream.
When writing to the standard output,
the "-c" option can be used to create the description information
between a pair of "%description" and "%enddescription" lines
at the beginning of the data stream or
the -O option can be used to specify the name of a table from
which the output description file name can be determined.
The description file is created in the current directory.
.P
When using standard input/output, one
can pipe the output of one UNITY command (or UNIX command) to the next.
However, if the -I/-O options are using instead of the "-c" option
to create the description information at the beginning of the table
in the output data stream, then
consideration must also be given to the execution order of commands in a pipeline.
The last command command gets executed first, then the next to last command,
up to the first command.
This means that the last command may be looking for an input description file
which has not yet been created as the output of the previous command.
The way to get around this problem is to have the last command sleep
before executing, giving the previous command a chance to create the
description file, as in the following:
.sp
.nf
asort -Ot login in /etc/passwd into - | (sleep 3; uprint -It -)
.fi
.sp
which prints a copy of the password file in sorted order on login.
In many cases, where the format of the file is not changed
(i.e., the input description file is the same as the output description file),
the same description file may be used, without creating an intermediate
description file, as in the following:
.sp
.nf
asort login in /etc/passwd | uprint -I/etc/passwd -
.sp
.SS "Error Checking and Messages"
Much error checking is done to protect the user from corrupting
a table and to aid in determining what errors have occurred.
The programs check to see that all positional arguments or keywords
(e.g., "\fIfrom\fP", "\fIinto\fP", etc.) appear in the proper place
in the command line arguments since their absence may mean
that the user has incorrectly formatted the command.
Checks are also done during initialization to determine that table and
description files are accessible before processing begins and that an output
table does not exist (so that a table is not
accidentally overwritten).
However, all commands which have the capability to create output tables
and description files can override this last restriction by specifying
\fIonto\fP instead of \fIinto\fP.
In this case, the output is appended
if the table already exists and the description file is not updated if
it already exists.
The user must beware that an important table is not overwritten.
.P
All programs print a "Usage:" message if the wrong number
of arguments is given for the program or if a positional argument or
keyword is missing.  Thus, a user can get the usage of a program
by entering the program name alone without arguments since all UNITY
programs require at least one argument.
Most of the commands print the number of records that are updated
or output.
This message can be
suppressed by specifying the -q option on the command line of each command.
All information and error messages are printed to the standard error output.
.P
The exit code for normal termination is 0 for all programs.  If an
error occurs, the exit code is set to 1.  In the validate program only,
a file or usage error generates an exit code of 1 and the
occurrence of one or more invalid records
in the file produces an exit code of 2.
.SS "Remaining Limitations"
UNITY still imposes some restrictions on use that might be desirable to
remove.  They include the following items:
.IP 1.
The entire table is locked during update.
It may be desirable to have a finer granularity of lock such as
being able to lock a set of records or a set of attributes.  This
would be much more complicated than the simplistic locking mechanism that was
implemented. 
This feature was not implemented because of the added complexity and since
it is not currently expected that
many users will update a single table \fIconcurrently\fP.
.IP 3.
\fIAll\fP records must be terminated with a newline.
If the last field of a record is a fixed width field, the programs read one
additional character to get the newline.
The terminator for a terminator field must be a newline if it is
the last field for the record.
Several routines depend on this restriction.
There may be instances where it is desirable to allow records without
newline separators.
It may also be desirable to have variable length records with
a binary length as the first bytes in the record.
Additionally, other attribute types (integer, float, etc.) may be desirable.
.sp
The user currently may have multi-line records if and only if the
embedded newline is a terminator for an attribute.
The \fIinsert\fP, \fIload\fP, and \fIucheck\fP programs will reject as invalid
records that have a newline in the middle of an attribute value.
.SS "General Comments"
The current UNITY limitation on record length is 4096 characters and can
easily be increased (or decreased) by changing the MAXREC definition in
db.h and building UNITY again.
Note that the line-length limitation of 512 still exists in many
editors so that they cannot be used to create or edit longer records.
These records can be created by joining two or more tables
(e.g., by using the \fIujoin\fP command in UNITY) or
using the \fIinsert\fP or \fIuenter\fP programs.
The maximum number of attributes per table is currently 300, the
maximum key length is now 128 characters, and the maximum number of keys
that can be indexed is 4096.
.bp
.in 0
.nf
.ce
\fIUNITY COMMAND USAGE\fP
.sp
\fIalter\fP [-c] [-q] [-Itable] [-Otable] aname1 \fI=\fP value [anameN f= aname] \\
 \fIin\fP table1 [\fIinto\fP table2] [where clause]
\fIasort\fP [-c] [-q] [sort flags] [-Itable] [-Otable] aname \fIin\fP table1 \\
 [\fIinto\fP table2]
\fIcatalog\fP [-llines] [-mmargin] [-wwidth] [-n] [-b] [-e] [-d] \\
 [-Itable] [-h header] [-f footer] [ aname ... \fIin\fP ] table1 \\
 [\fIinto\fP table2] [where clause]
\fIcompute\fP [-c] [-q] [-Itable] [-Otable] [-f%format] aname [op aname ...] \\
 \fIresult\fP [%format] aname1 [aname ... \fIresult\fP [%format] anameN] \\
 \fIin\fP table1 [\fIinto\fP table2] [where clause]
\fIddist\fP [-c] [-n] [-Itable] [-Otable] {aname1|count} \fIby\fP aname2 aname3 \\
 \fIin\fP table1 [\fIinto\fP table2] [where clause]
\fIdelete\fP [-c] [-q] [-Itable] [-Otable] \fIfrom\fP table1 [\fIinto\fP table2] \\
 where clause
\fIdisplay\fP [-n] [-q] [-Itable] table [{Dpath|Itable|des|data}]
\fIdist\fP [-c] [-n] [-Itable] [-Otable] {aname1|count} \fIby\fP aname2 \fIin\fP table1 \\
 [\fIinto\fP table2] [where clause]
\fIdtally\fP [-n] [-Itable] [-Otable] aname1 aname2 \fIin\fP table1 \\
 [\fIinto\fP table2] [where clause]
\fIerase-index\fP aname \fIfrom\fP table
\fIerase-table\fP table [table ...]
\fIformat\fP [-c] [-q] [-Itable] [-Otable] table1 \fIinto\fP table2
\fIindex\fP [-Itable] aname \fIin\fP table
\fIinsert\fP [-c] [-q] [-rErrorLimit] [-Itable] [-Otable] [-n[newline]] \\
 \fIin\fP table1 [\fIinto\fP table2] [ { \fIprompting\fP | \fIfrom\fP table3 } ]
\fIjoin\fP [-c] [-q] [-Itable1] [-Itable2] [-Otable] a1a[,a1b,...] \\
 a2a[,a2b,...] \fIfrom\fP table1 table2 [\fIinto\fP table3]
\fIload\fP [-q] [-rErrorLimit] [-Itable] data-file \fIto\fP table
\fImaketable\fP [-v] table
\fImerge\fP [-c] [-q] [-s<separator>] [-Itable] [-Otable] aname1 \fIin\fP table1 \\
 [\fIinto\fP table2]
\fInprintf\fP [-a] [-c] [-n] [-q] [-Itable] [-Otable] \\
 [-C <CenturyDigits>] [-S] [-W <DateWarningLimit>] \\
 aname1 [:modifier] [%format] [anameN ...] \\
 \fIin\fP table1 [\fIinto\fP table2] [where clause]
\fIpacket\fP [-q] [-Itable] [-Otable] [-n[<newline>]] [-u] table1 [\fIinto\fP table2]
\fIouterjoin\fP [-c] [-q] [-Itable1] [-Itable2] [-Otable] a1a[,a1b,...] \\
 a2a[,a2b,...] \fIfrom\fP table1 table2 [\fIinto\fP table3]
\fIregjoin\fP [-c] [-q] [-Itable1] [-Itable2] [-Otable] a1a[,a1b,...] \\
 a2a[,a2b,...] \fIfrom\fP table1 table2 [\fIinto\fP table3]
\fIsetdiff\fP [-c] [-q] [-Itable1] [-Itable2] a1a[,a1b,...] a2a[,a2b,...] \\
 \fIfrom\fP table1 table2 [\fIinto\fP table3]
\fIstat\fP [-c] [-Itable] [-Otable] [ aname ... \fIin\fP ] table1 \\
 [\fIinto\fP table2] [where clause]
\fItable\fP table1 aname1 atype1 [...] 
\fItally\fP [-c] [-n] [-Itable] [-Otable] aname \fIin\fP table1 \\
 [\fIinto\fP table2] [where clause]
\fItuple\fP [-c] [-q] [-Itable] [-Otable] [-n[<newline>]] [-u] table1 \\
 [\fIinto\fP table2]
\fIuadmin\fP [-rrel] [-t[name]] [-fflag[flag-val]] [-alogin] \\
 [-m[mrlist]] [-y[comment]] file ...
\fIucheck\fP [-q] [-rErrorLimit] [-Itable] table
\fIudelete\fP [-c] [-q] [-Itable] [-Otable] \fIfrom\fP table1 [\fIinto\fP table2] \\
 where clause
\fIudelta\fP [-rSID] [-s] [-n] [-glist] [-m[mrlist]] [-y[comment]] \\
 [-p] file ...
\fIuedit\fP [-q] [-Itable] [-v] [-n[nline]] [-u] table [where clause]
\fIuenter\fP [-q] [-Itable] [-v] [-n[nline]] [-u] [-c] \fIin\fP table
\fIuformat\fP [-c] [-q] [-Itable] [-Otable] table1 \fIinto\fP table2
\fIuget\fP [-rSID] [-ccutoff] [-ilist] [-x[list] [-aseq-no.] [-k] [-e] \\
 [-s] [-b] [-g] [-t] file ...
\fIujoin\fP [-c] [-q] [-Itable1] [-Itable2] [-Otable] a1a[,a1b,...] \\
 a2a[,a2b,...] \fIfrom\fP table1 table2 [\fIinto\fP table3]
\fIunload\fP table1 [ \fIto\fP table2 ]
\fIumenu\fP
\fIunmerge\fP -c] [-q] [-Itable] [-Otable] [-s<separator>] aname1 \fIin\fP table1 \\
 [\fIinto\fP table2]
\fIuprint\fP [options] [ aname ... \fIin\fP ] table1 [\fIinto\fP table2] \\
 [where clause]
\fIuprintf\fP [-c] [-q] [-Itable] [-Otable] [-fformat] [-nc] aname \\
 [aname ...] [ { \fIresult\fP | \fIresult:\fPformat } aname1] \\
 [aname ... { \fIresult\fP | \fIresult:\fPformat } anameN] \\
 \fIin\fP table1 [\fIinto\fP table2] [where clause]
\fIuselect\fP [-c] [-q] [-Itable] [-Otable] [aname [\fIas\fP aname2] ...] \\
 \fIfrom\fP table1 [\fIinto\fP table2] [where clause]
\fIvalcmp\fP [-l] [-Itable] table [outfname]
\fIvalidate\fP [-Itable] table1

Notes:
aname         is an attribute name
atype         is an attribute type definition of the form
       {w<fixed length> | t<terminator>} \\
       [<tab>[print width][print position]]

where clause  is defined as
<where clause>       ::=   \fIwhere\fP <expression>
<expression>         ::=   <expr1> | <expr1> \fIor\fP <expr1>
<expr1>              ::=   <expr2> | <expr2> \fIand\fP <expr2>
<expr2>              ::=   <expr3> | ! <expr3>
<expr3>              ::=   <field> <relop> <value> |
                           <unity field> <frelop> <unity field> |
                           ( <expression> )
<field>              ::=  \fIrec#\fP | <unity field> |
                          <unity field>:<numeric modifier>
<numeric modifier>   ::=  \fIb\fPinary | \fIh\fPex | \fIo\fPctal |
                          \fIn\fPumeric | \fIlen\fPgth
<frelop>             ::=   \fIf\fP<numerical operator> |
                           \fIf\fP<lexical operator> |
                           \fIf\fP<caseless operator> |
                           \fIf\fP<date operator>
<relop>              ::=   <numerical operator> |
                           <lexical operator> |
                           <caseless operator> |
                           <date operator> |
                           <reg. expr. operator>
<numerical operator> ::=   \fIeq\fP | \fIlt\fP | \fIgt\fP | \fIle\fP | \fIge\fP | \fIne\fP |
                           \fIneq\fP | \fInlt\fP | \fIngt\fP | \fInle\fP | \fInge\fP | \fInne\fP
<lexical operator>   ::=   \fIleq\fP | \fIllt\fP | \fIlgt\fP | \fIlle\fP | \fIlge\fP | \fIlne\fP
<caseless operator>  ::=   \fIceq\fP | \fIclt\fP | \fIcgt\fP | \fIcle\fP | \fIcge\fP | \fIcne\fP
<date operator>      ::=   \fIdeq\fP | \fIdlt\fP | \fIdgt\fP | \fIdle\fP | \fIdge\fP | \fIdne\fP
<reg. expr. operator>::=   \fIreq\fP | \fIrne\fP
<unity field>        ::=   any field in the description file
<value>              ::=   string

\fIrec#\fP above is replaced by the sequence number of a record
in the table.

The field comparison operators are indicated by an 'f' prefix on the
numerical, lexical, caseless, and date comparison operators.

uprint options are
  -f take next arg as footer   -w\fIk\fP page width \fIk\fP
  -h take next arg as header   -l\fIk\fP page length \fIk\fP
  -r suppress error messages   -o\fIk\fP offset left margin \fIk\fP positions
  -t suppress header/trailer   -s\fIc\fP separator character \fIc\fP
  -u suppress box              -n\fIc\fP translate \fIc\fP into newline 
  -b break on non-alphanumeric -a take next arg as attribute line
  -d\fIc\fP double space with \fIc\fP used for separator line
  -e number records            -v print user\-friendly names (verbose)
  -p don't print records across page breaks
  -R don't print repeated attribute values on left

sort flags are
  -b ignore leading blanks
  -d dictionary sort (letters,digits,blanks only)
  -f fold upper to lower case   
  -i ignore characters outside Ascii 040-0176
  -n numeric sort 
  -r reverse sort order
  -t\fIx\fP tab character separating sub-fields of attribute is \fIx\fP
  -\fIpos\fP, +\fIpos\fP - restricts sort sub-fields
  -u unique sort, cannot be used with -t, -D, -\fIpos\fP, +\fIpos\fP
  -D Date sort, cannot be used with -t, -u, -\fIpos\fP, +\fIpos\fP

Braces ({...}) show alternative choices.
Brackets ([...]) show optional information.
.fi
