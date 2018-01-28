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

extern struct utuple **findjointpl();

/*ARGSUSED*/
forgettuple( query, nodeptr, tplptr )
struct uquery *query;
struct qnode *nodeptr;
struct utuple *tplptr;
{
	/*
	 * Free the tuple's space, but only if the node
	 * is coming directly from a file.
	 */
	if ( (nodeptr->relio.fd >= 0 || (nodeptr->flags & N_FORCESAVE)) &&
			(nodeptr->flags & N_APPTUPLES) == 0 &&
			(tplptr->flags & TPL_FREED) == 0 )
	{
		/*
		 * Free the tuple space.
		 */
		if ( (tplptr->flags & TPL_NULLTPL) == 0 )
		{
			unusedtuple( &nodeptr->relio, tplptr->tplval,
				nodeptr->rel->attrcnt );
		}
		freetuple( tplptr );
	}

	return( TRUE );
}

/*ARGSUSED*/
deltuple( query, nodeptr, tplptr )
struct uquery *query;
register struct qnode *nodeptr;
register struct utuple *tplptr;
{
	register struct utupleblk *blkptr;
	register int i, tplsize;

	tplsize = tplptr->tplsize;	/* save for future use */

	if ( nodeptr->flags & (N_APPTUPLES|N_IGNORE) )
	{
		/*
		 * All the tuples were supplied by the application or
		 * we're in the middle of evaluating the OR-expressions
		 * of a query.
		 *
		 * In either case, don't delete the tuples; we just mark
		 * the tuple as invisible and decrement the node's tuple
		 * count.
		 */
		if ( nodeptr->flags & N_IGNORE )
		{
			tplptr->flags |= TPL_IGNORE;

			if ( (nodeptr->flags & N_APPTUPLES) == 0 &&
				(nodeptr->relio.fd >= 0 ||
				(nodeptr->flags & N_FORCESAVE) ) )
			{
				/*
				 * Tuple came from file, so save the tuple
				 * in the node.
				 */
				return( addtuple( query, nodeptr, tplptr ) );
			}
		}
		else if ( nodeptr->flags & N_APPTUPLES )
		{
			tplptr->flags |= TPL_INVISIBLE;
		}

		/*
		 * Look for the tuple among those that are in memory.
		 */
		for( blkptr = nodeptr->tuples; blkptr; blkptr = blkptr->next )
		{
			for( i = 0; i < blkptr->tuplecnt; i++ )
			{
				if ( blkptr->tuples[i] == tplptr )
				{
					/*
					 * Found the tuple.  Decrement the
					 * tuple count and memory size.
					 */
					nodeptr->tuplecnt--;
					nodeptr->memsize -= tplsize;
					return( TRUE );
				}
			}
		}
		return( TRUE );
	}
	else if ( (tplptr->flags & TPL_FREED) == 0 )
	{
		/*
		 * The tuple's not free, yet.  Free its tuple space.
		 */
		if ( (tplptr->flags & TPL_NULLTPL) == 0 )
		{
			unusedtuple( &nodeptr->relio, tplptr->tplval,
				nodeptr->rel->attrcnt );
		}
		freetuple( tplptr );
	}

	/*
	 * Look for the tuple among those that are in memory.
	 */
	for( blkptr = nodeptr->tuples; blkptr; blkptr = blkptr->next )
	{
		for( i = 0; i < blkptr->tuplecnt; i++ )
		{
			if ( blkptr->tuples[i] == tplptr )
			{
				/*
				 * Found the tuple; adjust the tuple blocks.
				 */
				nodeptr->tuplecnt--;
				nodeptr->memsize -= tplsize;
				/*
				 * Do any adjustment of the traversal
				 * pointers that are needed.  This is so
				 * gettuple() still works.
				 */
				if ( nodeptr->nextblk == blkptr &&
						nodeptr->nexttpl > i )
					--nodeptr->nexttpl;
				/*
				 * Delete the tuple from the block.  Should
				 * really do garbage collection, but we don't
				 * bother - yet.
				 */
				--blkptr->tuplecnt;
				memcpy( &blkptr->tuples[i],
					&blkptr->tuples[i + 1],
					(blkptr->tuplecnt - i) *
						sizeof( struct utuple * ) );
				blkptr->tuples[blkptr->tuplecnt] = NULL;

				return( TRUE );
			}
		}
	}

	return( TRUE );
}

deljointuple( query, nodeptr, tplptr )
struct uquery *query;
struct qnode *nodeptr;
struct utuple *tplptr;
{
	register unsigned int i;
	struct joinblock *nextblk, *tmpblk;
	int nexttpl, tmptpl;
	struct utuple *tmpstpl[MAXRELATION];
	struct utuple **stplptr;
	int rc;

	rc = deltuple( query, nodeptr, tplptr );

	/*
	 * Go through the supernode of the node looking for
	 * references to the tuple.  For all other nodes of the
	 * super tuple check if the respective tuples have
	 * other references within their respective nodes.  If so
	 * delete the tuple from it's node.  Finally, delete the
	 * original tuple from the original node.
	 */
	if ( nodeptr->snode )
	{
		nextblk = nodeptr->snode->joinptr;
		nexttpl = 0;
		while( (stplptr = findjointpl( query->nodecnt,
					&nextblk, &nexttpl,
					nodeptr->nodenum, tplptr )) != NULL )
		{
			memcpy( tmpstpl, stplptr,
				query->nodecnt * sizeof( struct utuple * ) );
			memcpy( stplptr, &nextblk->tuples[ nexttpl ],
				(nextblk->blkcnt - nexttpl) *
					sizeof( struct utuple * ) );
			nextblk->blkcnt -= query->nodecnt;
			nexttpl -= query->nodecnt;

			for( i = 0; i < query->nodecnt; i++ )
			{
				if ( i == nodeptr->nodenum ||
					query->nodelist[i]->snode != nodeptr->snode )
				{
					continue;
				}

				tmpblk = nodeptr->snode->joinptr;
				tmptpl = 0;
				if ( findjointpl( query->nodecnt,
						&tmpblk, &tmptpl,
						i, tmpstpl[i] ) == NULL )
				{
					if ( ! deltuple( query,
							query->nodelist[i],
							tmpstpl[i] ) )
						rc = FALSE;
				}
			}
		}
	}

	return( rc );
}
