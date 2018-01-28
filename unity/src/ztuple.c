/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "db.h"
#include <ctype.h>

extern FILE *utfp1,*utfp2;
static struct fmt xx[MAXATT];
static char *taname,*dvalue,newline, *prog;
extern	char *malloc();
extern	char *strrchr();
extern	FILE *putdescrwtbl();
static int nattr,lines,retcode;

extern	char	*table2, Dtable2[], *stdbuf;
extern	char	Uunames[MAXATT][MAXUNAME+1];	/* in mktbl2.c */
static	int	exitcode;

#ifdef __CYGWIN__
static getval();
#endif

tuple(argc,argv)
int argc;
char *argv[];
{
	int i,reccount,have,end, coption, qoption, onto=0, uoption;
	char Dtable1[MAXPATH+4], *Ioption, *Ooption;
	char dtable2[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char *tblname;
	int j,size_newrec;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}

	newline=NLCHAR;
	uoption = qoption = coption = retcode = 0;

	Ooption = Ioption = stdbuf = table2 = NULL;
	dtable2[0] = '\0';
	Dtable2[0] = '\0';
	exitcode = 1;

	while ( argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		switch(argv[1][1]) {
			default:
				error(E_BADFLAG,prog,argv[1]);
				return(exitcode);
			case 'n':
				newline=argv[1][2];
				if ( newline == '\n' )
				{
					error( E_GENERAL, "%s: ERROR: cannot have '\\n' as newline character\n",
						prog );
					return( exitcode );
				}
				break;
			case 'I':
				Ioption = &argv[1][2];
				break;
			case 'O':
				Ooption = &argv[1][2];
				break;
			case 'c':
			case 'q':
			case 'u':
				for (i=1; argv[1][i]; i++) {

					switch (argv[1][i]) {
					case 'c':
						coption = 1;
						break;
					case 'q':
						qoption = 1;
						break;
					case 'u':
						uoption = 1;
						break;
					default:
						error(E_BADFLAG, prog, &argv[1][i]);
						return(exitcode);
					}
				}
				break;
		}
		argc--;
		argv++;
	}
	if(argc == 4 && strcmp(argv[2],"onto") == 0)
		onto = 1;
	if(argc != 2 && !(argc == 4 && (onto || strcmp(argv[2],"into")==0)) ) {
		error(E_GENERAL,
"Usage: %s [-c] [-q] [-Itable] [-Otable] [-n[<newline>]] [-u] table1 \\\n\t[into table2]\n",
			prog);
		return(exitcode);
	}

	if ((Ooption) && (argc != 4)) {
		error(E_GENERAL,
			"%s: Missing \"into table\" clause for the \"-O%s\" option.\n",
			prog, Ooption);
		return(exitcode);
	}

	if(strcmp(argv[1],"-") == 0) {
		if(!Ioption) {
			error(E_GENERAL,"%s: No description file specified.\n",
				prog);
			return(exitcode);
		}
		utfp1 = stdin;
	}
	else {
		if ((utfp1 = fopen(argv[1],"r")) == NULL ) { 
			error(E_DATFOPEN,prog,argv[1]);
			return(exitcode);
		}
	}

	if((nattr = mkntbl2(prog,argv[1],Dtable1,xx,Ioption))==ERR)
		return(exitcode);

	if (argc != 4) {
		utfp2 = stdout;
		if((stdbuf=malloc(BUFSIZ)) != NULL)
			setbuf(stdout,stdbuf);
		tblname = "-";		/* current output table name */
	} else {
		if(strcmp(argv[3],"-") != 0) {
			if(chkaccess(argv[3],00) == 0) {
				if(!onto) {
					error(E_EXISTS,prog,argv[3]);
					return(exitcode);
				}
			}
			else
				table2 = argv[3];
			if(onto)
				utfp2 = fopen(argv[3],"a");
			else
				utfp2 = fopen(argv[3],"w");
			if(utfp2 == NULL) { 
				error(E_DATFOPEN,prog,argv[3]);
				return(exitcode);
			}
			if (Ooption) {
				if (!(*Ooption)) {
					/* do not create output descriptor file */
					dtable2[0] = '\0';
				} else if (strcmp(argv[3], Ooption) == 0) {
					/*
					 * check for existing and/or create
					 * output descriptor in data directory
					 */
					getfile(dtable2, Ooption, 1);
				} else {
					/* check current directory */
					getfile(dtable2, Ooption, 0);
				}
			} else {
				getfile(dtable2,argv[3],0);
			}
		}
		else {
			if ((Ooption) && (*Ooption)) {
				getfile(dtable2,Ooption,0);
			} else {
				dtable2[0] = '\0';
			}
			utfp2 = stdout;
			if((stdbuf=malloc(BUFSIZ)) != NULL)
				setbuf(stdout,stdbuf);
		}
		if ((dtable2[0]) &&
		    (strcmpfile(Dtable1,dtable2) != 0)) {
			if(chkaccess(dtable2,00) == 0) {
				if(!onto) {
					error(E_EXISTS,prog,dtable2);
					return(exitcode);
				}
			} else {
				strcpy(Dtable2,dtable2);
				if(copy(prog,Dtable1,Dtable2)!=0)
					return(exitcode);
			}
		}
		tblname = argv[3];	/* current output table name */
	}

	if ((coption) && (utfp2 == stdout) && (Ooption == NULL)) {
		if (putdescrwtbl(stdout, Dtable1) == NULL) {
			error(E_DATFOPEN,prog,"-");
			return(exitcode);
		}
	}

	end=reccount=have=lines=0;

    while(1) {
	size_newrec = 0;

	for (i=0;i<nattr;i++) {
		if (!have) 	/* if i dont already have a line */
			if (getline()==0)
				end = 1;
			else
				have = 1;
		if(end)
			break;
		if(strcmp(taname,"#####") == 0) 	/* record number line */
			if(getline() == 0) {
				end = 1;
				break;
			}
			else
				have = 1;
		if(( uoption && strcmp(Uunames[i],taname) != 0) ||
		   (!uoption && strcmp(xx[i].aname,taname) != 0) ) {
			/* field not in packet */
			if ((j = fillfield(i)) == ERR) {
				error(E_DATFWRITE,prog,tblname);
				return(exitcode);
			}
			size_newrec += j;
		} else {
			if(xx[i].flag== WN) { /* fixed length field */
				if (strlen(dvalue)!=xx[i].flen) {
					error(E_GENERAL,
			"%s: Wrong data length, should be %d,line %d\n",
					prog,xx[i].flen,lines);
					retcode = ERR;
				}
				if ((j = valput(lines,'\0')) == ERR) {
					error(E_DATFWRITE,prog,tblname);
					return(exitcode);
				}
				size_newrec += j;
				have=0;
			}
			else {	/* variable length fields */
				char lastch;

				if ((j = valput(lines,xx[i].inf[1])) == ERR) {
					error(E_DATFWRITE,prog,tblname);
					return(exitcode);
				}
				size_newrec += j;

				lastch = dvalue[ strlen( dvalue ) - 1 ];

			/* this next section continues getting lines
			   which begin with a tab and takes them
			   as a continuation of the variable length
			   field, inserting a "newline" character
			   for each new row read
			*/
				if (!getline())
					end=1;
				else while(taname[0]=='\0') {
					lastch = dvalue[ strlen( dvalue ) - 1 ];

					if (newline != '\0')
					{
						if (putc(newline,utfp2) == EOF) {
							error(E_DATFWRITE,prog,tblname);
							return(exitcode);
						}
						size_newrec++;
					}
					if ((j = valput(lines,xx[i].inf[1])) == ERR) {
						error(E_DATFWRITE,prog,tblname);
						return(exitcode);
					}
					size_newrec += j;
					if (!getline()) {
						end=1;
						break;
					}
				}
				if ( lastch == '\\' )
				{
					error( E_GENERAL, "%s (line %d): last character of attribute value cannot be a backslash (\\)\n",
						prog, lines );
					retcode = ERR;
				}

				if (putc(xx[i].inf[1],utfp2) == EOF) {
					error(E_DATFWRITE,prog,tblname);
					return(exitcode);
				}
				size_newrec++;
				have = 1;
			}
		}
		if (i+1==nattr) {
			if (xx[i].flag==WN)
			{
				if (putc('\n',utfp2) == EOF) {
					error(E_DATFWRITE,prog,tblname);
					return(exitcode);
				}
				size_newrec++;
			}
			reccount++;
		}
	}
	if ( size_newrec > MAXREC )
	{
		error( E_GENERAL, "%s: Error: rec# %d would be longer (%d) than maximum record length (%d)\n",
			prog, reccount, size_newrec, MAXREC );
		retcode = ERR;
	}
	if (end)
		break;
	if(strcmp(taname,"#####") != 0 &&
		(( uoption && usetnum(taname,nattr) == ERR) ||
		(!uoption &&  setnum(xx,taname,nattr) == ERR)) ) {
		error(E_GENERAL,
		"%s: packet does not match table on line %d\n",prog,lines);
		retcode = ERR;
		if (!getline())
			break;
	}
    }
	if (i>0 && i < nattr) {
		for(;i<nattr;i++) {
			if ((j = fillfield(i)) == ERR) {
				error(E_DATFWRITE,prog,tblname);
				return(exitcode);
			}
			size_newrec += j;
		}
		if (xx[nattr-1].flag==WN) {
			if (putc('\n',utfp2) == EOF) {
				error(E_DATFWRITE,prog,tblname);
				return(exitcode);
			}
			size_newrec++;
		}
		reccount++;
		if ( size_newrec > MAXREC )
		{
			error( E_GENERAL,
				"%s: Error: rec# %d would be longer (%d) than maximum record length (%d)\n",
				prog, reccount, size_newrec, MAXREC );
			retcode = ERR;
		}
	}
	if(retcode == ERR)
		return(exitcode);
	if(!qoption)
		error(E_GENERAL,"%s: %d records converted\n",prog,reccount);

	Dtable2[0] = '\0';
	table2 = NULL;
	exitcode = 0;
	return(exitcode);
}

static int
valput(line,ter)
int line;
char ter;
{
	int k;
	char c;
	int cnt;

	cnt = 0;
	for (k=0;c=dvalue[k];k++)
		/* check for invalid characters in value as writing out */

		/* PSF: 9/19/06 - Some people use international characters
		 * with the high-order bit set. Let's allow them too.
		 */
		if ((isprint(c) || c == '\t' || (c & 0x80) ) &&
			c!='\n' && c!=newline)
		{
			if ( c == ter )
			{
				if (putc( '\\', utfp2 ) == EOF) {
					retcode = ERR;
					return(ERR);
				}
				cnt++;
			}
			if (putc(c,utfp2) == EOF) {
				retcode = ERR;
				return(ERR);
			}
			cnt++;
		}
		else {
			error(E_GENERAL,"%s: Illegal character on line %d, ",
				prog,line);
			if(isprint(c))
				error(E_GENERAL,"'%c'.\n",c);
			else if ( c == '\t')
				error(E_GENERAL,"'tab'.\n");
			else
				error(E_GENERAL,"ASCII code %o\n",c);

			if(c == newline)
				error(E_GENERAL,"UNITY newline character.\n");
			else
				error(E_GENERAL,"Non-printable characters not allowed.\n");
			retcode = ERR;
		}

	return( cnt );
}

static int
getline()
{
	int cd;

	lines++;
	while ((cd=getc(utfp1))=='\n')
		lines++;
	if(cd == EOF) return(0);
	ungetc(cd,utfp1);
	newpair();
	if((cd=getval(utfp1,'\t',&taname))==ERR) {
		retcode = ERR;
		return(0);
	}
	if(cd == 0) {
		if(getval(utfp1,'\n',&dvalue)==ERR) {
			retcode = ERR;
			return(0);
		}
	}
	else
		dvalue = "";
	return(1);
}

static int
fillfield(field)
int field;
{
	register int k;

	if (xx[field].flag==WN) {
		for(k=0;k<xx[field].flen;k++) {
			if (putc(' ',utfp2) == EOF) {
				retcode = ERR;
				return(ERR);
			}
		}
	} else {
		if (putc(xx[field].inf[1],utfp2) == EOF) {
			retcode = ERR;
			return(ERR);
		}
		k = 1;
	}
	return(k);
}

static char recbuf[MAXREC],*recptr;
static int
newpair()
{
	recptr=recbuf;
}

static int
getval(file,endchar,start)
FILE *file; 
char endchar;
char **start;
{	
	int c;
	char *s;

	*start = s = recptr;
	while ( (c=getc(file)) != endchar && c != '\n' && c != EOF)
		if(s != &recbuf[MAXREC])
			*s++ = c;
		else {
			error(E_GENERAL, "%s: maximum line size exceeded.\n",
				prog);
			return(ERR);
		}
	if(c == EOF) {
		error(E_GENERAL,
			"%s: unexpected end of line %d\n",prog,lines);
		return(ERR);
	}
	*s++='\0';
	recptr=s;
	if(c == endchar)
		return(0);
	else
		return(1);
}

static int
usetnum(s,fldnum)
char *s;
int fldnum;
{
	int	i;

	for(i=0;i<fldnum;i++)
		if(strcmp(s,Uunames[i])==0)
			return(i);
	return(ERR);
}
