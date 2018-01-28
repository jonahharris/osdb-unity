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
#include <fcntl.h>
#include "urelation.h"
#include "uindex.h"

struct uindex *
bopen( path )
char *path;
{
	static struct uindex ind;
	static char node1[UI_NODESIZE], node2[UI_NODESIZE];

	ind.ordptr = 0;
	ind.rdptr = 0;
	if ( (ind.fd = open( path, O_RDONLY )) < 0 )
		return( NULL );

	ind.root = (struct unodehdr *)node1;
	ind.leaf = (struct unodehdr *)node2;

	if ( ! ndread( ind.fd, 0L, (struct unodehdr *)node1 ) ) {
		(void)close( ind.fd );
		return( NULL );
	}

	memcpy( node2, node1, UI_NODESIZE );

	return( &ind );
}
