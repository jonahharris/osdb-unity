.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH umenu "UNITY" "" "UNITY User's Manual"
.SA 1
.SH NAME
umenu \- menu access to UNITY commands
.SH SYNOPSIS
\fIumenu\fP
.SH DESCRIPTION
The \fIumenu\fP command provides a menu access to the UNITY uselect, asort,
uprint, and catalog commands.
.P
The only thing needed to get started is a file named \fIdbfiles\fP
in either the directory of execution
or in one of the directories specified in the $UNITYDFILES path
(a shell environment variable).
This file contains a list of database tables, one per line, to be accessed by
the menu.
The table names specified may be full pathnames for the tables
(e.g., /usr/user1/table),
relative pathnames (e.g., user1/table), or simple pathnames (e.g., table).
The \fIdbfiles\fP file may also contain "user" titles for each table
which will be printed out by the menu program when prompting for
table access (i.e., the actual table name is used by the programs for access
but the "user" names are the names that user sees).
These may be entered by following each table name by appending a vertical
bar (|) and then the user name.
Any character following the vertical bar is taken to be part of the user
name including white space.
The only restriction is that no line may contain a double quotes.
If any line in \fIdbfiles\fP contains a shell meta-character, the line
is first expanded using \fBecho\fP(1) and the shell.
Thus, the line
.sp
`logdir unity`/users|USERS OF UNITY
.sp
will be expanded to pick up the "users" file in the home directory
of the unity login.
.P
In addition to providing the \fIdbfiles\fP file, an \fIoverview\fP file
may be provided which gives an overview of the tables accessible
by this menu.
It may exist in either the directory of execution
or in one of the directories specified in the $UNITYDFILES path
(a shell environment variable).
The contents of \fIoverview\fP may be anything desirable for the application;
the file is simply printed to the standard error output
of the \fIumenu\fP command.
.P
The menu starts with three options for the user.
The \fIo\fP option prints the overview.
The \fIe\fP option indicates that the menu will be used in expanded
format; this option produces more verbose output and generally lists
the possible options before each prompt.
The \fIn\fP option provides for normal mode; this will provide a
series of prompts (questions) for the user that is more experienced with
the menu and allows for expanded output at each prompt by entering a question
mark.
The user first selects the table to be accessed, the attribute(s)
to be printed, the selection criteria by which to select
records (prompted for by attribute name, then comparison operator, then
comparison value, optionally "and" or "or" and further criteria),
the attribute on which to sort, and the output format (i.e., the
use of the uprint or catalog command).
If user\-friendly names have been provided in the associated description
files, these are printed instead of the attribute names on the menus by
\fIumenu\fP and on final output by \fIcatalog\fP and \fIuprint\fP
(see \fItable\fP(UNITY) for further information).
The example below should be provide any further details.
.P
All prompts and messages are printed to the standard error output;
the actual output of the \fIuprint\fP and \fIcatalog\fP commands
is printed to the standard output.
This allows the standard output to be redirected to a file to
which will contain the printed output while allowing the
user to enter the necessary information at the terminal.
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
.SH EXAMPLE
In the following example, the data entered by the user is in bold.
.sp
.nf
$ cat dbfiles
SP|SUPPLIER/PARTS
S|SUPPLIER
P|PARTS
INV|INVENTORY
$ umenu
Welcome to the unity menu system.
You may select an overview of the sequence of operations (type o),
or an expanded instruction option (type e),
or a normal prompt option (type n) or quit (type q).
Option = \fBe\fP
Select your area of interest from the following list of database file names
Valid Database file names:
      1 SUPPLIER/PARTS                      2 SUPPLIER                      
      3 PARTS                               4 INVENTORY                     

Enter the number of Database file you selected = \fB1\fP
A list of characteristics will be displayed which describe the database
file you have chosen.  Select the characteristic(s) which will tell you
what you want to know about this database file.
Valid characteristics for database file SUPPLIER/PARTS are:
    1 SUPPLIER NUMBER                   2 PART NUMBER                   
    3 QUANTITY                      
Enter number(s) of characteristic(s) you chose separated by commas
or spaces.  If you want to see all of them press Return.
\fB2, 3\fP
You have a choice of displaying only those records in your database file
which have specific characteristics you choose.  Each characteristic you
choose is compared to a specific value chosen by you to select the desired
records from the database file.  Choose the characteristic you want to use
in selecting which records are displayed from the following list.
Valid characteristics for database file SUPPLIER/PARTS:
    1 SUPPLIER NUMBER                   2 PART NUMBER                   
    3 QUANTITY                      
Enter number of characteristic you want to use in selecting which records
are displayed.  If you do not want to choose characteristics, press
Return and your records will be displayed.
\fB1\fP
You have a choice of ways of making this comparison such as
less than, equal to or greater than a numeric value, and lexically
equal to a word.  Select the comparison method you want to use
from the following list.
1  numerically equals          2  numeric less than
3  numeric greater than        4  numeric < or equal to
5  numeric > or equal to       6  numeric not equal to
7  lexically equal to          8  lex less than
9  lex greater than           10 lex < or equal to
11 lex > or equal to          12 lex not equal to
13 reg expr equal to          14 reg expr not equal to
Enter the number of the comparison method you chose = \fB8\fP
Now choose the value of the characteristic used in the comparison.  If
you don't know how the values are specified press "?" and a list of
the values in the database file will be displayed.
Enter the value for PART NUMBER you select for the comparison.
\fB?\fP
p1
p2
p3
Enter the value for PART NUMBER you select for the comparison.
\fBp3\fP
If you want to use additional characteristics to select which records are
displayed, you may choose to display only records which meet all the
conditions you choose ("and") or those which meet any one of the
conditions ("or").  Enter "and" or "or" (without quotes).
If you do not want to use any additional characteristics press Return
and your records will be displayed.
\fBand\fP
You have a choice of displaying only those records in your database file
which have specific characteristics you choose.  Each characteristic you
choose is compared to a specific value chosen by you to select the desired
records from the database file.  Choose the characteristic you want to use
in selecting which records are displayed from the following list.
Valid characteristics for database file SUPPLIER/PARTS:
    1 SUPPLIER NUMBER                   2 PART NUMBER                   
    3 QUANTITY                      
Enter number of characteristic you want to use in selecting which records
are displayed.  If you do not want to choose characteristics, press
Return and your records will be displayed.
\fB2\fP
You have a choice of ways of making this comparison such as
less than, equal to or greater than a numeric value, and lexically
equal to a word.  Select the comparison method you want to use
from the following list.
1  numerically equals          2  numeric less than
3  numeric greater than        4  numeric < or equal to
5  numeric > or equal to       6  numeric not equal to
7  lexically equal to          8  lex less than
9  lex greater than           10 lex < or equal to
11 lex > or equal to          12 lex not equal to
13 reg expr equal to          14 reg expr not equal to
Enter the number of the comparison method you chose = \fB9\fP
Now choose the value of the characteristic used in the comparison.  If
you don't know how the values are specified press "?" and a list of
the values in the database file will be displayed.
Enter the value for PART NUMBER you select for the comparison.
\fBp1\fP
If you want to use additional characteristics to select which records are
displayed, you may choose to display only records which meet all the
conditions you choose ("and") or those which meet any one of the
conditions ("or").  Enter "and" or "or" (without quotes).
If you do not want to use any additional characteristics press Return
and your records will be displayed.
<Return>
The characteristics for the file will be displayed.  Select the
characteristic by which you want the selected data sorted.
Valid characteristics for database file SUPPLIER/PARTS:
    1 SUPPLIER NUMBER                   2 PART NUMBER                   
    3 QUANTITY                      
Enter number of characteristic on which to sort or press RETURN
if no sort is desired.
\fB2\fP
There are two different types of displaying the data.
table - the data is printed in a tabular format with characteristic
        names appearing at the top of each column.
packet - the data is printed horizontally with the characteristic
        names appearing at the left and the data immediately following
        at the right.
Enter t for table format or p for packet format
\fBp\fP

---------------------------------------------------------------------------
---------------------------------------------------------------------------

PART NUMBER                        p2
QUANTITY                           200

---------------------------------------------------------------------------
---------------------------------------------------------------------------

PART NUMBER                        p2
QUANTITY                           300



Do you wish to continue? (y or n) = \fBn\fP
.fi
.SH FILES
.nf
unity/umenu.c - the C program
bin/umenu - the executable program
./dbfiles - list of database file names
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIasort\fP(UNITY), \fIcatalog\fP(UNITY), \fIuprint\fP(UNITY),
\fIuselect\fP(UNITY), \fItable\fP(UNITY)
