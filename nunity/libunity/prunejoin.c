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
#include "uerror.h"

extern struct utuple *gettuple();
extern int deltuple();
extern int addtuple();

long
prunejoin( query, jinfo, operptr )
struct uquery *query;
struct joininfo *jinfo;
struct uqoperation *operptr;
{
	register struct joinblock *blkptr;
	register int i;
	struct utuple *tpl1, *tpl2;
	long joincnt, delcnt;
	int rc;
	int nodecnt;
	struct qnode *nodelist[ MAXRELATION ];

	rc = TRUE;
	joincnt = 0L;

	nodecnt = 0;
	for( i = 0; i < query->nodecnt; i++ )
	{
		if ( query->nodelist[i]->snode == operptr->node1->snode )
			nodelist[ nodecnt++ ] = query->nodelist[i];
	}

	/*
	 * Both nodes are in the same supernode.  Find all supertuples
	 * containing both tuples.  Then call the tuple function for each one.
	 */
	for( blkptr = operptr->node1->snode->joinptr; rc && blkptr != NULL;
		blkptr = blkptr->next )
	{
		for( i = 0; rc && i < blkptr->blkcnt;  )
		{
			tpl1 = blkptr->tuples[ i + operptr->node1->nodenum ];
			tpl2 = blkptr->tuples[ i + operptr->node2->nodenum ];

			if ( (operptr->oper == UQOP_OUTERJOIN &&
				( (tpl1->flags & TPL_NULLTPL) != 0 ||
				(tpl2->flags & TPL_NULLTPL) != 0 ) ) ||
				chkjoinlist( operptr->cmplist, operptr->cmpcnt,
					operptr->node1, operptr->node2,
					tpl1, tpl2 ) )
			{
				register int j;

				for( j = 0; j < nodecnt; j++ )
					blkptr->tuples[ i + nodelist[j]->nodenum ]->flags |= TPL_JOINED;

				joincnt++;
				if ( (*jinfo->joinfunc)( query, jinfo->snode,
						operptr->node1, operptr->node2,
						tpl1, tpl2 ) <= 0 )
					rc = FALSE;

				i += query->nodecnt;	/* go to next tuple */
			}
			else
			{
				/* remove the super tuple */
				blkptr->blkcnt -= query->nodecnt;
				memcpy( &blkptr->tuples[ i ],
					&blkptr->tuples[ i + query->nodecnt ],
					(blkptr->blkcnt - i) *
						sizeof( struct utuple * ) );

				/* don't increment i */
			}
		}
	}

	if ( ! rc )
		return( -1L );

	/*
	 * Clean up each node in the super node and delete
	 * any tuples that did not participate in the join.
	 */
	delcnt = 0L;
	for( i = 0; i < nodecnt; i++ )
	{
		register int do_cnt;
		int (*delfunc)();

		if ( ! relreset( nodelist[i] ) )
		{
			rc = FALSE;
			break;
		}

		if ( nodelist[i] == operptr->node1 )
		{
			delfunc = jinfo->delfunc1;
			do_cnt = TRUE;
		}
		else if ( nodelist[i] == operptr->node2 )
		{
			delfunc = jinfo->delfunc2;
			do_cnt = TRUE;
		}
		else
		{
			delfunc = deltuple;
			do_cnt = FALSE;
		}

		while( (tpl1 = gettuple( nodelist[i] )) != NULL )
		{
			if ( tpl1->flags & TPL_JOINED )
				tpl1->flags &= ~TPL_JOINED;
			else
			{
				if ( (*delfunc)( query, nodelist[i],
						tpl1, jinfo, operptr ) <= 0 )
				{
					rc = FALSE;
					break;
				}
				if ( do_cnt )
					delcnt++;
			}
		}
		if ( uerror != UE_NOERR ) {
			rc = FALSE;
			break;
		}
	}

	/*
	 * Decide what to return.  If there was an error, return -1;
	 * otherwise, return the appropriate cnt.
	 */
	if ( ( ! rc ) || ( uerror != UE_NOERR ) )	/* unpack error */
		return( -1L );

	switch( operptr->oper ) {
	case UQOP_JOIN:
		return( joincnt );
	case UQOP_OUTERJOIN:
		return( joincnt + delcnt );
	default:
		return( -1L );
	}
}

long
pruneantijoin( query, jinfo, operptr )
struct uquery *query;
struct joininfo *jinfo;
struct uqoperation *operptr;
{
	register struct joinblock *blkptr;
	register int i;
	struct utuple *tpl1, *tpl2;
	long delcnt;
	int rc;
	int nodecnt;
	struct qnode *nodelist[ MAXRELATION ];

	rc = TRUE;

	nodecnt = 0;
	for( i = 0; i < query->nodecnt; i++ )
	{
		if ( query->nodelist[i]->snode == operptr->node1->snode )
			nodelist[ nodecnt++ ] = query->nodelist[i];
	}

	/*
	 * Both nodes are in the same supernode.  Find all supertuples
	 * containing both tuples.  Then call the tuple function for each one.
	 */
	for( blkptr = operptr->node1->snode->joinptr; rc && blkptr != NULL;
		blkptr = blkptr->next )
	{
		for( i = 0; rc && i < blkptr->blkcnt;  )
		{
			tpl1 = blkptr->tuples[ i + operptr->node1->nodenum ];
			tpl2 = blkptr->tuples[ i + operptr->node2->nodenum ];

			if ( chkjoinlist( operptr->cmplist, operptr->cmpcnt,
					operptr->node1, operptr->node2,
					tpl1, tpl2 ) )
			{
				register int j;

				/*
				 * Tuples match, so delete the super tuple
				 * and mark the tuples as joined.
				 */
				for( j = 0; j < nodecnt; j++ )
					blkptr->tuples[ i + nodelist[j]->nodenum ]->flags |= TPL_JOINED;

				/* remove the super tuple */
				blkptr->blkcnt -= query->nodecnt;
				memcpy( &blkptr->tuples[ i ],
					&blkptr->tuples[ i + query->nodecnt ],
					(blkptr->blkcnt - i) *
						sizeof( struct utuple * ) );

				/* don't increment i */
			}
			else	/* no match */
			{
				i += query->nodecnt;	/* go to next tuple */
			}
		}
	}

	if ( ! rc )
		return( -1L );

	/*
	 * Clean up each node in the super node and delete
	 * any tuples that participated in the join.
	 */
	delcnt = 0L;
	for( i = 0; i < nodecnt; i++ )
	{
		register int do_cnt;
		int (*delfunc)(), (*addfunc)();

		if ( ! relreset( nodelist[i] ) )
		{
			rc = FALSE;
			break;
		}

		if ( nodelist[i] == operptr->node1 )
		{
			delfunc = jinfo->delfunc1;
			addfunc = jinfo->addfunc1;
			do_cnt = TRUE;
		}
		else if ( nodelist[i] == operptr->node2 )
		{
			delfunc = jinfo->delfunc2;
			addfunc = jinfo->addfunc2;
			do_cnt = TRUE;
		}
		else
		{
			/*
			 * We're doing a set-difference, so delete
			 * tuples that match and keep those that don't.
			 */
			delfunc = addtuple;
			addfunc = deltuple;
			do_cnt = FALSE;
		}

		while( (tpl1 = gettuple( nodelist[i] )) != NULL )
		{
			if ( tpl1->flags & TPL_JOINED )
			{
				tpl1->flags &= ~TPL_JOINED;
				if ( (*addfunc)( query, nodelist[i],
						tpl1 ) <= 0 )
				{
					rc = FALSE;
					break;
				}
			}
			else
			{
				if ( (*delfunc)( query, nodelist[i],
						tpl1, jinfo, operptr ) <= 0 )
				{
					rc = FALSE;
					break;
				}
				if ( do_cnt )
					delcnt++;
			}
		}
		if ( uerror != UE_NOERR ) {
			rc = FALSE;
			break;
		}
	}

	/*
	 * Decide what to return.  If there was an error, return -1;
	 * otherwise, return the no match cnt.
	 */
	if ( ( ! rc ) || ( uerror != UE_NOERR ) )	/* unpack error */
		return( -1L );

	return( delcnt );
}
