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
extern	char	*mktemp(), *strcpy(), *malloc(), *strrchr();
extern  FILE    *getdescrwtbl();

extern	char	*table2, Dtable2[],*stdbuf;
extern  char    DtmpItable[];
extern	int	end_of_tbl;
static	int	exitcode;

static char *
charrep(c)
int c;
{	
	static char buf[8];
	switch(c) {
	case '\n':
		sprintf(buf,"\\n");
		break;
	case '\t':
		sprintf(buf,"\\t");
		break;
	case '\b':
		sprintf(buf,"\\b");
		break;
	case '\r':
		sprintf(buf,"\\r");
		break;
	case '\f':
		sprintf(buf,"\\f");
		break;
	case '\\':
		sprintf(buf,"\\\\");
		break;
	case '\'':
		sprintf(buf,"\\\'");
		break;
	default:
		if(!isprint(c))
			sprintf(buf,"\\%o",c);
		else
			sprintf(buf,"%c",c);
		break;
	}
	return(buf);
}

extern	FILE	*udfp2, *utfp1, *utfp2, *udfp1;
static	struct	fmt xx[MAXATT], yy[MAXATT];
static	int	nattr1, nattr2, yf[MAXATT], selnum, endf1, endf2, samedes;
static	char	*prog, *tblname;

uselect(argc, argv)
char	*argv[];
int	argc;
{
	struct index *tree;
	int	x, i, j, recordnr,op,indx,cnt,ret,indattr;
	int	qoption, coption, onto=0;
	char	*table1, Dtable1[MAXPATH+4],**p,*fndkey, *Ioption, *Ooption;
	char	dtable2[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	long	seekadr,num,seekval;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	qoption = selnum = endf1 = endf2 = coption = samedes = 0;  
	stdbuf = table2 = NULL;
	dtable2[0] = '\0';
	Dtable2[0] = '\0';
	DtmpItable[0] = '\0';
	Ooption = Ioption = NULL;
	exitcode = 1;

	while ( argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		switch(argv[1][1]) {
			default:
				error(E_BADFLAG,prog,argv[1]);
				return(exitcode);
			case 'I':
				Ioption = &argv[1][2];
				break;
			case 'O':
				Ooption = &argv[1][2];
				break;
			case 'c':
			case 'q':
				for (i=1; argv[1][i]; i++)
					switch (argv[1][i]) {
					case 'c':
						++coption;
						break;
					case 'q':
						qoption = 1;
						break;
					default:
						error(E_BADFLAG,prog,argv[1]);
						return(exitcode);
					}
				break;
		}
		argc--;
		argv++;
	}

	if (argc < 3) {
		usage(prog);
		return(exitcode);
	}
	if ((argc == 3) && (strcmp(argv[1],"from") == 0) &&
	    (coption == 0) && ((Ioption) || (strcmp(argv[2],"-") != 0))) {
	/* command - select from table */
		argv[0] = "/bin/cat";
		argv[1]=argv[2];
		argv[2] = 0;
		execv("/bin/cat",argv);
		error(E_GENERAL,"%s: Cannot exec %s\n",argv[0],"cat");
		return(exitcode);
	}

	for(x=1;x < argc && strcmp(argv[x],"from") != 0;x++)
		;

	if (x >= argc - 1) { 
		usage(prog);
		return(exitcode);
	}

	/* x is now index for "from" in command */

	table1 = argv[x+1];
	if(strcmp(table1,"-") == 0) {
		if(!Ioption) {
			/*
			 * Check for/get description from the input file.
			 */
			if (getdescrwtbl(stdin, &Ioption) == NULL) {
				error(E_GENERAL,
					"%s: Description file not specified.\n", prog);
				return(exitcode);
			}
		}
		utfp1 = stdin;
	}
	else {
		if ((utfp1 = fopen(table1,"r")) == NULL ) { 
			error(E_DATFOPEN,prog,table1);
			return(exitcode);
		}
	}

	if((nattr1 = mkntbl(prog,table1,Dtable1,xx,Ioption))==ERR)
		return(exitcode);
	if (xx[nattr1-1].flag == WN) endf1 = 1;

	if (x == 1) {
	 	/* no projection */
		for(i=0;i<nattr1;i++) {
			yf[i] = i;
			yy[i].flag = xx[i].flag;
			yy[i].flen = xx[i].flen;
			strcpy(yy[i].aname, xx[i].aname);
			strcpy(yy[i].inf, xx[i].inf);
			yy[i].prnt = xx[i].prnt;
			yy[i].justify = xx[i].justify;
		}
		samedes = 1;	/* both desc files the same */
		endf2 = endf1;
		nattr2 = nattr1;
	}
	else {	/* do projection */
		for(i=1,nattr2=0;i<x;i++,nattr2++) {	
			if((j=setnum(xx,argv[i],nattr1))==ERR) {
				error(E_ILLATTR,prog,argv[i],Dtable1);
				return(exitcode);
			}
			yf[nattr2] = j;
			if (i+1<x && strcmp(argv[i+1],"as")==0) {
				if(i+2 >= x) {
					error(E_GENERAL,
			"%s: 'as' specified with no matching attribute name.\n",
						prog);
					return(exitcode);
				}
				i = i + 2;
			}
			strcpy(yy[nattr2].aname,argv[i]);
			/* field type and length/terminator */
			yy[nattr2].flag = xx[j].flag;
			switch(xx[j].flag) {
			case WN:
				yy[nattr2].flen = xx[j].flen;
				break;
			case T:
				yy[nattr2].inf[0] = 't';
				if (i==x-1) {/* last attribute of output file */
					yy[nattr2].inf[1] = '\n';
				}
				else {	/* not last attr of output file */
					if (j==nattr1-1) {
						yy[nattr2].inf[1] = '\t';
					}
					else {
						yy[nattr2].inf[1] =
							xx[j].inf[1];
					}
				}
				break;
			}
			/* print print width and justification */
			yy[nattr2].prnt = xx[j].prnt;
			yy[nattr2].justify = xx[j].justify;
		}
		if(yy[nattr2-1].flag == WN)
			endf2 = 1;
	}

	x += 2;		/* x now points to any "into" or where clause */

	onto = ( x < argc && strcmp(argv[x],"onto") == 0 );

	if(!onto && (x >= argc || strcmp(argv[x],"into") != 0 ) ) {

		if (Ooption) {
			error(E_GENERAL,
				"%s: Missing \"into table\" clause for the \"-O%s\" option.\n",
				prog, Ooption);
			return(exitcode);
		}

		utfp2 = stdout;
		if((stdbuf=malloc(BUFSIZ)) != NULL)
			setbuf(stdout,stdbuf);
		tblname = "-";		/* current output table name */
	}
	else if ( x + 1 >= argc ) {
		usage(prog);
		return(exitcode);
	}
	else {
		++x;	/* skip "into" or "onto" */

		if ( strcmp(argv[x],"-") != 0) {
			if(chkaccess(argv[x],00) == 0) {
				if(!onto) {
					error(E_EXISTS,prog,argv[x]);
					return(exitcode);
				}
			}
			else
				table2 = argv[x];
			if(onto)
				utfp2 = fopen(argv[x],"a");
			else
				utfp2 = fopen(argv[x],"w");
			if(utfp2 == NULL) { 
				error(E_DATFOPEN,prog,argv[x]);
				return(exitcode);
			}
			if (Ooption) {
				if (!(*Ooption)) {
					/* do not create output descriptor file */
					dtable2[0] = '\0';
				} else if (strcmp(argv[x], Ooption) == 0) {
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
				getfile(dtable2,argv[x],0);
			}
		} else {
			if ((Ooption) && (*Ooption)) {
				getfile(dtable2,Ooption,0);
			} else if ((!(Ooption)) && (coption)) {
				/* make sure dtable2 is null ("") */
				dtable2[0] = '\0';
			} else if ((samedes) && (Ooption) && (!(*Ooption))) {
				/* make sure dtable2 is null ("") */
				dtable2[0] = '\0';
			} else {
				error(E_GENERAL,
					"%s: No output description file specified.\n",
					prog);
				return(exitcode);
			}
			utfp2 = stdout;
		}
		tblname = argv[x];	/* current output table name */

		/* check output description file */
		if (dtable2[0]) {
			if(chkaccess(dtable2,00) == 0) {
				if(!onto && !(samedes && strcmpfile(Dtable1,dtable2) == 0)){
					error(E_EXISTS,prog,dtable2);
					return(exitcode);
				}
				dtable2[0] = '\0';
			}
		}

		++x;	/* skip file name */
	}

	/* parse where clause and build evaluation tree */
	p = &argv[x];
	indx = getcond(p,xx,nattr1,prog,Dtable1);
	if (utfp1 == stdin)
		indx = 0;	/* can't use index */

	/* create output description/file if needed */
	if ((*dtable2) || ((coption) && (utfp2 == stdout) && (Ooption == NULL))) {
		strcpy(Dtable2,dtable2);
		if (samedes) {
			if (*Dtable2) {
				if (copy(prog,Dtable1,Dtable2)!=0)
					return(exitcode);
			} else {
				if (putdescrwtbl(stdout, Dtable1) == NULL) {
					error(E_DATFOPEN,prog,"-");
					return(exitcode);
				}
			}
		} else {
			FILE *dfp;

			if (*Dtable2) {
				if ((udfp2=fopen(Dtable2,"w"))==NULL) {	
					error(E_DESFOPEN,prog,Dtable2);
					return(exitcode);
				}
				dfp = udfp2;
			} else {
				fprintf(stdout, "%%description\n");
				dfp = stdout;
			}
			for(i=0;i<nattr2;i++) {
				fprintf(dfp,"%s\t",yy[i].aname);
				switch(yy[i].flag) {
				case WN:
					fprintf(dfp,"w%d",yy[i].flen);
					break;
				case T:
					fprintf(dfp,"t%s",
						charrep(yy[i].inf[1]));
					break;
				}
				/*
				 * print print width, justification,
				 * and null friendly attribute field
				 */
				fprintf(dfp,"\t%d%c\t\n",yy[i].prnt,
					yy[i].justify);
			}
			if (dfp == stdout) {
				fprintf(stdout, "%%enddescription\n");
				if (fflush(stdout) == EOF) {
					error(E_DATFWRITE,prog,"-");
					return(exitcode);
				}
			} else {
				if (fclose(udfp2) == EOF) {
					error(E_DATFWRITE,prog,Dtable2);
					return(exitcode);
				}
				udfp2 = NULL;
			}
		}
	}

	recordnr=0;
	/* handle indexing - only on first attribute that appears -
		somewhat cludgey but does save much time on some queries
	*/
	if( indx ) {
		/* no 'or' at level equal or lower than first attr */
		for(i=x+1; i + 1 < argc && argv[i][0] == '(';i++)
			;
		op = getop( i + 1 < argc ? argv[i+1] : "" );
		if( op == LLT || op == LLE ||
			op == LEQ || op == LNE || op == LGE || op == LGT ) {
			indx = indexch(table1,argv[i],"\0","\0",&udfp1,&tree);
		}
		else
			indx = ERR;
	}
	else
		indx = ERR;
	if(indx==0) {
	/* index used only
	   1. only on first attribute in where clause
	   2. only if no 'or' at level <= to level of first attr
	   3. only if operator is a lexical comparison and not against a field
	   4. only if index files exist
	*/
		indattr = i+2;
		switch(op) {
		case LLT:
		case LLE:
		case LNE:
			ret=rdindexed(tree,"\0",&fndkey,&seekval);
			break;
		case LEQ:
		case LGE:
		case LGT:
			ret=rdindexed(tree,argv[indattr],&fndkey,&seekval);
			break;
		}
	
		while(ret != END) {
			if(op == LGT && ret == FOUND)
				if(rdnext(tree,&fndkey,&seekval) == END)
					break;
	
			while(compb(op,fndkey,argv[indattr],0,0)) {
				fseek(udfp1,seekval,0);
				fread((char *)&num,sizeof(long),1,udfp1);
				cnt = -num;

				/* get records */
				for(j=0;j<cnt;j++) {
					fread((char *)&seekadr,
						sizeof(long),1,udfp1);
					fseek(utfp1,seekadr,0);
					end_of_tbl = 0;
					newrec();
					for(i=0; i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
					if (( i == 0 ) && ( end_of_tbl ))
						break;
					else if ( i < nattr1 )
					{
						error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of record at seek# %ld in %s\n",
							prog, i, seekadr, table1 );
						return( exitcode );
					}
					recordnr++;

					/* check record */
					if(selct(xx,recordnr)) 	/* selected */
						if (put() == ERR)
							return(exitcode);
				}
	
				if((ret=rdnext(tree,&fndkey,&seekval)) == END)
					break;
			}
			if(op != LNE || ret == END)
				break;
	
			/* LNE - look for keys above equal key */
			ret = rdnext(tree,&fndkey,&seekval);
			if(ret == END)
				break;
		}
		close(tree->fd);
	}
	else {
		end_of_tbl = 0;
		for(;;) { 
			newrec(); 
			for(i=0; i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
			if (( i == 0 ) && ( end_of_tbl ))
				break;
			else if ( i < nattr1 )
			{
				error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
					prog, i, recordnr + 1, table1 );
				return( exitcode );
			}
			if (endf1) 
				if ((feof(utfp1)) || ((i = getc(utfp1)) == EOF)) {
					error( E_GENERAL,
						"%s: Error: missing newline on rec# %d in %s\n",
						prog, recordnr + 1, table1 );
					return(exitcode);
				} else if (i != '\n') {
					error( E_GENERAL,
						"%s: Error: data overrun on rec# %d in %s\n",
						prog, recordnr + 1, table1 );
					return(exitcode);
				}
			recordnr++;
			if(selct(xx,recordnr)) 	/* record selected */
				if (put() == ERR)
					return(exitcode);
		}
	}

	if (fflush(utfp2) == EOF) {
		error(E_DATFWRITE, prog, tblname);
		return(exitcode);
	}
	if (utfp2 != stdout)
		if (fclose(utfp2) == EOF) {
		error(E_DATFWRITE, prog, tblname);
		return(exitcode);
	}
	utfp2 = NULL;

	if (utfp1 != stdin)
		fclose(utfp1);
	utfp1 = NULL;

	if(!qoption)
		error(E_GENERAL,"%s: %d records selected from %s.\n", prog,
		selnum, table1);

	exitcode = 0;
	table2 = NULL;
	Dtable2[0] = '\0';
	return(exitcode);
}

static int
put()
{
	int i;
	selnum++;
	if (samedes == 1)	{	/* no projection */
		for(i=0;i<nattr1;i++)
			if (putnrec(utfp2,&xx[i]) < 0) {
				error(E_DATFWRITE,prog,tblname);
				return(ERR);
			}
		if (endf1 == 1)
			if (putc('\n',utfp2) == EOF) {
				error(E_DATFWRITE,prog,tblname);
				return(ERR);
			}
	}
	else {	/* with projection */
		for(i=0; i<nattr2; i++)
		{
			yy[i].val = xx[ yf[i] ].val;
			if (putnrec(utfp2,&yy[i]) < 0) {
				error(E_DATFWRITE,prog,tblname);
				return(ERR);
			}
		}
		if (endf2 == 1)
			if (putc('\n',utfp2) == EOF) {
				error(E_DATFWRITE,prog,tblname);
				return(ERR);
			}
	}
	return(0);
}

/* PSF added HAVE_SELECT - other machines types have conflict also */
#if ( ! defined(SUNOS) && ! defined(HAVE_SELECT) )

/* for upward compatibility */
select(argc, argv)
char	*argv[];
int	argc;
{
	return(uselect(argc,argv));
}

#endif

static int
usage(prog)
char *prog;
{
	error(E_GENERAL, "Usage: %s %s \\\n\t%s\n", prog,
		"[-c] [-q] [-Itable] [-Otable] [aname [as aname2] ...]",
		"from table1 [into table2] [where clause]");
}
