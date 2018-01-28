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

extern	char	*table2, Dtable2[], *stdbuf, tmptable[];
extern	char	DtmpItable[];
extern FILE	*utfp1,*utfp2, *utfp0;
extern FILE	*popen();
extern	char	*strcpy(), *strcat(), *strrchr(), *malloc();
extern	FILE	*getdescrwtbl();

static	long location;
static	int	exitcode;

asort(argc, argv)
char	*argv[];
int	argc;
{
	int	c, coption, doption, toption, uoption, posoption;
	char	*prog, SORT[256], cmd[512];
	int	nattr1, endf1, sorted, i, qoption, onto = 0;
	char	Dtable1[MAXPATH+4], *Ooption, *Ioption, *tblname;
	char	dtable2[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	struct	fmt xx[MAXATT];

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}

	coption = doption = toption = uoption = posoption = 0;
	Ioption = Ooption = stdbuf = table2 = NULL;
	tmptable[0] = Dtable2[0] = DtmpItable[0] = '\0';
	tblname = "-";	/* current output table name */
	exitcode = 1;

	qoption = sorted = endf1 = 0;
	SORT[0] = '\0';

	while( argc > 1 && (argv[1][0] == '-'  || argv[1][0] == '+') && argv[1][1] != 0) {
		switch(argv[1][1]) {
		case 'O':
			Ooption = &argv[1][2];
			break;
		case 'I':
			Ioption = &argv[1][2];
			break;
		case 'c':
			coption = 1;
			break;
		case 'q':
			qoption = 1;
			break;
		case 'u':	/* unique - sort unique up to the ^A */
			uoption = 1;
			strcat(SORT," -u '-t\01' +0 -1");
			break;
		case 't':
			/* quote argument */
			if(argv[1][2] != '\0') {
				if(argv[1][2] == '\''){	/* take care of quote */
					strcat(SORT," '-t\\''");
				}
				else {
					strcat(SORT," '");
					strcat(SORT,argv[1]);
					strcat(SORT,"'");
				}
				toption = 1;
			}
			break;
		case 'D':
			doption=1;
			strcat(SORT,
			" -t/ -n +2.0 -2.3 +0.0 -1.0 +1.0 -2.0 +2.4 -3.0");
			break;
		case '0':	/* +pos or -pos for sort option */
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			posoption=1;
		default:	/* other sort options */
			/* quote argument */
			strcat(SORT," '");
			strcat(SORT,argv[1]);
			strcat(SORT,"'");
			break;
		}
		argc--;
		argv++;
	}
	if(toption == 1 && uoption == 1) {
		error(E_GENERAL, "%s:  Cannot have -t option with -u option\n",
			prog);
		return(1);
	}
	if(doption == 1 && uoption == 1) {
		error(E_GENERAL,"%s:  Cannot have date option with -u option\n",
			prog);
		return(1);
	}
	if(doption == 1 && toption == 1) {
		error(E_GENERAL,"%s:  Cannot have date option with -t option\n",
			prog);
		return(1);
	}
	if(posoption == 1 && uoption == 1) {
		error(E_GENERAL,
			"%s:  Cannot have [+-]position option with -u option\n",
			prog);
		return(1);
	}
	if(posoption == 1 && doption == 1) {
		error(E_GENERAL,
		"%s:  Cannot have [+-]position option with date option\n",
			prog);
		return(1);
	}

	if(argc == 6 && strcmp(argv[4],"onto") == 0)
		onto = 1;
	if(argc != 4 && !(argc == 6 && (onto || strcmp(argv[4],"into") == 0))
		|| strcmp(argv[2],"in") != 0) {
		error(E_GENERAL,
"Usage: %s [-c] [-q] [sort flags] [-Itable] [-Otable] aname in table1 \\\n\t[into table2]\n",
			prog);
		return(exitcode);
	}

	if ((Ooption) && (argc == 4)) {
		error(E_GENERAL,
			"%s: Missing \"into table\" clause for the \"-O%s\" option.\n",
			prog, Ooption);
		return(exitcode);
	}

	if(strcmp(argv[3],"-") != 0) {
		if ((utfp1 = fopen(argv[3],"r")) == NULL ) {
			error(E_DATFOPEN,prog,argv[3]);
			return(exitcode);
		}
		if (Ioption)
		{
			char *Iprefix = "";

			if (strcmp(Ioption,argv[3]) == 0)
			{
				/* Search data directory first since the alternate
				 * table is the same as the data table.
				 */
				if (Ioption[0] == '/') {
					if (Ioption[1] != '.' || Ioption[2] != '/')
						Iprefix = "/.";
				} else if (Ioption[0] == '.' && Ioption[1] == '/') {
					if (Ioption[2] != '.' || Ioption[3] != '/')
						Iprefix = "./";
				} else {
					Iprefix = "././";
				}
			}
			sprintf(cmd,"index2 -I%s%s %s %s | sort %s",
				Iprefix, Ioption, argv[1], argv[3], SORT);
		}
		else
			sprintf(cmd,"index2 %s %s | sort %s", argv[1],
				argv[3], SORT);
	}
	else {	/* must read it into a temporary file first */

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
		sprintf(tmptable,"/tmp/asort%d", getpid());
		if ((utfp1 = fopen(tmptable,"w+")) == NULL ) {
			error(E_TEMFOPEN,prog,tmptable);
			return(exitcode);
		}
		sprintf(cmd,"index2 -I%s %s %s | sort %s", Ioption, argv[1],
			tmptable, SORT);
	}

	if((nattr1 = mkntbl(prog,argv[3],Dtable1,xx,Ioption))==ERR)
			return(exitcode);
	if (xx[nattr1-1].flag == WN)
		endf1 = 1;

	if(argc < 6) {
		utfp2 = stdout;
		if((stdbuf=malloc(BUFSIZ)) != NULL)
			setbuf(stdout,stdbuf);
	}
	else {
		if(strcmp(argv[5],"-") != 0) {
			if(chkaccess(argv[5],00) == 0) {
				if(!onto) {
					error(E_EXISTS,prog,argv[5]);
					return(exitcode);
				}
			}
			else
				table2 = argv[5];
			if(onto)
				utfp2 = fopen(argv[5],"a");
			else
				utfp2 = fopen(argv[5],"w");
			if(utfp2  == NULL) {
				error(E_DATFOPEN,prog,argv[5]);
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
			tblname = argv[5];	/* current output table name */
		} else {
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
	}

	if ((coption) && (utfp2 == stdout) && (Ooption == NULL)) {
		if (putdescrwtbl(stdout, Dtable1) == NULL) {
			error(E_DATFOPEN,prog,"-");
			return(exitcode);
		}
	}

	if(tmptable[0] != '\0') {	/* read standard input into tmptable */
		while((c=getchar()) != EOF) {
			if (putc(c, utfp1) == EOF) {
				error(E_DATFWRITE, prog, tmptable);
				return(exitcode);
			}
		}
		if (fflush(utfp1) == EOF) {
			error(E_DATFWRITE, prog, tmptable);
			return(exitcode);
		}
	}

	if((utfp0 = popen(cmd,"r")) == NULL) {
		error(E_GENERAL,"%s: sort failed\n",prog);
		return(exitcode);
	}

	while(getone() != EOF) {	/* get next location */
		fseek(utfp1,location,0);	/* seek to record location */
		newrec(); 			/* get record */
		for(i=0;i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
		if (i != nattr1) {
			error(E_GENERAL,
				"%s: Error: record parsing failed on attribute %d of rec# %d sorted from %s\n",
				prog, i, sorted + 1, argv[3] );
			pclose(utfp0);
			utfp0 = NULL;
			return(exitcode);
		}
		for(i=0; i<nattr1;i++)		/* write record */
			if (putnrec(utfp2,&xx[i]) < 0) {
				error(E_DATFWRITE, prog, tblname);
				pclose(utfp0);
				utfp0 = NULL;
				return(exitcode);
			}
		if(endf1 == 1)
			if (putc('\n',utfp2) == EOF) {
				error(E_DATFWRITE, prog, tblname);
				pclose(utfp0);
				utfp0 = NULL;
				return(exitcode);
			}
		sorted++;
	}
	if (fflush(utfp2) == EOF) {
		error(E_DATFWRITE, prog, tblname);
		return(exitcode);
	}
	if (utfp2 != stdout) {
		if (fclose(utfp2) == EOF) {
			error(E_DATFWRITE, prog, tblname);
			return(exitcode);
		}
		utfp2 = NULL;
	}
	if (utfp1 != stdin)
		fclose(utfp1);
	utfp1 = NULL;
	pclose(utfp0);
	utfp0 = NULL;
	if(!qoption)
		error(E_GENERAL,"%s: %d records sorted from %s.\n", prog,
			sorted, argv[3]);
	table2 = NULL;
	Dtable2[0] = '\0';
	exitcode = 0;
	return(exitcode);
}

static int
getone()
{
	int c;

	/* throw away key value */
	for(;(c=getc(utfp0))!='\01'&& c!=EOF;);
	if(c==EOF)
		return(EOF);

	/* get seek location */
	fscanf(utfp0,"%ld",&location);

	/* get rid of newline */
	while((c=getc(utfp0))!='\n' && c!=EOF);
	return(0);
}
