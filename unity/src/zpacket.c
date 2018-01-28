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

extern	char	*strcpy(), *strrchr(), *malloc();
extern	FILE	*getdescrwtbl();

extern	char	*table2, Dtable2[],*stdbuf;
extern	char	DtmpItable[];
extern FILE	*utfp1, *utfp2;
extern	char	Uunames[MAXATT][MAXUNAME+1];	/* in mktbl2.c */
extern	int	end_of_tbl;
static	int	exitcode;

packet(argc, argv)
char	*argv[];
int	argc;
{
	struct	fmt xx[MAXATT];
	int	nattr1, endf1, packeted, i, j, dlen, qoption, onto=0, uoption;
	char	Dtable1[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	dtable2[MAXPATH+4], *tblname;
	char	*p1, newline,*prog, *Ioption, *Ooption;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}

	uoption = qoption = packeted = endf1 = 0;  

	Ooption = Ioption = stdbuf = table2 = NULL;
	Dtable2[0] = '\0';
	DtmpItable[0] = '\0';
	exitcode = 1;

	newline=NLCHAR;
	while ( argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		switch(argv[1][1]) {
			default:
				error(E_BADFLAG,prog,argv[1]);
				return(exitcode);
			case 'n':
				newline=argv[1][2];
				break;
			case 'I':
				Ioption = &argv[1][2];
				break;
			case 'O':
				Ooption = &argv[1][2];
				break;
			case 'q':
			case 'u':
				for (i=1; argv[1][i]; i++)
					switch (argv[1][i]) {
					case 'q':
						qoption = 1;
						break;
					case 'u':
						uoption = 1;
						break;
					default:
						error(E_BADFLAG,prog,&argv[1][i]);
						return(exitcode);
					}
				break;
		}
		argc--;
		argv++;
	}
	if(argc == 4 && strcmp(argv[2],"onto") == 0)
		onto = 1;
	if(argc != 2 && !(argc ==4 && (onto || strcmp(argv[2],"into")==0)) ) {
		error(E_GENERAL,
"Usage: %s [-q] [-Itable] [-Otable] [-n[<newline>]] [-u] table1 [into table2]\n",
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
		if ((utfp1 = fopen(argv[1],"r")) == NULL ) { 
			error(E_DATFOPEN,prog,argv[1]);
			return(exitcode);
		}
	}

	if((nattr1 = mkntbl2(prog,argv[1],Dtable1,xx,Ioption))==ERR)
		return(exitcode);

	if (xx[nattr1-1].flag == WN)
		endf1 = 1;

	if(argc == 4) {
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
				getfile(dtable2, argv[3], 0);
			}
		}
		else {
			if ((Ooption) && (*Ooption)) {
				getfile(Dtable2,Ooption,0);
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
	else {
		utfp2 = stdout;
		if((stdbuf=malloc(BUFSIZ)) != NULL)
			setbuf(stdout,stdbuf);
		tblname = "-";	/* current output table name */
	}

	/* no longer need any temporary input descriptor table */
	if (DtmpItable[0]) {
		/* Note this will also remove Dtable1 */
		unlink(DtmpItable);
		DtmpItable[0] = '\0';
	}

	end_of_tbl = 0;

	for(;;) {
		newrec(); 
		for(i=0;i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
		if ((i == 0) && (end_of_tbl))
			break;
		else if (i < nattr1)
		{
			error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
				prog, i, packeted + 1, argv[1] );

			return( exitcode );
		}

		if (endf1)
			if ((feof(utfp1)) || ((i = getc(utfp1)) == EOF)) {
				error( E_GENERAL,
					"%s: Error: missing newline on rec# %d in %s\n",
					prog, packeted + 1, argv[1] );
				return(exitcode);
			} else if (i != '\n') {
				error( E_GENERAL,
					"%s: Error: data overrun on rec# %d in %s\n",
					prog, packeted + 1, argv[1] );
				return(exitcode);
			}

		packeted++;

		if (fprintf(utfp2,"\n#####\t%d\n",packeted) < 0) {
			error(E_DATFWRITE,prog,tblname);
			return(exitcode);
		}
		for(i=0;i<nattr1;i++) {
			if(uoption) {
				if (fprintf(utfp2,"%s\t",Uunames[i]) < 0) {
					error(E_DATFWRITE,prog,tblname);
					return(exitcode);
				}
			} else {
				if (fprintf(utfp2,"%s\t",xx[i].aname) < 0) {
					error(E_DATFWRITE,prog,tblname);
					return(exitcode);
				}
			}
			p1=xx[i].val;
			dlen=strlen(p1);
			if (xx[i].flag==T && newline != '\0') {
				for (j=0;j<dlen;j++) {
					if (*p1==newline) {
						if ((putc('\n',utfp2) == EOF) ||
						    (putc('\t',utfp2) == EOF)) {
							error(E_DATFWRITE,prog,tblname);
							return(exitcode);
						}
					}
					else {
						if (putc(*p1,utfp2) == EOF) {
							error(E_DATFWRITE,prog,tblname);
							return(exitcode);
						}
					}
					p1++;
				}
			}
			else {
				for (j=0;j<dlen;j++)
					if (putc(*p1++,utfp2) == EOF) {
						error(E_DATFWRITE,prog,tblname);
						return(exitcode);
					}
			}
			if (putc('\n',utfp2) == EOF) {
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
			error(E_DATFWRITE,prog,tblname);
			return(exitcode);
		}
	}
	utfp2 = NULL;

	if(utfp1 != stdin)
		fclose(utfp1);
	utfp1 = NULL;

	if(!qoption)
		error(E_GENERAL,"%s: %d records packeted from %s.\n", prog,
			packeted, argv[1]);
	exitcode = 0;
	Dtable2[0] = '\0';
	table2 = NULL;

	return(exitcode);
}
