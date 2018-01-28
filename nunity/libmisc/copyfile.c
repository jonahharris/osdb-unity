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

copyfile( dest, src, append )
char *src;
char *dest;
int append;
{
	int rc;
	FILE *destfp;
	FILE *srcfp;

	if ( (srcfp = fopen( src, "r" )) == NULL )
		return( 0 );
	if ( (destfp = fopen( dest, append ? "w" : "a" )) == NULL )
		return( 0 );

	rc = copyfp( destfp, srcfp );

	fclose( destfp );
	fclose( srcfp );

	return( rc );
}
