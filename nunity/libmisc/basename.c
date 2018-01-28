/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define NULL (char *)0

extern char *strrchr( );

char *
basename( path )
char *path;
{
	char *ptr;

	while( path && *path ) {
		if ( (ptr = strrchr( path, '/' )) == NULL )
			return( path );
		else if ( *++ptr )
			return( ptr );
		ptr[-1] = '\0';		/* wipe out the last '/' */
	}

	return( path );
}
