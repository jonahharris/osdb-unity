/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdarg.h>
#include "uquery.h"
#include "message.h"

extern struct uquery *fmkquery();

#define MAXWHERE	50

/*VARARGS4*/
struct uquery *
vmkquery( int flags, int relcnt, int attrcnt, int sortcnt, int wherecnt, ... )
{
	va_list args;
	int i;
	char *func = "vmkquery";
	char rc = TRUE;
	struct uquery *query;
	char **wherelist = NULL;
	char *wherebuf[ MAXWHERE ];
	char *rellist[ MAXRELATION ];
	char *attrlist[ MAXATT ];

	va_start( args, wherecnt );

	if ( relcnt > MAXRELATION )
	{
		prmsg( MSG_ERROR, "(%s) too many relations specified (%d) -- max is %d",
			func, relcnt, MAXRELATION );
		rc = FALSE;
	}

	if ( sortcnt + attrcnt > MAXATT )
	{
		prmsg( MSG_ERROR, "(%s) too many attributes projected and/or sorted on (%d) -- max is %d",
			func, sortcnt + attrcnt, MAXATT );
		rc = FALSE;
	}

	if ( ! rc )
		return( NULL );

	if ( wherecnt > MAXWHERE )
	{
		/* allocate the space */
		wherelist = (char **)malloc( wherecnt * sizeof( char * ) );
		if ( wherelist == NULL )
		{
			prmsg( MSG_INTERNAL, "(%s) cannot allocate space for where-clause",
				func );
			return( NULL );
		}
	}
	else
		wherelist = wherebuf;

	for( i = 0; i < relcnt; i++ )
	{
		rellist[i] = va_arg( args, char * );
	}
	for( i = 0; i < attrcnt; i++ )
	{
		attrlist[i] = va_arg( args, char * );
	}
	for( i = 0; i < sortcnt; i++ )
	{
		attrlist[ attrcnt + i ] = va_arg( args, char * );
	}
	for( i = 0; i < wherecnt; i++ )
	{
		wherelist[i] = va_arg( args, char * );
	}

	query = fmkquery( flags, rellist, relcnt, attrlist, attrcnt,
			&attrlist[ attrcnt ], sortcnt, wherelist, wherecnt );

	if ( wherelist != wherebuf )
		free( wherelist );

	return( query );
}
