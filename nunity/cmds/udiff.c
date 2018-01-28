/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "uquery.h"
#include "format.h"
#include "message.h"

/*
 * Defines for the various clauses we can see.
 */
#define CL_RELNAMES	1
#define CL_SHOW		2
#define CL_COMPARE	3
#define CL_ALLCOMPARE	4
#define CL_IGNORE	5

extern char *getenv();
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
extern char *malloc();
#endif
extern char *cpdirname();
extern char *basename();
extern struct uquery *fmkquery();
FILE *do_retrieve();

extern char uversion[];			/* version string */

char *prog;

char *Qoption = ""; 	/* Quit - [do not] treat tuple error as fatal error */

short quiet = FALSE;

/*
 * Attribute names retrieve from queries
 */
static char *q_attnames[] = {
	"rec#",
	"all:nodisplay=",
};

main( argc, argv )
int argc;
char *argv[];
{
	register int i;
	register int curarg;
	int queryflags = Q_SORT;
	int keystart, keycnt;
	int cmpstart, cmpcnt;
	int showstart, showcnt;
	int ignstart, igncnt;
	int rc1, rc2, rcp1, rcp2;
	int clause;
	char find_compares;		/* look for attributes to compare? */
	char rep_extra1 = TRUE;	/* report extra tuples */
	char rep_extra2 = TRUE;	/* report extra tuples */
	char rep_differ = TRUE;	/* report different tuples */
	long identical, different, extra1, extra2;
	char *rel1path;
	char *rel2path;
	char **wherelist;
	int wherecnt;
	struct uquery *query1;	/* compiled version of query */
	struct uquery *query2;
	struct urelation *rel1, *rel2;
	FILE *fp1, *fp2;
	struct formatio frmio1;	/* results of queries */
	struct formatio frmio2;
	char *attrs1[ MAXATT ];
	char *attrs2[ MAXATT ];
	short attr1num[ MAXATT ];
	short attr2num[ MAXATT ];

	prog = basename( *argv );	/* save the program name */

	/*
	 * Parse any command-line options given.
	 */
	if ( argc < 5 )
		usage( );

	for( curarg = 1; curarg < argc; curarg++ )
	{
		register char *option;

		option = argv[curarg];
		if ( *option != '-' )
		{
#ifdef DEBUG
			if ( strncmp( option, "debug=", 6 ) == 0 ) {
				(void)set_qdebug( &option[6] );
				continue;
			}
#endif /* DEBUG */
			break;
		}

		while( option && *++option ) {
			switch( *option ) {
			case 'd':
#ifdef DEBUG
				if ( strcmp( option, "debug" ) == 0 ) {
					(void)set_qdebug( "all" );
					option = NULL;
				}
				else
				{
					rep_differ = FALSE;
				}
#else
				rep_differ = FALSE;
#endif
				break;
			case '1':
				rep_extra1 = FALSE;
				break;
			case '2':
				rep_extra2 = FALSE;
				break;
			case 'e':
				if ( option[1] == '1' )
				{
					rep_extra1 = FALSE;
					++option;
				}
				else if ( option[1] == '2' )
				{
					rep_extra2 = FALSE;
					++option;
				}
				else
				{
					rep_extra1 = FALSE;
					rep_extra2 = FALSE;
				}
				break;
			case 'Q':		/* don't quit due to tuple errors */
				Qoption = "-Q";
				/* ignore optional ErrorLimit */
				if ( option[1] ) {
					if ((( option[1] == '-' ) &&
					     ( isdigit( option[2] ))) ||
					    ( isdigit( option[1] ))) {
						option = NULL;
					}
				}
				else if ( curarg < argc - 2 ) {
					char *p = argv[ curarg + 1 ];
					if ((( p[0] == '-' ) &&
					     ( isdigit( p[1] ))) ||
					    ( isdigit( p[0] ))) {
						option = NULL;
						++curarg;
					}
				}
				break;
			case 'q':		/* don't print tuple count */
				quiet = TRUE;
				break;
			case 's':
				queryflags &= ~Q_SORT;	/* no need to sort */
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

	/*
	 * Get the list of key attributes.
	 */
	keystart = curarg;
	curarg = findclause( argc, argv, curarg, &keycnt, &clause );
	if ( keycnt == 1 && strcmp( argv[ keystart ], "rec#" ) == 0 )
		queryflags &= ~Q_SORT;	/* no need to sort */

	/*
	 * Now get any show or compare clauses.
	 */
	find_compares = TRUE;	/* look for comparisons between rels? */

	showstart = 0;		/* no clause seen yet */
	cmpstart = 0;
	ignstart = 0;
	showcnt = 0;
	cmpcnt = 0;
	igncnt = 0;
	while( clause != CL_RELNAMES )
	{
		switch( clause )
		{
		case CL_SHOW:
			if ( showstart > 0 )
			{
				prmsg( MSG_ERROR, "multiple 'show' clauses given on command line" );
				usage( );
			}
			showstart = curarg;
			curarg = findclause( argc, argv, curarg,
					&showcnt, &clause );
			break;
		case CL_COMPARE:
			find_compares = FALSE;

			/* We want to fall through here */
		case CL_ALLCOMPARE:
			if ( cmpstart > 0 )
			{
				prmsg( MSG_ERROR, "multiple 'compare' clauses given on command line" );
				usage( );
			}
			cmpstart = curarg;
			curarg = findclause( argc, argv, curarg,
					&cmpcnt, &clause );
			break;
		case CL_IGNORE:
			if ( ignstart > 0 )
			{
				prmsg( MSG_ERROR, "multiple 'ignore' clauses given on command line" );
				usage( );
			}
			ignstart = curarg;
			curarg = findclause( argc, argv, curarg,
					&igncnt, &clause );
			break;
		default:
			prmsg( MSG_ERROR, "no relation names given on command line" );
			usage( );
		}
	}

	/*
	 * Get the relation names.
	 */
	if ( argc - curarg < 2 )
	{
		prmsg( MSG_ERROR, "no relation names given on command line" );
		usage( );
	}
	rel1path = argv[ curarg++ ];
	rel2path = argv[ curarg++ ];

	/*
	 * Check for a where-clause
	 */
	if ( argc - curarg > 0 )
	{
		if ( strcmp( argv[ curarg ], "where" ) != 0 )
		{
			prmsg( MSG_ERROR, "unrecognized where-clause after file names (%s, %s)",
				rel1path, rel2path );
			usage();
		}

		curarg++;	/* skip "where" */
		wherecnt = argc - curarg;
		wherelist = &argv[ curarg ];
	}
	else
	{
		wherelist = NULL;
		wherecnt = 0;
	}

	/*
	 * In order to compare two different versions of the same file
	 * (with different descriptor files) we need to override the
	 * normal search order and check for the Descriptor in the
	 * datafile directory first.  Otherwise, there would be no
	 * way of specifying two different relations if one of them
	 * is in the current directory.
	 *
	 * In order to do this, we make use of the fact that when
	 * an alternate table is specified that contains a special
	 * path prefix or is the same as the table (datafile) name,
	 * the directory specified as part of  "=<alt_table>"
	 * is searched first.  If the user alreaded specified an
	 * alternate table descriptor to be used, then we do not
	 * need to create and append the name of the table as an
	 * alternate table.
	 */

	/*
	 * First check if the second file name is a directory and
	 * then add the alternate table specification if not present.
	 */
	{
		struct stat statbuf;
		int  len;
		char *tb1;
		char *tmp;
		char *alttbl;

		if ( stat( rel2path, &statbuf ) == 0 &&
			(statbuf.st_mode & 0170000) == 0040000 )
		{
			/* path is directory -- append file name */

			alttbl = strrchr( rel1path, '=' );
			if ( alttbl != NULL ) {
				*alttbl = '\0';
			}
			tb1 = basename( rel1path );
			len = strlen( rel2path );
			if ( alttbl != NULL ) {
				*alttbl = '=';
				len = len + strlen( tb1 ) + 2;
			} else {
				len = ( len * 2 ) + ( strlen( tb1 ) * 2 ) + 4;
			}
			tmp = malloc( len );
			if ( tmp == NULL )
			{
				prmsg( MSG_INTERNAL, "cannot get space for path name" );
				exit( 1 );
			}
			if ( alttbl ) {
				sprintf( tmp, "%s/%s", rel2path, tb1 );
			} else {
				sprintf( tmp, "%s/%s=%s/%s", rel2path, tb1, rel2path, tb1 );
			}
			rel2path = tmp;
		} else {
			if ( strrchr( rel2path, '=' ) == NULL )
			{
				len = strlen( rel2path );
				len = len * 2 + 2;
				tmp = malloc( len );
				if ( tmp == NULL )
				{
					prmsg( MSG_INTERNAL, "cannot get space for path name" );
					exit( 1 );
				}
				sprintf( tmp, "%s=%s", rel2path, rel2path );
				rel2path = tmp;
			}
		}
	}

	/*
	 * Now add the alternate table specification to table1 if not present.
	 */
	if ( strrchr( rel1path, '=' ) == NULL )
	{
		int	len;
		char	*tmp;

		len = strlen( rel1path );
		len = len * 2 + 2;
		tmp = malloc( len );
		if ( tmp == NULL )
		{
			prmsg( MSG_INTERNAL, "cannot get space for path name" );
			exit( 1 );
		}
		sprintf( tmp, "%s=%s", rel1path, rel1path );
		rel1path = tmp;
	}

	/*
	 * Build the queries.
	 */

	query1 = fmkquery( queryflags, &rel1path, 1, q_attnames, 2,
			NULL, 0, wherelist, wherecnt );
	if (  query1 == NULL )
	{
		(void)pruerror( );
		prmsg( MSG_ERROR, "first query compilation failed" );
		rel1 = NULL;
	}
	else
	{
		rel1 = query1->nodelist[0]->rel;
	}

	query2 = fmkquery( queryflags, &rel2path, 1, q_attnames, 2,
			NULL, 0, wherelist, wherecnt );
	if (  query2 == NULL )
	{
		(void)pruerror( );
		prmsg( MSG_ERROR, "second query compilation failed" );
		rel2 = NULL;
	}
	else
	{
		rel2 = query2->nodelist[0]->rel;
	}


	if (  rel1 == NULL || rel2 == NULL )
		exit( 1 );

#if 0
	if ( ( (rel1->relflgs & UR_DESCRWTBL) && strcmp( rel1path, "-" ) == 0 ) ||
		( (rel2->relflgs & UR_DESCRWTBL) && strcmp( rel2path, "-" ) == 0 ))
	{
		prmsg( MSG_ERROR, "cannot use standard input w/o an alternate table" );
		exit( 1 );
	}
#endif

	/*
	 * Now go through and parse the attribute names that
	 * were given.
	 */
	keycnt = parse_attrs( &argv[ keystart ], keycnt,
			rel1, rel2, attr1num, attr2num );
	if ( keycnt == 0 )
	{
		prmsg( MSG_ERROR, "no key attributes given on command line" );
		usage( );
	}

	showcnt = parse_attrs( &argv[ showstart ], showcnt,
			rel1, rel2,
			&attr1num[ keycnt ], &attr2num[ keycnt ] );

	cmpcnt = parse_attrs( &argv[ cmpstart ], cmpcnt,
			rel1, rel2,
			&attr1num[ keycnt + showcnt ],
			&attr2num[ keycnt + showcnt ] );

	rc1 = TRUE;
	for( i = 0; i < igncnt; i++ )
	{
		if ( findattr( rel1->attrs, rel1->attrcnt,
			argv[ ignstart + i ] ) < 0 )
		{
			prmsg( MSG_ERROR, "unrecognized ignored attribute '%s' in relation '%s'",
				argv[ ignstart + i ],
				rel1->path );
			rc1 = FALSE;
		}
		if ( findattr( rel2->attrs, rel2->attrcnt,
			argv[ ignstart + i ] ) < 0 )
		{
			prmsg( MSG_ERROR, "unrecognized ignored attribute '%s' in relation '%s'",
				argv[ ignstart + i ],
				rel2->path );
			rc1 = FALSE;
		}
	}
	if ( ! rc1 )
		usage();	/* never returns */

	if ( find_compares )
	{
		/*
		 * Look for all attributes with the same name in both
		 * relations which are not already in the compare lists.
		 */
		int j;

		for( i = 0; i < rel1->attrcnt; i++ )
		{
			if ( attr_ignored( &argv[ ignstart ], igncnt,
					rel1->attrs[ i ].aname ) ||
				attr_compared( attr1num, keycnt, i + 1 ) ||
				attr_compared( &attr1num[ keycnt + showcnt ],
					cmpcnt, i + 1 ) )
			{
				continue;
			}
			j = findattr( rel2->attrs, rel2->attrcnt,
					rel1->attrs[i].aname );
			if ( j < 0 )
				continue;

			if ( attr_ignored( &argv[ ignstart ], igncnt,
					rel2->attrs[ j ].aname ) ||
				attr_compared( attr2num, keycnt, j + 1 ) ||
				attr_compared( &attr2num[ keycnt + showcnt ],
					cmpcnt, j + 1 ) )
			{
				continue;
			}

			attr1num[ keycnt + showcnt + cmpcnt ] = i + 1;
			attr2num[ keycnt + showcnt + cmpcnt ] = j + 1;
			cmpcnt++;
		}
	}

	fp1 = do_retrieve( rel1, rel1path, attr1num, keycnt, queryflags,
			wherelist, wherecnt );
	if ( fp1 == NULL )
	{
		prmsg( MSG_ERROR, "cannot create sub-process for query1" );
	}
	else
	{
		initformatio( &frmio1, fileno( fp1 ) );
	}

	fp2 = do_retrieve( rel2, rel2path, attr2num, keycnt, queryflags,
			wherelist, wherecnt );
	if ( fp2 == NULL )
	{
		prmsg( MSG_ERROR, "cannot create sub-process for query2" );
	}
	else
	{
		initformatio( &frmio2, fileno( fp2 ) );
	}

	identical = 0;
	different = 0;
	extra1 = 0;
	extra2 = 0;
	rcp1 = 0;
	rcp2 = 0;

	rc1 = getrec( &frmio1, query1->attrcnt, FLDDELIM, attrs1, FALSE );
	rc2 = getrec( &frmio2, query2->attrcnt, FLDDELIM, attrs2, FALSE );
	while( (rc1 || rc2) &&
		(frmio1.flags & FIO_ERROR) == 0 &&
		(frmio2.flags & FIO_ERROR) == 0 )
	{
		int cmp;

		/*
		 * If either query finished then close the pipe
		 * and check for query error now so that we
		 * do not print a zillion diffs that are worthless
		 * when one of the two queries failed.
		 */
		cmp = 0;
		if ( ( ! rc1 ) && ( fp1 ) && ( frmio1.flags & FIO_EOF ) ) {
			rcp1 = pclose( fp1 );
			fp1 = NULL;
			if ( rcp1 )
				++cmp;	/* query 1 failed */
		}
		if ( ( ! rc2 ) && ( fp2 ) && ( frmio2.flags & FIO_EOF ) ) {
			rcp2 = pclose( fp2 );
			fp2 = NULL;
			if ( rcp2 )
				++cmp;	/* query 2 failed */
		}
		if ( cmp )
			break;	/* one of the queries failed */

		cmp = cmp_keyattrs( attrs1, attrs2, attr1num, attr2num,
				keycnt, rc1, rc2 );
		if ( cmp < 0 )		/* extra tuple in rel1 */
		{
			if ( rep_extra1 )
				pr_extra( query1, 1, attrs1 );
			extra1++;
			rc1 = getrec( &frmio1, query1->attrcnt, FLDDELIM,
					attrs1, FALSE );
		}
		else if ( cmp > 0 )	/* extra tuple in rel2 */
		{
			if ( rep_extra2 )
				pr_extra( query2, 2, attrs2 );
			extra2++;
			rc2 = getrec( &frmio2, query2->attrcnt, FLDDELIM,
					attrs2, FALSE );
		}
		else
		{
			/*
			 * Records have indentical keys, check
			 * non-key attrs.
			 */
			if ( chk_identical( query1, query2,
					attrs1, attrs2, attr1num, attr2num,
					keycnt, showcnt, cmpcnt, rep_differ ) )
				identical++;
			else
				different++;

			rc1 = getrec( &frmio1, query1->attrcnt, FLDDELIM,
					attrs1, FALSE );
			rc2 = getrec( &frmio2, query2->attrcnt, FLDDELIM,
					attrs2, FALSE );
		}
	}

	fflush( stdout );

	rc1 = 0;
	if ( frmio1.flags & FIO_ERROR )
	{
		prmsg( MSG_ERROR, "I/O error occurred while reading pipe for query on '%s'",
			rel1path );
		rc1 = 2;
	}
	if ( frmio2.flags & FIO_ERROR )
	{
		prmsg( MSG_ERROR, "I/O error occurred while reading pipe for query on '%s'",
			rel2path );
		rc1 = 2;
	}

	if ( ( rcp1 ) || ( ( fp1 ) && ( pclose( fp1 ) != 0 ) ) )
	{
		char *p;
		if ( ( p = strrchr( rel1path, '=' ) ) != NULL )
			*p = '\0';
		prmsg( MSG_ERROR, "query on relation '%s' failed", rel1path );
		if ( p != NULL )
			*p = '=';
		rc1 = 2;
	}
	if ( ( rcp2 ) || ( ( fp2 ) && ( pclose( fp2 ) != 0 ) ) )
	{
		char *p;
		if ( ( p = strrchr( rel2path, '=' ) ) != NULL )
			*p = '\0';
		prmsg( MSG_ERROR, "query on relation '%s' failed", rel2path );
		if ( p != NULL )
			*p = '=';
		rc1 = 2;
	}

	if ( ! quiet && rc1 == 0 )
	{
		prmsg( MSG_ERRNOTE, "%d identical record%s", identical,
			identical != 1 ? "s" : "" );
		if ( different != 0 )
			prmsg( MSG_ERRNOTE, "%d different record%s", different,
				different != 1 ? "s" : "" );
		if ( extra1 != 0 )
			prmsg( MSG_ERRNOTE, "%d extra record%s in %s",
				extra1, extra1 != 1 ? "s" : "",
				rel1->path );
		if ( extra2 != 0 )
			prmsg( MSG_ERRNOTE, "%d extra record%s in %s",
				extra2, extra2 != 1 ? "s" : "",
				rel2->path );
	}

	exit( rc1 );
}

usage( )
{
	prmsg( MSG_USAGE, "[-e[1|2]] [-dqsvV] [-Q] <key_attr1> [to <key_attr2>] ... \\" );
	prmsg( MSG_CONTINUE, "[show <attr1> [to <attr2>] ...] \\" );
	prmsg( MSG_CONTINUE, "[compare [only] <attr1> [to <attr2>] ...] \\" );
	prmsg( MSG_CONTINUE, "[ignore <attr>...] \\" );
	prmsg( MSG_CONTINUE, "in <table1>[=<alt_table>] <table2>[=<alt_table>] \\" );
	prmsg( MSG_CONTINUE, "[where <where-clause>]" );

	exit( 1 );
}

cmp_keyattrs( attrs1, attrs2, attr1num, attr2num, keycnt, rc1, rc2 )
char **attrs1;
char **attrs2;
short *attr1num;
short *attr2num;
int keycnt;
int rc1;
int rc2;
{
	int i, cmp;

	if ( ! rc1 )
		return( 1 );	/* rel1 ran out of tuples */
	else if ( ! rc2 )
		return( -1 );	/* rel2 ran out of tuples */

	for( i = 0; i < keycnt; i++ ) {
		cmp = strcmp( attrs1[ attr1num[ i ] ],
				attrs2[ attr2num[ i ] ] );
		if ( cmp != 0 )
			return( cmp );
	}

	return( 0 );
}

pr_extra( query, relnum, attrs )
struct uquery *query;
int relnum;
char **attrs;
{
	int i;
	struct qprojection *projptr;

	prmsg( MSG_ASIS, "Extra tuple (#%s) in table '%s':\n",
		attrs[0], query->nodelist[0]->rel->path );
	projptr = &query->attrlist[1];
	attrs++;
	for( i = 1; i < query->attrcnt; i++, projptr++, attrs++ )
	{
		prmsg( MSG_ASIS, "\t%d.%s = '%s'\n",
			relnum, projptr->prname, *attrs );
	}
}

is_key( keys, keycnt, attrnum )
short *keys;
int keycnt;
short attrnum;
{
	int i;

	for( i = 0; i < keycnt; i++, keys++ )
	{
		if ( *keys == attrnum )
			return( TRUE );
	}

	return( FALSE );
}

chk_identical( query1, query2, attrs1, attrs2, attr1num, attr2num,
	keycnt, showcnt, cmpcnt, rep_differ )
struct uquery *query1;
struct uquery *query2;
char **attrs1;
char **attrs2;
short *attr1num;
short *attr2num;
int keycnt;
int showcnt;
int cmpcnt;
char rep_differ;
{
	int rc = TRUE;
	int no_msg = TRUE;
	int i;
	int j;
	int cmpstart;

	cmpstart = keycnt + showcnt;

	for( i = cmpstart; i < cmpstart + cmpcnt; i++ )
	{
		if ( strcmp( attrs1[ attr1num[ i ] ],
				attrs2[ attr2num[ i ] ] ) == 0 )
			continue;

		rc = FALSE;

		if ( ! rep_differ )
			continue;	/* Don't display differences */

		/*
		 * This attribute doesn't match.  Print out a message
		 * if not already done.
		 */
		if ( no_msg )
		{
			no_msg = FALSE;
			prmsg( MSG_ASIS, "Table 1 tuple #%s and table 2 tuple #%s differ:\n",
				attrs1[0], attrs2[0] );
			prmsg( MSG_ASIS, "    Key attributes:\n" );
			for( j = 0; j < keycnt; j++ )
			{
				/*
				 * The keys are identical, so only print
				 * one copy.
				 */
				prmsg( MSG_ASIS, "\t1.%s and 2.%s = '%s'\n",
					query1->attrlist[ attr1num[ j ] ].prname,
					query2->attrlist[ attr2num[ j ] ].prname,
					attrs1[ attr1num[ j ] ] );
			}

			if ( showcnt > 0 )
				prmsg( MSG_ASIS, "    Other attributes:\n" );

			for( j = keycnt; j < keycnt + showcnt; j++ )
			{
				prmsg( MSG_ASIS, "\t1.%s = '%s'\t2.%s = '%s'\n",
					query1->attrlist[ attr1num[ j ] ].prname,
					attrs1[ attr1num[ j ] ],
					query2->attrlist[ attr2num[ j ] ].prname,
					attrs2[ attr2num[ j ] ] );
			}

			prmsg( MSG_ASIS, "    Differing attributes:\n" );
		}

		prmsg( MSG_ASIS, "\t1.%s = '%s'\t2.%s = '%s'\n",
			query1->attrlist[ attr1num[ i ] ].prname,
			attrs1[ attr1num[ i ] ],
			query2->attrlist[ attr2num[ i ] ].prname,
			attrs2[ attr2num[ i ] ] );
	}

	return( rc );
}

int
findclause( argc, argv, curarg, savecount, clause )
int argc;
char **argv;
int curarg;
int *savecount;
int *clause;
{
	int cnt;
	int skip;	/* keywords to skip */

	*clause = 0;
	skip = 0;
	for( cnt = curarg; cnt < argc; cnt++ )
	{
		if ( strcmp( argv[cnt], "in" ) == 0 )
		{
			*clause = CL_RELNAMES;
			skip = 1;		/* skip keyword */
			break;
		}
		else if ( strcmp( argv[cnt], "compare" ) == 0 )
		{
			if ( cnt < argc - 1 &&
				strcmp( argv[cnt + 1], "only" ) == 0 )
			{
				*clause = CL_COMPARE;
				skip = 2;	/* skip both keywords */
			}
			else
			{
				*clause = CL_ALLCOMPARE;
				skip = 1;	/* skip keyword */
			}
			break;
		}
		else if ( strcmp( argv[cnt], "show" ) == 0 )
		{
			*clause = CL_SHOW;
			skip = 1;		/* skip keyword */
			break;
		}
		else if ( strcmp( argv[cnt], "ignore" ) == 0 )
		{
			*clause = CL_IGNORE;
			skip = 1;		/* skip keyword */
			break;
		}
	}

	*savecount = cnt - curarg;

	return( cnt + skip );
}

int
parse_attrs( args, cnt, rel1, rel2, attr1num, attr2num )
char **args;
int cnt;
struct urelation *rel1;
struct urelation *rel2;
short *attr1num;
short *attr2num;
{
	int i;
	int attrcnt;
	char remap_attr;	/* is 'to' keyword recognized? */
	char mustmap_attr;	/* is 'to' keyword required? */

	remap_attr = FALSE;
	mustmap_attr = FALSE;
	attrcnt = 0;
	for( i = 0; i < cnt; i++ )
	{
		if ( strcmp( args[ i ], "to" ) == 0 &&
			(mustmap_attr || remap_attr) )
		{
			if ( ++i >= cnt )
			{
				prmsg( MSG_ERROR, "no second attribute given after 'to' keyword" );
				usage( );
			}

			remap_attr = FALSE;
			mustmap_attr = FALSE;

			attr2num[ attrcnt - 1 ] = findattr( rel2->attrs,
							rel2->attrcnt,
							args[ i ] ) + 1;
			if ( attr2num[ attrcnt - 1 ] <= 0 )
			{
				if ( strcmp( args[i], "rec#" ) == 0 )
					attr2num[ attrcnt - 1 ] = 0;
				else
				{
					prmsg( MSG_ERROR, "unrecognized attribute '%s' for relation '%s'",
						args[i], rel2->path );
					usage( );
				}
			}
		}
		else if ( mustmap_attr )
		{
			prmsg( MSG_ERROR, "unrecognized attribute '%s' for relation '%s'",
				args[ i - 1 ], rel2->path );
			usage( );
		}
		else	/* normal attribute */
		{
			attr1num[ attrcnt ] = findattr( rel1->attrs,
							rel1->attrcnt,
							args[ i ] ) + 1;
			if ( attr1num[ attrcnt ] <= 0 )
			{
				if ( strcmp( args[i], "rec#" ) == 0 )
					attr1num[ attrcnt ] = 0;
				else
				{
					prmsg( MSG_ERROR, "unrecognized attribute '%s' for relation '%s'",
						args[i], rel1->path );
					usage( );
				}
			}

			mustmap_attr = FALSE;
			remap_attr = TRUE;

			attr2num[ attrcnt ] = findattr( rel2->attrs,
							rel2->attrcnt,
							args[ i ] ) + 1;
			if ( attr2num[ attrcnt ] <= 0 )
			{
				if ( strcmp( args[i], "rec#" ) == 0 )
					attr2num[ attrcnt ] = 0;
				else
					mustmap_attr = TRUE;
			}

			++attrcnt;
		}
	}

	return( attrcnt );
}

attr_compared( attrs, cnt, num )
short *attrs;
int cnt;
short num;
{
	while( cnt-- > 0 )
	{
		if ( *attrs == num )
			return( TRUE );
		attrs++;
	}

	return( FALSE );
}

attr_ignored( attrs, cnt, aname )
char **attrs;
int cnt;
char *aname;
{
	while( cnt-- > 0 )
	{
		if ( strcmp( *attrs, aname ) == 0 )
			return( TRUE );
		attrs++;
	}

	return( FALSE );
}

FILE *
do_retrieve( relptr, relpath, attrnums, keycnt, queryflags, where, wherecnt )
struct urelation *relptr;
char *relpath;
short *attrnums;
short keycnt;
int queryflags;
char **where;
int wherecnt;
{
	char cmd[ 1024 ];
	char *ptr;
	short i;

	ptr = cmd;

	sprintf( ptr, "retrieve -q %s -d'%c' rec# all:nodisplay=:display ", Qoption, FLDDELIM );
	ptr += strlen( ptr );

	if ( queryflags & Q_SORT )
	{
		strcpy( ptr, "sorted by " );
		ptr += strlen( ptr );

		for( i = 0; i < keycnt; i++ )
		{
			strcpy( ptr, relptr->attrs[ attrnums[ i ] - 1 ].aname );
			ptr += strlen( ptr );
			*ptr++ = ' ';
		}
	}

	sprintf( ptr, "from %s", relpath );

	if ( wherecnt > 0 && where != NULL )
	{
		ptr += strlen( ptr );
		strcpy( ptr, " where" );

		for( i = 0; i < wherecnt; i++ )
		{
			ptr += strlen( ptr );
			sprintf( ptr, " '%s'", where[i] );
		}
	}

	return( popen( cmd, "r" ) );
}
