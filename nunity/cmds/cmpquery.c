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
#include <ctype.h>
#include "uquery.h"
#include "message.h"

/*
 * Definitions for saying what the next type of clause (attribute list
 * or where conditions) is on the command line.
 */
#define CL_WHERE	1	/* where clause is next on cmd line */
#define CL_SORT		2	/* list of attributes to sort on */

extern char *calloc();
extern struct uquery *fmkquery();
extern void prexpr(), prtree();
extern char *basename();

char *prog;

extern char uversion[];		/* version string */

main( argc, argv )
int argc;
char *argv[];
{
	int curarg;
	char *qname;
	char **attrnames, **relnames, **sortnames;
	int attrcnt, relcnt, sortcnt;
	struct uquery *query;
	char *str;
	int cnt;
	char queryflags;	/* flags for query */
	char clause, doheader, docode;
	char *include_prefix;
	char *outfile;
	FILE *fp;
	char name[MAXPATH+4];	/* allow for "/./" or "././" prefix */


	prog = basename( *argv );

	/*
	 * Parse any command-line options given.
	 */
	doheader = FALSE;
	docode = FALSE;
	include_prefix = "";	/* no directory for #includes */
	queryflags = 0;
	outfile = NULL;

	for( curarg = 1; curarg < argc; curarg++ ) {
		char *option;

		option = argv[curarg];
		if ( *option != '-' ) {
#ifdef DEBUG
			if ( strncmp( option, "debug=", 6 ) == 0 ) {
				(void)set_qdebug( &option[6] );
				continue;
			}
#endif /* DEBUG */
			break;
		}

		while( *++option ) {
			switch( *option ) {
			case 'c':
				docode = TRUE;
				break;
#ifdef DEBUG
			case 'd':
				if ( strcmp( option, "debug" ) == 0 ) {
					(void)set_qdebug( "all" );
					option = "\00\00";
					break;
				}
				else
					badoption( *option );
				break;
#endif
			case 'h':
				doheader = TRUE;
				break;
			case 'i':
				include_prefix = &option[1];
				option = "\00\00";
				break;
			case 'o':
				if ( option[1] ) {
					outfile = &option[1];
					option = "\00\00";
				}
				else if ( ++curarg < argc ) 
					outfile = argv[curarg];
				else {
					prmsg( MSG_ERROR, "no output file given with '-o' option" );
					usage( );
				}
				break;
			case 's':		/* sort the output */
				queryflags ^= Q_SORT;
				break;
			case 'V':
				prmsg( MSG_NOTE, "%s", uversion );
				exit( 0 );
				break;
			case 'u':
				queryflags ^= Q_UNIQUE;
				break;
			default:
				badoption( *option );
			}
		}
	}

	if ( ! docode && ! doheader ) {
		docode = TRUE;
		doheader = TRUE;
	}
	if ( strcmp( include_prefix, "-" ) == 0 ) {
		/*
		 * Don't put any #include in the generated header files
		 */
		include_prefix = NULL;
	}

	if ( argc - curarg < 3 )  /* minimum is "cmpquery qname on db" */
		usage( );

	qname = argv[curarg++];		/* save the query name */
	/*
	 * Check the query name to see if it is legit.
	 */
	if ( strlen( qname ) == 0 ) {
		prmsg( MSG_ERROR, "null query name given." );
		usage( );
	}
	if ( ! isalpha( *qname ) && *qname != '_' ) {
		prmsg( MSG_ERROR, "illegal character '%c' in query name",
			*qname );
		usage( );
	}
	for( str = qname + 1; *str; str++ ) {
		if ( ! isalnum( *str ) && *str != '_' ) {
			prmsg( MSG_ERROR, "illegal character '%c' in query name",
				*str );
			usage( );
		}
	}

	if ( strcmp( argv[curarg++], "on" ) != 0 )
		usage( );

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
	}
	if ( relcnt == 0 ) {	/* Gotta have a relation */
		prmsg( MSG_ERROR, "no relations given on command line" );
		usage( );
	}
	relnames = &argv[curarg];
	relcnt -= curarg;
	curarg += relcnt;
	if ( clause )
		curarg++;		/* skip over "where" */

	query = fmkquery( queryflags | Q_NOEXPAND, relnames, relcnt,
			attrnames, attrcnt, sortnames, sortcnt,
			(clause == CL_WHERE) ? &argv[ curarg ] : NULL,
			argc - curarg );
	if ( query == NULL )
	{
		(void)pruerror( );
		prmsg( MSG_ERROR, "query compilation failed" );
		exit( 1 );
	}

	if ( doheader ) {
		if ( ! genheader( qname, query, include_prefix ) ) {
			prmsg( MSG_ERROR, "cannot write to header file %s.h",
				qname );
			exit( 2 );
		}
	}

	if ( docode ) {
		if ( outfile == NULL ) {
			sprintf( name, "%s.c", qname );
			outfile = name;
			fp = fopen( outfile, "w" );
		}
		else if ( strcmp( outfile, "-" ) == 0 )
			fp = stdout;
		else
			fp = fopen( outfile, "w" );
		if ( fp == NULL ) {
			prmsg( MSG_ERROR, "cannot open output file %s",
				outfile );
			exit( 2 );
		}

		fputs( "/*\n * THIS IS A GENERATED FILE!!!\n *\n * This file contains the data representation of the query:\n\n",
			fp );
#if 0
		prexpr( fp, qptr, TRUE );
		putc( '\n', fp );
		prtree( fp, qptr, FALSE );
#endif
		fputs( "\n */\n\n", fp );
		if ( include_prefix )
			fprintf( fp, "#include \"%s%suquery.h\"\n\n",
				include_prefix,
				*include_prefix ? "/" : "" );

		if ( ! genquery( fp, qname, query ) ) {
			prmsg( MSG_INTERNAL, "cannot generate query code -- ran out of memory???" );
			exit( 2 );
		}
	}

	exit( 0 );
}

badoption( c )
char c;
{
	prmsg( MSG_ERROR, "unrecognized option '%c'", c );
	usage( );
}

usage( )
{
	prmsg( MSG_USAGE, "[-chsVu] [-i[-|<include_dir>]] [-o <output_file>] \\" );
	prmsg( MSG_CONTINUE, "<query_name> on [<attr>[:<modifiers>] [as <newname>]...] \\" );
	prmsg( MSG_CONTINUE, "[sorted [by <attr>[:<modifiers>...] ...]] [unique] \\" );
	prmsg( MSG_CONTINUE, "from <tables>... [where <where-clause>]" );
	prmsg( MSG_CONTINUE, "\nNOTE:  Any <include_dir> must immediately follow the '-i' without any\nspaces in between." );

	exit( 1 );
}
