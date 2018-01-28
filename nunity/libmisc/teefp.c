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

#define BUFSIZE		4096

teefp( dest1fp, dest2fp, srcfp )
FILE *dest1fp;
FILE *dest2fp;
FILE *srcfp;
{
	int i;
	char buf[BUFSIZE];

	/*
	 * Copy table to temp file.
	 */
	while( (i = fread( buf, 1, BUFSIZE, srcfp )) > 0 ) {
		if ( dest1fp && fwrite( buf, 1, i, dest1fp ) != i )
			return( 0 );		/* write error */
		if ( dest2fp && fwrite( buf, 1, i, dest2fp ) != i )
			return( 0 );		/* write error */

		if ( i != BUFSIZE )
			break;			/* end of file */
	}
	if ( i < 0 )
		return( 0 );			/* read error */

	return( 1 );
}
