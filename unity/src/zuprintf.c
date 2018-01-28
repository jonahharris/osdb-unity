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
#include <ctype.h>

#ifdef __CYGWIN__
static usage();
static fmtchk();
#endif

extern  char    *mktemp(), *strcpy(), *dirname(), *strrchr();
extern  FILE    *getdescrwtbl(), *putdescrwtbl();
extern	FILE	*packedopen();
extern	int	packedclose();

extern	char	*packedsuffix;
extern  char    tmptable[], lockfile[];
extern  char    DtmpItable[];
extern  FILE    *utfp1,*utfp2;
extern	char	*table2, Dtable2[];
extern	int	end_of_tbl;
extern	int	term_escapes;

static	int	exitcode;

uprintf(argc,argv)
register int argc;
register char *argv[];
{       
	int	resultptr,inptr, attrptr, whereptr, intoptr, onto=0, update;
	char	*s_args[MAXATT+MAXATT+1];	/* allow one extra per result */
	char	*starts[MAXATT+MAXATT];		/* allow one extra per result */
	char	RecordNumber[32];	/* buffer for "rec#" */
	char	dtable2[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	*tblname;
	int	coption, updated, s_cnt, j, nattr2, nattr3, packed;
	unsigned short	p[MAXATT+MAXATT+1];	/* allow one extra per result */
	short	s_start[MAXATT];		/* starting indexes into starts[] */
	short	s_count[MAXATT];		/* attribute count per result */
	int	afld, printed, i, endf1, nattr1, recordnr, qoption, foption;
	RETSIGTYPE	(*istat)(), (*qstat)(), (*hstat)(), (*tstat)();
	char    c, Dtable1[MAXPATH+4], newvalue[MAXREC], dir[MAXPATH+4], *prog;
	char	*format;
	char	**where, newline = '\0', *Ioption, *Ooption;
	off_t	size_newtable;
	struct	stat statbuf;
	struct  fmt xx[MAXATT+1];	/* add one for "rec#" */
					/* also serves as a fixed attribute */
	char fmtbuf[4];			/* Default format strings */

	/* program name */
	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	Ooption = Ioption = table2 = NULL;
	Dtable2[0] = '\0';
	DtmpItable[0] = '\0';
	RecordNumber[0] = RecordNumber[1] = '\0';

	qoption = foption = coption = printed = updated = endf1 = packed = 0;

	tmptable[0] = lockfile[0] = '\0';
	exitcode = 1;

	while (  argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		switch(argv[1][1]) {
		case 'n':
			/* break field values at newline */
			newline = (argv[1][2] != '\0') ? argv[1][2] : NLCHAR;
			break;
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
		case 'f':
			if(argv[1][2]) {
				foption = 1;
				/* save output format */
				format = &argv[1][2];
			}
			break;
		default:
			error(E_BADFLAG,prog,argv[1]);
			return(exitcode);
		}
		argc--;
		argv++;
	}

	update = whereptr = resultptr = inptr = intoptr= 0;

	for(afld=0,i=1;i<argc;i++)
		if ((strncmp(argv[i],"result",6) == 0) &&
		   ((argv[i][6] == '\0') || (argv[i][6] == ':'))) {
			if (((resultptr) && (i < resultptr + 3)) || (i < 2)) {
				(void) usage(prog);
				return(exitcode);
			}
			if (++afld > MAXATT) {
				error(E_GENERAL,
					"%s: maximum number of result attributes (%d) exceeded\n",
					prog, MAXATT);
				return(exitcode);
			}
			/*
			 * save current result index and
			 * increment the index so that
			 * "result" can be used as the
			 * name of a result attribute
			 */
			resultptr = i++;
			/*
			 * Give the user an explicit error message
			 * if they enter "result:format" as two
			 * arguments by indicating that ":format"
			 * is an illegal attribute name.
			 */
			if ((argc > i) && (argv[i][0] == ':')) {
				error(E_GENERAL,
					"%s: %s is an illegal attribute name\n",
					prog, argv[i]);
				return(exitcode);
			}
			/*
			 * If "in" follows the result attribute then
			 * there are no more results to be found.
			 */
			if ((i < argc-1) && (strcmp(argv[i+1],"in") == 0))
				break;
		}

	for(i=1;i<argc;i++)
		if(strcmp(argv[i],"in") == 0) {
			inptr = i;
			break;
		}

	/*
	 * Make sure we found the "in table" clause
	 * and if there was a previous result index
	 * then make sure that inptr = resultptr+2
	 */
	if ((inptr == 0) || (inptr >= argc-1) ||
	   ((resultptr) && (inptr != resultptr+2))) {
		(void) usage(prog);
		return(exitcode);
	}

	/* check for "into table" clause */
	if (argc >= inptr+4) {
		if (strcmp(argv[inptr+2],"into") == 0)
			intoptr = inptr+2;
		else if (strcmp(argv[inptr+2],"onto") == 0) {
			intoptr = inptr+2;
			onto = 1;
		}
	}

	/* get current argv[] index and check for "where-clause" and/or errors */
	if (intoptr)
		i = intoptr+2;
	else
		i = inptr+2;
	if (i != argc) {
		if (strcmp(argv[i],"where") != 0) {
			(void) usage(prog);
			return(exitcode);
		}
	}
	whereptr = i;	/* make sure whereptr set ok for getcond() */

	if (resultptr > 0 && intoptr == 0 && strcmp(argv[inptr+1],"-") != 0)
		update = 1;	/* doing an update on table using locks, etc. */

	if (foption == 0) {
		format = fmtbuf;
		strcpy(format, (resultptr) ? "%s" : "%s\n" );
	}

	/* process '\' escapes */
	constr(format);

	/* count the %s fields */
	s_cnt = fmtchk(prog,format,starts,0);
	if (s_cnt < 0) {
		if (s_cnt == -1)
			error(E_GENERAL,
			"%s: format request contains non-string conversion requests\n",
			prog);
		return(exitcode);
	}

	if (resultptr) {
		if (strrchr(format, '\n') != NULL) {
			error(E_GENERAL,
				"%s: format request contains embedded new-line character\n",
				prog);
			return(exitcode);
		}
		if ((format[0]) && (format[strlen(format)-1] == '\\')) {
			error(E_GENERAL,
				"%s: format request contains backslash as last character of attribute\n",
				prog);
			return(exitcode);
		}
	}

	if ((Ooption) && (!(intoptr))) {
		error(E_GENERAL,
			"%s: Missing \"into table\" clause for the \"-O%s\" option.\n",
			prog, Ooption);
		return(exitcode);
	}

	if(update) {/* get ready for update in place */
		strcpy(dir,argv[inptr+1]);
		dirname(dir);           /* get directory name of table */
		if(chkaccess(dir,2)!=0) { /* check write permission of directory */
			error(E_DIRWRITE,prog,dir);
			return(exitcode);
		}
	}
	if(strcmp(argv[inptr+1],"-") != 0) {
		if((utfp1 = fopen(argv[inptr+1],"r")) == NULL) {
			if ((update) ||
			   ((intoptr) && (strcmp(argv[inptr+1],argv[intoptr+1]) == 0)) ||
			   ((utfp1 = packedopen(argv[inptr+1])) == NULL)) {
				/* check read perm of table */
				error(E_DATFOPEN,prog,argv[inptr+1]);
				return(exitcode);
			}
			++packed;	/* reading packed table */
		}
	}
	else {
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

	if((nattr1 = mkntbl(prog,argv[inptr+1],Dtable1,xx,Ioption)) == ERR) {
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

	if (resultptr == 0) {
		attrptr = inptr;

		/* Process the anames */
		for(i=1,nattr2=0;i<attrptr;i++,nattr2++) {
			if(i >= MAXATT) {
				error(E_GENERAL,
					"%s: Too many attributes specified.\n",
					prog);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			if((j=setnum(xx,argv[i],nattr1+1)) == ERR) {
				error(E_ILLATTR,prog,argv[i],Dtable1);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			p[nattr2] = j;

			/* check for reference to "rec#" */
			if (j == nattr1) {
				s_args[nattr2] = RecordNumber;
				RecordNumber[0] = '0';
			}
		}
		if (s_cnt > nattr2) {
			error(E_GENERAL,
				"%s: too many %%s fields for given argument list\n",
				prog);
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}
		/*
		 * make sure p[s_cnt] is a valid attribute index
		 * and increment s_cnt to include any text that
		 * followed the last %s conversion request.
		 */
		if (s_cnt++ == nattr2)
			p[nattr2] = nattr1;	/* "rec#" */

	} else { /* (resultptr != 0) /*

		/*
		 * Initialize ending index into starts[]
		 * for each possible result attribute
		 * which is never zero when being used.
		 */
		for (i = 0; i < nattr1; i++)
			s_count[i] = 0;

		nattr2 = nattr3 = 0;

		for (j = 0, i = 1; i < inptr; j++)
		{
			char *formatptr;

			/*
			 * NOTE: The previous syntax check(s) insures that a
			 *	 "result" string will be found in argv[].
			 */
			resultptr = i + 1;
			while ((strncmp(argv[resultptr],"result",6) != 0) ||
			      ((argv[resultptr][6]) && (argv[resultptr][6] != ':'))) {
				++resultptr;
			}

			if ((afld = setnum(xx,argv[resultptr+1],nattr1)) == ERR) {
				error(E_ILLATTR,prog,argv[resultptr+1],Dtable1);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}

			if(xx[afld].flag == WN) {  
				error(E_GENERAL,
				"%s: Result attribute [%s] cannot be a fixed width field.\n",
					prog, argv[resultptr+1]);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}

			if (s_count[afld]) {
				error(E_GENERAL,
				"%s: Duplicate result attribute [%s]\n",
					prog, argv[resultptr+1]);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}

			if ((argv[resultptr][6] == ':') &&
			    (argv[resultptr][7] != '\0')) {
				formatptr = &argv[resultptr][7];

				/* process '\' escapes */
				constr(formatptr);

				if (strrchr(formatptr, '\n') != NULL) {
					error(E_GENERAL,
						"%s: format request for result %d contains embedded new-line character\n",
						prog, j+1);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
				if ((formatptr[0]) && (formatptr[strlen(formatptr)-1] == '\\')) {
					error(E_GENERAL,
						"%s: format request for result %d contains backslash as last character of attribute\n",
						prog, j+1);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
			} else {
				/* use the "-fformat" that was given (or its default) */
				formatptr = format;
			}

			/* count the %s fields */
			if ((s_cnt = fmtchk(prog,formatptr,&starts[nattr2],j)) < 0) {
				if (s_cnt == -1)
					error(E_GENERAL,
					"%s: format request contains non-string conversion requests\n",
					prog);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			/*
			 * Save current starting index
			 * and %s count plus one since
			 * there is always a plain text
			 * string that follows the last
			 * %s request, which may be the
			 * empty string ("").
			 */
			s_start[afld] = nattr2;
			s_count[afld] = s_cnt+1;

			while (s_cnt--)
			{
				if (i == resultptr) {
					error(E_GENERAL,
						"%s: too many %%s fields for result [%s] attribute list\n",
						prog, argv[resultptr+1]);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
				if (++nattr3 >= MAXATT) {
					error(E_GENERAL,
						"%s: Too many attributes specified.\n",
						prog);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
				/* get attribute number and increment loop index */
				if ((afld=setnum(xx,argv[i++],nattr1+1)) == ERR) {
					error(E_ILLATTR,prog,argv[i-1],Dtable1);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}

				/* check for reference to "rec#" */
				if (afld == nattr1) {
					s_args[nattr2] = RecordNumber;
					RecordNumber[0] = '0';
				}

				p[nattr2++] = afld;
			}

			p[nattr2++] = nattr1;	/* "rec#" is never a result */

			while (i < resultptr)
			{
				/*
				 * Since there are no %s conversion requests
				 * associated with these attributes in starts[]
				 * we just want to make sure they are valid
				 * attribute names and that the maximum
				 * attribute limit has not been exceed.
				 */
				if (++nattr3 >= MAXATT) {
					error(E_GENERAL,
						"%s: Too many attributes specified.\n",
						prog);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
				/* verify attribute and increment loop index */
				if (setnum(xx,argv[i++],nattr1+1) == ERR) {
					error(E_ILLATTR,prog,argv[i-1],Dtable1);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
			}

			i += 2;		/* argv[i] == "in" ? */
		}
	}

	/* parse where clause - build evaluation tree */
	where = &argv[whereptr];
	getcond(where,xx,nattr1,prog,Dtable1);

	if(update) {
		/* create the lock file for update-in-place */
		istat = signal(SIGINT,SIG_IGN);
		qstat = signal(SIGQUIT,SIG_IGN);
		hstat = signal(SIGHUP,SIG_IGN);
		tstat = signal(SIGTERM,SIG_IGN);

		/* create lock file */
		if(mklock(argv[inptr+1],prog,lockfile) != 0) {
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
		sprintf(tmptable,"%s/ctmpXXXXXX",dir);
		mktemp(tmptable);
		if((utfp2 = fopen(tmptable,"w")) == NULL)  {
			error(E_TEMFOPEN,prog,tmptable);
			return(exitcode);
		}
		tblname = tmptable;	/* current output table name */

	} else if (intoptr == 0) {
		tblname = "-";		/* current output table name */
		utfp2 = stdout;
	} else {
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
			if (resultptr != 0) {
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
					getfile(dtable2, argv[intoptr+1], 0);
				}
			} else {
				dtable2[0] = '\0';
			}
		} else {
			if ((resultptr) && (Ooption) && (*Ooption)) {
				getfile(dtable2, Ooption, 0);
			} else {
				/* make sure dtable2 is null ("") */
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
				if(copy(prog,Dtable1,Dtable2) != 0) {
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
			}
		}
	}

	if ((coption) && (resultptr) && (utfp2 == stdout) && (Ooption == NULL)) {
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
		/* get next record */
		newrec();
		for(i=0; i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
		if (( i == 0 ) && ( end_of_tbl ))
			break;
		else if ( i < nattr1 )
		{
			error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
				prog, i, recordnr + 1, argv[inptr+1] );

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

		if(endf1) 
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

		if (RecordNumber[0])
			sprintf(RecordNumber, "%u", recordnr);

		if (resultptr == 0) {	/* write out only selected records */

			if (selct(xx,recordnr)) {
				/* write only new field */
				for(i=0;i<nattr2;i++) {	/* get string arg list */
					s_args[i] = xx[p[i]].val;
					chng_nl(s_args[i], newline);
				}
				/* format result */
				sprntf(newvalue,starts,s_args,s_cnt);
				for(i = 0; (c = newvalue[i]) != 0; i++)
					putc(c,utfp2);	/* print */
				printed++;
			}

		} else {	/* write out all fields on all records */

			int size_newrec = 0;

			if (selct(xx,recordnr))
			{
				int k,l;

				/* get string arg list */
				for(j=0;j<nattr2;j++) {
					if ((k = p[j]) < nattr1)
						s_args[j] = xx[k].val;
				}

				/*
				 * loop through all fields
				 * and print new value as appropriate
				 */
				for(i = 0; i < nattr1; i++) {

					if (s_count[i]) {
						char *oldval;

						l = s_count[i];
						k = s_start[i];

						/* format result */
						sprntf(newvalue,&starts[k],&s_args[k],l);

						/* write new field value */
						oldval = xx[i].val;
						xx[i].val = newvalue;
						if ((j = putnrec(utfp2, &xx[i])) < 0) {
							error(E_DATFWRITE,prog,tblname);
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
						/* write old field value */
						if ((j = putnrec(utfp2,&xx[i])) < 0) {
							error(E_DATFWRITE,prog,tblname);
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
				++updated;

			} else {
				/* write all fields unchanged */
				for(i=0; i<nattr1; i++) {
					if ((j = putnrec(utfp2,&xx[i])) < 0) {
						error(E_DATFWRITE,prog,tblname);
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
			if(endf1 == 1)
			{
				if (putc('\n',utfp2) == EOF) {
					error(E_DATFWRITE,prog,tblname);
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
			printed++;
		}
	}

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
		if(unlink(argv[inptr+1]) < 0) {
			error(E_BADUNLINK,prog,argv[inptr+1],tmptable);
			(void) signal(SIGINT,istat);
			(void) signal(SIGQUIT,qstat);
			(void) signal(SIGHUP,hstat);
			(void) signal(SIGTERM,tstat);
			return(exitcode);
		}
		if(link(tmptable,argv[inptr+1])<0) {
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

	if (!qoption) {
		if (update) printed = updated;
		error(E_GENERAL,"%s: %d records printed in %s.\n",
			prog,printed,argv[inptr+1]);
	}
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

static int
constr(s)
char *s;
{
	char	*p=s, c;
	int	i;

	while ( (c = *s++) != '\0') {
		if (c!='\\') {
			*p++=c;
			continue;
		}
		switch(c = *s++) {
		default:
			*p++=c;
			break;
		case  0 :	/* EOS ( '\0' == 0 ) */
			s--;
			break;
		case 'n': 
			*p++='\n'; 
			break;
		case 't': 
			*p++='\t'; 
			break;
		case 'b': 
			*p++='\b'; 
			break;
		case 'r': 
			*p++='\r'; 
			break;
		case 'f': 
			*p++='\f'; 
			break;
		case '\\': 
			*p++='\\'; 
			break;
		case '\'': 
			*p++='\''; 
			break;
		case '0': 
		case '1': 
		case '2': 
		case '3':
		case '4': 
		case '5': 
		case '6': 
		case '7':
			i=c-'0';
			while (isdigit(c = *s++))
				i=8*i+c-'0';
			*p++=i;
			s--;
			break;
		}
	}
	*p = '\0';
}

static int
fmtchk(prog, string, starts, reqno)
char *prog;
char *string;		/* input print format string */
char *starts[];		/* where to load sprintf(3C) format strings */
int  reqno;		/* set to zero (0) to initialize buffer/indexes */
{
	static	unsigned	prtidx;	/* print request buffer index/offset */
	static	unsigned	snum;	/* total number of %s requests */
	static	char	prtbuf[MAXREC];
	int i, ok, n, percent;
	int first, period;	/* check format */

	/* reset indexes if first request */
	if (reqno == 0) {
		prtidx = 0;
		snum = 0;
	}

	n = 0;
	starts[n] = &prtbuf[prtidx];

	ok = 1;
	percent = 0;

	for(i=0; string[i] != '\0' && ok != 0;i++) {

		if (prtidx >= sizeof(prtbuf)) {
			error(E_GENERAL,
				"%s: format request exceeds maximum buffer size (%d)\n",
				prog, MAXREC);
			return(-3);
		}

		prtbuf[prtidx++] = string[i];

		if (!percent) {
			if (string[i] == '%') {
				percent = 1;
				first = 1;
			}
		}
		else {
			if(first)
				period = 0;
			switch(string[i]) {
			case '%':
				if (!first)
					ok = 0;
				else
					percent = 0;
				break;
			case '-':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				break;
			case '.':
				if(period)
					ok = 0;
				else
					period = 1;
				break;
			case 's':
				/*
				 * make sure we have not exceed the
				 * %s conversion request limit which
				 * also insures that we do not write
				 * beyond the end of starts[].
				 */
				if (++snum > MAXATT) {
					error(E_GENERAL,
						"%s: format %%s conversion requests exceed maximum attribute (%d) limit\n",
						prog, MAXATT);
					return(-2);
				}
				n = n + 1;
				percent = 0;

				if (prtidx >= sizeof(prtbuf)) {
					error(E_GENERAL,
						"%s: format request exceeds maximum buffer size (%d)\n",
						prog, MAXREC);
					return(-3);
				}

				prtbuf[prtidx++] = '\0';
				starts[n] = &prtbuf[prtidx];
				break;
			default:
				ok = 0;
				break;
			}
			first = 0;
		}
	}

	if (prtidx >= sizeof(prtbuf)) {
		error(E_GENERAL,
			"%s: format request exceeds maximum buffer size (%d)\n",
			prog, MAXREC);
		return(-3);
	}

	/* terminate the trailing text, if any, that follows the last %s */
	prtbuf[prtidx++] = '\0';

	if (!ok || percent) {
		/*
		 * Caller is expected to print an appropriate
		 * error message when the return code is -1.
		 */
		n = -1;
	}

	return(n);
}

static int
chng_nl(string,newline)
char *string;
char newline;
{
	int i;

	for(i=0; string[i] != '\0'; i++) {
		if (string[i] == newline)
			string[i] = '\n';
	}
}

static int
sprntf(newvalue,starts,s_args,s_cnt)
char *newvalue;
char *starts[];
char *s_args[];
int s_cnt;
{
	int i, len;

	len = 0;
	for(i=0; i < s_cnt; i++) {
		if (starts[i][0] != '\0')
			len += sprintf(&newvalue[len],starts[i],s_args[i]);
		else
			newvalue[len] = '\0';
	}
}

static int
usage( prog )
char	*prog;
{
	error(E_GENERAL, "Usage: %s %s \\\n\t%s \\\n\t%s \\\n\t%s\n", prog,
		"[-c] [-q] [-Itable] [-Otable] [-fformat] [-nc] aname",
		"[aname ...] [ { result | result:format } aname1]",
		"[aname ... { result | result:format } anameN]",
		"in table1 [into table2] [where clause]");
}
