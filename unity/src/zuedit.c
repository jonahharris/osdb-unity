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

static	char	*prog;
extern	char	*strcpy(),*strrchr(),*dirname(),*getenv(),*mktemp();

extern	char	Dpak[],*table2;
extern	char	Dtable2[],pak[];
extern  char    tmptable[], lockfile[];
extern	FILE	*utfp0, *utfp1,*utfp2;
extern	char	Uunames[MAXATT][MAXUNAME+1];	/* in mktbl2.c */
extern	int	end_of_tbl;
static	int	exitcode;
static	char	tabbuf[MAXPATH+4];	/* so table2 is a pointer */

uedit(argc, argv)
char	*argv[];
int	argc;
{
	struct	fmt xx[MAXATT];
	int	endf, i, j, selected,flg,nattr1;
	int	dlen,valopt,recordnr,qoption,uoption;
	char	*table,*p0,**p, *Ioption;
	char	Dtable[MAXPATH+4], pid[MAXFILE], uopt[4];
	char	cmd1[MAXCMD],cmd2[MAXCMD],cmd3[MAXCMD],*ed;
	char	newline, dir[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	string[256];
	struct	stat statbuf;
	RETSIGTYPE	(*istat)(), (*qstat)(), (*hstat)(), (*tstat)();

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}

	uoption = qoption = valopt = selected = endf = 0;  
	newline = NLCHAR;

	Dtable2[0] = pak[0] = Dpak[0] = uopt[0] = '\0';
	tmptable[0] = lockfile[0] = '\0';
	Ioption = table2 = NULL;
	exitcode = 1;

	while ( argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		switch(argv[1][1]) {
			default:
				error(E_BADFLAG,prog,argv[1]);
				return(exitcode);
			case 'I':
				Ioption = &argv[1][2];
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
			case 'q':
			case 'u':
			case 'v':
				for (i = 1; argv[1][i]; i++)
					switch (argv[1][i]) {
					case 'v':
						valopt = 1;
						break;
					case 'q':
						qoption = 1;
						break;
					case 'u':
						uoption = 1;
						strcpy(uopt,"-u");
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

	if(argc < 2) {
	error(E_GENERAL,
	"Usage: %s [-q] [-Itable] [-v] [-n[nline]] [-u] table [where clause]\n",
		prog);
		return(exitcode);
	}

	table = argv[1];

	strcpy(dir,table);
	dirname(dir);
	if(chkaccess(dir,02) < 0) {
		error(E_DIRWRITE,prog,dir);
		return(exitcode);
	}

	umask(002);

	istat = signal(SIGINT,SIG_IGN);
	qstat = signal(SIGQUIT,SIG_IGN);
	hstat = signal(SIGHUP,SIG_IGN);
	tstat = signal(SIGTERM,SIG_IGN);

	if(mklock(table,prog,lockfile) != 0) {
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

	sprintf(pid,"%d",getpid());
	sprintf(tabbuf,"table2%s",pid);
	table2 = tabbuf;
	sprintf(Dtable2,"D%s",table2);
	sprintf(pak,"pak%s",pid);
	sprintf(Dpak,"D%s",pak);
	sprintf(tmptable,"%s/tmpXXXXXX",dir);
	mktemp(tmptable);

	if ((utfp1 = fopen(table,"r")) == NULL) {	/* input table */
		error(E_DATFOPEN,prog,table);
		return(exitcode);
	}
	if ((utfp2 = fopen(pak,"w")) == NULL) {	/* output - selected */
		error(E_DATFWRITE,prog,pak);
		return(exitcode);
	}
	if ((utfp0 = fopen(tmptable,"w")) == NULL) {/* output - not selected */
		error(E_DATFWRITE,prog,tmptable);
		return(exitcode);
	}

	if((nattr1 = mkntbl2(prog,table,Dtable,xx,Ioption))==ERR)
		return(exitcode);
	if (xx[nattr1-1].flag == WN)
		endf = 1;

	p = &argv[2];
	getcond(p,xx,nattr1,prog,Dtable);

	recordnr = end_of_tbl = 0;
	for(;;) {
		newrec(); 
		for(i=0; i<nattr1 && getrec( utfp1, &xx[i] ) != ERR; i++)
			;
		if (( i == 0 ) && (end_of_tbl))
			break;
		else if ( i < nattr1 )
		{
			error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
				prog, i, recordnr + 1, table );

			error( E_GENERAL, "%s: Warning: no changes made\n",
				prog );
			return( exitcode );
		}
		if (endf) 
			if ((feof(utfp1)) || ((i = getc(utfp1)) == EOF)) {
				error( E_GENERAL,
					"%s: Error: missing newline on rec# %d in %s\n",
					prog, recordnr + 1, table );
				error( E_GENERAL,
					"%s: Warning: no changes made\n", prog );
				return(exitcode);
			} else if (i != '\n') {
				error( E_GENERAL,
					"%s: Error: data overrun on rec# %d in %s\n",
					prog, recordnr + 1, table );
				error( E_GENERAL,
					"%s: Warning: no changes made\n", prog );
				return(exitcode);
			}
		recordnr++;
		if(!selct(xx,recordnr)) {
			for(i=0; i<nattr1; i++)
				if (putnrec(utfp0,&xx[i]) < 0) {
					error(E_DATFWRITE,prog,tmptable);
					return(exitcode);
				}
			if (endf == 1)
				if (putc('\n',utfp0) == EOF) {
					error(E_DATFWRITE,prog,tmptable);
					return(exitcode);
				}
		}
		else {
			selected++;
			fprintf(utfp2,"#####	%d\n",selected);
			for (i=0;i<nattr1;i++) {
				if(uoption) {
					if (fprintf(utfp2,"%s\t",Uunames[i]) < 0) {
						error(E_DATFWRITE,prog,pak);
						return(exitcode);
					}
				} else {
					if (fprintf(utfp2,"%s\t",xx[i].aname) < 0) {
						error(E_DATFWRITE,prog,pak);
						return(exitcode);
					}
				}
				p0=xx[i].val;
				dlen=strlen(p0);
				if (xx[i].flag==T && newline != '\0') {
					for (j=0;j<dlen;j++) {
						if (*p0==newline) {
							if ((putc('\n',utfp2) == EOF) ||
							    (putc('\t',utfp2) == EOF)) {
								error(E_DATFWRITE,prog,pak);
								return(exitcode);
							}
						} else {
							if (putc(*p0,utfp2) == EOF) {
								error(E_DATFWRITE,prog,pak);
								return(exitcode);
							}
						}
						p0++;
					}
				}
				else {
					for (j=0;j<dlen;j++) {
						if (putc(*p0++,utfp2) == EOF) {
							error(E_DATFWRITE,prog,pak);
							return(exitcode);
						}
					}
				}
				if (putc('\n',utfp2) == EOF) {
					error(E_DATFWRITE,prog,pak);
					return(exitcode);
				}
			}
			if (putc('\n',utfp2) == EOF) {
				error(E_DATFWRITE,prog,pak);
				return(exitcode);
			}
		}
	}
	error(E_GENERAL,"%s: %d records selected from %s.\n", prog,
		selected, table);
	fclose(utfp1);
	utfp1 = NULL;
	if (fclose(utfp2) == EOF) {
		error(E_DATFWRITE,prog,pak);
		return(exitcode);
	}
	utfp2 = NULL;
	/* don't close utfp0 - ready to append */
	if (fflush(utfp0) == EOF) {
		error(E_DATFWRITE,prog,tmptable);
		return(exitcode);
	}

	if((ed = getenv("ED")) == NULL) {
		if((ed = getenv("EDITOR")) == NULL)
			ed = "ed";
	}
	sprintf(cmd1,"%s %s",ed,pak);
	if(newline)
		sprintf(cmd2,"tuple -n'%c' %s -I%s %s into %s",
			newline, uopt, pak, pak, table2);
	else
		sprintf(cmd2,"tuple -n %s -I%s %s into %s",
			uopt, pak, pak, table2);
	if (Ioption) {
		char *Iprefix = "";

		if (strcmp(Ioption,table) == 0)
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
		sprintf(cmd3,"validate -I%s%s %s", Iprefix, Ioption, table2);
	}
	else {
		char *tbl = table;

		/*
		 * Do not pass table name with a special prefix
		 * to validate in the -Itable option so that it
		 * does not search the data directory first.
		 */
		while ((strncmp(tbl,"/./",3) == 0) || (strncmp(tbl,"./",2) == 0))
			tbl += 2;

		sprintf(cmd3,"validate -I%s %s", tbl, table2);
	}

	if(link(Dtable,Dpak)<0) {
		if(copy(prog,Dtable,Dpak) != 0) {
			error(E_GENERAL,"%s: Cannot copy %s to %s\n",prog,
				Dtable,Dpak);
			return(exitcode);
		}
	}
	while ( 1 ) {
		unlink(table2);

		system(cmd1);			/* ed pak$$ */
		unlink(table2); unlink(Dtable2);
		if(system(cmd2) == 0) {		/*tuple -n pak$$ into table2$$*/
			if(!valopt) {
				if(concat()==ERR) /* read table2$$ into tmp$$ */
					return(exitcode);
				break;
			}
			if(system(cmd3) == 0) {
				if(concat() == ERR)
					return(exitcode);
				break;
			}
		}
		flg=1;
		while(flg){
			error(E_GENERAL,
				"reenter editor to correct problem (y or n)");
			gets(string);
			switch(string[0]) {
			case 'y':
				flg=0;
				break;
			case 'n':
				error(E_GENERAL,"Edit aborted, %s unchanged\n",
					table);
				error(E_GENERAL,"edited table is in %s\n",pak);
				pak[0] = '\0'; /* so uclean() won't remove */
				exitcode = 0;
				return(exitcode);
			default:
				error(E_GENERAL, "Try again!\n");
				break;
			}
		}
	}
	/*
	 * Close utfp0 - the tmptable.  This will flush out
	 * pending writes.  If not closed now, after
	 * the chown and chmod below, it may not be writable
	 * any more (at least not on some Unix versions), so
	 * the flush and close in uclean() will fail, and the
	 * file will be truncated.
	 */
	if (fclose(utfp0) == EOF) {
		error(E_DATFWRITE,prog,tmptable);
		return(exitcode);
	}
	utfp0 = NULL;

	flg=1;
	while(flg){
		error(E_GENERAL, "Make these changes in %s (y or n) ",table);
		gets(string);
		switch(string[0]) {
		case 'y':

			/* move the new table to the old */
			if (stat(table,&statbuf) != 0) {
				error( E_GENERAL,
					"%s: Error: stat(2) failure getting permissions and owner of old table %s\n",
					prog, table);
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
			error(E_LOCKSTMP,prog,table,tmptable);
			tmptable[0] = '\0';	/* don't remove temporary table */
			return(exitcode);
		}
#endif

			istat = signal(SIGINT,SIG_IGN);
			qstat = signal(SIGQUIT,SIG_IGN);
			hstat = signal(SIGHUP,SIG_IGN);
			tstat = signal(SIGTERM,SIG_IGN);

#ifdef	__STDC__
			if (rename(tmptable, table) != 0) {
				error(E_BADLINK,prog,tmptable);
				(void) signal(SIGINT,istat);
				(void) signal(SIGQUIT,qstat);
				(void) signal(SIGHUP,hstat);
				(void) signal(SIGTERM,tstat);
				return(exitcode);
			}
			tmptable[0] = '\0';	/* new table no longer exists */
#else	
			if(unlink(table)!=0) {
				error(E_BADUNLINK,prog,table,tmptable);
				tmptable[0] = '\0'; /* so uclean() won't remove */
				(void) signal(SIGINT,istat);
				(void) signal(SIGQUIT,qstat);
				(void) signal(SIGHUP,hstat);
				(void) signal(SIGTERM,tstat);
				return(exitcode);
			}
			if(link(tmptable,table)!=0) {
				error(E_BADLINK,prog, tmptable);
				tmptable[0] = '\0'; /* so uclean() won't remove */
				(void) signal(SIGINT,istat);
				(void) signal(SIGQUIT,qstat);
				(void) signal(SIGHUP,hstat);
				(void) signal(SIGTERM,tstat);
				return(exitcode);
			}
#endif
			if(!qoption)
				error(E_GENERAL, "Changes made, %s updated\n",table);
			flg=0;
			exitcode = 0;
			(void) signal(SIGINT,istat);
			(void) signal(SIGQUIT,qstat);
			(void) signal(SIGHUP,hstat);
			(void) signal(SIGTERM,tstat);
			break;
		case 'n':
			if(!qoption)
			error(E_GENERAL,"Changes not made\n");
			flg=0;
			break;
		default:
			error(E_GENERAL,"Try again!\n");
			break;
		}
	}
	exitcode = 0;
	return(exitcode);
}

static int
concat()
{
	char	fbuf[BUFSIZ];
	FILE 	*from;

	if((from = fopen(table2, "r")) == NULL) {
		error(E_DATFOPEN,prog,table2);
		return(ERR);
	}
	while(fgets(fbuf, BUFSIZ, from) != NULL) {
		if (fprintf(utfp0,"%s",fbuf) < 0) {
			error(E_DATFWRITE,prog,tmptable);
			return(ERR);
		}
	}
	fclose(from);
	return(0);
}
