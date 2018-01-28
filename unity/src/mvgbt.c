/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

mvgbt(n, len, a, b) 
char	*a, *b;
{
	register	int num=n*len;
	register	char *x=a, *y=b;

	if (x>y)
		for(;num>0;num--)
			*y++= *x++;
	else
		for(num-- ; num>=0 ; num--)
			*(y+num)= *(x+num) ;
}
