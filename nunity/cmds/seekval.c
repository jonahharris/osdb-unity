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
#include "uquery.h"
#include "message.h"
#include "seekval.h"

/*
 * Structure for storing seek locations of tuples.
 */
#define SEEKBLKSIZE	1022		/* to get 4K page allocations */

struct seekblk {
	struct seekblk *next;
	short blkcnt;
	long seekvals[SEEKBLKSIZE];
};

extern char *calloc();
extern int (*settplfunc())();

static struct seekblk *
shiftentries( blkptr, destloc, seekval )
register struct seekblk *blkptr;
int destloc;
long seekval;
{
	register int i;

	if ( blkptr == NULL )
	{
		blkptr = (struct seekblk *)calloc( (unsigned)1, sizeof( struct seekblk ) );
		if ( blkptr == NULL )
		{
			prmsg( MSG_INTERNAL, "cannot allocate memory to save seek values" );
			return( NULL );
		}
		blkptr->seekvals[0] = seekval;
		blkptr->blkcnt = 1;
	}
	else if ( destloc >= blkptr->blkcnt )
	{
		if ( blkptr->blkcnt >= SEEKBLKSIZE )
		{
			blkptr->next = shiftentries( blkptr->next, 0, seekval);
			if ( blkptr->next == NULL )
				return( NULL );
		}
		else
			blkptr->seekvals[blkptr->blkcnt++] = seekval;
	}
	else if ( blkptr->blkcnt >= SEEKBLKSIZE )
	{
		blkptr->next = shiftentries( blkptr->next, 0,
					blkptr->seekvals[SEEKBLKSIZE - 1] );
		if ( blkptr->next == NULL )
			return( NULL );
		for( i = SEEKBLKSIZE - 2; i >= destloc; i-- )
			blkptr->seekvals[i + 1] = blkptr->seekvals[i];
		blkptr->seekvals[destloc] = seekval;
	}
	else
	{
		for( i = blkptr->blkcnt - 1; i >= destloc; i-- )
			blkptr->seekvals[i + 1] = blkptr->seekvals[i];
		blkptr->seekvals[destloc] = seekval;
		++blkptr->blkcnt;
	}

	return( blkptr );		
}

/*
 * Variables for storing seek values.
 */
static struct seekblk *seekroot = NULL;
static long seekcnt = 0;
static int (*oldtplfunc)();
static char no_duplicates;
static char error_flag;

static int
saveseekval( attrvals, attrcnt, projptr )
char **attrvals;
int attrcnt;
struct qprojtuple *projptr;
{
	register struct seekblk *blkptr;
	register long seekval;
	register int high, mid, low;

	seekval = projptr->tplptr->lseek;

	if ( seekroot == NULL )
	{
		seekroot = shiftentries( (struct seekblk *)NULL, 0, seekval );
		if ( seekroot == NULL )
			return( FALSE );
	}
	else
	{
		blkptr = seekroot;
		while( 1 )
		{
			if ( blkptr->seekvals[blkptr->blkcnt - 1] >= seekval )
			{
				low = 0;
				high = blkptr->blkcnt - 1;
				while( low <= high )
				{
					mid = ( high + low ) / 2;
					if ( seekval == blkptr->seekvals[mid] )
					{
						/*
						 * Duplicate seek value
						 * Check if this is legal.
						 */
						if ( no_duplicates )
						{
							prmsg( MSG_ERROR, "query retrieves tuple at seek value %ld multiple times",
								seekval );
							error_flag = TRUE;
						}
						return( TRUE );
					}

					if ( seekval < blkptr->seekvals[mid] )
						high = mid - 1;
					else
						low = mid + 1;
				}
				break;
			}
			else if ( blkptr->next == NULL )
			{
				low = blkptr->blkcnt;
				break;
			}
			blkptr = blkptr->next;
		}

		if ( shiftentries( blkptr, low, seekval ) == NULL )
			return( FALSE );
	}

	seekcnt++;

	/*
	 * Call the original tuple function if there is one.
	 */
	if ( oldtplfunc != NULL &&
		! (*oldtplfunc)( attrvals, attrcnt, projptr ) )
	{
		return( FALSE );
	}

	return( TRUE );
}

static void
seekfree( )
{
	register struct seekblk *blkptr;
	register struct seekblk *next;

	blkptr = seekroot;
	seekroot = NULL;

	while( blkptr != NULL )
	{
		next = blkptr->next;
		free( blkptr );
		blkptr = next;
	}
}

int
seeksave( query, no_dups )
struct uquery *query;
char no_dups;
{
	struct qresult result;
	int rc;

	oldtplfunc = settplfunc( query, saveseekval );
	seekreset( );

	no_duplicates = no_dups;
	error_flag = FALSE;

	rc = queryeval( query, &result );
	if ( error_flag )
		rc = FALSE;

	(void)settplfunc( query, oldtplfunc );
	if ( ! rc )
	{
		seekfree( );
		seekcnt = -1;	/* some error occured, so no seek vals */
	}

	return( rc );
}

void
seekreset( )
{
	seekfree( );
	seekcnt = 0;

	seekrewind( );
}

long
getseekcnt( )
{
	return( seekcnt );
}

/*
 * Variables for traversing seek values
 */
static struct seekblk *blkptr = NULL;
static int blkcnt = -1;

void
seekrewind( )
{
	blkptr = seekroot;
	/* -1 in blkcnt means all seek values are found */
	blkcnt = (blkptr == NULL && seekcnt >= 0) ? -1 : 0;
}

int
seekfind( tplptr )
struct utuple *tplptr;
{
	if ( blkcnt < 0 )	/* all seek values are found */
		return( TRUE );

	while( blkptr != NULL && blkcnt >= blkptr->blkcnt )
	{
		blkptr = blkptr->next;
		blkcnt = 0;
	}

	if ( blkptr == NULL )
		return( FALSE );

	if ( tplptr->lseek == blkptr->seekvals[blkcnt] )
	{
		while( 1 )
		{
			if ( ++blkcnt >= blkptr->blkcnt )
			{
				blkptr = blkptr->next;
				blkcnt = 0;
			}
			if ( blkptr != NULL &&
				blkptr->seekvals[blkcnt] == tplptr->lseek )
			{
				/*
				 * Duplicate seek values in blocks - should
				 * not happen.  Also adjust the number of
				 * tuples that were saved.
				 */
				prmsg( MSG_INTERNAL, "duplicate seek value (%ld) encountered in seek-blocks; data table NOT corrupted, however",
					tplptr->lseek );
				--seekcnt;
			}
			else
				break;
		}

		return( TRUE );
	}
	else
	{
		while( blkptr != NULL &&
			tplptr->lseek > blkptr->seekvals[blkcnt] )
		{
			prmsg( MSG_INTERNAL, "could not find tuple at seek value %ld",
				blkptr->seekvals[blkcnt] );
			if ( ++blkcnt >= blkptr->blkcnt )
			{
				blkptr = blkptr->next;
				blkcnt = 0;
			}
		}

		return( FALSE );
	}
}

long
save_unchgd( perptr, noop, quiet, updfunc )
struct uperuseinfo *perptr;
char noop;
char quiet;
int (*updfunc)();
{
	register long updcnt;
	struct utuple tpl;
	char *attrvals[MAXATT];

	updcnt = 0;
	seekrewind( );

	tpl.tplval = attrvals;
	tpl.flags = 0;
	while( peruse_tuple( perptr, &tpl ) )
	{
		if ( ! quiet && (tpl.flags & TPL_ERRORMSK) )
			prtplerror( perptr->relptr, &tpl ); /* syntax error */

		if ( seekfind( &tpl ) )
		{
			/* tuple already been updated */
			updcnt++;

			if ( updfunc != NULL &&
				! (*updfunc)( perptr, noop, quiet,
						&tpl, updcnt ) )
			{
				return( -1 );
			}
		}
		else if ( ! noop && ! savetuple( perptr, attrvals ) )
		{
			(void)pruerror( );
			prmsg( MSG_INTERNAL, "cannot save tuple #%d for table '%s'",
				tpl.tuplenum, perptr->relptr->path );
			return( -1 );
		}
	}

	if ( is_uerror_set( ) )	/* unpackerror */
		return( -1 );

	return( updcnt );
}
