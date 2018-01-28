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

int
relreset( nodeptr )
struct qnode *nodeptr;
{
	int	rc = TRUE;

	if ( nodeptr->flags & N_APPTUPLES )
	{
		nodeptr->flags |= N_OPENED;
		nodeptr->relio.fd = -1;
	}
	else if ( (nodeptr->flags & N_OPENED) == 0 )	/* not opened yet */
	{
		if ( nodeptr->rel->path == NULL ||
			*nodeptr->rel->path =='\0' ||
			! relropen( &nodeptr->relio, nodeptr->rel,
					(FILE *)NULL ) )
		{
			set_uerror( UE_ROPEN );
			return( FALSE );
		}

		nodeptr->flags |= N_OPENED;
		nodeptr->tuples = NULL;
		nodeptr->tuplecnt = 0;
		nodeptr->memsize = 0;
	}
	else if ( nodeptr->relio.fd >= 0 )
	{
		/*
		 * Relation was previously opened, but was not completely
		 * traversed.  We only look at in-memory tuples to avoid
		 * adding a tuple twice.
		 */
		if ( (nodeptr->relio.flags & UIO_STDIN) == 0 ) {
			if ( ( nodeptr->relio.flags & UIO_FILEMASK ) != UIO_PACKED )
				close( nodeptr->relio.fd );
			else
			{
				if ( ( packedclose( nodeptr->relio.fd ) ) &&
				     ( nodeptr->relio.flags & UIO_EOF ) )
				{
					nodeptr->relio.flags |= UIO_ERROR;
					set_uerror( UE_PACKEDEOF );
					rc = FALSE;
				}
			}
		}
		nodeptr->relio.fd = -1;
	}

	nodeptr->nextblk = nodeptr->tuples;
	nodeptr->nexttpl = 0;

	return( rc );
}


relclose( nodeptr, keeptuples )
register struct qnode *nodeptr;
int keeptuples;
{
	int	rc = TRUE;

	if ( nodeptr->relio.fd >= 0 ) {
		if ( ( nodeptr->relio.flags & UIO_FILEMASK ) == UIO_PACKED )
		{
			if ( ( packedclose( nodeptr->relio.fd ) ) &&
			     ( nodeptr->relio.flags & UIO_EOF ) )
			{
				nodeptr->relio.flags |= UIO_ERROR;
				set_uerror( UE_PACKEDEOF );
				rc = FALSE;
			}
		} else if ( ( nodeptr->relio.flags & UIO_STDIN ) == 0 )
			close( nodeptr->relio.fd );
		nodeptr->relio.fd = -1;
	} else if ((( nodeptr->relio.flags & UIO_FILEMASK ) == UIO_PACKED ) &&
		   (( nodeptr->relio.flags & UIO_ERROR ) == UIO_ERROR )) {
		/*
		 * Once an error has been detected when closing
		 * a packed relation we will continue to return
		 * a failure code in order to guard against any
		 * application programs that fail to check for
		 * an error after a while readtuple type of loop.
		 */
		rc = FALSE;
	}

	if ( keeptuples )
		return( rc );

	nodeptr->tuplecnt = 0;
	nodeptr->nextblk = NULL;
	nodeptr->nexttpl = 0;
	nodeptr->memsize = 0;

	if ( nodeptr->flags & N_APPTUPLES ) {
		register struct utupleblk *blkptr;
		register int i;

		/*
		 * This node has all tuples supplied by the application.
		 * So mark all the tuples as visible.  This is the opposite
		 * of the intuitive sense of "close", perhaps "rewind" is
		 * a more appropriate word.
		 */
		for ( blkptr = nodeptr->tuples; blkptr;
				blkptr = blkptr->next ) {
			for( i = 0; i < blkptr->tuplecnt; i++ ) {
				blkptr->tuples[i]->flags &= ~TPL_INVISIBLE;
				nodeptr->memsize += blkptr->tuples[i]->tplsize;
			}
			nodeptr->tuplecnt += blkptr->tuplecnt;
		}

		nodeptr->flags |= N_OPENED;
		nodeptr->flags &= ~(N_SELFILTER|N_FORCESAVE);
	}
	else {
		freetplblks( nodeptr->tuples, TRUE );
		if ( ! (relrclose( &nodeptr->relio ) ) )
			rc = FALSE;	/* unpack error */
		nodeptr->tuples = NULL;
		nodeptr->flags &= ~(N_OPENED|N_SELFILTER|N_FORCESAVE);
	}

	return( rc );
}
