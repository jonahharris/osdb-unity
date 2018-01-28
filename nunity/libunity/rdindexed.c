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
#include "urelation.h"
#include "uindex.h"

rdindexed( indptr, key, keyval, seekval )
register struct uindex *indptr;
char *key;		/* search key */
char **keyval;		/* found key value returned */
long *seekval;		/* seek value in accession file returned */
{
	int i;
	long seekaddr;

	memcpy( (char *)indptr->leaf, (char *)indptr->root, UI_NODESIZE );
	while( indptr->leaf->level > 0 ) {
		(void)scan( key, indptr, &seekaddr );
		if ( ! ndread( indptr->fd, seekaddr, indptr->leaf ) )
			return( MISSED );
	}
	i = scan( key, indptr, &seekaddr );
	if (i == FOUND ) {
		if ( keyval != NULL )
			*keyval = indptr->ordptr;
		*seekval = seekaddr;
		return( FOUND );
	}
	if ( *indptr->ordptr == UI_MAXCHAR )
		return( END );

	if ( keyval != NULL )
		*keyval = indptr->ordptr;
	*seekval = seekaddr;

	return( MISSED );
}
