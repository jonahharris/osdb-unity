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

extern long lseek();

ndread(n,x,s)
int	n;
long	x; 
struct hdr	*s;
{
	union	{
		struct hdr	*header;
		char		*c;
		int		*i;
	}	ptr;

	if (lseek(n,x,0)<0)
		return(ERR);
	ptr.header = s;
	if (read(n,ptr.c,NDSZ)<NDSZ)
		return(ERR);
	if (s->nodeloc!=x)
		return(ERR);
	if (ndcheck(ptr.i)!=0)
		return(ERR);
	return(0);
}
