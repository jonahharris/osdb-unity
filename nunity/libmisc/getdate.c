/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <time.h>

ugetdate( buf )
char *buf;
{
	register struct tm *tmptr;
	long clock;

	(void)time( &clock );
	tmptr = localtime( &clock );

	sprintf( buf, "%02d/%02d/%02d %02d:%02d:%02d",
		tmptr->tm_year,
		tmptr->tm_mon + 1,
		tmptr->tm_mday,
		tmptr->tm_hour,
		tmptr->tm_min,
		tmptr->tm_sec );
}
