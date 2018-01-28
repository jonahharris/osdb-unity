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

static	int	nattr1;
static	struct	fmt xx[MAXATT];
extern	char	*mktemp(), *strcpy(), *strrchr(), *dirname();
extern	FILE	*getdescrwtbl(), *putdescrwtbl();
extern	FILE	*packedopen();
extern	int	packedclose();

extern	char	*packedsuffix;
extern  char    tmptable[], lockfile[];
extern	char	*table2, Dtable2[];
extern	char	DtmpItable[];
extern	FILE	*utfp1, *utfp2;
extern	int	term_escapes;
extern	int	end_of_tbl;
static	int	exitcode;

delete(argc, argv)
int	argc;
char	*argv[];
{
	RETSIGTYPE	(*istat)(), (*qstat)(), (*hstat)(), (*tstat)();
	int	i, deleted,endf1,recordnr,qoption,update,intoptr,onto,coption,packed;
	char	Dtable1[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	dir[MAXPATH+4];
	char	**p,*prog, *Ioption, *Ooption, *tblname;
	char	dtable2[MAXPATH+4];
	struct 	stat statbuf;
	off_t	size_newtable;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	intoptr = onto = coption = packed = qoption = 0;

	tmptable[0] = lockfile[0] = '\0';
	Ooption = Ioption = NULL;
	table2 = NULL;
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
				for (i=1; argv[1][i]; i++)
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
						error(E_BADFLAG, prog, &argv[1][i]);
						return(exitcode);
					}
				break;
		}
		argc--;
		argv++;
	}
	intoptr = 1;
	if(argc > 5) {
		if(strcmp(argv[3],"onto") == 0)
			onto = 1;
		if(onto || strcmp(argv[3],"into") == 0)
			intoptr = 3;
	}
	if(argc < 7 || strcmp(argv[1],"from") ||
		strcmp(argv[intoptr+2],"where")) {
		error(E_GENERAL,
"Usage: %s [-c] [-q] [-Itable] [-Otable] from table1 [into table2] \\\n\twhere clause\n",
			prog);
		return(exitcode);
	}
	if(intoptr == 1)
		intoptr = 0;

	if(strcmp(argv[2],"-") == 0) {
		update = 0;
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
	else if(intoptr) {
		update = 0;
		if ((utfp1 = fopen(argv[2],"r")) == NULL) {
			if ((strcmp(argv[2],argv[4]) == 0) ||
			    ((utfp1 = packedopen(argv[2])) == NULL)) {
				error(E_DATFOPEN,prog,argv[2]);
				return(exitcode);
			}
			++packed;	/* reading packed table */
		}
	}
	else {
		update = 1;
		strcpy(dir,argv[2]);
		dirname(dir);		/* get directory name of table */
		if(chkaccess(dir,2)!=0) {	/* check write permission of dir */
			error(E_DIRWRITE,prog,dir);
			return(exitcode);
		}
		if ((utfp1 = fopen(argv[2],"r")) == NULL) {
			error(E_DATFOPEN,prog,argv[2]);
			return(exitcode);
		}
	}

	if ((Ooption) && (!(intoptr))) {
		error(E_GENERAL,
			"%s: Missing \"into table\" clause for the \"-O%s\" option.\n",
			prog, Ooption);
		return(exitcode);
	}

	if((nattr1 = mkntbl(prog,argv[2],Dtable1,xx,Ioption))==ERR) {
		if (packed) {
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}
	if (xx[nattr1 - 1].flag == WN)
		endf1 = 1;
	else
		endf1 = 0;

	/* parse where clause and build evaluation tree */
	if(intoptr)
		p = &argv[intoptr+2];
	else
		p = &argv[3];
	getcond(p,xx,nattr1,prog,Dtable1);

	if(update) {
		istat = signal(SIGINT,SIG_IGN);
		qstat = signal(SIGQUIT,SIG_IGN);
		hstat = signal(SIGHUP,SIG_IGN);
		tstat = signal(SIGTERM,SIG_IGN);

		/* create lock file */
		if(mklock(argv[2],prog,lockfile) != 0) {
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
		sprintf(tmptable,"%s/dtmpXXXXXX",dir);
		mktemp(tmptable);
		if((utfp2 = fopen(tmptable,"w")) == NULL) {
			error(E_TEMFOPEN,prog,tmptable);
			return(exitcode);
		}
		tblname = tmptable;	/* current output table name */

	} else if(intoptr == 0) {
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
	recordnr = deleted = end_of_tbl = 0;
	for(;;) {
		int j, size_newrec;

		/* get a new record */
		newrec();
		for(i=0; i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
		if (( i == 0 ) && ( end_of_tbl ))
			break;
		else if ( i < nattr1 )
		{
			error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
				prog, i, recordnr + 1, argv[2] );

			if (update)
				error( E_GENERAL, "%s: Warning: no changes made\n",
					prog );
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
					prog, recordnr + 1, argv[2] );
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
					prog, recordnr + 1, argv[2] );
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
		size_newrec = 0;

		if(!selct(xx,recordnr)) {	/* record not selected */
			for (i=0; i<nattr1; i++) {	/* write record unchanged */
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
			if (endf1) {
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
			size_newtable += size_newrec;
		}
		else /* otherwise record not printed - deleted */
			deleted++;
	}

	if (utfp1 != stdin) {
		if (packed) {
			/* EOF == TRUE */
			if (packedclose(utfp1,TRUE) != 0) {
				error(E_PACKREAD,prog,argv[2],packedsuffix);
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

		if (stat(argv[2], &statbuf) != 0) {
			error( E_GENERAL,
				"%s: Error: stat(2) failure getting permissions and owner of old table %s\n",
				prog, argv[2]);
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
			error(E_LOCKSTMP,prog,argv[2],tmptable);
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
		if (rename(tmptable, argv[2]) != 0) {
			error(E_BADLINK,prog,tmptable);
			(void) signal(SIGINT,istat);
			(void) signal(SIGQUIT,qstat);
			(void) signal(SIGHUP,hstat);
			(void) signal(SIGTERM,tstat);
			return(exitcode);
		}
		tmptable[0] = '\0';	/* new table no longer exists */
#else
		if(unlink(argv[2]) < 0) {
			error(E_BADUNLINK,prog,argv[2],tmptable);
			(void) signal(SIGINT,istat);
			(void) signal(SIGQUIT,qstat);
			(void) signal(SIGHUP,hstat);
			(void) signal(SIGTERM,tstat);
			return(exitcode);
		}
		if(link(tmptable,argv[2])<0) {
			error(E_BADLINK,prog,tmptable);
			tmptable[0] = '\0';		/* don't remove */
			(void) signal(SIGINT,istat);
			(void) signal(SIGQUIT,qstat);
			(void) signal(SIGHUP,hstat);
			(void) signal(SIGTERM,tstat);
			return(exitcode);
		}
#endif
	}
	if(!qoption)
		error(E_GENERAL,"%s: %d records deleted in %s.\n",prog,
			deleted,argv[2]);
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
