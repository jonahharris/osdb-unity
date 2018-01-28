/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* This function does a double distribution			*/
/* includes dtally command					*/
#include "db.h"
#include <ctype.h>
#define NOVALUE -1e30
#define MAXROW 20	/* initial number of rows */
#define I_INCR 10	/* size of increase in number of rows */
#define MAXCOL MAXATT	/* number of columns - if this is changed,
			   it must not exceed MAXATT as defined in db.h */

static	char	**rname, *cname[MAXCOL];
static	int	rcnt, ccnt;	/* next row, col to be used */
static	int	row_size;	/* current number of rows */
static	double	**ddtbl;	/* the results */
extern	char	*strcpy(), *strrchr(), *malloc(), *realloc();
extern	FILE	*getdescrwtbl();
extern	FILE	*packedopen();
extern	int	packedclose();

extern	char	*packedsuffix;
extern	char	*table2, Dtable2[],*stdbuf;
extern	char	DtmpItable[];
extern FILE	*utfp1,*utfp2,*udfp2;
extern	int	end_of_tbl;
static	int	exitcode, dtally;

ddist(argc, argv)
int	argc;
char	*argv[];
{
	struct	fmt xx[MAXATT];
	unsigned len;
	int nattr1,afld1,afld2,afld3,i,j,k,rexist,cexist,seqno,floating;
	int errflg,sign,signcnt,started,deccnt,init,endf1,sortflg;
	double total[MAXCOL];
	char *p1, *p2, *p3, *q, Dtable1[MAXPATH+4], *Ioption, *Ooption;
	char dtable2[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	*tblname;
	double atof(), num;
	char	*prog, **parms, *table1;
	int	fromptr, intoptr, whereptr, byptr, prntflg, onto, coption, packed;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}

	onto = dtally = coption = packed = 0;
	sortflg = 1;
	prntflg = 0;
	floating = 0;

	Ooption = Ioption = stdbuf = table2 = NULL;
	udfp2 = NULL;
	dtable2[0] = '\0';
	Dtable2[0] = '\0';
	DtmpItable[0] = '\0';
	floating = seqno = rcnt = ccnt = 0;
	exitcode = 1;

	while ( argc > 1 &&  argv[1][0] == '-' && argv[1][1] != '\0') {
		switch(argv[1][1]) {
		case 'I':
			Ioption = &argv[1][2];
			break;
		case 'O':
			Ooption = &argv[1][2];
			break;
		case 'c':
		case 'n':
			for (i = 1; argv[1][i]; i++)
				switch (argv[1][i]) {
				case 'c':
					++coption;
					break;
				case 'n':
					sortflg = 0;
					break;
				default:
					error(E_BADFLAG,prog,&argv[1][i]);
					return(exitcode);
				}
			break;
		default:
			error(E_BADFLAG,prog,argv[1]);
			return(exitcode);
		}
		argc--;
		argv++;
	}

	for(i=1,fromptr = 0;i < argc; i++)	/* find "from" or "in" */
		if(strcmp(argv[i],"from") == 0 || strcmp(argv[i],"in") == 0) {
			fromptr = i;
			break;
		}
	for(i=1,intoptr = 0;i < argc; i++) /* find "into" if there is one*/
		if(strcmp(argv[i],"into") == 0 ||
		   strcmp(argv[i],"onto") == 0) {
			intoptr = i;
			if(argv[i][0] == 'o')
				onto = 1;
			break;
		}
	for(i=1,whereptr = 0;i < argc; i++) /* find "where" if there is one*/
		if(strcmp(argv[i],"where") == 0) {
			whereptr = i;
			break;
		}
	if(whereptr == 0)	/* make sure whereptr set ok for getcond() */
		if(intoptr == 0)
			whereptr = fromptr + 2;
		else
			whereptr = fromptr + 4;

	/* try to check for correct syntax */
	if(fromptr == 0 ||
	!( (intoptr == 0 && ( (whereptr == 0 && fromptr == argc - 2) ||
			       (whereptr > 0 && fromptr == whereptr - 2))) ||
	    (intoptr > 0 && fromptr == intoptr - 2  &&
			(whereptr == 0 || fromptr == whereptr - 4)) )  )
		exitcode = 1;
	else
		exitcode = 0;

	/* check command name */
	if(strcmp(prog,"dtally") == 0) {
		dtally = 1;
		if(exitcode == 1 || fromptr != 3) {
			error(E_GENERAL,
"Usage: %s [-c] [-n] [-Itable] [-Otable] aname1 aname2 in table1 \\\n\t[into table2] [where clause]\n",
				prog);
			exitcode = 1;
			return(exitcode);
			
		}
	}
	else {
		for(i=1,byptr = 0;i < argc; i++) /* find "by" */
			if(strcmp(argv[i],"by") == 0) {
				byptr = i;
				break;
			}
		if(exitcode == 1 || fromptr != 5 || byptr != 2) {
			error(E_GENERAL,
"Usage: %s [-c] [-n] [-Itable] [-Otable] {aname1|count} by aname2 aname3 \\\n\tin table1 [into table2] [where clause]\n",
				prog);
			exitcode = 1;
			return(exitcode);
		}
		if(strcmp(argv[byptr-1],"count") == 0)
			dtally = 1;
	}
	exitcode = 1;

	if ((Ooption) && (intoptr == 0)) {
		error(E_GENERAL,
			"%s: Missing \"into table\" clause for the \"-O%s\" option.\n",
			prog, Ooption);
		return(exitcode);
	}

	table1 = argv[fromptr+1];
	if(strcmp(table1,"-") == 0) {
		if(!Ioption) {
			/*
			 * Check for/get description from the input file.
			 */
			if (getdescrwtbl(stdin, &Ioption) == NULL) {
				error(E_GENERAL,
					"%s: No description file specified.\n", prog);
				return(exitcode);
			}
		}
		utfp1 = stdin;
	}
	else {
		if ((utfp1 = fopen(table1,"r")) == NULL ) { 
			if (((intoptr) && (strcmp(table1,argv[intoptr+1]) == 0)) ||
			    ((utfp1 = packedopen(table1)) == NULL)) {
				error(E_DATFOPEN,prog,table1);
				return(exitcode);
			}
			++packed;	/* reading packed table */
		}
	}

	if((nattr1 = mkntbl(prog,table1,Dtable1,xx,Ioption))==ERR) {
		if (packed) {
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}
	if (xx[nattr1-1].flag == WN)
		endf1 = 1;
	else
		endf1 = 0;

	/* no longer need temporary input description */
	if (DtmpItable[0]) {
		(void) unlink(DtmpItable);	/* created by getdescrwtbl() */
		DtmpItable[0] = '\0';
	}

	if((afld2 = setnum(xx,argv[fromptr-2],nattr1))==ERR) {
		error(E_ILLATTR,prog,argv[fromptr-2],Dtable1);
		if (packed) {
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}
	if((afld3 = setnum(xx,argv[fromptr-1],nattr1))==ERR) {
		error(E_ILLATTR,prog,argv[fromptr-1],Dtable1);
		if (packed) {
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}
	if(!dtally) {
		if((afld1 = setnum(xx,argv[byptr-1],nattr1))==ERR) {
			error(E_ILLATTR,prog,argv[byptr-1],Dtable1);
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}
	}

	/* parse where clause and build evaluation tree */
	parms = &argv[whereptr];
	getcond(parms,xx,nattr1,prog,Dtable1);

	if(intoptr == 0) {
		prntflg = 1;			/* print to stdout */
		utfp2 = stdout;
		if((stdbuf=malloc(BUFSIZ)) != NULL)
			setbuf(utfp2,stdbuf);
		coption = 0;		/* ignore create table description requests */
		tblname = "-";		/* current output table name */
	}
	else {
		if(strcmp(argv[intoptr+1],"-") != 0) {
			if(chkaccess(argv[intoptr+1],00) == 0) {
				if(!onto) {
					error(E_EXISTS,prog,argv[intoptr+1]);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
			}
			else
				table2 = argv[intoptr+1];
			if(onto)
				utfp2 = fopen(argv[intoptr+1],"a");
			else
				utfp2 = fopen(argv[intoptr+1],"w");
			if(utfp2 == NULL) { 
				error(E_DATFOPEN,prog,argv[intoptr+1]);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			if (Ooption) {
				if (!(*Ooption)) {
					/* do not create output descriptor file */
					dtable2[0] = '\0';
				} else if (strcmp(argv[intoptr+1], Ooption) == 0) {
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
				getfile(dtable2,argv[intoptr+1],0);
			}
		}
		else {
			if ((Ooption) && (*Ooption)) {
				getfile(dtable2,Ooption,0);
			} else if ((!(Ooption)) && (coption)) {
				/* make sure dtable2 is null ("") */
				dtable2[0] = '\0';
			} else {
				error(E_GENERAL,
					"%s: No output description file specified.\n",
					prog);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			utfp2 = stdout;
			if((stdbuf=malloc(BUFSIZ)) != NULL)
				setbuf(utfp2,stdbuf);
		}
		tblname = argv[intoptr+1];	/* current output table name */

		/* check/create output description file */
		if (dtable2[0]) {
			if(chkaccess(dtable2,00) == 0) {
				if (!onto) {
					error(E_EXISTS,prog,dtable2);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
				dtable2[0] = '\0';
			}
		}
		if (dtable2[0]) {
			coption = 0;
			strcpy(Dtable2,dtable2);
		} else {
			if ((utfp2 != stdout) || (Ooption))
				coption = 0;
		}
	}

	/* initialize the dynamic arrays.  written with intent
	   of eventually making columns dynamic.
	   problem - if user hits break, uclean() does not
	   clean up the malloc'ed space */
	if((ddtbl=(double **)malloc(MAXROW * sizeof(double *))) == NULL) {
		error(E_SPACE,prog);
		if (packed) {
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}
	for(i = 0; i < MAXROW; i++) {
		if((ddtbl[i]=(double *)malloc(MAXCOL * sizeof(double)))==NULL){
			error(E_SPACE,prog);
			for(j=0; j < i-1; j++)
				free((char *)ddtbl[j]);
			free((char *)ddtbl);
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}
		if(dtally) {
			for(j=0; j < MAXCOL; j++)
				ddtbl[i][j] = 0.0;
		}
		else {
			for(j=0; j < MAXCOL; j++)
				ddtbl[i][j] = NOVALUE;
		}
	}
	row_size = MAXROW;
	if((rname=(char **)malloc(MAXROW*sizeof(char *))) == NULL) {
		error(E_SPACE,prog);
		for(i=0; i < row_size; i++)
			free((char *)ddtbl[i]);
		free((char *)ddtbl);
		if (packed) {
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}

	end_of_tbl = 0;

	for(;;) {	/*   top of infinite loop   */
		/* get next record */
		newrec();
		for(i=0; i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
		if ((i == 0) && (end_of_tbl))
			break;
		else if (i < nattr1) {
			error(E_GENERAL,
				"%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
				prog, i, seqno+1, table1);
			clean();
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}
		if (endf1) 
			if ((feof(utfp1)) || ((i = getc(utfp1)) == EOF)) {
				error( E_GENERAL,
					"%s: Error: missing newline on rec# %d in %s\n",
					prog, seqno + 1, table1 );
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			} else if (i != '\n') {
				error( E_GENERAL,
					"%s: Error: data overrun on rec# %d in %s\n",
					prog, seqno + 1, table1 );
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
		++seqno;
		if(!selct(xx,seqno))  /* not selected */
			continue;

		p2 = xx[afld2].val;
		p3 = xx[afld3].val;

		if(!dtally) {
			errflg = started = signcnt = deccnt = 0;
			sign = 1;
			for(q=p1=xx[afld1].val;!errflg && *q != '\0'; q++) {
				/*  test for valid numeric strings */
				switch(*q) {
				case '+':
					if(started || signcnt > 0)
						errflg = 1;
					else {
						p1++;
						signcnt++;
					}
					break;
				case ' ':
					if(started || signcnt > 0)
						errflg = 1;
					else
						p1++;
					break;
				case '-':
					if(started || signcnt > 0)
						errflg = 1;
					else {
						sign = -1;
						signcnt++;
						p1++;
					}
					break;
				case '.':
					if(deccnt)
						errflg = 1;
					else {
						deccnt = 1;
						started = 1;
						floating = 1;
					}
					break;
				default:
					if(!isdigit(*q))
						errflg = 1;
					else
						started = 1;
					break;
				}
			}
			if(errflg) {
				error(E_NONNUMERIC,prog,seqno,argv[byptr-1],
					xx[afld1].val);
				continue;
			}
			num = atof(p1) * sign;   /*ascii to floating point*/
		}

		rexist = cexist = 0;
		for(i=0; i<rcnt; i++) {  /* is p2 already in rname[]*/
			if (strcmp(p2,rname[i]) == 0) {
				rexist = 1;
				break;
			}
		}
		for(j=0; j<ccnt; j++) { /*is p3 already in cname[]*/
			if (strcmp(p3,cname[j]) == 0) {
				cexist = 1;
				break;
			}
		}
		if (rexist == 1 && cexist == 1) { /*both are in*/
			if (!dtally) {
				if (ddtbl[i][j] > NOVALUE)
					ddtbl[i][j] += num;
				else
					ddtbl[i][j] = num;
			}
			else
					ddtbl[i][j] += 1;
		}
		else if (rexist==0 && cexist==0) { /*neither are in*/
			if (ccnt >= MAXCOL) {
				error(E_GENERAL,
				"%s: Exceeding maximum columns allowed for %s\n",
				prog,argv[fromptr-1]);
				clean();
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			if(rcnt >= row_size && re_size() == 0) {
				clean();
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			if (!dtally)
				ddtbl[rcnt][ccnt] = num;  /*put number in*/
			else
				ddtbl[rcnt][ccnt] += 1;
			len = strlen(p2) + 1;
			if ((rname[rcnt] = malloc(len)) == NULL) {
				error(E_SPACE,prog);
				clean();
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			strcpy(rname[rcnt],p2); /*put p2 in*/
			rcnt++;
			len = strlen(p3) +1;
			if ((cname[ccnt] = malloc(len))  == NULL) {
				error(E_SPACE,prog);
				clean();
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			strcpy(cname[ccnt],p3);  /*put p3 in*/
			ccnt++;
		}
		else if (rexist==1 && cexist==0) { /*p2 is in and p3 is not*/
			if (ccnt >= MAXCOL) {
				error(E_GENERAL,
			"%s: Exceeding maximum columns allowed for %s\n",
				prog, argv[fromptr-1]);
				clean();
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			if (!dtally)
				ddtbl[i][ccnt] = num;  /*put number in*/
			else
				ddtbl[i][ccnt] += 1;
			len = strlen(p3) + 1;
			if ((cname[ccnt] = malloc(len)) == NULL) {
				error(E_SPACE,prog);
				clean();
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			strcpy(cname[ccnt],p3);  /*put p3 in*/
			ccnt++;
		}
		else if (rexist==0 && cexist==1) { /*p3 is in and p2 is not*/
			if (rcnt >= row_size && re_size() == 0) {
				error(E_SPACE,prog);
				clean();
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			if (!dtally)
				ddtbl[rcnt][j] = num;  /*put number in*/
			else
				ddtbl[rcnt][j] += 1;
			len = strlen(p2) + 1;
			if ((rname[rcnt] = malloc(len)) == NULL) {
				error(E_SPACE,prog);
				clean();
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			strcpy(rname[rcnt],p2);  /*put p2 in*/
			rcnt++;
		}
	}        /*   bottom of infinite loop   */

	if (utfp1 != stdin) {
		if (packed) {
			/* EOF == TRUE */
			if (packedclose(utfp1,TRUE) != 0) {
				error(E_PACKREAD,prog,table1,packedsuffix);
				utfp1 = NULL;
				clean();
				return(exitcode);
			}
		}
		else
			fclose(utfp1);
	}
	utfp1 = NULL;

	if(sortflg) {
		sortrow();
		sortcol();
	}

	if(prntflg) {
		for (init= -5,j=0; j != ccnt;) {
			/* five columns per section */
			init+=5;
			j+=5;
			if (j>ccnt)
				j=ccnt;
	
			/* print out values in cname[] -
			   12 characters/value per line*/
			fprintf(utfp2,"            ");
			for(i=init; i<j; i++)
				fprintf(utfp2," %12.12s", cname[i]);
			fprintf(utfp2,"\n");

			/* second line of cname[] values */
			fprintf(utfp2,"            ");
			for(i=init; i<j; i++) {
				if (strlen(cname[i]) <= 12)
					fprintf(utfp2,"             ");
				else /* print the rest of cname[] */
					fprintf(utfp2," %-12.12s",
						&cname[i][12]);
			}
			fprintf(utfp2,"\n");
	
			for(i=0; i<rcnt; i++) {
				/* print out values in rname[] -
				   12 characters/value per line*/
				fprintf(utfp2,"%-12.12s", rname[i]);
				if(strlen(rname[i]) > 12)
					fprintf(utfp2,"\n%-12.12s",
						&rname[i][12]);
	
				/* put out counts or sums */
				for(k=init; k<j; k++) {
					/* if no value for ddtbl[][], put "-" */
					if (ddtbl[i][k] <= NOVALUE)
						fprintf(utfp2,"            -");
					else if (dtally) {
						fprintf(utfp2,"  %11.0f",
						ddtbl[i][k]);
						total[k] += ddtbl[i][k];
					}
					else { /* !dtally */
						if (floating)
							/* p1 is double field*/
							fprintf(utfp2,"  %11.2f",
								ddtbl[i][k]);
						else
							/* p1 is integer field*/
							fprintf(utfp2,"  %11.0f",
								ddtbl[i][k]);
					}
				}
				fprintf(utfp2,"\n");
			}
	
			if (dtally) {
				/* put out totals */
				fprintf(utfp2,"            ");
				for(i=init;i<j;i++)
					fprintf(utfp2,"        -----");
				if ((j-init)<=5)
					fprintf(utfp2,"\n");
				fprintf(utfp2,"TOTAL              ");
				for(i=init;i<j;i++)
					fprintf(utfp2,"%6.0f       ",total[i]);
				fprintf(utfp2,"\n");
			}
			fprintf(utfp2,"\n\n\n");
		}
	}
	else		/* print to output file */
	{
		int	rc;

		if ((Dtable2[0]) || (coption))
		{
			FILE *dfp;

			if (*Dtable2) {
				if ((udfp2=fopen(Dtable2,"w"))==NULL) {	
					error(E_DESFOPEN,prog,Dtable2);
					clean();
					return(exitcode);
				}
				dfp = udfp2;
			} else {
				fprintf(stdout, "%%description\n");
				dfp = stdout;
			}
			/* create output description file */
			fprintf(dfp,"value\tt:\t12l\t\n");
			for(i=0; i<ccnt-1; i++) {
				for(j=0;cname[i][j] != '\0'; j++)
					if(!isalnum(cname[i][j]))
						cname[i][j] = '_';
				if(!isalpha(cname[i][0]))
					fprintf(dfp,"v%-.10s\tt:\t12r\t\n",
						cname[i]);
				else
					fprintf(dfp,"%-.11s\tt:\t12r\t\n",
						cname[i]);
			}

			for(j=0;cname[i][j] != '\0'; j++)
				if(!isalnum(cname[i][j]))
					cname[i][j] = '_';
			if(!isalpha(cname[i][0]))
				fprintf(dfp,"v%-.10s\tt\\n\t12r\t\n",
					cname[ccnt-1]);
			else
				fprintf(dfp,"%-.11s\tt\\n\t12r\t\n",
					cname[ccnt-1]);

			if (dfp == stdout) {
				fprintf(stdout, "%%enddescription\n");
				if (fflush(stdout) == EOF) {
					error(E_DATFWRITE,prog,"-");
					clean();
					return(exitcode);
				}
			} else {
				if (fclose(udfp2) == EOF) {
					error(E_DATFWRITE,prog,Dtable2);
					clean();
					return(exitcode);
				}
				udfp2 = NULL;
			}
		}

		for(rc=i=0; i<rcnt; i++) {

			/* print out values in rname[] */
			if (fprintf(utfp2,"%s", rname[i]) < 0)
				++rc;

			/* put out counts or sums */
			for(k=0; k<ccnt; k++) {
				if (fprintf(utfp2,":") < 0)
					++rc;

				/* if no value for ddtbl[][], put "-" */
				if (ddtbl[i][k] <= NOVALUE) {
					if (fprintf(utfp2,"-") < 0)
						++rc;
				} else if (dtally) {
					if (fprintf(utfp2,"%.0f", ddtbl[i][k]) < 0)
						++rc;
				}
				else { /* !dtally */
					if (floating) {
						/* p1 is double field*/
						if (fprintf(utfp2,"%.2f", ddtbl[i][k]) < 0)
							++rc;
					} else {
						/* p1 is integer field*/
						if (fprintf(utfp2,"%.0f", ddtbl[i][k]) < 0)
							++rc;
					}
				}
			}
			if (fprintf(utfp2,"\n") < 0)
				++rc;

			if (rc) {
				error(E_DATFWRITE,prog,tblname);
				clean();
				return(exitcode);
			}
		}
	}
	if (fflush(utfp2) == EOF) {
		error(E_DATFWRITE,prog,tblname);
		clean();
		return(exitcode);
	}
	if (utfp2 != stdout) {
		if (fclose(utfp2) == EOF) {
			error(E_DATFWRITE, prog, tblname);
			clean();
			return(exitcode);
		}
	}
	utfp2 = NULL;

	Dtable2[0] = '\0';
	table2 = NULL;
	exitcode = 0;
	clean();
	return(exitcode);
}

static int
sortrow()
{
	int	gap, i, j, k;
	char	*tname;
	double	temp;

	for(gap = rcnt/2;gap >0;gap /=2) {
		for(i=gap;i<rcnt;i++) {
			for(j= i-gap;j >=0;j -=gap) {
				if (strcmp(rname[j],rname[j+gap]) <=0)
					break;

				tname=rname[j];
				rname[j]=rname[j+gap];
				rname[j+gap]=tname;

				for(k=0;k<ccnt;k++) {
					temp=ddtbl[j][k];
					ddtbl[j][k]=ddtbl[j+gap][k];
					ddtbl[j+gap][k]=temp;
				}
			}
		}
	}
}

static int
sortcol()
{
	int	gap, i, j, k;
	char	*tname;
	double	temp;

	for(gap = ccnt/2;gap >0;gap /=2) {
		for(i=gap;i<ccnt;i++) {
			for(j=i-gap;j >=0;j -=gap) {
				if (strcmp(cname[j],cname[j+gap]) <=0)
					break;

				tname=cname[j];
				cname[j]=cname[j+gap];
				cname[j+gap]=tname;

				for(k=0;k<rcnt;k++) {
					temp=ddtbl[k][j];
					ddtbl[k][j]=ddtbl[k][j+gap];
					ddtbl[k][j+gap]=temp;
				}
			}
		}
	}
}

/*  this routine increases the number of rows in the ddtbl[][] array
    and the rname[] array.  it returns zero on failure.
*/
static int
re_size()
{
	register int i,i2,j;

	i = row_size;
	i2 = row_size + I_INCR;
	if((ddtbl = (double **)realloc((char *)ddtbl, i2 * sizeof(double *)))
	    == (double **)NULL)
		return(0);
	for( ; i < i2; i++) {
		if((ddtbl[i] = (double *)malloc(MAXCOL * sizeof(double)))
		    == (double *)NULL)
			return(0);
		if(dtally) {
			for(j=0;j<MAXCOL;j++)
				ddtbl[i][j] = 0;
		}
		else {
			for(j=0;j<MAXCOL;j++)
				ddtbl[i][j] = NOVALUE;
		}
	}
	if((rname = (char **)realloc((char *)rname, i2 * sizeof(char *))) ==
		(char **)NULL) {
		return(0);
	}
	row_size = i2;
	return(1);
}

static int
clean()
{
	int i;

	if(ddtbl != NULL) {
		for(i=0; i < row_size; i++)
			free((char *)ddtbl[i]);
		free((char *)ddtbl);
	}
	if(rname != NULL) {
		for(i=0; i < rcnt; i++) {
			if(rname[i] != 0)
				free((char *)rname[i]);
		}
		free((char *)rname);
	}
	for(i=0; i < ccnt; i++) {
		if(cname[i] != 0)
			free((char *)cname[i]);
	}
}
