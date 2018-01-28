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
#include <signal.h>
#define MAXJOIN 20
static	int	exitcode;


#ifdef REGJOIN
/* include the stuff from regexp.h */
extern int circf;
extern char *compile();
extern int step();

static char express[MAXJOIN][256];
static int circum[MAXJOIN];
#endif

static	char	*prog, *tblname;
static	int	nattr0, nattr1, nattr2, endf0, endf1, endf2;
static	struct	fmt xx[MAXATT],  yy[MAXATT],  zz[MAXATT];
static char xx_friendly[ MAXATT ][ MAXUNAME + 1 ];
static char yy_friendly[ MAXATT ][ MAXUNAME + 1 ];
static char zz_friendly[ MAXATT ][ MAXUNAME + 1 ];
static	char	ibuf[MAXREC], *ibufptr,*back;
extern	char	*strcat(), *strcpy(), *strrchr(), *charrep();
static	short	attr1[MAXJOIN], attr2[MAXJOIN];
static	int	fld1, fld2;
extern	int	end_of_tbl;
extern	char	*malloc();

extern	char	*table2, Dtable2[],*stdbuf;
extern	char	DtmpItable[];
extern	FILE	*utfp0, *utfp1, *utfp2, *udfp1, *udfp2;
extern	FILE	*getdescrwtbl();

extern void reset(), savebackup(), backup();

#if defined(REGJOIN) || defined(OUTERJOIN)

#ifdef REGJOIN
regjoin(argc, argv)
#endif

#ifdef OUTERJOIN
outerjoin(argc, argv)
#endif

#else
ujoin(argc, argv)
#endif
char	*argv[];
int	argc;
{	
	int	i, j, joined, match, coption, qoption, onto=0, prntflg;
	char	Dtable0[MAXPATH+4], name[MAXPATH+4], *Ioption1, *Ioption2, *Ooption;
	char	dtable2[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	*t1, *t2;
	int	indx;
	char	*keyval;
	struct	index *tree;
	long	seekval, num, loc,l;
#ifdef OUTERJOIN
	int	found_one;
#endif
	int recordnr0, recordnr1;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}

	qoption = joined = endf2 = endf0 = endf1 = 0;
	coption = 0;

	Ooption = Ioption1 = Ioption2 = stdbuf = table2 = NULL;
	dtable2[0] = '\0';
	Dtable2[0] = '\0';
	DtmpItable[0] = '\0';
	exitcode = 1;

	while ( argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		switch(argv[1][1]) {
			default:
				error(E_BADFLAG,prog,argv[1]);
				return(exitcode);
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
						error(E_BADFLAG,prog,&argv[1][i]);
						return(exitcode);
					}
				break;
			case 'I':
				if (Ioption1)
					Ioption2 = &argv[1][2];
				else
					Ioption1 = &argv[1][2];
				break;
			case 'O':
				Ooption = &argv[1][2];
				break;
		}
		argc--;
		argv++;
	}
	if(argc == 8 && strcmp(argv[6],"onto") == 0)
		onto = 1;
	if ( ( argc != 6 && argc != 8 ) ||
		strcmp( argv[3], "from" ) != 0 ||
		( argc == 8 && ! onto && strcmp( argv[6], "into" ) != 0 ) )
	{
		error(E_GENERAL,
"Usage: %s [-c] [-q] [-Itable1] [-Itable2] [-Otable] a1a[,a1b,...] \\\n\ta2a[,a2b,...] from table1 table2 [into table3]\n",
		prog);
		return(exitcode);
	}
	if(strcmp(argv[4],"-") == 0) {
		if(!Ioption1) {
			/*
			 * Check for/get description from the input file.
			 */
			if (getdescrwtbl(stdin, &Ioption1) == NULL) {
				error(E_GENERAL,"%s: No description file specified.\n",
					prog);
				return(exitcode);
			}
		}
		if(strcmp(argv[5],"-") == 0) {
			error(E_GENERAL,
		"%s: Cannot have both input files come from standard input\n",
			prog);
			return(exitcode);
		}
		utfp0 = stdin;
	}
	else {
		if((utfp0=fopen(argv[4],"r"))==NULL) {
			error(E_DATFOPEN,prog,argv[4]);
			return(exitcode);
		}
	}
	if(strcmp(argv[5],"-") == 0) {
#if defined(OUTERJOIN) || defined(REGJOIN)
		/* for outerjoin command, second table cannot be
		   the standard input since algorithm must plow
		   through every record i first file to see if
		   there is a match;  also, not allowed for regjoin
		   since expressions must be compiled from first file */
		error(E_GENERAL,"%s:  Second table may not be standard input\n",
			prog);
		return(exitcode);
#else
		if(!Ioption2) {
			/*
			 * Check for/get description from the input file.
			 */
			if (getdescrwtbl(stdin, &Ioption2) == NULL) {
				error(E_GENERAL,
				"%s: Second description file not specified.\n",
					prog);
				return(exitcode);
			}
		}
		utfp1 = stdin;
#endif
	}
	else {
		if ((utfp1 = fopen(argv[5],"r")) == NULL) {
			error(E_DATFOPEN,prog,argv[5]);
			return(exitcode);
		}
	}

	nattr0 = _mktbl( prog, argv[4], Dtable0, xx, xx_friendly, MAXATT, Ioption1 );
	if( nattr0 == ERR )
		return( exitcode );

	/* get join fields for first file */
	for(i=fld1=0;fld1<MAXJOIN && argv[1][i] != '\0';fld1++) {
		while(argv[1][i] == ' ' || argv[1][i] == ',')
			i++;		/* skip over comma and white space */
		if(argv[1][i] == '\0')	/* end of argument */
			break;
		for(j=0;argv[1][i] != '\0' && argv[1][i] != ' ' &&
			argv[1][i] != ','; j++,i++)
			name[j] = argv[1][i];	/* get field name */
		name[j] = '\0';
		if ((attr1[fld1]=setnum(xx,name,nattr0)) ==ERR) {
			error(E_ILLATTR,prog,name,Dtable0);
			return(exitcode);
		}
	}
	if(argv[1][i] != '\0') {
		error(E_GENERAL,"%s: too many fields being joined on.\n",prog);
		return(exitcode);
	}
	if(fld1 == 0) {
		error(E_GENERAL,"%s: no join field specified for %s.\n",
			argv[4]);
		return(exitcode);
	}

	nattr1 = _mktbl( prog, argv[5], Dtable0, yy, yy_friendly, MAXATT, Ioption2 );
	if( nattr1 == ERR )
		return( exitcode );

	/* get join fields for second file */
	for(i=fld2=0;fld2<MAXJOIN && argv[2][i] != '\0';fld2++) {
		while(argv[2][i] == ' ' || argv[2][i] == ',')
			i++;
		if(argv[2][i] == '\0')
			break;
		for(j=0;argv[2][i] != '\0' && argv[2][i] != ' ' &&
			argv[2][i] != ','; j++,i++)
			name[j] = argv[2][i];
		name[j] = '\0';
		if ((attr2[fld2]=setnum(yy,name,nattr1)) ==ERR) {
			error(E_ILLATTR,prog,name,Dtable0);
			return(exitcode);
		}
		yy[attr2[fld2]].justify = '\01';	/* indicate joined on */
	}
	if(argv[2][i] != '\0') {
		error(E_GENERAL,"%s: too many fields being joined on.\n",prog);
		return(exitcode);
	}
	if(fld2 == 0) {
		error(E_GENERAL,"%s: no join field specified for %s.\n",
			argv[5]);
		return(exitcode);
	}

	if(fld1 != fld2) {
		error(E_GENERAL,
		"%s: Number of attributes to be joined on does not match.\n",
			prog);
		return(exitcode);
	}

	for(i=0;i<nattr0;i++) {		/* create output description */
		zz[i].flag = xx[i].flag;
		zz[i].flen = xx[i].flen;
		strcpy(zz[i].aname, xx[i].aname);
		strcpy(zz[i].inf, xx[i].inf);
		zz[i].prnt = xx[i].prnt;
		zz[i].justify = xx[i].justify;
		strcpy( zz_friendly[i], xx_friendly[i] );
	}
	prntflg = (argc == 6) ? 1 : 0;
	if(nattr1 != fld2) {	/* one or more of attr in 2nd file remain */
		if(zz[nattr0-1].flag == T)
			zz[nattr0-1].inf[1] = '\t';
		for(i=0,j=nattr0;i<nattr1;i++,j++) {
			if (yy[i].justify == '\01') {
			/* attribute being joined on - don't include */
				j--;		/* don't bump j counter */
				continue;
			}
			zz[j].flag = yy[i].flag;
			zz[j].flen = yy[i].flen;
			strcpy(zz[j].aname, yy[i].aname);
			strcpy(zz[j].inf, yy[i].inf);
			zz[j].prnt = yy[i].prnt;
			zz[j].justify = yy[i].justify;
			if (!prntflg && setnum(xx,yy[i].aname,nattr0)!=ERR) {
				error(E_GENERAL,
					"%s: duplicate attribute %s\n",
					prog,yy[i].aname);
				if (strlen(zz[j].aname) >= ANAMELEN) {
					error(E_GENERAL,
						"Unable to rename maximum length (%d) attribute!\n",
						prog, ANAMELEN);
					return(exitcode);
				}
				error(E_GENERAL,
					"Second attribute renamed %s1\n",
					yy[i].aname);
				strcat(zz[j].aname,"1");
			}

			strcpy( zz_friendly[j], yy_friendly[i] );
		}
	}
	nattr2 = nattr0 + nattr1 - fld1;
	if(zz[nattr2-1].flag == T && zz[nattr2-1].inf[1] != '\n')
		/* final terminator must be newline */
		zz[nattr2-1].inf[1] = '\n';

	for(i=0;i<fld1;i++)
		xx[attr1[i]].justify = '\01';	/* indicate joined on */
#ifdef REGJOIN
	/* use name from second file since attribute in first file is
	   the regular expression, not the actual value */
	for(i=0;i<fld1;i++)
		strcpy(zz[attr1[i]].aname,yy[attr2[i]].aname);
#endif

	if (xx[nattr0-1].flag == WN) endf0 = 1;
	if (yy[nattr1-1].flag == WN) endf1 = 1;
	if (zz[nattr2-1].flag == WN) endf2 = 1;

	if(prntflg) {

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
	else {
		if(strcmp(argv[7],"-") != 0) {
			if(chkaccess(argv[7],00) == 0)
			{
				if (!onto ) {
					error(E_EXISTS,prog,argv[7]);
					return(exitcode);
				}
			}
			else
				table2 = argv[7];

			utfp2 = fopen(argv[7], onto ? "a" : "w" );
			if(utfp2 == NULL)
			{
				error(E_DATFOPEN,prog,argv[7]);
				return(exitcode);
			}
			if (Ooption) {
				if (!(*Ooption)) {
					/* do not create output descriptor file */
					dtable2[0] = '\0';
				} else if (strcmp(argv[7], Ooption) == 0) {
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
				getfile(dtable2,argv[7],0);
			}
		} else {
			if ((Ooption) && (*Ooption)) {
				getfile(dtable2,Ooption,0);
			} else if ((!(Ooption)) && (coption)) {
				/* make sure dtable2 is null ("") */
				dtable2[0] = '\0';
			} else {
				error(E_GENERAL,
				"%s: No output description file specified.\n",
					prog);
				return(exitcode);
			}
			utfp2 = stdout;
			if((stdbuf=malloc(BUFSIZ)) != NULL)
				setbuf(stdout,stdbuf);
		}
		tblname = argv[7];	/* current output table name */

		/* check output description file */
		if (dtable2[0]) {
			if (chkaccess(dtable2,00) == 0) {
				if (!onto) {
					error(E_EXISTS,prog,dtable2);
					return(exitcode);
				}
				dtable2[0] = '\0';
			}
		}
	}

	/* create output description/file if needed */
	if ((*dtable2) || ((coption) && (utfp2 == stdout) && (Ooption == NULL)))
	{
		FILE *dfp;

		strcpy(Dtable2,dtable2);

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
			fprintf(dfp,"%s\t",zz[i].aname);
			switch(zz[i].flag) {
			case WN:
				fprintf(dfp,"w%d",zz[i].flen);
				break;
			case T:
				fprintf(dfp,"t%s",charrep(zz[i].inf[1]));
				break;
			}
			/* print print width and justification */
			fprintf(dfp, "\t%d%c", zz[i].prnt, zz[i].justify);

			if ( strcmp( zz[i].aname, zz_friendly[i] ) )
				fprintf(dfp, "\t%s\n", zz_friendly[i]);
			else
				fprintf(dfp, "\t\n");
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

	t1 = argv[4];	/* for normal ujoin case */
	t2 = argv[5];	/* for normal ujoin case */
#ifdef REGJOIN
	t1 = "";	/* can't use indexes */
	t2 = "";	/* can't use indexes */
#endif
#ifdef OUTERJOIN
	t1 = "";	/* can't use index on first table, even if it exists */
#endif

	indx = indexch(t2, yy[attr2[0]].aname, t1, xx[attr1[0]].aname,
		&udfp1,&tree);

	if(indx == 0) {	/* index exists for argv[5] - this won't be used
			   for regjoin, only used for ujoin and outerjoin */
		recordnr0 = 0;
		for(;;) {
			recordnr0++;
			end_of_tbl = 0;
			newrec();
			for(i=0;i<nattr0 && getrec(utfp0,&xx[i])!=ERR; i++);
			if ((i == 0) && (end_of_tbl)) {
				close(tree->fd);

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

				if (utfp0 != stdin)
					fclose(utfp0);
				utfp0 = NULL;

				if(!qoption)
					error(E_GENERAL,
				"%s: %d records joined from %s and %s.\n",
				prog,joined,argv[4],argv[5]);

				exitcode = 0;
				Dtable2[0] = '\0';
				table2 = NULL;
				return(exitcode);
			}
 			else if ( i < nattr0 )
			{
				error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
					prog, i, recordnr0, argv[4] );
				return( exitcode );
			}
			if (endf0) 
				if ((feof(utfp0)) || ((i = getc(utfp0)) == EOF)) {
					error( E_GENERAL,
						"%s: Error: missing newline on rec# %d in %s\n",
						prog, recordnr0, argv[4] );
					return(exitcode);
				} else if (i != '\n') {
					error( E_GENERAL,
						"%s: Error: data overrun on rec# %d in %s\n",
						prog, recordnr0, argv[4] );
					return(exitcode);
				}
#ifdef OUTERJOIN
			found_one = 0;
			reset();	/* reset buffer pointer */
			if ( copyfirst() == ERR )
				return( exitcode );
			if ( copy1j() == ERR )
				return( exitcode );
			savebackup();
#endif
			if(rdindexed(tree,xx[attr1[0]].val,&keyval,&seekval)
				==FOUND) {
#ifndef OUTERJOIN
				reset();	/* reset buffer pointer */
				if ( copyfirst() == ERR )
					return( exitcode );
				if ( copy1j() == ERR )
					return( exitcode );
				savebackup();
#endif

				/* get the record in the second file */
				fseek(udfp1,seekval,0);
				fread((char *)&num,sizeof(long),1,udfp1);
				for(l=0;l< -num;l++) {
					fread((char *)&loc,sizeof(long),1,
						udfp1);
					fseek(utfp1,loc,0);
					newrec();
					for(j=0;j<nattr1 &&
						 getrec(utfp1,&yy[j])!=ERR;j++);
		 			if ( j < nattr1 )
					{
						error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of record at seek# %ld in %s\n",
							prog, j, loc, argv[5] );
						return( exitcode );
					}
					match=1;
					for(j=1;j<fld1;j++) {
						if (strcmp(zz[attr1[j]].val,
							yy[attr2[j]].val)!=0) {
							match = 0;
							break;
						}
					}
					if(!match)
						continue;
#ifdef OUTERJOIN
					found_one = 1;
#endif
					backup();
					if ( copysecond() == ERR )
						return( exitcode );
					if ( output() == ERR )
						return( exitcode );
					joined++;
				}
			}
#ifdef OUTERJOIN
			if(!found_one) {
				backup();
				/* null out rest of record */
				if ( copynull() == ERR )
					return( exitcode );
				if ( output() == ERR )
					return( exitcode );
				joined++;
			}
#endif
		}
	}
	else if (indx == 1) {	/* index exists for argv[4] - this is
				   never used for regjoin or outerjoin */
		recordnr1 = 0;
		for(;;) {
			recordnr1++;
			end_of_tbl = 0;
			newrec();
			for(i = 0; i < nattr1 && getrec(utfp1,&yy[i])!=ERR;i++);
			if ((i == 0) && (end_of_tbl))
			{
				close(tree->fd);

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

				if (utfp0 != stdin)
					fclose(utfp0);
				utfp0 = NULL;

				if(!qoption)
					error(E_GENERAL,
				"%s: %d records joined from %s and %s.\n",
				prog,joined,argv[4],argv[5]);

				exitcode = 0;
				Dtable2[0] = '\0';
				table2 = NULL;
				return(exitcode);
			}
			else if ( i < nattr0 )
			{
				error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
					prog, i, recordnr1, argv[5] );
				return( exitcode );
			}
			if (endf1) 
				if ((feof(utfp1)) || ((i = getc(utfp1)) == EOF)) {
					error( E_GENERAL,
						"%s: Error: missing newline on rec# %d in %s\n",
						prog, recordnr1, argv[5] );
					return(exitcode);
				} else if (i != '\n') {
					error( E_GENERAL,
						"%s: Error: data overrun on rec# %d in %s\n",
						prog, recordnr1, argv[5] );
					return(exitcode);
				}
			if (rdindexed(tree,yy[attr2[0]].val,&keyval,&seekval)
				!=FOUND)
				continue;
			reset();		/* reset buffer pointer */
			if ( copysecond() == ERR )
				return( exitcode );
			if ( copy2j() == ERR )
				return( exitcode );
			savebackup();
			fseek(udfp1,seekval,0);
			fread((char *)&num,sizeof(long),1,udfp1);
			for(l = 0; l < -num; l++) {
				fread((char *)&loc,sizeof(long),1,udfp1);
				fseek(utfp0,loc,0);
				newrec();
				for(j=0;j<nattr0 && getrec(utfp0,&xx[j])!=ERR;
					j++);
	 			if ( j < nattr0 )
				{
					error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of record at seek# %ld in %s\n",
						prog, j, loc, argv[4] );
					return( exitcode );
				}
				match=1;
				for(j=1;j<fld1;j++) {
					if (strcmp(xx[attr1[j]].val,
						zz[attr1[j]].val)!=0) {
						match = 0;
						break;
					}
				}
				if(!match)
					continue;
				backup();
				if ( copyfirst() == ERR )
					return( exitcode );
				if ( output() == ERR )
					return( exitcode );
				joined++;
			}
		}
	}

	/* no index exists for either aname1 or aname2 */

	if(utfp1 != stdin) {	/* second table is not standard input -
				   if indexes don't exist, this first
				   case must be used for both outerjoin
				   and regjoin (checked above); default
				   for ujoin if neither table is standard input.
				*/
		recordnr0 = 0;
		for(;;) {
#ifdef OUTERJOIN
			found_one = 0;
#endif
			recordnr0++;
			end_of_tbl = 0;
			newrec();
			for(i=0;i<nattr0 && getrec(utfp0,&xx[i])!=ERR; i++);
			if ((i == 0) && (end_of_tbl))
				break;
			else if ( i < nattr0 )
			{
				error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
					prog, i, recordnr0, argv[4] );
				return( exitcode );
			}
			if (endf0) 
				if ((feof(utfp0)) || ((i = getc(utfp0)) == EOF)) {
					error( E_GENERAL,
						"%s: Error: missing newline on rec# %d in %s\n",
						prog, recordnr0, argv[4] );
					return(exitcode);
				} else if (i != '\n') {
					error( E_GENERAL,
						"%s: Error: data overrun on rec# %d in %s\n",
						prog, recordnr0, argv[4] );
					return(exitcode);
				}
			reset();		/* reset buffer pointer */
			if ( copyfirst() == ERR )
				return( exitcode );
#ifdef REGJOIN
			for(i=0;i<fld1;i++) {
				/* compile regular expressions */
				compile(xx[attr1[i]].val,&express[i][0],
					&express[i][256],'\0');
				circum[i] = circf;
			}
#else
			if ( copy1j() == ERR )
				return( exitcode );
#endif
			savebackup();
			rewind(utfp1);
			recordnr1 = end_of_tbl = 0;
			for(;;) {
				recordnr1++;
				newrec();
				for(i=0;i<nattr1 && getrec(utfp1,&yy[i])!=ERR;
					i++);
				if ((i == 0) && (end_of_tbl))
					break;
				else if ( i < nattr1 )
				{
					error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
						prog, i, recordnr1, argv[5] );
					return( exitcode );
				}
				if (endf1) 
					if ((feof(utfp1)) || ((i = getc(utfp1)) == EOF)) {
						error( E_GENERAL,
							"%s: Error: missing newline on rec# %d in %s\n",
							prog, recordnr1, argv[5] );
						return(exitcode);
					} else if (i != '\n') {
						error( E_GENERAL,
							"%s: Error: data overrun on rec# %d in %s\n",
							prog, recordnr1, argv[5] );
						return(exitcode);
					}
				match=1;
				for(i=0;i<fld1;i++) {
#ifdef REGJOIN
					circf = circum[i];
					if(!step(yy[attr2[i]].val,express[i]))
#else
					if(strcmp(zz[attr1[i]].val,
						yy[attr2[i]].val)!=0)
#endif
					{
						match = 0;
						break;
					}
				}
				if(!match)
					continue;
				backup();
				if ( copysecond() == ERR )
					return( exitcode );
#ifdef REGJOIN
				if ( copy2j() == ERR )
					return( exitcode );
#endif
				if ( output() == ERR )
					return( exitcode );
				joined++;
#ifdef OUTERJOIN
				found_one = 1;
#endif
			}
#ifdef OUTERJOIN
			if(!found_one) {
				backup();
				/* null out rest of record */
				if ( copynull() == ERR )
					return( exitcode );
				if ( output() == ERR )
					return( exitcode );
				joined++;
			}
#endif
		}
	}
	else {	/* first table is not standard input - this case is only
		   used for normal ujoin where no indexes and second table
		   is standard input */
		recordnr1 = 0;
		for(;;) {
			recordnr1++;
			end_of_tbl = 0;
			newrec();
			for(i = 0; i < nattr1 && getrec(utfp1,&yy[i])!=ERR;i++);
			if ((i == 0) && (end_of_tbl))
				break;
			else if ( i < nattr1 )
			{
				error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
					prog, i, recordnr1, argv[5] );
				return( exitcode );
			}

			if (endf1) 
				if ((feof(utfp1)) || ((i = getc(utfp1)) == EOF)) {
					error( E_GENERAL,
						"%s: Error: missing newline on rec# %d in %s\n",
						prog, recordnr1, argv[5] );
					return(exitcode);
				} else if (i != '\n') {
					error( E_GENERAL,
						"%s: Error: data overrun on rec# %d in %s\n",
						prog, recordnr1, argv[5] );
					return(exitcode);
				}
			reset();		/* reset buffer pointer */
			if ( copysecond() == ERR )
				return( exitcode );
			if ( copy2j() == ERR )
				return( exitcode );
			savebackup();

			rewind(utfp0);
			recordnr0 = end_of_tbl = 0;
			for(;;) {
				recordnr0++;
				newrec();
				for(j=0;j<nattr0 && getrec(utfp0,&xx[j])!=ERR;
					j++);
				if ((j == 0) && (end_of_tbl))
					break;
				else if ( j < nattr0 )
				{
					error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
						prog, j, recordnr0, argv[4] );
					return( exitcode );
				}
				if(endf0) 
					if ((feof(utfp0)) || ((j = getc(utfp0)) == EOF)) {
						error( E_GENERAL,
							"%s: Error: missing newline on rec# %d in %s\n",
							prog, recordnr0, argv[4] );
						return(exitcode);
					} else if (j != '\n') {
						error( E_GENERAL,
							"%s: Error: data overrun on rec# %d in %s\n",
							prog, recordnr0, argv[4] );
						return(exitcode);
					}
				match=1;
				for(j=1;j<fld1;j++) {
					if (strcmp(xx[attr1[j]].val,
						zz[attr1[j]].val)!=0) {
						match = 0;
						break;
					}
				}
				if(!match)
					continue;
				backup();
				if ( copyfirst() == ERR )
					return( exitcode );
				if ( output() == ERR )
					return( exitcode );
				joined++;
			}
		}
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
	}
	utfp2 = NULL;

	if (utfp1 != stdin)
		fclose(utfp1);
	utfp1 = NULL;

	if (utfp0 != stdin)
		fclose(utfp0);
	utfp0 = NULL;

	if(!qoption)
		error(E_GENERAL,"%s: %d records joined from %s and %s.\n",
			prog,joined,argv[4],argv[5]);
	exitcode = 0;
	Dtable2[0] = '\0';
	table2 = NULL;
	return(exitcode);
}

static void
reset()
{
	/* reset buffer pointer to beginning of buffer */
	ibufptr=ibuf;
}

static void
savebackup()
{
	back=ibufptr;
}

static void
backup()
{
	ibufptr = back;
}

static int
copyfirst()
{
	int	j;

	/* copy fields for first file except joined fields */
	for(j=0;j<nattr0;j++) {
		if(xx[j].justify != '\01')
		{
			if ( copyfld(&xx[j],&zz[j]) == ERR )
				return( ERR );
		}
	}

	return( 0 );
}

static int
copy1j()
{
	/* copy fields for joined fields from first file */
	int	j;
	for(j=0;j<fld1;j++)
	{
		if ( copyfld(&xx[attr1[j]],&zz[attr1[j]]) == ERR )
			return( ERR );
	}

	return( 0 );
}

static int
copysecond()
{
	/* copy fields for second file except joined fields */
	int	i,j;

	for(i=0,j=nattr0;i<nattr1;i++) {
		if (yy[i].justify != '\01') {
			if ( copyfld(&yy[i],&zz[j]) == ERR )
				return( ERR );
			j++;
		}
	}

	return( 0 );
}

static int
copy2j()
{
	/* copy fields for joined fields from second file */
	int	j;

	for(j=0;j<fld2;j++)
	{
		if ( copyfld(&yy[attr2[j]],&zz[attr1[j]]) == ERR )
			return( ERR );
	}

	return( 0 );
}

static int
output()
{
	int j, k;
	int size_newrec;

	size_newrec = 0;
	for(j=0;j<nattr2;j++) {
		if ((k = putnrec(utfp2,&zz[j])) < 0) {
			error(E_DATFWRITE,prog,tblname);
			return(ERR);
		}
		size_newrec += k;
	}
	if (endf2 == 1)
	{
		if (putc('\n',utfp2) == EOF) {
			error(E_DATFWRITE,prog,tblname);
			return(ERR);
		}
		size_newrec++;
	}

	if ( size_newrec > MAXREC )
	{
		error( E_GENERAL, "Error: new record is longer (%d) than maximum record length (%d)\n",
			size_newrec, MAXREC );
		return( ERR );
	}

	return( 0 );
}

static int
copyfld(a,b)
struct fmt *a,*b;
{
	register char	*xb;
	register int	len;

	xb=b->val=ibufptr;
	switch(a->flag) {
	case WN:
		len = a->flen;
		break;
	case T:
		len = strlen( a->val );
		break;
	}
	if ( xb + len + 1 > &ibuf[ MAXREC ] )
	{
		error( E_GENERAL, "Error: new record would be longer (%d) than maximum record length (%d)\n",
			xb + len + 1 - ibuf, MAXREC );
		return( ERR );
	}
	strncpy( xb, a->val, len );
	xb += len;
	*xb++ = '\0';

	ibufptr=xb;

	return( 0 );
}

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

#ifdef OUTERJOIN
static int
copynull()
{
	/* copy fields for second file except joined fields putting
	   in null or blank values */
	int	i,j;

	for(i=0,j=nattr0;i<nattr1;i++) {
		if (yy[i].justify != '\01') {
			if (copy2null(&yy[i],&zz[j]) == ERR )
				return(ERR);
			j++;
		}
	}
	return(0);
}

static int
copy2null(a,b)
struct fmt *a,*b;
{
	char	*xb;
	int	i;

	switch(a->flag) {
	case WN:
		xb = b->val = ibufptr;
		if ( xb + a->flen + 1 > &ibuf[MAXREC] )
		{
			error( E_GENERAL, "Error: new record would be longer (%d) than maximum record length (%d)\n",
				xb + a->flen + 1 - ibuf, MAXREC );
			return( ERR );
		}
		for(i=0;i<a->flen;i++)
			*xb++ = ' ';	/* pad with blanks */
		*xb++='\0';
		ibufptr = xb;
		break;
	case T:
		if ( ibufptr == ibuf )
		{
			*ibufptr = '\0';
			b->val = ibufptr++;
		}
		else	/* use NULL of last string */
			b->val = &ibufptr[-1];
		break;
	}

	return( 0 );
}
#endif
