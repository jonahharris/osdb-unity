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

static long tplcnt;

/*ARGSUSED*/
static int
cntfunc( attrvals, projcnt, projptr )
char **attrvals;
int projcnt;
struct qprojtuple *projptr;
{
	tplcnt++;

	return( TRUE );
}

int
cnttuples( query )
struct uquery *query;
{
	struct qresult result;
	int (*oldfunc)();

	oldfunc = query->tplfunc;

	settplfunc( query, cntfunc );

	tplcnt = 0;
	if ( ! queryeval( query, &result ) )
		return( -1 );

	settplfunc( query, oldfunc );

	return( tplcnt );
}
