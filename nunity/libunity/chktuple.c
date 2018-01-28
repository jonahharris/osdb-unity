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

#ifdef	__STDC__
#include <stdlib.h>
#else
extern	long strtol();
#endif

extern char *regex();
extern struct qnode *exprnode();
extern double atof();

static
chkstrlist( list1, list2, cnt )
register char **list1, **list2;
register int cnt;
{
	while( cnt-- > 0 ) {
		if ( strcmp( *list1++, *list2++ ) )
			return( FALSE );
	}

	return( TRUE );
}

static
chkcaselist( list1, list2, cnt )
register char **list1, **list2;
register int cnt;
{
	while( cnt-- > 0 ) {
		if ( nocasecmp( *list1++, *list2++ ) )
			return( FALSE );
	}

	return( TRUE );
}

static
chknumlist( list1, list2, cnt )
register char **list1, **list2;
register int cnt;
{
	while( cnt-- > 0 ) {
		if ( atof( *list1++ ) != atof( *list2++ ) )
			return( FALSE );
	}

	return( TRUE );
}

static
chkdatelist( list1, list2, cnt )
register char **list1, **list2;
register int cnt;
{
	int cmp;

	while( cnt-- > 0 ) {
		if ( ! chkdate( *list1++, *list2++, &cmp ) || cmp != 0 )
			return( FALSE );
	}

	return( TRUE );
}

static
chkDatelist( list1, list2, cnt )
register char **list1, **list2;
register int cnt;
{
	int cmp;

	while( cnt-- > 0 ) {
		if ( ! chkDate( *list1++, *list2++, &cmp ) || cmp != 0 )
			return( FALSE );
	}

	return( TRUE );
}

static
chkrexplist( list1, list2, cnt )
register char **list1, **list2;
register int cnt;
{
	while( cnt-- > 0 ) {
		if ( ! regex( *list2++, *list1++, NULL ) )
			return( FALSE );
	}

	return( TRUE );
}

static
chkset( qptr, tptr )
register struct queryexpr *qptr;
register struct utuple *tptr;
{
	register int i, cnt;
	register struct attrref *refptr;
	register char **strlist;
	char buf[20];
	char *attrvals[MAXATT];

	cnt = qptr->elem1.alist.cnt;
	refptr = qptr->elem1.alist.list;
	strlist = qptr->elem2.strlist;

	for( i = 0; i < cnt; i++, refptr++ ) {
		switch( refptr->attr ) {
		case ATTR_RECNUM:
			sprintf( buf, "%ld", tptr->tuplenum );
			attrvals[i] = buf;
			break;
		case ATTR_SEEK:
			sprintf( buf, "%ld", tptr->lseek );
			attrvals[i] = buf;
			break;
		default:	/* normal attribute */
			attrvals[i] = tptr->tplval[refptr->attr];
			break;
		}
	}

	switch( qptr->cmptype ) {
	case QCMP_STRING:
		while( *strlist ) {
			if ( chkstrlist( attrvals, strlist, cnt ) )
				return( TRUE );
			strlist += cnt;
		}
		break;
	case QCMP_CASELESS:
		while( *strlist ) {
			if ( chkcaselist( attrvals, strlist, cnt ) )
				return( TRUE );
			strlist += cnt;
		}
		break;
	case QCMP_NUMBER:
		while( *strlist ) {
			if ( chknumlist( attrvals, strlist, cnt ) )
				return( TRUE );
			strlist += cnt;
		}
		break;
	case QCMP_DATE:
		while( *strlist ) {
			if ( chkdatelist( attrvals, strlist, cnt ) )
				return( TRUE );
			strlist += cnt;
		}
		break;
	case QCMP_DATEONLY:
		while( *strlist ) {
			if ( chkDatelist( attrvals, strlist, cnt ) )
				return( TRUE );
			strlist += cnt;
		}
		break;
	case QCMP_REGEXPR:
		while( *strlist ) {
			if ( chkrexplist( attrvals, strlist, cnt ) )
				return( TRUE );
			strlist += cnt;
		}
		break;
	}

	return( FALSE );
}

chkcompare( nodeptr, tptr, qptr )
struct qnode *nodeptr;
register struct utuple *tptr;
register struct queryexpr *qptr;
{
	register char *str1, *str2;
	register double num1, num2;
	register int cmp;
	char buf[20];		/* for record numbers */

	if ( qptr->trvflags & QE_CHECKED ) {
		/*
		 * This tuple was previously checked.  Return the same
		 * status as was returned last time.  This is just
		 * a speed improvement that takes advantage of the fact
		 * that comparison nodes in the query expression are
		 * not duplicated, even though they may be used in
		 * multiple edges.
		 */
		return( qptr->trvflags & QE_TPLMATCH );
	}

	if ( ISSETCMP( qptr->optype ) ) {
		/*
		 * Doing a set comparison.  If this comparison does
		 * not apply to this node, return no match.  (A set
		 * comparison can only apply to a single node.)
		 */
		if ( qptr->elem1.alist.list->rel != nodeptr )
			return( FALSE );

		/*
		 * Check the set comparison.  If the operation
		 * is NOT IN, then return the opposite of the
		 * actual comparison result.  It **might** be
		 * slightly faster to pass the NOT down to chkset()
		 * (it could then return as soon as possible),
		 * but the code would be a lot more complicated.
		 * I decided it wasn't worth it.
		 */
		if ( qptr->optype == OPIN )
			return( chkset( qptr, tptr ) );
		else
			return( ! chkset( qptr, tptr ) );
	}

	if ( qptr->opflags & HASJOIN ) {
		/*
		 * This expression is a join.  We return a FALSE,
		 * since we're only considering selection.
		 */
		return( FALSE );
	}

	if ( qptr->elem1.attr.rel != nodeptr ) {
		/*
		 * This selection expression does not apply to this
		 * node.
		 */
		return( FALSE );
	}

	/*
	 * Do the actual comparison.
	 */
	switch( qptr->cmptype ) {
	case QCMP_NUMBER:
		switch( qptr->elem1.attr.attr ) {
		case ATTR_RECNUM:
			num1 = tptr->tuplenum;
			break;
		case ATTR_SEEK:
			num1 = tptr->lseek;
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
					num1 = (int) strtoul( tptr->tplval[qptr->elem1.attr.attr],
								(char **)NULL, qptr->modifier1 );
#else
					num1 = (int) strtol( tptr->tplval[qptr->elem1.attr.attr],
								(char **)NULL, qptr->modifier1 );
#endif
				} else if ( qptr->modifier1 == 1 ) {
					num1 = strlen( tptr->tplval[qptr->elem1.attr.attr] );
				} else {
					return( FALSE );
				}
			} else {
				num1 = atof( tptr->tplval[qptr->elem1.attr.attr] );
			}
		}
		if ( (qptr->opflags & ISATTR2) == 0 )
			num2 = qptr->elem2.numval;
		else {
			switch( qptr->elem2.attr.attr ) {
			case ATTR_RECNUM:
				num2 = tptr->tuplenum;
				break;
			case ATTR_SEEK:
				num2 = tptr->lseek;
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
						num2 = (int) strtoul( tptr->tplval[qptr->elem2.attr.attr],
								(char **)NULL, qptr->modifier2 );
#else
						num2 = (int) strtol( tptr->tplval[qptr->elem2.attr.attr],
									(char **)NULL, qptr->modifier2 );
#endif
					} else if ( qptr->modifier2 == 1 ) {
						num2 = strlen( tptr->tplval[qptr->elem2.attr.attr] );
					} else {
						return( FALSE );
					}
				} else {
					num2 = atof( tptr->tplval[qptr->elem2.attr.attr] );
				}
			}
		}

		cmp = ( ( num1 < num2 ) ? -1 : ( num1 != num2 ) );
		break;
	case QCMP_DATE:
	{
		int tmp_cmp;

		str1 = tptr->tplval[qptr->elem1.attr.attr];
		str2 = (qptr->opflags & ISATTR2) ?
				tptr->tplval[qptr->elem2.attr.attr] :
				qptr ->elem2.strval;
		if ( ! chkdate( str1, str2, &tmp_cmp ) )
			return( FALSE );
		cmp = tmp_cmp;
		break;
	}
	case QCMP_DATEONLY:
	{
		int tmp_cmp;

		str1 = tptr->tplval[qptr->elem1.attr.attr];
		str2 = (qptr->opflags & ISATTR2) ?
				tptr->tplval[qptr->elem2.attr.attr] :
				qptr ->elem2.strval;
		if ( ! chkDate( str1, str2, &tmp_cmp ) )
			return( FALSE );
		cmp = tmp_cmp;
		break;
	}
	case QCMP_STRING:
		str1 = tptr->tplval[qptr->elem1.attr.attr];
		str2 = (qptr->opflags & ISATTR2) ?
				tptr->tplval[qptr->elem2.attr.attr] :
				qptr ->elem2.strval;
		cmp = strcmp( str1, str2 );
		break;
	case QCMP_CASELESS:
		str1 = tptr->tplval[qptr->elem1.attr.attr];
		str2 = (qptr->opflags & ISATTR2) ?
				tptr->tplval[qptr->elem2.attr.attr] :
				qptr ->elem2.strval;
		cmp = nocasecmp( str1, str2 );
		break;
	case QCMP_REGEXPR:
		switch( qptr->elem1.attr.attr ) {
		case ATTR_RECNUM:
			sprintf( buf, "%ld", tptr->tuplenum );
			str1 = buf;
			break;
		case ATTR_SEEK:
			sprintf( buf, "%ld", tptr->lseek );
			str1 = buf;
			break;
		default:
			str1 = tptr->tplval[qptr->elem1.attr.attr];
			break;
		}
		cmp = ( ! regex( qptr->elem2.strval, str1, NULL ) );
		break;
	default:
		return( FALSE );
	}

	switch( qptr->optype ) {
	case OPEQ:
		return( cmp == 0 );
	case OPNE:
		return( cmp != 0 );
	case OPLT:
		return( cmp < 0 );
	case OPLE:
		return( cmp <= 0 );
	case OPGT:
		return( cmp > 0 );
	case OPGE:
		return( cmp >= 0 );
	}

	return( FALSE );
}

chkelse( nodeptr, tptr, qptr )
struct qnode *nodeptr;
struct utuple *tptr;
struct queryexpr *qptr;
{
	/*
	 * Each ELSE sub-clause has been put into CNF.  So
	 * take the sub-expression of each AND and treat it
	 * like an edge itself.
	 */
	while( qptr->optype == OPAND ) {
		if ( ! chkelse( nodeptr, tptr, qptr->elem1.expr ) )
			return( FALSE );
		qptr = qptr->elem2.expr;
	}

	/*
	 * We have a sub-expression of the ELSE that is not
	 * an AND.  So treat it just like any other edge and
	 * check the tuple against it.
	 */
	return ( matchedge( nodeptr, tptr, qptr ) );
}

static int
matchedge( nodeptr, tptr, qptr )
struct qnode *nodeptr;
struct utuple *tptr;
struct queryexpr *qptr;
{
	register int op;

	while( 1 ) {
		op = qptr->optype;
		if ( ISCOMPARE( op ) ) {
			if ( chkcompare( nodeptr, tptr, qptr ) ) {
				qptr->trvflags |= QE_CHECKED|QE_TPLMATCH;
				return( TRUE );
			}
			qptr->trvflags |= QE_CHECKED;
			return( FALSE );
		}
		else if ( op == OPELSE ) {
			/*
			 * ELSE-clauses can only contain references to
			 * a single node.  So if that node isn't this
			 * one, return what the current status is.
			 */
			if ( exprnode( qptr ) != nodeptr )
				return( FALSE );

			if ( chkelse( nodeptr, tptr, qptr->elem1.expr ) ) {
				/*
				 * The tuple matched the left side of the
				 * ELSE-clause.  Mark the match direction.
				 * and return a match.  We'll later save
				 * tuple references for the left side and
				 * delete any tuples that previously matched
				 * the right side of the ELSE.
				 */
				qptr->trvflags |= QE_TPLLEFT;
				return( TRUE );
			}
			else if ( qptr->trvflags & QE_LEFTCHILD ) {
				/*
				 * The tuple did not match the left side
				 * of the ELSE, but some previous tuple
				 * did.  There is no sense in checking
				 * the right side; just return no match.
				 */
				return( FALSE );
			}

			if ( chkelse( nodeptr, tptr, qptr->elem2.expr ) ) {
				/*
				 * The tuple matched the right side of the
				 * ELSE-clause.  Mark the match direction.
				 * and return a match.  We'll later save
				 * tuple references for the right side.
				 */
				qptr->trvflags |= QE_TPLRIGHT;
				return( TRUE );
			}
			else
				return( FALSE );
		}
		else {
			/*
			 * This is a boolean OR expression.  Treat it
			 * like a normal edge.
			 */
			if ( matchedge( nodeptr, tptr, qptr->elem1.expr ) ) {
				/*
				 * Since we're doing ORs, any success
				 * causes an immediate success of the
				 * whole match.
				 */
				return( TRUE );
			}
		}

		qptr = qptr->elem2.expr;
	}
}

static void
chktuplerefs( query, nodeptr, blkptr )
struct uquery *query;
struct qnode *nodeptr;
register struct utupleblk *blkptr;
{
	register int i;
	register struct utupleblk *next;

	/*
	 * Check the reference count for each tuple.  If it is
	 * zero all matches for the tuple have been superseded.
	 * so delete the tuple from the node.
	 */
	for( ; blkptr; blkptr = next ) {
		for( i = 0; i < blkptr->tuplecnt; i++ ) {
			if ( --blkptr->tuples[i]->refcnt == 0 )
				(void)deltuple( query, nodeptr,
						blkptr->tuples[i] );
		}
		next = blkptr->next;
		freetplblk( blkptr );
	}
}

static void
unsavetuplerefs( query, nodeptr, qptr )
struct uquery *query;
struct qnode *nodeptr;
register struct queryexpr *qptr;
{
	while( 1 ) {
		if ( qptr->trvflags & QE_LEFTCHILD ) {
			if ( (qptr->elem1.expr->opflags & HASELSE) == 0 ) {
				chktuplerefs( query, nodeptr, qptr->tuples );
				qptr->tuples = NULL;
				return;
			}
			else
				qptr = qptr->elem1.expr;
		}
		else if ( qptr->trvflags & QE_RIGHTCHILD ) {
			if ( (qptr->elem2.expr->opflags & HASELSE) == 0 ) {
				chktuplerefs( query, nodeptr, qptr->tuples );
				qptr->tuples = NULL;
				return;
			}
			else
				qptr = qptr->elem2.expr;
		}
		else
			return;
	}
}

static int
savetupleref( query, nodeptr, tptr, qptr, dosave )
struct uquery *query;
struct qnode *nodeptr;
struct utuple *tptr;
register struct queryexpr *qptr;
register int dosave;
{
	/*
	 * Save the reference to the given tuple for ELSE-clauses.
	 * Also mark which sub-expression matched an ELSE: left (elem1.expr)
	 * or right (elem2.expr).  Additionally, we remove any superseded
	 * tuple ref's (i.e., ref's that were on the right side, but now
	 * are on the left.) and reset all current tuple match flags.
	 *
	 * There are two sets of flags we're playing with here: the
	 * current tuple match flags (QE_TPLLEFT and QE_TPLRIGHT)
	 * and the past tuple(s) match flags (QE_LEFTCHILD and QE_RIGHTCHILD).
	 * We'll look at the current match flags to see where to put
	 * the reference to the current tuple.  We'll look at the past
	 * match flags to see if prior tuple references have been
	 * superseded.
	 *
	 * We save references to a tuple only where the match actually
	 * took place.  However, the marking takes place all up and down
	 * the expression.
	 *
	 * If "dosave" is FALSE we remove the current tuple match
	 * flags, but don't change the past tuple match flags.  This
	 * is the case when the current tuple failed to match on some
	 * edge.
	 */
	while( qptr->opflags & HASELSE ) {
		/*
		 * This expression has an ELSE somewhere.  If the main
		 * expression is an ELSE, mark and save the ref, depending
		 * on which sub-expr matched.  Otherwise descend both
		 * sub-expr and repeat the process.
		 */
		if ( qptr->optype == OPELSE ) {
			if ( qptr->trvflags & QE_TPLLEFT ) {
				qptr->trvflags &= ~QE_TPLLEFT;
				if ( dosave ) {
					if ( qptr->trvflags & QE_RIGHTCHILD ) {
						unsavetuplerefs( query, nodeptr, qptr );
						qptr->trvflags &= ~QE_RIGHTCHILD;
					}
					qptr->trvflags |= QE_LEFTCHILD;
					if ( (qptr->elem1.expr->opflags & HASELSE) == 0 &&
							! addtupleref( &qptr->tuples, tptr ) )
						return( FALSE );
				}
			}
			else if ( qptr->trvflags & QE_TPLRIGHT ) {
				qptr->trvflags &= ~QE_TPLRIGHT;
				if ( dosave ) {
					qptr->trvflags |= QE_RIGHTCHILD;
					if ( (qptr->elem2.expr->opflags & HASELSE) == 0 &&
							! addtupleref( &qptr->tuples, tptr ) )
						return( FALSE );
				}
			}
		}

		if ( ! savetupleref( query, nodeptr, tptr, qptr->elem1.expr, dosave ) )
			return( FALSE );
		qptr = qptr->elem2.expr;
	}

	return( TRUE );
}

static void
nomatch( qptr )
register struct queryexpr *qptr;
{
	while( ISBOOL( qptr->optype ) ) {
		nomatch( qptr->elem1.expr );
		qptr = qptr->elem2.expr;
	}

	qptr->trvflags &= ~(QE_CHECKED|QE_TPLMATCH);
}

chktuple( query, operptr, nodeptr, tptr, ignore )
struct uquery *query;
struct uqoperation *operptr;
register struct qnode *nodeptr;
struct utuple *tptr;
int ignore;		/* compare to ignore - because of indexing */
{
	register int i, maxedge;
	int dosave;
	int tplmatch;

	/*
	 * Unmark all tuple match flags for the expression.
	 * Also remove any edge markers.  The edge markers
	 * say which edges the tuple was actually compared
	 * against and contains an ELSE.
	 */
	for( i = 0; i < operptr->cmpcnt; i++ ) {
		nomatch( operptr->cmplist[i] );
	}

	/*
	 * Don't mark match paths or save references to this tuple
	 * unless there is an ELSE in one of the expressions.
	 */
	dosave = FALSE;
	tplmatch = TRUE;

	for( i = 0;  i < operptr->cmpcnt; i++ ) {
		if ( i == ignore ) {
			/*
		 	 * This is the edge from the indexes.
			 */
			continue;
		}

		if ( operptr->cmplist[i]->opflags & HASELSE ) {
			/*
			 * Set the dosave flag so we'll mark
			 * match paths and save tuple references.
			 */
			dosave = TRUE;
		}

		if ( ! matchedge( nodeptr, tptr, operptr->cmplist[i] ) ) {
			tplmatch = FALSE;
			break;
		}
	}

	/*
	 * We must save the tuple in ELSE-clauses.
	 * This includes resetting the QE_TPLLEFT and
	 * QE_TPLRIGHT flags.
	 */
	if ( dosave ) {
		/*
		 * We are only interested in those edges that we
		 * traversed that have ELSEs.  No other edges
		 * had match paths set.
		 */
		maxedge = i;
		for( i = 0; i < maxedge; i++ ) {
			(void)savetupleref( query, nodeptr, tptr,
					operptr->cmplist[i], tplmatch );
		}
	}

	return( tplmatch );
}

static void
undotupleref( nodeptr, qptr )
struct qnode *nodeptr;
register struct queryexpr *qptr;
{
	while( qptr->opflags & HASELSE ) {
		if ( qptr->optype == OPELSE ) {
			freetplblks( qptr->tuples, FALSE );
			qptr->tuples = NULL;
			qptr->trvflags &= ~(QE_RIGHTCHILD|QE_LEFTCHILD);
		}
		undotupleref( nodeptr, qptr->elem1.expr );
		qptr = qptr->elem2.expr;
	}
}

void
unsaveelse( operptr, nodeptr )
struct uqoperation *operptr;
register struct qnode *nodeptr;
{
	register int i;

	for( i = 0; i < operptr->cmpcnt; i++ )
		undotupleref( nodeptr, operptr->cmplist[i] );
}
