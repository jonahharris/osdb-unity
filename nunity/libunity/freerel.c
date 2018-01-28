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

void
freeattrlist( attrptr, attrcnt )
struct uattribute *attrptr;
int attrcnt;
{
	while( attrcnt-- > 0 )
	{
		if ( attrptr->friendly )
		{
			free( attrptr->friendly );
			attrptr->friendly = NULL;
		}
		attrptr++;
	}
}

void
freerelinfo( relptr )
struct urelation *relptr;
{
	if ( relptr->dpath && (relptr->relflgs & UR_DPATHALLOC) )
		free( relptr->dpath );

	if ( relptr->path && (relptr->relflgs & UR_PATHALLOC) )
		free( relptr->path );

	if ( relptr->attrs )
	{
		freeattrlist( relptr->attrs, relptr->attrcnt );
		free( relptr->attrs );
	}

	if ( relptr->relflgs & UR_RELALLOC )
		free( relptr );
}
