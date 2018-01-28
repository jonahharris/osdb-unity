/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/utsname.h>

localmach( machine )
char *machine;
{
	struct utsname name;

	uname( &name );
	return( strcmp( machine, name.nodename ) == 0 );
}
