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

rdindexed(ind, key, keyval, seekval)
struct	index *ind;
char	*key;		/* search key */
char	**keyval; 	/* found key value returned */
long	*seekval;	/* seek value in chkaccession file returned */
{
	int	i;
	long	seekaddr;

	mvgbt(1,NDSZ,(char*)ind->root,(char*)ind->leaf);
	while(ind->leaf->level >0) {
		(void)scan(key,ind,&seekaddr);
		if (ndread(ind->fd,seekaddr,ind->leaf)==ERR) {
			error(E_ERR,"rdindexed","ndread");
			return(MISSED);
		}
	}
	i=scan(key,ind,&seekaddr);
	if (i == FOUND) {
		if (keyval != 0)
			*keyval = ind->ordptr;
		*seekval = seekaddr;
		return(FOUND);
	}
	if (*ind->ordptr == MAXCHAR)
		return(END);
	if (keyval != 0)
		*keyval = ind->ordptr;
	*seekval = seekaddr;
	return(MISSED);
}
