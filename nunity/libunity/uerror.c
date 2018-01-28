/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "uerror.h"

int uerror;

void
set_uerror( error )
int error;
{
	uerror = error;
}

void
reset_uerror( )
{
	uerror = UE_NOERR;
}

int
is_uerror_set( )
{
	return( uerror != UE_NOERR );
}
