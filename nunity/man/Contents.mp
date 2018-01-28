.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH CONTENTS UNITY "" "UNITY User's Manual"
.in 0
.nf
Intro ..  introduction to UNITY commands and application programs
Uinthl ...........................  High Level Function Interface
Uintll .....................................  Low Level Interface
alter ...  alter the value of an attribute in one or more records
asort ....................  sort a table on a specified attribute
catalog .....  vertically formatted presentation of a UNITY table
compute .........  compute arithmetic expressions on UNITY tables
delete, udelete ..  delete one or more records from a UNITY table
dist, ddist ...  single and double distributions on a UNITY table
erase-index, ei ........................  erase UNITY index files
erase-table, et ..........  erase UNITY table and auxiliary files
format, uformat  ................  change format of a UNITY table
index .......  produce an index on an attribute for a UNITY table
insert ...................  insert new records into a UNITY table
load ........  load records between a UNIX file and a UNITY table
maketable ..................   interactive UNITY table definition
merge, unmerge .  join/split attribute values for given attribute
nprintf ..............  numeric/reformatted print on UNITY tables
outerjoin ..................  outerjoin two existing UNITY tables
packet .................  convert records to stream packet format
regjoin ..  join two existing tables based on regular expressions
setdiff ..  find set difference of attributes on two UNITY tables
stat ..  do elementary statistical analysis on numeric attributes
table ...........   define table name, attributes, and data types
tuple ................  converts data from packet to tuple format
uadmin ..........................  store a UNITY table under SCCS
ucheck ............................  check UNITY table attributes
udelta .........  make a delta to a UNITY table stored under SCCS
udisplay, udis ...  print description and/or data for UNITY table
uedit ............................  edit records in a UNITY table
uenter ........................  enter records into a UNITY table
uget ..........  get a version of a UNITY table stored under SCCS
ujoin, join .....................  join two existing UNITY tables
uman ........................  print man pages for UNITY commands
umenu ............................  menu access to UNITY commands
unity ...............................  access UNITY DBMS commands
unload ....  unload records between a UNITY table and a UNIX file
uprint, print ..........  formatted presentation of a UNITY table
uprintf ...................  user-formatted print on UNITY tables
uselect, select ...  do selection and projection on a UNITY table
val ........................  function to validate a UNITY record
valcmp ..............................  compile a validation table
validate ......................................  record validator
.TH CONTENTS NewUNITY "" "New UNITY User's Manual"
.in 0
.nf
cmpquery(1) .... compile a retrieve query to embed in a C program
nalter(1) ........... change attribute values in a UNITY relation
ndelete(1) ................. delete records from a UNITY relation
ninsert(1) ..................  insert records in a UNITY relation
nuedit(1) ......................... edit records in a UNITY table
prblock(1) ............. format input lines vertically in a block
prtable(1) ... format lines read from standard input into a table
qprintf(1) ... format lines .. according to user-specified format
retrieve(1) .............. retrieve information from UNITY tables
udiff(1) ................. compare the tuples in two UNITY tables
ulock(1) ................. remove the lock file for a UNITY table
umerge(1) .................  merge new tuples in a UNITY relation
usort(1) ..................................... sort a UNITY table

bldquery(3) .............. build a query structure from relation,
                          attribute, and where-clause information
do_insert(3) .................. insert tuples in a UNITY relation
exprojlist(3) ........ expand references to the ``all'' attribute
                                             in a projection list
getrelinfo(3) ......................... locate and read attribute
                                       information for a relation
freerelinfo(3) ........... free memory associated with a relation
lookupattr(3) ... locate information about a particular attribute
mkquery(3) .................... make a query from relation names,
                              attribute names, and a where-clause
nexttuple(3) ..................... traverse the result of a query
parsewhere(3) ................ parse the where-clause for a query
peruse(3) ............... step through tuples in a UNITY relation
                                       and optionally change them
prmsg(3) .............................. print a formatted message
queryeval(3) ................. find tuples matching a given query
setalloc(3) ............... set maximum tuple size in application
settplfunc(3) ................... set a function to be called for
                                  each retrieved tuple in a query
setunpackenv(3) ...... setup environment to read packed relations
uerror(3) ......................... UNITY error code variable and
                                     error message print routines
descriptor(4) ........ attribute information for a UNITY relation
