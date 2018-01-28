/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* This function does a non-numeric distribution 			*/
/* This includes the tally command					*/

#include "db.h"
#include <ctype.h>
#define MAXROW 1000


static	int	cnt;
static	struct	s1 {
	char *dname;    /* argv[fromptr-1] value	*/
	double dnum;	/* sum of argv[byptr-1] values	*/
	int dcnt;	/* count occurrences of values	*/
};
static	struct s1 temp, dtbl[MAXROW];
extern	char	*malloc(),*strcpy(),*strrchr();
extern	FILE	*getdescrwtbl();
extern	FILE	*packedopen();
extern	int	packedclose();

extern	char	*packedsuffix;
extern	char	*table2, Dtable2[],*stdbuf;
extern	char	DtmpItable[];
extern FILE	*utfp1,*utfp2,*udfp2;
extern	int	end_of_tbl;
static	int	exitcode;

extern void sort();

dist(argc, argv)
int	argc; 
char	*argv[];
{
	struct	fmt xx[MAXATT];
	double	atof(),num;
	int	nattr1,afld1,afld2,i,exist,seqno,endf1,packed;
	int	started,sign,signcnt,deccnt,errflg,floating,tally,total;
	unsigned len;
	char	*p1, *p2, *q, **parms, *prog;
	char	dtable2[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	*tblname;
	char	Dtable1[MAXPATH+4], *table1, *Ioption, *Ooption, coption;
	int	fromptr, intoptr, whereptr, byptr;
	int	sortflg = 1, prntflg = 0, onto=0;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}

	floating = tally = cnt = total = coption = packed = 0;

	Ooption = Ioption = stdbuf = table2 = NULL;
	udfp2 = NULL;
	dtable2[0] = '\0';
	Dtable2[0] = '\0';
	DtmpItable[0] = '\0';
	exitcode = 1;

	if(argc == 1) {
		usage(prog);
		return(exitcode);
	}

	while ( argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
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
	exitcode = 0;
	if(fromptr == 0 ||
	!( (intoptr == 0 && ( (whereptr == 0 && fromptr == argc - 2) ||
			       (whereptr > 0 && fromptr == whereptr - 2))) ||
	    (intoptr > 0 && fromptr == intoptr - 2  &&
			(whereptr == 0 || fromptr == whereptr - 4)) )  )
		exitcode = 1;

	/* check command name */
	if(strcmp(prog,"tally") == 0) {
		tally = 1;
		if(exitcode == 1 || fromptr != 2) {
			usage(prog);
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
		if(exitcode == 1 || fromptr != 4 || byptr != 2) {
			usage(prog);
			exitcode = 1;
			return(exitcode);
		}
		if(strcmp(argv[byptr-1],"count") == 0)
			tally = 1;
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

	if((afld2=setnum(xx,argv[fromptr-1],nattr1))==ERR) {
		error(E_ILLATTR,prog,argv[fromptr-1],Dtable1);
		if (packed) {
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}

	if(!tally) {
		if((afld1=setnum(xx,argv[byptr-1],nattr1))==ERR) {
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
			setbuf(stdout,stdbuf);

		tblname = "-";		/* current output table name */
	}
	else {
		prntflg = 0;			/* dont print */
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
			} else {
				/* make sure dtable2 is null ("") */
				dtable2[0] = '\0';
			}
			utfp2 = stdout;
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
		if ((dtable2[0]) || ((coption) &&
		     (utfp2 == stdout) && (Ooption == NULL)))
		{
			FILE *dfp;

			strcpy(Dtable2,dtable2);

			if (*Dtable2) {
				if ((udfp2=fopen(Dtable2,"w"))==NULL) {	
					error(E_DESFOPEN,prog,Dtable2);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
				dfp = udfp2;
			} else {
				fprintf(stdout, "%%description\n");
				dfp = stdout;
			}

			fprintf(dfp,"name\tt:\t16l\tFIELD VALUE\n");
			if(!tally)
				fprintf(dfp,
					"total\tt:\t12r\tTOTAL OF VALUE\n");
			fprintf(dfp,"count\tt\\n\t8r\tCOUNT OF VALUES\n");

			if (dfp == stdout) {
				fprintf(stdout, "%%enddescription\n");
				if (fflush(stdout) == EOF) {
					error(E_DATFWRITE,prog,"-");
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
			} else {
				if (fclose(udfp2) == EOF) {
					error(E_DATFWRITE,prog,Dtable2);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
				udfp2 = NULL;
			}
		}

	}

	seqno = end_of_tbl = 0;
	for(;;) { /* infinite loop */
		newrec();
		for(i=0;i < nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
		if ((i == 0) && (end_of_tbl))
			break;
		else if (i < nattr1) {
			error(E_GENERAL,
				"%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
				prog, i, seqno+1, table1);
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
		++seqno;  /* record number of table */
		if(!selct(xx,seqno))  /* not selected */
			continue;
		p2=xx[afld2].val;

		if (!tally) {	/* get the numeric value */
			errflg = started = signcnt = deccnt = 0;
			sign = 1;
			q=p1= xx[afld1].val;
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
				continue;	/* process next record */
			}
			num = atof(p1) * sign;   /*ascii to floating point*/
		}

		exist=0;
		for(i=0;i < cnt;i++) {   /* is p2 in dtbl? */
			if(strcmp(p2,dtbl[i].dname)==0) {
				exist=1;    /* yes - add num to dnum */
				if (tally==0)
					dtbl[i].dnum += num;
				dtbl[i].dcnt++;
				break;
			}
		}
		if (exist == 0) {  /* if string is not in dtbl */
			if (cnt >= MAXROW) { 
				error(E_GENERAL,"Out of table space\n");
				break;
			}
			dtbl[cnt].dcnt = 1;
			dtbl[cnt].dnum = num;  /* put number in */
			len=strlen(p2)+1;  /* get length of string p2 */
			if ((dtbl[cnt].dname=malloc(len))==NULL) {
				error(E_SPACE,prog);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			strcpy(dtbl[cnt].dname,p2);
			cnt++;
		}
	}

	if (utfp1 != stdin) {
		if (packed) {
			/* EOF == TRUE */
			if (packedclose(utfp1,TRUE) != 0) {
				error(E_PACKREAD,prog,table1,packedsuffix);
				utfp1 = NULL;
				return(exitcode);
			}
		}
		else
			fclose(utfp1);
	}
	utfp1 = NULL;

	if(sortflg)	/* NOSORT not specified */
		sort();

	if(prntflg) {	/* output to stdout */
		if (tally)
			fprintf(utfp2,"\n\nNAME           COUNT\n\n");
		else
			fprintf(utfp2,
				"\n\nNAME                TOTAL  COUNT\n\n");
		for(i=0;i<cnt;i++) {
			fprintf(utfp2,"%-12.12s",dtbl[i].dname);
			if(strlen(dtbl[i].dname) > 12)
				fprintf(utfp2,"\n%-12.12s",&dtbl[i].dname[12]);
			if (tally) {
				total += dtbl[i].dcnt;
				fprintf(utfp2,"  %6d\n",dtbl[i].dcnt);
			}
			else { /* tally == 0 */
				if (floating)
					fprintf(utfp2,"  %11.2f %6d\n",
						dtbl[i].dnum, dtbl[i].dcnt);
				else
					fprintf(utfp2,"  %11.0f %6d\n",
						dtbl[i].dnum, dtbl[i].dcnt);
			}
		}
		if(tally) {
			fprintf(utfp2,"              ------\n");
			fprintf(utfp2,"TOTAL         %6d\n",total);
		}
	}
	else {		/* print to output file */
		int rc;

		for(rc=i=0;i<cnt;i++) {
			if (fprintf(utfp2,"%s:",dtbl[i].dname) < 0)
				++rc;
			if (tally) {
				if (fprintf(utfp2,"%d\n",dtbl[i].dcnt) < 0)
					++rc;
			} else { /* tally == 0 */
				if (floating) {
					if (fprintf(utfp2,"%.2f:%d\n",
					     dtbl[i].dnum, dtbl[i].dcnt) < 0)
						++rc;
				} else {
					if (fprintf(utfp2,"%.0f:%d\n",
					     dtbl[i].dnum, dtbl[i].dcnt) < 0)
						++rc;
				}
			}
			if (rc) {
				error(E_DATFWRITE,prog,tblname);
				return(exitcode);
			}
		}
	}
	if (fflush(utfp2) == EOF) {
		error(E_DATFWRITE,prog,tblname);
		return(exitcode);
	}
	if (utfp2 != stdout) {
		if (fclose(utfp2) == EOF) {
			error(E_DATFWRITE, prog, tblname);
			return(exitcode);
		}
	}
	utfp2 = NULL;

	table2 = NULL;
	Dtable2[0] = '\0';
	exitcode = 0;
	return(exitcode);
}

static void
sort()
{
	int gap,i,j;

	for(gap = cnt/2;gap >0;gap /=2) {
		for(i=gap;i<cnt;i++) {
			for(j= i-gap;j >=0;j -=gap) {
				if (strcmp(dtbl[j].dname,dtbl[j+gap].dname) <=0)
					break;

				temp.dname=dtbl[j].dname;
				dtbl[j].dname=dtbl[j+gap].dname;
				dtbl[j+gap].dname=temp.dname;
				temp.dnum=dtbl[j].dnum;
				dtbl[j].dnum=dtbl[j+gap].dnum;
				dtbl[j+gap].dnum=temp.dnum;

				temp.dcnt=dtbl[j].dcnt;
				dtbl[j].dcnt=dtbl[j+gap].dcnt;
				dtbl[j+gap].dcnt=temp.dcnt;
			}
		}
	}
}

static int
usage(prog)
char *prog;
{
	if (strcmp(prog,"tally") == 0) {
		error(E_GENERAL, "Usage: %s %s \\\n\t%s\n", prog,
			"[-c] [-n] [-Itable] [-Otable] aname in table1",
			"[into table2] [where clause]");
	} else {
		error(E_GENERAL, "Usage: %s %s \\\n\t%s\n", prog,
			"[-c] [-n] [-Itable] [-Otable] {aname1|count} by aname2 in table1",
			"[into table2] [where clause]");
	}
}
