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
#ifdef DEBUG
#include "message.h"
#endif

extern struct utuple **newsupertpl(), **nullsupertpl();

struct utuple **
findjointpl( nodecnt, blklist, tindex, nodenum, tplptr )
int nodecnt;
struct joinblock **blklist;
int *tindex;
register unsigned int nodenum;
register struct utuple *tplptr;
{
	register struct joinblock *blkptr;
	register int i, start;

	start = *tindex;
	for( blkptr = *blklist; blkptr; blkptr = blkptr->next, start = 0 )
	{
		for( i = start; i < blkptr->blkcnt; i += nodecnt )
		{
			if ( blkptr->tuples[i + nodenum] == tplptr )
			{
				*blklist = blkptr;
				*tindex = i + nodecnt;
				return( &blkptr->tuples[i] );
			}
		}
	}

	*blklist = NULL;
	*tindex = 0;

	return( NULL );
}

merge_stpl( dest, src1, src2, cnt )
register struct utuple **dest;
register struct utuple **src1;
register struct utuple **src2;
int cnt;
{
	register int i;

	for( i = 0; i < cnt; i++ )
	{
		if ( src1[i] != NULL && src2[i] != NULL )
		{
			set_uerror( UE_QUERYEVAL );
			return( FALSE );
		}
		dest[i] = src1[i] ? src1[i] : src2[i];
	}

	return( TRUE );
}

/*ARGSUSED*/	
addjointuple( query, snode, node1, node2, tpl1, tpl2 )
struct uquery *query;
struct supernode *snode;
struct qnode *node1, *node2;
struct utuple *tpl1, *tpl2;
{
	struct joinblock *nextblk1, *nextblk2;
	int nexttpl1, nexttpl2;
	register struct utuple **newstpl;

	if ( snode == NULL )
	{
		/*
		 * No need to do anything.  No new result.
		 */
		return( TRUE );
	}

	if ( node1->snode == node2->snode )
	{
		if ( node1->snode == NULL )
		{
			/*
			 * Both nodes are singletons.  Add just these
			 * tuples to the super node.
			 */
			newstpl = newsupertpl( snode->joinptr, query );
			if ( newstpl == NULL )
				return( FALSE );

			newstpl[ node1->nodenum ] = tpl1;
			newstpl[ node2->nodenum ] = tpl2;
		}
		else
		{
			set_uerror( UE_QUERYEVAL );
			return( FALSE );
		}
	}
	else if ( node1->snode == NULL )
	{
		/*
		 * Node1 is a singleton, node2 is a supernode.
		 */
		register struct utuple **stpl;

		nextblk1 = node2->snode->joinptr;
		nexttpl1 = 0;
		stpl = findjointpl( query->nodecnt,
				&nextblk1, &nexttpl1,
				node2->nodenum, tpl2 );
		if ( stpl == NULL && (tpl2->flags & TPL_NULLTPL) != 0 )
		{
			stpl = nullsupertpl( query, node2, tpl2 );
			if ( stpl == NULL )
				return( FALSE );
		}
		while( stpl != NULL )
		{
			newstpl = newsupertpl( snode->joinptr, query );
			if ( newstpl == NULL )
				return( FALSE );

			memcpy( newstpl, stpl,
				query->nodecnt * sizeof( struct utuple * ));
			newstpl[ node1->nodenum ] = tpl1;

			stpl = findjointpl( query->nodecnt,
					&nextblk1, &nexttpl1,
					node2->nodenum, tpl2 );
		}
	}
	else if ( node2->snode == NULL )
	{
		/*
		 * Node2 is a singleton, node1 is a supernode.
		 */
		register struct utuple **stpl;

		nextblk1 = node1->snode->joinptr;
		nexttpl1 = 0;

		stpl = findjointpl( query->nodecnt, &nextblk1, &nexttpl1,
				node1->nodenum, tpl1 );
		if ( stpl == NULL && (tpl1->flags & TPL_NULLTPL) != 0 )
		{
			stpl = nullsupertpl( query, node1, tpl1 );
			if ( stpl == NULL )
				return( FALSE );
		}
		while( stpl != NULL )
		{
			newstpl = newsupertpl( snode->joinptr, query );
			if ( newstpl == NULL )
				return( FALSE );

			memcpy( newstpl, stpl,
				query->nodecnt * sizeof( struct utuple * ) );
			newstpl[ node2->nodenum ] = tpl2;

			stpl = findjointpl( query->nodecnt,
					&nextblk1, &nexttpl1,
					node1->nodenum, tpl1 );
		}
	}
	else
	{
		/*
		 * Both nodes are in different supernodes.
		 */
		register struct utuple **stpl1, **stpl2;

		nextblk1 = node1->snode->joinptr;
		nexttpl1 = 0;
		while( (stpl1 = findjointpl( query->nodecnt,
					&nextblk1, &nexttpl1,
					node1->nodenum, tpl1 )) != NULL )
		{
			nextblk2 = node2->snode->joinptr;
			nexttpl2 = 0;
			while( (stpl2 = findjointpl( query->nodecnt,
					&nextblk2, &nexttpl2,
					node2->nodenum, tpl2 )) != NULL )
			{
				newstpl = newsupertpl( snode->joinptr, query );
				if ( newstpl == NULL )
					return( FALSE );

				if ( ! merge_stpl( newstpl, stpl1, stpl2,
						query->nodecnt ) )
					return( FALSE );
			}
		}
	}

	return( TRUE );
}

/*ARGSUSED*/
fakejointpl( query, snode, node1, node2, tpl1, tpl2 )
struct uquery *query;
struct supernode *snode;
struct qnode *node1, *node2;
struct utuple *tpl1, *tpl2;
{
	return( TRUE );
}

#ifdef DEBUG
pr_supernode( snode, query, cnt )
struct supernode *snode;
struct uquery *query;
int cnt;
{
	short i, j, k;
	struct joinblock *blkptr;
	struct utuple **stpl;
	struct qnode *nodeptr;

	if ( cnt == 0 )
		cnt = -1;
	for( blkptr = snode->joinptr;
		blkptr != NULL && cnt != 0;
		blkptr = blkptr->next )
	{
		for( i = 0; i < blkptr->blkcnt && cnt != 0;
			i += query->nodecnt )
		{
			if ( cnt > 0 )
				cnt--;

			stpl = &blkptr->tuples[ i ];

			for( j = 0; j < query->nodecnt; j++ )
			{
				if ( stpl[ j ] == NULL )
					continue;

				nodeptr = query->nodelist[ j ];

				for( k = 0; k < nodeptr->rel->attrcnt; k++ )
				{
					if ( stpl[ j ]->tplval[ k ] == NULL )
						continue;

					prmsg( MSG_DEBUG, "%s(#%d).%s = %s",
						nodeptr->rel->path,
						j,
						nodeptr->rel->attrs[ k ].aname,
						stpl[ j ]->tplval[ k ] );
				}
			}

			prmsg( MSG_DEBUG, "**********" );
		}
	}
}
#endif
