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

copysrcfile( destfp, src )
FILE *destfp;
char *src;
{
	int rc;
	FILE *srcfp;

	if ( (srcfp = fopen( src, "r" )) == NULL )
		return( 0 );
	rc = copyfp( destfp, srcfp );
	fclose( srcfp );

	return( rc );
}
