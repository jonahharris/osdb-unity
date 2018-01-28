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
struct uperuseinfo *perptr;
char quiet, noop;
char force, friendly, newline = '~', validate, no_validate;
FILE *editfp;
char editfile[ MAXPATH + 4 ];	/* allow for "/./" or "././" prefix */
char tplfile[ MAXPATH + 4 ];	/* allow for "/./" or "././" prefix */

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
	struct urelation *modrel;
	int blankcnt = 0;
	char *editor;
	FILE *tmpfp;
	char *attrnames[ 1 ];
	char *relnames[ MAXRELATION ];
	char *valprefix;			/* prefix for validate -Itable option */
	char cmdline[ 2 * MAXPATH + 8 ];	/* allow for "/./" or "././" prefix */
	char descfile[ MAXPATH + 4 ];		/* descriptor path for validate */

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
			case 'b':
				if ( isdigit( option[1] ) )
				{
					blankcnt = atoi( &option[1] );
					option = "\0\0";
				}
				else if ( argc > 1 && isdigit( *argv[1] ) )
				{
					blankcnt = atoi( argv[1] );
					--argc;
					++argv;
				}
				else
					blankcnt = 1;
				break;
			case 'c':
				validate = TRUE;
				break;
			case 'f':
				force = TRUE;
				break;
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
				if ( option[1] )
				{
					newline = *++option;
					if ( newline == '\n' )
					{
						prmsg( MSG_ERROR, "cannot have '\\n' as newline character" );
						usage( );
					}
				}
				else
					newline = '\0';
				break;
			case 'v':
				friendly = TRUE;
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
				/* We want to fall through here. */
#endif
				noop = TRUE;
				break;
			default:
				prmsg( MSG_ERROR, "unrecognized option '%c'",
					*option );
				usage( );
			}
		}
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

	modrel = getrelinfo( relnames[0], NULL, TRUE );
	if ( modrel == NULL )
	{
		(void)pruerror();
		prmsg( MSG_ERROR, "cannot get relation info for table '%s'",
			relnames[0] );
		usage( );
	}

	if ( ! chkupdate( modrel->path ) )
	{
		prmsg( MSG_ERROR, "cannot update table '%s'", modrel->path );
		usage( );
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

		maxattr = modrel->attrcnt;
	}

	(void)set_attralloc( maxattr );

	if ( validate )
	{
		FILE *fp;
		char *dfile;
		char buf[ MAXPATH + 1 + 4 ];	/* allow for "/./" or "././" prefix */

		extern FILE *findufile();

		/*
		 * Set up the descriptor file path for validation.
		 */
		dfile = basename( modrel->dpath );
		if ( *dfile == 'D' )	/* descriptor file */
		{
			cpdirname( descfile, modrel->dpath );
			strcat( descfile, "/" );
			strcat( descfile, dfile + 1 );
		}
		else			/* description with table */
			strcpy( descfile, modrel->dpath );

		/*
		 * If the validation file is not available,
		 * then validation cannot be done.
		 * Search the datafile (description) directory first ('d').
		 */
		fp = findufile( buf, descfile, 'V', 'd' );
		if ( fp == NULL )
		{
			prmsg( MSG_WARN, "cannot locate validation file for description '%s' -- no validation done",
				descfile );
			no_validate = TRUE;
		}
		else {
			(void)fclose( fp );

			/*
			 * Setup path prefix for validate -I option
			 * to search the given data directory first.
			 */
			valprefix = "";

			if ( descfile[0] == '/' ) {
				if ( ( descfile[1] != '.' ) || ( descfile[2] != '/' ) )
					valprefix = "/.";
			}
			else if ( ( descfile[0] == '.' ) && ( descfile[1] == '/' ) ) {
				if ( ( descfile[2] != '.' ) || (descfile[3] != '/' ) )
					valprefix = "./";
			}
			else {
				valprefix = "././";
			}
		}

		/*
		 * If the description is with the table, we can't do
		 * validation.
		 */
		if ( (modrel->relflgs & UR_DESCRWTBL) != 0 || *dfile != 'D' )
		{
			prmsg( MSG_WARN, "cannot validate tables that have the description with the data -- no validation done" );
			no_validate = TRUE;
		}
	}

	/*
	 * Open edit file for packet formated tuples.
	 */
	(void)cpdirname( editfile, modrel->path );
	strcat( editfile, "/" );
	strcat( editfile, "pakXXXXXX" );
	mktemp( editfile );
	editfp = fopen( editfile, "w" );
	if ( editfp == NULL )
	{
		prmsg( MSG_ERROR, "cannot open edit file '%s' for writing",
			editfile );
		exit( 2 );
	}

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
			prmsg( MSG_ERROR, "query evaluation failed -- no updates done" );
			cleanup( TRUE, 2 );
		}
		else if ( getseekcnt( ) == 0 && blankcnt == 0 )
		{
			prmsg( MSG_ERROR, "no records match the given where clause" );
			cleanup( TRUE, 2 );
		}
	}
	else
		seekreset( );	/* reset seek structs so all tuples updated */

	updcnt = save_unchgd( perptr, noop, TRUE, doupdate );
	if ( updcnt < 0 )
	{
		(void)pruerror( );
		prmsg( MSG_ERROR, "due to previous errors, no updates done" );
		(void)end_peruse( perptr, FALSE );
		exit( 2 );
	}
	else if ( query != NULL && updcnt != getseekcnt( ) )
	{
		prmsg( MSG_INTERNAL, "cannot find all tuples destined to be updated (updated %ld, should have been %ld) -- update aborted",
			updcnt, getseekcnt( ) );

		(void)end_peruse( perptr, FALSE );
		exit( 2 );
	}

	/*
	 * Put out blank records to packet file.
	 */
	if ( blankcnt > 0 )
	{
		char *attrvals[ MAXATT ];

		for( i = 0; i < modrel->attrcnt; i++ )
			attrvals[i] = "";

		for( i = 0; i < blankcnt; i++ )
		{
			updcnt++;
			if ( ! packet( editfp, modrel, attrvals, updcnt,
					friendly, newline ) )
			{
				pruerror( );
				prmsg( MSG_ERROR, "cannot write blank records to editing file '%s'",
					blankcnt, editfile );
				cleanup( TRUE, 2 );
			}
		}
	}

	if ( fclose( editfp ) == EOF )
	{
		prmsg( MSG_ERROR, "cannot close editing file '%s'",
			editfile );
		cleanup( TRUE, 2 );
	}

	if ( ! quiet) {
		if ( utplerrors ) {
			prmsg( MSG_WARN, "%d tuple error%s encountered reading relation(s)",
				utplerrors, utplerrors != 1 ? "s" : "" );
			prmsg( MSG_QUESTION, "Hit ENTER to continue:" );
			gets( cmdline );
		}
	}

	editor = getenv( "ED" );
	if ( editor == NULL || *editor == '\0' )
	{
		editor = getenv( "EDITOR" );
		if ( editor == NULL || *editor == '\0' )
			editor = "ed";
	}

	cpdirname( tplfile, modrel->path );
	strcat( tplfile, "/" );
	strcat( tplfile, "tmpXXXXXX" );
	mktemp( tplfile );

	while ( 1 )
	{
		/*
		 * Edit the packet file.
		 */
		sprintf( cmdline, "%s %s", editor, editfile );
		if ( system( cmdline ) != 0 )	/* edit pak$$ */
		{
			prmsg( MSG_WARN, "edit command failed '%s'", cmdline );
		}

		editfp = fopen( editfile, "r" );
		if ( editfp == NULL )
		{
			prmsg( MSG_ERROR, "cannot open edit file '%s' for reading",
				editfile );
			cleanup( FALSE, 2 );
		}

		tmpfp = fopen( tplfile, "w" );
		if ( tmpfp == NULL )
		{
			prmsg( MSG_ERROR, "cannot open temporary file '%s' for writing",
				tplfile );
			cleanup( FALSE, 2 );
		}

		/*
		 * Convert the updated tuples back to tuple format.
		 * Save number of converted tuples in updcnt.
		 * If conversion fails report the error.
		 */
		updcnt = unpacket( editfp, tmpfp, modrel, friendly, newline );

		(void)fclose( editfp );
		if ( fclose( tmpfp ) == EOF )
		{
			prmsg( MSG_ERROR, "cannot close temporary file '%s'",
				tplfile );
			cleanup( FALSE, 2 );
		}

		if ( updcnt >= 0 )	/* successful parse */
		{
			if ( ! quiet )
				prmsg( MSG_ERRNOTE, "%ld record%s %sconverted",
					updcnt, updcnt != 1 ? "s" : "",
					noop ? "would be " : "" );

			/*
			 * Validate the converted tuples if requested.
			 */
			if ( validate )
			{
				if ( no_validate )
				{
					prmsg( MSG_WARN, "no validation performed due to previous warnings" );
					break;
				}

				sprintf( cmdline, "validate -I'%s%s' %s",
					valprefix, descfile, tplfile );
				if ( system( cmdline ) == 0 )
					break;
				prmsg( MSG_ERROR, "tuple validation failed" );
			}
			else
				break;
		}
		else
		{
			pruerror( );
			prmsg( MSG_ERROR, "tuple parsing failed on line %ld",
				-updcnt );
		}

		/*
		 * There was some problem with the edit file.
		 */
		while( 1 )
		{
			prmsg( MSG_QUESTION, "Re-enter editor to correct problem (y or n)?" );
			cmdline[0] == 'n';
			gets( cmdline );
			if ( cmdline[0] == 'n' )
			{
				prmsg( MSG_ERRNOTE, "edit aborted -- %s unchanged",
					modrel->path );
				cleanup( FALSE, 0 );
			}
			else if ( cmdline[0] == 'y' )
				break;
			else
				prmsg( MSG_ERROR, "unrecognized input -- try again" );
		}

	} /* end while editing table */

	/*
	 * Copy temporary into into new version of table.
	 */
	if ( ! noop && ! copysrcfile( perptr->tmpfp, tplfile ) )
	{
		prmsg( MSG_ERROR, "cannot copy temporary file '%s' to new table",
			tplfile );
		cleanup( FALSE, 2 );
	}

	/*
	 * Ask if the updates should be done if ! force && ! noop.
	 */
	while( ! force && ! noop )
	{
		prmsg( MSG_QUESTION, "make these changes in %s (y or n)?",
			modrel->path );
		cmdline[0] = 'n';
		gets( cmdline );
		if ( cmdline[0] == 'y' )
			break;
		else if ( cmdline[0] == 'n' )
		{
			if ( ! quiet )
				prmsg( MSG_ERRNOTE, "changes not made in %s",
					modrel->path );

			cleanup( TRUE, 0 );
		}
		else
			prmsg( MSG_ERROR, "unrecognized input -- try again" );
	}

	(void)signal( SIGHUP, SIG_IGN );	/* ignore signals */
	(void)signal( SIGINT, SIG_IGN );
	(void)signal( SIGQUIT, SIG_IGN );
	(void)signal( SIGTERM, SIG_IGN );

	(void)unlink( tplfile );
	(void)unlink( editfile );

	if ( ! end_peruse( perptr, ! noop ) )
	{
		(void)pruerror( );
		prmsg( MSG_ERROR, "cannot update table '%s' -- no modifications done",
			modrel->path );
		exit( 2 );
	}

	if ( ! quiet )
	{
		if ( noop )
			prmsg( MSG_ERRNOTE, "demo mode -- '%s' not updated",
				modrel->path );
		else
			prmsg( MSG_ERRNOTE, "'%s' updated successfully",
				modrel->path );
	}

	exit( 0 );
}

usage( )
{
	prmsg( MSG_USAGE, "[-b<blank_cnt>] [-cdfiqvV] [-Q [ErrorLimit]] [-n<newline_char>] \\" );
	prmsg( MSG_CONTINUE, "<table>[=<alt_table>] [with <table>[=<alt_table>]...] \\" );
	prmsg( MSG_CONTINUE, "[where <where-clause>]" );

	exit( 1 );
}

badarg( arg )
char *arg;
{
	prmsg( MSG_ERROR, "unrecognized argument '%s'", arg );
	usage( );
}

cleanup( del_edit, exitcode )
char del_edit;
int exitcode;
{
	if ( del_edit )
		(void)unlink( editfile );
	else
		prmsg( MSG_ERRNOTE, "edited table is in '%s'", editfile );

	(void)unlink( tplfile );
	(void)end_peruse( perptr, FALSE );

	exit( exitcode );
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

	(void)unlink( editfile );
	(void)unlink( tplfile );

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
	/*
	 * Save tuple in packet format for editing.
	 */
	if ( ! packet( editfp, perptr->relptr, tplptr->tplval, updcnt,
			friendly, newline ) )
	{
		pruerror( );
		prmsg( MSG_ERROR, "cannot save tuple %ld in edit file '%s'",
			updcnt, editfile );
		return( FALSE );
	}

	return( TRUE );
}
