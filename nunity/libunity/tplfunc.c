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

extern struct utuple **findjointpl(), **nullsupertpl();

tplfunc_sel( query, nodeptr, tpl )
register struct uquery *query;
struct qnode *nodeptr;
struct utuple *tpl;
{
	register struct qprojtuple *projptr;
	register struct qprojection *refptr;
	register int i, numcnt, cnt;
	struct qprojtuple projlist[MAXATT];
	char *attrvals[MAXATT];
	char recnums[MAXRELATION * 2][12];

	if ( nodeptr->flags & N_IGNORE )
	{
		if ( ! addtuple( query, nodeptr, tpl ) )
			return( FALSE );
	}

	if ( (nodeptr->flags & N_PROJECT) == 0 )
		return( TRUE );

	numcnt = 0;
	cnt = 0;
	projptr = projlist;
	for( refptr = query->attrlist, i = 0;
		i < query->attrcnt;
		i++, refptr++ )
	{

		if ( refptr->flags & QP_NODISPLAY )
			continue;

		projptr->tplptr = tpl;
		projptr->projptr = refptr;


		if ( ( refptr->flags & QP_NEWVALUE ) && ( refptr->attorval ) )
		{
			attrvals[cnt] = refptr->attorval;
		} else {
			switch( refptr->attr ) {
			case ATTR_RECNUM:
				attrvals[cnt] = recnums[numcnt++];
				sprintf( attrvals[cnt], "%ld", tpl->tuplenum );
				break;
			case ATTR_SEEK:
				attrvals[cnt] = recnums[numcnt++];
				sprintf( attrvals[cnt], "%ld", tpl->lseek );
				break;
			default:	/* normal attribute */
				attrvals[cnt] = tpl->tplval[refptr->attr];
				break;
			}
		}
		cnt++;
		projptr++;
	}

	i = (*query->tplfunc)( attrvals, projptr - projlist, projlist );

	if ( (nodeptr->flags & N_IGNORE) == 0 )
		(void)forgettuple( query, nodeptr, tpl );

	return( i );
}

static
no_snodes( query, node1, node2, tpl1, tpl2 )
struct uquery *query;
struct qnode *node1;
struct qnode *node2;
struct utuple *tpl1;
struct utuple *tpl2;
{
	register struct qprojtuple *projptr;
	register struct qprojection *refptr;
	register int i, cnt, numcnt;
	struct qprojtuple projlist[MAXATT];
	char *attrvals[MAXATT];
	char recnums[MAXRELATION * 2][12];

	/*
	 * Neither tuple is in a super node.  So just
	 * output the tuples themselves.
	 */
	numcnt = 0;
	cnt = 0;
	projptr = projlist;
	for( refptr = query->attrlist, i = 0;
		i < query->attrcnt;
		i++, refptr++ )
	{

		if ( refptr->flags & QP_NODISPLAY )
			continue;

		if ( refptr->rel == node1 )
			projptr->tplptr = tpl1;
		else if ( refptr->rel == node2 )
			projptr->tplptr = tpl2;
		else
		{
			set_uerror( UE_DECOMPOSE );
			return( -1 );
		}

		projptr->projptr = refptr;

		if ( ( refptr->flags & QP_NEWVALUE ) && ( refptr->attorval ) )
		{
			attrvals[cnt] = refptr->attorval;
		} else {
			switch( refptr->attr ) {
			case ATTR_RECNUM:
				attrvals[cnt] = recnums[numcnt++];
				sprintf( attrvals[cnt], "%ld", projptr->tplptr->tuplenum );
				break;
			case ATTR_SEEK:
				attrvals[cnt] = recnums[numcnt++];
				sprintf( attrvals[cnt], "%ld", projptr->tplptr->lseek );
				break;
			default:	/* normal attribute */
				attrvals[cnt] = projptr->tplptr->tplval[refptr->attr];
				break;
			}
		}
		cnt++;
		projptr++;
	}

	return( (*query->tplfunc)( attrvals, projptr - projlist, projlist ) );
}

static
one_snode( query, node1, node2, tpl1, tpl2 )
struct uquery *query;
struct qnode *node1;
struct qnode *node2;
struct utuple *tpl1;
struct utuple *tpl2;
{
	register struct qprojtuple *projptr;
	register struct qprojection *refptr;
	register int i, cnt, numcnt;
	struct joinblock *nextblk;
	int nexttpl;
	struct utuple **stpl;
	struct qprojtuple projlist[MAXATT];
	char *attrvals[MAXATT];
	char recnums[MAXRELATION * 2][12];

	/*
	 * Node1 is in a super node, but node2 is not.  For each occurance
	 * of tpl1 in node1's supernode, set up the tuples and call the
	 * tuple function.
	 */
	nextblk = node1->snode->joinptr;
	nexttpl = 0;
	stpl = findjointpl( query->nodecnt, &nextblk, &nexttpl,
			node1->nodenum, tpl1 );
	if ( stpl == NULL && (tpl1->flags & TPL_NULLTPL) != 0 )
	{
		stpl = nullsupertpl( query, node1, tpl1 );
		if ( stpl == NULL )
			return( FALSE );
	}
	while( stpl != NULL )
	{
		numcnt = 0;
		cnt = 0;
		projptr = projlist;
		for( refptr = query->attrlist, i = 0;
			i < query->attrcnt;
			i++, refptr++ )
		{

			if ( refptr->flags & QP_NODISPLAY )
				continue;

			projptr->tplptr = ( refptr->rel == node2 ) ? tpl2 :
						stpl[refptr->rel->nodenum];
			projptr->projptr = refptr;

			if ( ( refptr->flags & QP_NEWVALUE ) && ( refptr->attorval ) )
			{
				attrvals[cnt] = refptr->attorval;
			} else {
				switch( refptr->attr ) {
				case ATTR_RECNUM:
					attrvals[cnt] = recnums[numcnt++];
					sprintf( attrvals[cnt], "%ld", projptr->tplptr->tuplenum );
					break;
				case ATTR_SEEK:
					attrvals[cnt] = recnums[numcnt++];
					sprintf( attrvals[cnt], "%ld", projptr->tplptr->lseek );
					break;
				default:	/* normal attribute */
					attrvals[cnt] = projptr->tplptr->tplval[refptr->attr];
					break;
				}
			}
			cnt++;
			projptr++;
		}

		if ( ! (*query->tplfunc)( attrvals, projptr - projlist,
				projlist ) )
		{
			return( FALSE );
		}

		stpl = findjointpl( query->nodecnt, &nextblk, &nexttpl,
				node1->nodenum, tpl1 );
	}

	return( TRUE );
}

static
two_snodes( query, node1, node2, tpl1, tpl2 )
struct uquery *query;
struct qnode *node1;
struct qnode *node2;
struct utuple *tpl1;
struct utuple *tpl2;
{
	register struct qprojtuple *projptr;
	register struct qprojection *refptr;
	register struct utuple **stpl1, **stpl2;
	register int i, cnt, numcnt;
	struct joinblock *nextblk1, *nextblk2;
	int nexttpl1, nexttpl2;
	struct qprojtuple projlist[MAXATT];
	char *attrvals[MAXATT];
	char recnums[MAXRELATION * 2][12];

	/*
	 * Both nodes are in different supernodes so do a join of the
	 * two tuples and call the tuple function for each join
	 * combination.
	 */
	nextblk1 = node1->snode->joinptr;
	nexttpl1 = 0;
	while( (stpl1 = findjointpl( query->nodecnt, &nextblk1, &nexttpl1,
				node1->nodenum, tpl1 )) != NULL )
	{
		nextblk2 = node2->snode->joinptr;
		nexttpl2 = 0;
		while( (stpl2 = findjointpl( query->nodecnt,
					&nextblk2, &nexttpl2,
					node2->nodenum, tpl2 )) != NULL )
		{
			numcnt = 0;
			cnt = 0;
			projptr = projlist;
			for( refptr = query->attrlist, i = 0;
					i < query->attrcnt;
					i++, refptr++ )
			{
				if ( refptr->flags & QP_NODISPLAY )
					continue;

				projptr->tplptr = ( refptr->rel->snode == node1->snode ) ?
					stpl1[refptr->rel->nodenum] :
					stpl2[refptr->rel->nodenum];
				projptr->projptr = refptr;

				if ( ( refptr->flags & QP_NEWVALUE ) && ( refptr->attorval ) )
				{
					attrvals[cnt] = refptr->attorval;
				} else {
					switch( refptr->attr ) {
					case ATTR_RECNUM:
						attrvals[cnt] = recnums[numcnt++];
						sprintf( attrvals[cnt], "%ld", projptr->tplptr->tuplenum );
						break;
					case ATTR_SEEK:
						attrvals[cnt] = recnums[numcnt++];
						sprintf( attrvals[cnt], "%ld", projptr->tplptr->lseek );
						break;
					default:	/* normal attribute */
						attrvals[cnt] = projptr->tplptr->tplval[refptr->attr];
						break;
					}
				}
				cnt++;
				projptr++;
			}
			if ( ! (*query->tplfunc)( attrvals, projptr - projlist,
					projlist))
				return( FALSE );
		}
	}

	return( TRUE );
}

static
same_snodes( query, node1, node2, tpl1, tpl2 )
struct uquery *query;
struct qnode *node1;
struct qnode *node2;
struct utuple *tpl1;
struct utuple *tpl2;
{
	register struct qprojtuple *projptr;
	register struct qprojection *refptr;
	register struct utuple **stpl;
	register int i, cnt, numcnt;
	struct joinblock *nextblk;
	int nexttpl;
	struct qprojtuple projlist[MAXATT];
	char *attrvals[MAXATT];
	char recnums[MAXRELATION * 2][12];

	/*
	 * Both nodes are in the same supernode.  Find all supertuples
	 * containing both tuples.  Then call the tuple function for each one.
	 */
	nextblk = node1->snode->joinptr;
	nexttpl = 0;
	while( (stpl = findjointpl( query->nodecnt, &nextblk, &nexttpl,
				node1->nodenum, tpl1 )) != NULL )
	{
		if ( stpl[node2->nodenum] != tpl2 )
			continue;

		numcnt = 0;
		cnt = 0;
		projptr = projlist;
		for( refptr = query->attrlist, i = 0;
			i < query->attrcnt;
			i++, refptr++ )
		{
			if ( refptr->flags & QP_NODISPLAY )
				continue;

			projptr->tplptr = stpl[refptr->rel->nodenum];
			projptr->projptr = refptr;

			if ( ( refptr->flags & QP_NEWVALUE ) && ( refptr->attorval ) )
			{
				attrvals[cnt] = refptr->attorval;
			} else {
				switch( refptr->attr ) {
				case ATTR_RECNUM:
					attrvals[cnt] = recnums[numcnt++];
					sprintf( attrvals[cnt], "%ld", projptr->tplptr->tuplenum );
					break;
				case ATTR_SEEK:
					attrvals[cnt] = recnums[numcnt++];
					sprintf( attrvals[cnt], "%ld", projptr->tplptr->lseek );
					break;
				default:	/* normal attribute */
					attrvals[cnt] = projptr->tplptr->tplval[refptr->attr];
					break;
				}
			}
			cnt++;
			projptr++;
		}
		if ( ! (*query->tplfunc)( attrvals, projptr - projlist,
				projlist ) )
			return( FALSE );
	}

	return( TRUE );
}

/*ARGSUSED*/
tplfunc_join( query, snode, node1, node2, tpl1, tpl2 )
struct uquery *query;
struct supernode *snode;
struct qnode *node1;
struct qnode *node2;
struct utuple *tpl1;
struct utuple *tpl2;
{
	if ( node1->snode == NULL )
	{
		if ( node2->snode == NULL )
			return( no_snodes( query, node1, node2, tpl1, tpl2 ) );
		else
			return( one_snode( query, node2, node1, tpl2, tpl1 ) );
	}
	else if ( node2->snode == NULL )
		return( one_snode( query, node1, node2, tpl1, tpl2 ) );
	else if ( node2->snode == node1->snode )
		return( same_snodes( query, node1, node2, tpl1, tpl2 ) );

	return( two_snodes( query, node1, node2, tpl1, tpl2 ) );
}

tplfunc_prod( result )
struct qresult *result;
{
	register struct qprojtuple *projptr;
	int projcnt;
	register struct qprojection *refptr;
	register int i;
	register struct utuple **stplptr;
	struct uquery *query;
	struct qprojtuple projlist[MAXATT];
	char *attrvals[MAXATT];

	if ( ! initresult( result ) )
		return( TRUE );		/* no tuples is not an error */

	query = result->query;
	projcnt = 0;
	projptr = projlist;
	for( refptr = query->attrlist, i = 0;
		i < query->attrcnt; i++, refptr++ )
	{
		if ( (refptr->flags & QP_NODISPLAY) == 0 )
		{
			projptr->projptr = refptr;
			projptr++;
			projcnt++;
		}
	}

	/*
	 * Note: nexttuple() takes care of the QP_NEWVALUE case
	 */

	while( nexttuple( query, result, attrvals ) )
	{
		/*
		 * Set up the tuple pointers in the order they're given in the
		 * projection list.
		 */
		stplptr = &result->curblk->tuples[result->curblk->curtpl];

		projptr = projlist;
		for( refptr = query->attrlist, i = 0;
			i < query->attrcnt; i++, refptr++ )
		{
			if ( (refptr->flags & QP_NODISPLAY) == 0 )
			{
				projptr->tplptr = stplptr[ refptr->rel->nodenum ];
				projptr++;
			}
		}

		if ( ! (*query->tplfunc)( attrvals, projcnt, projlist ) )
			break;
	}

	return( uerror == UE_NOERR );
}

int
(*settplfunc( query, tplfunc ))()
struct uquery *query;
int (*tplfunc)();
{
	int (*oldfunc)();

	oldfunc = query->tplfunc;
	query->tplfunc = tplfunc;

	return( oldfunc );
}
