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

void
unmkquery( query, free_nodes, free_rels )
struct uquery *query;
int free_nodes;		/* free querynode structures */
int free_rels;		/* free relation structures? */
{
	int i;
	struct uqoperation *operptr, *next;


	/* free the query operations */
	for( operptr = query->operlist; operptr; operptr = next )
	{
		/* Free each query expression */
		for( i = 0; i < operptr->cmpcnt; i++ )
			freeqexprtree( operptr->cmplist[i] );

		free( operptr->cmplist );

		next = operptr->next;
		free( operptr );
	}

	/* free the projected attribute list */
	if ( query->attrlist )
		free( query->attrlist );

	/*
	 * Free the nodes and/or relations if needed.  We
	 * assume the nodes and relations are all allocated
	 * together.
	 */
	if ( free_rels )
		freerelinfo( query->nodelist[0]->rel );
	if ( free_nodes )
		free( query->nodelist[0] );

	free( query );
}
