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
#include <time.h>

#ifndef	FALSE
#define	FALSE	0
#endif
#ifndef	TRUE
#define	TRUE	1
#endif

/*
 * NOTE: Enumerated values for numeric type modifiers
 *	 are assigned the value that correspond to
 *	 their numeric (integer) base and all other
 *	 string or special function modifiers are
 *	 assigned values starting at 64 so that
 *	 the low order bits can be directly used
 *	 as the input (integer) base modifier.
 */
typedef enum prtmodifier {
	PRT_M_NULL = 0,
	PRT_M_NUMERIC = 1,
	PRT_M_BINARY = 2,
	PRT_M_OCTAL = 8,
	PRT_M_HEX = 16,
	PRT_M_STRING = 64,
	PRT_M_TOLOWER = 65,
	PRT_M_TRLOWER = 66,
	PRT_M_LTRLOWER = 67,
	PRT_M_RTRLOWER = 68,
	PRT_M_TOUPPER = 69,
	PRT_M_TRUPPER = 70,
	PRT_M_LTRUPPER = 71,
	PRT_M_RTRUPPER = 72,
	PRT_M_TRIM = 73,
	PRT_M_LTRIM = 74,
	PRT_M_RTRIM = 75,
	PRT_M_DATE = 76
} PRTMODIFIER;

struct printmod {
	char *modifier;
	enum prtmodifier mod;
	short len;
};

static struct printmod modlist[] = {
	"binary",	PRT_M_BINARY,		1,
	"hex",		PRT_M_HEX,		1,
	"octal",	PRT_M_OCTAL,		1,
	"numeric",	PRT_M_NUMERIC,		1,
	"ascii",	PRT_M_STRING,		1,
	"string",	PRT_M_STRING,		1,
	"tolower",	PRT_M_TOLOWER,		3,
	"toupper",	PRT_M_TOUPPER,		3,
	"trlower",	PRT_M_TOLOWER,		3,
	"ltrlower",	PRT_M_TOLOWER,		4,
	"rtrlower",	PRT_M_TOLOWER,		4,
	"trupper",	PRT_M_TRUPPER,		3,
	"ltrupper",	PRT_M_LTRUPPER,		4,
	"rtrupper",	PRT_M_RTRUPPER,		4,
	"trim",		PRT_M_TRIM,		2,
	"ltrim",	PRT_M_LTRIM,		3,
	"rtrim",	PRT_M_RTRIM,		3,
#ifndef	INTCOMP
	"date",		PRT_M_DATE,		2,
#endif
	NULL,		PRT_M_NULL,		0,
};

extern  char    *mktemp(), *strcpy(), *dirname(), *strrchr(), *strchr();
extern	char	*strcpylower(), *strcpyupper();
extern	FILE	*getdescrwtbl(), *putdescrwtbl();
extern	FILE	*packedopen();
extern	int	packedclose();

extern	char	*packedsuffix;
extern  char    tmptable[], lockfile[];
extern  FILE    *utfp1,*utfp2;
extern	char	*table2, Dtable2[];
extern	char	DtmpItable[];
extern	int	term_escapes;
extern	int	end_of_tbl;
static	int	exitcode;

#ifndef	INTCOMP

extern double	cnvtdate();
extern int	get_dt_buffers();

static int *dt_add2year;/* pointer to cnvtdate() static buffer */
static int *dt_year;	/* pointer to cnvtdate() static buffer */
static int *dt_month;	/* pointer to cnvtdate() static buffer */
static int *dt_day;	/* pointer to cnvtdate() static buffer */
static int *dt_hour;	/* pointer to cnvtdate() static buffer */
static int *dt_minute;	/* pointer to cnvtdate() static buffer */
static int *dt_second;	/* pointer to cnvtdate() static buffer */

static int CC;		/* centry years to be added or removed */
static int SS;		/* time (%T) is to include seconds ? */

#endif

static enum prtmodifier
getprintmod( str )
char *str;
{
	register struct printmod *prtptr;

	for ( prtptr = modlist; prtptr->modifier; prtptr++ ) {
		if ( strncmp( prtptr->modifier, str, prtptr->len ) == 0 )
		if ( strncmp( prtptr->modifier, str, prtptr->len ) == 0 )
			return( prtptr->mod );
	}

	return( PRT_M_NULL );
}

static void
prtmodlistmsg()
{
	register struct printmod *prtptr;
	register char *p, *q;
	int	i;
	char	msgbuf[1024];

	for ( i = 1, p = msgbuf, prtptr = modlist; prtptr->modifier; prtptr++, i++ ) {
		if ( i == 7 || i == 12 ) { /* compensate for longer 2nd and 3rd lines */
			*p++ = '\n';
			*p++ = '\t';
		}
		q = prtptr->modifier;
		while ( *q ) {
			*p++ = *q++;
		}
		*p++ = ' ';
	}
	*p = '\0';

	error(E_GENERAL, "Modifier must be one of the following:\n\t%s\n", msgbuf );
}

#ifndef	INTCOMP

static char *
checkDateFormat( datefmt )
register char *datefmt;
{
	if ( *datefmt++ != '%' ) {
		return( NULL );
	}
	if (( *datefmt == '/' ) ||
	    ( *datefmt == '-' ) ||
	    ( *datefmt == '.' ) ||
	    ( *datefmt == '#' )) {
		++datefmt;
	}
	if ( *datefmt == 'D' ) {
		++datefmt;
	} else {
		return( NULL );
	}
	if ( ( *datefmt == '\0' ) || ( *datefmt == ':' ) ) {
		return( datefmt );	/* print Date without Time */
	}
	if (( *datefmt == ' ' ) || ( *datefmt == '\t' )) {
		++datefmt;
	} else if (( datefmt[0] == '%' ) && ( datefmt[1] == 't' )) {
		datefmt += 2;
	}
	if ( *datefmt != '%' ) {
		return( NULL );
	} else {
		++datefmt;
	}
	if (( *datefmt == ':' ) || ( *datefmt == '.' ) || ( *datefmt == '#' )) {
		++datefmt;
	}
	if ( *datefmt != 'T' ) {
		return( NULL );
	} else {
		++datefmt;
	}
	if ( ( *datefmt == '\0' ) || ( *datefmt == ':' ) ) {
		return( datefmt );	/* print Date and Time */
	}
	return( NULL );
}

static char *
formatDate( datefmt, datebuf )
register char *datefmt;
char	*datebuf;
{
	char termdchar;
	char termDchar;
	char *termDstr;
	char termtchar;
	char termTchar;
	char yearstring[32];

	if ( *datefmt++ != '%' ) {
		return( NULL );
	}

	/*
	 * If the input date did not contain the century digits in the year
	 * then check if a specific century was requested instead of the one
	 * provided by cnvtdate() that is calculated using a sliding window
	 * based on the current year.
	 */
	if ( CC == 0 ) {
		sprintf( yearstring, "%04d", *dt_year );
	} else if ( CC < 0 ) {
		sprintf( yearstring, "%02d", *dt_year % 100 );
	} else {
		if ( *dt_add2year ) {
			sprintf( yearstring, "%02d%02d", CC, *dt_year % 100 );
		} else {
			sprintf( yearstring, "%04d", *dt_year );
		}
	}

	switch ( *datefmt ) {
	case '#':
		datefmt += 2;
		if ( ( *datefmt ) && ( *datefmt != ':' ) )
		{
			if ( *datefmt != '%' ) {
				if ( *datefmt == ' ' ) {
					termDstr = " ";
				} else {
					termDstr = "\t";
				}
				datefmt += 2;
			} else {
				datefmt += 1;
				if ( *datefmt == 't' ) {
					termDstr = "\t";
					datefmt += 2;
				} else {
					if (( *datefmt == 'T' ) || ( *datefmt == '#' )) {
						termDstr = "\0\0";
					} else {
						termDstr = " ";
					}
				}
			}
			switch ( *datefmt ) {
			case ':':
			case '.':
				if ( SS ) {
					sprintf( datebuf, "%s%02d%02d%s%02d%c%02d%c%02d",
						yearstring, *dt_month, *dt_day, termDstr,
						*dt_hour, *datefmt, *dt_minute, *datefmt, *dt_second );
				} else {
					sprintf( datebuf, "%s%02d%02d%s%02d%c%02d",
						yearstring, *dt_month, *dt_day, termDstr,
						*dt_hour, *datefmt, *dt_minute );
				}
				break;
			case '#':
			default:
				sprintf( datebuf, "%s%02d%02d%s%02d%02d%02d",
					yearstring, *dt_month, *dt_day, termDstr,
					*dt_hour, *dt_minute, *dt_second );
			}
		} else {
			sprintf( datebuf, "%s%02d%02d", yearstring, *dt_month, *dt_day );
		}
		break;
	case '.':
		datefmt += 2;
		if ( ( *datefmt ) && ( *datefmt != ':' ) )
		{
			if ( *datefmt != '%' ) {
				if ( *datefmt == ' ' ) {
					termDchar = ' ';
				} else {
					termDchar = '\t';
				}
				datefmt += 2;
			} else {
				datefmt += 1;
				if ( *datefmt == 't' ) {
					termDchar = '\t';
					datefmt += 2;
				} else {
					termDchar = ' ';
				}
			}
			switch ( *datefmt ) {
			case '#':
				sprintf( datebuf, "%02d.%02d.%s%c%02d%02d%02d",
					*dt_day, *dt_month, yearstring, termDchar,
					*dt_hour, *dt_minute, *dt_second );
				break;
			case ':':
			case '.':
			default:
				if ( *datefmt == ':' ) {
					termtchar = ':';
				} else {
					termtchar = '.';
				}
				if ( SS ) {
					termTchar = termtchar;
				} else {
					termTchar = '\0';
				}
				sprintf( datebuf, "%02d.%02d.%s%c%02d%c%02d%c%02d",
					*dt_day, *dt_month, yearstring, termDchar,
						*dt_hour, termtchar, *dt_minute,
						termTchar, *dt_second );
			}
		} else {
			sprintf( datebuf, "%02d.%02d.%s", *dt_day, *dt_month, yearstring );
		}
		break;
	case '-':
	case '/':
	case 'D':
		if ( *datefmt != 'D' ) {
			termdchar = *datefmt;
			datefmt += 2;
		} else {
			termdchar = '/';
			datefmt += 1;
		}
		if ( ( *datefmt ) && ( *datefmt != ':' ) )
		{
			if ( *datefmt != '%' ) {
				if ( *datefmt == ' ' ) {
					termDchar = ' ';
				} else {
					termDchar = '\t';
				}
				datefmt += 2;
			} else {
				datefmt += 1;
				if ( *datefmt == 't' ) {
					termDchar = '\t';
					datefmt += 2;
				} else {
					termDchar = ' ';
				}
			}
			switch ( *datefmt ) {
			case '#':
				sprintf( datebuf, "%02d%c%02d%c%s%c%02d%02d%02d",
					*dt_month, termdchar, *dt_day,
					termdchar, yearstring, termDchar,
					*dt_hour, *dt_minute, *dt_second );
				break;
			case ':':
			case '.':
			default:
				if ( *datefmt == '.' ) {
					termtchar = '.';
				} else {
					termtchar = ':';
				}
				if ( SS ) {
					termTchar = termtchar;
				} else {
					termTchar = '\0';
				}
				sprintf( datebuf, "%02d%c%02d%c%s%c%02d%c%02d%c%02d",
					*dt_month, termdchar, *dt_day, termdchar,
					yearstring, termDchar, *dt_hour, termtchar,
					*dt_minute, termTchar, *dt_second );
			}
		} else {
			sprintf( datebuf, "%02d%c%02d%c%s",
				*dt_month, termdchar, *dt_day, termdchar, yearstring );
		}
		break;
	case 's':
	default:
		return( NULL );
	}

	return( datebuf );
}

#endif


/*
 * copy string s2 to s1 in upper case
 */
char *
strcpyupper( s1, s2 )
register char *s1;
register char *s2;
{
	register char ch;
	char	*s0;

	s0 = s1;

	while ( *s2 ) {
		ch = *s2++;
		if ( islower( ch ) )
			ch = toupper( ch );
		*s1++ = ch;
	}
	*s1 = '\0';

	return( s0 );
}

/*
 * copy string s2 to s1 in lower case
 */
char *
strcpylower( s1, s2 )
register char *s1;
register char *s2;
{
	register char ch;
	char	*s0;

	s0 = s1;

	while ( *s2 ) {
		ch = *s2++;
		if ( isupper( ch ) )
			ch = tolower( ch );
		*s1++ = ch;
	}
	*s1 = '\0';

	return( s0 );
}


nprintf(argc, argv)
register int argc;
register char *argv[];
{       
	int	inptr, attrptr, whereptr, intoptr, onto, update;
	int	i, j, nattr1, nattr2, updated, printed, recordnr;
	int	endf1, printall, coption, noption, qoption;
	int	packed;
	int	size_newrec;
	RETSIGTYPE	(*istat)(), (*qstat)(), (*hstat)(), (*tstat)();
	char	dtable2[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char    c, Dtable1[MAXPATH+4], dir[MAXPATH+4], *prog;
	char	**where, newline = '\0', *Ioption, *Ooption;
	char	bmodifier[MAXATT], conversion[MAXATT], *format[MAXATT];
	char	*oldval, *tblname;
	off_t	size_newtable;
	struct	stat statbuf;
	struct  fmt xx[MAXATT];
#ifdef	INTCOMP
	int	number;
#else
	int	warnlimit;
	double	number;
	char	*optarg;
#endif
	char	newvalue[MAXREC+1];	/* keep at end in case of sprintf(3C) overflow */

	prog = strrchr(argv[0], '/');
	if (prog == NULL) {
		prog = argv[0];			/* program name */
	} else {
		++prog;
	}
	Ooption = Ioption = table2 = NULL;
	Dtable2[0] = '\0';
	DtmpItable[0] = '\0';
	qoption = printall = coption = onto = noption = printed = updated = endf1 = 0;
	packed = 0;
#ifndef	INTCOMP
	SS = TRUE;	/* include seconds when printing date and time */
	warnlimit = 10;	/* default number of date error warnings to be printed */
#endif
	tmptable[0] = lockfile[0] = '\0';
	exitcode = 1;

	while (  argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		switch(argv[1][1]) {
		case 'I':
			Ioption = &argv[1][2];
			break;
		case 'O':
			Ooption = &argv[1][2];
			break;
#ifndef	INTCOMP
		case 'C':
		case 'S':
		case 'W':
#endif
		case 'a':
		case 'c':
		case 'n':
		case 'q':
			for (i = 1; argv[1][i]; i++) {

				switch (argv[1][i]) {
#ifndef	INTCOMP
				case 'C':
					/*
					 * Century digits.
					 */
					if (argv[1][++i]) {
						optarg = &argv[1][i];
					} else if (argc > 2) {
						--argc;
						++argv;
						i = 0;
						optarg = argv[1];
					} else {
						optarg = NULL;
					}
					if (optarg != NULL) {
						if ((optarg[0] == '-') && (optarg[1] == '\0')) {
							/*
							 * Remove century digits
							 */
							CC = -1;
						} else if (isdigit(*optarg)) {
							CC = atoi(optarg);
							if ((CC < 0) || (CC > 99)) {
								optarg = NULL;
							}
						} else {
							optarg = NULL;
						}
					}
					if ( optarg == NULL ) {
						error(E_GENERAL,
							"%s: Parameter to -C option must be number between 0 and 99 or just '-'\n",
							prog );
						return(exitcode);
					} else {
						/* increment i to last char of optarg */
						while (argv[1][i+1])
							++i;
					}
					break;
				case 'S':
					/*
					 * Do not include seconds
					 * when printing date and time.
					 */
					SS = FALSE;
					break;
				case 'W':
					/*
					 * Date error warning limit
					 */
					if (argv[1][++i]) {
						optarg = &argv[1][i];
					} else if (argc > 2) {
						--argc;
						++argv;
						i = 0;
						optarg = argv[1];
					} else {
						optarg = NULL;
					}
					if (optarg != NULL) {
						if ((isdigit(*optarg)) ||
						   ((optarg[0] == '-') &&
						    (isdigit(optarg[1])))) {
							warnlimit = atoi(optarg);
						} else {
							optarg = NULL;
						}
					}
					if ( optarg == NULL ) {
						error(E_GENERAL,
							"%s: Invalid date warning limit for -W option.\n",
							prog );
						return(exitcode);
					} else {
						/* increment i to last char of optarg */
						while (argv[1][i+1])
							++i;
					}
					break;
#endif
				case 'a':
					printall = 1;	/* print all records */
					break;
				case 'c':
					coption = 1;	/* create description in the output data stream */
					break;
				case 'n':
					noption = 1;	/* reformat null fields */
					break;
				case 'q':
					qoption = 1;	/* don't print number of records */
					break;
				default:
					error(E_BADFLAG, prog, &argv[1][i]);
					return(exitcode);
					break;
				}
			}
			break;
		default:
			error(E_BADFLAG, prog, argv[1]);
			return(exitcode);
		}
		argc--;
		argv++;
	}

	update = whereptr = inptr = 0;

	for(i=1;i<argc;i++)
		if(strcmp(argv[i],"in") == 0) {
			/*
			 * make sure that this was not the last argv
			 * since "in" must be followed by a filename.
			 */
			if (argc > i + 1) {
				inptr = i;
			}
			break;
		}
	for(i=1;i<argc;i++)
		if(strcmp(argv[i],"where") == 0) {
			whereptr = i;
			break;
		}
	for(i=1,intoptr = 0;i < argc; i++) /* find "into" if there is one*/
		if(strcmp(argv[i],"into") == 0 ||
		   strcmp(argv[i],"onto") == 0) {
			intoptr = i;
			if(argv[i][0] == 'o')
				onto = 1;
			break;
		}
	if ((intoptr == 0) && (inptr) && (strcmp(argv[inptr+1],"-") != 0))
		update = 1;	/* doing an update on table using locks, etc. */

	attrptr = inptr;

	if (!((inptr > 1) &&
	     ((intoptr == 0 && ((whereptr == 0 && inptr == argc-2)
			|| (whereptr == inptr + 2))) ||
	      (intoptr == inptr+2 && ((whereptr == 0 && inptr == argc-4)
			|| (whereptr == inptr + 4))))))
	{
#ifdef	INTCOMP
		error(E_GENERAL, "Usage: %s %s \\\n\t%s \\\n\t%s\n\n", prog,
			"[-a] [-c] [-n] [-q] [-Itable] [-Otable]",
			"aname1 [:modifier] [%format] [anameN ...]",
			"in table1 [into table2] [where clause]");
#else
		error(E_GENERAL, "Usage: %s %s \\\n\t%s \\\n\t%s \\\n\t%s\n\n", prog,
			"[-a] [-c] [-n] [-q] [-Itable] [-Otable]",
			"[-C <CenturyDigits>] [-S] [-W <DateWarningLimit>]",
			"aname1 [:modifier] [%format] [anameN ...]",
			"in table1 [into table2] [where clause]");
#endif
		prtmodlistmsg();
		return(exitcode);
	}

	if ((Ooption) && (!(intoptr))) {
		error(E_GENERAL,
			"%s: Missing \"into table\" clause for the \"-O%s\" option.\n",
			prog, Ooption);
		return(exitcode);
	}

	if (whereptr == 0)	/* make sure whereptr set ok for getcond() */
		if (intoptr == 0)
			whereptr = inptr + 2;
		else
			whereptr = inptr + 4;

	if (update) {/* get ready for update in place */
		strcpy(dir, argv[inptr+1]);
		dirname(dir);           /* get directory name of table */
		if(chkaccess(dir,2)!=0) { /* check write permission of directory */
			error(E_DIRWRITE,prog,dir);
			return(exitcode);
		}
		++printall;	/* do not delete any records */
	}
	if (strcmp(argv[inptr+1],"-") != 0) {
		if((utfp1 = fopen(argv[inptr+1],"r")) == NULL) {
			if ((intoptr == 0) ||
			    (strcmp(argv[inptr+1],argv[intoptr+1]) == 0) ||
			   ((utfp1 = packedopen(argv[inptr+1])) == NULL)) {
				/* check read perm of table */
				error(E_DATFOPEN,prog,argv[inptr+1]);
				return(exitcode);
			}
			++packed;	/* reading packed table */
		}
	} else {
		if(!Ioption) {
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

	if((nattr1 = mkntbl(prog,argv[inptr+1],Dtable1,xx,Ioption)) == ERR) {
		if (packed) {
			/* EOF == FALSE */
			(void)packedclose(utfp1,FALSE);
			utfp1 = NULL;
		}
		return(exitcode);
	}
	if (xx[nattr1 - 1].flag == WN)
		endf1 = 1;

	/* Initialize attribute input base modifier and conversion type arrays */
	for (i=0; i<nattr1;i++) {
		bmodifier[i] = 0;
		conversion[i] = 0;
	}

	/* Process the anames */
	for (i=1, nattr2=0; i<attrptr; nattr2++)
	{
		char	c, *p, *m_ptr, *f_ptr;
		enum prtmodifier	p_mod;
		int	conv_code;	/* %format conversion char */

		if(nattr2 >= MAXATT) {
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

		p = argv[i];

		/*
		 * check if :modifier or %format follows attribute name
		 */
		while (*p != 0 && *p != ':' && *p != '%')
			++p;

		/* terminate attribute name with EOS and lookup attribute index */
		c = *p;
		if (c) *p = '\0';
		j = setnum(xx,argv[i],nattr1);
		if (c) *p = c;

		if (j == ERR) {
			error(E_ILLATTR, prog, argv[i], Dtable1);
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}

		if (conversion[j]) {
			error(E_GENERAL,
				"%s: Non-unique attribute name [%s]\n",
				prog, xx[j].aname);
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
				utfp1 = NULL;
			}
			return(exitcode);
		}

		conv_code = '\0';
		f_ptr = NULL;
		m_ptr = NULL;

		if (*p == 0) p = argv[++i];

		while (*p == ':' || *p == '%') {
			if (*p == '%') {
				if (f_ptr != NULL) {
					error(E_GENERAL,
						"%s: Duplicate attribute [%s] format string [%s]\n",
						prog, xx[j].aname, p);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}

#ifdef	INTCOMP
				f_ptr = p++;	/* start of %format string */
#else
				f_ptr = p;	/* start of %format string */
				if ((p = checkDateFormat(f_ptr)) != NULL ) {
					conv_code = 'D';
				} else {
					p = f_ptr + 1;
				}
#endif
				while (*p != 0 && *p != '%' && *p != ':')
					++p;

				/* check if %format string terminated with '\0' */
				if (*p) {
					if (*p == '%') {
						error(E_GENERAL,
							"%s: Duplicate or invalid attribute [%s] format string [%s]\n",
							prog, xx[j].aname, f_ptr);
						if (packed) {
							/* EOF == FALSE */
							(void)packedclose(utfp1,FALSE);
							utfp1 = NULL;
						}
						return(exitcode);
					}
					if (m_ptr != NULL) {
						error(E_GENERAL,
							"%s: Duplicate attribute [%s] modifier [%s]\n",
							prog, xx[j].aname, p);
						if (packed) {
							/* EOF == FALSE */
							(void)packedclose(utfp1,FALSE);
							utfp1 = NULL;
						}
						return(exitcode);
					}
					/*
					 * terminate %format string by zapping
					 * the ':' in the modifier string which
					 * does not need to be saved and advance
					 * the pointer
					 */
					*p++ = 0;
					m_ptr = p;	/* start of :modifier w/o the ':' */
				}
			} else {
				/*
				 *	*p == ':'
				 */
				if (m_ptr != NULL) {
					error(E_GENERAL,
						"%s: Duplicate attribute [%s] modifier [%s]\n",
						prog, xx[j].aname, p);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
				m_ptr = ++p;	/* start of :modifier w/o the ':' */
			}

			while (*p != 0 && *p != '%' && *p != ':')
					++p;

			if (*p == '\0') p = argv[++i];	/* update pointer and loop count */
		}

		if (m_ptr != NULL) {
			p_mod = getprintmod(m_ptr);
			switch (p_mod) {
			case PRT_M_BINARY:
				bmodifier[j] = 2;
				break;
			case PRT_M_OCTAL:
				bmodifier[j] = 8;
				break;
			case PRT_M_HEX:
				bmodifier[j] = 16;
				break;
			case PRT_M_NUMERIC:
				bmodifier[j] = 0;
				if (f_ptr == NULL) {
#ifdef	INTCOMP
					conversion[j] = 'd';
					format[j] = "%d";
#else
					conversion[j] = 'g';
					format[j] = "%g";
#endif
				}
				break;
			case PRT_M_STRING:
				bmodifier[j] = 0;
				break;
			case PRT_M_TOLOWER:
			case PRT_M_TRLOWER:
			case PRT_M_LTRLOWER:
			case PRT_M_RTRLOWER:
			case PRT_M_TOUPPER:
			case PRT_M_TRUPPER:
			case PRT_M_LTRUPPER:
			case PRT_M_RTRUPPER:
			case PRT_M_TRIM:
			case PRT_M_LTRIM:
			case PRT_M_RTRIM:
				bmodifier[j] = (char) p_mod;
				if (f_ptr == NULL) {
					conversion[j] = 's';
					format[j] = "%s";
				}
				break;
#ifndef	INTCOMP
			case PRT_M_DATE:
				if (f_ptr == NULL) {
					conversion[j] = 'D';
					format[j] = "%s";
				}
				break;
#endif
			case PRT_M_NULL:
			default:
				error(E_GENERAL,
				"%s: Invalid attribute [%s] modifier [:%s]\n",
					prog, xx[j].aname, m_ptr);
				prtmodlistmsg();
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
		} else {
			p_mod = PRT_M_NULL;
		}

		if (f_ptr != NULL)
		{
			/* check print format */
#ifndef	INTCOMP
			if (conv_code != 'D')
#endif
			conv_code = fmtchk(f_ptr);

			if (conv_code == -1) {
				error(E_GENERAL,"%s: bad attribute [%s] format [%s]\n",
					prog, xx[j].aname, f_ptr);
				error(E_GENERAL,
#ifdef INTCOMP
				"Must be of form: %[+ -#]*[0-9]*\\.{0,1}[0-9]*[bdiouxXs]\n");
#else
				"Must be of form: %[+ -#]*[0-9]*\\.{0,1}[0-9]*[bdiouxXeEfgGs]\n");
				error(E_GENERAL, "   or date time: %[/.-#]D[%t]%[:.#]T\n");
				error(E_GENERAL, "   or date only: %[/.-#]D\n");
#endif
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			} else {
				/* check for input base modifier and output format conflict */
				if (m_ptr != NULL)
				{
					c = 0;

					switch (p_mod) {
					case PRT_M_STRING:
					case PRT_M_TOLOWER:
					case PRT_M_TRLOWER:
					case PRT_M_LTRLOWER:
					case PRT_M_RTRLOWER:
					case PRT_M_TOUPPER:
					case PRT_M_TRUPPER:
					case PRT_M_LTRUPPER:
					case PRT_M_RTRUPPER:
					case PRT_M_TRIM:
					case PRT_M_LTRIM:
					case PRT_M_RTRIM:
						if (conv_code != 's') {
							++c;
						}
						break;
#ifndef	INTCOMP
					case PRT_M_DATE:
						if (conv_code == 's') {
							if (strcmp(f_ptr,"%s") == 0) {
								conv_code = 'D';
							} else {
								++c;
							}
						} else if (conv_code != 'D') {
							++c;
						}
					break;
#endif
					case PRT_M_NUMERIC:
					case PRT_M_BINARY:
					case PRT_M_OCTAL:
					case PRT_M_HEX:
						if ((conv_code == 's') || (conv_code == 'D')) {
							++c;
						}
						break;
					default:
						++c;
					}

					if (c) {
						error(E_GENERAL,
							"%s: format string [%s] not valid with attribute [%s] modifier [:%s]\n",
							prog, f_ptr, xx[j].aname, m_ptr);
						if (packed) {
							/* EOF == FALSE */
							(void)packedclose(utfp1,FALSE);
							utfp1 = NULL;
						}
						return(exitcode);
					}
				}
			}

			/* save format string and conversion code */
			format[j] = f_ptr;
			conversion[j] = conv_code;

		} else {
			switch (p_mod) {
			case PRT_M_NUMERIC:
			case PRT_M_BINARY:
			case PRT_M_OCTAL:
			case PRT_M_HEX:
#ifdef	INTCOMP
				conversion[j] = 'd';
				format[j] = "%d";
#else
				conversion[j] = 'g';
				format[j] = "%g";
#endif
				break;
			case PRT_M_STRING:
			case PRT_M_TOLOWER:
			case PRT_M_TRLOWER:
			case PRT_M_LTRLOWER:
			case PRT_M_RTRLOWER:
			case PRT_M_TOUPPER:
			case PRT_M_TRUPPER:
			case PRT_M_LTRUPPER:
			case PRT_M_RTRUPPER:
			case PRT_M_TRIM:
			case PRT_M_LTRIM:
			case PRT_M_RTRIM:
#ifndef	INTCOMP
			case PRT_M_DATE:
				if (conversion[j] == 0) {
					conversion[j] = 'D';
					format[j] = "%s";
				}
				break;
#endif
			default:
				if (conversion[j] == 0) {
					conversion[j] = 's';
					format[j] = "%s";	/* default to string (i.e., copy) */
				}
				break;
			}
		}

		if (xx[j].flag == WN) {
			/*
			 * Do not report fixed width field error
			 * if simple string copy, toupper, or tolower.
			 */
			c = 0;
			switch (p_mod) {
			case PRT_M_NULL:
			case PRT_M_STRING:
			case PRT_M_TOLOWER:
			case PRT_M_TOUPPER:
#ifndef	INTCOMP
			case PRT_M_DATE:
				if (((conversion[j] == 's') || (conversion[j] == 'D')) &&
#else
				if ((conversion[j] == 's') &&
#endif
				    (strcmp(format[j], "%s") == 0))
					break;
				/* FALLTHROUGH */
			default:
				error(E_GENERAL,
					"%s: Cannot convert fixed width attribute [%s]\n",
					prog, xx[j].aname);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
		}
	}

	/* parse where clause - build evaluation tree */
	where = &argv[whereptr];
	getcond(where,xx,nattr1,prog,Dtable1);

	if (update) {
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
		tblname = "-";			/* current output table name */
		utfp2 = stdout;
	} else {
		if (strcmp(argv[intoptr+1],"-") != 0) {
			if (chkaccess(argv[intoptr+1],00) == 0) {
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
			if (onto)
				utfp2 = fopen(argv[intoptr+1],"a");
			else
				utfp2 = fopen(argv[intoptr+1],"w");
			if (utfp2 == NULL) { 
				error(E_DATFOPEN,prog,argv[intoptr+1]);
				if (packed) {
					/* EOF == FALSE */
					(void)packedclose(utfp1,FALSE);
					utfp1 = NULL;
				}
				return(exitcode);
			}
			if ((Ooption) && (!(*Ooption))) {
				/* do not create output descriptor file */
				dtable2[0] = '\0';
			} else if ((Ooption) &&
			    (strcmp(argv[intoptr+1], Ooption) == 0)) {
				/*
				 * check for existing and/or create
				 * output descriptor in data directory
				 */
				getfile(dtable2, argv[intoptr+1], 1);
			} else {
				char *ptr;

				if (Ooption)
					ptr = Ooption;
				else
					ptr = argv[intoptr+1];
				/*
				 * Check if the output descriptor can be found
				 * using the standard search path order.
				 */
				if (getfile(dtable2, ptr, 's') == 0) {
					/*
					 * If its the same as the input descriptor file
					 * then don't create an output descriptor file.
					 */
					if (strcmpfile(Dtable1, dtable2) == 0) {
						dtable2[0] = '\0';
					} else {
						/*
						 * Not a match so make sure we check
						 * the current directory since that is
						 * the first place to be searched.
						 */
						getfile(dtable2, ptr, 0);
						if (chkaccess(dtable2, 00) != 0) {
							/*
							 * Descriptor does not exist
							 * in the current directory
							 * so create one in the same
							 * directory as the datafile
							 * if not an alternate table.
							 */
							if (!(Ooption))
								getfile(dtable2, ptr, 1);
						}
					}
				} else {
					if (Ooption)
						/* create description in current directory */
						getfile(dtable2, ptr, 0);
					else
						/* create description in data directory */
						getfile(dtable2, ptr, 1);
				}
			}
		} else {
			if ((Ooption) && (*Ooption)) {
				getfile(dtable2, Ooption, 0);
			} else {
				/* make sure dtable2 is null ("") */
				dtable2[0] = '\0';
			}
			utfp2 = stdout;
		}
		if ((dtable2[0]) &&
		    (strcmpfile(Dtable1, dtable2) != 0)) {
			if (chkaccess(dtable2, 00) == 0) {
				if(!onto) {
					error(E_EXISTS, prog, dtable2);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
			} else {
				strcpy(Dtable2,dtable2);
				if (copy(prog, Dtable1, Dtable2) != 0) {
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

#ifndef	INTCOMP
	get_dt_buffers(&dt_add2year,&dt_year,&dt_month,&dt_day,&dt_hour,&dt_minute,&dt_second);
#endif
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

		/* write out all fields on all records */

		size_newrec = 0;

		if (selct(xx, recordnr))
		{
			/* write out all fields with specified changes */
			for (i = 0; i < nattr1; i++)
			{
				oldval = xx[i].val;

				if ((conversion[i] == 0) ||
				    ((*oldval == 0) && (noption == 0))) {
					if ((j = putnrec(utfp2, &xx[i])) < 0) {
						error(E_DATFWRITE, prog, tblname);
						if (packed) {
							/* EOF == FALSE */
							(void)packedclose(utfp1,FALSE);
							utfp1 = NULL;
						}
						return(exitcode);
					}
					size_newrec += j;
					continue;
				}

				newvalue[0] = 0;
#ifndef	INTCOMP
				if (conversion[i] == 'D')
				{
					number = cnvtdate(oldval, TRUE);
					if (number == 0) {
						if (format[i][1] == 's') {
							if (warnlimit) {
								error(E_GENERAL,
									"%s: Warning: Date Error: rec# %d attribute %s: %s\n",
									prog, recordnr, xx[i].aname, oldval);
								--warnlimit;
							}
						}
						else if ((*dt_year == 0) ||
							 (*dt_month == 0) ||
							 (*dt_day == 0) ||
							 (strchr(format[i],'T')))
						{
							error(E_GENERAL,
								"%s: Date Error: rec# %d attribute %s: %s\n",
								prog, recordnr, xx[i].aname, oldval);
							if (packed) {
								/* EOF == FALSE */
								(void) packedclose(utfp1,FALSE);
								utfp1 = NULL;
							}
							return(exitcode);
						}
					}
				}
				else
#endif
				if (conversion[i] != 's') {	/* do input base conversion if needed */
					/*
					 * Initialize number to the logical default
					 * value for the case that the input string
					 * is null or in case the input conversion
					 * functions do not assign a value to number.
					 */
					number = 0;

					if (*oldval) {
						if (bmodifier[i]) {
#ifdef	__STDC__
							number = (int) strtoul(oldval, (char **)NULL, bmodifier[i]);
#else
							number = strtol(oldval, (char **)NULL, bmodifier[i]);
#endif
						} else {
#ifdef  INTCOMP
							sscanf(oldval, "%d", &number);
#else
							sscanf(oldval, "%lf", &number);
#endif
						}
					}
				}

				switch (conversion[i]) {
#ifndef	INTCOMP
				case 'D':
					/* date format */
					if (formatDate(format[i],newvalue) == NULL) {
						/*
						 * Date format was "%s"
						 * so keep old date value.
						 */
						if ((j = putnrec(utfp2, &xx[i])) < 0) {
							error(E_DATFWRITE, prog, tblname);
							if (packed) {
								/* EOF == FALSE */
								(void)packedclose(utfp1,FALSE);
								utfp1 = NULL;
							}
							return(exitcode);
						}
						size_newrec += j;
						continue;
					}
					break;
#endif
				case 'b':
					/* binary format */
					sprintfb(newvalue, format[i], (unsigned) number);
					break;
				case 'd':
				case 'i':
					/* integer format */
					sprintf(newvalue, format[i], (int) number);
					break;
				case 'o':
				case 'u':
				case 'x':
				case 'X':
					/* unsigned integer format */
					sprintf(newvalue, format[i], (unsigned) number);
					break;
				case 's':
					{
						register char *p, *q;
						enum prtmodifier p_mod;
						char	tobuf[MAXREC+1];

						p = oldval;

						switch ((p_mod = (enum prtmodifier) bmodifier[i])) {
						case PRT_M_NULL:
							sprintf(newvalue, format[i], p);
							break;
						case PRT_M_TOLOWER:
							p = strcpylower(tobuf, p);
							sprintf(newvalue, format[i], p);
							break;
						case PRT_M_TOUPPER:
							p = strcpyupper(tobuf, p);
							sprintf(newvalue, format[i], p);
							break;
						case PRT_M_LTRIM:
						case PRT_M_LTRLOWER:
						case PRT_M_LTRUPPER:
							while (*p == ' ')
								++p;
							switch (p_mod) {
							case PRT_M_LTRLOWER:
								p = strcpylower(tobuf, p);
								break;
							case PRT_M_LTRUPPER:
								p = strcpyupper(tobuf, p);
								break;
							}
							sprintf(newvalue, format[i], p);
							break;
						case PRT_M_TRIM:
						case PRT_M_TRLOWER:
						case PRT_M_TRUPPER:
							while (*p == ' ')
								++p;
							/* FALLTHROUGH */
						case PRT_M_RTRIM:
						case PRT_M_RTRLOWER:
						case PRT_M_RTRUPPER:
							j = strlen(p);
							if (j) {
								q = p + j - 1;
								while ( ( q >= p ) && ( *q == ' ' ) )
									--q;
								++q;
								if ( j > q - p ) {
									*q = '\0';
								} else {
									q = NULL;
								}
							} else {
								q = NULL;
							} 
							switch (p_mod) {
							case PRT_M_TRLOWER:
							case PRT_M_LTRLOWER:
								p = strcpylower(tobuf, p);
								break;
							case PRT_M_TRUPPER:
							case PRT_M_LTRUPPER:
								p = strcpyupper(tobuf, p);
								break;
							}
							sprintf(newvalue, format[i], p);
							if (q != NULL) {
								*q = ' ';
							}
							break;
						default:
							sprintf(newvalue, format[i], p);
							break;
						}
					}
					break;
				default:	/* floating point format */
					sprintf(newvalue, format[i], number);
					break;
				}

				xx[i].val = newvalue;
				if ((j = putnrec(utfp2, &xx[i])) < 0) {
					error(E_DATFWRITE, prog, tblname);
					if (packed) {
						/* EOF == FALSE */
						(void)packedclose(utfp1,FALSE);
						utfp1 = NULL;
					}
					return(exitcode);
				}
				size_newrec += j;
				xx[i].val = oldval;
			} 
			++updated;
		} else {
			if (printall) {
				/* write out all fields without any changes */
				for (i = 0; i < nattr1; i++) {
					if ((j = putnrec(utfp2, &xx[i])) < 0) {
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
				++printed;
			}
		}

		if (endf1 == 1) {
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
	if (utfp2 != stdout) {
		if (fclose(utfp2) == EOF) {
			error(E_DATFWRITE, prog, tblname);
			return(exitcode);
		}
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
	{
		if (update) {
			error(E_GENERAL,"%s: %d records updated in %s\n",
				prog, updated, argv[inptr+1]);
		} else {
			if (printed == 0) {
				error(E_GENERAL,"%s: %d records printed in %s\n",
					prog, updated, argv[inptr+1]);
			} else {
				error(E_GENERAL,"%s: %d records updated out of %d records printed in %s\n",
					prog, updated, (printed + updated), argv[inptr+1]);
			}
		}
	}
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

static int
fmtchk(format)
char	*format;
{
	register int		i;
	register char		*p;
	int			dot;
	int			minus;
	int			zero, leadzero;
	int			precision;
	int			width;
	int			fmterr;
	char			conversion;

	/* output format for computed field
#ifdef INTCOMP
	   must be of form %[+ -#]*[0-9]*\.{0,1}[0-9]*[bdiouxXs]
#else
	   must be of form %[+ -#]*[0-9]*\.{0,1}[0-9]*[bdiouxXeEfgGs]
#endif
	*/

	conversion = 0;

	precision = 1;	/* default precision */

	dot = minus = zero = leadzero = width = 0;

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
			if (dot) {
				++fmterr;
			}
			++dot;
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
		case 's':
			if (*p != 0) {
				++fmterr;
			}
			if (fmterr == 0) {
				conversion = i;
			}
			break;
		case '0':
			if ((!(dot)) && (!(minus))) {
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
					if (dot) {
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
