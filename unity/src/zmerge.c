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
#include <signal.h>
#define SEPARATOR ','

static	int	afld1, nattr1, endf1;
static	struct	fmt xx[MAXATT], zz[MAXATT];
static	char	ibuf[MAXREC], *ibufptr,*prog;
extern	char	*strcat(), *strcpy(), *strrchr();
extern	char	*malloc();
extern	FILE	*getdescrwtbl(), *putdescrwtbl();
extern	FILE	*packedopen();
extern	int	packedclose();

extern	char	*packedsuffix;
extern	char	*table2, Dtable2[],*stdbuf;
extern	char	DtmpItable[];
extern	FILE	*utfp1, *utfp2;
extern	int	end_of_tbl;
static	int	exitcode;

merge(argc, argv)
char	*argv[];
int	argc;
{
	int	i, merged, firstline, ret, coption, qoption, onto=0;
	char	Dtable1[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	separator, *Ioption, *Ooption, *tblname;
	char	dtable2[MAXPATH+4];
	int	recordnr, size_newrec;
	int	packed;
	char	*attrbuf;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	qoption = coption = merged = packed = endf1 = 0;
	separator = SEPARATOR;

	Ooption = Ioption = stdbuf = table2 = NULL;
	DtmpItable[0] = '\0';
	Dtable2[0] = '\0';
	dtable2[0] = '\0';
	exitcode = 1;

	while ( argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
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
	if(argc!=4 && !(argc == 6 && (onto || strcmp(argv[4],"into")==0)) ||
		strcmp(argv[2],"in") ) {
		error(E_GENERAL,
"Usage: %s [-c] [-q] [-s<separator>] [-Itable] [-Otable] aname1 in table1 \\\n\t[into table2]\n",
		 prog);
		return(exitcode);
	}

	if ((Ooption) && (argc != 6 )) {
		error(E_GENERAL,
			"%s: Missing \"into table\" clause for the \"-O%s\" option.\n",
			prog, Ooption);
		return(exitcode);
	}

	if(strcmp(argv[3],"-") == 0) {
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
		if ((utfp1=fopen(argv[3],"r"))==NULL) {
			if (((argc == 6) && (strcmp(argv[3],argv[5]) == 0)) ||
			    ((utfp1 = packedopen(argv[3])) == NULL)) {
				error(E_DATFOPEN,prog,argv[3]);
				return(exitcode);
			}
			++packed;	/* reading a packed table */
		}
	}

	if((nattr1=mkntbl(prog,argv[3],Dtable1,xx,Ioption))== ERR) {
		if (packed) {
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}
	if (xx[nattr1-1].flag == WN)
		endf1 = 1;
	if ((afld1=setnum(xx,argv[1],nattr1))==ERR) {
		error(E_ILLATTR,prog,argv[1],Dtable1);
		if (packed) {
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}
	if(xx[afld1].flag != T) {
		error(E_GENERAL,
		"%s: Error: Field to be merged must be a terminator field.\n",
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

	for(i=0;i<nattr1;i++) {
		zz[i].flag = xx[i].flag;
		zz[i].flen = xx[i].flen;
		strcpy(zz[i].aname, xx[i].aname);
		strcpy(zz[i].inf, xx[i].inf);
		zz[i].prnt = xx[i].prnt;
		zz[i].justify = xx[i].justify;
	}

	firstline = 1;
	recordnr = end_of_tbl = 0;
	size_newrec = 0;
	attrbuf = ibuf;		/* to keep compiler happy */
	for(;;) {
		recordnr++;
		newrec();
		for(i=0;i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
		if (( i == 0 ) && (end_of_tbl))
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

		/* do comparison with previous record */
		if( ! firstline && (ret=same()) == 0) {
			if ( mergeval( &xx[afld1], separator, xx[afld1].val ) == ERR ) {
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return( exitcode );
			}
		}
		else {
			if(!firstline) {
				int j;

				zz[afld1].val = attrbuf;
				size_newrec = 0;
				for(i=0;i<nattr1;i++) {
					if ((j = putnrec(utfp2,&zz[i])) < 0) {
						error(E_DATFWRITE, prog, tblname);
						if (packed) {
							/* EOF == FALSE */
							(void)packedclose(utfp1,FALSE);
							utfp1 = NULL;
						}
						return(exitcode);
					}
					size_newrec += j;
				}
				if(endf1)
				{
					if (putc('\n',utfp2) == EOF) {
						error(E_DATFWRITE, prog, tblname);
						if (packed) {
							/* EOF == FALSE */
							(void)packedclose(utfp1,FALSE);
							utfp1 = NULL;
						}
						return(exitcode);
					}
					size_newrec++;
				}
				merged++;
				if ( size_newrec > MAXREC )
				{
					error( E_GENERAL, "%s: Error: new rec# %d would be longer (%d) than maximum record length (%d)\n",
						prog, merged, size_newrec, MAXREC );
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return( exitcode );
				}
				if(ret < 0) {
					error(E_GENERAL,
					"%s: Error: rec# %d in %s is not in sorted order:\n",
						prog, argv[3], recordnr );
					/* print out record out of order */
					for(i=0;i<nattr1;i++) {
						if(xx[i].flag == WN)
							xx[i].val[xx[i].flen]=
								'\0';
						error(E_GENERAL,xx[i].val);
						if(xx[i].flag == T)
							error(E_GENERAL,
							&xx[i].inf[1]);
					}
					if(endf1)
						error(E_GENERAL,"\n");
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return( exitcode );
				}
			}
			else
				firstline = 0;

			if ( copyrec() == ERR ) {
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return( exitcode );
			}

			attrbuf = ibufptr;	/* save start of value */
			if ( mergeval( &xx[afld1], '\0', xx[afld1].val ) == ERR ) {
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return( exitcode );
			}
		}
	}
	if (!firstline) {
		int j;

		zz[afld1].val = attrbuf;
		size_newrec = 0;
		for(i=0;i<nattr1;i++) {
			if ((j = putnrec(utfp2,&zz[i])) < 0) {
				error(E_DATFWRITE, prog, tblname);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			size_newrec += j;
		}
		if(endf1)
		{
			if (putc('\n',utfp2) == EOF) {
				error(E_DATFWRITE, prog, tblname);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			size_newrec++;
		}
		merged++;
		if ( size_newrec > MAXREC )
		{
			error( E_GENERAL, "%s: Error: new rec# %d would be longer (%d) than maximum record length (%d)\n",
				prog, merged, size_newrec, MAXREC );
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return( exitcode );
		}
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
		error(E_GENERAL,"%s: %d records merged from %s.\n",
			prog,merged,argv[3]);
	exitcode = 0;
	Dtable2[0] = '\0';	/* don't remove */
	table2 = NULL;
	return(exitcode);
}

static int
copyrec()
{
	int	j;

	ibufptr=ibuf;
	for(j=0;j<nattr1;j++)
	{
		if ( j != afld1 )
		{
			zz[j].val = ibufptr;
			if ( mergeval( &zz[j], '\0', xx[j].val ) == ERR )
				return( ERR );
			ibufptr++;	/* move past null terminator */
		}
	}
	return(0);
}

static int
mergeval( fmt, delim, val )
struct fmt *fmt;
char delim;
char *val;
{
	register int len, incr;

	len = fmt->flag == WN ? fmt->flen : strlen( val );
	incr = ( delim != '\0') ? 2 : 1;

	if ( ibufptr + len + incr > &ibuf[MAXREC] )
	{
		error( E_GENERAL, "%s: Error: merged record is longer (%d) than maximum record size (%d)\n",
			prog, ibufptr + len + incr - ibuf, MAXREC );
		return( ERR );
	}
	if( delim != '\0')
		*ibufptr++ = delim;
	strncpy( ibufptr, val, len );
	ibufptr += len;
	*ibufptr = '\0';	/* don't increment */

	return( 0 );
}

static int
same()
{
	int i,j;

	for(i=0;i<nattr1;i++) {
		if(i == afld1) continue;
		j=strcmp(xx[i].val,zz[i].val);
		if(j < 0)
			return(-1);	/* record out of order */
		else if(j> 0)
			return(1);
	}
	return(0);		/* the same */
}
