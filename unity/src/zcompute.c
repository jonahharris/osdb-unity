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
#include <sys/types.h>
#include <sys/stat.h>

extern  char    *mktemp(), *strcpy(), *strchr(), *strrchr(), *dirname();
extern	FILE	*getdescrwtbl(), *putdescrwtbl();
extern	FILE	*packedopen();
extern	int	packedclose();

extern	char	*packedsuffix;
extern  char    tmptable[], lockfile[];
extern	char	*table2, Dtable2[];
extern	FILE	*utfp1, *utfp2;
extern	char	DtmpItable[];
extern	int	term_escapes;
extern	int	end_of_tbl;

static	int	exitcode;

compute(argc,argv)
int argc;
char *argv[];
{       
	int	afld, computed, i, endf1, nattr1, recordnr, qoption, update;
	int	inptr, intoptr, onto, coption, packed;
	int	default_conversion, conversion[MAXATT];
	RETSIGTYPE	(*istat)(), (*qstat)(), (*hstat)(), (*tstat)();
	char    Dtable1[MAXPATH+4],newvalue[MAXREC],dir[MAXPATH+4],**p,*prog;
	char	*default_format, *format[MAXATT];
	char	*Ioption, *Ooption, *tblname;
	char	dtable2[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	RecordNumber[32];	/* buffer for "rec#" */
	off_t	size_newtable;
	struct  stat statbuf;
	struct  fmt xx[MAXATT+1];	/* add one for "rec#" */
#ifdef INTCOMP
	int	ret,nueval();
#else
	double	ret,nueval();
#endif

	prog = strrchr(argv[0], '/');		/* program name */
	if (prog == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	inptr = intoptr = onto = coption = qoption = computed = endf1 = packed = 0;

	tmptable[0] = lockfile[0] = '\0';
	exitcode = 1;
	Ooption = Ioption = NULL;
	table2 = NULL;
	Dtable2[0] = '\0';
	RecordNumber[0] = '\0';

#ifdef INTCOMP
	default_format = "%d";		/* default output format of computed field */
	default_conversion = 'd';
#else
	default_format = "%.3f";	/* default output format of computed field */
	default_conversion = 'f';
#endif

	while( argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		/* get options */
		switch(argv[1][1]) {
		default:
			error(E_BADFLAG,prog,argv[1]);
			return(exitcode);
		case 'c':
		case 'q':
			for (i=1; argv[1][i]; i++) {

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
			}
			break;
		case 'I':
			Ioption = &argv[1][2];
			break;
		case 'O':
			Ooption = &argv[1][2];
			break;
		case 'f':	/* get output format for computed field */
			if (argv[1][2]) {
				default_format = &argv[1][2];
				if ((default_conversion = fmtchk(default_format)) == -1) {
					error(E_GENERAL,"%s: Bad output format\n",prog);
					error(E_GENERAL,
#ifdef INTCOMP
			"Must be of form %[+ -#]*[0-9]*\\.{0,1}[0-9]*[bdiouxX]\n");
#else
			"Must be of form %[+ -#]*[0-9]*\\.{0,1}[0-9]*[bdiouxXeEfgG]\n");
#endif
					return(exitcode);
				}
			}
			break;
		}
		argc--;
		argv++;
	}

	for (i=1;i<argc;i++)
		if (strcmp(argv[i],"in") == 0) {
			inptr = i;
			if ( i + 1 >= argc )
			{
				usage( prog );
				return( exitcode );
			}
			break;
		}

	if (( inptr == 0 ) || ( inptr < 4 ))
	{
		usage( prog );
		return(exitcode);
	}

	if (( inptr > 0 ) && ( inptr + 2 < argc ))
	{
		if ( strcmp(argv[inptr+2],"onto") == 0)
		{
			onto = 1;
			intoptr = inptr + 2;
		}
		else if ( strcmp(argv[inptr+2],"into") == 0 )
		{
			onto = 0;
			intoptr = inptr + 2;
		}
		if (( intoptr > 0 ) && ( intoptr + 1 >= argc ))
		{
			usage( prog );
			return( exitcode );
		}
	}

	if ((Ooption) && (!(intoptr))) {
		error(E_GENERAL,
			"%s: Missing \"into table\" clause for the \"-O%s\" option.\n",
			prog, Ooption);
		return(exitcode);
	}

	if (strcmp(argv[inptr+1],"-") == 0) {
		update = 0;
		if (!(Ioption)) {
			/*
			 * Check for/get description from the input file.
			 */
			if (getdescrwtbl(stdin, &Ioption) == NULL) {

				error(E_GENERAL, "%s: No description file specified.\n",
				prog);
				return(exitcode);
			}
		}
		utfp1 = stdin;
	}
	else if (intoptr) {
		update = 0;
		/* check read perm of table */
		if ((utfp1 = fopen(argv[inptr+1],"r")) == NULL) {
			if ((strcmp(argv[inptr+1],argv[intoptr+1]) == 0) ||
			   ((utfp1 = packedopen(argv[inptr+1])) == NULL)) {
				error(E_DATFOPEN,prog,argv[inptr+1]);
				return(exitcode);
			}
			++packed;	/* reading packed table */
		}
	}
	else {
		update = 1;
		strcpy(dir,argv[inptr+1]);
		dirname(dir);           /* get directory name of table */
		if (chkaccess(dir,2)!=0) {  /* check write permission of dir */
			error(E_DIRWRITE,prog,dir);
			return(exitcode);
		}
		/* check read perm of table */
		if ((utfp1 = fopen(argv[inptr+1],"r")) == NULL) {
			error(E_DATFOPEN,prog,argv[inptr+1]);
			return(exitcode);
		}
	}

	if ((nattr1 = mkntbl(prog,argv[inptr+1],Dtable1,xx,Ioption)) == ERR) {
		if (packed) {
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}
	if (xx[nattr1 - 1].flag == WN)
		endf1 = 1;

	/* add "rec#" as last field equal to the attribute count */
	strcpy(xx[nattr1].aname,"rec#");
	xx[nattr1].flag = WN;
	xx[nattr1].flen = 0;
	xx[nattr1].val = RecordNumber;

	/* default all fields to being copied */
	for (i=0; i<nattr1; i++) {
		conversion[i] = 0;
	}

	i = 1;

	do {
		int	j, k;
		char	*ptr;

		/*
		 * save start of expression for call to nuexpr()
		 */
		j = i;

		ptr = NULL;

		while ( i < inptr ) {
			if (strncmp(argv[i],"result", 6) == 0) {
				/*
				 * check for %format appended to result
				 * with or without a leading ':' since
				 * uprintf(UNITY) requires the ':'
				 */
				ptr = &argv[i][6];
				if ((*ptr == '\0') ||
				    (*ptr == ':') ||
				    (*ptr == '%'))
					break;
			}
			++i;
		}

		/*
		 * Ignore a leading ':' at this point
		 * since uprintf(UNITY) uses/requires
		 * "format" (if present) to be directly
		 * appended (i.e., "result:format")
		 * but treat a leading ':' on argv[i+1]
		 * as an error since it is not part of
		 * the documented usage.
		 */
		if ((ptr != NULL) && (*ptr == ':'))
			++ptr;

		if ((ptr != NULL) && (*ptr == '\0') &&
		    (i < inptr - 1) && (argv[i+1][0] == '%')) {
			ptr = argv[i+1];
			k = i + 2;
		} else {		/* k == index of result attribute */
			k = i + 1;
		}

		if (( i == j ) || ( k >= inptr ))
		{
			usage( prog );
			efreen(MAXATT);
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return( exitcode );
		}

		if ((afld = setnum(xx,argv[k],nattr1)) == ERR) { 
			error(E_ILLATTR,prog,argv[k],Dtable1);
			efreen(MAXATT);
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}
		if (conversion[afld]) {
			error(E_GENERAL,
				"%s: Duplicate result attribute [%s]\n",
				prog, xx[afld].aname);
			efreen(MAXATT);
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}
		if (xx[afld].flag == WN) {  
			error(E_GENERAL,
				"%s: Result attribute [%s] cannot be a fixed width field.\n",
				prog, xx[afld].aname);
			efreen(MAXATT);
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}

		/*
		 * Initialize start of expression and
		 * mark end of expression via NULL
		 * and increment argv[] index for
		 * next result loop iteration.
		 */
		p = &argv[j];
		argv[i] = NULL;
		i = k + 1;

		if ((ptr == NULL) || (*ptr == '\0')) {
			conversion[afld] = default_conversion;
			format[afld] = default_format;
		} else {
			if ((conversion[afld] = fmtchk(ptr)) == -1) {
				error(E_GENERAL,
					"%s: Bad output format for result attribute [%s]\n",
					prog, xx[afld].aname);
				error(E_GENERAL,
#ifdef INTCOMP
			"Must be of form %[+ -#]*[0-9]*\\.{0,1}[0-9]*[bdiouxX]\n");
#else
			"Must be of form %[+ -#]*[0-9]*\\.{0,1}[0-9]*[bdiouxXeEfgG]\n");
#endif
				efreen(MAXATT);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			format[afld] = ptr;
		}

		/* Parse expression and build evaluation tree */
		nuexpr(p,xx,nattr1+1,prog,Dtable1,afld);

	} while ( i < inptr );

	/* parse where clause - build evaluation tree */
	if (intoptr)
		p = &argv[intoptr+2];
	else
		p = &argv[inptr+2];
	getcond(p,xx,nattr1,prog,Dtable1);

	if (update) {
		istat = signal(SIGINT,SIG_IGN);
		qstat = signal(SIGQUIT,SIG_IGN);
		hstat = signal(SIGHUP,SIG_IGN);
		tstat = signal(SIGTERM,SIG_IGN);

		/* create lock file */
		if (mklock(argv[inptr+1],prog,lockfile) != 0) {
			(void) signal(SIGINT,istat);
			(void) signal(SIGQUIT,qstat);
			(void) signal(SIGHUP,hstat);
			(void) signal(SIGTERM,tstat);
			efreen(MAXATT);
			return(exitcode);
		}

		(void) signal(SIGINT,istat);
		(void) signal(SIGQUIT,qstat);
		(void) signal(SIGHUP,hstat);
		(void) signal(SIGTERM,tstat);

		/* create temporary file */
		sprintf(tmptable,"%s/ctmpXXXXXX",dir);
		mktemp(tmptable);
		if ((utfp2 = fopen(tmptable,"w")) == NULL)  {
			error(E_TEMFOPEN,prog,tmptable);
			efreen(MAXATT);
			return(exitcode);
		}
		tblname = tmptable;	/* current output table name */

	} else if (intoptr == 0) {
		tblname = "-";		/* current output table name */
		utfp2 = stdout;

	} else {	/* into clause - handle output description file */
		if (strcmp(argv[intoptr+1],"-") != 0) {
			if (chkaccess(argv[intoptr+1],00) == 0) {
				if (!onto) {
					error(E_EXISTS,prog,argv[intoptr+1]);
					efreen(MAXATT);
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
			if (onto)
				utfp2 = fopen(argv[intoptr+1],"a");
			else
				utfp2 = fopen(argv[intoptr+1],"w");
			if (utfp2 == NULL) { 
				error(E_DATFOPEN,prog,argv[intoptr+1]);
				efreen(MAXATT);
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
				/* make sure dtable2 is null ("") */
				dtable2[0] = '\0';
			}
			utfp2 = stdout;
		}
		if ((dtable2[0]) &&
		    (strcmpfile(Dtable1,dtable2) != 0)) {
			if (chkaccess(dtable2,00) == 0) {
				if (!onto) {
					error(E_EXISTS,prog,dtable2);
					efreen(MAXATT);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
			} else {
				strcpy(Dtable2,dtable2);
				if (copy(prog,Dtable1,Dtable2)!=0) {
					efreen(MAXATT);
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
			efreen(MAXATT);
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
	for (;;) {
		int j, size_newrec;

		/* get next record */
		newrec();
		for (i=0; i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
		if (( i == 0 ) && ( end_of_tbl ))
			break;
		else if ( i < nattr1 )
		{
			error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
				prog, i, recordnr + 1, argv[inptr+1] );

			error( E_GENERAL, "%s: Warning: no changes made\n",
				prog );
			efreen(MAXATT);
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
					prog, recordnr + 1, argv[inptr+1] );
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
					prog, recordnr + 1, argv[inptr+1] );
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
		sprintf(RecordNumber, "%u", recordnr);
		size_newrec = 0;

		if (selct(xx,recordnr)) {
			/*
			 * loop through all fields and compute new value as appropriate
			 */
			for (i=0; i<nattr1; i++) {
				if (conversion[i])
				{
					char *oldval;

					ret = nueval(xx, i);	/* evaluate expression */

					switch (conversion[i]) {
					case 'b':
						/* binary (unsigned) format */
						sprintfb(newvalue,format[i],(unsigned)ret);	/* format result */
						break;
					case 'd':
					case 'i':
						/* interger format */
						sprintf(newvalue,format[i],(int)ret);	/* format result */
						break;
					case 'o':
					case 'u':
					case 'x':
					case 'X':
						/* unsigned interger format */
						sprintf(newvalue,format[i],(unsigned)ret);	/* format result */
						break;
					default:
						/* floating point format */
						sprintf(newvalue,format[i],ret);	/* format result */
						break;
					}

					oldval = xx[i].val;
					xx[i].val = newvalue;
					if ((j = putnrec( utfp2, &xx[i] )) < 0) {
						error(E_DATFWRITE, prog, tblname);
						efreen(MAXATT);
						if (packed) {
							/* EOF == FALSE */
							(void)packedclose(utfp1,FALSE);
							utfp1 = NULL;
						}
						return(exitcode);
					}
					size_newrec += j;
					xx[i].val = oldval;

				} else {
					/* copy field without any changes */
					if ((j = putnrec( utfp2, &xx[i] )) < 0) {
						error(E_DATFWRITE, prog, tblname);
						efreen(MAXATT);
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
			computed++;
		} else {
			/* write all fields without any changes */
			for (i=0; i<nattr1; i++) {
				if ((j = putnrec( utfp2, &xx[i] )) < 0) {
					error(E_DATFWRITE, prog, tblname);
					efreen(MAXATT);
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

		if (endf1 == 1)
		{
			if (putc('\n',utfp2) == EOF) {
				error(E_DATFWRITE, prog, tblname);
				efreen(MAXATT);
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
			efreen(MAXATT);
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return( exitcode );
		}

		size_newtable += size_newrec;
	}

	efreen(MAXATT);

	if (utfp1 != stdin) {
		if (packed) {
			/* EOF == TRUE */
			if (packedclose(utfp1,TRUE) != 0) {
				error(E_PACKREAD,prog,argv[inptr+1],packedsuffix);
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

	if (update) {

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

		if (stat(argv[inptr+1], &statbuf) != 0) {
			error( E_GENERAL,
				"%s: Error: stat(2) failure getting permissions and owner of old table %s\n",
				prog, argv[inptr+1]);
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
			error(E_LOCKSTMP,prog,argv[inptr+1],tmptable);
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
		if (rename(tmptable, argv[inptr+1]) != 0) {
			error(E_BADLINK,prog,tmptable);
			(void) signal(SIGINT,istat);
			(void) signal(SIGQUIT,qstat);
			(void) signal(SIGHUP,hstat);
			(void) signal(SIGTERM,tstat);
			return(exitcode);
		}
		tmptable[0] = '\0';	/* new table no longer exists */
#else
		if (unlink(argv[inptr+1]) < 0) {
			error(E_BADUNLINK,prog,argv[inptr+1],tmptable);
			(void) signal(SIGINT,istat);
			(void) signal(SIGQUIT,qstat);
			(void) signal(SIGHUP,hstat);
			(void) signal(SIGTERM,tstat);
			return(exitcode);
		}
		if (link(tmptable,argv[inptr+1]) < 0) {
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

	if (!qoption)
		error(E_GENERAL,"%s: %d records computed in %s.\n",
			prog,computed,argv[inptr+1]);
	exitcode = 0;

	if (update) {
		(void) signal(SIGINT,istat);
		(void) signal(SIGQUIT,qstat);
		(void) signal(SIGHUP,hstat);
		(void) signal(SIGTERM,tstat);
	}
	table2 = NULL;
	Dtable2[0] = '\0';
	return(exitcode);
}

static
usage( prog )
char *prog;
{
	error(E_GENERAL, "Usage: %s %s \\\n\t%s \\\n\t%s\n", prog,
		"[-c] [-q] [-Itable] [-Otable] [-f%format] aname [op aname ...]",
		"result [%format] aname1 [aname ... result [%format] anameN]",
		"in table1 [into table2] [where-clause]");
}

static int
fmtchk(format)
char	*format;
{
	register int		i;
	register char		*p;
	int			period;
	int			minus;
	int			zero, leadzero;
	int			precision;
	int			width;
	int			fmterr;
	char			conversion;

	/* output format for computed field
#ifdef INTCOMP
	   must be of form %[+ -#]*[0-9]*\.{0,1}[0-9]*[bdiouxX]
#else
	   must be of form %[+ -#]*[0-9]*\.{0,1}[0-9]*[bdiouxXeEfgG]
#endif
	*/

	conversion = 0;

	precision = 1;	/* default precision */

	period = minus = zero = leadzero = width = 0;

	fmterr = i = 0;

	if ((format == NULL) || (*format != '%')) {
		p = "";
		++fmterr;
	} else {
		p = format + 1;

		/*
		 * Skip over initial flags that do not change
		 * the basic width/precision check/calculation.
		 */
		if ((*p == '#') ||
		    (*p == '+') ||
		    (*p == ' ')) {
			++p;
		}
	}

	while (*p) {
		i = *p++;
		switch (i) {
		case '-':
			if (minus) {
				++fmterr;
			}
			++minus;
			zero = 0;	/* use ' ' to pad width */
			break;
		case '.':
			if (period) {
				++fmterr;
			}
			++period;
			precision = 0;
			zero = 0;	/* use ' ' to pad width */
			break;
		case 'b':
		case 'd':
		case 'i':
		case 'o':
		case 'u':
		case 'x':
		case 'X':
#ifndef	INTCOMP
		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G':
#endif
			if (*p != 0) {
				++fmterr;
			}
			if (fmterr == 0) {
				conversion = i;
			}
			break;
		case '0':
			if ((!(period)) && (!(minus))) {
				++zero;
				++leadzero;
			}
			/* FALLTHROUGH */
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			{
				register	num;

				num = i - '0';

				while (isdigit(i = *p)) {
					num = num * 10 + i - '0';
					++p;
				}
				if (num < MAXREC) {
					if (period) {
						precision = num;
					} else {
						width = num;
					}
				} else {
					++fmterr;
				}
			}
			break;
		default:
			/* invalid format */
			++fmterr;
		}
	}

	/* were there any format errors? */
	if ((fmterr) || (!(conversion))) {
		return(-1);
	}

	return(conversion);
}
