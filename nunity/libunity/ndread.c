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

extern long lseek();

ndread( fd, addr, node )
int fd;
long addr; 
struct unodehdr *node;
{
	if ( lseek( fd, addr, 0 ) < 0L )
		return( FALSE );
	if ( read( fd, (char *)node, UI_NODESIZE ) < UI_NODESIZE )
		return( FALSE );

	return( node->nodeloc == addr && ndcheck( node ) == 0 );
}
