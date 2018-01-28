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

copydestfile( dest, srcfp, append )
FILE *srcfp;
char *dest;
int append;
{
	int rc;
	FILE *destfp;

	if ( (destfp = fopen( dest, append ? "w" : "a" )) == NULL )
		return( 0 );
	rc = copyfp( destfp, srcfp );
	fclose( destfp );

	return( rc );
}
