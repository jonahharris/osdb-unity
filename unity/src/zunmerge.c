/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include "db.h"
#define SEPARATOR ','

static int nattr1,endf1,afld1;
static struct fmt xx[MAXATT];
extern char *strcpy(), *strrchr(), *malloc();
extern FILE *getdescrwtbl(), *putdescrwtbl();
extern FILE *packedopen();
extern int  packedclose();

extern	char	*packedsuffix;
extern	char	*table2, Dtable2[],*stdbuf;
extern	char	DtmpItable[];
extern	FILE	*utfp1, *utfp2;
extern	int	end_of_tbl;
static	int	exitcode;

unmerge(argc, argv)
char	*argv[];
int	argc;
{
	char	separator;
	int	i, unmerged, coption, qoption, onto=0;
	char	Dtable1[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	*tblname, *prog, *p1, *Ioption, *Ooption;
	char	dtable2[MAXPATH+4];
	int	recordnr, packed;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	qoption = coption = endf1 = packed = 0;  
	separator=SEPARATOR;
	Ooption = Ioption = stdbuf = table2 = NULL;
	DtmpItable[0] = '\0';
	Dtable2[0] = '\0';
	dtable2[0] = '\0';
	exitcode = 1;

	while ( argc > 1 && argv[1][0] == '-') {
		switch(argv[1][1]) {
			default:
				error(E_BADFLAG,prog,argv[1]);
				return(exitcode);
			case 's':
				separator=argv[1][2];
				break;
			case 'c':
			case 'q':
				for (i = 1; argv[1][i]; i++)
					switch (argv[1][i]) {
					case 'c':
						/* create description in the output data stream */
						coption = 1;
						break;
					case 'q':
						/* don't print number of records */
						qoption = 1;
						break;
					default:
						error(E_BADFLAG,prog,&argv[1][i]);
						return(exitcode);
					}
				break;
			case 'I':
				Ioption = &argv[1][2];
				break;
			case 'O':
				Ooption = &argv[1][2];
				break;
		}
		argc--;
		argv++;
	}

	if(argc == 6 && strcmp(argv[4],"onto") == 0)
		onto = 1;
	if(argc != 4 && !(argc ==6 && (onto || strcmp(argv[4],"into")==0)) ||
		strcmp(argv[2],"in") ){
		error(E_GENERAL,
"Usage: %s [-c] [-q] [-Itable] [-Otable] [-s<separator>] aname1 in table1 \\\n\t[into table2]\n",
			prog);
		return(exitcode);
	}

	if ((Ooption) && (argc != 6 )) {
		error(E_GENERAL,
			"%s: Missing \"into table\" clause for the \"-O%s\" option.\n",
			prog, Ooption);
		return(exitcode);
	}

	if (strcmp(argv[3],"-") == 0) {
		if(!Ioption) {
			/*
			 * Check for/get description from the input file.
			 */
			if (getdescrwtbl(stdin, &Ioption) == NULL) {
				error(E_GENERAL,"%s: No description file specified.\n",
					prog);
				return(exitcode);
			}
		}
		utfp1 = stdin;
	}
	else {
		if ((utfp1 = fopen(argv[3],"r")) == NULL ) { 
			if (((argc == 6) && (strcmp(argv[3],argv[5]) == 0)) ||
			    ((utfp1 = packedopen(argv[3])) == NULL)) {
				error(E_DATFOPEN,prog,argv[3]);
				return(exitcode);
			}
			++packed;	/* reading a packed table */
		}
	}

	if((nattr1 = mkntbl(prog,argv[3],Dtable1,xx,Ioption))==ERR) {
		if (packed) {
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}
	if (xx[nattr1-1].flag == WN)
		endf1 = 1;
	if((afld1=setnum(xx,argv[1],nattr1)) == ERR) {
		error(E_ILLATTR,prog,argv[1]);
		if (packed) {
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}
	if(xx[afld1].flag != T) {
		error(E_GENERAL,
		"%s: Field to be unmerged must be a terminator field.\n",
		prog);
		if (packed) {
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}

	if (argc != 6) {
		utfp2 = stdout;
		if((stdbuf=malloc(BUFSIZ)) != NULL)
			setbuf(stdout,stdbuf);
		tblname = "-";		/* current output table name */
	} else {	/* argc == 6 */
		if(strcmp(argv[5],"-") != 0) {
			if(chkaccess(argv[5],00) == 0) {
				if(!onto) {
					error(E_EXISTS,prog,argv[5]);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
			}
			else
				table2 = argv[5];
			if(onto)
				utfp2 = fopen(argv[5],"a");
			else
				utfp2 = fopen(argv[5],"w");
			if(utfp2 == NULL) { 
				error(E_DATFOPEN,prog,argv[5]);
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
				} else if (strcmp(argv[5], Ooption) == 0) {
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
				getfile(dtable2,argv[5],0);
			}
		} else {
			if ((Ooption) && (*Ooption)) {
				getfile(dtable2,Ooption,0);
			} else {
				dtable2[0] = '\0';
			}
			utfp2 = stdout;
		}
		if ((dtable2[0]) &&
		    (strcmpfile(Dtable1,dtable2) != 0)) {
			if(chkaccess(dtable2,00) == 0) {
				if(!onto) {
					error(E_EXISTS,prog,dtable2);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
			} else {
				strcpy(Dtable2,dtable2);
				if(copy(prog,Dtable1,Dtable2)!=0) {
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
			}
		}
		tblname = argv[5];	/* current output table name */
	}

	if ((coption) && (utfp2 == stdout) && (Ooption == NULL)) {
		if (putdescrwtbl(stdout, Dtable1) == NULL) {
			error(E_DATFOPEN,prog,"-");
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}
	}

	/* no longer need any temporary input descriptor table */
	if (DtmpItable[0]) {
		/* Note this will also remove Dtable1 */
		unlink(DtmpItable);
		DtmpItable[0] = '\0';
	}

	unmerged = 0;
	recordnr = end_of_tbl = 0;
	for(;;) {
		recordnr++;
		newrec(); 
		for(i=0;i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
		if (( i == 0 ) && ( end_of_tbl ))
			break;
		else if ( i < nattr1 )
		{
			error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
				prog, i, recordnr, argv[3] );
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return( exitcode );
		}

		if (endf1) 
			if ((feof(utfp1)) || ((i = getc(utfp1)) == EOF)) {
				error( E_GENERAL,
					"%s: Error: missing newline on rec# %d in %s\n",
					prog, recordnr, argv[3] );
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			} else if (i != '\n') {
				error( E_GENERAL,
					"%s: Error: data overrun on rec# %d in %s\n",
					prog, recordnr, argv[3] );
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}

		if (putfirst() == ERR) {
			error(E_DATFWRITE, prog, tblname);
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}
		for (p1=xx[afld1].val;*p1;p1++) {
			if (*p1 == separator) {
				if (putc(xx[afld1].inf[1],utfp2) == EOF) {
					error(E_DATFWRITE, prog, tblname);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
				if (putsecond() == ERR) {
					error(E_DATFWRITE, prog, tblname);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
				unmerged++;
				if (p1[1] != '\0') {
					if (putfirst() == ERR) {
						error(E_DATFWRITE, prog, tblname);
						if (packed) {
							/* EOF == FALSE */
							(void)packedclose(utfp1,FALSE);
							utfp1 = NULL;
						}
						return(exitcode);
					}
				}
			} else {
				if (putc(*p1,utfp2) == EOF) {
					error(E_DATFWRITE, prog, tblname);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
			}
		}
		if (putc(xx[afld1].inf[1],utfp2) == ERR) {
			error(E_DATFWRITE, prog, tblname);
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}
		if (putsecond() == ERR) {
			error(E_DATFWRITE, prog, tblname);
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}
		unmerged++;
	}

	if (utfp1 != stdin) {
		if (packed) {
			/* EOF == TRUE */
			if (packedclose(utfp1,TRUE) != 0) {
				error(E_PACKREAD,prog,argv[3],packedsuffix);
				utfp1 = NULL;
				return(exitcode);
			}
		}
		else
			fclose(utfp1);
	}
	utfp1 = NULL;

	if (fflush(utfp2) == EOF) {
		error(E_DATFWRITE, prog, tblname);
		return(exitcode);
	}
	if (utfp2 != stdout) {
		if (fclose(utfp2) == EOF) {
			error(E_DATFWRITE, prog, tblname);
			return(exitcode);
		}
	}
	utfp2 = NULL;

	if(!qoption)
		error(E_GENERAL,"%s: %d records unmerged.\n",prog,unmerged);

	exitcode = 0;
	table2 = NULL;
	Dtable2[0] = '\0';
	return(exitcode);
}

static int
putfirst()
{
	int i;
	for(i=0;i<afld1;i++)
		if (putnrec(utfp2,&xx[i]) < 0) {
			return(ERR);
		}
	return(0);
}

static int
putsecond()
{
	int i;
	for(i=afld1+1;i<nattr1;i++)
		if (putnrec(utfp2,&xx[i]) < 0) {
			return(ERR);
		}
	if (endf1) {
		if (putc('\n',utfp2) == EOF) {
			return(ERR);
		}
	}
	return(0);
}
