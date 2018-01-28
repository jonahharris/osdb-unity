/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "uquery.h"
#include "uerror.h"

struct utuple *newtuple();

struct utuple *
gettuple( nodeptr )
struct qnode *nodeptr;
{
	register struct utupleblk *blkptr;
	register int cnt;
	register struct utuple *tplptr;

	if ( (nodeptr->flags & N_OPENED) == 0 )
	{
		set_uerror( UE_NOINIT );
		return( NULL );
	}

	if ( nodeptr->relio.fd < 0 )
	{
		/*
		 * Get next tuple from those in memory.
		 */
		if ( (blkptr = nodeptr->nextblk) == NULL )
			return( NULL );
		cnt = nodeptr->nexttpl;

		while( 1 )
		{
			while( cnt >= blkptr->tuplecnt )
			{
				if ( (blkptr = blkptr->next) == NULL )
					return( NULL );
				cnt = 0;
			}

			tplptr = blkptr->tuples[cnt++];
			if ( (tplptr->flags & (TPL_IGNORE|TPL_INVISIBLE)) == 0 )
				break;		/* found the next tuple */
		}

		nodeptr->nextblk = blkptr;
		nodeptr->nexttpl = cnt;
	}
	else if ( nodeptr->flags & N_SELFILTER )
	{
		/*
		 * Get tuple from file and see if it passes selection
		 * criteria.
		 */
		if ( (tplptr = newtuple( nodeptr )) == NULL )
			return( NULL );

		while( 1 )
		{
			if ( ! readtuple( nodeptr->rel, &nodeptr->relio,
					tplptr, nodeptr->memattr ) )
			{
				freetuple( tplptr );

				if ( ( nodeptr->relio.flags & UIO_FILEMASK ) == UIO_PACKED )
				{
					if ( ( packedclose( nodeptr->relio.fd ) ) &&
					     ( nodeptr->relio.flags & UIO_EOF ) )
					{
						nodeptr->relio.flags |= UIO_ERROR;
						set_uerror( UE_PACKEDEOF );
					}
				} else if ( ( nodeptr->relio.flags & UIO_STDIN ) == 0 )
					close( nodeptr->relio.fd );
				nodeptr->relio.fd = -1;
				nodeptr->nextblk = NULL;
				nodeptr->nexttpl = 0;
				nodeptr->flags &= ~N_SELFILTER;
				nodeptr->tmptpl = NULL;

				return( NULL );
			}

			if ( chktuple( (struct uquery *)NULL,
					(struct uqoperation *)nodeptr->tmptpl,
					nodeptr, tplptr, -1 ) )
				return( tplptr );

			if ( nodeptr->flags & N_IGNORE )
			{
				tplptr->flags |= TPL_IGNORE;
				if ( ! addtuple( (struct uquery *)NULL,
						nodeptr, tplptr ) )
					return( NULL );
				tplptr = newtuple( nodeptr );
				if ( tplptr == NULL )
					return( NULL );
			}
			else if ( (tplptr->flags & TPL_NULLTPL) == 0 )
			{
				unusedtuple( &nodeptr->relio,
					tplptr->tplval,
					nodeptr->rel->attrcnt );
			}
		}
	}
	else
	{
		/*
		 * Get the next tuple from relation file.
		 */
		if ( (tplptr = newtuple( nodeptr )) == NULL )
			return( NULL );

		if ( ! readtuple( nodeptr->rel, &nodeptr->relio,
				tplptr, nodeptr->memattr ) )
		{
			freetuple( tplptr );

			if ( ( nodeptr->relio.flags & UIO_FILEMASK ) == UIO_PACKED )
			{
				if ( ( packedclose( nodeptr->relio.fd ) ) &&
				     ( nodeptr->relio.flags & UIO_EOF ) )
				{
					nodeptr->relio.flags |= UIO_ERROR;
					set_uerror( UE_PACKEDEOF );
				}
			} else if ( ( nodeptr->relio.flags & UIO_STDIN ) == 0 )
				close( nodeptr->relio.fd );
			nodeptr->relio.fd = -1;

			return( NULL );
		}
	}

	return( tplptr );
}
