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
#include "uerror.h"

extern char *calloc();

#define	EXPRPAGESIZE	36

struct exprpage {
	struct exprpage *next;
	struct queryexpr entries[EXPRPAGESIZE];
};

static struct queryexpr *freelist, *allocblk;
static short alloccnt;
static struct exprpage *pagelist;

struct queryexpr *
newqexpr( )
{
	register struct queryexpr *qptr;
	struct exprpage *epgptr;

	if ( freelist ) {
		qptr = freelist;
		freelist = qptr->elem1.expr;
	}
	else if ( alloccnt-- > 0 )
		qptr = allocblk++;
	else {
		if ( (epgptr = (struct exprpage *)calloc( 1, sizeof( struct exprpage ) ) ) == NULL ) {
			set_uerror( UE_NOMEM );
			return( NULL );
		}
			
		if ( pagelist )
			epgptr->next = pagelist->next;
		pagelist = epgptr;

		allocblk = epgptr->entries;
		qptr = allocblk++;
		alloccnt = EXPRPAGESIZE - 1;
	}

	return( qptr );
}

_freeallqexpr( )
{
	struct exprpage *next;

	if ( inquery() )
		return( FALSE );

	alloccnt = 0;
	allocblk = freelist = NULL;
	while( pagelist ) {
		next = pagelist;
		pagelist = pagelist->next;
		free( next );
	}

	return( TRUE );
}

freeqexpr( qptr )
register struct queryexpr *qptr;
{
	if ( ISSETCMP( qptr->optype ) )
	{
		/*
		 * Must free attribute list and string lists in
		 * set comparisons
		 */
		free( qptr->elem1.alist.list );
		free( qptr->elem2.strlist[0] );
		free( qptr->elem2.strlist );
	}

	qptr->elem1.expr = freelist;
	freelist = qptr;

}

freeqexprtree( qptr )
register struct queryexpr *qptr;
{
	register struct queryexpr *oldqptr;

	while( ISBOOL( qptr->optype ) ) {
		freeqexprtree( qptr->elem1.expr );
		oldqptr = qptr;
		qptr = qptr->elem2.expr;
		freeqexpr( oldqptr );
	}

	freeqexpr( qptr );
}
