/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define NULL ((char *)0)

extern char *strrchr( );

char *
dirname( path )
register char *path;
{
	register char *ptr;

	if ( (ptr = strrchr( path, '/' )) == NULL ) {
		if ( path )
			*path = '\0';
		return( path );
	}

	while( ptr && ptr[1] == '\0' ) {	/* get rid of trailing '/' */
		*ptr = '\0';
		ptr = strrchr( path, '/' );
	}

	ptr[1] = '\0';

	return( path );	
}
