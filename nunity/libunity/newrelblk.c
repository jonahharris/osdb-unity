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
#include "urelation.h"
#include "uerror.h"
#include "udbio.h"

extern char *malloc();

static struct urelblock *freelist;
static unsigned short freecnt;

#ifndef MAXFREEIOBLK
#define MAXFREEIOBLK	4	/* maximum free I/O blocks */
#endif

struct urelblock *
newrelblk( )
{
	register struct urelblock *blkptr;

	if ( freelist )
	{
		freecnt--;
		blkptr = freelist;
		freelist = freelist->next;
	}
	else
	{
		blkptr = (struct urelblock *)malloc( sizeof( *blkptr ) );
		if ( blkptr == NULL )
		{
			set_uerror( UE_NOMEM );
			return( NULL );
		}
	}
	blkptr->attrcnt = 0;
	blkptr->next = NULL;
	blkptr->curloc = blkptr->data;
	blkptr->end = blkptr->data;

	return( blkptr );
}

freerelblk( blkptr )
register struct urelblock *blkptr;
{
	if ( blkptr != NULL )
	{
		if ( freecnt < MAXFREEIOBLK )
		{
			freecnt++;
			blkptr->next = freelist;
			freelist = blkptr;
		}
		else
			free( blkptr );
	}
}

freerelblist( blkptr )
register struct urelblock *blkptr;
{
	register struct urelblock *next;

	while( blkptr != NULL )
	{
		next = blkptr->next;
		freerelblk( blkptr );
		blkptr = next;
	}
}

_freeallrelblk( )
{
	register struct urelblock *blkptr;
	register struct urelblock *next;

	if ( inquery() )
		return( FALSE );

	for( blkptr = freelist; blkptr; blkptr = next )
	{
		next = blkptr->next;
		free( blkptr );
	}

	freelist = NULL;
	freecnt = 0;

	return( TRUE );
}
