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
#include "urelation.h"
#include "udbio.h"
#include "uerror.h"
#include "message.h"

extern FILE *packfpopen();
extern char *malloc();
extern char *basename();
extern char *cpdirname();
extern char *strchr();
extern char *getenv();
extern char *packedcatcmd;

static
FILE *
chkdescrwtbl( dpath, relptr )
char *dpath;
struct urelation *relptr;
{
	FILE *fp;
	static char description[] = "%description\n";
	char *curchar;
	int ch;

	if ( dpath[0] == '-' && dpath[1] == '\0' )
	{
		fp = stdin;
	}
	else if ( (fp = fopen( dpath, "r" )) == NULL )
	{
		if ( ( packedcatcmd == NULL ) ||
		     ( statpacked( dpath, NULL ) != 0 ) )
			return( NULL );

		/* Need to read from a packed relation file. */
		if ( ( fp = packfpopen( packedcatcmd, dpath ) ) != NULL )
			relptr->relflgs |= UR_PACKEDTBL;
		else
			return( NULL );
	}

	curchar = description;
	while( (ch = getc( fp )) != EOF )
	{
		if ( *curchar != ch )
			break;

		if ( *++curchar == '\0' )	/* everything matched */
		{
			relptr->relflgs |= UR_DESCRWTBL;
			return( fp );
		}
	}

	if ( fp != stdin ) {
		if ( relptr->relflgs & UR_PACKEDTBL ) {
			(void)pkclose( fp );
			relptr->relflgs &= ~UR_PACKEDTBL;
		}
		else
			(void)fclose( fp );
	}
	else
		(void)ungetc( ch, fp );	/* put non-matching char back */

	return( NULL );
}

FILE *
findufile( path, table, prefix, search )
char *path;
char *table;
char prefix;
int search;
{
	register char *filename;
	register char *ptr, *next;
	static   char *defaultsearch = "ucd";
	FILE *fp;
	int currentdir, datadir, udfiles;
	char where2search[8];
	char *searchnext;

	filename = basename( table );

	currentdir = datadir = udfiles = 0;

	next = where2search;

	/* Was a particular directory given to search first? */
	switch ( search ) {
	case 'C':
	case 'c':
		/* current directory */
		*next++ = 'c';
		++currentdir;
		break;
	case 'D':
	case 'd':
		/* datafile directory */
		*next++ = 'd';
		++datadir;
		break;
	case 'U':
	case 'u':
		/* UNITYDFILES directory(s) */
		*next++ = 'u';
		++udfiles;
		break;
	default:
		break;
	}

	/* If UNITYDSEARCH is not set then use default search order. */
	ptr = getenv("UNITYDSEARCH");
	if ( ptr == NULL ) ptr = defaultsearch;

	while ( *ptr ) {
		switch ( *ptr++ ) {
		case 'C':
		case 'c':
			/* only search current directory once */
			if ( currentdir ) continue;
			*next++ = 'c';
			++currentdir;
			break;
		case 'D':
		case 'd':
			/* only search datafile directory once */
			if ( datadir ) continue;
			*next++ = 'd';
			++datadir;
			break;
		case 'U':
		case 'u':
			/* only search UNITYDFILES once */
			if ( udfiles ) continue;
			*next++ = 'u';
			++udfiles;
			break;
		default:
			/* ignore invalid search indicator */
			break;
		}
	}
	next = '\0';	/* terminate where2search[] string */

	/*
	 * In case UNITYDSEARCH was an empty string (or garbage)
	 * make sure there is at least one place to search.
	 */
	if ( where2search[0] ) {
		searchnext = where2search;
	} else {
		searchnext = defaultsearch;
	}

	while ( *searchnext )
	{
		switch ( *searchnext++ ) {

		case 'c':
			/*
			 * check the current directory
			 */
			sprintf( path, "%c%s", prefix, filename );
			if ( (fp = fopen( path, "r" )) != NULL )
				return( fp );
			break;

		case 'd':
			/*
			 * check the datafile directory
			 */
			ptr = cpdirname( path, table );
			sprintf( ptr, "/%c%s", prefix, filename );
			if ( ( fp = fopen( path, "r" ) ) != NULL ) {
				return( fp );
			}
			break;

		case 'u':
			/*
			 * check UNITYDFILES directory(s)
			 */
			ptr = getenv( "UNITYDFILES" );
			while( ptr && *ptr )
			{
				next = strchr( ptr, ':' );
				if ( next )
					*next = '\0';
				if ( *ptr )
					sprintf( path, "%s/%c%s", ptr, prefix, filename );
				else
					sprintf( path, "%c%s", prefix, filename );
				if ( next )
					*next++ = ':';	/* put the colon back & go on */

				if ( (fp = fopen( path, "r" )) != NULL )
					return( fp );
				ptr = next;
			}
			break;

		default:
			break;
		}
	}

	return( NULL );
}

static
FILE *
finddescr( table, path, relptr, search )
char *table;
char *path;
struct urelation *relptr;
int search;
{
	FILE *fp;

	fp = findufile( path, table, 'D', search );
	if ( fp != NULL )
		return( fp );

	strcpy( path, table );
	return( chkdescrwtbl( path, relptr ) );
}

#define DFLT_PRWIDTH	12	/* default print width if none given */
#define MAXLINE		1024

static int
frddescr( fp, relptr )
FILE *fp;
struct urelation *relptr;
{
	struct uattribute attrs[MAXATT];
	char buf[MAXLINE+1];
	register int attrcnt;
	register struct uattribute *attrptr;
	register char *bptr, *tmpptr;
	register int cnt;
	char tmpch, default_term, seen_space;
	char justify, nodisplay;

	nodisplay = TRUE;
	attrcnt = 0;
	while( fgets( buf, MAXLINE, fp ) != NULL )
	{
		if ( buf[0] == '#' || buf[0] == '\n' )
			continue;

		if ( (relptr->relflgs & UR_DESCRWTBL) &&
			strcmp( buf, "%enddescription\n" ) == 0 )
		{
			break;	/* description finished */
		}

		bptr = buf;
		if ( attrcnt >= MAXATT )
		{
			freeattrlist( attrs, attrcnt );
			set_uerror( UE_NUMATTR );
			return( FALSE );
		}

		attrptr = &attrs[attrcnt++];
		attrptr->friendly = NULL;
		attrptr->prwidth = DFLT_PRWIDTH;
		attrptr->justify = 'l';
		attrptr->flags = 0;

		cnt = strlen( bptr );
		if ( bptr[cnt-1] == '\n' )
			bptr[cnt-1] = '\0';
		else
			bptr[MAXLINE] = '\0';

		/*
		 * Get the attribute name.
		 */
		if ( ! isalpha( *bptr ) && *bptr != '_' ) {
			/*
			 * Bad attribute name
			 */
			freeattrlist( attrs, attrcnt );
			set_uerror( UE_ATTRNAME );
			return( FALSE );
		}
		cnt = 0;
		tmpptr = bptr;
		while( (isalnum( *bptr ) || *bptr == '_') && cnt < ANAMELEN )
			attrptr->aname[cnt++] = *bptr++;
		attrptr->aname[cnt] = '\0';

		if ( *bptr != ' ' && *bptr != '\t' && *bptr != '\0' ) 
		{
			/*
			 * Attribute name is too long.
			 * Warn user it will be truncated.
			 */

			while ( *bptr != ' ' && *bptr != '\t' && *bptr != '\0' ) 
				bptr++;
			tmpch = *bptr;
			*bptr = '\0';

			prmsg( MSG_WARN, "Attribute name '%s' truncated to %d characters",
				tmpptr, ANAMELEN );
			*bptr = tmpch;
		}

		while( *bptr == ' ' )
			bptr++;

		if ( *bptr == '\0' )
		{
			/*
			 * Everything is defaulted.
			 */
			attrptr->attrtype = UAT_TERMCHAR;
			attrptr->terminate = ':';
			default_term = TRUE;
			continue;
		}
		else if ( *bptr != '\t' )
		{
			freeattrlist( attrs, attrcnt );
			set_uerror( UE_NOTAB );
			return( FALSE );
		}
		else	/* Remove white space */
		{
			++bptr;
			while( *bptr == ' ' )
				bptr++;
		}

		/*
		 * Get the field type and width or termination char.
		 */
		default_term = FALSE;
		switch( *bptr++ ) {
		case 't':
			attrptr->attrtype = UAT_TERMCHAR;
			if ( *bptr == '\\' )
			{
				switch( *++bptr ) {
				case 't':
					attrptr->terminate = '\t';
					break;
				case 'n':
					attrptr->terminate = '\n';
					break;
				case '\0':
					freeattrlist( attrs, attrcnt );
					set_uerror( UE_BSEOL );
					return( FALSE );
				case '\\':
					attrptr->terminate = '\\';
					break;
				case 'a':
					attrptr->terminate = '\a';
					break;
				case 'b':
					attrptr->terminate = '\b';
					break;
				case 'f':
					attrptr->terminate = '\f';
					break;
				case 'r':
					attrptr->terminate = '\r';
					break;
				case 'v':
					attrptr->terminate = '\v';
					break;
				case '\'':
					attrptr->terminate = '\'';
					break;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					{
						char c = *bptr;
						int i = c - '0';
						while ( (isdigit(c = bptr[1])) &&
							(c != '8' && c != '9') &&
							(i <= 0x1f) ) {
							i = 8 * i + c - '0';
							++bptr;
						}
						attrptr->terminate = (char)i;
					}
					break;
				default:
					attrptr->terminate = *bptr;
					break;
				}
			}
			else if ( *bptr )
				attrptr->terminate = *bptr;
			else
			{
				freeattrlist( attrs, attrcnt );
				set_uerror( UE_TERMCH );
				return( FALSE );
			}
			++bptr;
			break;
		case 'w':
			attrptr->attrtype = UAT_FIXEDWIDTH;
			tmpptr = bptr;
			while( isdigit( *bptr ) )
				bptr++;
			tmpch = *bptr;
			*bptr = '\0';
 			attrptr->terminate = atoi( tmpptr );
			if ( ( attrptr->terminate == 0 && *tmpptr != '0' ) ||
			     ( attrptr->terminate >= DBBLKSIZE ) )
			{
				freeattrlist( attrs, attrcnt );
				set_uerror( UE_ATTRWIDTH );
				return( FALSE );
			}
			attrptr->prwidth = attrptr->terminate;
			*bptr = tmpch;
			break;
		default:
			freeattrlist( attrs, attrcnt );
			set_uerror( UE_ATTRTYPE );
			return( FALSE );
		}

		seen_space = FALSE;
		while( *bptr == ' ' )
		{
			seen_space = TRUE;
			bptr++;
		}

		if ( *bptr == '\0' )
			continue;
		else if ( *bptr != '\t' && ! seen_space )
		{
			freeattrlist( attrs, attrcnt );
			set_uerror( UE_NOTAB );
			return( FALSE );
		}
		else	/* Remove white space */
		{
			if ( *bptr == '\t' )
				++bptr;
			while( *bptr == ' ' )
				bptr++;
		}

		/*
		 * Now get the print information.
		 * We will allow the print justification
		 * to preceed or follow the print width.
		 */
		tmpptr = bptr;
		attrptr->prwidth = 0;
		switch( *bptr ) {
		case 'c':
		case 'C':
		case 'l':
		case 'L':
		case 'r':
		case 'R':
			justify = *bptr++;
			break;
		default:
			justify = '\0';
			break;
		}
		while( isdigit( *bptr ) )
			bptr++;
		if ( tmpptr != bptr )
		{
			tmpch = *bptr;
			*bptr = '\0';
			attrptr->prwidth = atoi( tmpptr );
			*bptr = tmpch;
		}
		else
			attrptr->prwidth = DFLT_PRWIDTH;

		switch( *bptr ) {
		case 'c':
		case 'C':
		case 'l':
		case 'L':
		case 'r':
		case 'R':
			if ( justify != '\0' ) {
				freeattrlist( attrs, attrcnt );
				set_uerror( UE_JUST );
				return( FALSE );
			}
			justify = *bptr++;
			break;
		case '\t':
		case ' ':
		case '\0':
			if ( justify == '\0' )
				justify = 'l';
			break;
		default:
			freeattrlist( attrs, attrcnt );
			set_uerror( UE_JUST );
			return( FALSE );
		}

		attrptr->justify = justify;

		/*
		 * Upper case print justification indicates that, in general,
		 * the attribute is not to be printed (displayed) regardless
		 * of whether or not zero (0) has been given as the print width.
		 */
		if ( ( attrptr->prwidth != 0 ) && ( ! isupper( justify ) ) )
			nodisplay = FALSE;

		seen_space = FALSE;
		while( *bptr == ' ' )
		{
			seen_space = TRUE;
			bptr++;
		}

		if ( *bptr == '\0' )
			continue;
		else if ( *bptr != '\t' && ! seen_space )
		{
			freeattrlist( attrs, attrcnt );
			set_uerror( UE_NOTAB );
			return( FALSE );
		}
		else	/* Remove white space */
		{
			if ( *bptr == '\t' )
				++bptr;
			else	/* remove blanks only if no tab*/
				while( *bptr == ' ' )
					bptr++;
		}

		if ( *bptr )
		{
			attrptr->friendly = malloc( (unsigned)(strlen( bptr ) + 1 ) );
			if ( attrptr->friendly == NULL )
			{
				freeattrlist( attrs, attrcnt );
				set_uerror( UE_NOMEM );
				return( FALSE );
			}
			strcpy( attrptr->friendly, bptr );
		}
	}

	/*
	 * Check that last field is terminated by a new-line.
	 */
	if ( default_term )	/* last attr had default terminate char */
		attrptr->terminate = '\n';
	else if ( attrptr->attrtype == UAT_TERMCHAR &&
		attrptr->terminate != '\n' )
	{
		freeattrlist( attrs, attrcnt );
		set_uerror( UE_NLTERM );
		return( FALSE );
	}

	attrptr = (struct uattribute *)malloc( (unsigned)( attrcnt * sizeof( *attrptr ) ) );
	if ( attrptr == NULL ) {
		freeattrlist( attrs, attrcnt );
		set_uerror( UE_NOMEM );
		return( FALSE );
	}

	relptr->attrcnt = attrcnt;
	while( --attrcnt >= 0 )
		attrptr[attrcnt] = attrs[attrcnt];
	relptr->attrs = attrptr;

	if ( nodisplay == TRUE )
		relptr->relflgs != UR_NODISPLAY;

	return( TRUE );
}

static int
readdescr( relptr, descpath, savedpath, search )
struct urelation *relptr;
char *descpath;
int savedpath;
int search;
{
	FILE *fp;
	int rc;
	char buf[MAXPATH + 1 + 4];	/* allow for "/./" or "././" prefix */

	if ( (fp = finddescr( descpath, buf, relptr, search )) == NULL )
	{
		set_uerror( UE_NODESCR );
		return( FALSE );
	}

	if ( savedpath )
	{
		relptr->dpath = malloc( (unsigned)(strlen( buf ) + 1) );
		if ( relptr->dpath == NULL )
		{
			set_uerror( UE_NOMEM );
			return( FALSE );
		}
		relptr->relflgs |= UR_DPATHALLOC;
		strcpy( relptr->dpath, buf );
	}

	rc = frddescr( fp, relptr );

	if ( fp != stdin ) {
		if ( relptr->relflgs & UR_PACKEDTBL ) {
			(void)pkclose( fp );
			relptr->relflgs &= ~UR_PACKEDTBL;
		}
		else
			(void)fclose( fp );
	}

	return( rc );
}

struct urelation *
getrelinfo( pathname, relptr, savedpath )
char *pathname;
struct urelation *relptr;
int savedpath;
{
	char *descpath;
	char tblpath[MAXPATH * 2 + 1 + 8];	/* allow for "/./" or "././" prefix(s) */
	int search;

	search = 0;
	strcpy( tblpath, pathname );
	descpath = strchr( tblpath, '=' );
	if ( descpath != NULL )
	{
		*descpath++ = '\0';
		/*
		 * If the alternate description includes a
		 * "/./" or "././" prefix or is the same as
		 * the table name (path) then we will search
		 * the data directory first.
		 */
		if (( strncmp( descpath, "/./", 3 ) == 0 ) ||
		    ( strncmp( descpath, "././", 4 ) == 0 ) ||
		    ( strcmp( tblpath, descpath ) == 0 )) {
			search = 'd';
		}
	}
	else
	{
		descpath = tblpath;
	}

	if ( relptr == NULL )
	{
		relptr = (struct urelation *)malloc( sizeof( *relptr ) );
		if ( relptr == NULL ) {
			set_uerror( UE_NOMEM );
			return( NULL );
		}
		relptr->relflgs = UR_RELALLOC;
	}
	else
		relptr->relflgs = 0;

	relptr->dpath = NULL;
	relptr->path = NULL;
	relptr->attrs = NULL;
	relptr->attrcnt = 0;
	relptr->flags = 0;

	relptr->path = malloc( strlen( tblpath ) + 1 );
	if ( relptr->path == NULL )
	{
		set_uerror( UE_NOMEM );
		freerelinfo( relptr );
		return( FALSE );
	}
	relptr->relflgs |= UR_PATHALLOC;
	strcpy( relptr->path, tblpath );

	if ( ! readdescr( relptr, descpath, savedpath, search ) )
	{
		freerelinfo( relptr );
		return( NULL );
	}

	if ( descpath != tblpath )
	{
		FILE *fp;

		/*
		 * Alternate description given.  Check if there
		 * is a description in the real table.
		 */
		relptr->relflgs &= ~UR_DESCRWTBL;
		fp = chkdescrwtbl( relptr->path, relptr );
		if ( fp != NULL && fp != stdin ) {
			if ( relptr->relflgs & UR_PACKEDTBL ) {
				(void)pkclose( fp );
				relptr->relflgs &= ~UR_PACKEDTBL;
			}
			else
				(void)fclose( fp );
		}
	}

	return( relptr );
}
