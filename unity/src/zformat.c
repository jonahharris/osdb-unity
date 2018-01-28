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

extern	char *strcpy(), *strrchr();

extern	char	*table2, Dtable2[],*stdbuf;
extern	char	DtmpItable[];
extern	FILE	*utfp1, *utfp2;
extern	FILE	*getdescrwtbl(), *putdescrwtbl();
extern	FILE	*packedopen();
extern	int	packedclose();

extern	char	*packedsuffix;
extern	int	end_of_tbl;
static	int	exitcode;

format(argc, argv)
char	*argv[];
int	argc;
{
	struct	fmt xx[MAXATT], yy[MAXATT];
	int	nattr1, nattr2, xf[MAXATT], endf1, endf2, formated, i, j, len;
	char	Dtable1[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	data[MAXREC], *prog, *Ioption, *Ooption;
	char	dtable2[MAXPATH+4];
	int	coption, packed, qoption, onto=0;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}

	qoption = formated = endf1 = endf2  = coption = packed = 0;  
	Ioption = Ooption = stdbuf = table2 = NULL;
	Dtable2[0] = '\0';
	DtmpItable[0] = '\0';

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
				for (i=1; argv[1][i]; i++) {
					switch (argv[1][i]) {
					case 'c':
						++coption;
						break;
					case 'q':
						++qoption;
						break;
					default:
						error(E_BADFLAG,prog,argv[1][i]);
						return(exitcode);
					}
				}
				break;
		}
		argc--;
		argv++;
	}

	if(argc > 2 && strcmp(argv[2],"onto") == 0)
		onto = 1;
	if(argc != 4 || !(onto || strcmp(argv[2],"into") == 0) ) {
		error(E_GENERAL,
	"Usage: %s [-c] [-q] [-Itable] [-Otable] table1 into table2\n", prog);
		return(exitcode);
	}

	if(strcmp(argv[1],"-") == 0) {
		if(!Ioption) {
			/*
			 * Check for/get description from the input file.
			 */
			if (getdescrwtbl(stdin, &Ioption) == NULL) {
				error(E_GENERAL,"%s: No description file specified\n",
					prog);
				return(exitcode);
			}
		}
		utfp1 = stdin;
	}
	else {
		if((utfp1 = fopen(argv[1],"r")) == NULL ) { 
			if ((strcmp(argv[1],argv[3]) == 0) ||
			    ((utfp1 = packedopen(argv[1])) == NULL)) {
				error(E_DATFOPEN,prog,argv[1]);
				return(exitcode);
			}
			++packed;	/* reading packed table */
		}
	}

	if((nattr1 = mkntbl(prog,argv[1],Dtable1,xx,Ioption))==ERR) {
		if (packed) {
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}
	if (xx[nattr1-1].flag == WN)
		endf1 = 1;

	/* no longer need any temporary input descriptor table */
	if (DtmpItable[0]) {
		/* Note this will also remove Dtable1 */
		unlink(DtmpItable);
		DtmpItable[0] = '\0';
	}
	
	if(strcmp(argv[3],"-") == 0) {
		if(!Ooption) {
			error(E_GENERAL,
			"%s: Second description file not specified.\n",
				prog);
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}
		utfp2 = stdout;
	}
	else {
		if (chkaccess(argv[3],00) == 0) {
			if(!onto) {
				error(E_EXISTS,prog,argv[3]);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
		} else
			table2 = argv[3];
		if(onto)
			utfp2 = fopen(argv[3],"a");
		else
			utfp2 = fopen(argv[3],"w");
		if(utfp2 == NULL) { 
			error(E_DATFOPEN,prog,argv[3]);
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}
	}

	if((nattr2 = mkntbl(prog,argv[3],dtable2,yy,Ooption))==ERR) {
		if (packed) {
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}

	if (yy[nattr2-1].flag == WN)
		endf2 = 1;

	if (qoption <= 1) {
		for(i=0;i<nattr2;i++) { 
			if ((xf[i] = setnum(xx,yy[i].aname,nattr1)) < 0) {
				error(E_GENERAL,
	"%s: Warning - attribute %s not in %s and is being created in %s.\n",
	prog,yy[i].aname,argv[1],argv[3]);
			}
		}
	}

	if ((coption) && (utfp2 == stdout)) {
		if (putdescrwtbl(stdout, dtable2) == NULL) {
			error(E_DATFOPEN,prog,"-");
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}
	}

	end_of_tbl = 0;

	for(;;) { 
		int size_newrec;

		newrec(); 
		for(i=0;i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
		if (( i == 0 ) && ( end_of_tbl ))
			break;
		else if ( i < nattr1 )
		{
			error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
				prog, i, formated + 1, argv[1] );

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
					prog, formated + 1, argv[1] );
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			} else if (i != '\n') {
				error( E_GENERAL,
					"%s: Error: data overrun on rec# %d in %s\n",
					prog, formated + 1, argv[1] );
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}

		size_newrec = 0;
		for(i=0;i<nattr2;i++) {
			if(xf[i] < 0) {
				if(yy[i].flag == WN) {
					for(j=0; j<yy[i].flen; j++)
						data[j] = ' ';
					data[j] = '\0';
				}
				else {
					data[0] = '\0';
				}
				yy[i].val = data;
			}
			else if( yy[i].flag == WN &&
				(len=strlen(xx[xf[i]].val)) < yy[i].flen ) {
				/* pad with blanks */
				if(yy[i].justify == 'l') {
					strcpy(data,xx[xf[i]].val);
					for(j=len; j<yy[i].flen; j++)
						data[j] = ' ';
				}
				else {
					for(j=0; j<yy[i].flen - len; j++)
						data[j] = ' ';

					strcpy(&data[j],xx[xf[i]].val);
				}

				yy[i].val = data;
			}
			else
				yy[i].val = xx[xf[i]].val;
		/* putnrec() silently truncates fixed width field if too wide */
			if ((j = putnrec(utfp2,&yy[i])) < 0) {
				error(E_DATFWRITE, prog, argv[3]);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			size_newrec += j;
		}
		if (endf2 == 1)
		{
			if (putc('\n',utfp2) == EOF) {
				error(E_DATFWRITE, prog, argv[3]);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			size_newrec++;
		}
		if ( size_newrec > MAXREC )
		{
			error( E_GENERAL, "%s: Error: new rec# %d would be longer (%d) than maximum record length (%d)\n",
				prog, formated + 1, size_newrec, MAXREC );
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return( exitcode );
		}

		formated++;
	}

	if (utfp1 != stdin) {
		if (packed) {
			/* EOF == TRUE */
			if (packedclose(utfp1,TRUE) != 0) {
				error(E_PACKREAD,prog,argv[1],packedsuffix);
				utfp1 = NULL;
				return(exitcode);
			}
		}
		else
			fclose(utfp1);
	}
	utfp1 = NULL;

	if (fflush(utfp2) == EOF) {
		error(E_DATFWRITE, prog, argv[3]);
		return(exitcode);
	}
	if (utfp2 != stdout) {
		if (fclose(utfp2) == EOF) {
			error(E_DATFWRITE, prog, argv[3]);
			return(exitcode);
		}
	}
	utfp2 = NULL;

	if(!qoption)
		error(E_GENERAL,"%s: %d records formated from %s.\n", prog,
			formated, argv[1]);

	table2 = NULL;
	Dtable2[0] = '\0';
	exitcode = 0;
	return(exitcode);
}
