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

extern struct supernode *newsupernode();
extern long snode_nodes();
extern struct utuple **newsupertpl();
extern struct utuple *gettuple();

static
joinresults( query, snode1ptr, snode2 )
struct uquery *query;
struct supernode **snode1ptr;
register struct supernode *snode2;
{
	register int i, j;
	register struct joinblock *blk1, *blk2;
	register struct utuple **stpl;
	register struct supernode *snode, *snode1;

	snode = newsupernode( 0L );
	if ( snode == NULL )
		return( FALSE );

	snode->next = snode2->next;

	snode1 = *snode1ptr;
	*snode1ptr = snode;

	for( i = 0; i < query->nodecnt; i++ )
	{
		if ( query->nodelist[i]->snode == snode1 ||
			query->nodelist[i]->snode == snode2 )
		{
			query->nodelist[i]->snode = snode;
			snode->node_present |= 1 << i;
		}
	}

	for( blk1 = snode1->joinptr; blk1 != NULL; blk1 = blk1->next )
	{
		for( i = 0; i < blk1->blkcnt; i += query->nodecnt )
		{
			for( blk2 = snode2->joinptr; blk2 != NULL;
				blk2 = blk2->next )
			{
				for( j = 0; j < blk2->blkcnt;
						j += query->nodecnt )
				{
					stpl = newsupertpl( snode->joinptr,
							query );
					if ( stpl == NULL )
						return( FALSE );

					if ( ! merge_stpl( stpl,
							&blk1->tuples[i],
							&blk2->tuples[j],
							query->nodecnt ) )
						return( FALSE );
				}
			}
		}
	}

	freesupernode( (struct supernode **)NULL, snode1 );
	freesupernode( (struct supernode **)NULL, snode2 );

	return( uerror == UE_NOERR );
}

static
addnode( result, nodeptr, more_oper )
register struct qresult *result;
struct qnode *nodeptr;
char more_oper;
{
	register int i;
	register struct utuple *tptr;
	register struct utuple **stpl;
	char all_in_file;
	struct supernode *snode, *oldsnode;

	/*
	 * This node has not been added to the result, yet.
	 * So add it to the result.  We add it at the head
	 * of the list so we'll re-traverse it's tuples the
	 * least number of times.  This is just in case the
	 * relation has not been read into memory, yet.
	 */
	snode = newsupernode( 0L );
	if ( snode == NULL )
		return( FALSE );

	all_in_file = TRUE;
	if ( result->snode == NULL )
	{
		oldsnode = NULL;
	}
	else
	{
		register struct qnode **nodelist;

		oldsnode = result->snode;
		snode->next = oldsnode->next;
		oldsnode->next = NULL;

		nodelist = result->query->nodelist;
		for( i = 0; i < result->query->nodecnt; i++ )
		{
			if ( nodelist[i]->snode == oldsnode )
			{
				nodelist[i]->snode = snode;
				snode->node_present |= 1 << i;
				if ( (nodelist[i]->flags & N_INFILE) == 0 )
					all_in_file = FALSE;
			}
		}
	}

	result->snode = snode;
	nodeptr->snode = snode;

	if ( (nodeptr->flags & N_OPENED) == 0 && ! more_oper &&
		(result->query->flags & (Q_SORT|Q_UNIQUE)) == 0 )
	{
		/*
		 * The file has not been brought into
		 * memory yet (and isn't needed for sorting
		 * or uniqueness, or other subsequent operations),
		 * so mark the result as such.
		 */
		nodeptr->flags |= N_INFILE;
		result->flags |= QR_INFILE;

		/*
		 * Now save the results of the old supernode in
		 * the new one.  We do this by just switching join
		 * blocks between the old and new super nodes.
		 */
		if ( oldsnode != NULL )
		{
			struct joinblock *blkptr;

			blkptr = oldsnode->joinptr;
			oldsnode->joinptr = snode->joinptr;
			snode->joinptr = blkptr;
		}
	}
	else if ( oldsnode == NULL || all_in_file )
	{
		if ( ! relreset( nodeptr ) )
			return( FALSE );

		while( (tptr = gettuple( nodeptr )) != NULL )
		{
			if ( ! addtuple( result->query, nodeptr, tptr ) )
				return( FALSE );

			stpl = newsupertpl( snode->joinptr, result->query );
			if ( stpl == NULL )
				return( FALSE );
			stpl[ nodeptr->nodenum ] = tptr;
		}
	}
	else
	{
		register struct joinblock *blkptr;

		if ( ! relreset( nodeptr ) )
			return( FALSE );

		while( (tptr = gettuple( nodeptr )) != NULL )
		{
			if ( ! addtuple( result->query, nodeptr, tptr ) )
				return( FALSE );

			for( blkptr = oldsnode->joinptr; blkptr != NULL;
				blkptr = blkptr->next )
			{
				for( i = 0; i < blkptr->blkcnt;
					i += result->query->nodecnt )
				{
					stpl = newsupertpl( snode->joinptr,
							result->query );
					if ( stpl == NULL )
						return( FALSE );

					memcpy( stpl, &blkptr->tuples[i],
						result->query->nodecnt * sizeof( struct utuple * ) );
					stpl[nodeptr->nodenum] = tptr;
				}
			}
		}
	}

	if ( oldsnode != NULL )
		freesupernode( (struct supernode **)NULL, oldsnode );

	return( uerror == UE_NOERR );
}

bldresult( result, more_oper )
struct qresult *result;
char more_oper;
{
	register int i;
	int rc = TRUE;

	for( i = 0; i < result->query->nodecnt; i++ )
	{
		register struct qnode *nodeptr;

		nodeptr = result->query->nodelist[i];
		if ( (nodeptr->flags & N_PROJECT) &&
				nodeptr->snode == NULL &&
				! addnode( result, nodeptr, more_oper ) )
			return( FALSE );
	}

	while( result->snode != NULL && result->snode->next != NULL )
	{
		/* multiple join results */
		if ( ! joinresults( result->query, &result->snode,
				result->snode->next ) )
			return( FALSE );
	}

	/* Call the tuple function if there is no sorting */
	if ( result->query->tplfunc != NULL &&
		(result->query->flags & (Q_SORT|Q_UNIQUE)) == 0 )
	{
		if ( ! tplfunc_prod( result ) )
			rc = FALSE;	/* unpack error */

		if ( ! emptyresult( result, TRUE ) )
			rc = FALSE;	/* unpack error */
	}

	return( rc );
}

finalresult( result )
struct qresult *result;
{
	int i;
	int rc = TRUE;

	for( i = 0; i < result->query->nodecnt; i++ )
	{
		register struct qnode *nodeptr;

		nodeptr = result->query->nodelist[i];
		if ( (nodeptr->flags & N_PROJECT) == 0 )
		{
			/*
			 * We don't do anything with the node's
			 * supernode (if any).  At this point the
			 * only operation that remains to be done
			 * is projection, and that will not involve
			 * this node, nor it's tuples in the supernode.
			 */
			if ( ! relclose( nodeptr, FALSE ) )
				return( FALSE );	/* unpack error */
		}
	}

	if ( result->snode != NULL && (result->query->flags & Q_SORT) )
	{
		register struct joinblock *blkptr;

		/*
		 * Sort the tuples.  We only sort the tuples here
		 * within each join block.  In nexttuple() we take
		 * care of sorting tuples among the blocks.
		 */
		for( blkptr = result->snode->joinptr; blkptr != NULL;
			blkptr = blkptr->next )
		{
			sort_tuples( result, blkptr );
		}
	}

	/* Call the tuple function if necessary */
	if ( result->query->tplfunc )
	{
		if ( ! tplfunc_prod( result ) )
			rc = FALSE;	/* unpack error */

		if ( ! emptyresult( result, FALSE ) )
			rc = FALSE;	/* unpack error */
	}

	return( rc );
}

emptyresult( result, intermediate )
struct qresult *result;
int intermediate;
{
	register struct supernode *snode, *next;

	int	rc = TRUE;

	/*
	 * Free all the join blocks of the supernodes and then free each
	 * supernode.
	 */
	for( snode = result->snode; snode; snode = next )
	{
		next = snode->next;

		freesupernode( (struct supernode **)NULL, snode );
	}

	result->snode = NULL;
	result->curblk = NULL;
	result->flags &= ~QR_INFILE;

	if ( ! intermediate )
	{
		register int i;

		for( i = 0; i < result->query->nodecnt; i++ ) {
			if ( ! relclose( result->query->nodelist[i], FALSE ) )
				rc = FALSE;	/* unpack error */
		}
	}

	return( rc );
}

freeresult( result )
struct qresult *result;
{
	return( emptyresult( result, FALSE ) );
}

append_result( result, old_snode )
struct qresult *result;
struct supernode **old_snode;
{
	struct joinblock **prevblk;

	if ( *old_snode == NULL )
	{
		*old_snode = result->snode;
		result->snode = NULL;
		result->curblk = NULL;
		return;
	}

	for( prevblk = &old_snode[0]->joinptr; *prevblk;
			prevblk = &prevblk[0]->next )
		;
	*prevblk = result->snode->joinptr;
	result->snode->joinptr = NULL;

	freesupernode( (struct supernode **)NULL, result->snode );

	result->snode = NULL;
}
