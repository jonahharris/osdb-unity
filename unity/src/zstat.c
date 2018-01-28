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

extern	char	*table2, Dtable2[],*stdbuf;
extern	FILE	*utfp1, *utfp2, *udfp2;
extern	FILE	*getdescrwtbl();
extern	FILE	*packedopen();
extern	int	packedclose();
extern	char	*packedsuffix;
extern	char	*strcpy(), *strrchr(), *malloc();
extern	char	DtmpItable[];
extern	int	end_of_tbl;
static	int	exitcode;

ustat(argc, argv)
char	*argv[];
int	argc;
{
	int	inptr, whereptr, intoptr, endf1, i, j, onto=0;
	int	n, errflg, sign, signcnt, started, deccnt, coption, packed;
	int	nattr1, nattr2, recordnr, p[MAXATT];
	char	*table1, Dtable1[MAXPATH+4], *prog,**where, *pt, *q, *Ioption;
	char	dtable2[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	*tblname;
	char	*Ooption;
	double	valu, sqs[MAXATT], sqrtp;
	extern	double atof(), sqrt();
	struct	fmt xx[MAXATT];
	struct{
		double   total;     /* sum of attr values*/
		double	avg;       /* average value */
		double	min;       /* minimum value */
		double	max;       /* maximum value */
		double	std;       /* standard deviation between values */
		int	cnt;       /* counts num. of values used to sum total */
	}  stbl[MAXATT];
	char	*fmt2;			/* output format */

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}

	endf1 = inptr = whereptr = intoptr = coption = packed = 0;
	Ooption = Ioption = stdbuf = table2 = NULL;
	dtable2[0] = '\0';
	Dtable2[0] = '\0';
	DtmpItable[0] = '\0';
	exitcode = 1;

	while (  argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		switch(argv[1][1]) {
		case 'I':
			Ioption = &argv[1][2];
			break;
		case 'O':
			Ooption = &argv[1][2];
			break;
		case 'c':
			++coption;
			break;
		default:
			error(E_BADFLAG,prog,argv[1]);
			return(exitcode);
		}
		argc--;
		argv++;
	}

	for(i=1;i<argc;i++)	/* find "in" arg if there is one */
		if(strcmp(argv[i],"in") == 0) {
			inptr = i;
			break;
		}
	for(i=1;i<argc;i++)	/* find "where" arg if there is one */
		if(strcmp(argv[i],"where") == 0) {
			whereptr = i;
			break;
		}
	for(i=1;i<argc;i++)	/* find "into" arg if there is one */
		if(strcmp(argv[i],"into") == 0 ||
		   strcmp(argv[i],"onto") == 0) {
			intoptr = i;
			if(argv[i][0] == 'o')
				onto = 1;
			break;
		}

	/* try to check for correct syntax */
	if(!( (inptr == 0 && ((intoptr == 0 && ((whereptr == 0 && argc == 2) ||
						(whereptr == 2 && argc >= 6)))
			||  (intoptr == 2 &&    ((whereptr ==0 && argc == 4) ||
						(whereptr == 4 && argc >= 8)))))
	|| (inptr > 0 && ((intoptr == 0 && ((whereptr == 0 && inptr == argc-2)
						|| (whereptr == inptr + 2)))
			|| (intoptr > 0 && ((whereptr==0 && intoptr == inptr+2)
			|| (whereptr==inptr+4 && intoptr==inptr+2)))))))
	{
		error(E_GENERAL,
"Usage: %s [-c] [-Itable] [-Otable] [ aname ... in ] table1 \\\n\t[into table2] [where clause]\n",prog);
		return(exitcode);
	}

	if(whereptr == 0)	/* make sure whereptr set ok for getcond() */
		if(intoptr == 0)
			whereptr = inptr + 2;
		else
			whereptr = inptr + 4;

	if ((Ooption) && (intoptr == 0)) {
		error(E_GENERAL,
			"%s: Missing \"into table\" clause for the \"-O%s\" option.\n",
			prog, Ooption);
		return(exitcode);
	}

	/* open data and description files */
	table1 = argv[inptr+1];
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

	/* no longer need temporary input description */
	if (DtmpItable[0]) {
		(void) unlink(DtmpItable);	/* created by getdescrwtbl() */
		DtmpItable[0] = '\0';
	}

	if(inptr > 0) {
		/* projection - get position numbers for fields */
		for(i=1,nattr2=0;i<inptr;i++,nattr2++) {
			if(i >= MAXATT) {
				error(E_GENERAL,
				"%s: Too many attributes specified.\n",
				prog);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			if((j=setnum(xx,argv[i],nattr1)) == ERR) {
				error(E_ILLATTR,prog,argv[i],table1);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			p[nattr2] = j;
		}
	}
	else {	/* statistics for all attributes - set position numbers */
		nattr2 = nattr1;
		for(i=0;i<nattr2; i++)
			p[i] = i;
	}

	/* parse where clause and build evaluation tree */
	where = &argv[whereptr];
	getcond(where,xx,nattr1,prog,Dtable1);

	if (intoptr == 0) {	/* write to standard output */
		utfp2 = stdout;
		if((stdbuf=malloc(BUFSIZ)) != NULL)
			setbuf(stdout,stdbuf);
		tblname = "-";		/* current output table name */
	} else {
		/* create description and data files */
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
				table2 = argv[intoptr + 1];
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
			if((stdbuf=malloc(BUFSIZ)) != NULL)
				setbuf(stdout,stdbuf);
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
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					error(E_DESFOPEN,prog,Dtable2);
					return(exitcode);
				}
				dfp = udfp2;
			} else {
				fprintf(stdout, "%%description\n");
				dfp = stdout;
			}

			fprintf( dfp, "%s\tt|\t12l\tATTRIBUTE NAME\n",
				"aname" );
			fprintf( dfp, "%s\tt|\t12r\tTOTAL OF VALUES\n",
				"total" );
			fprintf( dfp, "%s\tt|\t7r\tCOUNT OF VALUES\n",
				"count" );
			fprintf( dfp, "%s\tt|\t10r\tAVERAGE OF VALUES\n",
				"average" );
			fprintf( dfp, "%s\tt|\t10r\tMINIMUM VALUE\n",
				"min" );
			fprintf( dfp, "%s\tt|\t10r\tMAXIMUM VALUE\n",
				"max" );
			fprintf( dfp, "%s\tt\\n\t10r\tSTANDARD DEVIATION\n",
				"std_dev" );

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

	/*initialize statistics structure values*/
	for(i=0; i<MAXATT; i++) {
		stbl[i].total = 0.0;
		stbl[i].cnt = 0;
		stbl[i].avg = 0.0;
		stbl[i].min= 1e30;
		stbl[i].max = -1e30;
		stbl[i].std = 0.0;
		sqs[i] = 0.0;
	}
	valu = 0.0;

	recordnr = end_of_tbl = 0;

	for(;;) {
		/* get next record */
		newrec(); 
		for(i=0;i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
		if ((i == 0) && (end_of_tbl))
			break;
		else if (i < nattr1) {
			error(E_GENERAL,
				"%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
				prog, i, recordnr+1, table1);
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
					prog, recordnr + 1, table1 );
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			} else if (i != '\n') {
				error( E_GENERAL,
					"%s: Error: data overrun on rec# %d in %s\n",
					prog, recordnr + 1, table1 );
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}

		recordnr++;

		/* check match for selection criteria */
		if(!selct(xx,recordnr))
			continue;

		for(i=0;i<nattr2;i++) {
			/* test each value to ensure a valid numeric string*/
			errflg = started = signcnt = deccnt = 0;
			sign = 1;
			for(q=pt=xx[p[i]].val;!errflg && *q != '\0'; q++) {
				switch(*q) {
				case '+':
					if(started || signcnt > 0)
						errflg = 1;
					else {
						pt++;
						signcnt++;
					}
					break;
				case ' ':
					if(started || signcnt > 0)
						errflg = 1;
					else
						pt++;
					break;
				case '-':
					if(started || signcnt > 0)
						errflg = 1;
					else {
						sign = -1;
						signcnt++;
						pt++;
					}
					break;
				case '.':
					if(deccnt)
						errflg = 1;
					else {
						deccnt = 1;
						started = 1;
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
				error(E_NONNUMERIC,prog,recordnr,xx[p[i]].aname,
					xx[p[i]].val);
				continue;
			}
			if(*pt != '\0') {
				/* value not NULL - convert to floating point
				   and add to statistics
				*/
				valu = atof(pt);
				valu = sign * valu;
				stbl[i].cnt++;
				stbl[i].total += valu;
				if (valu < stbl[i].min)   /*find min. value*/
					stbl[i].min = valu;
				if (valu > stbl[i].max)   /*find max. value*/
					stbl[i].max = valu;
				sqs[i] += valu*valu;/*the sum of the squares*/
			}
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

	if (!intoptr ) {	/* to standard output */
				/* print heading */
		fprintf(utfp2,"\n\n\n");
		fprintf(utfp2,"                    total  count     average       min");
		fprintf(utfp2,"       max   std dev\n");
		fmt2 = "%12s  %11.2f %6d   %9.2f %9.2f %9.2f %9.2f\n";
	}
	else	/* into table */
		fmt2 = "%s|%.2f|%d|%.2f|%.2f|%.2f|%.2f\n";

	for(i=0; i<nattr2; i++) {
		n = stbl[i].cnt;
		if(n == 0)
			stbl[i].min = stbl[i].max = 0.0;
		else {
			stbl[i].avg = stbl[i].total/n;  /*compute average*/
			/*compute standard deviation*/
			sqrtp = (sqs[i] /n) - (stbl[i].avg*stbl[i].avg);
			if(sqrtp <= 0.0)
				stbl[i].std = 0.0;
			else
				stbl[i].std = sqrt(sqrtp);
		}
		if (fprintf(utfp2, fmt2, xx[p[i]].aname,stbl[i].total,stbl[i].cnt,
		      stbl[i].avg, stbl[i].min, stbl[i].max, stbl[i].std) < 0) {
			error(E_DATFWRITE,prog,tblname);
			return(exitcode);
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
