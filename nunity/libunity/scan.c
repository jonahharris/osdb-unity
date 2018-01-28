/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "urelation.h"
#include "uindex.h"

scan( key, indptr, seekval )
char *key;
long *seekval; 
struct uindex *indptr;
{
	register int rc;
	register char *ptr1;
	register char *ptr2;

	ptr1 = (char *)indptr->leaf + sizeof( struct unodehdr );
	while( (rc = strcmp( key, ptr1 )) > 0 ) {
		while( *ptr1++ )
			;
		ptr1 += sizeof( long );  /* skip over address to next key */
	}

	indptr->ordptr = ptr1;

	/*
	 * Copy the seek address into seekval and update rdptr.
	 * We can't just use ptr1, because its alignment may not
	 * be right to do a long assignment.
	 */
	while( *ptr1++ )	/* skip the key */
		;
	for( ptr2 = (char *)seekval; ptr2 < (char *)(seekval + 1); )
		*ptr2++ = *ptr1++;

	indptr->rdptr = ptr1;

	return( rc == 0 ? FOUND : MISSED );
}
