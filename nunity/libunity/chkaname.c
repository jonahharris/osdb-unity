/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include "urelation.h"
#include "uerror.h"

chkaname( str )
register char *str;
{
	/*
	 * Check the attribute name for legal chars.
	 */
	if ( ! isalpha( *str ) && *str != '_' ) {
		set_uerror( UE_ATTRNAME );
		return( FALSE );
	}
	while( *++str ) {
		if ( ! isalnum( *str ) && *str != '_' ) {
			set_uerror( UE_ATTRNAME );
			return( FALSE );
		}
	}

	return( TRUE );
}
