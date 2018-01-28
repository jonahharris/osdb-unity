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
#include <ctype.h>
#include "message.h"
#include "permission.h"
#include "uquery.h"
#include "seekval.h"

extern unsigned short geteuid(), getegid();
extern char *getenv();
extern char *basename();
extern struct uquery *fmkquery();
extern struct urelation *getrelinfo();
extern void setunpackenv();

RETSIGTYPE catchsig();
int doupdate();

extern char uversion[];		/* version string */

extern int  uchecktuple;	/* if set readtuple() will count/report tuple errors */
extern long utplerrors; 	/* no. of tuples read with tuple errors */
extern long utplelimit; 	/* limit on number of tuple errors reported */
extern int  utplmsgtype;	/* MSG_ERROR or MSG_WARN type tuple error message */

char *prog;
struct urelation *modrel;
struct uperuseinfo *perptr;
char quiet, noop;
char printrecnum, printold;
char flddelim = '\0';
char flddelim_set = FALSE;

main( argc, argv )
int argc;
char *argv[];
{
	register int i;
	int maxattr;
	int relcnt;
	long updcnt;
	int queryflags = 0;
	struct uquery *query;
	char *attrnames[ 1 ];
	char *relnames[ MAXRELATION ];

	prog = basename( *argv++ );

	if ( argc < 2 )
		usage( );

	for( --argc; argc > 0; argc--, argv++ ) {
		char *option;

		option = *argv;
		if ( *option != '-' ) {
#ifdef DEBUG
			if ( strncmp( option, "debug=", 6 ) == 0 ) {
				(void)set_qdebug( &option[6] );
				continue;
			}
#endif /* DEBUG */
			break;
		}

		while( *++option )
		{
			switch( *option ) {
			case 'i':
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
						option = "Q";
					}
				}
				else if ( argc > 1 ) {
					char *p = argv[1];
					if ((( p[0] == '-' ) &&
					     ( isdigit( p[1] ))) ||
					    ( isdigit( p[0] ))) {
						uchecktuple = UCHECKPRINT;
						utplelimit = atoi( p );
						option = "Q";
						--argc;
						++argv;
					}
				}
				break;
			case 'q':
				quiet = TRUE;
				break;
			case 'n':
				noop = TRUE;
				break;
			case 'o':
				printold = TRUE;
				break;
			case 'r':
				printrecnum = TRUE;
				break;
			case 'V':
				prmsg( MSG_NOTE, "%s", uversion );
				exit( 0 );
			case 'd':
#ifdef DEBUG
				if ( strcmp( option, "debug" ) == 0 ) {
					(void)set_qdebug( "all" );
					option = "\0\0";
					break;
				}
#endif
				if ( strncmp( option, "delim", 5 ) == 0 ) {
					if ( option[5] ) {
						cnvtbslsh( &option[5], &option[5] );
						flddelim = option[5];
						flddelim_set = TRUE;
					}
					option = "\0\0";
					break;
				}

				noop = TRUE;
				break;
			default:
				prmsg( MSG_ERROR, "unrecognized option '%c'",
					*option );
				usage( );
			}
		}
	}

	if ( argc > 0 && strcmp( "from", *argv ) == 0 )
	{
		--argc;
		++argv;
	}

	if ( argc < 1  ) {	/* gotta have a table */
		prmsg( MSG_ERROR, "no tables given on command line" );
		usage( );
	}

	/*
	 * Set environment to handle read (unpack) of packed relations
	 * based on the UNITYUNPACK environement variable.
	 */
	setunpackenv();

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

	relnames[0] = *argv++;	/* save modified table name */
	--argc;
	relcnt = 1;

	if ( argc > 0 && strcmp( *argv, "with" ) == 0 )
	{
		--argc;
		++argv;
		while( argc > 0 && strcmp( *argv, "where" ) != 0 )
		{
			if ( relcnt >= MAXRELATION )
			{
				prmsg( MSG_ERROR, "too many relations given on command line (max is %d)",
					MAXRELATION );
				exit( 1 );
			}

			relnames[ relcnt++ ] = *argv;
			--argc;
			++argv;
		}
	}

	if ( argc > 0 )		/* build query for retrieved tuples */
	{
		unsigned short uid, gid;
		register struct urelation *relptr;

		if ( strcmp( *argv, "where" ) != 0 )
			badarg( *argv );
		--argc;
		++argv;

		/*
		 * Get a query on retrieved tuples.
		 */
		attrnames[0] = "1.seek#";
		query = fmkquery( queryflags, relnames, relcnt, attrnames, 1,
				(char **)NULL, 0, argv, argc );
		if ( query == NULL )
		{
			/* message already printed */
			exit( 1 );
		}

		modrel = query->nodelist[0]->rel;

		uid = geteuid();
		gid = getegid();

		for( i = 0; i < query->nodecnt; i++ )
		{
			relptr = query->nodelist[i]->rel;
			if ( relptr->attrcnt > maxattr )
				maxattr = relptr->attrcnt;
	
			if ( strcmp( relptr->path, "-" ) != 0 &&
				! chkperm( relptr->path, P_READ, uid, gid ) )
			{
				prmsg( MSG_ERROR, "cannot read file '%s'",
					relptr->path );
				usage( );
			}
		}
	}
	else
	{
		query = NULL;

		modrel = getrelinfo( relnames[0], NULL, TRUE );
		if ( modrel == NULL )
		{
			(void)pruerror();
			prmsg( MSG_ERROR, "cannot get relation info for table '%s'",
				relnames[0] );
			usage( );
		}

		maxattr = modrel->attrcnt;
	}

	if ( ! chkupdate( modrel->path ) )
	{
		prmsg( MSG_ERROR, "cannot update table '%s'", modrel->path );
		usage( );
	}

	(void)set_attralloc( maxattr );

	(void)set_sig( SIGHUP, catchsig );	/* catch signals */
	(void)set_sig( SIGINT, catchsig );
	(void)set_sig( SIGQUIT, catchsig );
	(void)signal( SIGILL, catchsig );
#ifdef SIGIOT
	(void)signal( SIGIOT, catchsig );
#endif
#ifdef SIGEMT
	(void)signal( SIGEMT, catchsig );
#endif
	(void)signal( SIGFPE, catchsig );
	(void)signal( SIGBUS, catchsig );
	(void)signal( SIGSEGV, catchsig );
#ifdef SIGSYS
	(void)signal( SIGSYS, catchsig );
#endif
	(void)signal( SIGPIPE, catchsig );
	(void)set_sig( SIGTERM, catchsig );

	if ( (perptr = init_peruse( modrel, noop ? "r" : "w" )) == NULL )
	{
		(void)pruerror( );
		prmsg( MSG_ERROR, "cannot initialize update to table '%s'",
			modrel->path );
		exit( 2 );
	}

	if ( query != NULL )
	{
		if ( ! seeksave( query, FALSE ) )
		{
			(void)pruerror( );
			prmsg( MSG_ERROR, "query evaluation failed -- no deletions done" );
			(void)end_peruse( perptr, FALSE );
			exit( 2 );
		}
		else if ( getseekcnt( ) == 0 )
		{
			if ( ! end_peruse( perptr, FALSE ) ) {
				(void)pruerror( );
				prmsg( MSG_ERROR, "due to previous errors, no deletions done" );
				exit( 2 );
			}
			if ( ! quiet ) {
				if ( utplerrors ) {
					prmsg( MSG_WARN, "%d tuple error%s encountered reading relation(s)",
						utplerrors, utplerrors != 1 ? "s" : "" );
				}
				prmsg( MSG_ERRNOTE, "0 records %sdeleted",
					noop ? "would be " : "" );
			}
			exit( 0 );
		}
	}
	else
		seekreset( );	/* reset seek structs so all tuples updated */

	updcnt = save_unchgd( perptr, noop, TRUE, doupdate );
	if ( updcnt < 0 )
	{
		(void)pruerror( );
		prmsg( MSG_ERROR, "due to previous errors, no deletions done" );
		(void)end_peruse( perptr, FALSE );
		exit( 2 );
	}
	else if ( query != NULL && updcnt != getseekcnt( ) )
	{
		prmsg( MSG_INTERNAL, "cannot find all tuples destined to be deleted (deleted %ld, should have been %ld) -- deletion aborted",
			updcnt, getseekcnt( ) );

		(void)end_peruse( perptr, FALSE );
		exit( 2 );
	}

	(void)signal( SIGHUP, SIG_IGN );	/* ignore signals */
	(void)signal( SIGINT, SIG_IGN );
	(void)signal( SIGQUIT, SIG_IGN );
	(void)signal( SIGTERM, SIG_IGN );

	if ( ! end_peruse( perptr, ! noop ) )
	{
		(void)pruerror( );
		prmsg( MSG_ERROR, "cannot update table '%s' -- no modifications done",
			modrel->path );
		exit( 2 );
	}

	if ( ! quiet )
	{
		if ( utplerrors ) {
			prmsg( MSG_WARN, "%d tuple error%s encountered reading relation(s)",
				utplerrors, utplerrors != 1 ? "s" : "" );
		}
		prmsg( MSG_ERRNOTE, "%d record%s %sdeleted", updcnt,
			updcnt != 1 ? "s" : "",
			noop ? "would be " : "" );
	}

	exit( 0 );
}

usage( )
{
	prmsg( MSG_USAGE, "[-dioqrV] [-Q [ErrorLimit]] [from] <table[=<alt_table>] \\" );
	prmsg( MSG_CONTINUE, "[with <table>[=<alt_table>]...] [where <where-clause>]" );

	exit( 1 );
}

badarg( arg )
char *arg;
{
	prmsg( MSG_ERROR, "unrecognized argument '%s'", arg );
	usage( );
}

set_sig( sig, func )
int sig;
RETSIGTYPE (*func)();
{
	if ( signal( sig, SIG_IGN ) != (RETSIGTYPE (*)())SIG_IGN )
		(void)signal( sig, func );
}

RETSIGTYPE
catchsig( sig )
int sig;
{
	(void)signal( sig, SIG_IGN );

	(void)end_peruse( (struct uperuseinfo *)NULL, FALSE );

	if ( sig == SIGHUP || sig == SIGINT || sig == SIGTERM ||
		sig == SIGQUIT )
	{
		prmsg( MSG_ALERT, "killed by signal (%d) -- no changes made to table",
			sig );

		if ( sig != SIGQUIT )
			exit( 3 );
	}

	(void)signal( sig, SIG_DFL );	/* do the normal operation */
	(void)kill( getpid( ), sig );

	exit( 2 );		/* should never reach here */
}

/*ARGSUSED*/
int
doupdate( perptr, noop, quiet, tplptr, updcnt )
struct uperuseinfo *perptr;
char noop;
char quiet;
struct utuple *tplptr;
long updcnt;
{
	register int i;
	register struct uattribute *attrptr;

	if ( printrecnum )
		printf( "%ld%c", tplptr->tuplenum, printold ? ':' : '\n' );

	if ( printold )
	{
		/*
		 * Print out the old deleted tuple.
		 */
		for( i = 0, attrptr = perptr->relptr->attrs;
			i < perptr->relptr->attrcnt;
			i++, attrptr++ )
		{
			if ( flddelim_set ) {
				(void)writeattrvalue( stdout, tplptr->tplval[i],
					i == perptr->relptr->attrcnt - 1 ?
					'\n' : (char)flddelim, TRUE,
					attrptr->terminate,
					attrptr->attrtype == UAT_TERMCHAR ?
						QP_TERMCHAR : QP_FIXEDWIDTH );
			} else {
				char delim, prdelim;
				unsigned char attrtype;
				unsigned short width;

				if ( attrptr->attrtype == UAT_TERMCHAR ) {
					delim = (char)attrptr->terminate;
					prdelim = TRUE;
					attrtype = QP_TERMCHAR;
				} else {
					attrtype = QP_FIXEDWIDTH;
					width = attrptr->terminate;
					if ( i == perptr->relptr->attrcnt - 1 ) {
						delim = '\n';
						prdelim = TRUE;
					} else {
						delim = '\0';
						prdelim = FALSE;
					}
				}
				(void)writeattrvalue( stdout, tplptr->tplval[i],
					delim, prdelim, width, attrtype );
			}
		}
	}

	return( TRUE );
}
