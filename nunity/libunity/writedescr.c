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
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include "urelation.h"
#include "uerror.h"

extern char *basename();
extern char *cpdirname();


legalattrs( relptr )
register struct urelation *relptr;
{
	register unsigned int i;
	register struct uattribute *attrptr;

	for( i = 0, attrptr = relptr->attrs; i < relptr->attrcnt;
			i++, attrptr++ ) {
		if ( ! chkaname( attrptr->aname ) ) {
			set_uerror( UE_ATTRNAME );
			return( FALSE );
		}
		else if ( findattr( relptr->attrs, i, attrptr->aname ) >= 0 ) {
			set_uerror( UE_DUPATTR );
			return( FALSE );
		}
	}

	return( TRUE );
}

static int
fwritedescr( fp, relptr )
FILE *fp;
struct urelation *relptr;
{
	register int i;
	register struct uattribute *attrptr;

	for( attrptr = relptr->attrs, i = 0; i < relptr->attrcnt;
		i++, attrptr++ )
	{
		fputs( attrptr->aname, fp );
		putc( '\t', fp );

		if ( attrptr->attrtype == UAT_TERMCHAR )
		{
			switch( (char)attrptr->terminate ) {
			case '\n':
				fputs( "t\\n", fp );
				break;
			case '\t':
				fputs( "t\\t", fp );
				break;
			case '\b':
				fputs( "t\\b", fp );
				break;
			case '\r':
				fputs( "t\\r", fp );
				break;
			case '\f':
				fputs( "t\\f", fp );
				break;
			case '\\':
				fputs( "t\\\\", fp );
				break;
			case '\'':
				fputs( "t\\\'", fp );
				break;
			case '\0':
				fputs( "t\\0", fp );
				break;
			default:
				putc( 't', fp );
				if ( ! isprint( (char)attrptr->terminate ) )
					fprintf( fp, "\\%o", (unsigned char)attrptr->terminate );
				else
					putc( (char)attrptr->terminate, fp );
			}
		}
		else
			fprintf( fp, "w%d", attrptr->terminate );

		fprintf( fp, "\t%d%c", attrptr->prwidth, attrptr->justify );

		if ( attrptr->friendly != NULL )
			fprintf( fp, "\t%s\n", attrptr->friendly );
		else
			fprintf( fp, "\t\n" );
	}

	return( fflush( fp ) == EOF ? FALSE : TRUE );
}

writedescr( relptr, dir )
register struct urelation *relptr;
char *dir;
{
	FILE *fp;
	register char *str;
	char resetmode, rc;
	RETSIGTYPE (*sigint)(), (*sigquit)(), (*sighup)(), (*sigterm)();
	struct stat statbuf;
	char realdescr[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char tmpdescr[MAXPATH+4];	/* allow for "/./" or "././" prefix */

	if ( ! legalattrs( relptr ) )
		return( FALSE );

	/*
	 * Set up name of descriptor file and name of temporary
	 * descriptor file.
	 */
	if ( dir ) {
		/*
		 * Put in given directory.
		 */
		strcpy( tmpdescr, dir );
		str = &tmpdescr[ strlen( tmpdescr ) ];
		if ( str != tmpdescr )
			*str++ = '/';
		*str = '\0';
		sprintf( realdescr, "%sD%s", tmpdescr, basename( relptr->path ) );
	}
	else if ( relptr->dpath ) {
		/*
		 * Use given path name for descriptor file.
		 */
		str = cpdirname( tmpdescr, relptr->dpath );
		if ( str != tmpdescr )
			*str++ = '/';
		*str = '\0';
		sprintf( realdescr, "%sD%s", tmpdescr, basename( relptr->dpath ) );
	}
	else {
		/*
		 * Put in same directory as relation.
		 */
		str = cpdirname( tmpdescr, relptr->path );
		if ( str != tmpdescr )
			*str++ = '/';
		*str = '\0';
		sprintf( realdescr, "%sD%s", tmpdescr, basename( relptr->path ) );
	}

	/*
	 * Got some touchy code coming up, ignore all the signals.
	 * No interruptions allowed, while we're writing, linking, and
	 * unlinking the old and new descriptor files.
	 */
	sigint = signal( SIGINT, SIG_IGN );
	sighup = signal( SIGHUP, SIG_IGN );
	sigquit = signal( SIGQUIT, SIG_IGN );
	sigterm = signal( SIGTERM, SIG_IGN );

	strcpy( str, "DtmpXXXXXX" );
	mktemp( tmpdescr );

	if ( (fp = fopen( tmpdescr, "w" )) == NULL ) {
		set_uerror( UE_WOPEN );

		(void)signal( SIGINT, sigint );	/* reset signals */
		(void)signal( SIGHUP, sighup );
		(void)signal( SIGQUIT, sigquit );
		(void)signal( SIGTERM, sigterm );

		return( FALSE );
	}

	if ( ! fwritedescr( fp, relptr ) )
	{
		(void)fclose( fp );

		set_uerror( UE_WRITE );
		unlink( tmpdescr );

		(void)signal( SIGINT, sigint );	/* reset signals */
		(void)signal( SIGHUP, sighup );
		(void)signal( SIGQUIT, sigquit );
		(void)signal( SIGTERM, sigterm );

		return( FALSE );
	}

	(void)fclose( fp );

	/*
	 * Get the stats on the original file.
	 */
	if ( stat( realdescr, &statbuf ) < 0 ) {
		/*
		 * Original file not there; don't change owner.
		 */
		resetmode = FALSE;
	}
	else
		resetmode = TRUE;

	/*
	 * Remove the old file and link the new file to the
	 * old file's name.
	 *
	 * NOTE:  It would be better here to link the old
	 * file to a tmpfile first, then unlink the old file,
	 * then link the new file to the old file, and finally
	 * remove the tmpfile.  This would avoid the (unlikely)
	 * case where the new file has been deleted out from
	 * under us by some one else.
	 */
	if ( (unlink( realdescr ) >= 0 || errno == ENOENT) &&
			link( tmpdescr, realdescr ) >= 0 ) {
		unlink( tmpdescr );
		if ( resetmode ) {
			/*
			 * Reset all the pertinent stats on the new
			 * file to that of the old file.
			 */
			chmod( realdescr, (int)statbuf.st_mode );
			chown( realdescr, (int)statbuf.st_uid,
				(int)statbuf.st_gid );
		}
		rc = TRUE;
	}
	else {
		set_uerror( UE_LINK );
		rc = FALSE;
	}

	(void)signal( SIGINT, sigint );	/* reset signals */
	(void)signal( SIGHUP, sighup );
	(void)signal( SIGQUIT, sigquit );
	(void)signal( SIGTERM, sigterm );

	return( rc );
}

wrtbldescr( fp, relptr )
FILE *fp;
struct urelation *relptr;
{
	int rc;

	if ( ! legalattrs( relptr ) )
		return( FALSE );

	fputs( "%description\n", fp );
	rc = fwritedescr( fp, relptr );
	fputs( "%enddescription\n", fp );

	return( fflush( fp ) == EOF ? FALSE : rc );
}
