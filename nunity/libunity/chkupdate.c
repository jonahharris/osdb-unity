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
#include "permission.h"

extern char *cpdirname();
extern int chkperm();
extern unsigned short geteuid(), getegid();

chkupdate( relpath )
char *relpath;
{
	char dirname[MAXPATH+4];	/* allow for "/./" or "././" prefix */

	if ( cpdirname( dirname, relpath ) == dirname ) {
		dirname[0] = '.';
		dirname[1] = '\0';
	}

	return( chkperm( dirname, P_WRITE|P_EXEC, geteuid(), getegid() ) );
}
