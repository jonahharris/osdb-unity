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
/* read next key value and handle moving to new node */

extern	char	*strcpy();

rdnext(ind, keyval, seekval)
struct	index *ind;
char	**keyval; 	/* return address of key found */
long	*seekval;	/* seek value associated with key */
{
	char	*a, buf[KEYLEN];
	int	i;
	union	{
		char	*c;
		long	*l;
	}	ptr;

	if ( *(a=ind->rdptr) != MAXCHAR) {	/* not at end of node */
		if ( keyval != 0 )
			*keyval=a;
		while(*a++!=0);
		ptr.l = seekval;
		for (i=0;i<sizeof(long);i++)
			*ptr.c++ = *a++;

		/* update read pointers */
		ind->ordptr = ind->rdptr;
		ind->rdptr = a;
		return(FOUND);
	}

	/* get a new node by appending a '1' to the last key value read
	   (the next alphabetically greater key value) and using rdindexed
	*/
	strcpy(buf,ind->ordptr);
	for(a=buf;*a!=0;a++);
	*a++='\1';
	*a='\0';
	return(rdindexed(ind,buf,keyval,seekval));
}
