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
#include <sys/types.h>
#include <sys/stat.h>

extern	char	*mktemp(), *strcpy(), *strrchr(), *dirname();
extern  FILE    *getdescrwtbl(), *putdescrwtbl();
extern	FILE	*packedopen();
extern	int	packedclose();

extern	char	*packedsuffix;
extern	char	tmptable[], lockfile[];
extern	char	*table2, Dtable2[];
extern  char    DtmpItable[];
extern	FILE	*utfp1, *utfp2;
extern	int	end_of_tbl;
extern	int	term_escapes;
static	int	exitcode;

alter(argc,argv)
int argc;
char *argv[];
{	
	int	afld1, altered, i, j, endf1, nattr1, recordnr, qoption, update;
	int	inptr, intoptr, onto, coption, fupdcnt, packed;
	char	Dtable1[MAXPATH+4], dir[MAXPATH+4];
	char	*newvalue[MAXATT];
	char	fixedwbuf[MAXREC+1];
	char	RecordNumber[32];	/* buffer for "rec#" */
	char	*table1, **p, *prog, *Ioption, *Ooption;
	char	dtable2[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	*tblname;
	off_t	size_newtable;
	struct	stat statbuf;
	struct	fmt xx[MAXATT+1];	/* add one for "rec#" */
	struct	{
		short	target;
		short	source;
	} fupdate[MAXATT], *fupdptr;
	RETSIGTYPE (*istat)(), (*qstat)(), (*hstat)(), (*tstat)();

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	inptr = onto = intoptr = qoption = coption = altered = endf1 = packed = 0;
	table2 = Ooption = Ioption = NULL;
	Dtable2[0] = '\0';
	DtmpItable[0] = '\0';
	RecordNumber[0] = RecordNumber[1] = '\0';

	exitcode = 1;
	tmptable[0] = lockfile[0] = '\0';

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
				for (i=1; argv[1][i]; i++)
					switch (argv[1][i]) {
					case 'c':
						coption = 1;
						break;
					case 'q':
						qoption = 1;
						break;
					default:
						error(E_BADFLAG,prog,argv[1]);
						return(exitcode);
					}
				break;
		}
		argc--;
		argv++;
	}

	if (argc >= 6) {
		for (i=4; i+1<argc; i+= 3) {
			/*
			 * We will allow "in" to be an attribute name
			 * and assume that table name will never be
			 * "=" or "f=" but could be "to".
			 */
			if ((strcmp(argv[i],"in") == 0) &&
			    (strcmp(argv[i+1],"=") != 0) &&
			    (strcmp(argv[i+1],"f=") != 0)) {
				inptr = i;
				break;
			} else {
				if ((strcmp(argv[i+1],"=") != 0) &&
				    (strcmp(argv[i+1],"f=") != 0) &&
				    (strcmp(argv[i+1],"to") != 0)) {
					break;
				}
			}
		}
	}
	if ((argc < 6) || (inptr == 0) || (argc <= inptr+1) ||
	    (strcmp(argv[2],"to") && strcmp(argv[2],"=") && strcmp(argv[2], "f="))) {
		error(E_GENERAL, "Usage: %s %s \\\n\t%s\n", prog,
			"[-c] [-q] [-Itable] [-Otable] aname1 = value [anameN f= aname]",
			"in table1 [into table2] [where clause]");
		return(exitcode);
	}
	if(argc > inptr+3) {
		if(strcmp(argv[inptr+2],"onto") == 0)
			onto = 1;
		if(onto || strcmp(argv[inptr+2],"into") == 0)
			intoptr = inptr+2;
	}

	table1 = argv[inptr+1];

	if(strcmp(table1,"-") == 0) {
		update = 0;
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
	else if(intoptr) {
		update = 0;
		/* check read perm of table */
		if((utfp1 = fopen(table1,"r")) == NULL) {
			if ((strcmp(table1,argv[intoptr+1]) == 0) ||
			    ((utfp1 = packedopen(table1)) == NULL)) {
				error(E_DATFOPEN,prog,table1);
				return(exitcode);
			}
			++packed;	/* reading packed table */
		}
	}
	else {
		update = 1;
		strcpy(dir,table1);
		dirname(dir);		/* get directory name of table */
		if(chkaccess(dir,2)!=0) {	/* check write permission of dir */
			error(E_DIRWRITE,prog,dir);
			return(exitcode);
		}
		/* check read perm of table */
		if((utfp1 = fopen(table1,"r")) == NULL) {
			error(E_DATFOPEN,prog,table1);
			return(exitcode);
		}
	}

	if ((Ooption) && (!(intoptr))) {
		error(E_GENERAL,
			"%s: Missing \"into table\" clause for the \"-O%s\" option.\n",
			prog, Ooption);
		return(exitcode);
	}

	if((nattr1 = mkntbl(prog,table1,Dtable1,xx,Ioption)) == ERR) {
		if (packed) {
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}
	if(xx[nattr1 - 1].flag == WN)
		endf1 = 1;

	/* add "rec#" as last field equal to the attribute count */
	strcpy(xx[nattr1].aname,"rec#");
	xx[nattr1].flag = WN;
	xx[nattr1].flen = 0;
	xx[nattr1].val = RecordNumber;

	/* parse where clause - build evalutation tree */
	if (intoptr)
		p = &argv[intoptr+2];
	else
		p = &argv[inptr+2];
	getcond(p,xx,nattr1,prog,Dtable1);

	for (fupdptr = fupdate, i = 0; i < nattr1; i++, fupdptr++) {
		newvalue[i] = NULL;
		fupdptr->target = 0;
		fupdptr->source = 0;
	}

	for (fupdptr = fupdate, fupdcnt = 0, j = 0, i = 1; i+2 < inptr; i += 3) {

		if ((afld1 = setnum(xx,argv[i],nattr1)) == ERR) { 
			error(E_ILLATTR,prog,argv[i],Dtable1);
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}
		if (newvalue[afld1] != NULL) {
			error(E_GENERAL,
				"Duplicate alter attempt on attribute %s\n",
				prog, argv[i]);
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}
		/* check if field assignment */
		if (argv[i+1][0] == 'f')
		{
			int	afld2;

			/* add 1 to nattr1 for "rec#" to be checked */
			if ((afld2 = setnum(xx,argv[i+2],nattr1+1)) == ERR) {
				error(E_ILLATTR,prog,argv[i+2],Dtable1);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			if ((xx[afld1].flag == WN) &&
			   ((xx[afld2].flag != WN) ||
			    (xx[afld1].flen != xx[afld2].flen) ||
			    (afld2 == nattr1))) {
				error(E_GENERAL,
					"Cannot do field assignment to fixed width attribute %s\n",
					prog, argv[i]);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			if (afld2 != nattr1) {
				fupdptr->target = afld1;
				fupdptr->source = afld2;
				fupdptr++;
				fupdcnt++;

				/*
				 * newvale for this attribute has to be dynamically set
				 * but for now we will set it to the source attribute name
				 * so we can check for duplicate alter request.
				 */
				newvalue[afld1] = argv[i+2];
			} else {
				newvalue[afld1] = RecordNumber;
				/*
				 * Change RecordNumber from "" to "0"
				 * to indicate that it is being used.
				 */
				RecordNumber[0] = '0';
			}
		}
		else if (xx[afld1].flag == WN)
		{
			int	len;

			if ((len = strlen(argv[i+2])) != xx[afld1].flen) {
				if (len > xx[afld1].flen) {
					error(E_GENERAL,
						"New value to big for fixed width attribute %s\n",
						prog, argv[i]);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
				if (xx[afld1].flen + j >= MAXREC) {
					error(E_GENERAL,
						"Size of fixed width attributes exceed record limit (%d) for table %s\n",
						prog, MAXREC, table1);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
				newvalue[afld1] = &fixedwbuf[j];
				strcpy(fixedwbuf, argv[i+2]);
				j += len;
				len = xx[afld1].flen - len;
				while (len) {
					fixedwbuf[j++] = ' ';	/* pad with blanks */
					--len;
				}
			} else {
				newvalue[afld1] = argv[i+2];
			}
		}  else {
			newvalue[afld1] = argv[i+2];
		}
	}

	if(update) {
		/* create the lock file */
		istat = signal(SIGINT,SIG_IGN);
		qstat = signal(SIGQUIT,SIG_IGN);
		hstat = signal(SIGHUP,SIG_IGN);
		tstat = signal(SIGTERM,SIG_IGN);

		if(mklock(table1,prog,lockfile) != 0) {
			/* re-set signals */
			(void) signal(SIGINT,istat);
			(void) signal(SIGQUIT,qstat);
			(void) signal(SIGHUP,hstat);
			(void) signal(SIGTERM,tstat);
			return(exitcode);
		}

		(void) signal(SIGINT,istat);
		(void) signal(SIGQUIT,qstat);
		(void) signal(SIGHUP,hstat);
		(void) signal(SIGTERM,tstat);

		/* create temporary file */
		sprintf(tmptable,"%s/atmpXXXXXX",dir);
		mktemp(tmptable);
		if((utfp2 = fopen(tmptable,"w")) == NULL) {
			error(E_TEMFOPEN,prog,tmptable);
			return(exitcode);
		}
		tblname = tmptable;	/* current output table name */

	} else if (intoptr == 0) {
		tblname = "-";		/* current output table name */
		utfp2 = stdout;

	} else {	/* into clause - handle output description file */
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
		tblname = argv[intoptr+1];	/* current output table name */
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

	size_newtable = term_escapes = 0;
	recordnr = end_of_tbl = 0;
	for(;;) {
		int j, size_newrec;

		/* get next record */
		newrec();
		for(i=0; i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
		if (( i == 0 ) && ( end_of_tbl ))
			break;
		else if ( i < nattr1 )
		{
			error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
				prog, i, recordnr + 1, table1 );

			if (update)
				error( E_GENERAL, "%s: Warning: no changes made\n", prog );

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
					prog, recordnr + 1, table1 );
				if (update)
					error( E_GENERAL,
						"%s: Warning: no changes made\n", prog );
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
				if (update)
					error( E_GENERAL,
						"%s: Warning: no changes made\n", prog );
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}

		recordnr++;
		if (RecordNumber[0])
			sprintf(RecordNumber, "%u", recordnr);
		size_newrec = 0;
		if(!selct(xx,recordnr)) { /* not selected - write unchaged */
			for(i=0; i<nattr1; i++) {
				if ((j = putnrec(utfp2,&xx[i])) < 0) {
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
		}
		else
		{	/* record match found */

			if (fupdcnt) {
				for (fupdptr = fupdate, i = fupdcnt; i != 0; i--, fupdptr++) {
					newvalue[fupdptr->target] = xx[fupdptr->source].val;
				}
			}

			for (i = 0; i < nattr1; i++)  {
				if (newvalue[i] != NULL)
				{	/* write new attribute value */
					char *oldval;

					oldval = xx[i].val;
					xx[i].val = newvalue[i];
					j = putnrec(utfp2,&xx[i]);
					xx[i].val = oldval;
				} else {
					/* write original attribute value */
					j = putnrec(utfp2,&xx[i]);
				}
				if (j < 0) {
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
			altered++;
		}

		if (endf1 == 1)
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

		if ( size_newrec > MAXREC )
		{
			error( E_GENERAL, "%s: Error: new rec# %d would be longer (%d) than maximum record length (%d)\n",
				prog, recordnr, size_newrec, MAXREC );
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return( exitcode );
		}
		size_newtable += size_newrec;
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

	if(update) {

		/* verify size of new table */
		if (stat(tmptable, &statbuf) != 0) {
			error( E_GENERAL,
				"%s: Error: stat(2) failure getting size of new table %s\n",
				prog, tmptable);
			return(exitcode);
		}
		if (term_escapes > 0) {
			size_newtable += term_escapes;
		}
		if (statbuf.st_size != size_newtable) {
			error( E_GENERAL,
				"%s: Error: size of new table (%u) is not the expected size (%u)\n",
				prog, statbuf.st_size, size_newtable);
			return(exitcode);
		}

		/* move the new table to the old */
		if (stat(table1,&statbuf) != 0) {
			error( E_GENERAL,
				"%s: Error: stat(2) failure getting permissions and owner of old table %s\n",
				prog, table1);
			return(exitcode);
		}
		chmod(tmptable,(int)statbuf.st_mode);
		chown(tmptable,(int)statbuf.st_uid,(int)statbuf.st_gid);

#ifdef	ADVLOCK
		/*
		 * Check that the lock file still exists
		 * and that it has not been tampered with.
		 */
		if ((lockfile[0] != '\0') && (cklock(lockfile) != 0))
		{
			error(E_LOCKSTMP,prog,table1,tmptable);
			tmptable[0] = '\0';	/* don't remove temporary table */
			return(exitcode);
		}
#endif
		/* ignore signals */
		istat = signal(SIGINT,SIG_IGN);
		qstat = signal(SIGQUIT,SIG_IGN);
		hstat = signal(SIGHUP,SIG_IGN);
		tstat = signal(SIGTERM,SIG_IGN);

#ifdef	__STDC__
		if (rename(tmptable, table1) != 0) {
			error(E_BADLINK,prog,tmptable);
			(void) signal(SIGINT,istat);
			(void) signal(SIGQUIT,qstat);
			(void) signal(SIGHUP,hstat);
			(void) signal(SIGTERM,tstat);
			return(exitcode);
		}
		tmptable[0] = '\0';	/* new table no longer exists */
#else	
		if(unlink(table1) < 0) {
			error(E_BADUNLINK,prog,table1,tmptable);
			(void) signal(SIGINT,istat);
			(void) signal(SIGQUIT,qstat);
			(void) signal(SIGHUP,hstat);
			(void) signal(SIGTERM,tstat);
			return(exitcode);
		}
		if(link(tmptable,table1)<0) {
			error(E_BADLINK,prog,tmptable);
			tmptable[0] = '\0';/* don't remove temporary table */
			(void) signal(SIGINT,istat);
			(void) signal(SIGQUIT,qstat);
			(void) signal(SIGHUP,hstat);
			(void) signal(SIGTERM,tstat);
			return(exitcode);
		}
#endif
	}

	if(!qoption)
		error(E_GENERAL,"%s: %d records altered in %s.\n",prog,altered,
		table1);
	exitcode = 0;
	if(update) {
		(void) signal(SIGINT,istat);
		(void) signal(SIGQUIT,qstat);
		(void) signal(SIGHUP,hstat);
		(void) signal(SIGTERM,tstat);
	}
	table2 = NULL;
	Dtable2[0] = '\0';
	return(exitcode);
}
