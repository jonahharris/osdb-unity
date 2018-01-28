/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "db.h"

ndcheck(x)
int *x;
{
	int	s=0;
	int	i;

	for(i=0 ; i<NDSZ/sizeof(int) ; i++)
		s ^= *x++;
	return(s);
}
