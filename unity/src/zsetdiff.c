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
#define MAXJOIN 20

static	char	*prog;
extern	FILE	*utfp0, *utfp1, *utfp2, *udfp1;
static	int	nattr0, nattr1, endf0, endf1;
static	struct	fmt xx[MAXATT],  yy[MAXATT];
static	char	ibuf[MAXREC], *ibufptr;
extern	char	*strcat(), *strcpy(), *strrchr(), *malloc();
extern	FILE	*getdescrwtbl(), *putdescrwtbl();
static	short	attr1[MAXJOIN], attr2[MAXJOIN];
static	int	fld1, fld2;

extern	char	*table2, Dtable2[],*stdbuf;
extern	char	DtmpItable[];
extern	int	end_of_tbl;
static	int	exitcode;

setdiff(argc, argv)
char	*argv[];
int	argc;
{	
	char	Dtable0[MAXPATH+4], Dtable1[MAXPATH+4], *Ioption1, *Ioption2;
	char	dtable2[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	*tblname;
	char	name[MAXPATH+4], *Ooption;
	int	i, j, difference, indx, match, coption, qoption, onto=0;
	char	*keyval;
	struct	index *tree;
	long	seekval, num, loc,l;
	int recordnr0, recordnr1;	/* record number counters */

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}

	qoption = coption = difference = endf0 = endf1 = 0;
	Ooption = Ioption2 = Ioption1 = stdbuf = table2 = NULL;
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
			case 'O':
				Ooption = &argv[1][2];
				break;
			case 'I':
				if(Ioption1)
					Ioption2 = &argv[1][2];
				else
					Ioption1 = &argv[1][2];
				break;
		}
		argc--;
		argv++;
	}
	if(argc == 8 && strcmp(argv[6],"onto") == 0)
		onto = 1;
	if(argc != 6 && !(argc == 8 && (onto || strcmp(argv[6],"into")==0)) ||
		strcmp(argv[3],"from")) {  
		error(E_GENERAL,
"Usage: %s [-c] [-q] [-Itable1] [-Itable2] a1a[,a1b,...] a2a[,a2b,...] \\\n\tfrom table1 table2 [into table3]\n",
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
		utfp0 = stdin;
	}
	else {
		if ((utfp0=fopen(argv[4],"r"))==NULL) {
			error(E_DATFOPEN,prog,argv[4]);
			return(exitcode);
		}
	}
	if(strcmp(argv[5],"-") == 0) {
		/* second table cannot be stdin - must be able to rewind */
		error(E_GENERAL,
			"%s: second table cannot be standard input.\n", prog);
		return(exitcode);
	}
	else {
		if ((utfp1 = fopen(argv[5],"r")) == NULL) {
			error(E_DATFOPEN,prog,argv[5]);
			return(exitcode);
		}
	}

	if((nattr0=mkntbl(prog,argv[4],Dtable0,xx,Ioption1))==ERR)
		return(exitcode);

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
		error(E_GENERAL,"%s: too many fields being differenced.\n",
			prog);
		return(exitcode);
	}
	if(fld1 == 0) {
		error(E_GENERAL,"%s: no difference field specified for %s.\n",
			argv[4]);
		return(exitcode);
	}

	if((nattr1=mkntbl(prog,argv[5],Dtable1,yy,Ioption2))==ERR)
		return(exitcode);

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
			error(E_ILLATTR,prog,name,Dtable1);
			return(exitcode);
		}
	}
	if(argv[2][i] != '\0') {
		error(E_GENERAL,"%s: too many fields being differenced.\n",
			prog);
		return(exitcode);
	}
	if(fld2 == 0) {
		error(E_GENERAL,"%s: no difference field specified for %s.\n",
			argv[5]);
		return(exitcode);
	}

	if(fld1 != fld2) {
		error(E_GENERAL,
		"%s: Uneven number of attributes to be differenced.\n",prog);
		return(exitcode);
	}

	if (xx[nattr0-1].flag == WN) endf0 = 1;
	if (yy[nattr1-1].flag == WN) endf1 = 1;

	if(argc == 6) {

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
			if(chkaccess(argv[7],00) == 0) {
				if(!onto) {
					error(E_EXISTS,prog,argv[7]);
					return(exitcode);
				}
			}
			else
				table2 = argv[7];
			if(onto)
				utfp2 = fopen(argv[7],"a");
			else
				utfp2 = fopen(argv[7],"w");
			if(utfp2 == NULL) {
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
				return(exitcode);
			}
			utfp2 = stdout;
			if((stdbuf=malloc(BUFSIZ)) != NULL)
				setbuf(stdout,stdbuf);
		}

		/* check output description file */
		if ((dtable2[0]) &&
		    (strcmpfile(Dtable0,dtable2) != 0)) {
			if(chkaccess(dtable2,00) == 0) {
				if(!onto) {
					error(E_EXISTS,prog,dtable2);
					return(exitcode);
				}
			} else {
				strcpy(Dtable2,dtable2);
				if(copy(prog,Dtable0,Dtable2)!=0)
					return(exitcode);
			}
		}
		tblname = argv[7];	/* current output table name */
	}

	if ((coption) && (utfp2 == stdout) && (Ooption == NULL)) {
		if (putdescrwtbl(stdout, Dtable0) == NULL) {
			error(E_DATFOPEN,prog,"-");
			return(exitcode);
		}
	}

	recordnr0 = 0;

	/* check for index - note that second table cannot be stdin */
	indx = indexch(argv[5],yy[attr2[0]].aname,"\0","\0",&udfp1, &tree);
	if(indx == 0) {	/* index exists for argv[5] */
		for(;;) {
			recordnr0++;
			end_of_tbl = 0;
			newrec();
			for(i=0;i<nattr0 && getrec(utfp0,&xx[i])!=ERR; i++);
			if ((i == 0 ) && (end_of_tbl)) {
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
					"%s: %d records difference from %s and %s.\n",
					prog,difference,argv[4],argv[5]);
				exitcode = 0;
				table2 = NULL;
				Dtable2[0] = '\0';
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
			reset();
			copyfirst();
			if ( rdindexed(tree,xx[attr1[0]].val,&keyval,&seekval)
				!=FOUND) { /* write out record */
				if (output() == ERR) {
					error(E_DATFWRITE, prog, tblname);
					return(exitcode);
				}
				difference++;
			} else {
				fseek(udfp1,seekval,0);
				fread((char *)&num,sizeof(long),1,udfp1);
				match = 0;
				for(l=0;l< -num;l++) {
					fread((char *)&loc,sizeof(long),1,udfp1);
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
					for(i=0;i<fld1;i++) {
						if (strcmp(xx[attr1[i]].val,
							yy[attr2[i]].val)!=0) {
							match = 0;
							break;
						}
					}
					if(match)
						break;
				}
				if(!match) {
					if (output() == ERR) {
						error(E_DATFWRITE, prog, tblname);
						return(exitcode);
					}
					difference++;
				}
			}
		}
	}
	/* no index exists for aname2 */
	for(;;) {
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
		copyfirst();
		rewind(utfp1);
		match = 0;	/* in case no record read */
		recordnr1 = end_of_tbl = 0;
		for(;;) {
			recordnr1++;
			newrec();
			for(i=0;i<nattr1 && getrec(utfp1,&yy[i])!=ERR; i++);
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
				if (strcmp(xx[attr1[i]].val,
					yy[attr2[i]].val)!=0) {
					match = 0;
					break;
				}
			}
			if(match)
				break;
		}
		if(!match) {
			if (output() == ERR) {
				error(E_DATFWRITE, prog, tblname);
				return(exitcode);
			}
			difference++;
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
		error(E_GENERAL,"%s: %d records difference from %s and %s.\n",
			prog,difference,argv[4],argv[5]);
	exitcode = 0;
	table2 = NULL;
	Dtable2[0] = '\0';
	return(exitcode);
}

static int
reset()
{
	/* reset buffer pointer to beginning of buffer */
	ibufptr=ibuf;
}

static int
copyfirst()
{
	int	j;

	/* copy fields for first file */
	for(j=0;j<nattr0;j++)
		copyfld(&xx[j],&xx[j]);
}

static int
output()
{
	int j;

	for(j=0;j<nattr0;j++)
		if (putnrec(utfp2,&xx[j]) < 0)
			return(ERR);
	if (endf0 == 1)
		if (putc('\n',utfp2) == EOF)
			return(ERR);
	return(0);
}

static int
copyfld(a,b)
struct fmt *a,*b;
{
	char	*xa=a->val;
	char	*xb;
	int	i;

	xb=b->val=ibufptr;
	switch(a->flag) {
	case WN:
		for(i=0;i<a->flen;i++) *xb++ = *xa++;
		*xb++=0;
		break;
	case T:
		while(*xb++ = *xa++);
		break;
	}
	ibufptr=xb;
}
