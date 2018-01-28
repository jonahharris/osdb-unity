.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH "Low Level Interface" "UNITY" "" ""
.SA 1
.SH NAME
Uintll \- Low Level Interface
.P
.nf
check \- check format of input file
compb \- do comparison of two values based on operator
copy \- copy one file to another
dirname \- get directory name of pathname
error \- print UNITY error message
getcond \- compile UNITY conditional expression (where clause)
getfile \- get description file name from table name
get2file \- get auxiliary file name from table name
getop \- get comparison operator code for operator
getrec \- get next attribute value
indexch \- check to see if index available for attribute on file
mklock \- make lockfile for table
mkntbl \- read description file and fill in C-structure with information,
         throw away user\-friendly names
mkntbl2 \- read description file and fill in C-structure with information,
          store user\-friendly names in Uunames
mvgbt \- memory copy based on memory addresses
ncheck \- check format of input file and limit number of error messages
ndcheck \- compute checksum on b-tree node (block)
ndread \- read node (block) on b-tree
newrec \- initialize for reading new record
putrec \- write next attribute value
putnrec \- write next attribute value and report I/O errors as RC < 0
rdindexed \- read key value from index
rdnext \- read next key value from index
scan \- scan b-tree block for key value
selct \- evaluate UNITY conditional expression (where clause)
setnum \- get attribute number in table
sprintfb \- convert unsigned integer to binary digit string
ueval \- evaluate UNITY arithmetic expression
uexpr \- compile UNITY arithmetic expression
.fi
.SH SYNOPSIS
.nf
check(infile,table)/* obsolete function - use ncheck() instead */
char *infile;      /* input file to check */
char *table;       /* table whose description file is used for check */

compb(op,s,v,sibase,vibase)
int op;            /* operator */
char *s;           /* first operand */
char *v;           /* second operand */
int sibase;        /* base of ``s'' if integer (0 = floating point) */
int vibase;        /* base of ``v'' if integer (0 = floating point) */

copy(prog,source, target)
char *prog;        /* main program */
char *source;      /* source file name */
char *target;      /* target file name */

char *
dirname(p)
char *p;           /* pathname */


error(n,s,a,b,c,d,e)
int n;             /* UNITY error type */
char *s;           /* error string */


getcond(p,x,n,pr,dfile)
char **p;         /* expression */
char *pr;         /* main program name */
struct fmt x[];   /* array of format information for table */
int n;            /* number of attributes for table */
char *dfile;      /* description file name */


getfile(Dtable, table, dir)
char *Dtable;      /* output description file name */
char *table;       /* input table name  */
int dir;           /* if 0, check current directory
                      if 1, check table directory
                      if 2, check UNITYDFILES directory
                      if 'c', check current directory first
                      if 'd', check data directory first
                      if 'u', check UNITYDFILES first
                      if 's', check using standard search order */


FILE *
get2file(type, prefix, iname, oname, amode)
int type;          /* if 0, check current directory
                      if 1, check table directory
                      if 2, check UNITYDFILES path directories
                      if 'c', check current directory first
                      if 'd', check data directory first
                      if 'u', check UNITYDFILES first
                      if 's', check using standard search order */
char *prefix;      /* prefix to add to simple file name */
char *iname;       /* input file name */
char *oname;       /* output file name */
char *amode;       /* open mode (passed to fopen()) */


getop(s)
char *s;           /* operator string */


getrec(file,fmt)
FILE *file;        /* input file */
struct fmt *fmt;   /* format of next attribute */

indexch(file0, attr0, file1, attr1, list, btree)
char *file0;       /* first table to check */
char *attr0;       /* attribute on first table */
char *file1;       /* second table to check - may be NULL */
char *attr1;       /* attribute on second table */
struct index **btree; /* information about btree if found */
FILE **list;       /* file descriptor for list file with btree */

mklock(file,prog,lockfile)
char *file;         /* table name */
char *prog          /* main program name */
char *lockfile;     /* lock file name returned */

mkntbl(prog, table, Dtable, fmt, Itable)
char *prog;         /* main program name */
char *table;        /* table name */
char *Dtable;       /* description file name returned */
struct fmt *fmt;    /* array with format of all attributes returned */
char *Itable;       /* alternate table name (may be null) */

mkntbl2(prog, table, Dtable, fmt, Itable)
char *prog;         /* main program name */
char *table;        /* table name */
char *Dtable;       /* description file name returned */
struct fmt *fmt;    /* array with format of all attributes returned */
char *Itable;       /* alternate table name (may be null) */

mvgbt(n, len, a, b) 
int n;              /* number of items to move */
int size;           /* size of each item */
char *a;            /* to address */
char *b;            /* from address */

ncheck(infile,table,errlimit,errors,records)
char *infile;      /* input file to check */
char *table;       /* table whose description file is used for check */
int  errlimit;     /* maximum number of error messages to be printed */
int  *errors;      /* return count of the number of errors found */
int  *records;     /* return count of the number of records checked */

ndcheck(x)
int	*x;         /* buffer on which to compute checksum */

ndread(n,x,s)
int	n;          /* file descriptor */
long	x;          /* seek offset */
struct hdr	*s; /* pointer to btree block */

newrec()            /* initialize for new record */


putrec(file, fmt)   /* obsolete function - use putnrec() instead */


putnrec(file, fmt)  /* I/O error occurred if return code < 0 */
FILE *file;         /* output file */
struct fmt *fmt;    /* format and value of next attribute */

rdindexed(ind, key, keyval, seekval)
struct index *ind;/* index information */
char *key;        /* search key */
char **keyval;    /* found key value returned */
long *seekval;    /* seek value in accession file returned */

rdnext(ind, keyval, seekval)
struct index *ind;/* index information */
char **keyval;    /* return address of key found */
long *seekval;    /* seek value associated with key */

scan(key, ind, seekval)
char	*key;       /* key value to search for */
long	*seekval;   /* seek value associated with found key */
struct	index *ind; /* btree block and information */

selct(x,r)
struct fmt x[];   /* array of format information and values */
int r;            /* record number */

setnum(fld, s, fldnum)
struct fmt *fld;  /* array of format information for table */
char *s;          /* attribute name */
int fldnum;       /* number of attributes for table (array limit) */

sprintfb(s, format, number)
char    *s;       /* destination for binary output string */
char    *format;  /* "%b" type format specification */
unsigned number;  /* number to be converted to binary digit string */

uexpr(e,x,n,pr,dfile)
char **e;        /* expression */
char *pr;        /* main program name */
struct fmt x[];  /* array of format information for table */
int n;           /* number of attributes for table */
char *dfile;     /* description file name */


#ifdef INTCOMP
int
#else
float
#endif
ueval(x)
struct fmt x[];   /* array of format information and values */


.fi
.SH DESCRIPTION
These functions are the low level functions used by all UNITY
programs.
.SH EXAMPLE
.nf
/* read table and print out only records specified
   by where clause */
#include <stdio.h>
#include <sys/types.h>
#include "db.h"

main(argc, argv)
char    *argv[];
int     argc;
{
        extern  int end_of_tbl;
        extern  int term_escapes;

        FILE    *utfp1;
        struct  fmt xx[MAXATT];
        int     nattr1, endf1, recordnr, i, j;
        char    Dtable1[MAXPATH+4];
        char    *prog, *p;
        off_t   size_newtable;

        recordnr = endf1 = 0;
        stdbuf = NULL;
        prog = argv[0];

        if ((utfp1 = fopen(argv[1],"r")) == NULL ) {
                error(E_DATFOPEN,prog,argv[1]);
                exit(1);
        }

        /* parse where clause and build evaluation tree */
        p = &argv[3];
        getcond(p,xx,nattr1,prog,Dtable1);

        size_newtable = term_escapes = 0;
        recordnr = deleted = end_of_tbl = 0;
        for(;;) {
                /* get a new record */
                newrec();
                for(i=0; i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
                if ((i==0) && (end_of_tbl)) break;
                if (i<nattr1) {
                        error(E_GENERAL,
                            "%s: %s %d of rec# %d in %s\\n", prog,
                            "Error: record parsing failed on attribute",
                            i, recordnr+1, argv[1]);
                        exit(1);
                }
                if (endf1) {
                        if ((feof(utfp1)) || ((i=getc(utfp1))==EOF)) {
                                error(E_GENERAL,
                                    "%s: %s on rec# %d in %s\\n",
                                    prog, "Error: missing newline",
                                    recordnr+1,argv[1]);
                                exit(1);
                        } else if (i != '\\n') {
                                error(E_GENERAL,
                                    "%s: %s on rec# %d in %s\\n",
                                    prog, "Error: data overrun",
                                    recordnr+1,argv[1]);
                                exit(1);
                        }
                }
                recordnr++;
                if(selct(xx,recordnr)) {        /* record selected */
                        /* write record unchanged */
                        for(i=0; i<nattr1; i++) {
                                if ((j=putnrec(stdout,&xx[i])) < 0) {
                                        error(E_DATFWRITE,prog,"-");
                                        exit(1);
                                }
                                size_newtable += j;
                        }
                        if (endf1) {
                                if ((putc('\\n',stdout) == EOF) {
                                        error(E_DATFWRITE,prog,"-");
                                        exit(1);
                                }
                                ++size_newtable;
                        }
                }
                /* otherwise record not printed */
        }
        /*
         * For total ouput table size we also need
         * to add the number of termination character
         * escapes (backslashes) that were printed.
         */
        size_newtable += term_escapes;

        return(0);
}
.fi
.SH FILES
.nf
lib/dblib.a - archive file containing the object modules
.fi
.SH "SEE ALSO"
\fIIntro\fP(UNITY), \fIUinthl\fP(UNITY)
