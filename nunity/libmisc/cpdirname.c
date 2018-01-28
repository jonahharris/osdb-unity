/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern char *strrchr( );

char *
cpdirname( dest, path )
register char *dest;
register char *path;
{
	register char *ptr;

	ptr = strrchr( path, '/' );
	while( ptr && ptr[1] == '\0' ) {	/* get rid of trailing '/' */
		*ptr = '\0';
		ptr = strrchr( path, '/' );
	}

	if ( ptr ) {
		while( path < ptr )
			*dest++ = *path++;
	}
	else
		*dest++ = '.';

	*dest = '\0';

	return( dest );	
}
