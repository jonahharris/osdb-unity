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

/*
 * Read next key value and handle moving to new node.
 */
rdnext( indptr, keyval, seekval )
struct uindex *indptr;
char **keyval; 	/* return address of key found */
long *seekval;	/* seek value associated with key */
{
	register char *ptr1, *ptr2;
	char buf[UI_MAXKEYLEN + 2];

	if ( *(ptr1 = indptr->rdptr) != UI_MAXCHAR ) { /* not at end of node */
		if ( keyval != NULL )
			*keyval = ptr1;

		while( *ptr1++ != '\0' )
			;
		for( ptr2 = (char *)seekval; ptr2 < (char *)(seekval + 1); )
			*ptr2++ = *ptr1++;

		/* update read pointers */
		indptr->ordptr = indptr->rdptr;
		indptr->rdptr = ptr1;

		return( FOUND );
	}

	/*
	 * Get a new node by appending a '\01' to the last key value read
	 * (the next alphabetically greater key value) and calling rdindexed().
	 */
	for( ptr2 = buf, ptr1 = indptr->ordptr; *ptr1 != '\0'; )
		*ptr2++ = *ptr1++;
	*ptr2++ = '\01';
	*ptr2 = '\0';

	return( rdindexed( indptr, buf, keyval, seekval ) );
}
