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

setnum(fld, s, fldnum)
struct	fmt *fld; 
char	*s;
{
	int	i;

	for(i=0;i<fldnum;i++)
		if(strcmp(s,fld[i].aname)==0) return(i);
	return(ERR);
}
