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

findattr( attrptr, attrcnt, attr )
register struct uattribute *attrptr;
unsigned int attrcnt;
char *attr;
{
	register int i;

	for( i = 0; i < attrcnt; i++, attrptr++ ) {
		if ( strcmp( attrptr->aname, attr ) == 0 )
			return( i );
	}

	return( -1 );
}

