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

#ifdef	__STDC__
#include <stdlib.h>
#else
extern	long strtol();
#endif

extern double atof();
extern struct utuple **findjointpl();
extern struct utuple _unulltpl;

chkjcond( qptr, node1, node2, tpl1, tpl2 )
register struct queryexpr *qptr;
struct qnode *node1;
struct qnode *node2;
register struct utuple *tpl1, *tpl2;
{
	register long cmp;
	register double num1, num2;

	/*
	 * If this comparison is not a join, then something went
	 * wrong in the decomposition.
	 */
	if ( ISBOOL( qptr->optype ) || (qptr->opflags & HASJOIN) == 0 )
	{
		set_uerror( UE_DECOMPOSE );
		return( FALSE );
	}

	/*
	 * The NULL tuple always fails a match.
	 */
	if ( (tpl1->flags & TPL_NULLTPL) || (tpl2->flags & TPL_NULLTPL) )
	{
		return( FALSE );
	}

	/*
	 * We have an unfinished join comparison.
	 * Check that we've got the right nodes in the right order.
	 */
	if ( qptr->elem1.attr.rel == node2 &&
		qptr->elem2.attr.rel == node1 )
	{
		register struct qnode *tmpnode;
		register struct utuple *tmptpl;

		/*
		 * The nodes are reversed from the comparison.
		 * Reverse the nodes and tuples.
		 */
		tmpnode = node1;
		node1 = node2;
		node2 = tmpnode;
		tmptpl = tpl1;
		tpl1 = tpl2;
		tpl2 = tmptpl;
	}
	else if ( qptr->elem1.attr.rel != node1 ||
		qptr->elem2.attr.rel != node2 )
	{
		/*
		 * This join does not apply to these nodes.
		 * This is not supposed to happen.
		 */
		set_uerror( UE_DECOMPOSE );
		return( FALSE );
	}

	/*
	 * Do the real comparison.
	 */
	switch( qptr->cmptype ) {
	case QCMP_STRING:
		cmp = strcmp( tpl1->tplval[qptr->elem1.attr.attr],
				tpl2->tplval[qptr->elem2.attr.attr] );
		break;
	case QCMP_CASELESS:
		cmp = nocasecmp( tpl1->tplval[qptr->elem1.attr.attr],
				tpl2->tplval[qptr->elem2.attr.attr] );
		break;
	case QCMP_NUMBER:
		switch( qptr->elem1.attr.attr ) {
		case ATTR_RECNUM:
			num1 = tpl1->tuplenum;
			break;
		case ATTR_SEEK:
			num1 = tpl1->lseek;
			break;
		default:	/* normal attribute */
			if ( qptr->modifier1 ) {
				if ( ( qptr->modifier1 >= 2 ) && ( qptr->modifier1 <= 36 ) ) {
#ifdef	__STDC__
					/*
					 * Use unsigned conversion if available to
					 * avoid problem with maximum unsigned value
					 * since most (all) non-base 10 values are
					 * initially generated from an unsigned int.
					 */
					num1 = (int) strtoul( tpl1->tplval[qptr->elem1.attr.attr],
								(char **)NULL, qptr->modifier1 );
#else
					num1 = (int) strtol( tpl1->tplval[qptr->elem1.attr.attr],
								(char **)NULL, qptr->modifier1 );
#endif
				} else if ( qptr->modifier1 == 1 ) {
					num1 = strlen( tpl1->tplval[qptr->elem1.attr.attr] );
				} else {
					return( FALSE );
				}
			} else {
				num1 = atof( tpl1->tplval[qptr->elem1.attr.attr] );
			}
			break;
		}
		switch( qptr->elem2.attr.attr ) {
		case ATTR_RECNUM:
			num2 = tpl2->tuplenum;
			break;
		case ATTR_SEEK:
			num2 = tpl2->lseek;
			break;
		default:	/* normal attribute */
			if ( qptr->modifier2 ) {
				if ( ( qptr->modifier2 >= 2 ) && ( qptr->modifier2 <= 36 ) ) {
#ifdef	__STDC__
					/*
					 * Use unsigned conversion if available to
					 * avoid problem with maximum unsigned value
					 * since most (all) non-base 10 values are
					 * initially generated from an unsigned int.
					 */
					num2 = (int) strtoul( tpl2->tplval[qptr->elem2.attr.attr],
								(char **)NULL, qptr->modifier2 );
#else
					num2 = (int) strtol( tpl2->tplval[qptr->elem2.attr.attr],
								(char **)NULL, qptr->modifier2 );
#endif
				} else if ( qptr->modifier2 == 1 ) {
					num2 = strlen( tpl2->tplval[qptr->elem2.attr.attr] );
				} else {
					return( FALSE );
				}
			} else {
				num2 = atof( tpl2->tplval[qptr->elem2.attr.attr] );
			}
			break;
		}
		cmp = ( ( num1 < num2 ) ? -1 : ( num1 != num2 ) );
		break;
	case QCMP_DATE:
	{
		int tmp_cmp;

		if ( ! chkdate( tpl1->tplval[qptr->elem1.attr.attr],
				tpl2->tplval[qptr->elem2.attr.attr], &tmp_cmp ) )
			return( FALSE );
		cmp = tmp_cmp;
		break;
	}
	case QCMP_DATEONLY:
	{
		int tmp_cmp;

		if ( ! chkDate( tpl1->tplval[qptr->elem1.attr.attr],
				tpl2->tplval[qptr->elem2.attr.attr], &tmp_cmp ) )
			return( FALSE );
		cmp = tmp_cmp;
		break;
	}
	default:
		return( FALSE );
	}

	switch( qptr->optype & ~OPANTIJOIN ) {
	case OPEQ:
		return( cmp == 0 );
	case OPNE:
		return( cmp != 0 );
	case OPGT:
		return( cmp > 0 );
	case OPGE:
		return( cmp >= 0 );
	case OPLT:
		return( cmp < 0 );
	case OPLE:
		return( cmp <= 0 );
	}

	return( FALSE );
}

chkjoinlist( condlist, condcnt, node1, node2, tpl1, tpl2 )
struct queryexpr **condlist;
unsigned short condcnt;
struct qnode *node1;
struct qnode *node2;
register struct utuple *tpl1, *tpl2;
{
	/*
	 * The NULL tuple always fails a match.
	 */
	if ( (tpl1->flags & TPL_NULLTPL) || (tpl2->flags & TPL_NULLTPL) )
	{
		return( FALSE );
	}

	while( condcnt-- != 0 )
	{
		if ( ! chkjcond( *condlist++, node1, node2,
				tpl1, tpl2 ) )
		{
			/*
			 * Because we're doing ANDs of single join
			 * comparisons, we can return an immediate
			 * failure of the match.
			 */
			tpl1->flags |= TPL_DELETE;
			tpl2->flags |= TPL_DELETE;
			return( FALSE );
		}
	}

	return( TRUE );
}
