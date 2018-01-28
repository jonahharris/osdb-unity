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

struct qnode *
exprnode( qptr )
register struct queryexpr *qptr;
{
	register struct qnode *nodeptr;
	/*
	 * Find the relation used in the expression.
	 * We either return NULL if the nodes of this clause are not
	 * the same or return the node that they reference.
	 */
	if( ISBOOL( qptr->optype ) ) {
		if ( (nodeptr = exprnode( qptr->elem1.expr )) == NULL )
			return( NULL );
		return ( nodeptr == exprnode(qptr->elem2.expr) ? nodeptr : NULL );
	}
	else if ( ISSETCMP( qptr->optype ) )
	{
		return( qptr->elem1.alist.list[0].rel );
	}
	else if ( qptr->opflags & ISATTR2 )
	{
		return( qptr->elem1.attr.rel == qptr->elem2.attr.rel ?
			qptr->elem1.attr.rel : NULL );
	}
	else
		return( qptr->elem1.attr.rel );
}
