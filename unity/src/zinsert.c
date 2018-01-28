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

extern	FILE	*utfp1, *utfp2;
extern	FILE	*getdescrwtbl(), *putdescrwtbl();
extern	char	*dirname(), *mktemp(), *strcpy(), *strrchr();

static	struct	fmt xx[MAXATT];
static	char	buf[MAXREC],newline;
static	int	nattr1,terminal=0;

extern  char    tmptable[], lockfile[];
static	int	exitcode;
extern	char	*table2, Dtable2[],*stdbuf;
extern	char	DtmpItable[];
static	char	*table3;

insert(argc, argv)
int	argc; 
char	*argv[];
{
	char	Dtable1[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	*prog, dir[MAXPATH+4], *Ioption, *Ooption;
	RETSIGTYPE	(*istat)(), (*qstat)(), (*hstat)(), (*tstat)();
	int	i, c, prompt, qoption, update, intoptr, onto;
	int	coption, errorlimit;
	char	dtable2[MAXPATH+4], *tblname, *tmpstdin;
	struct	stat statbuf;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	intoptr = onto = coption = qoption = prompt = 0;
	newline = NLCHAR;
	Ooption = Ioption = NULL;
	table3 = table2 = NULL;
	tmpstdin = NULL;
	Dtable2[0] = '\0';
	DtmpItable[0] = '\0';

	tmptable[0] = lockfile[0] = '\0';
	errorlimit = 1;
	exitcode = 1;
	stdbuf = NULL;

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
			case 'n':
				newline=argv[1][2];
				if ( newline == '\n' )
				{
					error( E_GENERAL, "%s: ERROR: cannot have '\\n' as newline character\n",
						prog );
					return( exitcode );
				}
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
						qoption = 1;
						break;
					default:
						error(E_BADFLAG, prog, &argv[1][i]);
						return(exitcode);
					}
				break;
			case 'r':
				if (argv[1][2]) {
					if (((errorlimit = atoi(&argv[1][2])) <= 0) &&
					     (argv[1][2] != '0') && (argv[1][2] != '-')) {
						error(E_GENERAL,
							"%s: Invalid error report limit %s\n",
							prog, argv[1]);
						return(exitcode);
					}
				} else {
					errorlimit = 0;
				}
				break;
		}
		argc--;
		argv++;
	}
	if (argc < 3 || strcmp(argv[1],"in")) { 
		error(E_GENERAL, "Usage: %s %s \\\n\t%s\n", prog,
			"[-c] [-q] [-rErrorLimit] [-Itable] [-Otable] [-n[newline]]",
			"in table1 [into table2] [ { prompting | from table3 } ]");
		return(exitcode);
	}
	if(argc > 4) {
		if(strcmp(argv[3],"onto") == 0)
			onto = 1;
		if(onto || strcmp(argv[3],"into") == 0)
			intoptr = 3;
	}
	if ((Ooption) && (!(intoptr))) {
		error(E_GENERAL,
			"%s: Missing \"into table\" clause for the \"-O%s\" option.\n",
			prog, Ooption);
		return(exitcode);
	}
	if(strcmp(argv[2],"-") == 0) {
		update = 0;
		if(!Ioption) {
			if ((strcmp(argv[argc-1],"-")) ||
			    (strcmp(argv[argc-2],"from")) ||
			    (getdescrwtbl(stdin,&Ioption) == NULL)) {
				error(E_GENERAL,"%s: No description file specified.\n",
					prog);
				return(exitcode);
			}
		}
	}
	else if(intoptr)
		update = 0;
	else {
		update = 1;
		strcpy(dir,argv[2]);
		dirname(dir);	/* get directory name of table */
		if(chkaccess(dir,2)!=0) {
			error(E_DIRWRITE,prog,dir);
			return(exitcode);
		}

		if(chkaccess(argv[2],0)!=0)
			close(creat(argv[2],0666));	/* create table */
	}

	if((intoptr == 0 && argc == 4 && argv[3][0] == 'p') ||
	   (intoptr && argc == 6 && argv[5][0] == 'p')) {
		/* prompting parameter - set prompting flag and ignore
		   last parameter */
		prompt = 1;
		argc--;
	}
	if(isatty(0) && isatty(1))
		terminal = 1;

	if((intoptr == 0 && argc == 3) || (intoptr && argc == 5)) {
		if(strcmp(argv[2],"-") == 0) {
			error(E_GENERAL,
	"%s: Input table \"-\" and interactive input are not compatible\n",
			prog);
			return(exitcode);
		}

		/* interactive input - set it up so that input file is
		   always argv[argc-1] whether interactive or "from table3" */

		argc++;
		argv[argc-1] = "itmpXXXXXX";
		mktemp(argv[argc-1]);

		table3 = argv[argc-1];

		if ((utfp1 = fopen(argv[argc-1],"w")) == NULL) {/* create */
			error(E_TEMFOPEN,prog,argv[argc-1]);
			return(exitcode);
		}
		if((nattr1=mkntbl(prog,argv[2],Dtable1,xx,Ioption))==ERR)
			return(exitcode);
		if(terminal)
			error(E_GENERAL,
				"Terminate insertion with EOF (ctrl-d)\n");
		if(prompt)
			c = promptrec(prog, table3);
		else
			c = recget(prog, table3);
		if ( c == ERR )
		{
			error( E_GENERAL, "%s: Warning: due to previous errors, no changes were made to '%s'\n",
				prog, argv[2] );
			error( E_GENERAL, "%s: Data to be inserted was left in %s.\n",
				prog, table3);
			return( exitcode );
		}

		fclose(utfp1);
		utfp1 = NULL;
		error(E_GENERAL,"%s: Data now in %s.\n",prog,argv[argc-1]);
		if(terminal) {
			error(E_GENERAL,"Insert records? ");
			if((c=getchar()) == 'n' || c == 'N') {
				exitcode = 0;
				return(exitcode);
			}
		}
	}
	else {
		if((argc != 5 && argc != 7) ||
		   (intoptr == 0 && strcmp(argv[3],"from")) ||
		   (intoptr && strcmp(argv[5],"from")) ) {
			error(E_GENERAL, "Usage: %s %s \\\n\t%s\n", prog,
				"[-c] [-q] [-rErrorLimit] [-Itable] [-Otable] in table1",
				"[into table2] from table3");
			return(exitcode);
		}
		if((nattr1=mkntbl(prog,argv[2],Dtable1,xx,Ioption))==ERR)
			return(exitcode);

		/* if from stdin then read into temporay file */
		if (strcmp(argv[argc-1],"-") == 0) {

			tmpstdin = argv[argc-1];
			argv[argc-1] = "itmpXXXXXX";
			mktemp(argv[argc-1]);
			table3 = argv[argc-1];

			if ((utfp1 = fopen(argv[argc-1],"w")) == NULL) {/* create */
				error(E_TEMFOPEN,prog,table3);
				return(exitcode);
			}
			/* copy stdin to the temp file */
			while ((c=getc(stdin)) != EOF)
				if (putc(c,utfp1) == EOF) {
					error(E_DATFWRITE,prog,table3);
					unlink(table3);
					table3 = NULL;
					return(exitcode);
				}
			if (fclose(utfp1) == EOF) {
				error(E_DATFWRITE,prog,table3);
				unlink(table3);
				table3 = NULL;
				return(exitcode);
			}
			utfp1 = NULL;
		}
	}

	/*
	 * Setup alternate description with the special "/./" or "././" prefix
	 * so that ncheck() will use the same description we obtained via mkntbl()
	 */
	{
		int	errors, records;
		char	*ptr;
	
		ptr = Dtable1;

		/*
		 * Remove any existing prefixes that would/could cause
		 * the resulting path/file name to exceed MAXPATH
		 */
		while ((strncmp(ptr,"/./",3) == 0) || (strncmp(ptr,"./",2) == 0))
			ptr += 2;

		if (*ptr == '/')
			sprintf(dtable2, "/.%s", ptr);
		else
			sprintf(dtable2, "././%s", ptr);

		ptr = strrchr(dtable2, '/');
		++ptr;				/* now pointing to 'D' */
		strcpy(ptr, ptr+1);

		if ((i = ncheck(argv[argc-1],dtable2,errorlimit,&errors,&records)) == ERR)
		{
			if (tmpstdin) {
				unlink(table3);
				table3 = NULL;
				argv[argc-1] = tmpstdin;
			}
			if (records)
				error(E_GENERAL,
					"%s: check record errors (%d) - %d records left in %s\n",
					prog, errors, records, argv[argc-1]);
			else
				error(E_GENERAL,
					"%s: Error - records left in %s\n",
					prog, argv[argc-1]);
			return(exitcode);
		}
	}

	/* at this point, no files are open; the input to be inserted is in
	   file argv[argc-1] which can/will no longer can be "-" */

	if(update) {
		istat = signal(SIGINT,SIG_IGN);
		qstat = signal(SIGQUIT,SIG_IGN);
		hstat = signal(SIGHUP,SIG_IGN);
		tstat = signal(SIGTERM,SIG_IGN);

		if(mklock(argv[2],prog,lockfile) != 0) {
			if (tmpstdin) {
				unlink(table3);
				table3 = NULL;
			}
			(void) signal(SIGINT,istat);
			(void) signal(SIGQUIT,qstat);
			(void) signal(SIGHUP,hstat);
			(void) signal(SIGTERM,tstat);
			return(exitcode);
		}
		/* copy original table to temp file */
		if((utfp1=fopen(argv[2],"r")) == NULL) {
			error(E_DATFOPEN,prog,argv[2]);
			if (tmpstdin) {
				unlink(table3);
				table3 = NULL;
			}
			return(exitcode);
		}
		sprintf(tmptable,"%s/itmp2XXXXXX",dir);	/* temp file */
		mktemp(tmptable);
		if((utfp2 = fopen(tmptable,"w"))== NULL) {/* create tmp file */
			error(E_TEMFOPEN,prog,tmptable);
			if (tmpstdin) {
				unlink(table3);
				table3 = NULL;
			}
			return(exitcode);
		}
		while ((c=getc(utfp1)) != EOF)
			if (putc(c,utfp2) == EOF) {
				error(E_DATFWRITE,prog,tmptable);
				if (tmpstdin) {
					unlink(table3);
					table3 = NULL;
				}
				return(exitcode);
			}
		fclose(utfp1);
		utfp1 = NULL;
		tblname = tmptable;	/* current output table name */

		/* leave tmp file open to append new records */
	}
	else if (intoptr == 0) {	/* insert in - */

		if ((coption) && (Ooption == NULL)) {
			if (putdescrwtbl(stdout, Dtable1) == NULL) {
				error(E_DATFOPEN,prog,"-");
				if (tmpstdin) {
					unlink(table3);
					table3 = NULL;
				}
				return(exitcode);
			}
		}
		utfp2 = stdout;
		tblname = "-";		/* current output table name */
	}
	else {	/* insert in table1 into table2 ; table1 and table2 may or may
		   not be "-" */
		if(strcmp(argv[intoptr+1],"-") != 0) {
			/* create output table */
			if(chkaccess(argv[intoptr+1],00) == 0) {
				if(!onto) {
					error(E_EXISTS,prog,argv[intoptr+1]);
					if (tmpstdin) {
						unlink(table3);
						table3 = NULL;
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
				if (tmpstdin) {
					unlink(table3);
					table3 = NULL;
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
			/* get output description file; out table is stdout */
			if ((Ooption) && (*Ooption)) {
				getfile(dtable2,Ooption,0);
			} else if ((coption) && (Ooption == NULL)) {
				if (putdescrwtbl(stdout, Dtable1) == NULL) {
					error(E_DATFOPEN,prog,"-");
					if (tmpstdin) {
						unlink(table3);
						table3 = NULL;
					}
					return(exitcode);
				}
				dtable2[0] = '\0';
			} else {
				dtable2[0] = '\0';
			}
			utfp2 = stdout;
		}
		/* generate output description file */
		if ((dtable2[0]) &&
		    (strcmpfile(Dtable1,dtable2) != 0)) {
			if(chkaccess(dtable2,00) == 0) {
				if(!onto) {
					error(E_EXISTS,prog,dtable2);
					if (tmpstdin) {
						unlink(table3);
						table3 = NULL;
					}
					return(exitcode);
				}
			}
			else {
				strcpy(Dtable2,dtable2);
				if(copy(prog,Dtable1,Dtable2)!=0) {
					if (tmpstdin) {
						unlink(table3);
						table3 = NULL;
					}
					return(exitcode);
				}
			}
		}
		tblname = argv[intoptr+1];	/* current output table name */

		/* copy input file, if any, to output */
		if(strcmp(argv[2],"-") != 0) {
			/* copy table to temp file */
			if ((utfp1=fopen(argv[2],"r")) == NULL) {
				error(E_DATFOPEN,prog,argv[2]);
				if (tmpstdin) {
					unlink(table3);
					table3 = NULL;
				}
				return(exitcode);
			}
			while ((c=getc(utfp1)) != EOF)
				if (putc(c,utfp2) == EOF) {
					error(E_DATFWRITE,prog,tblname);
					if (tmpstdin) {
						unlink(table3);
						table3 = NULL;
					}
					return(exitcode);
				}
			fclose(utfp1);
			utfp1 = NULL;
		}
	}

	/* no longer need any temporary input descriptor table */
	if (DtmpItable[0]) {
		/* Note this will also remove Dtable1 */
		unlink(DtmpItable);
		DtmpItable[0] = '\0';
	}

	/* copy new records to output file */
	if(strcmp(argv[argc-1],"-") == 0) {
		/*
		 * Should not hit this case anymore
		 * but will leave it just in case
		 * someone removes the "from -" stuff.
		 */
		utfp1 = stdin;
	} else {
		if((utfp1 = fopen(argv[argc-1],"r"))== NULL) {
			error(E_TEMFOPEN,prog,argv[argc-1]);
			if (tmpstdin) {
				unlink(table3);
				table3 = NULL;
			}
			return(exitcode);
		}
	}
	while((c=getc(utfp1)) != EOF)
		if (putc(c,utfp2) == EOF) {
			error(E_DATFWRITE,prog,tblname);
			if (tmpstdin) {
				unlink(table3);
				table3 = NULL;
			}
			return(exitcode);
		}
	if (utfp2 != stdout) {
		if (fclose(utfp2) == EOF) {
			error(E_DATFWRITE,prog,tblname);
			if (tmpstdin) {
				unlink(table3);
				table3 = NULL;
			}
			return(exitcode);
		}
	} else {
		if (fflush(utfp2) == EOF) {
			error(E_DATFWRITE,prog,tblname);
			if (tmpstdin) {
				unlink(table3);
				table3 = NULL;
			}
			return(exitcode);
		}
	}
	utfp2 = NULL;

	if(utfp1 != stdin)
		fclose(utfp1);
	utfp1 = NULL;

	if (tmpstdin) {
		unlink(table3);
		table3 = NULL;
	}

	if(update) {
		/* move the new table to the old */
		if (stat(argv[2],&statbuf) != 0) {
			error(E_GENERAL,
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
			tmptable[0] = '\0';
			(void) signal(SIGINT,istat);
			(void) signal(SIGQUIT,qstat);
			(void) signal(SIGHUP,hstat);
			(void) signal(SIGTERM,tstat);
			return(exitcode);
		}
		(void)unlink( tmptable );
		tmptable[0] = '\0';
#endif
	}

	if(!qoption) {
		if (intoptr == 0)
			intoptr = 1;	/* pointer to "in" */
		error(E_GENERAL,"%s: %d records inserted %s %s\n",
			prog, i, argv[intoptr], argv[intoptr+1]);
	}

	exitcode = 0;
	if(update) {
		(void) signal(SIGINT,istat);
		(void) signal(SIGQUIT,qstat);
		(void) signal(SIGHUP,hstat);
		(void) signal(SIGTERM,tstat);
	}
	else {
		table2 = NULL;
		Dtable2[0] = '\0';
	}
	if(table3) {
		unlink(table3);
		table3 = NULL;
	}

	return(exitcode);
}

static int
promptrec( prog, tblname )
char *prog;
char *tblname;
{
	int	i,j,eof;
	int recordnr;
	int size_newrec;
	char *val;

	eof = 0;
	recordnr = 0;
	while(!eof) {
		recordnr++;
		error(E_GENERAL,"New record:\n");
		j = 0;
		val = buf;
		i = 0;
		while( i < nattr1 )
		{
			xx[i].val = val;
			error(E_GENERAL,"%s:",xx[i].aname);
			if(gets( val ) == NULL) {
				eof = 1;
				break;
			}
			if (xx[i].flag==WN) {
				j = strlen( val );

				if( j != xx[i].flen )
				{
					error(E_GENERAL, "Attribute %s must be %d characters long.\n",
						xx[i].aname, xx[i].flen);
					continue;
				}
			}
			else {	/* xx[i].flag==T */
				j = strlen( val );
				while(val[j-1] == '\\' && 
					(j<2 || val[j-2] != '\\') )
				{
					/*
					 * Replace backslash with newline
					 * or just remove backslash.
					 */
					if ( newline )
						val[ j - 1 ] = newline;
					else
						val[ --j ] = '\0';
					if ( gets( &val[j] ) == NULL )
					{
						eof = 1;
						break;
					}
					j = strlen( val );
				}
			}

			val[j++] = '\0';
			val += j;
			i++;		/* next attribute */
		}
		if( i == nattr1 )
		{
			if(terminal)
			{
				char s[BUFSIZ];

				error(E_GENERAL,"Insert record? ");
				gets(s);
				if(s[0] == 'n' || s[0] == 'N')
					continue;
			}
			size_newrec = 0;
			for ( i = 0; i < nattr1; i++ ) {
				if ((j = putnrec(utfp1, &xx[i])) < 0) {
					error(E_DATFWRITE,prog,tblname);
					return(ERR);
				}
				size_newrec += j;
			}

			if(xx[nattr1-1].flag == WN)
			{
				if (putc('\n',utfp1) == EOF) {
					error(E_DATFWRITE,prog,tblname);
					return(ERR);
				}
				size_newrec++;
			}

			if ( size_newrec > MAXREC )
			{
				error( E_GENERAL, "%s: Error: new rec# %d would be longer (%d) than maximum record length (%d)\n",
					prog, recordnr, size_newrec, MAXREC );
				return( ERR );
			}
		}
	}

	if (fflush(utfp1) == EOF) {
		error(E_DATFWRITE,prog,tblname);
		return(ERR);
	}

	return( 0 );
}

static int
recget( prog, tblname )
char *prog;
char *tblname;
{
	extern	int	end_of_tbl;
	int	i, j;
	int	size_newrec;
	int	recordnr;

	recordnr = end_of_tbl = 0;
	for(;;) {
		/* get next record */

		error(E_GENERAL,"new record:\n");
		recordnr++;
		newrec();
		for(i=0; i<nattr1 && getrec(stdin,&xx[i])!=ERR; i++);
		if (( i == 0 ) && ( end_of_tbl ))
			break;
		else if ( i < nattr1 )
		{
			error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of rec# %d\n",
				prog, i, recordnr );

			return( ERR );
		}

		if(xx[nattr1-1].flag == WN) 
			if ((feof(stdin)) || ((i = getc(stdin)) == EOF)) {
				error( E_GENERAL,
					"%s: Error: missing newline on rec# %d\n",
					prog, recordnr );
				return( ERR );
			} else if (i != '\n') {
				error( E_GENERAL,
					"%s: Error: data overrun on rec# %d\n",
					prog, recordnr );
				return( ERR );
			}

		size_newrec = 0;
		for(i=0; i<nattr1;i++) {
			if ((j = putnrec(utfp1,&xx[i])) < 0) {
				error(E_DATFWRITE,prog,tblname);
				return( ERR );
			}
			size_newrec += j;
		}
		if(xx[nattr1-1].flag == WN)
		{
			if (putc('\n',utfp1) == EOF) {
				error(E_DATFWRITE,prog,tblname);
				return( ERR );
			}
			size_newrec++;
		}

		if ( size_newrec > MAXREC )
		{
			error( E_GENERAL, "%s: Error: new rec# %d would be longer (%d) than maximum record length (%d)\n",
				prog, recordnr, size_newrec, MAXREC );
			return( ERR );
		}
	}

	if (fflush(utfp1) == EOF) {
		error(E_DATFWRITE,prog,tblname);
		return(ERR);
	}

	return( 0 );
}
