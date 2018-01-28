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


struct	index
*bopen(s)
char *s;
{
	static char ind[sizeof(struct index)];
	static char node1[NDSZ], node2[NDSZ];

	/* unions used to avoid pointer warnings */
	union {
		struct index *x;
		char	*c;
	}	ptr1;
	union {
		struct hdr *header;
		char	*c;
	}	ptr2;

	ptr1.c = ind;					/* index */
	ptr1.x->ordptr = 0;
	ptr1.x->rdptr = 0;
	if ( (ptr1.x->fd=open(s,2)) < 0)		 /* open index */
		return(NULL);

	ptr2.c = node1;
	ptr1.x->root = ptr2.header;			/* space for root node*/

	ptr2.c = node2;
	ptr1.x->leaf = ptr2.header;			/* space for leaf node*/

	if (ndread(ptr1.x->fd,0L,ptr1.x->root) == ERR)	/* read root node */
		return(NULL);
	mvgbt(1,NDSZ,(char *)ptr1.x->root,
		(char *)ptr1.x->leaf);			/* copy root to leaf */
	return(ptr1.x);
}
