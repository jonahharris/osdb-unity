/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include "uquery.h"
#include "uerror.h"
#include "qdebug.h"
#include "message.h"

/*
 * This file takes a query expression tree and transforms it into a
 * query ready for evaluation.
 */

extern char *malloc();
extern struct queryexpr *newqexpr();
extern char *calloc(), *realloc();
extern void prtree(), prexpr();
extern struct qnode *exprnode();

static int
pad_fwattr( refptr, strptr )
struct attrref *refptr;
char **strptr;
{
	register struct uattribute *attrptr;
	char *newstr;

	/*
	 * Pad fixed width fields out to their appropriate
	 * length.
	 */
	if ( ATTR_SPECIAL( refptr->attr ) )
		return( TRUE );

	attrptr = &refptr->rel->rel->attrs[ refptr->attr ];

	if ( attrptr->attrtype != UAT_FIXEDWIDTH )
		return( TRUE );

	newstr = malloc( attrptr->terminate + 1 );
	if ( newstr == NULL )
	{
		set_uerror( UE_NOMEM );
		return( FALSE );
	}

	sprintf( newstr, "%-*.*s", attrptr->terminate, attrptr->terminate,
		*strptr );
	*strptr = newstr;

	return( TRUE );
}

static int
pad_fwexpr( qptr )
register struct queryexpr *qptr;
{
	while( ISBOOL( qptr->optype ) )
	{
		if ( ! pad_fwexpr( qptr->elem1.expr ) )
			return( FALSE );
		qptr = qptr->elem2.expr;
	}

	if ( qptr->cmptype != QCMP_STRING && qptr->cmptype != QCMP_CASELESS )
		return( TRUE );

	else if ( ISSETCMP( qptr->optype ) )
	{
		char **strlist;
		int i;

		for( i = 0, strlist = qptr->elem2.strlist;
			*strlist != NULL;
			i = (i + 1) % qptr->elem1.alist.cnt, strlist++ )
		{
			if ( ! pad_fwattr( &qptr->elem1.alist.list[i],
					strlist ) )
				return( FALSE );
		}

		return( TRUE );
	}
	else if ( qptr->opflags & ISATTR2 )
		return( TRUE );

	else
		return( pad_fwattr( &qptr->elem1.attr, &qptr->elem2.strval ) );
}

static void
demorgan( qptr )
register struct queryexpr *qptr;
{
	/*
	 * Apply DeMorgan's laws to a query expression.  This
	 * also applies to anti-joins since an anti-join is equivalent
	 * to NOT in terms of boolean logic:
	 *
	 *	NOT ( A OR B ) => NOT A AND NOT B
	 *	NOT ( A AND B ) => NOT A OR NOT B
	 *	ANTIJOIN ( A OR B ) => ANTIJOIN A AND ANTIJOIN B
	 *	ANTIJOIN ( A AND B ) => ANTIJOIN A OR ANTIJOIN B
	 *
	 * We also apply DeMorgan's laws to else-clauses, but in a
	 * strange way:
	 *
	 *	NOT ( A ELSE B ) => NOT A AND NOT B
	 *
	 * Here's the strange part:  Unlike normal boolean operations,
	 * two NOT's does not get you back where you started:
	 *
	 *	NOT NOT ( A ELSE B ) => A OR B
	 *
	 * After this routine has been called for all nodes in an
	 * expression, the negations will apply only to compare operations.
	 */
	while( ISBOOL( qptr->optype ) ) {
		switch( qptr->optype ) {
		case OPNOT|OPELSE:
		case OPNOT|OPOR:
			qptr->optype = OPAND;
			NOT( qptr->elem1.expr->optype );
			NOT( qptr->elem2.expr->optype );
			break;
		case OPNOT|OPAND:
			qptr->optype = OPOR;
			NOT( qptr->elem1.expr->optype );
			NOT( qptr->elem2.expr->optype );
			break;
		case OPANTIJOIN|OPELSE:
		case OPANTIJOIN|OPOR:
			qptr->optype = OPAND;
			qptr->elem1.expr->optype ^= OPANTIJOIN;
			qptr->elem2.expr->optype ^= OPANTIJOIN;
			break;
		case OPANTIJOIN|OPAND:
			qptr->optype = OPOR;
			qptr->elem1.expr->optype ^= OPANTIJOIN;
			qptr->elem2.expr->optype ^= OPANTIJOIN;
			break;
		}

		demorgan( qptr->elem1.expr );
		qptr = qptr->elem2.expr;
	}
}

static struct queryexpr *
cpexpr( qptr )
register struct queryexpr *qptr;
{
	register struct queryexpr *newexpr;

	if ( ISBOOL( qptr->optype ) ) {
		if ( (newexpr = newqexpr( )) == NULL )
			return( NULL );

		newexpr->optype = qptr->optype;
		newexpr->opflags = qptr->opflags;
		newexpr->tuples = NULL;
		if ( (newexpr->elem1.expr = cpexpr( qptr->elem1.expr )) == NULL )
			return( NULL );
		if ( (newexpr->elem2.expr = cpexpr( qptr->elem2.expr )) == NULL )
			return( NULL );

		return( newexpr );
	}

	/*
	 * The expression is a compare; just return the expression
	 * itself.  This saves on memory and also allows us to
	 * evaluate each comparison only once when matching tuples,
	 * no matter how many edges it gets distributed into.
	 */
	return( qptr );
}

static int
distrlaw( qptr, stopptr, top_op, lower_op )
register struct queryexpr *qptr;
struct queryexpr *stopptr;
unsigned short top_op;
unsigned short lower_op;
{
	register struct queryexpr *expr1, *expr2, *newexpr1;
	register unsigned short elem1op, elem2op;

	/*
	 * This routine assumes that demorgan() has been called
	 * for all nodes in the expression.  This assures that all
	 * negations only apply to compare operations.
	 *
	 * Apply the distributive law to expression to get
	 * either conjunctive-normal form (CNF) (top_op = OPAND
	 * and lower_op = OPOR) or disjunctive-normal form (DNF)
	 * (top_op = OPOR and lower_op = OPAND):
	 *
	 *	CNF works like this:
	 *	c or ( a and b ) => ( c or a ) and ( c or b )
	 *	( a and b ) or c => ( a or c ) and ( b or c )
	 *
	 *	DNF works like this:
	 *	c and ( a or b ) => ( c and a ) or ( c and b )
	 *	( a or b ) and c => ( a and c ) or ( b and c )
	 *
	 * Else-clauses are NOT distributive, so we don't deal
	 * with them here.  (There is a form of distribution that
	 * applies to else-clauses, but I have not formalized or
	 * implemented it, yet. - Eric)
	 */
	if ( qptr == stopptr || ISCOMPARE( qptr->optype ) )
		return( TRUE );

	if ( ! distrlaw( qptr->elem1.expr, stopptr, top_op, lower_op ) )
		return( FALSE );
	if ( ! distrlaw( qptr->elem2.expr, stopptr, top_op, lower_op ) )
		return( FALSE );

	if ( qptr->optype == lower_op ) {
		expr1 = qptr->elem1.expr;
		expr2 = qptr->elem2.expr;
		elem1op = expr1->optype;
		elem2op = expr2->optype;
		if ( elem1op == top_op || elem2op == top_op ) {
			if ( (newexpr1 = newqexpr( )) == NULL )
				return( FALSE );

			newexpr1->tuples = NULL;

			qptr->optype = top_op;
			qptr->opflags |= SUBEXPR;
			if ( elem1op == top_op ) {
				/*
				 * We have the case (CNF):
				 *   ( a and b ) or c => ( a or c ) and ( b or c )
				 * or (DNF):
				 *   ( a or b ) and c => ( a and c ) or ( b and c )
				 *
				 * When we have a choice, we ARBITRARILY do it
				 * this way.
				 */
				expr1->optype = lower_op;
				expr1->opflags = (expr1->opflags & ~(HASJOIN|HASSELECT)) | SUBEXPR;
				newexpr1->optype = lower_op;
				newexpr1->opflags = expr1->opflags | (expr1->elem2.expr->opflags | expr2->opflags) & (HASJOIN|HASSELECT);
				expr1->opflags |= (expr1->elem1.expr->opflags | expr2->opflags) & (HASJOIN|HASSELECT);

				newexpr1->elem1.expr = expr1->elem2.expr;
				if ( (newexpr1->elem2.expr = cpexpr( expr2 )) == NULL )
					return( FALSE );
				expr1->elem2.expr = expr2;
				qptr->elem2.expr = newexpr1;
			}
			else {
				/*
				 * We have the case (CNF):
				 *   c or ( a and b ) => ( c or a ) and ( c or b )
				 * or (DNF):
				 *   c and ( a or b ) => ( c and a ) or ( c and b )
				 */
				expr2->optype = lower_op;
				expr2->opflags = expr2->opflags | SUBEXPR;
				newexpr1->optype = lower_op;
				newexpr1->opflags = expr2->opflags | (expr2->elem1.expr->opflags | expr1->opflags) & (HASJOIN|HASSELECT);
				expr2->opflags |= (expr2->elem1.expr->opflags | expr1->opflags) & (HASJOIN|HASSELECT);
				if ( (newexpr1->elem1.expr = cpexpr( expr1 )) == NULL )
					return( FALSE );

				newexpr1->elem2.expr = expr2->elem1.expr;
				expr2->elem1.expr = expr1;
				qptr->elem1.expr = newexpr1;
			}

			/*
			 * Since we've created two new expressions, we
			 * need to walk the subtree again and re-apply
			 * the distributive law.
			 */
			if ( ! distrlaw( qptr->elem1.expr, stopptr,
					top_op, lower_op ) )
				return( FALSE );
			if ( ! distrlaw( qptr->elem2.expr, stopptr,
					top_op, lower_op ) )
				return( FALSE );
		}
	}

	return( TRUE );
}

static void
switchexpr( qptr )
register struct queryexpr *qptr;
{
	union qoperand tmpop;

	tmpop = qptr->elem1;
	qptr->elem1 = qptr->elem2;
	qptr->elem2 = tmpop;
}

static struct queryexpr *
growright( parent, qptr )
struct queryexpr **parent;
register struct queryexpr *qptr;
{
	register struct queryexpr *tmpexpr;

	/*
	 * Transform the tree so that it's right-descending.
	 * We're gauranteed not to screw up the expression
	 * because we're working within ONE conjunctive clause.
	 *
	 * Pictorially, the transformation is:
	 *
	 *    *parent          *parent
	 *        \                \
	 *         A                B
	 *        / \     ==>      / \
	 *       B   C            D   A
	 *      / \                  / \
	 *     D   E                E   C
	 *
	 * We also have to adjust the HASELSE flags:  if D contains
	 * an else, but E and C do not, then A should have its HASELSE
	 * flag removed.  Alternately, if C contains an else, but
	 * D and E do not, B should have its HASELSE flag turned on.
	 */
#ifdef DEBUG
	if ( _qdebug & QDBG_EXPR ) {
		prmsg( MSG_DEBUG, "(growright): OLD tree:" );
		prtree( stderr, *parent, FALSE );
	}
#endif

	tmpexpr = qptr->elem1.expr;
	*parent = tmpexpr;			/* *parent linked to B */
	qptr->elem1.expr = tmpexpr->elem2.expr;	/* A linked to E */
	tmpexpr->elem2.expr = qptr;		/* B linked to A */

	if ( (qptr->elem1.expr->opflags & HASELSE) == 0 ) {
		if ( tmpexpr->elem1.expr->opflags & HASELSE ) {
			/*
			 * E doesn't, but D does; check C and
			 * flip the flags.
			 */
			if ( (qptr->elem2.expr->opflags & HASELSE) == 0 )
				qptr->opflags &= ~HASELSE;
		}
		/*
		 * E and D don't; check C and
		 * flip the flags.
		 */
		else if ( qptr->elem2.expr->opflags & HASELSE )
			tmpexpr->opflags |= HASELSE;
	}

#ifdef DEBUG
	if ( _qdebug & QDBG_EXPR ) {
		prmsg( MSG_DEBUG, "(growright): NEW tree:" );
		prtree( stderr, *parent, FALSE );
	}
#endif

	/*
	 * Now return the adjusted expression so that we can do
	 * everything again -- just in case there is need for another
	 * transformation.
	 */
	return( tmpexpr );
}

static void
rightdescend( parent, qptr )
struct queryexpr **parent;
register struct queryexpr *qptr;
{
	register unsigned short elem1op, elem2op;
	register unsigned short op1flags, op2flags;

	while( qptr->optype == OPOR ) {

#ifdef DEBUG
		if ( _qdebug & QDBG_EXPR )
			prmsg( MSG_DEBUG, "(rightdescend): growing right on OR's" );
#endif

		elem1op = qptr->elem1.expr->optype;
		elem2op = qptr->elem2.expr->optype;
		op1flags = qptr->elem1.expr->opflags;
		op2flags = qptr->elem2.expr->opflags;
		if ( ISCOMPARE( elem1op ) )
			break;

		if ( ISCOMPARE( elem2op ) ) {
			/*
			 * The left clause is an OR or ELSE, but the
			 * right clause is a comparison, so just
			 * switch the expressions and return.
			 */
#ifdef DEBUG
			if ( _qdebug & QDBG_EXPR )
				prmsg( MSG_DEBUG, "(rightdescend): switching exprs because op 1 is bool and op 2 is a compare" );
#endif

			switchexpr( qptr );
			break;
		}
	
		if ( elem1op == OPELSE ) {
			/*
			 * The left op is an ELSE; if the right op does
			 * not contain an ELSE switch the expressions to
			 * get all ELSEs on the right.  Otherwise, the
			 * tree is already right-descending and we break.
			 */
			if ( (op2flags & HASELSE) == 0 ) {
#ifdef DEBUG
				if ( _qdebug & QDBG_EXPR )
					prmsg( MSG_DEBUG, "(rightdescend): switching exprs because op 1 is an ELSE and op 2 does not have ELSE" );
#endif
				switchexpr( qptr );
			}
			else
				break;
		}
		else if ( (op1flags & HASELSE) && (op2flags & HASELSE) == 0 ) {
			/*
			 * Both sub-trees are OR's.  Furthermore,
			 * The right clause does not contain an else, but
			 * the left side does, so switch the elements
			 * to get all else's toward the right.
			 */
#ifdef DEBUG
			if ( _qdebug & QDBG_EXPR )
				prmsg( MSG_DEBUG, "(rightdescend): switching exprs because both op 1 and 2 are ORs, but only op 1 has ELSE" );
#endif

			switchexpr( qptr );
		}

		/*
		 * The left sub-tree is still an OR.  Therefore,
		 * transform the tree so that it's right-descending.
		 * We're gauranteed not to screw up the expression
		 * because we're working within ONE conjunctive clause.
		 * Also reset qptr so that we can do everything again --
		 * just in case there is still another OR in the left
		 * sub-tree.
		 */
		qptr = growright( parent, qptr );
	}

	/*
	 * If the expression was NOT an OR, then check to see if it was
	 * an ELSE.  These also are changed into a right-descending tree.
	 * However, no switching is done because an ELSE is NOT commutative.
	 */
	while( qptr->optype == OPELSE && qptr->elem1.expr->optype == OPELSE ) {
#ifdef DEBUG
		if ( _qdebug & QDBG_EXPR )
			prmsg( MSG_DEBUG, "(righdescend): growing right on ELSE's" );
#endif
		qptr = growright( parent, qptr );
	}
}

static void
sorttree( qptr, level, stopptr )
register struct queryexpr *qptr;
int level;
struct queryexpr *stopptr;
{
	register int op;

	/*
	 * This routine assumes that demorgan() and distrlaw() have
	 * been called for all nodes in the expression.  This assures
	 * that all negations and anti-joins only apply to compare
	 * operations and that the query is in conjunctive-normal
	 * form (CNF) except within else-clauses.
	 *
	 * This routine transforms the expression into a right-descending
	 * tree and orders the OR's before the ELSE's.  It also makes sure
	 * that expressions without ELSE's come before those that have them.
	 */
	while( ISBOOL( qptr->optype ) ) {
		if ( qptr == stopptr )
			return;

		op = qptr->optype;
		if ( op == OPAND ) {
			/*
			 * If the right tree doesn't have an ELSE, but the
			 * left tree does, then switch the expressions.
			 * This simple check makes sure that all edges without
			 * ELSE's will be evaluated before those with ELSE's.
			 * This will prune out as many traversals of the
			 * ELSE-handling code as possible.  (Handling ELSE's
			 * is more expensive than the normal operations.)
			 */
			if ( (qptr->elem2.expr->opflags & HASELSE) == 0 &&
					(qptr->elem1.expr->opflags & HASELSE) ) {
#ifdef DEBUG
				if ( _qdebug & QDBG_EXPR )
					prmsg( MSG_DEBUG, "(sorttree level %d) switching exprs because op 1 has ELSE and op 2 does not",
						level );
#endif
				switchexpr( qptr );
			}
		}
		else {
			rightdescend( &qptr->elem1.expr, qptr->elem1.expr );
			rightdescend( &qptr->elem2.expr, qptr->elem2.expr );

			/*
			 * If the root of the tree is an OR and ( ( the left
			 * tree is boolean and the right is a compare ) or
			 * ( the left tree is ELSE and the right is an OR ) or
			 * the left tree is OR and has ELSE and right is ELSE ) )
			 * then switch the expressions.  This check cannot be made
			 * by rightdescend() because it never sees the root
			 * node.
			 */
			if ( op == OPOR ) {
				if ( ISBOOL( qptr->elem1.expr->optype ) &&
						ISCOMPARE( qptr->elem2.expr->optype ) ) {
#ifdef DEBUG
					if ( _qdebug & QDBG_EXPR )
						prmsg( MSG_DEBUG, "(sorttree, level %d): switching exprs because op 1 is bool and op 2 is compare",
							level );
#endif
					switchexpr( qptr );
				}
				else if ( qptr->elem1.expr->optype == OPELSE &&
						qptr->elem2.expr->optype == OPOR &&
						(qptr->elem2.expr->opflags & HASELSE) == 0 ) {

#ifdef DEBUG
					if ( _qdebug & QDBG_EXPR )
						prmsg( MSG_DEBUG, "(sorttree level %d): switching exprs because op 1 is ELSE and op 2 is OR and has no ELSE",
							level );
#endif
					switchexpr( qptr );
				}
				else if ( qptr->elem2.expr->optype == OPELSE &&
						qptr->elem1.expr->optype == OPOR &&
						(qptr->elem1.expr->opflags & HASELSE) != 0 ) {

#ifdef DEBUG
					if ( _qdebug & QDBG_EXPR )
						prmsg( MSG_DEBUG, "(sorttree level %d): switching exprs because op 2 is ELSE and op 1 is OR and has ELSE",
							level );
#endif
					switchexpr( qptr );
				}
#ifdef DEBUG
				else if ( _qdebug & QDBG_EXPR )
					prmsg( MSG_DEBUG, "(sorttree level %d): leaving nodes as they are for OR",
						level );
#endif
			}
#ifdef DEBUG
			else if ( _qdebug & QDBG_EXPR )
				prmsg( MSG_DEBUG, "(sorttree level %d): leaving nodes as they are because op is ELSE",
						level );
#endif
		}

		sorttree( qptr->elem1.expr, ++level, stopptr );
		qptr = qptr->elem2.expr;
	} /* end while ISBOOL( qptr->optype ) */
}


/*
 * Below are the routines that build the operations for a query.
 */
static void
mark_attrs( operptr, qptr )
struct uqoperation *operptr;
register struct queryexpr *qptr;
{
	register struct qnode *nodeptr;
	register struct attrref *refptr;
	register int i;

	/*
	 * Save the attribute information for the expression.
	 */
	while( ISBOOL( qptr->optype ) ) {
		mark_attrs( operptr, qptr->elem1.expr );

		qptr = qptr->elem2.expr;
	}

	if ( ISSETCMP( qptr->optype ) ) {
		nodeptr = qptr->elem1.alist.list[0].rel;
		refptr = qptr->elem1.alist.list;

		for( i = 0; i < qptr->elem1.alist.cnt; i++, refptr++ ) {
			if ( ! ATTR_SPECIAL( refptr->attr ) )
				nodeptr->memattr[refptr->attr] |= MA_SELECT;
			else if ( refptr->attr == ATTR_RECNUM )
			{
				operptr->flags |= UQF_RECNUM;
				nodeptr->flags |= N_PROJRECNUM;
			}
		}
	}
	else {
		nodeptr = qptr->elem1.attr.rel;
		if ( ! ATTR_SPECIAL( qptr->elem1.attr.attr ) )
			nodeptr->memattr[qptr->elem1.attr.attr] |= MA_SELECT;
		else if ( qptr->elem1.attr.attr == ATTR_RECNUM )
		{
			operptr->flags |= UQF_RECNUM;
			nodeptr->flags |= N_PROJRECNUM;
		}

		if ( qptr->opflags & ISATTR2 ) {
			nodeptr = qptr->elem2.attr.rel;
			if ( ! ATTR_SPECIAL( qptr->elem2.attr.attr ) )
				nodeptr->memattr[qptr->elem2.attr.attr] |= MA_SELECT;
			else if ( qptr->elem2.attr.attr == ATTR_RECNUM )
			{
				operptr->flags |= UQF_RECNUM;
				nodeptr->flags |= N_PROJRECNUM;
			}
		}
	}
}

static struct queryexpr *
split_expr( qptr, level )
struct queryexpr *qptr;
int level;
{
	struct queryexpr *split1, *split2;

	if ( ISCOMPARE( qptr->optype ) )
		return( NULL );		/* no place to split */

	switch( qptr->optype ) {
	case OPELSE:
		return( NULL );		/* no place to split */
	case OPOR:
		if ( (qptr->elem1.expr->opflags & HASJOIN) ||
			(qptr->elem2.expr->opflags & HASJOIN) )
		{
			return( qptr );
		}
		else
		{
			struct qnode *nodeptr;

			/*
			 * If the node(s) of each sub-expression are not
			 * the same, then there is an implicit join
			 * in this expression.  So our split point is here.
			 */
			nodeptr = exprnode( qptr->elem1.expr );
			if ( nodeptr == NULL ||
				nodeptr != exprnode( qptr->elem2.expr ) )
			{
				return( qptr );
			}

			/* all one node -- no split point */
			return( NULL );
		}
		
	case OPAND:
		split1 = split_expr( qptr->elem1.expr, level + 1 );
		split2 = split_expr( qptr->elem2.expr, level + 1 );
		if ( split1 != NULL && split2 != NULL )
			return( qptr );
		else if ( split1 != NULL && split2 == NULL ) {
			split2 = qptr->elem1.expr;
			qptr->elem1.expr = qptr->elem2.expr;
			qptr->elem2.expr = split2;

			return( split1 );
		}
		else
			return( split2 );
	}

	/* something is wrong */
	set_uerror( UE_DECOMPOSE );

	return( NULL );
}

#define CMPINITSIZE	8	/* initial # of compares in an operation */
#define CMPMORESIZE	4	/* additional # of compares added to oper */

static struct uqoperation *
add_oper( operlist, qptr, node1, node2, optype, ojdirect )
struct queryexpr *qptr;
struct uqoperation **operlist;
struct qnode *node1;
struct qnode *node2;
UQOPERATION optype;
char ojdirect;
{
	struct uqoperation *operptr;
	int i;

	for( operptr = *operlist; operptr != NULL; operptr = operptr->next )
	{
		if ( operptr->oper == optype &&
			(( operptr->node1 == node1 &&
				operptr->node2 == node2 ) ||
			( operptr->node1 == node2 &&
				operptr->node2 == node1 )) )
		{
			if ( ! ojdirect )
				break;

			/*
			 * For directional outer-joins, not only
			 * must the operations and nodes be the
			 * same, but they must be specified in the
			 * same order.
			 */
			if ( (operptr->flags & UQF_OJDIRECT) &&
				qptr->elem1.attr.rel == operptr->cmplist[0]->elem1.attr.rel )
			{
				/* same nodes in the same order */
				break;
			}
		}
	}

	if ( operptr == NULL )
	{
		/*
		 * No other operation done on these nodes in this list.
		 * Allocate a new operation.
		 */
		operptr = (struct uqoperation *)calloc( sizeof( struct uqoperation ), 1 );
		if ( operptr == NULL )
		{
			set_uerror( UE_NOMEM );
			return( NULL );
		}
		operptr->oper = optype;
		if ( ojdirect )
			operptr->flags |= UQF_OJDIRECT;
		operptr->node1 = node1;
		operptr->node2 = node2;

		operptr->maxcmp = CMPINITSIZE;
		operptr->cmplist = (struct queryexpr **)calloc( sizeof( struct queryexpr * ),
								CMPINITSIZE );
		if ( operptr->cmplist == NULL ) {
			set_uerror( UE_NOMEM );
			free( operptr );
			return( NULL );
		}
		operptr->next = *operlist;
		*operlist = operptr;
	}
	else if ( operptr->cmpcnt >= operptr->maxcmp )
	{
		operptr->maxcmp += CMPMORESIZE;
		operptr->cmplist = (struct queryexpr **)realloc( operptr->cmplist,
					sizeof( struct queryexpr * ) *
						operptr->maxcmp );
		if ( operptr->cmplist == NULL )
		{
			set_uerror( UE_NOMEM );
			return( NULL );
		}
	}

	mark_attrs( operptr, qptr );

	for ( i = 0; i < operptr->cmpcnt; i++ )
	{
		if ( operptr->cmplist[ i ] == qptr )	/* compare there */
			return( operptr );
	}

	operptr->cmplist[ operptr->cmpcnt++ ] = qptr;

	return( operptr );
}

static int
seloper( exprptr, qptr, operlist )
struct queryexpr *exprptr;
struct queryexpr *qptr;
struct uqoperation **operlist;
{
	/*
	 * There may be several nodes in a one selection-only edge.
	 * Add each one individually.
	 */
	while( ISBOOL( qptr->optype ) ) {
		if ( ! seloper( exprptr, qptr->elem1.expr, operlist ) )
			return( FALSE );
		qptr = qptr->elem1.expr;
	}

	if ( add_oper( operlist, exprptr,
			ISSETCMP( qptr->optype ) ?
				qptr->elem1.alist.list[0].rel :
				qptr->elem1.attr.rel,
			(struct qnode *)NULL, UQOP_SELECT, FALSE ) == NULL )
		return( FALSE );
	else
		return( TRUE );
}

static int
joinoper( qptr, operlist )
struct queryexpr *qptr;
struct uqoperation **operlist;
{
	struct uqoperation *operptr;

	if ( ISBOOL( qptr->optype ) ) {
		set_uerror( UE_DECOMPOSE );
		return( FALSE );
	}

	operptr = add_oper( operlist, qptr, qptr->elem1.attr.rel,
			qptr->elem2.attr.rel,
			(qptr->optype & OPANTIJOIN) ? UQOP_ANTIJOIN :
				(qptr->opflags & OPOUTERJOIN) ?
					UQOP_OUTERJOIN :
				UQOP_JOIN,
			(qptr->opflags & OPOJDIRECT) ? TRUE : FALSE );
	if ( operptr == NULL )
		return( FALSE );

	if ( (qptr->optype & ~OPANTIJOIN) != OPEQ )
		operptr->nonequal++;

	return( TRUE );
}

static struct uqoperation **
lastoper( prevptr )
struct uqoperation **prevptr;
{
	if ( *prevptr == NULL )
		return( prevptr );

	while( prevptr[0]->next != NULL )
		prevptr = &prevptr[0]->next;

	return( prevptr );
}

static int
bldoper( qptr, stopptr, operptr, or_opers )
struct queryexpr *qptr;
struct queryexpr *stopptr;
struct uqoperation **operptr;
struct uqoperation **or_opers;
{
	struct uqoperation **tmpop;
	struct uqoperation *lastop;
	struct uqoperation *nextop;

	while( ISBOOL( qptr->optype ) )
	{
		if ( qptr == stopptr )
			return( TRUE );

		switch( qptr->optype ) {
		case OPAND:
			if ( ! bldoper( qptr->elem1.expr, stopptr,
					operptr, or_opers ) )
				return( FALSE );
			qptr = qptr->elem2.expr;
			continue;
		case OPOR:	/* should only happen for select */
			if ( qptr->opflags & HASJOIN ) {
				set_uerror( UE_DECOMPOSE );
				return( FALSE );
			}
			/* We want to fall through here */
		case OPELSE:	/* selection operation */
			if ( ! seloper( qptr, qptr, operptr ) )
				return( FALSE );

			return( TRUE );
		}
	}

	if ( qptr->opflags & HASSELECT )
	{
		/* convert any anti-join operations to simple negation */
		if ( qptr->optype & OPANTIJOIN )
		{
			qptr->optype &= ~OPANTIJOIN;
			NOT( qptr->optype );
		}

		return( seloper( qptr, qptr, operptr ) );
	}

	/*
	 * We've got a join operation.  If this an anti-join or there
	 * were no OR operations, then we can add this operation into
	 * the normal list.  Otherwise we need to add this join into
	 * each set of OR-ed operations (since we can't easily save the
	 * result of join operations during query evaluation).  We can
	 * do anti-joins because they only add/delete tuples from individual
	 * nodes, not super-nodes.
	 */
	if ( (qptr->optype & OPANTIJOIN) ||
			or_opers == NULL || *or_opers == NULL )
		return( joinoper( qptr, operptr ) );

	tmpop = or_opers;
	while( *tmpop )
	{
		unsigned short opflags;

		opflags = (*tmpop)->flags & UQF_RELRESET;
		(*tmpop)->flags &= ~UQF_RELRESET;

		for( lastop = *tmpop; (lastop->flags & UQF_PROJECT) == 0;
				lastop = lastop->next )
			;

		nextop = lastop->next;
		lastop->next = NULL;

		if ( ! joinoper( qptr, tmpop ) )
			return( FALSE );

		lastop->next = nextop;
		(*tmpop)->flags |= opflags;

		tmpop = &lastop->next;
	}

	return( TRUE );
}

static int
bld_or_opers( qptr, query )
struct queryexpr *qptr;
struct uquery *query;
{
	struct uqoperation *newoper;
	struct uqoperation **operptr;

	if ( qptr == NULL )
		return( TRUE );

	while( qptr->optype == OPOR ) {
		if ( ! bld_or_opers( qptr->elem1.expr, query ) )
			return( FALSE );
		qptr = qptr->elem2.expr;
	}

	newoper = NULL;
	if ( ! bldoper( qptr, (struct queryexpr *)NULL, &newoper,
			(struct uqoperation **)NULL ) )
		return( FALSE );

	operptr = lastoper( &query->operlist );
	if ( *operptr )
		operptr[0]->next = newoper;
	else
		operptr[0] = newoper;
	operptr = lastoper( operptr );

	/* project the result after the last operation in new list */
	operptr[0]->flags |= UQF_PROJECT;

	/* reset the relations before the first operation in new list */
	newoper->flags |= UQF_RELRESET;

	return( TRUE );
}

static int
cmp_select( qptr1, qptr2 )
struct queryexpr **qptr1;
struct queryexpr **qptr2;
{
	/*
	 * We want all the selections with an ELSE to come before
	 * those that don't have it.
	 */
	if ( (qptr1[0]->opflags & HASELSE) == (qptr2[0]->opflags & HASELSE) )
		return( 0 );

	if ( (qptr1[0]->opflags & HASELSE) )
		return( -1 );

	return( 1 );
}

static int
cmp_join( qptr1, qptr2 )
struct queryexpr **qptr1;
struct queryexpr **qptr2;
{
	/*
	 * We need all the equi-join comparisons to come before the
	 * non-equi-joins.
	 */
	if ( qptr1[0]->optype == qptr2[0]->optype )
		return( 0 );

	if ( (qptr1[0]->optype & ~OPANTIJOIN) == OPEQ )
		return( -1 );

	return( 1 );
}

#ifdef DEBUG
pr_oper( operptr )
struct uqoperation *operptr;
{
	char *optype;
	char *node2, *connect;
	int i;
	char num2[20];
	char flags[100];

	switch( operptr->oper ) {
	case UQOP_SELECT:
		optype = "SELECT";
		break;
	case UQOP_JOIN:
		optype = "NORMAL-JOIN";
		break;
	case UQOP_ANTIJOIN:
		optype = "ANTI-JOIN";
		break;
	case UQOP_OUTERJOIN:
		optype = "OUTER-JOIN";
		break;
	default:
		optype = "**unknown**";
		break;
	}

	if ( operptr->node2 != NULL ) {
		node2 = operptr->node2->rel->path;
		sprintf( num2, " (#%u)", operptr->node2->nodenum + 1 );
		connect = " and ";

	}
	else {
		node2 = "";
		connect = "";
		num2[0] = '\0';
	}

	flags[0] = '\0';
	if ( operptr->flags & UQF_PROJECT )
		strcat( flags, "|PROJECT" );
	if ( operptr->flags & UQF_RELRESET )
		strcat( flags, "|RELRESET" );
	if ( operptr->flags & UQF_RECNUM )
		strcat( flags, "|RECNUM" );
	if ( operptr->flags & UQF_OJDIRECT )
		strcat( flags, "|OJDIRECT" );
	if ( flags[0] )
		strcat( flags, "|" );
	else
		strcpy( flags, "**NONE**" );

	prmsg( MSG_DEBUG, "%s operation on %s (#%u)%s%s%s\n\t(flags = %s, cmpcnt = %d, maxcmp = %d, nonequal = %d):",
		optype, operptr->node1->rel->path,
		operptr->node1->nodenum + 1, connect, node2, num2, flags,
		operptr->cmpcnt, operptr->maxcmp, operptr->nonequal );

	for( i = 0; i < operptr->cmpcnt; i++ ) {
		if ( i != 0 )
			fputs( "\t   and ", stderr );
		else
			fputs( "\t", stderr );

		operptr->cmplist[i]->opflags |= SUBEXPR;

		prexpr( stderr, operptr->cmplist[i], FALSE );
	}
}
#endif

static
bldoperations( query, qptr, dnfptr )
struct uquery *query;
struct queryexpr *qptr;
struct queryexpr *dnfptr;
{
	struct uqoperation *operptr;
	struct uqoperation **operlist;

	query->operlist = NULL;
	if ( ! bld_or_opers( dnfptr, query ) )
		return( FALSE );

	operptr = NULL;
	if ( ! bldoper( qptr, dnfptr, &operptr, &query->operlist ) )
		return( FALSE );

	operlist = lastoper( &operptr );
	if ( *operlist != NULL ) {
		operlist[0]->next = query->operlist;
		query->operlist = operptr;

		operlist = lastoper( operlist );
		if ( *operlist != NULL )
			operlist[0]->flags |= UQF_PROJECT;
	}

	/*
	 * Now sort the comparisons of each operation.  For selection
	 * we need all the edges containins ELSE to come first.  For
	 * joins we need the equality comparisons to come first.
	 */
#ifdef DEBUG
	/*
	 * DEBUG: Print the operations used to evaluate the
	 * query expression.
	 */
	if ( (_qdebug & QDBG_EXPR) ) {
		prmsg( MSG_DEBUG, "(bldquery): below are operations to evaluate query:\n" );
	}
#endif
	for( operptr = query->operlist; operptr; operptr = operptr->next )
	{
		if ( operptr->oper == UQOP_SELECT )
			qsort( operptr->cmplist, operptr->cmpcnt,
				sizeof( struct queryexpr * ), cmp_select );
		else if ( operptr->nonequal != 0 )
			qsort( operptr->cmplist, operptr->cmpcnt,
				sizeof( struct queryexpr * ), cmp_join );

#ifdef DEBUG
		if ( (_qdebug & QDBG_EXPR) ) {
			pr_oper( operptr );
		}
#endif
		
	}
#ifdef DEBUG
	if ( (_qdebug & QDBG_EXPR) ) {
		prmsg( MSG_DEBUG, "(bldquery): end of query operations list\n" );
	}
#endif

	return( TRUE );
}

struct uquery *
fbldquery( rellist, relcnt, projlist, projcnt, qptr, flags )
struct qnode *rellist;
int relcnt;
struct qprojection *projlist;
int projcnt;
struct queryexpr *qptr;
int flags;
{
	register struct qnode *nodeptr;
	register struct uquery *query;
	int i, j;
	struct qprojection *projptr;
	struct queryexpr *dnfptr;
	struct stat relstat;


	if ( relcnt > MAXRELATION ) {
		set_uerror( UE_NUMREL );
		return( NULL );
	}

#ifdef DEBUG
	/*
	 * DEBUG: Print the original form of the query expression.
	 */
	if ( (_qdebug & QDBG_EXPR) && qptr ) {
		prexpr( stderr, qptr, TRUE );
		prtree( stderr, qptr, FALSE );
	}
#endif

	/*
	 * Put the query into normalized form.  We divide the
	 * expression into two parts.  The first is only composed of
	 * of AND operations, and OR or ELSE operations that only
	 * involve selection.  This part of the expression is put
	 * into conjunctive normal form (a set of AND'ed OR-causes)
	 * and the expression is made right-descending with OR's
	 * before ELSE's within each conjunctive clause.
	 * The second part of the expression starts at the highest
	 * boolean operation where both sides involve OR operations
	 * with joins or anti-joins.  This part of the expression is
	 * put into disjunctive normal form (a set of OR'ed AND-clauses).
	 */
	if ( qptr )
	{
		/*
		 * Pad strings for fixed width attributes to their
		 * proper length.
		 */
		if ( ! pad_fwexpr( qptr ) )
			return( NULL );

		demorgan( qptr );

		dnfptr = split_expr( qptr, 0 );
		if ( ! distrlaw( qptr, dnfptr, OPAND, OPOR ) )
			return( NULL );
		sorttree( qptr, 1, dnfptr );

		if ( ! distrlaw( dnfptr, (struct queryexpr *)NULL,
				OPOR, OPAND ) )
			return( NULL );
	}

#ifdef DEBUG
	/*
	 * DEBUG: Print the normalized and right-descending form of the
	 *	query expression.
	 */
	if ( (_qdebug & QDBG_EXPR) && qptr ) {
		prexpr( stderr, qptr, TRUE );
		prtree( stderr, qptr, FALSE );
	}
#endif

	query = (struct uquery *)calloc( 1, sizeof( *query ) );
	if ( query == NULL ) {
		set_uerror( UE_NOMEM );
		return( NULL );
	}
	query->flags = flags & (Q_SORT|Q_UNIQUE|Q_FRIENDLY);
	query->sortcnt = 0;
	query->attrlist = projlist;
	query->attrcnt = projcnt;
	query->version = UQUERY_VERSION;

	/*
	 * Expand any "all" attributes, if specified.  This will also
	 * take care of which attributes are sorted, etc.
	 */
	if ( (flags & Q_NOEXPAND) == 0 )
	{
		if ( ! exprojlist( query, query->flags & Q_FRIENDLY ) )
		{
			free( query );
			return( NULL );
		}
	}
	else
	{
		register int i;
		register struct qprojection *newref, *refptr;

		newref = (struct qprojection *) calloc( query->attrcnt, sizeof( struct qprojection) );
		if ( newref == NULL )
		{
			set_uerror( UE_NOMEM );
			free( query );
			return( NULL );
		}

		for ( refptr = query->attrlist, i = 0;
			i < query->attrcnt;
			i++ )
		{
			newref[i] = refptr[i];
		}

		query->attrlist = newref;
	}

	/*
	 * Now build the connection graph for the query.  First allocate
	 * the node space.  Then traverse the query and create the
	 * operations to evaluate the query.
	 */
	query->nodecnt = relcnt;
	for( i = 0; i < relcnt; i++, nodeptr++ )
	{
		nodeptr = &rellist[i];
		query->nodelist[i] = nodeptr;

		nodeptr->nodenum = i;
		nodeptr->flags = 0;
		nodeptr->tuplecnt = 0;
		nodeptr->tuples = NULL;
		nodeptr->nextblk = NULL;
		nodeptr->nexttpl = 0;
		for( j = 0; j < MAXATT; j++ )
			nodeptr->memattr[j] = '\0';
	}

	/*
	 * In this next loop we mark the attributes that will need to be
	 * saved as we read in tuples.
	 */
	projptr = projlist;
	for( i = 0; i < projcnt; i++, projptr++ ) {
		nodeptr = projptr->rel;

		nodeptr->flags |= N_PROJECT;
		if ( projptr->attr == ATTR_ALL ) {
			for( j = 0; j < nodeptr->rel->attrcnt; j++ )
				nodeptr->memattr[j] |= MA_PROJECT;
		}
		else if ( projptr->attr == ATTR_RECNUM )
			nodeptr->flags |= N_PROJRECNUM;
		else if ( projptr->attr != ATTR_SEEK )
			nodeptr->memattr[projptr->attr] |= MA_PROJECT;
	}

	if ( qptr ) {
		if ( ! bldoperations( query, qptr, dnfptr ) ) {
			free( query );
			return( NULL );
		}

		for( i = 0; i < query->nodecnt; i++ ) {
			/*
			 * NOTE: We expect all packed relations to fail
			 *	 on the stat(2) call below so that no
			 *	 attempt is made to use indexes.
			 */
			nodeptr = query->nodelist[i];
			if ( strcmp( nodeptr->rel->path, "-" ) == 0 ||
					stat( nodeptr->rel->path, &relstat ) < 0){
				nodeptr->relsize = 0x7fffffff;	/* very large */
				continue;
			}

			nodeptr->relsize = relstat.st_size;

			/*
			 * Check the type of device for the relation.
			 * If it's a fifo device, then indexes cannot be
			 * used on it, since it is not possible to seek.
			 */
			if ( (relstat.st_mode & S_IFMT) == S_IFIFO )
				continue;

			for( j = 0; j < nodeptr->rel->attrcnt; j++ ) {
				if ( (nodeptr->memattr[j] & MA_SELECT) &&
						chkindex( nodeptr->rel->path,
							&relstat,
							nodeptr->rel->attrs[j].aname ) )
					nodeptr->memattr[j] |= MA_INDEX;
			}
		}
	}

	if ( (flags & Q_NOEXPAND) == 0 )
		query->flags |= Q_INIT;

	return( query );
}

struct uquery *
bldquery( rellist, relcnt, projlist, projcnt, qptr )
struct qnode *rellist;
int relcnt;
struct qprojection *projlist;
int projcnt;
struct queryexpr *qptr;
{
	return( fbldquery( rellist, relcnt, projlist, projcnt, qptr, 0 ) );
}

struct uquery *
_bldquery( rellist, relcnt, projlist, projcnt, qptr, expand, flags )
struct qnode *rellist;
int relcnt;
struct qprojection *projlist;
int projcnt;
struct queryexpr *qptr;
int expand;
int flags;
{
	return( fbldquery( rellist, relcnt, projlist, projcnt, qptr,
		expand ? flags : flags | Q_NOEXPAND ) );
}
