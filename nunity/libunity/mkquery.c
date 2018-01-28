/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "uquery.h"

extern struct uquery *fmkquery( );

struct uquery *
mkquery( relnames, relcnt, attrnames, anamecnt, where, wherecnt )
char **relnames;
int relcnt;
char **attrnames;
int anamecnt;
char **where;
int wherecnt;
{
	return( fmkquery( 0, relnames, relcnt, attrnames, anamecnt,
			NULL, 0, where, wherecnt ) );
}
