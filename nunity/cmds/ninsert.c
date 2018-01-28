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
#include "uquery.h"
#include "message.h"
#include "permission.h"

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
extern char *calloc();
#endif
extern char *basename();
extern struct urelation *getrelinfo();
extern char *copystr();		/* unity attribute value save routine */
extern void setunpackenv();

int readfromrel();		/* functions for reading tuples */
int prompttuple();

RETSIGTYPE catchsig();

#define UI_CONSTANT	0x01	/* dest attr is a constant value */
#define UI_RECNUM	0x02	/* dest attr is a record number */

struct insattrinfo {
	char flags;		/* flags -- see above */
	char *destattr;		/* destination attr name */
	char *srcattr;		/* src attr name if not constant value */
	short sattrnum;		/* src attr number if not constant */
};

extern char uversion[];		/* version string */

extern int  uchecktuple;	/* if set readtuple() will count/report tuple errors */
extern long utplerrors; 	/* no. of tuples read with tuple errors */
extern long utplelimit; 	/* limit on number of tuple errors reported */
extern int  utplmsgtype;	/* MSG_ERROR or MSG_WARN type tuple error message */

char *prog;
char zerowidth = TRUE;
char newline = '~';
struct urelio srcrelio;

static unsigned long recnum;	/* rec# from source table */
static unsigned long rectotal;	/* total records in the output table */
char recnumfmt[8];		/* print format for rec# in recnumbuf[] */
char RecNumfmt[8];		/* print format for REC# in RecNumbuf[] */
char recnumbuf[12];		/* buffer to print rec# from input table */
char RecNumbuf[12];		/* buffer to print REC# being added to the output table */


main( argc, argv )
int argc;
char *argv[];
{
	char quiet;
	char create_descr;
	char descr_wtbl = FALSE;
	char delimiter;
	char delimiter_set;
	char noexist;
	char exact_cnt;
	int (*readfunc)();
	int record_cnt;

	char mapdone, allconstant, match_attrs;
	register int i, cnt;
	register struct uattribute *attrptr;
	register struct insattrinfo *infoptr;
	char *srctable;
	char *desttable, *destdescr, *equalsign;
	char **attrnames;
	short attrcnt;
	short recnumcnt;
	struct uinsertinfo *insptr;
	struct urelation srcrel;
	struct urelation destrel;
	struct urelation *allrelptr;
	struct insattrinfo attrinfo[MAXATT];
	char *fldlist[MAXATT];


	prog = basename( *argv );

	if ( argc < 2 )
		usage( );

	quiet = FALSE;
	delimiter = '\0';
	delimiter_set = FALSE;
	create_descr = FALSE;
	readfunc = readfromrel;
	record_cnt = 0;
	exact_cnt = FALSE;

	while( --argc > 0 && **++argv == '-') {
		char *option;

		for( option = &argv[0][1]; *option; option++ ) {
			switch( *option ) {
			case 'c':
				create_descr = TRUE;
				break;
			case 'C':
				create_descr = TRUE;
				descr_wtbl ^= TRUE;
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
			case 'd':
				if ( option[1] ) {
					cnvtbslsh( &option[1], &option[1] );
					delimiter = option[1];
					delimiter_set = TRUE;
					option = "\0\0";
				}
				break;
			case 'p':
				readfunc = prompttuple;
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
			case 'F':
			case 'f':
				i = option[0];
				if ( option[1] ) {
					cnt = atoi( &option[1] );
					option = "\0";
				}
				else if ( argc > 0 ) {
					--argc;
					cnt = atoi( *++argv );
				}
				else
					cnt = 0;
				if (( cnt <= 0 ) || ( cnt > 10 )) {
					prmsg( MSG_ERROR, "precision of record number for -%c must be between 1 and 10", (char) i );
					usage( );
				} else {
					if ( i == 'F' ) {
						sprintf( RecNumfmt, "%%0%dlu", cnt );
					} else {
						sprintf( recnumfmt, "%%0%dlu", cnt );
					}
				}
				break;
			case 'm':
			case 'r':
				exact_cnt = (*option == 'r');

				if ( option[1] ) {
					record_cnt = atoi( &option[1] );
					option = "\0";
				}
				else if ( argc > 0 ) {
					--argc;
					record_cnt = atoi( *++argv );
				}
				else
					record_cnt = 0;
				if ( record_cnt <= 0 ) {
					prmsg( MSG_ERROR, "unrecognized record count for -r or -m" );
					usage( );
				}
				break;
			case 'V':
				prmsg( MSG_NOTE, "%s", uversion );
				exit( 0 );
			case 'z':
				zerowidth = FALSE;
				break;
			default:
				prmsg( MSG_ERROR, "unrecognized option '%c'",
					*option );
				usage( );
			}
		}
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

	/*
	 * Initialize attribute count to zero (0)
	 * in source and destination relation structures
	 * so we can test if the relation information
	 * is available when attempting to process the
	 * special "all" attribute.
	 */
	srcrel.attrcnt = 0;
	destrel.attrcnt = 0;

	/*
	 * Now count any attribute names for the insertion
	 */
	attrcnt = 0;
	attrnames = argv;
	while( argc-- > 0 ) {
		char *name;

		name = *argv++;
		if ( strcmp( name, "into" ) == 0 ) {
			noexist = TRUE;
			break;
		}
		else if ( strcmp( name, "in" ) == 0 ||
				strcmp( name, "onto" ) == 0 ) {
			noexist = FALSE;
			break;
		}
		++attrcnt;
	}

	if ( argc <= 0 ) {	/* gotta have a table name */
		prmsg( MSG_ERROR, "no table listed on command line to insert into" );
		usage( );
	}

	/*
	 * Check the table to insert into.
	 */
	desttable = *argv++;
	--argc;
	destdescr = equalsign = strchr( desttable, '=' );
	if ( destdescr )
		*destdescr++ = '\0';
	else
		destdescr = desttable;
	if ( chkperm( desttable, P_EXIST ) )
	{
		if ( noexist ) {
			prmsg( MSG_ERROR, "table %s already exists", desttable );
			usage( );
		}
		else if ( create_descr ) {
			prmsg( MSG_ERROR, "table %s already exists -- cannot create descriptor file",
				desttable );
			usage();
		}

		if ( ! chkupdate( desttable ) ) {
			prmsg( MSG_ERROR, "no permission to update table '%s'",
				desttable );
			exit( 2 );
		}
	}

	srctable = NULL;
	if ( argc > 0 && strcmp( *argv, "from" ) == 0 )
	{
		if ( argc < 2 )
		{
			prmsg( MSG_ERROR, "no table name given after 'from' keyword" );
			usage( );
		}

		readfunc = readfromrel;

		srctable = *++argv;
		++argv;
		argc -= 2;
		if ( ! getrelinfo( srctable, &srcrel, FALSE ))
		{
			prmsg( MSG_ERROR, "cannot get descriptor information for table '%s'",
				srctable );
			exit( 2 );
		}
	}
	else if ( argc > 0 && **argv == 'p' )
	{
		--argc;
		++argv;
		readfunc = prompttuple;
	}

	if ( argc > 0 )
	{
		prmsg( MSG_ERROR, "unrecognized extra option '%s'", *argv );
		usage( );
	}


	/*
	 * If the description file doesn't exist yet, then we'll create it,
	 * as long as there are attributes on the command line or a src table
	 * to do it from.  If an alternate table/description was given,
	 * we must pass getrelinfo() the table name with the =<alt_table>
	 * so that it can determine whether or not the user specifed that
	 * the data directory should be searched first for the desctription.
	 */
	if ( equalsign )
		*equalsign = '=';
	if ( ! create_descr && ! getrelinfo( desttable, &destrel, FALSE ) )
	{
		if ( equalsign )
			*equalsign = '\0';
		if ( attrcnt > 0 || srctable ) {
			create_descr = TRUE;
			reset_uerror( );	/* clear uerror set by getrelinfo( ) */
		} else {
			(void)pruerror( );
			prmsg( MSG_ERROR, "cannot get description info for table '%s'",
				destdescr );
			usage( );
		}
	}
	else
	{
		if ( equalsign )
			*equalsign = '\0';
		destrel.path = desttable;
	}

	/*
	 * Set up the insertion attribute information.
	 */
	allconstant = ( attrcnt != 0 );
	allrelptr = NULL;
	recnumcnt = 0;
	cnt = 0;
	for( i = 0, infoptr = attrinfo; i < attrcnt; i++ ) {
		char *ptr;

		if ( strcmp( attrnames[i], "as" ) == 0 )
		{
			if ( cnt == 0 ) {
				prmsg( MSG_ERROR, "no initial attribute given in renaming" );
				usage( );
			}
			if ( i + 1 >= attrcnt ) {
				prmsg( MSG_ERROR, "no new attribute name given after 'as'" );
				usage( );
			}
			else if ( infoptr[-1].flags & UI_CONSTANT ) {
				prmsg( MSG_ERROR, "cannot supply constant value and rename attribute" );
				usage();
			}
			else if ( infoptr[-1].srcattr ) {
				prmsg( MSG_ERROR, "cannot rename an attribute more than once" );
				usage();
			}
			infoptr[-1].srcattr = infoptr[-1].destattr;
			infoptr[-1].destattr = attrnames[++i];
			if ( ( infoptr[-1].flags & UI_RECNUM ) == 0 ) {
				allconstant = FALSE;
			}
		}
		else if ( ( strcmp( attrnames[i], "all" ) == 0 ) ||
			  ( strncmp( attrnames[i], "all:nodisplay=", 14) == 0 ) )
		{
			if ( allrelptr ) {
				prmsg( MSG_ERROR, "cannot reference special 'all' attribute more than once" );
				exit( 1 );
			}
			if ( cnt >= MAXATT ) {
				prmsg( MSG_ERROR, "too many projected attributes -- max is %d", MAXATT );
				exit( 1 );
			}

			allconstant = FALSE;

			if ( ( create_descr == TRUE ) &&
			     ( srctable ) && ( srcrel.attrcnt > 0 ) &&
			   ( ( attrnames[i][3] != '\0' ) ||
			     ( findattr( srcrel.attrs, srcrel.attrcnt, attrnames[i] ) < 0 ) ) )
			{
				allrelptr = &srcrel;
			}
			else if ( ( create_descr == TRUE ) ||
				  ( destrel.attrcnt == 0 ) ||
				( ( findattr( destrel.attrs, destrel.attrcnt, "all" ) >= 0 ) ) )
			{
				infoptr->destattr = attrnames[i];
				infoptr->flags = 0;
				infoptr->srcattr = NULL;
				infoptr->sattrnum = -1;
				infoptr++;
				cnt++;
			}
			else
			{
				allrelptr = &destrel;
			}

			if ( allrelptr )
			{
				register int j;
				int allcnt;
				char skipattr[MAXATT];

				if ( ( attrnames[i][3] == '\0' ) && ( zerowidth ) )
				{
					allcnt = j = allrelptr->attrcnt;
					while ( j != 0 )
						skipattr[ --j ] = FALSE;
				}
				else
				{
					allcnt = 0;
					for ( j = 0; j < allrelptr->attrcnt; j++ )
					{
						if ( ( zerowidth ) ||
						   ( ( allrelptr->attrs[j].prwidth != 0 ) &&
						     ( ! isupper( allrelptr->attrs[j].justify ) ) ) ) {
							skipattr[ j ] = FALSE;
							++allcnt;
						} else {
							skipattr[ j ] = TRUE;
						}
					}
					if (( attrnames[i][3] != '\0' ) && ( attrnames[i][14] != '\0' ))
					{
						char *p, *comma;

						p = &attrnames[i][14];
						while ( p != NULL ) {
							comma = strchr( p, ',' );
							if ( comma ) {
								*comma = '\0';
							}
							j = findattr( allrelptr->attrs, allrelptr->attrcnt, p );
							if ( j < 0 ) {
								prmsg( MSG_ERROR, "unrecognized attribute '%s' given for 'all:nodisplay='", p );
								exit( 1 );
							}
							if ( skipattr[ j ] == FALSE ) {
								skipattr[ j ] = TRUE;
								--allcnt;
							}
							if ( comma ) {
								*comma++ = ',';
							}
							p = comma;
						}
					}
					if ( allcnt <= 0 ) {
						prmsg( MSG_ERROR, "all attributes are marked nodisplay or zero-width" );
						exit( 1 );
					}
				}

				if ( ( cnt + allcnt ) > MAXATT ) {
					prmsg( MSG_ERROR, "too many projected attributes -- max is %d", MAXATT );
					exit( 1 );
				}
				if ( ( i + 1 < attrcnt ) &&
				     ( strcmp( attrnames[i+1], "as" ) == 0 ) ) {
					prmsg( MSG_ERROR, "cannot rename special 'all' attribute" );
					exit( 1 );
				}
				for ( j = 0; j < allrelptr->attrcnt; j++ ) {
					if ( skipattr[ j ] == FALSE ) {
						infoptr->destattr = allrelptr->attrs[j].aname;
						infoptr->flags = 0;
						infoptr->srcattr = NULL;
						infoptr->sattrnum = -1;
						infoptr++;
						cnt++;
					}
				}
				continue;
			}
		}
		else
		{
			if ( cnt >= MAXATT ) {
				prmsg( MSG_ERROR, "too many projected attributes -- max is %d", MAXATT );
				exit( 1 );
			}
			infoptr->destattr = attrnames[i];
			infoptr->flags = 0;
			infoptr->srcattr = NULL;
			infoptr->sattrnum = -1;
			infoptr++;
			cnt++;
		}
		if ( (ptr = strchr( attrnames[i], '=' )) != NULL ) {
			if ( infoptr[-1].srcattr ) {
				prmsg( MSG_ERROR, "cannot supply constant value and rename attribute" );
				usage();
			}
			*ptr++ = '\0';
			infoptr[-1].flags |= UI_CONSTANT;
			fldlist[cnt - 1] = ptr;
		}
		else if ( strcmp( attrnames[i], "rec#" ) == 0 ) {
			infoptr[-1].flags |= UI_RECNUM;
			fldlist[cnt - 1] = recnumbuf;
			if ( recnumfmt[0] == '\0' ) {
				sprintf( recnumfmt, "%%lu" );
			}
			++recnumcnt;
		}
		else if ( strcmp( attrnames[i], "REC#" ) == 0 ) {
			infoptr[-1].flags |= UI_RECNUM;
			fldlist[cnt - 1] = RecNumbuf;
			if ( RecNumfmt[0] == '\0' ) {
				sprintf( RecNumfmt, "%%lu" );
			}
			++recnumcnt;
		}
		else {
			if ( ( infoptr[-1].flags & UI_RECNUM ) == 0 )
				allconstant = FALSE;
		}
	}
	attrcnt = cnt;
	mapdone = allconstant;

	/*
	 * Make sure that not all attributes are record number attributes.
	 *
	 * Note: If all attributes are based on record number and
	 *	 we are to prompt for the input then there is nothing
	 *	 left to prompt for so we will make it a restriction
	 *	 no matter where the input comes from.
	 */
	if ( ( recnumcnt != 0 ) && ( recnumcnt == attrcnt ) ) {
		prmsg( MSG_ERROR, "cannot have all attributes based on record number (rec# or REC#)" );
		exit( 1 );
	}

	/*
	 * Set up the source attribute information.
	 *
	 * If a source table was not given, set up the source relation.
	 * Of course if all attribute values were given on the command line,
	 * there's no need to do this.
	 */
	if ( srctable == NULL && ! allconstant ) {
		/*
		 * Get things from standard input.  In order to do
		 * this we set up relation info from stdin.
		 */
		if ( delimiter_set == FALSE )
			delimiter = '\n';
		srcrel.path = "-";
		srcrel.dpath = NULL;
		srcrel.relflgs = 0;

		if ( attrcnt == 0 )
		{
			if ( create_descr ) {
				prmsg( MSG_ERROR, "cannot create description when no source table and no attributes given on command line" );
				exit( 1 );
			}
			/*
			 * Take the attributes straight from the destination
			 * relation.  (We've already made sure there is a
			 * destination description.
			 * We also set up the straight accross
			 * mapping from destination to source attributes.
			 */
			attrptr = (struct uattribute *)calloc( destrel.attrcnt,
						sizeof( struct uattribute ) );
			srcrel.attrs = attrptr;
			srcrel.attrcnt = destrel.attrcnt;
			cnt = 0;
			infoptr = attrinfo;
			for( i = 0; i < srcrel.attrcnt; i++, attrptr++ ) {
				*attrptr = destrel.attrs[i];
				if ( readfunc != prompttuple ||
						attrptr->attrtype == UAT_TERMCHAR ) {
					attrptr->attrtype = UAT_TERMCHAR;
					attrptr->terminate = delimiter;
				}
				if ( ( zerowidth ) ||
				   ( ( attrptr->prwidth != 0 ) && ( ! isupper( attrptr->justify ) ) ) ) {
					infoptr->destattr = attrptr->aname;
					infoptr->srcattr = attrptr->aname;
					infoptr->flags = 0;
					infoptr->sattrnum = i;
					infoptr++;
					cnt++;
				}
			}
			if ( cnt == 0 ) {
				prmsg( MSG_ERROR, "all attributes are marked nodisplay or zero-width" );
				exit( 1 );
			}
			attrcnt = cnt;
			mapdone = TRUE;	/* map src to dest attrs is done */
			if ( srcrel.attrs[srcrel.attrcnt - 1].attrtype == UAT_TERMCHAR ) {
				srcrel.attrs[srcrel.attrcnt - 1].terminate = '\n';
			}
		}
		else {
			/*
			 * Create a relation description based on the
			 * source attributes mentioned on the command line.
			 * We'll also set up the mapping from destination
			 * attributes to source attributes.
			 */
			attrptr = (struct uattribute *)calloc( (unsigned)attrcnt,
						sizeof( struct uattribute ) );
			srcrel.attrs = attrptr;
			srcrel.attrcnt = 0;
			for( i = 0, infoptr = attrinfo; i < attrcnt;
					i++, infoptr++ ) {
				char *attrname;

				if ( infoptr->flags & (UI_CONSTANT|UI_RECNUM) )
					continue;
				attrname = infoptr->srcattr ?
						infoptr->srcattr :
						infoptr->destattr;
				infoptr->sattrnum = findattr( srcrel.attrs,
								srcrel.attrcnt,
								attrname );
				if ( infoptr->sattrnum < 0 ) {
					/*
					 * This attribute hasn't been used yet.
					 * Add it to the relation and save the
					 * index.
					 */
					strncpy( attrptr->aname, attrname,
						ANAMELEN );
					attrptr->friendly = infoptr->srcattr;
					attrptr->attrtype = UAT_TERMCHAR;
					attrptr->justify = 'l';
					attrptr->terminate = delimiter;
					attrptr->prwidth = 10;
					attrptr++;
					infoptr->sattrnum = srcrel.attrcnt++;
				}
			}
			if ( srcrel.attrs[srcrel.attrcnt - 1].attrtype == UAT_TERMCHAR ) {
				srcrel.attrs[srcrel.attrcnt - 1].terminate = '\n';
			}
			mapdone = TRUE;	/* map src to dest attrs is done */
		}
	}

	/*
	 * Initialize the I/O for the source relation if necessary.
	 */
	if ( readfunc != prompttuple && ! allconstant &&
		! relropen( &srcrelio, &srcrel, (FILE *)NULL ) )
	{
		prmsg( MSG_ERROR, "cannot open I/O for relation '%s'",
			srcrel.path );
		exit( 2 );
	}

	/*
	 * Set up the destination attribute information.
	 */
	if ( create_descr )
	{
		/*
		 * Create the descriptor file.
		 */
		if ( srctable == NULL && attrcnt == 0 ) {
			prmsg( MSG_ERROR, "cannot read attribute info for table '%s' and no attributes given",
				destdescr );
			exit( 2 );
		}
		if ( attrcnt == 0 ) {
			/*
			 * Use the source table as the description for
			 * the destination table.
			 */
			destrel.attrs = srcrel.attrs;
			destrel.attrcnt = srcrel.attrcnt;
		}
		else {
			/*
			 * Use the attributes on the command line as the
			 * attributes of the new relation.
			 */
			attrptr = (struct uattribute *)calloc( (unsigned)attrcnt,
						sizeof( struct uattribute ) );
			destrel.attrs = attrptr;
			destrel.attrcnt = attrcnt;
			for( i = 0, infoptr = attrinfo; i < attrcnt;
					i++, attrptr++, infoptr++ ) {
				if ( ! chkaname( infoptr->destattr ) ) {
					prmsg( MSG_ERROR, "illegal attribute name '%s'",
						infoptr->destattr );
					exit( 1 );
				}
				strncpy( attrptr->aname, infoptr->destattr,
					ANAMELEN );
				attrptr->attrtype = UAT_TERMCHAR;
				attrptr->justify = 'l';
				attrptr->terminate = delimiter_set ? delimiter : ':';
				attrptr->friendly = NULL;
				attrptr->prwidth = 10;
			}
			destrel.attrs[attrcnt - 1].terminate = '\n';
		}

		destrel.relflgs = 0;
		destrel.path = destdescr;
		destrel.dpath = NULL;
		if ( descr_wtbl )
			destrel.relflgs |= UR_DESCRWTBL;
		else if ( ! writedescr( &destrel, NULL ) )
		{
			(void)pruerror();
			prmsg( MSG_ERROR, "cannot write descriptor information for table '%s'",
				destdescr );
			exit( 2 );
		}
		destrel.path = desttable;
	}

	/*
	 * If no attributes were given on the command line.  Then
	 * the insertion attributes are the non-zero length attributes
	 * in the destination description.
	 */
	if ( attrcnt == 0 )
	{
		match_attrs = TRUE;
		cnt = 0;
		infoptr = attrinfo;
		for( i = 0, attrptr = destrel.attrs; i < destrel.attrcnt;
				i++, attrptr++ )
		{
			if ( ( zerowidth ) ||
			   ( ( attrptr->prwidth != 0 ) && ( ! isupper( attrptr->justify ) ) ) ) {
				infoptr->flags = 0;
				infoptr->destattr = attrptr->aname;
				infoptr->srcattr = attrptr->aname;
				infoptr->sattrnum = -1;
				infoptr++;
				cnt++;
			}
		}
		if ( cnt == 0 ) {
			prmsg( MSG_ERROR, "all attributes are marked nodisplay or zero-width" );
			exit( 1 );
		}
		attrcnt = cnt;
		mapdone = FALSE;
	}
	else
		match_attrs = FALSE;

	/*
	 * Look up each destination attribute and make sure we
	 * can find the source attribute it corresponds to.
	 */
	if ( ! mapdone )
	{
		for( i = 0, infoptr = attrinfo; i < attrcnt; i++, infoptr++ )
		{
			char *attrname;

			if ( infoptr->flags & (UI_CONSTANT|UI_RECNUM) )
				continue;
			attrname = infoptr->srcattr ? infoptr->srcattr :
					infoptr->destattr;
			infoptr->sattrnum = findattr( srcrel.attrs,
							srcrel.attrcnt,
							attrname );
			if ( infoptr->sattrnum < 0 )
			{
				/*
				 * We have an unknown attribute.  If
				 * explicit attributes were given on
				 * the command line then this is an error
				 * (since we can't find the listed attribute).
				 * If no attributes were given, then we
				 * use an empty string as the attribute
				 * value.
				 */
				if ( ! match_attrs )
				{
					prmsg( MSG_ERROR, "cannot locate destination attribute '%s' in source relation",
						attrname );
					usage();
				}
				else
				{
					fldlist[i] = "";
					infoptr->flags |= UI_CONSTANT;
				}
			}
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

	/*
	 * Check/Count tuples if REC# was specified
	 * or if "into <table>" was specified so that
	 * we can eliminate the race condition that
	 * exists since we have to test if the output
	 * file already exisits before init_insert()
	 * locks the table and then creates the output
	 * file if it does not exist.
	 */
	if ( ( RecNumfmt[0] != '\0' ) || ( noexist == TRUE ) ) {
		insptr = init_insert_tc( &destrel, "", &rectotal );
	} else {
		insptr = init_insert( &destrel, "" );
	}
	if ( insptr == NULL ) {
		(void)pruerror( );
		prmsg( MSG_ERROR, "cannot initialize insert for table '%s'",
			desttable );
		exit( 2 );
	}
	if ( ( noexist == TRUE ) && ( rectotal != 0 ) ) {
		prmsg( MSG_ERROR, "table %s already exists and contains %lu tuple(s)", desttable, rectotal );
		(void)end_insert( insptr, FALSE );
		exit( 1 );
	}

	for( i = 0, infoptr = attrinfo; i < attrcnt; i++, infoptr++ ) {
		if ( ! addattr( insptr, infoptr->destattr ) ) {
			(void)pruerror( );
			prmsg( MSG_ERROR, "cannot do insertion for attribute '%s' in table '%s'",
				infoptr->destattr, desttable );
			(void)end_insert( insptr, FALSE );
			exit( 1 );
		}
	}

	if ( recnumfmt[0] == '\0' )
		sprintf( recnumfmt, "%%lu" );

	if ( allconstant ) {
		if ( RecNumfmt[0] )
			sprintf( RecNumbuf, RecNumfmt, ++rectotal );
		sprintf( recnumbuf, recnumfmt, ++recnum );

		if ( ! do_insert( insptr, fldlist ) ) {
			(void)pruerror( );
			prmsg( MSG_ERROR, "cannot insert record in table '%s' -- no insertions done",
				desttable );
			(void)end_insert( insptr, FALSE );
			exit( 2 );
		}
		i = 1;
	}
	else {
		i = 0;
		while( (record_cnt <= 0 || i < record_cnt) &&
			getrec( &srcrel, fldlist, attrinfo, attrcnt, readfunc ) )
		{
			if ( RecNumfmt[0] )
				sprintf( RecNumbuf, RecNumfmt, ++rectotal );
			sprintf( recnumbuf, recnumfmt, ++recnum );
			if ( ! do_insert( insptr, fldlist ) ) {
				(void)pruerror( );
				prmsg( MSG_ERROR, "cannot insert record in table '%s' -- no insertions done",
					desttable );
				(void)end_insert( insptr, FALSE );
				exit( 2 );
			}
			i++;
		}

		if ( is_uerror_set() ) {
			(void)pruerror( );
			prmsg( MSG_ALERT, "read failed abnormally on record %d%s",
				i + 1,
				i > 0 ? " -- other records still inserted" : "" );
		}
	}

	if ( exact_cnt && i != record_cnt ) {
		prmsg( MSG_ERROR, "exactly %d record%s must be given -- no insertions done",
			record_cnt, record_cnt != 1 ? "s" : "" );
		(void)end_insert( insptr, FALSE );
		exit( 4 );
	}

	(void)signal( SIGHUP, SIG_IGN );	/* ignore signals */
	(void)signal( SIGINT, SIG_IGN );
	(void)signal( SIGQUIT, SIG_IGN );
	(void)signal( SIGTERM, SIG_IGN );

	if ( ! end_insert( insptr, TRUE ) ) {
		(void)pruerror( );
		prmsg( MSG_ERROR, "cannot finish insert for table '%s' -- no insertions done",
			desttable );
		exit( 2 );
	}

	if ( ! quiet ) {
		if ( utplerrors ) {
			prmsg( MSG_WARN, "%d tuple error%s encountered reading relation(s)",
				utplerrors, utplerrors != 1 ? "s" : "" );
		}
		if ( rectotal ) {
			prmsg( MSG_ERRNOTE, "%d record%s inserted - %lu total", i,
				i != 1 ? "s" : "", rectotal );
		} else {
			prmsg( MSG_ERRNOTE, "%d record%s inserted", i,
				i != 1 ? "s" : "" );
		}
	}

	exit( 0 );
}

usage( )
{
	prmsg( MSG_USAGE, "[-cCpqVz] [-Q [ErrorLimit]] \\" );
	prmsg( MSG_CONTINUE, "[-d<delim>] [-m|r<record_cnt>] [-n<newline_char>] \\" );
	prmsg( MSG_CONTINUE, "[-f<rec#_precision>] [-F<REC#_precision>] \\" );
	prmsg( MSG_CONTINUE, "[[<srcattr> as ]<destattr>[=<value>]]... \\" );
	prmsg( MSG_CONTINUE, "into|onto <table1>[=<alt_table>] [from <table2>[=<alt_table>]]" );

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
catchsig( sig )
int sig;
{
	signal( sig, SIG_IGN );

	(void)end_insert( (struct uinsertinfo *)NULL, FALSE );

	if ( sig == SIGHUP || sig == SIGINT || sig == SIGTERM || sig == SIGQUIT ){
		prmsg( MSG_ALERT, "killed by signal (%d) -- no insertions done",
			sig );

		if ( sig != SIGQUIT )
			exit( 3 );
	}

	(void)signal( sig, SIG_DFL );	/* do the normal operation */
	(void)kill( getpid( ), sig );

	exit( 2 );		/* should never reach here */
}

getrec( relptr, attrvals, attrinfo, attrcnt, readfunc )
struct urelation *relptr;
char **attrvals;
struct insattrinfo *attrinfo;
int attrcnt;
int (*readfunc)();
{
	char *tmpattrs[MAXATT];
	register int i;

	if ( ! (*readfunc)( relptr, tmpattrs ) )
		return( FALSE );

	for( i = 0; i < attrcnt; i++, attrinfo++ ) {
		if ( (attrinfo->flags & (UI_CONSTANT|UI_RECNUM) ) == 0 )
			attrvals[i] = tmpattrs[attrinfo->sattrnum];
	}

	return( TRUE );
}

readfromrel( relptr, attrvals )
struct urelation *relptr;
register char **attrvals;
{
	struct utuple tpl;
	int rc;

	tpl.tplval = attrvals;

	rc = readtuple( relptr, &srcrelio, &tpl, NULL );

	/**********************************
	if ( tpl.flags & TPL_ERRORMSK )
	{
		prtplerror( relptr, &tpl );
	}
	**********************************/

	return( rc );
}

prompttuple( relptr, attrvals )
struct urelation *relptr;
register char **attrvals;
{
	register char *bptr;
	register int i;
	register struct uattribute *attrptr;
	register int totallen, linelen;

	static struct urelblock *blklist = NULL;
	static char attrbuf[ DBBLKSIZE ];


	prmsg( MSG_ERRNOTE, "new record:" );

	freerelblist( blklist );	/* free any previous record */
	blklist = NULL;

	for( i = 0, attrptr = relptr->attrs; i < relptr->attrcnt;
		i++, attrptr++ )
	{
		if ( ( ! zerowidth ) &&
		   ( ( attrptr->prwidth == 0 ) || ( isupper( attrptr->justify ) ) ) )
		{
			/* Normally, zero width fields are ignored */
			attrvals[i] = "";
			continue;
		}

		totallen = 0;
		bptr = attrbuf;
		prmsg( MSG_ERRASIS, "%s: ", attrptr->friendly ?
			attrptr->friendly : attrptr->aname );

		while( totallen < DBBLKSIZE - 1 )
		{
			if ( fgets( bptr, DBBLKSIZE - totallen, stdin ) == NULL )
			{
				prmsg( MSG_ERRASIS, "\n" );
				if ( i > 0 || totallen != 0 )
					prmsg( MSG_ALERT, "last record only partially entered -- record ignored, any other records still inserted" );
				freerelblist( blklist );
				blklist = NULL;
				return( FALSE );
			}

			linelen = strlen( bptr );
			bptr += linelen;
			totallen += linelen;

			if ( bptr[ -1 ] != '\n' )
			{
				/*
				 * We have a premature EOF or an attribute
				 * that is too long.  In either case, go
				 * to the top of the loop to print which
				 * ever condition is appropriate.
				 */
				continue;
			}

			/*
			 * Get rid of newline at end of string.
			 */
			--linelen;
			--totallen;
			--bptr;
			*bptr = '\0';

			/*
			 * Check for continuation character ('\').
			 */
			if ( linelen > 0 && bptr[ -1 ] == '\\' )
			{
				if ( newline &&
					attrptr->attrtype != UAT_FIXEDWIDTH )
				{
					bptr[ -1 ] = newline;
				}
				else
				{
					--linelen;
					--totallen;
					--bptr;
					*bptr = '\0';
				}
				prmsg( MSG_ERRASIS, "continue: " );
				continue;
			}

			if ( attrptr->attrtype == UAT_FIXEDWIDTH &&
				totallen != attrptr->terminate )
			{
				prmsg( MSG_ERROR, "value must be %d characters long, current value is %d characters",
					attrptr->terminate, totallen );
				totallen = 0;
				bptr = attrbuf;
				prmsg( MSG_ERRASIS, "%s: ", attrptr->friendly ?
					attrptr->friendly : attrptr->aname );
			}
			else
				break;

		} /* end while( totallen < DBBLKSIZE - 1 ) */

		if ( totallen >= DBBLKSIZE - 1 )
		{
			register int ch, lastch;

			linelen = totallen + 1;
			lastch = '\0';
			while( (ch = getchar( )) != EOF )
			{
				if ( ch == '\n' )
				{
					if ( lastch != '\\' )
						break;

					/* don't increment attr len */
				}
				else
					linelen++;

				lastch = ch;
			}

			prmsg( MSG_ALERT, "attribute '%s' is too long (%d chars, max is %d chars) -- extra chars truncated",
				attrptr->aname, linelen, DBBLKSIZE );
		}

		attrvals[i] = copystr( &blklist, attrbuf, totallen, 0, FALSE );
		if ( attrvals[i] == NULL )
		{
			pruerror();
			prmsg( MSG_ERROR, "cannot allocate space for attribute '%s'",
				attrptr->aname );
			freerelblist( blklist );
			blklist = NULL;
			return( FALSE );
		}

	} /* end for each attribute */

	return( TRUE );
}
