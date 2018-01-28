/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "uquery.h"
#include "message.h"
#include "permission.h"

/*
 * Definitions for saying what the next type of clause (output
 * redirection or where conditions) is on the command line.
 */
#define CL_WHERE	1	/* where clause is next on cmd line */
#define CL_OUTPUT	2	/* redirect output is next on cmd line */
#define CL_SORT		3	/* list of attributes to sort on */

extern struct uquery *fmkquery();
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
extern char *calloc();
#endif
extern char *basename();
extern void setunpackenv();

RETSIGTYPE catch_sig();

#define MAXCMDLINE	8192

#define TABLECMD	"prtable"
#define BLOCKCMD	"prblock"
#define PRINTFCMD	"qprintf"
#define FLDDELIM	'\007'		/* default attribute delimiter */

extern char uversion[];			/* version string */

extern int  uqpnvalue;		/* set to QP_NEWVALUE to allow "value=" attribute modifier */
extern int  uchecktuple;	/* if set readtuple() will count/report tuple errors */
extern long utplerrors; 	/* no. of tuples read with tuple errors */
extern long utplelimit; 	/* limit on number of tuple errors reported */
extern int  utplmsgtype;	/* MSG_ERROR or MSG_WARN type tuple error message */

char *prog;

struct uinsertinfo *insptr;		/* insert structure for insertions */
struct urelation relinfo;		/* relation infor for insertions */
FILE *output;				/* file output in not inserting */
int recordlimit;			/* number of records to be retrieved when set */
int tplcnt;				/* number of tuples retrieved */
int delimcnt;				/* number of displayed attrs */

char prargs[MAXCMDLINE];		/* formatting options */
char *prargptr;

short quiet = FALSE;
short doformat = FALSE;
short create_descrwtbl = FALSE;	/* put description with output table? */
short chk_existence = FALSE;	/* check if tuple exists, but don't print it */

char *formatcmd = NULL;
char *formatstr = NULL;
char flddelim = '\0';
char flddelim_set = FALSE;
short delimiteropt = 0;
char termdelimiter[256];
short notdelimiter = FALSE;
short widthdelimiter = FALSE;

main( argc, argv )
int argc;
char *argv[];
{
	register int i, cnt;
	int clause;		/* what is next clause on cmd line? */
	char *mode;		/* open mode for output */
	register int curarg;
	char *outfile;		/* output file */
	char *outdescr;		/* output file descriptor */
	char *equalsign;	/* ptr to "=<alt_table>" part of output file */
	int maxattr;
	char **relnames;
	int relcnt;		/* relation count */
	struct uquery *query;	/* compiled version of query */
	struct qresult result;
	int attrcnt;		/* attribute count */
	char **attrnames;	/* attr names as arguments */
	int sortcnt;		/* sort attribute count */
	char **sortnames;	/* sort attribute names */
	char queryflags = 0;	/* flags for query */
	int exit_code;		/* return code */

	prog = basename( *argv );	/* save the program name */

	/*
	 * Parse any command-line options given.
	 */
	if ( argc < 2 )
		usage( );

	prargptr = prargs;
	for( curarg = 1; curarg < argc; curarg++ )
	{
		register char *option;

		option = argv[curarg];

		/*
		 * Look for a non-flag or the standard input
		 * file name "-" or "-=<relation>".  This will
		 * signal the end of flag parsing.
		 */
		if ( *option != '-' || option[1] == '\0' ||
			option[1] == '=' )
		{
#ifdef DEBUG
			if ( strncmp( option, "debug=", 6 ) == 0 )
			{
				(void)set_qdebug( &option[6] );
				continue;
			}
#endif /* DEBUG */
			break;
		}

		while( option && *++option )
		{
			switch( *option )
			{
			case 'a':	/* format flag option */
			case 'b':
			case 'N':
			case 'p':
			case 'P':
			case 'R':
			case 't':
			case 'U':
				*prargptr++ = ' ';
				*prargptr++ = '-';
				*prargptr++ = *option;
				doformat = TRUE;
				break;
			case 'e':
			case 'w':
			case 'W':
				doformat = TRUE;
				*prargptr++ = ' ';
				*prargptr++ = '-';
				*prargptr++ = *option;
				if ( ( option[1] ) && ( isdigit( option[1] ) ) ) {
					++option;
				} else if ( ( ! option[1] ) && ( curarg < argc - 2 ) &&
				   ( isdigit( argv[curarg+1][0] ) ) &&
				   ( strchr( argv[curarg+1], '.' ) == NULL ) ) {
					option = argv[++curarg];
				} else {
					break;
				}
				strcpy( prargptr, option );
				prargptr += strlen( prargptr );
				option = NULL;
				break;
			/*
			 * Below are options the where rest of arg or next
			 * is passed
			 */
			case 'l': /* map to upper case to avoid ambiguities */
				*option = 'L';
				/* fall thru here */
			case 'D':
			case 'C':
			case 'f':
			case 'h':
			case 'L':
			case 'm':
			case 'n':
			case 'o':
			case 'S':
				doformat = TRUE;
				*prargptr++ = ' ';
				*prargptr++ = '-';
				*prargptr++ = *option;
				if ( option[1] )
					option++;
				else if ( curarg < argc - 2 )
					option = argv[++curarg];
				else
					break;
				if ( strchr( option, '\'' ) == NULL )
					i = '\'';	/* use single quote */
				else if ( strchr( option, '"' ) == NULL )
					i = '"';	/* use double quote */
				else		/* can't use either quote */
					i = 0;
				if ( i )	/* quote the arg */
					*prargptr++ = (char)i;
				strcpy( prargptr, option );
				prargptr += strlen( prargptr );
				if ( i )
					*prargptr++ = (char)i;
				option = NULL;
				break;
			case 'A':		/* print output in raw form */
				formatcmd = NULL;
				formatstr = NULL;
				doformat = FALSE;
				break;
			case 'B':		/* format output as block */
				formatcmd = BLOCKCMD;
				formatstr = NULL;
				doformat = TRUE;
				break;
			case 'c':
				create_descrwtbl ^= TRUE;
				break;
			case 'd':
#ifdef DEBUG
				if ( strcmp( option, "debug" ) == 0 ) {
					(void)set_qdebug( "sel,expr,join" );
					option = NULL;
				}
				else
#endif
				if ( option[1] ) {
					++delimiteropt;
					cnvtbslsh( &option[1], &option[1] );
					if ( flddelim_set ) {
						for ( i = 1; i == 1 || option[i]; i++ )
							termdelimiter[(option[i] & 0xff)] = TRUE;
					} else {
						flddelim = option[1];
						if ( option[2] == '!' )
							notdelimiter = TRUE;
						flddelim_set = TRUE;
					}
					option = NULL;
				} else {
					if ( flddelim_set ) {
						++delimiteropt;
						widthdelimiter = TRUE;
					}
				}
				break;
			case 'E':
				chk_existence ^= TRUE;
				break;
			case 'F':		/* format output as printf */
				formatcmd = PRINTFCMD;
				if ( option[1] ) {
					formatstr = &option[1];
					option = NULL;
				}
				else if ( curarg < argc - 2 )
					formatstr = argv[++curarg];
				else
					formatstr = "";
				doformat = TRUE;
				break;
			case 'i':		/* all caseless compares */
				queryflags ^= Q_NOCASECMP;
				break;
			case 'Q':		/* don't quit due to tuple errors */
				/*
				 * default to count tuple errors
				 * and make a note that we saw -Q
				 */
				utplelimit = 0;
				uchecktuple = UCHECKCOUNT;
				if ( option[1] ) {
					if ((( option[1] == '-' ) &&
					     ( isdigit( option[2] ))) ||
					    ( isdigit( option[1] ))) {

						uchecktuple = UCHECKPRINT;
						utplelimit = atoi( &option[1] );
						option = NULL;
					}
				}
				else if ( curarg < argc - 2 ) {
					char *p = argv[ curarg + 1 ];
					if ((( p[0] == '-' ) &&
					     ( isdigit( p[1] ))) ||
					    ( isdigit( p[0] ))) {
						uchecktuple = UCHECKPRINT;
						utplelimit = atoi( p );
						option = NULL;
						++curarg;
					}
				}
				break;
			case 'q':		/* don't print tuple count */
				quiet = TRUE;
				break;
			case 'r':		/* record limit */
				if ( ( option[1] ) && ( isdigit( option[1] ) ) ) {
					recordlimit = atoi( &option[1] );
					option = NULL;
				}
				else if ( ( option[1] == '\0' ) &&
				          ( curarg < argc - 2 ) &&
				          ( isdigit( *argv[ curarg + 1 ] ) ) ) {
					recordlimit = atoi( argv[ curarg + 1] );
					option = NULL;
					++curarg;
				}
				else {
					prmsg( MSG_ERROR, "'%c' option requires numeric argument",
						*option );
					usage( );
				}
				break;
			case 's':		/* sort the output */
				queryflags ^= Q_SORT;
				break;
			case 'T':		/* format output as table */
				formatcmd = TABLECMD;
				formatstr = NULL;
				doformat = TRUE;
				break;
			case 'u':
				queryflags ^= Q_UNIQUE;
				break;
			case 'V':
				prmsg( MSG_NOTE, "%s", uversion );
				exit( 0 );
			case 'v':	/* toggle: use "user-friendly" names */
				queryflags ^= Q_FRIENDLY;
				break;
			default:
				prmsg( MSG_ERROR, "unrecognized option '%c'",
					*option );
				usage( );
			}
		}
	}
	*prargptr = '\0';

	if ( ! uchecktuple ) {
		/* treat tuple errors as fatal errors */
		uchecktuple = UCHECKFATAL;
		utplmsgtype = MSG_ERROR;
		utplelimit = 1;
	} else {
		if ( ( utplelimit == 0 ) && ( quiet ) ) {
			uchecktuple = UCHECKIGNORE;
		}
		utplmsgtype = MSG_WARN;
	}
	utplerrors = 0;

	/*
	 * Set environment to handle read (unpack) of packed relations
	 * based on the UNITYUNPACK environement variable.
	 */
	setunpackenv();

	/*
	 * Allow "value=..." attribute modifier to be recognized.
	 */
	uqpnvalue = QP_NEWVALUE;

	if ( chk_existence )
		doformat = FALSE;	/* no formatting for existence */

	if ( doformat ) {
		if ( formatcmd == NULL )
			formatcmd = TABLECMD;
		if ( flddelim == '\0' ) {
			flddelim = FLDDELIM;
			flddelim_set = TRUE;
		}
	}
	else	/* unformatted ouptut - friendly names don't apply */
	{
		queryflags &= ~Q_FRIENDLY;
	}

	/*
	 * If there is a list of attributes to project, find them and 
	 * save them away for later use.
	 */
	clause = 0;
	cnt = 0;	/* key words seen after any attribute names */
	for( attrcnt = curarg; attrcnt < argc; attrcnt++ ) {
		if ( strcmp( argv[attrcnt], "unique" ) == 0 ) {
			queryflags |= Q_UNIQUE;
			cnt++;
			/* continue to look for other key words */
		}
		else if ( strcmp( argv[attrcnt], "sorted" ) == 0 ) {
			queryflags |= Q_SORT;
			cnt++;
			if ( attrcnt + 1 < argc &&
				strcmp( argv[ attrcnt + 1], "unique" ) == 0 )
			{
				queryflags |= Q_UNIQUE;
				cnt++;
				attrcnt++;
			}

			if ( attrcnt + 1 < argc &&
				strcmp( argv[ attrcnt + 1], "by" ) == 0 )
			{
				clause = CL_SORT;
				cnt++;
				attrcnt += 2;	/* skip  keywords */
				/* end of any attribute list so break out */
				break;
			}
			/* continue to look for other key words */
		}
		else if ( strcmp( argv[attrcnt], "from" ) == 0 ||
			strcmp( argv[attrcnt], "in" ) == 0 )
		{
			cnt++;
			attrcnt++;	/* skip "from" or "in" keyword */
			/* end of any attribute list so break out */
			break;
		}
		else if ( cnt > 0 ||
				strcmp( argv[attrcnt], "where" ) == 0 ||
				strcmp( argv[attrcnt], "into" ) == 0 ||
				strcmp( argv[attrcnt], "onto" ) == 0 )
			break;
	}

	if ( cnt != 0 ) { /* may have projected attribute names */
		int tmp;

		tmp = attrcnt;
		attrcnt -= curarg + cnt;
		attrnames = ( attrcnt > 0 ) ? &argv[curarg] : NULL;
		curarg = tmp;  /* skip attributes and keywords */

		if ( attrcnt == 1 && **attrnames == '\0' )
		{
			/*
			 * Projecting the single, null attribute.
			 * This doesn't work at the library level,
			 * so make it the same as -E.
			 */
			chk_existence = TRUE;
			attrcnt = 0;
			attrnames = NULL;
		}
	}
	else {
		/*
		 * No attribute names were given on the command line.
		 * (No "sorted", "unique", or "from" keyword was seen.)
		 * We'll recheck the same arguments again as a
		 * list of relation names.
		 */
		attrcnt = 0;
		attrnames = NULL;
	}

	/*
	 * If there is a list of sort attributes, find them and 
	 * save them away for later use.
	 */
	if ( clause == CL_SORT )
	{
		int tmp;

		cnt = 0;	/* key words seen after any attribute names */
		for( sortcnt = curarg; sortcnt < argc; sortcnt++ )
		{
			if ( strcmp( argv[sortcnt], "unique" ) == 0 ) {
				queryflags |= Q_UNIQUE;
				cnt++;
				/* continue to look for other key words */
			}
			else if ( strcmp( argv[sortcnt], "from" ) == 0 ||
				strcmp( argv[sortcnt], "in" ) == 0 )
			{
				cnt++;
				sortcnt++;	/* skip "from" or "in" keyword */
				/* end of any attribute list so break out */
				break;
			}
			else if ( cnt > 0 ||
				strcmp( argv[sortcnt], "where" ) == 0 ||
				strcmp( argv[sortcnt], "into" ) == 0 ||
				strcmp( argv[sortcnt], "onto" ) == 0 )
			{
				break;
			}
		}

		tmp = sortcnt;
		sortcnt -= curarg + cnt;
		sortnames = ( sortcnt > 0 ) ? &argv[curarg] : NULL;
		curarg = tmp;  /* skip attributes and keywords */
	}
	else
	{
		sortcnt = 0;
		sortnames = NULL;
	}

	/*
	 * Parse the list of relation names.
	 */
	clause = 0;
	for( relcnt = curarg; relcnt < argc; relcnt++ ) {
		if ( strcmp( argv[relcnt], "where" ) == 0 ) {
			clause = CL_WHERE;
			break;
		}
		else if ( strcmp( argv[relcnt], "into" ) == 0 ) {
			clause = CL_OUTPUT;
			mode = "w";
			break;
		}
		else if ( strcmp( argv[relcnt], "onto" ) == 0 ) {
			clause = CL_OUTPUT;
			mode = "a";
			break;
		}
	}
	relcnt -= curarg;
	if ( relcnt == 0 ) {	/* Gotta have a relation */
		prmsg( MSG_ERROR, "no relations given on command line" );
		usage( );
	}
	relnames = &argv[curarg];
	curarg += relcnt;	/* skip over relation names */
	if ( clause )
		curarg++;		/* skip over keyword */

	/*
	 * If the next clause is for output redirection, save the output file.
	 */
	if ( clause == CL_OUTPUT && ! chk_existence )
	{
		outfile = argv[curarg++];
		outdescr = equalsign = strchr( outfile, '=' );
		if ( outdescr )
			*outdescr++ = '\0';	/* get rid of '=' */
		if ( *mode == 'w' && chkperm( outfile, P_EXIST ) )
		{
			prmsg( MSG_ERROR, "output file '%s' already exists",
				outfile );
			exit( 2 );
		}

		/*
		 * See if there is a where-clause.
		 */
		if ( curarg < argc ) {
			if ( strcmp( argv[curarg++], "where" ) == 0 )
				clause = CL_WHERE;
			else
				usage( );	/* unrecognized cmd line */
		}
	}
	else {
		outfile = NULL;
		outdescr = NULL;
		equalsign = NULL;
	}

	if ( chk_existence )
	{
		/*
		 * Only checking existence, sorting and uniqueness
		 * are useless.  So are friendly names.
		 */
		queryflags &= ~(Q_SORT|Q_UNIQUE|Q_FRIENDLY);
		sortcnt = 0;
	}

	/*
	 * Now do the real work.  First find which tuples apply (from the
	 * where-clause), and then print out the appropriate information.
	 */
	query = fmkquery( queryflags, relnames, relcnt, attrnames, attrcnt,
			sortnames, sortcnt, &argv[ curarg ], argc - curarg );
	if (  query == NULL ) {
		/* message already printed */
		exit( 1 );
	}

	/*
	 * Check permissions on each relation and see how many
	 * attributes each has.
	 */
	maxattr = 0;
	for( i = 0; i < query->nodecnt; i++ )
	{
		if ( strcmp( query->nodelist[i]->rel->path, "-" ) != 0 &&
			! chkperm( query->nodelist[i]->rel->path, P_READ,
				(unsigned short)geteuid(),
				(unsigned short)getegid() ) )
		{
			prmsg( MSG_ERROR, "cannot read file '%s'",
				query->nodelist[i]->rel->path );
			usage( );
		}
		if ( query->nodelist[i]->rel->attrcnt > maxattr )
			maxattr = query->nodelist[i]->rel->attrcnt;
	}
	(void)set_attralloc( maxattr );

	delimcnt = 0;
	for( i = query->attrcnt - 1; i >= 0; i-- )
	{
		if ( (query->attrlist[i].flags & QP_NODISPLAY) == 0 )
		{
			/*
			 * If we have a field delimiter, set the terminate
			 * character to it on all but the last projected
			 * attribute.  (The last projected attribute always
			 * has a new-line as it's terminator.)
			 */
			if ( flddelim_set ) {
				if ( delimcnt == 0 ) {
					if ( ( query->attrlist[i].attrtype == QP_FIXEDWIDTH ) &&
					   ( ( delimiteropt == 1 ) ||
					   ( ( delimiteropt >= 2 ) &&
					     ( widthdelimiter != notdelimiter ) ) ) ) {
						query->attrlist[i].attrtype = QP_TERMCHAR;
						query->attrlist[i].attrwidth = 0;
						query->attrlist[i].terminate = '\n';
					}
				} else {
					if ( delimiteropt == 0 ) {
						query->attrlist[i].terminate = (char)flddelim;
					} else if ( delimiteropt == 1 ) {
						query->attrlist[i].attrtype = QP_TERMCHAR;
						query->attrlist[i].attrwidth = 0;
						query->attrlist[i].terminate = (char)flddelim;
					} else {
						if ( query->attrlist[i].attrtype == QP_TERMCHAR ) {
							if ( termdelimiter[query->attrlist[i].terminate] != notdelimiter )
								query->attrlist[i].terminate = (char)flddelim;
						} else {
							if ( widthdelimiter != notdelimiter ) {
								query->attrlist[i].attrtype = QP_TERMCHAR;
								query->attrlist[i].attrwidth = 0;
								query->attrlist[i].terminate = (char)flddelim;
							}
						}
					}
				}
			}
			delimcnt++;
		}
	}

	if ( delimcnt != 0 )
	{
		/*
		 * We need to pass a pointer to the '=' (if present)
		 * in the original output file specified on the
		 * command line so that the entire "table[=<alt_table>]"
		 * can be passed to getrelinfo() so that it can determine
		 * whether or not the user wants the data directory to
		 * be searched first for the output description.
		 */
		set_output( query, outfile, equalsign, outdescr, mode );
	}
	else if ( query->attrcnt != 0 ) {
		prmsg( MSG_ERROR, "all projected attributes are marked 'nodisplay'" );
		usage( );
	}

	/*
	 * Catch all signals so we don't leave any DB's dangling with
	 * lock files still around.
	 */
	(void)set_sig( SIGHUP, catch_sig );
	(void)set_sig( SIGINT, catch_sig );
	(void)set_sig( SIGQUIT, catch_sig );
	(void)signal( SIGILL, catch_sig );
	(void)signal( SIGTRAP, catch_sig );
#ifdef SIGIOT
	(void)signal( SIGIOT, catch_sig );
#endif
#ifdef SIGEMT
	(void)signal( SIGEMT, catch_sig );
#endif
	(void)signal( SIGFPE, catch_sig );
	(void)signal( SIGBUS, catch_sig );
	(void)signal( SIGSEGV, catch_sig );
#ifdef SIGSYS
	(void)signal( SIGSYS, catch_sig );
#endif
	(void)signal( SIGPIPE, catch_sig );
	(void)set_sig( SIGUSR1, catch_sig );
	(void)set_sig( SIGUSR2, catch_sig );
	(void)signal( SIGALRM, SIG_IGN );

	tplcnt = 0;
	if ( ! queryeval( query, &result ) )
	{
		if ( is_uerror_set() ) {
			/*
			 * If output is being formatted then
			 * flush/terminate the output process
			 * to make it easier for the user to
			 * see the error message(s).
			 */
			if ( ( ! insptr ) && ( output ) && ( doformat ) ) {
				if (fflush( output ) != 0 ) {
					perror( prog );
					prmsg( MSG_WARN,
						"Failure writing data to [%s]",
						formatcmd );
				}
				if (pclose( output ) != 0 ) {
					prmsg( MSG_WARN, 
						"[%s] returned non-zero",
						formatcmd );
				}
			}
			(void)pruerror( );
			if ( tplcnt) {
				prmsg( MSG_ERROR,
					"query evaluation failed after retrieving %d tuple%s",
					tplcnt, tplcnt != 1 ? "s" : "" );
			} else {
				prmsg( MSG_ERROR, "query evaluation failed" );
			}
			exit( 2 );
		}
		if ( query->attrcnt == 0 || chk_existence )
			exit( 4 );	/* no tuples retrieved */
	}
	else if ( chk_existence )
		exit( 4 );	/* no tuples retrieved */
	else if ( query->attrcnt == 0 )
		exit( 0 );

	exit_code = 0;
	if ( insptr ) {
		if ( ! end_insert( insptr, TRUE ) ) {
			(void)pruerror();
			prmsg( MSG_ERROR, "cannot finish insert into %s",
				outfile );
			exit( 2 );
		}
	}
	else if ( output ) {
		if ( fflush( output ) != 0 ) {
				perror( prog );
				prmsg( MSG_WARN, "Failure writing data" );
				exit_code = 2;
		}
		if ( doformat ) {
			if ( pclose( output ) != 0 ) {
				prmsg( MSG_ERROR,
					"[%s] did not complete successfully",
					formatcmd );
				if ( exit_code == 0 )
					exit_code = 5;
			}
		}
	}

	if ( ! quiet ) {
		if ( utplerrors ) {
			prmsg( MSG_WARN, "%d tuple error%s encountered reading relation(s)",
				utplerrors, utplerrors != 1 ? "s" : "" );
		}
		prmsg( MSG_ERRNOTE, "%d record%s retrieved", tplcnt,
			tplcnt != 1 ? "s" : "" );
	}

	exit( exit_code );
}

usage( )
{
	prmsg( MSG_USAGE, "[-cEiqsuvV] [-d<delim>] [-Q[<ErrorLimit>]] [-r<RecordLimit>] \\" );
	prmsg( MSG_CONTINUE, "[-F[<format>]|-A|-B|-T] [<format_options>] \\" );
	prmsg( MSG_CONTINUE, "[<attr>[:<modifiers>...] [as <newattr>] ...] \\" );
	prmsg( MSG_CONTINUE, "[sorted [by <attr>[:<modifiers>...] ...]] [unique] \\" );
	prmsg( MSG_CONTINUE, "from <table>[=<alt_table>]... [into|onto <table>[=<alt_table>]] \\" );
	prmsg( MSG_CONTINUE, "[where <where-clause>]" );

	exit( 1 );
}

set_sig( sig, func )
int sig;
RETSIGTYPE (*func)();
{
	if ( signal( sig, SIG_IGN ) != (RETSIGTYPE (*)())SIG_IGN )
		(void)signal( sig, func );
}

RETSIGTYPE
catch_sig( sig )
int sig;
{
	(void)signal( sig, SIG_IGN );	/* ignore any more signals */

	(void)end_insert( (struct uinsertinfo *)NULL, FALSE );

	(void)signal( sig, SIG_DFL );
	kill( getpid( ), sig );

	exit( 2 );	/* if signal comes quickly we'll never get here */
}

insertattrvals( attrvals, projcnt, projptr )
char **attrvals;
int projcnt;
struct qprojtuple *projptr;
{
	if ( ( recordlimit ) && ( tplcnt >= recordlimit ) )
		return( TRUE );

	tplcnt++;

#ifndef NOTPLERROR
	/**************************************************************
	if ( ! quiet )
	{
		register int i;

		for( i = 0; i < projcnt; i++, projptr++ ) {
			if ( projptr->tplptr->flags & TPL_ERRORMSK )
			{
				prtplerror( projptr->projptr->rel->rel,
					projptr->tplptr );
			}
		}
	}
	**************************************************************/
#endif

	if ( ! do_insert( insptr, attrvals ) ) {
		(void)pruerror( );
		prmsg( MSG_ERROR, "insertion failed on record %d",
			tplcnt );
		(void)end_insert( (struct uinsertinfo *)NULL, FALSE );
		exit( 2 );
	}

	return( TRUE );
}

tuple_exist( attrvals, projcnt, projptr )
char **attrvals;
int projcnt;
struct qprojtuple *projptr;
{
	/*
	 * This function is used as the tuple function when
	 * existence is being checked.  It is only called when
	 * a tuple exists matching the where clause and projection.
	 * Since there is such a tuple, we just exit with a
	 * successful status.
	 */
	exit( 0 );
}

writeattrvals( attrvals, projcnt, projptr )
char **attrvals;
int projcnt;
struct qprojtuple *projptr;
{
	register int i;

	if ( ( recordlimit ) && ( tplcnt >= recordlimit ) )
		return( TRUE );

	/*
	 * Wait to increment tplcnt until finished
	 * processing this tuple so that if there
	 * is an error, the query evaluation failed
	 * message can correctly reflect how many
	 * tuples had been sucessfully retrieved.
	 */
	for( i = 0; i < projcnt; i++, projptr++ ) {
#ifndef NOTPLERROR
		/********************************************************
		if ( (projptr->tplptr->flags & TPL_ERRORMSK) && ! quiet )
		{
			prtplerror( projptr->projptr->rel->rel,
				projptr->tplptr );
		}
		********************************************************/
#endif
		if ( ! writeattrvalue( output, attrvals[i],
				projptr->projptr->terminate, TRUE,
				projptr->projptr->attrwidth,
				projptr->projptr->attrtype ) )
		{
			(void)pruerror();
			prmsg( MSG_ERROR, "cannot write %d attribute in tuple %d -- exiting",
				i, tplcnt + 1 );
			return( FALSE );
		}
	}
	tplcnt++;

	return( TRUE );
}

FILE *
setformat( query )
struct uquery *query;
{
	register char *bufptr;
	register int i;
	register struct qprojection *refptr;
	register char formtable;
	char cmdline[MAXCMDLINE];

	bufptr = cmdline;

	formtable = ( strcmp( formatcmd, TABLECMD ) == 0 );

	strcpy( bufptr, formatcmd );
	bufptr += strlen( bufptr );
	if ( formatstr ) {
		if ( strchr( formatstr, '\'' ) == NULL )
			sprintf( bufptr, " '%s'", formatstr );
		else if ( strchr( formatstr, '"' ) == NULL )
			sprintf( bufptr, " \"%s\"", formatstr );
		else
			sprintf( bufptr, " '%s'", formatstr );

		bufptr += strlen( bufptr );
	}

	if ( flddelim == '\'' )
		strcpy( bufptr, " -d\\'" );
	else
		sprintf( bufptr, " -d'%c'", (char) flddelim );
	bufptr += strlen( bufptr );

	sprintf( bufptr, " -F%d ", delimcnt );
	bufptr += strlen( bufptr );

	strcpy( bufptr, prargs );
	bufptr += strlen( bufptr );

	for( refptr = query->attrlist, i = 0; i < query->attrcnt;
			i++, refptr++ ) {

		if ( (refptr->flags & QP_NODISPLAY) == 0 ) {
			if ( formtable ) {
				sprintf( bufptr, " -%c%d", tolower( refptr->justify ),
					refptr->prwidth );
				bufptr += strlen( bufptr );
			}
			sprintf( bufptr, " \"%s\"", refptr->prname );
			bufptr += strlen( bufptr );
		}
	}

	return( popen( cmdline, "w" ) );
}

set_output( query, file, equalsign, descr, mode )
struct uquery *query;
char *file;
char *equalsign;
char *descr;
char *mode;
{
	register int i;
	register struct qprojection *refptr;

	if ( doformat ) {
		if ( descr )
			prmsg( MSG_WARN, "alternate table format not supported with formatted output -- ignored" );

		settplfunc( query, writeattrvals );

		if ( file ) {
			fclose( stdout );
			output = fopen( file, mode );
			if ( output == NULL ) {
				prmsg( MSG_ERROR, "cannot open output file '%s'",
					file );
				exit( 2 );
			}
			*stdout = *output;
		}

		output = setformat( query );
		if ( output == NULL ) {
			/*
			 * Since all field delimiters have already been modified
			 * it is no longer possible to "drop-back" to printing
			 * unformatted output so we have to report an error and exit.
			 */
			prmsg( MSG_ERROR, "cannot start up format command '%s'",
				formatcmd );
			output = stdout;
			exit( 2 );
		}
		return;
	}
	else if ( chk_existence )
	{
		output = NULL;
		settplfunc( query, tuple_exist );
		return;
	}
	else if ( file == NULL ) {
		output = stdout;
		settplfunc( query, writeattrvals );
		if ( create_descrwtbl )
		{
			if ( ! querytorel( query, &relinfo, NULL,
						NULL, UR_DESCRWTBL ) )
			{
				(void)pruerror();
				prmsg( MSG_ERROR, "cannot convert projected attributes to relation to write descriptor" );
				exit( 2 );
			}
			if ( ! wrtbldescr( stdout, &relinfo ) )
			{
				(void)pruerror( );
				prmsg( MSG_ERROR, "cannot write descriptor information with data table",
					relinfo.path );
				exit( 2 );
			}
		}
		return;
	}

	/*
	 * Output is going to another relation
	 */
	settplfunc( query, insertattrvals );

	/*
	 * If an alternate table (i.e., table=alt_table)
	 * was given on the command line then we need to
	 * pass the entire table=alt_table string to
	 * getrelinfo() so that it can determine whether
	 * or not the data directory is to be searched
	 * first for the description.
	 */
	if ( equalsign )
		*equalsign = '=';

	if ( getrelinfo( file, &relinfo, FALSE ) == NULL ) {

		/* remove the "=<alt_table>" from the output file name if present */
		if ( equalsign )
			*equalsign = '\0';

		/*
		 * Must make our own descriptor file.
		 */
		if ( querytorel( query, &relinfo, file, descr,
				create_descrwtbl ? UR_DESCRWTBL : 0 ) == NULL )
		{
			(void)pruerror();
			prmsg( MSG_ERROR, "cannot convert projected attributes to relation" );
			exit( 2 );
		}

		if ( ! create_descrwtbl && ! writedescr( &relinfo, NULL ) )
		{
			(void)pruerror( );
			prmsg( MSG_ERROR, "cannot write descriptor information for file '%s'",
				relinfo.path );
			exit( 2 );
		}
	}
	else
	{
		/* remove the "=<alt_table>" from the output file name if present */
		if ( equalsign )
			*equalsign = '\0';

		if ( descr )
			relinfo.path = file;
	}

	if ( (insptr = init_insert( &relinfo, "" )) == NULL ) {
		(void)pruerror( );
		prmsg( MSG_ERROR, "cannot start insert for output file '%s'",
			file );
		exit( 2 );
	}

	for( i = 0, refptr = query->attrlist; i < query->attrcnt;
			i++, refptr++ ) {
		if ( refptr->flags & QP_NODISPLAY )
			continue;

		if ( ! addattr( insptr, refptr->prname ) ) {
			(void)pruerror( );
			prmsg( MSG_ERROR, "cannot initialize for insertion on attribute %s",
				refptr->prname );
			(void)end_insert( (struct uinsertinfo *)NULL, FALSE );
			exit( 2 );
		}
	}
}
