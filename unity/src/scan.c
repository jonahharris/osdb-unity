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

scan(key, ind, seekval)
char	*key;
long	*seekval; 
struct	index *ind;
{
	int	ret, i;
	union	{
		char	*b;
		struct hdr *x;
	}	ptr1;
	union	{
		char	*c;
		long	*l;
	}	ptr2;

	ptr1.x=ind->leaf;
	ptr1.b=ptr1.b+HDR;
	while(strcmp(key,ptr1.b) > 0) {
		while( *ptr1.b++ != 0 );
		ptr1.b += sizeof(long);
	}
	if(strcmp(key,ptr1.b)==0)
		ret=FOUND;
	else
		ret=MISSED;
	ind->ordptr = ptr1.b;

	/* copy the seek address into seekval and update rdptr */
	while( *ptr1.b++ !=0 );
	ptr2.l = seekval;
	for (i=0;i<sizeof(long);i++)
		*ptr2.c++ = *ptr1.b++;
	ind->rdptr = ptr1.b;

	return(ret);
}
