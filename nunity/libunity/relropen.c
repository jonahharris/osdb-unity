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
#include <errno.h>
#include "urelation.h"
#include "udbio.h"
#include "uerror.h"

#ifndef	NULL
#define	NULL	((char *)0)
#endif

extern char *packedcatcmd;

extern struct urelblock *readnextblk();

relropen( ioptr, relptr, descfp )
struct urelio *ioptr;
struct urelation *relptr;
FILE *descfp;		/* file to copy descriptor to if present */
{
	int rc = TRUE;
	register struct urelblock *blkptr;
	register char *curchar;

	static char enddescr[] = "%enddescription\n";
	static char strtdescr[] = "%description\n";

	ioptr->flags = 0;
	ioptr->readbuf = ioptr->savebuf = ioptr->curblk = NULL;
	ioptr->blkcnt = 0;
	ioptr->tuplecnt = 0;

	if ( strcmp( relptr->path, "-" ) == 0 )
	{
		ioptr->fd = 0;
		ioptr->flags &= ~UIO_FILEMASK;
		ioptr->flags |= UIO_STDIN;
	}
	else
	{
		if ( ( packedcatcmd == NULL ) ||
		   ( ( ioptr->flags & UIO_FILEMASK ) != UIO_PACKED ) )
		{
			ioptr->flags &= ~UIO_FILEMASK;

			if ( (ioptr->fd = open( relptr->path, 0 )) < 0 )
			{
				if ( ( errno == ENOENT ) && ( packedcatcmd ) )
				{
					if ( statpacked( relptr->path, NULL ) == 0 )
					{
						ioptr->flags |= UIO_PACKED;
					} else {
						errno = ENOENT;		/* restore errno */
						set_uerror( UE_ROPEN );
						return( FALSE );
					}
				} else {
					set_uerror( UE_ROPEN );
					return( FALSE );
				}
			} else {
				ioptr->flags |= UIO_NORMAL;
			}
		}
		if ( ( ioptr->flags & UIO_FILEMASK ) == UIO_PACKED )
		{
			if ( (ioptr->fd = packedopen( packedcatcmd, relptr->path )) < 0 )
			{
				set_uerror( UE_RPOPEN );
				return( FALSE );
			}
		}
	}

	blkptr = readnextblk( ioptr, (struct urelblock *)NULL );
	if ( blkptr != NULL &&
		strncmp( blkptr->data, strtdescr,
			sizeof( strtdescr ) - 1 ) == 0 )
	{
		/*
		 * Skip everything up until the end-description string.
		 */
		if ( descfp != NULL )
			fputs( strtdescr, descfp );

		blkptr->curloc = blkptr->data + sizeof( strtdescr ) - 1;
		curchar = enddescr;
		while( 1 )
		{
			if ( blkptr->curloc >= blkptr->end )
			{
				/* free all I/O blocks */
				freerelblist( ioptr->readbuf );
				ioptr->curblk = NULL;
				ioptr->readbuf = NULL;

				blkptr = readnextblk( ioptr,
						(struct urelblock *)NULL );
				if ( blkptr == NULL )
					return( TRUE );
			}
			if ( descfp != NULL )
			{
				putc( *blkptr->curloc, descfp );
			}

			if ( *blkptr->curloc == *curchar )
			{
				++blkptr->curloc;
				if ( *++curchar == '\0' )
					return( TRUE );	/* matched all chars */

				continue;	/* check next char */
			}

			/*
			 * No match.  Look for next new-line char.
			 */
			curchar = enddescr;
			while( *blkptr->curloc++ != '\n' )
			{
				if ( blkptr->curloc >= blkptr->end )
				{
					/* free all I/O blocks */
					freerelblist( ioptr->readbuf );
					ioptr->curblk = NULL;
					ioptr->readbuf = NULL;

					blkptr = readnextblk( ioptr,
							(struct urelblock *)NULL );
					if ( blkptr == NULL )
						return( TRUE );
				}
				if ( descfp != NULL )
				{
					putc( *blkptr->curloc, descfp );
				}
			}
		}
	}
	else if ( descfp != NULL &&
		(relptr->relflgs & UR_DESCRWTBL) != 0 )
	{
		/*
		 * Write out the description with the data
		 */
		if ( ! wrtbldescr( descfp, relptr ) )
			rc = FALSE;
	}

	return( rc );
}

relrclose( ioptr )
register struct urelio *ioptr;
{
	int	rc = TRUE;

	if ( ioptr->fd >= 0 ) {
		if ( (ioptr->flags & UIO_STDIN) == 0 ) {
			if ( ( ioptr->flags & UIO_FILEMASK ) != UIO_PACKED )
				close( ioptr->fd );
			else
			{
				if ( ( packedclose( ioptr->fd ) ) &&
				     ( ioptr->flags & UIO_EOF ) )
				{
					ioptr->flags |= UIO_ERROR;
					set_uerror( UE_PACKEDEOF );
					rc = FALSE;
				}
			}
		}
		ioptr->fd = -1;
	} else if ((( ioptr->flags & UIO_FILEMASK ) == UIO_PACKED ) &&
		   (( ioptr->flags & UIO_ERROR ) == UIO_ERROR )) {
		/*
		 * Once an error has been detected when closing
		 * a packed relation we will continue to return
		 * a failure code in order to guard against any
		 * application programs that fail to check for
		 * an error after a while readtuple type of loop.
		 */
		rc = FALSE;
	}

	freerelblist( ioptr->readbuf );
	freerelblist( ioptr->savebuf );
	ioptr->curblk = ioptr->savebuf = ioptr->readbuf = NULL;
	ioptr->blkcnt = 0;
	ioptr->tuplecnt = 0;

	return( rc );
}
