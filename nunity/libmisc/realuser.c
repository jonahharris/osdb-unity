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
#include <pwd.h>
#include <sys/utsname.h>

extern struct passwd *getpwuid();

char *
realuser( )
{
	struct passwd *pwdptr;
	struct utsname local;
	static char machlogin[20];

	if ( machlogin[0] == '\0' ) {
		pwdptr = getpwuid( getuid() );
		if ( pwdptr == NULL )
			return( NULL );
		uname( &local );

		sprintf( machlogin, "%s!%s", local.nodename, pwdptr->pw_name );
	}

	return( machlogin );
}
