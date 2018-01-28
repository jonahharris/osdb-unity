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
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include "message.h"
#include "permission.h"
#include "uquery.h"


#ifndef	__STDC__
extern long strtol();
extern char *strchr(), *strrchr();
#endif

extern unsigned short getegid(), geteuid();
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
extern char *calloc(), *malloc();
#endif
extern char *basename();
extern struct uquery *fbldquery();
extern struct urelation *getrelinfo();

RETSIGTYPE catchsig();
int alterattrs();
int writeattrvals();

extern char uversion[];			/* version string */

extern int  uchecktuple;	/* if set readtuple() will count/report tuple errors */
extern long utplerrors; 	/* no. of tuples read with tuple errors */
extern long utplelimit; 	/* limit on number of tuple errors reported */
extern int  utplmsgtype;	/* MSG_ERROR or MSG_WARN type tuple error message */

char *prog;
static char chk_sorted = FALSE;	/* Check if table is sorted */
static char create_descrwtbl = FALSE;	/* put description with output table? */
static char noop = FALSE;
static char quiet = FALSE;
static char sortstdin = FALSE;
static struct uperuseinfo *perptr;
static struct uquery *query;
static struct urelation *sortrel;
static unsigned long deletecnt;
static unsigned long modifycnt;
static unsigned long tuplecnt;
static unsigned long totalcnt;
static short debug = 0;
static short maxattr = 0;
static short projcnt = 0;
static unsigned short sort_priority = 0;

static
set_attr_all( projptr, nodeptr )
register struct qprojection *projptr;
register struct qnode *nodeptr;
{
	projptr->rel = nodeptr;
	projptr->attr = ATTR_ALL;
	projptr->flags = 0;
	projptr->priority = 0xffff;
	projptr->sortattr = 0;
	projptr->attorval = NULL;
	projptr->delim = NULL;
	projptr->subcnt = 0;
	projptr->prwidth = 0;
	projptr->prname = NULL;
	projptr->justify = '\0';
	projptr->terminate = '\0';
	projptr->attrwidth = 0;
	projptr->attrtype = -1;
}

main( argc, argv )
int argc;
char *argv[];
{
	extern int uerror;
	register int i, attridx;
	char *relname;
	unsigned short uid, gid;
	int already_sorted;
	int queryflags = Q_SORT;
	char caseless = FALSE;
	char rmblanks = FALSE;
	struct qresult result;

	prog = basename( *argv++ );

	for( --argc; argc > 0; argc--, argv++ )
	{
		register char *option;

		option = *argv;
		if ( *option != '-' )
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

		while( *++option )
		{
			switch( *option ) {
			case 'c':
				create_descrwtbl = TRUE;
				break;
			case 'E':
				chk_sorted = TRUE;
				noop = TRUE;
				quiet = TRUE;
				break;
			case 'M':
			case 'm':
				{
					char *p;

					if ( option[1] ) {
						p = &option[1];
					} else if ( argc > 1 ) {
						p = argv[1];
						--argc;
						++argv;
					} else {
						usage( );
					}
					if ( *p == ':' )
						++p;

					if ( strcmp( p, "caseless" ) == 0 ) {
						caseless = TRUE;
					} else if ( ( strcmp( p, "blanks" ) == 0 ) ||
						    ( strcmp( p, "noblanks" ) == 0 ) ) {
						rmblanks = TRUE;
					} else {
						prmsg( MSG_ERROR, "invalid -M '%s' attribute modifier", p );
						usage( );
					}
					option = "M";
				}
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
			case 's':
				queryflags ^= Q_SORT;
				break;
			case 'u':
				queryflags ^= Q_UNIQUE;
				break;
			case 'V':
				prmsg( MSG_NOTE, "%s", uversion );
				exit( 0 );
			case 'n':
				noop = TRUE;
				break;
			case 'd':
#ifdef DEBUG
				if ( strcmp( option, "debug" ) == 0 )
				{
					(void)set_qdebug( "all" );
					option = "\0\0";
					break;
				}
#endif
				++debug;
				break;
			default:
				prmsg( MSG_ERROR, "unrecognized option '%c'",
					*option );
				usage( );
			}
		}
	}

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

	if ( ( argc < 2 ) ||
	     ( strcmp( argv[argc-2], "in" ) != 0 ) ||
	    (( strcmp( argv[0], "by" ) != 0 ) && ( argc != 2 )) ) {
		usage( );
	}

	/*
	 * Get the relation.
	 */
	relname = argv[argc-1];

	/*
	 * Check if stdin is the table to be sorted.
	 */
	if ( ( relname[0] == '-' ) &&
	   ( ( relname[1] == '\0' ) ||
	     ( relname[1] == '=' ) ) ) {
		sortstdin = TRUE;
	} else {
		sortstdin = FALSE;
	}

	{
		register struct qprojection *projptr;
		struct qnode *nodelist;			/* node space for query */
		struct qprojection attrlist[MAXATT];	/* projected attributes */

		nodelist = (struct qnode *)calloc( (unsigned) 1, sizeof( *nodelist ) );
		if ( nodelist == NULL )
		{
			prmsg( MSG_INTERNAL, "cannot allocate space for relation information" );
			exit( 2 );
		}

		sortrel = getrelinfo( relname, NULL, FALSE );
		if ( ( sortrel == NULL ) || ( sortrel->attrcnt <= 0 ) )
		{
			prmsg( MSG_ERROR, "cannot get relation info for table '%s'", relname );
			usage( );
		}
		nodelist[0].rel = sortrel;	/* save relation in qnode */

		maxattr = sortrel->attrcnt;

		if ( ! sortstdin && ! noop && ! chkupdate( sortrel->path ) )
		{
			prmsg( MSG_ERROR, "cannot update table '%s'", sortrel->path );
			usage( );
		}


		/* project all attributes in the relation */
		projptr = attrlist;
		for ( projcnt = 0, i = 0; i < maxattr; i++ )
		{
			if ( ! lookupprojattr( sortrel->attrs[i].aname, nodelist, 1, projptr, FALSE ) )
			{
				prmsg( MSG_INTERNAL, "unable to lookup projected attribute name '%s'" );
				exit( 2 );
			}
			projptr++;
			projcnt++;
		}

		/*
		 * Get list of attributes to sort by
		 * and save them in the projection list.
		 */
		if ( ( argc > 0 ) && ( strcmp( argv[0], "by" ) == 0 ) )
		{
			register char *p;
			unsigned allmodcnt;
			unsigned sortalloc;
			int	 j;
			struct qprojection sortattr;

			for ( i = 1; i < argc - 2; i++ )
			{
				p = argv[i];
				if ( *p == ':' ) {
					prmsg( MSG_ERROR, "modifer '%s' must be prefixed by an attribute name", p );
					usage( );
				}

				if ( ( strncmp( p, "ALL:nodisplay=", 14 ) == 0 ) ||
				     ( ! lookupprojattr( p, nodelist, 1, &sortattr, FALSE ) ) )
				{
					if ( ( strncmp( p, "ALL", 3 ) == 0 ) &&
					   ( ( p[3] == '\0' ) || ( p[3] == ':' ) ) )
					{
						set_attr_all( &sortattr, &nodelist[0] );

						if ( ( p[3] == ':' ) &&
						     ( ! attr_modifier( &sortattr, &p[3], TRUE ) ) )
						{
							/* message already printed */
							exit( 1 );
						}
					}
					else
					{
						prmsg( MSG_ERROR, "unrecognized sort attribute name '%s'", p );
						exit( 1 );
					}
				}

				while ( argv[i+1][0] == ':' )
				{
					p = argv[++i];

					if ( ! attr_modifier( &sortattr, p, TRUE ) )
					{
						/* message already printed */
						exit( 1 );
					}
				}

				/*
				 * Clear out any flag bits that are not
				 * used for sorting including attorval
				 * which is set for all:nodisplay=...
				 */
				sortattr.flags &= (QP_SORTMASK|QP_DESCENDING|QP_RMBLANKS);
				sortattr.attorval = NULL;

				/*
				 * Merge the sort attribute info with the projected
				 * attribute info if this is not a special attribute
				 * and of this is the first time this attribute has
				 * been included in the sorted by list.
				 */
				if ( ( sortattr.attr >= 0 ) &&
				     ( attrlist[ sortattr.attr ].priority > sort_priority ) )
				{
					projptr = &attrlist[ sortattr.attr ];

					projptr->flags = QP_SORT | sortattr.flags;
					projptr->subcnt = sortattr.subcnt;
					projptr->priority = sort_priority++;
					projptr->delim = sortattr.delim;
					for ( j = 0; j < (int) sortattr.subcnt; j++ )
						projptr->subsort[j] = sortattr.subsort[j];
					continue;
				}

				if ( sortattr.attr != ATTR_ALL )
				{
					if ( projcnt >= MAXATT ) {
						prmsg( MSG_ERROR, "too many projected attributes -- max is %d", MAXATT );
						exit( 1 );
					}

					attrlist[ projcnt ] = sortattr;
					projptr = &attrlist[ projcnt++ ];

					projptr->flags |= QP_SORT | QP_NODISPLAY;
					projptr->priority = sort_priority++;

					/*
					 * If normal attribute then set flag
					 * then this attribute should not be
					 * skipped when checking uniqueness.
					 */
					if ( sortattr.attr >= 0 )
						projptr->flags |= QP_SORTUNIQ;
					continue;
				}

				/*
				 * When ATTR_ALL (all or ALL) is specified,
				 * then all (normal) attributes that
				 * have not been sorted will be sorted
				 * in order using the sort information
				 * appended to the special "all" attribute.
				 */
				for ( projptr = attrlist, attridx = 0; attridx < maxattr; attridx++, projptr++ )
				{
					if ( projptr->priority < sort_priority )
						continue;

					projptr->flags = QP_SORT | sortattr.flags;
					projptr->subcnt = sortattr.subcnt;
					projptr->priority = sort_priority++;
					projptr->delim = sortattr.delim;
					for ( j = 0; j < (int) sortattr.subcnt; j++ )
						projptr->subsort[j] = sortattr.subsort[j];
				}
			}

			if ( sort_priority == 0 ) {
				prmsg( MSG_ERROR, "no attributes in the \"by\" clause" );
				usage( );
			}
		}

		query = fbldquery( nodelist, 1, attrlist, projcnt, (struct queryexpr *)NULL, queryflags );
	}

	if ( query == NULL )
	{
		prmsg( MSG_ERROR, "cannot create query for sorting tuples" );
		exit( 1 );
	}

	if ( query->attrcnt != projcnt )
	{
		prmsg( MSG_INTERNAL, "attribute count for query (%d) does not match projection (%d)",
			query->attrcnt, projcnt );
	}

	uid = geteuid();
	gid = getegid();

	(void)set_attralloc( projcnt );

	if ( caseless || rmblanks )
	{
		register unsigned short	flags;

		for ( i = 0; i < projcnt; i++ ) {
			if ( ( queryflags & Q_UNIQUE ) || ( query->sortcnt == 0 ) ||
			     ( query->attrlist[i].priority != 0xffff ) ) {
				flags = query->attrlist[i].flags;
				switch ( flags & QP_SORTMASK ) {
				case QP_CASELESS:
				case QP_NOCASEDICT:
				case QP_NOCASEPRINT:
					if ( rmblanks )
						flags |= QP_RMBLANKS;
					break;
				case QP_DICTIONARY:
					if ( caseless ) {
						flags &= ~QP_SORTMASK;
						flags |= QP_NOCASEDICT;
					}
					if ( rmblanks )
						flags ^= QP_RMBLANKS;
					break;
				case QP_PRINTABLE:
					if ( caseless ) {
						flags &= ~QP_SORTMASK;
						flags |= QP_NOCASEPRINT;
					}
					if ( rmblanks )
						flags |= QP_RMBLANKS;
					break;
				case QP_STRING:
				case 0:
					if ( caseless ) {
						flags &= ~QP_SORTMASK;
						flags |= QP_CASELESS;
					}
					if ( rmblanks )
						flags |= QP_RMBLANKS;
					break;
				default:
					break;
				}
				query->attrlist[i].flags = flags;
			}
		}
	}

	if ( ( debug ) && ( noop ) )
	{
		register int i;
		register int j;
		fprintf(stderr, "attrcnt = %u\n", query->attrcnt);
		fprintf(stderr, "sortcnt = %u\n", query->sortcnt);
		for (i = 0; i < (int) query->attrcnt; i++) {
			fprintf(stderr, "attr[%02u] = %-2d", i, query->attrlist[i].attr);
			fprintf(stderr, " sortattr = %-2u", query->attrlist[i].sortattr);
			fprintf(stderr, " priority = %-5u", query->attrlist[i].priority);
			fprintf(stderr, " flags = 0x%04x", query->attrlist[i].flags);
			fprintf(stderr, " subcnt = %u", query->attrlist[i].subcnt);
			fprintf(stderr, " prname = %s\n", query->attrlist[i].prname);
		}
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

	if ( sortstdin ) {
		if ( ( create_descrwtbl ) && ( ! noop ) )
		{
			if ( ! wrtbldescr( stdout, query->nodelist[0]->rel ) )
			{
				(void)pruerror( );
				prmsg( MSG_ERROR, "cannot write descriptor information with data table",
					sortrel->path );
				exit( 2 );
			}
		}
		settplfunc( query, writeattrvals );
	} else {
		if ( (perptr = init_peruse( sortrel, noop ? "r" : "w" )) == NULL )
		{
			(void)pruerror( );
			prmsg( MSG_ERROR, "cannot initialize update to table '%s'",
				sortrel->path );
			exit( 2 );
		}
		settplfunc( query, alterattrs );
	}

	modifycnt = 0;
	tuplecnt = 0;
	totalcnt = 0;

	/*
	 * The alterattrs() or writeattrvals() function
	 * will be called by queryeval().
	 */
	if ( ! queryeval( query, &result ) )
	{
		if ( uerror ) {
			(void)pruerror( );
			if ( ( tuplecnt ) && ( ! noop ) && ( sortstdin ) ) {
				prmsg( MSG_ERROR,
					"query evaluation failed after sorting %u tuple%s",
					tuplecnt, tuplecnt != 1 ? "s" : "" );
			} else {
				prmsg( MSG_ERROR, "query evaluation failed" );
			}
			if ( ! sortstdin ) {
				(void)end_peruse( perptr, FALSE );
			}
		} else {
			if ( ( tuplecnt ) && ( ! noop ) && ( sortstdin ) ) {
				prmsg( MSG_ERROR,
					"failed after sorting %u tuple%s",
					tuplecnt, tuplecnt != 1 ? "s" : "" );
			} else {
				prmsg( MSG_ERROR, "failed due to previous error(s)" );
			}
		}
		if ( ! sortstdin ) {
			(void)end_peruse( perptr, FALSE );
		}
		exit( 2 );
	}

	(void)signal( SIGHUP, SIG_IGN );	/* ignore signals */
	(void)signal( SIGINT, SIG_IGN );
	(void)signal( SIGQUIT, SIG_IGN );
	(void)signal( SIGTERM, SIG_IGN );

	if ( totalcnt > tuplecnt ) {
		already_sorted = FALSE;
		deletecnt = totalcnt - tuplecnt;
	} else {
		if ( ( totalcnt == tuplecnt ) && ( modifycnt == 0 ) ) {
			already_sorted = TRUE;
		} else {
			already_sorted = FALSE;
		}
		deletecnt = 0;
	}

	if ( ! sortstdin )
	{
		if ( ( ! stop_save( perptr, FALSE ) ) ||
		     ( ! end_peruse( perptr, ( ( ! noop ) && ( tuplecnt ) && ( ! already_sorted ) ) ) ) ) {
			if ( uerror )
				(void)pruerror( );
			prmsg( MSG_ERROR, "failed due to previous error(s)" );
			exit( 2 );
		}
	}

	if ( ( chk_sorted ) && ( ! already_sorted ) ) {
		exit( 4 );      /* table not sorted */
	}

	if ( ! quiet )
	{
		char	*sptr = ( tuplecnt != 1 ) ? "s" : ""; 

		if ( utplerrors ) {
			prmsg( MSG_WARN, "%d tuple error%s encountered reading relation",
				utplerrors, utplerrors != 1 ? "s" : "" );
		}

		if ( deletecnt ) {
			prmsg( MSG_ERRNOTE, "%u record%s %ssorted and %u deleted",
				tuplecnt, sptr, noop ? "would be " : "", deletecnt );
		} else if ( ( already_sorted ) && ( noop ) ) {
			prmsg( MSG_ERRNOTE, "%u record%s already sorted",
				tuplecnt, sptr);
		} else {
			prmsg( MSG_ERRNOTE, "%u record%s %ssorted",
				tuplecnt, sptr, noop ? "would be " : "" );
		}
	}

	exit( 0 );
}

usage( )
{
	prmsg( MSG_USAGE, "[-cnqsuEV] [-M {blanks|caseless}] [-Q [ErrorLimit]] \\" );
	prmsg( MSG_CONTINUE, "[by <attr>[:modifier] ... ] in <table>[=<alt_table>]" );
	exit( 1 );
}

set_sig( sig, func )
int sig;
RETSIGTYPE (*func)();
{
	if ( signal( sig, SIG_IGN ) != (RETSIGTYPE(*)())SIG_IGN )
		(void)signal( sig, func );
}

RETSIGTYPE
catchsig( sig )
int sig;
{
	(void)signal( sig, SIG_IGN );

	if ( ! sortstdin ) {
		(void)end_peruse( (struct uperuseinfo *)NULL, FALSE );
	}

	if ( sig == SIGHUP || sig == SIGINT || sig == SIGTERM || sig == SIGQUIT ){
		prmsg( MSG_ALERT, "killed by signal (%d) -- no alterations done",
			sig );

		if ( sig != SIGQUIT )
			exit( 3 );
	}

	(void)signal( sig, SIG_DFL );	/* do the normal operation */
	(void)kill( getpid( ), sig );

	exit( 2 );		/* should never reach here */
}

alterattrs( attrvals, attrcnt, projptr )
char **attrvals;
int attrcnt;
struct qprojtuple *projptr;	/* WARNING: projptr->projptr is NULL */
{
	if ( projptr->tplptr->tuplenum != ++tuplecnt) {
		++modifycnt;
	}

	if ( ! noop && ! savetuple( perptr, attrvals ) )
	{
		(void)pruerror( );
		prmsg( MSG_INTERNAL, "tuple save failed for table '%s'",
			perptr->relptr->path );
		return( FALSE );
	}

	if ( ! totalcnt ) {
		totalcnt = query->nodelist[0]->tuplecnt;
	}

	return( TRUE );
}

writeattrvals( attrvals, projcnt, projptr )
char **attrvals;
int projcnt;
struct qprojtuple *projptr;
{
	register int i;

	/*
	 * Wait to increment tuplecnt until finished
	 * processing this tuple so that if there
	 * is an error, the query evaluation failed
	 * message can correctly reflect how many
	 * tuples had been sucessfully retrieved.
	 */
	if ( projptr->tplptr->tuplenum != tuplecnt + 1) {
		++modifycnt;
	}

	if ( ! noop ) {
		for( i = 0; i < projcnt; i++, projptr++ ) {
			if ( ! writeattrvalue( stdout, attrvals[i],
					projptr->projptr->terminate, TRUE,
					projptr->projptr->attrwidth,
					projptr->projptr->attrtype ) )
			{
				(void)pruerror();
				prmsg( MSG_ERROR, "cannot write %d attribute in tuple %d -- exiting",
					i, tuplecnt + 1 );
				return( FALSE );
			}
		}
	}

	tuplecnt++;

	if ( ! totalcnt ) {
		totalcnt = query->nodelist[0]->tuplecnt;
	}

	return( TRUE );
}

