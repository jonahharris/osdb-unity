/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include "urelation.h"

writetuple( fp, relptr, attrvals )
FILE *fp;
register struct urelation *relptr;
register char **attrvals;
{
	register int i;

	for( i = 0; i < relptr->attrcnt; i++ ) {
		if ( ! writeattr( fp, &relptr->attrs[i], attrvals[i],
				i == relptr->attrcnt - 1 ) )
			return( FALSE );
	}

	return( TRUE );
}
