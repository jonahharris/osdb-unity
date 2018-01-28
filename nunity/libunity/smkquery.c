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
#include "message.h"

extern struct uquery *fmkquery();

#define MAXWHERE	50

struct uquery *
smkquery( flags, relstr, attrstr, sortstr, wherestr )
int flags;
char *relstr;
char *attrstr;
char *sortstr;
char *wherestr;
{
	int relcnt;
	int attrcnt;
	int sortcnt;
	int wherecnt;
	char *func = "smkquery";
	char *delim = " \t\n\r";
	char rc = TRUE;
	struct uquery *query;
	char **wherelist = NULL;
	char *wherebuf[ MAXWHERE ];
	char *rellist[ MAXRELATION ];
	char *attrlist[ MAXATT ];

	relcnt = cntsflds( relstr, delim );
	if ( relcnt > MAXRELATION )
	{
		prmsg( MSG_ERROR, "(%s) too many relations specified (%d) -- max is %d",
			func, relcnt, MAXRELATION );
		rc = FALSE;
	}

	if ( attrstr == NULL || *attrstr == '\0' )
		attrcnt = 0;
	else
	{
		attrcnt = cntsflds( attrstr, delim );
	}

	sortcnt = cntsflds( sortstr, delim );

	if ( sortcnt + attrcnt > MAXATT )
	{
		prmsg( MSG_ERROR, "(%s) too many attributes projected and/or sorted on (%d) -- max is %d",
			func, sortcnt + attrcnt, MAXATT );
		rc = FALSE;
	}

	if ( ! rc )
		return( NULL );

	if ( wherestr == NULL || *wherestr == '\0' )
	{
		wherecnt = 0;
		wherelist = wherebuf;
	}
	else
	{
		wherecnt = cntsflds( wherestr, delim );
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
	}

	relcnt = splitsflds( rellist, MAXRELATION, relstr, delim );
	if ( attrcnt > 0 )
		attrcnt = splitsflds( attrlist, MAXATT, attrstr, delim );
	if ( sortcnt > 0 )
		sortcnt = splitsflds( &attrlist[ attrcnt ], MAXATT - attrcnt,
					sortstr, delim );
	if ( wherecnt > 0 )
		wherecnt = splitsflds( wherelist, wherecnt, wherestr, delim );

	query = fmkquery( flags, rellist, relcnt, attrlist, attrcnt,
			&attrlist[ attrcnt ], sortcnt, wherelist, wherecnt );

	if ( wherelist != wherebuf )
		free( wherelist );

	return( query );
}
