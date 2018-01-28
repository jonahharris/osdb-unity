.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH val UNITY "" "UNITY User's Manual"
.SA 1
.SH NAME
val \- function to validate a UNITY record
.SH SYNOPSIS
.nf
val(fmt,errors,errcnt,Wname)
struct  fmt     *fmt;   /* record layout and values */
char    **errors;       /* array of pointers to error codes */
int     *errcnt;        /* count of validation errors */
char    *Wname;         /* validation object module */
.fi
.SH INTRODUCTION
Most software systems validate their input data by passing it
through a program that tests the data items for such things
as proper syntax, inclusion in a numerical range,
appearance in a list of allowed values, and so on.
The various tests are coded directly in a conventional programming language.
This method is satisfactory if the definition of the data items
to be validated and the validation criteria are stable and universal.
However, if new data items are introduced or old ones redefined,
or if the validation criteria change with time or from customer-to-customer,
this method results in a major maintenance and testing burden.
.P
The validator described here uses a different approach.
The validation criteria are expressed in a \fIvalidation table\fP,
coded in a special-purpose \fIvalidation language\fP
that consists of Boolean expressions describing \fIvalidation conditions\fP
and \fIerror codes\fP denoting errors to be generated if certain
conditions are false.
The validation table is compiled by a \fIvalidation compiler\fP
into instructions for a \fIvalidation machine\fP.
When a set of data items (in the form of a record)
is to be validated, the validation machine executes the compiled instructions
and either reports success or issues a list of error codes.
.P
The approach to validation used here has these advantages over
the "hard-coded" approach:
.TP
1.
It is easier to learn the validation language and to write validation conditions
in it than it is to learn and program in a programming language
(such as COBOL, PL/I, SNOBOL or C).
This makes it feasible for non-programmers to code or modify the
validation table.
In fact, it is also substantially easier for programmers to
code in the validation language than in C.
.TP
2.
A desirable capability for some systems is for users to be able to add
their own table attributes.
Some method for expressing the validation criteria for these added
fields is necessary.
Such a system will be more uniform if that method is the same as the one
used to code validation criteria for the previously defined fields.
.TP
3.
If the validation machine is operating correctly,
it is impossible for anything coded in the validation language to generate
a result outside of the definition of the validation machine
(for example, a PDP11 memory fault).
Errors in coding the validation table are either caught by the
validation compiler or, at worst, result in incorrect validation
(a valid data item being called invalid or an invalid data item being
overlooked).
If one is trying to fix a problem other than incorrect validation,
one has only to look at the validation machine;
one need not bother with the validation table.
This simplifies maintenance considerably.
In fact, it makes it possible for each user to modify the validation table
to suit themselves, without jeopardizing another user's ability to maintain the
rest of the system.
.TP
4.
Since the effect of an error in the validation table is limited
to improper validation, and since the same computer instructions in
the validation compiler and machine are executed by many different
validation conditions, it is easier to test the validation module than
if the validation conditions were coded in a programming language.
It is easier to test the validation table because the number of things
to test for is so limited.
It is easier to test the validation compiler and machine because the
number of relevant test cases is not affected by the size or complexity
of the validation table.
Also, when the validation table changes it is not necessary to retest
the validation compiler and machine.
.TP
5.
The validation table serves as a formal document (more readable than
a program) describing the validations done by a particular
system installation.
Users wishing to interface other systems to a system using the validator
will find this document useful.
.P
This manual page explains how to code the validation table.
The manual page for validate(UNITY) describes the other tables and how
to use the validator.
.SH "VALIDATION TABLE"
.SS Overview
In its simplest form the validation table consists of Boolean expressions,
representing validation conditions, along the left margin
with an error code following each expression.
For example:
.sp
.nf
LN != ""                                      ln1
NPA % "[2-9][01][1-9]"                        npa1
.fi
.sp
The first line implements the validation requirement that the
list name is required.
It tests if the \fIfield\fP LN ("list name") is not equal
(the != \fIoperator\fP) to the null \fIstring\fP.
If this condition is false (that is, if the list name is omitted),
then error "ln1" is generated.
The second line implements the validation requirement that
the first digit of the NPA (area code of a telephone number)
must be between 2 and 9,
the second digit must be 0 or 1, and the third digit must be between 1 and 9.
It tests if the field NPA matches (the % operator) the pattern, or
\fIregular expression\fP, given by the string within quotes.
This regular expression consists of three subexpressions within
brackets, one for each digit.
Field references (such as LN and NPA) and strings are \fIoperands\fP.
Other operands are \fInumbers\fP and \fIfunctions\fP.
There is a wide variety of operators, including ones for numerical
and lexical comparisons, arithmetic, Boolean relations and pattern matching.
Operands and operators may be combined to form validation conditions
of arbitrary complexity.
Validation conditions may stand alone (like the two above) or they may
be arranged so that some conditions are dependent on others.
.P
The next three sub-sections describe operands, operators and
regular expressions, respectively.
The fourth sub-section describes the format of the validation table.
.SS Operands
.TP 3
Strings
Strings consist of a sequence of zero or more characters;
a string of zero characters is called a \fInull\fP string.
They are used for the values of fields, returned values of some
functions and results of some operators, as described below.
String constants may also be coded directly by enclosing the string
by double quotes (").
Within string constants the character blank and the characters
.br
.nf
abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ
0123456789!#$%&'()*+,-./:;<=>?@[]^_`{|}~
.fi
.br
represent themselves.
Other characters may be represented by these escapes:
.sp
\\" - "
.sp
\\b - backspace
.sp
\\n - newline
.sp
\\r - return
.sp
\\t - tab
.sp
\\\\ - \\
.sp
\\N - the character whose internal representation is the
1, 2 or 3 digit octal number N.
N may not be equal to 0.
.sp
The following are some examples of string constants:
.sp
.nf
""
"abcd"
"x\etx\e234"
.fi
.sp
The last string consists of 4 characters: an "x",
a tab, another "x" and the character whose octal representation is 234.
.TP 3
Numbers
Numbers are integers in the range -999999999 through 999999999.
Some functions and operators produce numbers;
number constants may also be coded directly.
In a context (such as an operand of +) where a number is required
a string may also be used, but it must form a legal number constant.
Number constants are written as a sequence of digits and must
be in the range 0 through 999999999.
Negative number constants may not be coded directly,
but the negation operator (unary -) may be used to accomplish the
same effect.
.sp
The following are examples of number constants:
.sp
.nf
0
123
987654321
.fi
.TP 3
Fields
The value of a data item may be incorporated into a validation condition
by coding the name of the field corresponding to that item.
Field or attribute names consist of up to 31 characters,
chosen from the set "A" through "Z",
"a" through "z", "0" through "9" and "_".
The first character must be alphabetic or "_".
Field names are not defined by the validation module;
any field name listed in the \fIdescription file\fP may be used.
The meaning of a field is determined by the module that passes the
record to be validated to the validation module.
.sp
A field reference always has a string value.
The string is null if the field has a null value in the record.
The following are examples of field references:
.sp
.nf
LN
NPA
STATE
.fi
.TP 3
Functions
Functions provide facilities beyond those provided by operators.
They are referenced by the function name, followed by an opening
parenthesis, followed by zero or more arguments separated by commas,
followed by a closing parenthesis.
For example:
.nf
substr(NPA,2,1)
.fi
.sp
All functions are built into the validation machine;
it is not possible for the validation table coder to define additional
functions.
The following functions are currently available:
.sp
\fIprint\fP( str)
.br
This function prints its argument on the standard output and returns 1.
.sp
\fItblsearch\fP(tbl,\ keycol,\ keyval,\ retcol)
.br
This function provides a mechanism to search simple tables.
Each of the 4 arguments are converted to strings if necessary.
The first argument is the name of the unity table containing the data
to be searched.
The second argument is the name of the column or attribute to be searched,
and the third argument is the value to be searched for in that column.
The last argument gives the name of the column or attribute in which the
corresponding value is returned.
If the search fails, the returned string is null.
\fItblsearch\fP is used as in this example:
.sp
.nf
tblsearch("TABLE", "IND", CS, "CS") != ""          badcs
.fi
to validate the CS attribute using attribute IND in unity table TABLE.
.sp
\fIsize\fP(str)
.br
This function returns the number of characters in its argument,
which is converted to a string if necessary.
For example,
.sp
.nf
size("") returns 0,
size("xxx") returns 3 and
size(12) returns 2.
.fi
.sp
\fIsubstr\fP(str,\ start,\ length)
.br
This function returns a substring of its first argument.
The second argument specifies the starting character of the substring;
the first character is numbered 1.
The third argument specifies the length of the substring;
if it is omitted it is taken to be infinity.
Only the intersection of the specified substring and the actual string
is returned.
If this intersection is null then the null string is returned.
For example,
.sp
.nf
substr("abc",2,2) returns "bc".
substr("xyzzyx",3) returns "zzyx".
substr("hijk",7,2) returns the null string.
.fi
.sp
\fIdate\fP()
.br
The current date in a string of the form YYMMDD is returned.
.sp
\fItime\fP()
.br
The current time in a string of the form HHMMSS is returned.
.sp
\fIgetenv\fP( var_name )
.br
This function returns the value of the given \fIvar_name\fR
from the environment, the same as \fIgetenv\fR(3).
If \fIvar_name\fR is not defined in the environment,
then an empty string is returned.
.sp
\fIaccess\fP( pathname, mode )
.br
This function checks the accessibility of the given \fIpathname\fR
for the given \fImode\fR,
the same as the \fIaccess\fR(2) system call.
See the \fIaccess\fR(2) manual page for more details.
.sp
\fIindex\fP( string, ch )
.br
This function looks for the first occurance of the given
character \fIch\fR in the given \fIstring\fR.
The index of the starting location is returned,
as useable by \fIsubstr\fR(), if it is found.
Zero is returned if the character is not found.
.sp
\fIrindex\fP( string, ch )
.br
This function looks for the last occurance of the given
character \fIch\fR in the given \fIstring\fR.
The index of the starting location is returned,
as useable by \fIsubstr\fR(), if it is found.
Zero is returned if the character is not found.
.sp
\fIsystem\fP( cmdline )
.br
The given command line is passed to the shell for interpretation.
The exit code from the shell is returned.
.sp
\fIpipe\fP( cmdline )
.br
The given command line is passed to \fIpopen\fR(3) and the
output of the command line (up to 255 characters) is returned.
.SS Operators
The following table summarizes the available operators.
The first column contains the operator.
The second column gives the precedence;
higher-numbered operators are performed before lower-numbered ones.
For example, in the expression 1+2*3 the multiplication is performed before the
addition because the "*" operator has a precedence of 7 and the "+" operator
has a precedence of 6.
The third, fourth and fifth columns indicate whether the left operand,
right operand and result are strings (S) or numbers (N).
Except for the right operand of the regular expression operators,
operands are converted if necessary.
Numbers always convert successfully to strings,
but strings convert successfully to numbers only if they consist
of 9 or fewer digits.
The last column explains the operator briefly.
.sp
.nf
.ul 2
                  Left     Right
Op.      Prec.    Operand  Operand  Result   Explanation

|        1        N        N        N        Or
&        2        N        N        N        And
==       3        S        S        N        Lex. equal
!=       3        S        S        N        Lex. not equal
#==      3        N        N        N        Num. equal
#!=      3        N        N        N        Num. not equal
%        3        S        S        N        Reg. expr. match
%%       3        S        S        N        Reg. expr. word match
$<       4        S        S        N        Lex. less
$<=      4        S        S        N        Lex. less or equal
$>       4        S        S        N        Lex. greater
$>=      4        S        S        N        Lex. greater or equal
<        4        N        N        N        Num. less
<=       4        N        N        N        Num. less or equal
>        4        N        N        N        Num. greater
>=       4        N        N        N        Num. greater or equal
$        5        S        S        S        Concatenate
+        6        N        N        N        Add
-        6        N        N        N        Subtract
*        7        N        N        N        Multiply
/        7        N        N        N        Divide
-        8                 N        N        Negative (unary)
!        8                 N        N        Not (unary)
.fi
.P
Additional information about the operators:
.TP 3
|\ \ &\ \ !
The Boolean operators consider the value 0 to be \fIfalse\fP
and any nonzero value to be \fItrue\fP.
The | operator produces 1 if either of its operands is true;
otherwise it produces 0.
The & operator produces 1 only if both of its operands are true;
otherwise it produces 0.
The unary ! operator produces 1 if its operand is false and 0 if its
operand is true.
Both operands of | and & are always evaluated.
.TP 3
==\ \ !=\ \ $<\ \ $<=\ \ $>\ \ $>=
The lexical comparison operators treat their operands as
strings and produce 0 or 1.
Strings are compared by considering the characters from left to
right using the ASCII collating sequence.
If one string is a prefix of the other, the shorter string is considered
lexically less than the longer string;
for example, "abc" is lexically less than "abcde".
.TP 3
#==\ \ #!=\ \ <\ \ <=\ \ >\ \ >=
The numeric comparison operators treat their operands as numbers and
produce 0 or 1.
The operands are compared numerically.
As an example, the expression 2<10 produces 1, while the expression
2$<10 produces 0.
.TP
+\ \ -\ \ *\ \ /\ \ -
The arithmetic operators treat their operands as numbers and produce
numbers.
Integer arithmetic is used, which means that any remainder resulting
from the / operator is lost.
.TP 3
$
The only string operator is $.
It concatenates its right operand onto the right of its left operand
and produces the resulting string.
Concatenation is a rather expensive operator;
it should be avoided if the same effect can just as conveniently be
achieved another way.
.TP 3
%\ \ %%
The regular expression operators treat their left operand as a string
and convert it if necessary;
the right operand must be a \fIstring constant\fP.
The right operand is interpreted as a regular expression.
The % operator produces 1 if the regular expression matches the left operand;
otherwise it produces 0.
The %% operator is similar,
but it considers the left operand to be a sequence of words separated
by exactly one blank.
It produces 1 if the regular expression matches every word;
otherwise it produces 0.
Regular expressions are described the next section.
.SS "Regular Expressions"
With a few exceptions, (the main one is that a regular expression must match
the entire string in order for the match to be considered successful)
regular expressions in the validation language are identical to those used in
the UNIX(TM) text editor.
For completeness, however, they will be described in this section.
.sp
A regular expression is a way of specifying a pattern that will match certain
strings.
Perhaps the simplest regular expression is one that consists of characters
to be matched exactly.
For example, the regular expression "abcdef" matches the string "abcdef".
In fact, the % operator need not be used for simple regular expressions
like these:
the expressions "abcdef"%"abcdef" and "abcdef"=="abcdef" are equivalent.
.sp
The power of regular expressions arises from the fact that special
characters may be used to specify more complex match conditions.
These special characters are explained below.
.TP
"."
The period matches any character.
For example,
the regular expression "xyz.." matches any 5 character string beginning
with "xyz".
.TP
"*"
The asterisk causes the character preceding it to match zero or
more occurrences, instead of just one.
For example, the regular expression "xy*z" will match the strings
"xz", "xyz", "xyyz", etc.
The regular expression "x.*" will match any string beginning with "x".
The regular expression ".*x.*" will match any string that contains
an "x" anywhere.
.TP
"+"
The plus is similar to the asterisk, except it matches \fIone\fP
or more occurrences instead of \fIzero\fP or more.
For example, the regular expressions "xx*" and "x+" are equivalent.
.TP
"[]"
A string of characters enclosed by square brackets
matches any character in the string.
This may be thought of as a more restricted form of the period.
For example, the regular expression "[xyz]r" matches "xr", "yr" and "zr".
The regular expression "[abc][def]" matches "ad", "ae", "af", "bd", "be", etc.
A hyphen may be used to indicate a range of characters to save typing.
For example, "[a-k]" is equivalent to \%"[abcdefghijk]".
A hyphen represents itself only if it is the first or last character within the brackets.
If a circumflex (^) follows the left bracket immediately,
then any character except those in the string is matched.
For example, "[^fn]" matches any character other than "f" or "n".
.sp
The asterisk and plus work with bracketed strings as well as with
single characters.
For example, the regular expression "[xyz]*" matches "", "x", "xx",
"xy", "yx", "xxxzyxzzzy", etc.
.TP
"{}"
Curly brackets following a single character or bracketed string are
used to indicate a range.
If a single integer is within the curly brackets it indicates exactly that
many occurrences of the preceding character or bracketed string.
For example,
the regular expression "x{4}" is equivalent to "xxxx".
The regular expression "[0-9]{3}" is equivalent to "[0-9][0-9][0-9]".
If two integers separated by a comma are within the curly brackets,
the preceding character or bracketed string is repeated at least as many times
as the first integer indicates and at most as many times as the second
integer indicates.
For example, "x{2,5}" matches "xx", "xxx", "xxxx" and "xxxxx",
but not "x" or "xxxxxx".
If the comma is present but the second integer is omitted,
it is taken to be infinity.
Hence,
the asterisk and plus are equivalent to {0,} and {1,},
respectively.
.TP
"()"
Parentheses can be used to group characters and/or bracketed strings to
make them subject to the asterisk, plus and curly brackets.
For example, the regular expression "(x[ab]y){2,3}"
matches "xayxay", "xbyxby", "xayxby", "xbyxay", "xayxayxay", etc.
It doesn't match "xay" or "xayxayxayxay", nor, of course,
does it match things like "xaby" or "xyxy".
.P
If any of the special characters listed above are needed to represent
themselves, they may be preceded by a backslash (\e).
For example, the regular expression "\e**" matches a string of zero
or more asterisks.
.SS Format
.TP 3
Comments
Comments may appear anywhere a blank may appear,
except within string constants or other comments.
Comments begin with /* and end with */.
The best place for comments is probably above the
validation conditions they describe.
For example:
.nf
/* Street name must be capitalized alpha words. */
STREET        %%        "[A-Z][a-z]*"                st1
.fi
.TP 3
Expressions
Operands and operators may be combined to form expressions.
Parentheses may be
used to override the precedence of operators or to improve readability.
It is also a good idea to use parentheses if one is not sure of the
precedence of an operator.
.br
For example:
.nf
.ce
(PROD == "plfd" | PROD == "smmt") & NPA == "609"
.fi
(We could have coded 609 as a number,
but that would have caused an unnecessary conversion since the
value of a field is always a string.)
.sp
Blanks may be used between operands and operators to improve readability.
Whenever any of the characters
.nf
.ce
|&=<>#$+-*/%!
.fi
appear adjacent outside of string constants and comments,
they are assumed to be an operator even though such an operator
may not be defined (in which case a diagnostic would result).
Therefore in coding expressions like
.nf
5 * -3
.fi
the blank between the asterisk and hyphen is required.
.SS "Table Structure"
The validation table is contained in an ordinary UNIX text file.
It is created and modified with the UNIX text editor.
The table is compiled with the \fIvalcmp\fP(UNITY) command.
Compilation produces object code which the machine accesses
at execution time.
.P
The validation machine executes the table for each record presented to it.
The validation table is executed from top to bottom.
If a validation condition is \fIfalse\fP,
as indicated by its having zero as its value,
the error code following it is issued by the validation machine.
A maximum of 256 error codes may be issued for a single record.
.P
As in the examples that have appeared above,
an error code follows its validation condition with at least
one tab and any number of additional blanks and tabs in between.
Error codes may be as long as 9 characters,
and may consist of characters chosen from the set "A" through "Z",
"a" through "z", "0" through "9" and "_".
.P
An error table may be optionally provided which gives a mapping
from error codes specified in the validation script to user\-friendly
error messages.
In this case, the error message is printed instead of the error code.
See the \fIvalidate\fP(UNITY) manual page for more details.
.P
If a validation condition is indented by one tab under another
validation condition, the indented validation condition,
as well as validation conditions indented under it,
is not executed if the one above it is \fIfalse\fP.
The following skeletal validation table will be used to illustrate
this.
A, B, C, D, E and F are validation conditions.
.nf
A                                e1
        B                        e2
                C                e3
                D                e4
        E                        e5
F                                e6
.fi
If A is false, error code e1 is issued and testing skips to F.
If A is true, B is tested next.
If B is false, error code e2 is issued and testing skips to E.
If B is true, C is tested next, followed by D.
Error code e3 is issued if C is false and error code e4 is issued
if D is false.
After D is tested E is tested.
Error code e5 is issued if E is false.
Finally, F is tested and error code e6 is issued if it is false.
.P
Validation conditions that have other validation conditions indented under
them need not have error codes.
Such validation conditions would serve to determine whether other validation
conditions are to be tested, but they themselves would not cause an
error code to be issued.
.P
A validation condition may be continued on another line by ending
the continued line with a backslash (\e).
Leading tabs and blanks on the continuation line are ignored.
String constants may not be continued on another line.
Comments do not have to be continued with a backslash;
successive lines are considered part of the comment until the */ is encountered.
.P
The example in EXAMPLE section should be studied
for further illustration of the format and flow through the validation table.
.SH "OTHER TABLES"
.TP 2
Description file
All fields referenced in the validation table must appear in the
description file.
A description of the layout of this file is given on the \fItable\fP(UNITY)
manual page.
An example description file appears the next section.
.TP 2
Error file
Messages corresponding to the error codes used in the validation
table are defined in the error table (see \fIvalidate\fP(UNITY)).
See the next section for an error table example.
.SH EXAMPLE
This section contains an example of a validation table,
a description file, and an error table.
In studying this example,
the reader should not be concerned with the appropriateness of validations,
choice of field names,
human-engineering of error messages, or any other real-life qualities.
The sole purpose is to illustrate the use of the three tables.
.nf
\fIValidation Table\fP
.sp
01 /* Non-numeric CO? */
02 !(CO % "[0-9]{3}")
03         /* 2LN? */
04         CO % "[A-Z]{2}[0-9]"                        co1
05                 /* 2nd letter can't be Q or Z */
06                 CO % ".[^QZ]."                      co2
07                 /* Manual mobile? */
08                 CO % "[JQYZ].."
09                         /* Legal letter combination? */
10                         CO % "J[KLPRS]." |\e
11                         CO % "Q[ABFJPR]." |\e
12                         CO % "Y[JKLPRS]." |\e
13                         CO % "Z[ABFHLMORWY]."       co3
14 /* List name required. */
15 !(LNAME == "")                                      lname1
16         /* List name must be capitalized alpha words. */
17         LNAME %% "[A-Z][a-z]*"                      lname2
18 /* House number must be <=5 digits. */
19 HSNM % "[0-9]{0,5}"                                 hsnm1
20 /* Street is required. */
21 !(STRT == "")                                       strt1
22         /* Street must be capitalized alpha words. */
23         STRT %% "[A-Z][a-z]*"                       strt2
24 /* NPA must be numeric. */
25 NPA % "[0-9]{3}"                                    npa1
26         /* Check NPA syntax (NPA table not available) */
27         NPA % "[2-9][01][1-9]"                      npa2
28 /* Line numeric. */
29 /* THRFR must be alpha. */
30 THRFR % "[A-Z][a-z]*"                               thrfr1
31         /* THRFR must be in table */
32         tblsearch("thrfrtbl","thrfr",THRFR,"thrfr") \e
                != ""      thrfr2
.fi
.nf
.sp
\fIDescription file\fP
.sp
LNAME     t:    20l     LISTING NAME
CO        t:    3l      CO
HSNM      t:    5l      HOUSE NUMBER
STRT      t:    20l     STREET
NPA       t:    3l      NPA
THRFR     t\\n  10l     THOROUGHFARE
.fi
.sp
.nf
\fIError Table\fP
.sp
co1        Non-numeric CO must be of the form 2LN.
co2        Second letter of 2LN CO can't be Q or Z.
co3        Illegal letter combination for Manual Mobile CO.
lname1     List name is required.
lname2     List name must be capitalized alpha words.
hsnm1      House number must consist of 5 digits or less.
strt1      Street is required.
strt2      Street must be capitalized alphabetic words.
npa1       NPA must consist of 3 digits.
npa2       Illegal digit combination for NPA.
line1      Line number must consist of 4 digits.
thrfr1     Thoroughfare designation must be capitalized word.
thrfr2     Thoroughfare designation must be in table.
.fi
.sp
The following notes will aid in understanding the validation table.
They are keyed to the line numbers appearing at the left of
the table above.
These numbers are not part of the table itself.
.TP 4
02
The part of the validation within parentheses is true if the CO consists of
exactly 3 digits from 0 to 9.
The ! operator inverts the truth value,
so the entire validation condition is true if the CO is non-numeric.
The indented validation conditions are concerned with non-numeric COs,
and they will be skipped if the CO is numeric.
.TP 4
04
The regular expression matches two capital letters from A to Z
followed by a digit from 0 to 9.
Since this test is not performed if the CO is numeric,
if the test fails error code co1 is issued and the validation machine
skips to line 15.
.TP 4
06
The bracketed string [^QZ] is adequate here only because the second letter
has already been tested on line 4.
If it had not been, any character, including one like "4", "g" or ":",
would match.
.TP 4
10
This validation condition consists of 4 regular expression matches connected by
the | (or) operator.
The validation condition is written on 4 lines for readability;
the backslash is used for continuation.
The error code co3 applies to the entire validation condition,
not just to line 13.
Note that it is possible for both error codes co2 and co3 to be issued for
the same set of field-value pairs.
.TP 4
17
The regular expression is coded to match one word of the list name.
If it consists of multiple words separated by blanks
(for example, "Springsteen\ Bruce"),
the %% operator will succeed only if every word is matched.
.TP 4
32
The table "thrfrtbl" is not defined by the validation module itself;
it was created as a separate unity table.
The  table "thrfrtbl" could be
.sp
.nf
Ave
Rd
St
Dock
.sp
with description file Dthrfrtbl containing the line:
.sp
thrfr   t\\n
.fi
.sp
The function \fItblsearch\fP looks for the string value of the attribute
THRFR in the "thrfr" column (the only attribute) and returns the value in
column "thrfr", namely the same string.
However, if it cannot find a match it returns a null string.
This makes the validation condition false, causing error code
thrfr2 to be issued.
Any number of tables may be used in \fItblsearch\fP commands.
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
.TP
UNITYUNPACK
A string of the form "command.suffix" where "command" is the the name of
a command that can be used to read (cat) the contents of a packed relation (file)
and "suffix" is the 1-3 character suffix (without the '.') that is appended to
the filename when the file has been packed (compressed).
Typical values for UNITYUNPACK are "pcat.z", "zcat.Z", and "gzcat.gz".
.IP
It should be noted that packed relations are not allowed as the target
of any type of insert, update, or delete operation.
In addition, using packed relations prevents the use of attribute indexes
and also prevents any packed tuples from being validated, see validate(UNITY).
See the New Unity setunpackenv(3) manual page for more information.
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIvalidate\fP(UNITY), \fIvalcmp\fP(UNITY)
