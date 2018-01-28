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

ndcheck( node )
struct unodehdr *node;
{
	register int chksum;
	register int *ptr, *endptr;

	chksum = 0;
	ptr = (int *)node;
	endptr = (int *)((char *)ptr + UI_NODESIZE);
	while( ptr < endptr )
		chksum ^= *ptr++;

	return( chksum );
}
