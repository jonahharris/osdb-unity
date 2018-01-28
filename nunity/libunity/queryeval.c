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
#ifdef DEBUG
#include "message.h"
#include "qdebug.h"
#endif

/*
 * This file evaluates a query based on some relations, some query
 * expression, and a set of attributes to project from the results
 * of the query.  In algebraic terms it evaluates queries of the form:
 *
 * project( attributes, select( queryexpr, cartesion_product( db1, db2,...)))
 */

extern int deltuple(), deljointuple();
extern int addtuple(), addjointuple(), forgettuple(), outerjoin();
extern int fakejointpl();
extern int tplfunc_sel(), tplfunc_join();

extern struct utuple *gettuple();
extern struct supernode *newsupernode();
extern long snode_nodes();
extern void unsaveelse();

struct uqoperation *findjoin( );
struct uqoperation *findselect( );


static unsigned short _querycnt;

inquery()
{
	return( _querycnt != 0 );
}

static void
_startquery()
{
	++_querycnt;
}

static void
_endquery()
{
	if ( _querycnt != 0 )
		_querycnt--;
}

static void
rmv_ignore( nodeptr )
struct qnode *nodeptr;
{
	register struct utupleblk *blkptr;
	register int i;

	nodeptr->tuplecnt = 0;
	nodeptr->memsize = 0;
	nodeptr->snode = NULL;

	for( blkptr = nodeptr->tuples; blkptr; blkptr = blkptr->next )
	{
		for( i = 0; i < blkptr->tuplecnt; i++ )
		{
			blkptr->tuples[i]->flags &= ~TPL_IGNORE;
			nodeptr->tuplecnt++;
			nodeptr->memsize += blkptr->tuples[i]->tplsize;
		}
	}
}

int
queryeval( query, result )
struct uquery *query;
struct qresult *result;
{
	long havetuples;
	short do_project;
	char moreoper, sorting, result_saved;
	register int i, maxatt;
	struct uqoperation *operlist;
	struct uqoperation *operptr;
	UQOPERATION jointype;
	struct supernode *old_snode;

	extern long do_select(), do_join();

	if ( (query->flags & Q_INIT) == 0 )
	{
		set_uerror( UE_NOINIT );
		return( FALSE );
	}

	reset_uerror( );	/* no error has been done */

	init_cnvtdate( );	/* reset current date (year) for call to cnvtdate() */

	maxatt = get_attralloc();
	for( i = 0; i < query->nodecnt; i++ )
	{
		register struct qnode *nodeptr;

		nodeptr = query->nodelist[i];
		if ( nodeptr->rel->attrcnt > maxatt )
		{
			set_uerror( UE_NUMATTR );	/* not enough attr space */
			return( FALSE );
		}

		nodeptr->flags &= ~(N_INFILE|N_IGNORE);
		if ( nodeptr->flags & N_APPTUPLES )
			nodeptr->flags |= N_OPENED;
		else
		{
			nodeptr->flags &= ~N_OPENED;
			nodeptr->tuples = NULL;
		}
		nodeptr->relio.fd = -1;
		nodeptr->snode = NULL;
		nodeptr->tmptpl = NULL;
	}

	for( operptr = query->operlist; operptr; operptr = operptr->next )
		operptr->flags &= ~UQF_DONE;

	result->query = query;
	result->flags = 0;
	result->snode = NULL;
	result->curblk = NULL;

	if ( query->operlist == NULL && query->attrcnt == 0 )
		return( TRUE );

	_startquery( );		/* we're in a query now */

	/*
	 * "havetuples" can be 0 (no tuples), < 0 (query error), > 0 (normal).
	 */
	havetuples = 1;

	/*
	 * See if some operation needs to be performed after all
	 * the query operations are done.
	 */
	sorting = ( query->tplfunc == NULL ||
			(query->flags & (Q_SORT|Q_UNIQUE)) );

	old_snode = NULL;
	operlist = query->operlist;
	result_saved = TRUE;

	/*
	 * We do operations while there are any more to do and
	 * we haven't had an error in some operation.
	 */
	while( operlist != NULL && havetuples >= 0 )
	{
		char equijoin;

		result->flags = 0;
		result->snode = NULL;
		result->curblk = NULL;

		if ( operlist->flags & UQF_RELRESET )
		{
			/*
			 * Mark deleted tuples as ignored in each
			 * node; don't actually delete them.  If a node
			 * already had the ignore flag set, then we
			 * must "un-ignore" all its tuples.
			 */
			for( i = 0; i < query->nodecnt; i++ )
			{
				if ( query->nodelist[i]->flags & N_IGNORE )
					rmv_ignore( query->nodelist[i] );
				else
					query->nodelist[i]->flags |= N_IGNORE;
			}
		}

		/*
		 * Each OR-list represents a list of AND-ed operations.
		 * Therefore if any operation doesn't have any
		 * tuples, the whole OR-list fails.
		 */
		moreoper = TRUE;
		havetuples = 1L;
		while( moreoper && havetuples > 0 )
		{
			char hasjoin;

			operptr = findselect( query, operlist,
						&moreoper, &hasjoin );
			if ( operptr == NULL )
				break;

#ifdef DEBUG
			if ( _qdebug & QDBG_SELECT )
			{
				prmsg( MSG_DEBUG, "(queryeval): doing select operation (moreop=%u, otherop=%u):",
					moreoper, sorting );
				pr_oper( operptr );
			}
#endif

			result_saved = (moreoper || sorting);

			havetuples = do_select( query, operptr, hasjoin,
						result_saved );
			operptr->flags |= UQF_DONE;
		}

		/*
		 * Now do the join operations.  This while-loop will
		 * handle all types of joins.  We'll start with
		 * normal joins and outer joins, then anti-joins
		 * (i.e., set difference).  This order is NOT arbitrary;
		 * all joins must be done before anti-joins to get
		 * the correct result.
		 *
		 * For efficiency sake, we try to delay non-equi-joins
		 * as long a possible.
		 *
		 * With all these considerations, the order of
		 * operation evaluation is:
		 *
		 *	equi-join
		 *	equi-outer-join
		 *	non-equi-join
		 *	non-equi-outer-join
		 *	equi-antijoin
		 *	non-equi-antijoin
		 */
		jointype = UQOP_JOIN;
		equijoin = TRUE;
		while( moreoper && havetuples > 0 )
		{
			operptr = findjoin( result, operlist, &moreoper,
					jointype, equijoin );
			if ( operptr == NULL )
			{
				/* Go to next type of join */

				if ( jointype == UQOP_JOIN )
					jointype = UQOP_OUTERJOIN;
				else if ( jointype == UQOP_OUTERJOIN )
				{
					if ( equijoin )
					{
						/* Do non-equi-joins */
						equijoin = FALSE;
						jointype = UQOP_JOIN;
					}
					else
					{
						equijoin = TRUE;
						jointype = UQOP_ANTIJOIN;
					}
				}
				else  /* jointype == UQOP_ANTIJOIN  */
				{
					if ( equijoin )
						equijoin = FALSE;
					else
						break;	/* all done */
				}

				continue;
			}
#ifdef DEBUG
			if ( _qdebug & QDBG_JOIN )
			{
				prmsg( MSG_DEBUG, "(queryeval): doing join operation (moreop=%u, otherop=%u):",
					moreoper, sorting );
				pr_oper( operptr );
			}
#endif
			result_saved = (moreoper || sorting);

			havetuples = do_join( result, operptr, result_saved );
			operptr->flags |= UQF_DONE;
		}

		/*
		 * Now advance past all the operations we've just
		 * done.
		 */
		do_project = FALSE;
		operptr = operlist;
		do {
			if ( operlist != operptr &&
				(operptr->flags & UQF_RELRESET) )
			{
				/* found next OR-list of operations */
				break;
			}

			if ( operptr->flags & UQF_PROJECT )
				do_project = TRUE;

			if ( havetuples > 0 &&
				(operptr->flags & UQF_DONE) == 0 )
			{
				set_uerror( UE_QUERYEVAL );
				havetuples = -1L;
				break;
			}

			operptr = operptr->next;

		} while( operptr != NULL );

		/* Now reset the operation list */
		operlist = operptr;

		/*
		 * Take care of the result of this set of operations.
		 */
		if ( ! do_project )
		{
			/*
			 * We've only partially completed the
			 * query.  Go back and do the OR'ed clauses.
			 */
			continue;
		}
		else if ( query->attrcnt == 0 )
		{
			/*
			 * No tuples needed.  Just
			 * return the first time we find
			 * some tuple matching everything.
			 */
			emptyresult( result, FALSE );
			if ( havetuples != 0L )
			{
				_endquery();
				return( havetuples > 0L );
			}
		}
		else if ( havetuples <= 0L || ! result_saved )
		{
			/*
			 * There was an error or the query results
			 * were already projected (i.e., not saved).
			 * We empty out the result structure for the
			 * next set of operations (if there is any).
			 */
			emptyresult( result, TRUE );
		}
		else
		{
			/*
			 * Build the result of this set of operations.
			 */
			if ( ! bldresult( result, operlist != 0 ) )
			{
				_endquery();
				return( FALSE );
			}

			/*
			 * If sorting is NOT set then the we have multiple
			 * super nodes that were merged.
			 */
			if ( sorting )
				append_result( result, &old_snode );
		}

	} /* end while operlist != 0 && havetuples >= 0L */

	/* Put together the final result if necessary */
	if ( result_saved && (havetuples > 0L ||
		(havetuples >= 0L && old_snode != NULL) ) )
	{
		/* save the merge of all snodes */
		if ( sorting )
			result->snode = old_snode;

		/*
		 * Must rebuild the result just in case there
		 * were no operations to do.  We wouldn't
		 * have built a result previously in that case.
		 *
		 * However, if havetuples == 0 then we do not
		 * want to attempt to rebuild the result since
		 * that will destroy any previously saved
		 * results that were saved due to sorting etc.
		 */
		if ( ( havetuples > 0L ) && ( ! bldresult( result, FALSE ) ) )
		{
			emptyresult( result, FALSE );
			_endquery();
			return( FALSE );
		}

		if ( ! finalresult( result ) )
		{
			_endquery();
			return( FALSE );
		}
	}
	else
		emptyresult( result, FALSE );

	_endquery();

	return( query->attrcnt == 0 ? havetuples > 0L : havetuples >= 0L );
}

static int
chkidxattr( nodeptr, qptr )
register struct qnode *nodeptr;
register struct queryexpr *qptr;
{
	register struct attrref *attrptr;
	register int i;

	while( ISBOOL( qptr->optype ) )
	{
		if ( chkidxattr( nodeptr, qptr->elem1.expr ) == FALSE )
			return( FALSE );
		qptr = qptr->elem2.expr;
	}

	if ( ISSETCMP( qptr->optype ) )
	{
		attrptr = qptr->elem1.alist.list;
		if ( attrptr->rel != nodeptr )
			return( -1 );
		/*
		 * We can only do indexes on string comparisons.
		 * Additionally, if a "NOT-IN" operation is specified,
		 * it is generally faster to not use indexes.  So in
		 * these cases, return FALSE.
		 */
		if ( qptr->cmptype != QCMP_STRING || qptr->optype == OPNOTIN )
			return( FALSE );

		for( i = 0; i < qptr->elem1.alist.cnt; i++, attrptr++ )
		{
			if ( ATTR_SPECIAL( attrptr->attr ) ) /* rec#, seek# */
				return( FALSE );

			nodeptr->memattr[attrptr->attr] |= MA_MARKER;
			if ( (nodeptr->memattr[attrptr->attr] & MA_INDEX) == 0 )
				return( FALSE );
		}

		return( TRUE );
	}

	switch( qptr->opflags & (ISATTR1|ISATTR2) ) {
	case ISATTR1:
		attrptr = &qptr->elem1.attr;
		break;
	case ISATTR2:
		attrptr = &qptr->elem2.attr;
		break;
	case (ISATTR1|ISATTR2):
		return( qptr->elem1.attr.rel == nodeptr ||
				qptr->elem2.attr.rel == nodeptr ?
			FALSE : -1 );
	default:
		return( -1 );
	}

	if ( attrptr->rel != nodeptr )
		return( -1 );

	if ( ATTR_SPECIAL( attrptr->attr ) || qptr->cmptype != QCMP_STRING )
		return( FALSE );

	nodeptr->memattr[attrptr->attr] |= MA_MARKER;

	return( (nodeptr->memattr[attrptr->attr] & MA_INDEX) ?
			TRUE : FALSE );
}

static int
useindex( operptr, nodeptr )
struct uqoperation *operptr;
struct qnode *nodeptr;
{
	register int i, j;
	register int indexcnt, maxcnt;
	int found;

	found = -1;

	/*
	 * Can't use indexes if we need to project the record number.
	 * If the relation is already in memory, we could use indexes,
	 * but this is not efficient, since the tuples are not sorted
	 * by seek#.  If they were, it would be efficient, but that
	 * would require some extra coding in addtuple().
	 */
	if ( (nodeptr->flags & N_PROJRECNUM) != 0 )
		return( found );

	/*
	 * We can only use indexes if there is an index for all
	 * the attributes used in all comparisons.
	 */
	maxcnt = MAXATT;
	for( i = 0; i < operptr->cmpcnt; i++ )
	{
		for( j = 0; j < nodeptr->rel->attrcnt; j++ )
			nodeptr->memattr[j] &= ~MA_MARKER;

		/*
		 * chkidxattr() can return TRUE, FALSE, or -1 (not apply).
		 * We can only use indexes on a TRUE return.
		 */
		if ( chkidxattr( nodeptr, operptr->cmplist[i] ) != TRUE )
			continue;

		indexcnt = 0;
		for( j = 0; j < nodeptr->rel->attrcnt; j++ )
		{
			if ( nodeptr->memattr[j] & MA_MARKER )
				indexcnt++;
		}
		if ( indexcnt == 1 )	/* only one attr indexed -- best case */
			return( i );
		else if ( indexcnt < maxcnt )
		{
			found = i;
			maxcnt = indexcnt;
		}
	}

	return( found );
}

static long
matchtuples( query, operptr, nodeptr, addfunc )
struct uquery *query;
struct uqoperation *operptr;
register struct qnode *nodeptr;
int (*addfunc)();
{
	register struct utuple *tptr;
	register long tuplecnt;
	int rc;

	/*
	 * Find the tuples of a single relation that match
	 * the given conditions of the operation.
	 */
	tuplecnt = 0L;
	rc = 1;
	if ( ! relreset( nodeptr ) )
		rc = -1;
	while( rc > 0 && (tptr = gettuple( nodeptr )) != NULL )
	{
		if ( chktuple( query, operptr, nodeptr, tptr, -1 ) )
		{
			tuplecnt++;
			if ( ! (*addfunc)( query, nodeptr, tptr ) )
				rc = -1;
		}
		else if ( ! deltuple( query, nodeptr, tptr ) )
			rc = -1;
	}

	if ( is_uerror_set( ) )	/* unpack error */
		rc = -1;

	/* remove tuple refs saved for else's */
	unsaveelse( operptr, nodeptr );

	return( rc < 0 ? (long) rc : tuplecnt );
}

static long
do_select( query, operptr, hasjoin, moreoper )
struct uquery *query;
struct uqoperation *operptr;
char hasjoin;
char moreoper;
{
	register long cnt;
	struct qnode *nodeptr;
	char useselse, tuple_by_tuple;
	int edgenum;
	int (*addfunc)();
	extern long tuple_by_index();

	useselse = ( operptr->cmplist[0]->opflags & HASELSE ) ? TRUE : FALSE;
	nodeptr = operptr->node1;

	addfunc = moreoper ? addtuple : tplfunc_sel;

	tuple_by_tuple = TRUE;
	if ( (edgenum = useindex( operptr, nodeptr )) >= 0 )
	{
		/*
		 * All the attributes in the expression for the returned edge
		 * are indexed.  We can look up the tuples that match the
		 * individual comparisons.
		 */
#ifdef DEBUG
		if ( _qdebug & QDBG_SELECT )
		{
			prmsg( MSG_DEBUG, "(do_select): using indexes for select on %s",
				nodeptr->rel->path );
		}
#endif
		/*
		 * Get the tuples matching the index.  It's usually more
		 * efficient to read in all the tuples at once, rather
		 * than doing a selection filter as in a tuple-by-tuple
		 * selection.
		 */
		cnt = tuple_by_index( operptr, nodeptr, edgenum, query, addfunc );
		if ( cnt >= 0L )	/* no need to go through tuple-by-tuple */
			tuple_by_tuple = FALSE;
		else
		{
			/*
			 * The look-up failed on the indexes.  So delete any
			 * tuples associated with the node and get ready to
			 * go through the tuples one-by-one.
			 */
			if ( ! relclose( nodeptr, FALSE ) )
				return(-1);	/* unpack error */
			reset_uerror( );
		}
	}

	if ( tuple_by_tuple )
	{
		/*
		 * Some attribute in the expression is not indexed.  So, since
		 * we have to go through all the tuples anyway, we compare
		 * the conditions to each tuple in the relation.
		 *
		 * If there is some future join operation on this node
		 * and we don't have an else-clause and the relation is
		 * still in a file, then we can implement the selection
		 * as a filter.
		 */
		if ( hasjoin && ! useselse &&
			(nodeptr->flags & (N_SELFILTER|N_OPENED|N_APPTUPLES)) == 0 )
		{
			/*
			 * Implement the selection as a filter in gettuple().
			 * This saves the overhead of saving all the tuples
			 * that match the selection.  This can only be done
			 * if there is no else-clause for the node.  It
			 * usually only saves time if there is a join on the
			 * node, we're not on the last operation for the node,
			 * and we're not using indexes to do the selection.
			 */
			nodeptr->flags |= N_SELFILTER;
			nodeptr->tmptpl = (struct utuple *)operptr;

#ifdef DEBUG
			if ( _qdebug & QDBG_SELECT )
			{
				prmsg( MSG_DEBUG, "(do_select): using filter for select on %s",
					nodeptr->rel->path );
			}
#endif
			return( 1L );
		}

#ifdef DEBUG
		if ( _qdebug & QDBG_SELECT )
		{
			prmsg( MSG_DEBUG, "(do_select): going tuple-by-tuple for select on %s",
				nodeptr->rel->path );
		}
#endif

		cnt = matchtuples( query, operptr, nodeptr, addfunc );
	}

#ifdef DEBUG
	if ( _qdebug & QDBG_SELECT )
	{
		prmsg( MSG_DEBUG, "(do_select): found up to %ld tuples from select on %s",
			cnt, nodeptr->rel->path );
	}
#endif

	return( cnt );
}

static long
tpl_estimate( nodeptr )
struct qnode *nodeptr;
{
	/*
	 * Return the ratio of relation space to attribute counts
	 * (a rough estimate of the number of tuples).
	 */
	return( (nodeptr->flags & N_OPENED) ?
		nodeptr->memsize / nodeptr->rel->attrcnt :
		nodeptr->relsize / nodeptr->rel->attrcnt );
}

static long
smaller_node( n1, n2 )
register struct qnode *n1, *n2;
{
	/*
	 * Compare the nodes' tuple counts if possible.  If not,
	 * we compare the ratio of relation space to attribute counts
	 * (a rough estimate of the number of tuples).
	 */
	if ( (n1->flags & N_OPENED) && (n2->flags & N_OPENED) )
		return( n1->tuplecnt - n2->tuplecnt );
	else
		return( tpl_estimate( n1 ) - tpl_estimate( n2 ) );
}

static
cntprojnodes( query )
struct uquery *query;
{
	int i, cnt;

	cnt = 0;
	for( i = 0; i < query->nodecnt; i++ )
	{
		if ( query->nodelist[i]->flags & N_PROJECT )
			cnt++;
	}

	return( cnt );
}

static struct uqoperation *
findselect( query, operlist, moreoper, hasjoin )
struct uquery *query;
struct uqoperation *operlist;
char *moreoper;
char *hasjoin;
{
	struct uqoperation *operptr;
	struct uqoperation *smallest;

	smallest = NULL;
	*moreoper = FALSE;
	*hasjoin = FALSE;
	for( operptr = operlist; operptr; operptr = operptr->next )
	{
		if ( operptr != operlist &&
				(operptr->flags & UQF_RELRESET) )
			break;

		if ( operptr->flags & UQF_DONE )
			continue;

		if ( operptr->oper != UQOP_SELECT )
		{
			/* some other operation to do in this OR-list */
			*moreoper = TRUE;
			continue;
		}

		/*
		 * Find the select operation with the smallest node in it.
		 * Also note if there is some other select yet to do.
		 */
		if ( smallest == NULL )
			smallest = operptr;
		else
		{
			*moreoper = TRUE;
			if ( smaller_node( operptr->node1, smallest->node1 ) < 0 )
				smallest = operptr;
		}
	}

	if ( smallest == NULL )
		return( NULL );

	if ( ! *moreoper )
	{
		/*
		 * If there is an else in this selection, then
		 * there are more operations to do.
		 */
		if ( smallest->cmplist[0]->opflags & HASELSE )
			*moreoper = TRUE;

		/*
		 * If this is not the only projected node,
		 * then there are more operations to do.
		 */
		if ( (smallest->node1->flags & N_PROJECT) == 0 ||
			cntprojnodes( query ) != 1 )
		{
			*moreoper = TRUE;
		}

		return( smallest );
	}

	/*
	 * Check if there is a future join operation involving
	 * this node.  This will be used to see if the selection
	 * can be done as filter or not.
	 */
	*hasjoin = FALSE;
	for( operptr = operlist; operptr; operptr = operptr->next )
	{
		if ( operptr != operlist &&
				(operptr->flags & UQF_RELRESET) )
			break;

		if ( operptr->flags & UQF_DONE )
			continue;

		switch( operptr->oper ) {
		case UQOP_JOIN:
		case UQOP_OUTERJOIN:
		case UQOP_ANTIJOIN:
			if ( operptr->node1 == smallest->node1 ||
				operptr->node2 == smallest->node1 )
			{
				*hasjoin = TRUE;
				return( smallest );
			}
			break;
		}
	}

	return( smallest );
}

static int
cntsnodes( result )
struct qresult *result;
{
	register struct supernode *snode;
	register int cnt, i;

	cnt = 0;
	for( snode = result->snode; snode; snode = snode->next )
		cnt++;

	for( i = 0; i < result->query->nodecnt; i++ )
	{
		/*
		 * If there's not supernode for this node and it's
		 * projected and no join (i.e., normal or outer join)
		 * has been done on it, then a cross-product will be
		 * needed so increment the count.
		 *
		 * Note:  The only way to have no supernode but still
		 * a join done on a relation is if it was the last
		 * operation of the query and the tuples were just
		 * passed to a tuple function.
		 */
		if ( result->query->nodelist[i]->snode == NULL &&
			(result->query->nodelist[i]->flags & (N_PROJECT|N_JOINDONE)) == N_PROJECT )
		{
			cnt++;
		}
	}

	return( cnt );
}

static long
smaller_join( oper1, oper2 )
struct uqoperation *oper1;
struct uqoperation *oper2;
{
	if ( (oper1->node1->flags & N_OPENED) &&
		(oper1->node2->flags & N_OPENED) &&
		(oper2->node1->flags & N_OPENED) &&
		(oper2->node2->flags & N_OPENED) )
	{
		/*
		 * All nodes have been previously opened, so
		 * we can use tuple counts to estimate the
		 * join possibilities.
		 */
		return( oper1->node1->tuplecnt * oper1->node2->tuplecnt -
			oper2->node1->tuplecnt * oper2->node2->tuplecnt );
	}
	else
	{
		/*
		 * The nodes are not all in memory, so we get a
		 * rough estimate of the number of tuples for
		 * estimating the join possibilities.
		 */
		return( tpl_estimate( oper1->node1 ) *
				tpl_estimate( oper1->node2 ) -
			tpl_estimate( oper2->node1 ) *
				tpl_estimate( oper2->node2 ) );
	}
}

static struct uqoperation *
findjoin( result, operlist, moreoper, jointype, equijoin )
struct qresult *result;
struct uqoperation *operlist;
char *moreoper;
UQOPERATION jointype;	/* UQOP_ANTIJOIN, UQOP_OUTERJOIN or UQOP_JOIN */
char equijoin;
{
	struct uqoperation *operptr;
	struct uqoperation *smallest;
	int snodecnt;
	int i;

	snodecnt = cntsnodes( result );

	smallest = NULL;
	*moreoper = (snodecnt > 2);
	for( operptr = operlist; operptr; operptr = operptr->next )
	{

		if ( operptr != operlist &&
				(operptr->flags & UQF_RELRESET) )
			break;

		if ( operptr->flags & UQF_DONE )
			continue;

		if ( operptr->oper != jointype ||
			(equijoin && operptr->cmpcnt == operptr->nonequal ) )
		{
			/* some other operation to do in this OR-list */
			*moreoper = TRUE;
			continue;
		}

		/* Check which node is smaller in this operation */
		if ( smaller_node( operptr->node2, operptr->node1 ) < 0 )
		{
			struct qnode *tmp;

			tmp = operptr->node2;
			operptr->node2 = operptr->node1;
			operptr->node1 = tmp;
		}

		/*
		 * Find the join operation with the smallest nodes in it.
		 * Also note if there is some other operation yet to do.
		 */
		if ( smallest == NULL )
		{
			smallest = operptr;
		}
		else
		{
			/*
			 * There's another operation, either operptr or
			 * the current smallest.
			 */
			*moreoper = TRUE;

			if ( smaller_join( operptr, smallest ) < 0 )
			{
				smallest = operptr;
			}
		}
	}

	if ( smallest == NULL )
	{
		/*
		 * We don't have any join to do so if there's
		 * more than one supernode then a cross-product
		 * is needed.
		 */
		if ( snodecnt > 1 )
		{
			*moreoper = TRUE;
		}
		return( NULL );
	}
	else if ( *moreoper == TRUE )
		return( smallest );

	/*
	 * We have a "jointype" operation. Look for special
	 * cases that preclude us calling any tuple function directly.
	 * We only look for things specific to the join operations
	 * themselves; other conditions (such as sorting) are checked by
	 * the queryeval() function.
	 */
	switch( jointype ) {
	case UQOP_JOIN:
	case UQOP_OUTERJOIN:
		/*
		 * For normal and outer joins, we must make sure this
		 * join puts together all the nodes that are projected.
		 */
		for( i = 0; i < result->query->nodecnt; i++ )
		{
			register struct qnode *nodeptr;

			/*
			 * Look for any other node that is not part of the
			 * super-nodes of the nodes being joined.  If there
			 * is one, then a cross-product must be done after
			 * the join.
			 */
			nodeptr = result->query->nodelist[i];
			if ( (nodeptr->flags & N_PROJECT) &&
				nodeptr != smallest->node1 &&
				nodeptr != smallest->node2 &&
				( nodeptr->snode == NULL ||
				(nodeptr->snode != smallest->node1->snode &&
				nodeptr->snode != smallest->node2->snode )) )
			{
				*moreoper = TRUE;
				break;
			}
		}
		break;

	case UQOP_ANTIJOIN:
		/*
		 * For anti-joins, check the following cases to see if there
		 * are more query operations:
		 *
		 * 1. If neither relation is projected then we need to
		 *    display the results of those that are.
		 * 2. If more than one relation is projected, then we will
		 *    need to build up a cross product from the result of
		 *    this anti-join.
		 */
		if ( (smallest->node1->flags & N_PROJECT) == 
				(smallest->node2->flags & N_PROJECT) )
		{
			*moreoper = TRUE;  /* neither or both projected */
		}
		else if ( cntprojnodes( result->query ) != 1 )
		{
			*moreoper = TRUE;
		}
		break;
	}

	return( smallest );
}

#ifdef DEBUG

struct funcname {
	int (*func)();
	char *str;
};

struct funcname jfunclist[] = {
	addjointuple,	"addjointuple",
	tplfunc_join,	"tplfunc_join",
	fakejointpl,	"fakejointpl",

	deltuple,	"deltuple",
	addtuple,	"addtuple",
	tplfunc_sel,	"tplfunc_sel",
	forgettuple,	"forgettuple",
	deljointuple,	"deljointuple",
	outerjoin,	"outerjoin",
};

#define MAXJFUNC( list )	(sizeof( list ) / sizeof( struct funcname ))

static char *
joinfunc_to_str( func )
int (*func)();
{
	struct funcname *funcptr;

	for( funcptr = jfunclist;
		funcptr < &jfunclist[ MAXJFUNC( jfunclist ) ];
		funcptr++ )
	{
		if ( funcptr->func == func )
			return( funcptr->str );
	}

	return( "**UNKNOWN**" );
}
#endif /* DEBUG */

static long
do_join( result, operptr, moreoper )
register struct qresult *result;
struct uqoperation *operptr;
char moreoper;
{
	register int i;
	register long tmpcnt;
	struct joininfo jinfo;
	extern long pruneantijoin(), prunejoin(), hashjoin(), blkjoin();

	/*
	 * If doing joins (not anti-joins), create a new supernode
	 * for the join result based on the two nodes and whether
	 * they are already in a supernode or not.  (Anti-joins do
	 * NOT join tuples together, the tuples are left in their
	 * individual nodes.)
	 */
	if ( ! moreoper || operptr->oper == UQOP_ANTIJOIN )
	{
		/*
		 * We're on the last operation or we're doing
		 * anti-joins.  There is no need to create a new
		 * super node.  The join code will pass
		 * any matching tuples to the application's tuple
		 * function if this is the last operation or
		 * just store the tuples in their individual nodes
		 * for anti-joins.
		 */
		jinfo.snode = NULL;
	}
	else if ( operptr->node1->snode == operptr->node2->snode &&
		operptr->node1->snode != NULL )
	{
		/*
		 * Both nodes are already in the same super node.
		 * (A cycle in the query causes this.)  We'll just
		 * prune the existing super-node, so no new one is needed.
		 */
		jinfo.snode = NULL;
	}
 	else
	{
		/*
		 * Set up a new supernode for the join result and
		 * copy all the node information to it.
		 */
		jinfo.snode = newsupernode( snode_nodes( operptr->node1,
						operptr->node2 ) );
		jinfo.snode->next = result->snode;
		result->snode = jinfo.snode;
	}

	if ( operptr->oper == UQOP_ANTIJOIN )
	{
		jinfo.joinfunc = fakejointpl;
		jinfo.addfunc1 = (operptr->node1->snode == NULL) ? deltuple :
					deljointuple;
		jinfo.addfunc2 = (operptr->node2->snode == NULL) ? deltuple :
					deljointuple;
		if ( moreoper )
		{
			jinfo.delfunc1 = addtuple;
			jinfo.delfunc2 = addtuple;
		}
		else
		{
#ifdef DEBUG
			if ( (operptr->node1->flags & N_PROJECT) &&
				(operptr->node2->flags & N_PROJECT) )
			{
				set_uerror( UE_QUERYEVAL );
				return( -1L );
			}
#endif
			/*
			 * We're on the last operation.  If either node
			 * has not been read in yet, we don't want to
			 * bother stuffing it in the node.  So we set the add
			 * function for the nodes to be forgettuple().
			 * This function just free's the tuple.  addtuple() is
			 * a no-op if the node is in memory.
			 */
			if ( operptr->node1->flags & N_PROJECT )
			{
				jinfo.delfunc1 = tplfunc_sel;
				jinfo.delfunc2 =
					(operptr->node2->flags & (N_OPENED|N_IGNORE)) ?
						addtuple :
						forgettuple;
			}
			else
			{
				jinfo.delfunc1 =
					(operptr->node1->flags & (N_OPENED|N_IGNORE)) ?
						addtuple :
						forgettuple;
				jinfo.delfunc2 = tplfunc_sel;
			}
		}
	}
	else	/* normal or outer join */
	{
		jinfo.delfunc1 = operptr->oper == UQOP_OUTERJOIN ?
					(operptr->flags & UQF_OJDIRECT) &&
						operptr->cmplist[0]->elem1.attr.rel != operptr->node1 ?
					deltuple :
					outerjoin :
				operptr->node1->snode == NULL ? deltuple :
				deljointuple;
		jinfo.delfunc2 = operptr->oper == UQOP_OUTERJOIN ?
					(operptr->flags & UQF_OJDIRECT) &&
						operptr->cmplist[0]->elem1.attr.rel != operptr->node2 ?
					deltuple :
					outerjoin :
				operptr->node2->snode == NULL ? deltuple :
				deljointuple;

		if ( moreoper )
		{
			/*
			 * There are more operations to perform, so save
			 * tuples in the nodes and/or supernode.
			 */
			jinfo.joinfunc = addjointuple;
			jinfo.addfunc1 = addtuple;
			jinfo.addfunc2 = addtuple;
		}
		else
		{
			jinfo.joinfunc = tplfunc_join;
			/*
			 * We're on the last operation.  If either node
			 * has not been read in yet, we don't want to
			 * bother stuffing it in the node.  So we set the add
			 * function for the nodes to be forgettuple().
			 * This function just free's the tuple.  addtuple() is
			 * a no-op if the node is in memory.
			 */
			jinfo.addfunc1 = (operptr->node1->flags & (N_OPENED|N_IGNORE)) ?
						addtuple : forgettuple;
			jinfo.addfunc2 = (operptr->node2->flags & (N_OPENED|N_IGNORE)) ?
						addtuple : forgettuple;
		}
	}

#ifdef DEBUG
	if ( _qdebug & QDBG_JOIN )
	{
		prmsg( MSG_DEBUG, "(do_join): joinfunc=%s(), add1=%s(), add2=%s(), del1=%s(), del2=%s()",
			joinfunc_to_str( jinfo.joinfunc ),
			joinfunc_to_str( jinfo.addfunc1 ),
			joinfunc_to_str( jinfo.addfunc2 ),
			joinfunc_to_str( jinfo.delfunc1 ),
			joinfunc_to_str( jinfo.delfunc2 ) );
	}
#endif

	if ( operptr->node1->snode != NULL &&
		operptr->node1->snode == operptr->node2->snode )
	{
		/*
		 * Relations were previously already joined.  Just
		 * go through their supernode and look for matching
		 * tuples.
		 */
#ifdef DEBUG
		if ( _qdebug & QDBG_JOIN )
			prmsg( MSG_DEBUG, "(do_join): pruning common super-node of relations 0x%lx and 0x%lx",
				operptr->node1, operptr->node2 );
#endif

		if ( operptr->oper == UQOP_ANTIJOIN )
			tmpcnt = pruneantijoin( result->query, &jinfo, operptr );
		else
			tmpcnt = prunejoin( result->query, &jinfo, operptr );
	}
	else if ( operptr->cmpcnt != operptr->nonequal )
	{
		/*
		 * We have some equality comparison on which to do
		 * a hashed join.
		 */
#ifdef DEBUG
		if ( _qdebug & QDBG_JOIN )
			prmsg( MSG_DEBUG, "(do_join): doing single hash join on relations" );
#endif

		tmpcnt = hashjoin( result->query, &jinfo, operptr );
	}
	else
	{
		/*
		 * We can't do a hash join because there are only
		 * non-equality comparisons.
		 */
#ifdef DEBUG
		if ( _qdebug & QDBG_JOIN )
			prmsg( MSG_DEBUG, "(do_join): doing block join on relations" );
#endif

		tmpcnt = blkjoin( result->query, &jinfo, operptr );
	}

#ifdef DEBUG
	if ( _qdebug & QDBG_JOIN )
	{
		if ( tmpcnt >= 0L )
			prmsg( MSG_DEBUG, "(do_join): found %ld tuples in join on relations",
				tmpcnt );
		else
		{
			/* print the error message, but save the error code */
			set_uerror( pruerror() );
			prmsg( MSG_DEBUG, "(do_join): join failed on relations" );
		}
	}
#endif

	if ( jinfo.snode )
	{
		/*
		 * Replace the new super node as part of the
		 * result and free up the old super nodes.
		 */
		if ( operptr->node1->snode != NULL )
			freesupernode( &result->snode, operptr->node1->snode );
		if ( operptr->node2->snode != NULL &&
			operptr->node2->snode != operptr->node1->snode )
		{
			freesupernode( &result->snode, operptr->node2->snode );
		}

		if ( operptr->node1->snode != NULL ||
			operptr->node2->snode != NULL )
		{
			for( i = 0; i < result->query->nodecnt; i++ )
			{
				register struct qnode *nodeptr;
	
				nodeptr = result->query->nodelist[i];
				if ( nodeptr != operptr->node1 &&
					nodeptr != operptr->node2 &&
					nodeptr->snode != NULL &&
					( nodeptr->snode == operptr->node1->snode ||
					nodeptr->snode == operptr->node2->snode ) )
				{
					nodeptr->snode = jinfo.snode;
				}
			}
		}
		operptr->node1->snode = jinfo.snode;
		operptr->node2->snode = jinfo.snode;
	}

	/*
	 * Mark the join as done.
	 */
	if ( operptr->oper != UQOP_ANTIJOIN )
	{
		operptr->node1->flags |= N_JOINDONE;
		operptr->node2->flags |= N_JOINDONE;
	}

	return( tmpcnt );
}
