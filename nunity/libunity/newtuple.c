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
#include "uquery.h"
#include "uerror.h"
#include "message.h"

extern char *calloc();

#define TPLPAGESIZE	146	/* so a page is close to 4096 */
#define ATTRPAGESIZE	1024	/* so a page is close to 4096 */

struct utuplepage {
	struct utuplepage *next;
	struct utuple entries[TPLPAGESIZE];
};

struct attrpage {
	struct attrpage *next;
	char *entries[ATTRPAGESIZE];
};

static struct utuple *tfreelist, *tallocblk;
static short talloccnt;
static struct utuplepage *tpagelist;
static char **atfreelist, **atallocblk;
static short atalloccnt;
static short attrcnt = MAXATT;	/* num. of attrs allocated per tuple */
static struct attrpage *atpagelist;


struct utuple *
newtuple( nodeptr )
struct qnode *nodeptr;
{
	register struct utuple *tplptr;

	if ( nodeptr->rel->attrcnt > attrcnt ) {
		set_uerror( UE_NUMATTR );
		return( NULL );
	}

	if ( tfreelist ) {
		tplptr = tfreelist;
		tfreelist = (struct utuple *)tfreelist->tplval;
		tplptr->refcnt = 0;
		tplptr->flags = 0;
	}
	else if ( talloccnt-- > 0 )
		tplptr = tallocblk++;
	else {
		register struct utuplepage *tpgptr;

		tpgptr = (struct utuplepage *)calloc( 1, sizeof( struct utuplepage ) );
		if ( tpgptr == NULL ) {
			set_uerror( UE_NOMEM );
			return( NULL );
		}
		if ( tpagelist )
			tpgptr->next = tpagelist->next;
		tpagelist = tpgptr;

		tallocblk = tpgptr->entries;
		tplptr = tallocblk++;
		talloccnt = TPLPAGESIZE - 1;
	}

	tplptr->lseek = -1;
	tplptr->tuplenum = -1;

	if ( atfreelist ) {
		tplptr->tplval = atfreelist;
		atfreelist = (char **)(*atfreelist);
	}
	else if ( atalloccnt-- > 0 ) {
		tplptr->tplval = atallocblk;
		atallocblk += attrcnt;
	}
	else {
		register struct attrpage *atpgptr;

		atpgptr = (struct attrpage *)calloc( 1, sizeof( struct attrpage ) );
		if ( atpgptr == NULL ) {
			tplptr->tplval = NULL;
			freetuple( tplptr );
			set_uerror( UE_NOMEM );
			return( NULL );
		}

		if ( atpagelist )
			atpgptr->next = atpagelist->next;
		atpagelist = atpgptr;

		tplptr->tplval = atpgptr->entries;
		atallocblk = &atpgptr->entries[attrcnt];
		atalloccnt = (ATTRPAGESIZE / attrcnt) - 1;
	}

	return( tplptr );
}

get_attralloc( )
{
	return( attrcnt );
}

set_attralloc( cnt )
int cnt;
{
	if ( inquery() || cnt < 1 || cnt > MAXATT )
		return( FALSE );

	if ( cnt > attrcnt )
		(void)_freealltuples( );

	attrcnt = cnt;

	return( TRUE );
}

extern struct utuple _unulltpl;		/* from outerjoin.c */

freetuple( tplptr )
register struct utuple *tplptr;
{
#if 0
	if ( tplptr == NULL || tplptr == &_unulltpl )
		return;
#endif

	if ( tplptr->flags & TPL_FREED ) {
#ifdef DEBUG
		prmsg( MSG_INTERNAL, "freed tuple twice" );
#endif
		return;		/* somebody freed the tuple twice */
	}

	if ( tplptr->tplval ) {
		*tplptr->tplval = (char *)atfreelist;
		atfreelist = tplptr->tplval;
	}

	tplptr->tplval = (char **)tfreelist;
	tfreelist = tplptr;
	tplptr->flags = TPL_FREED;
}

_freealltuples( )
{
	register struct utuplepage *nexttpg;
	register struct attrpage *nextatpg;

	if ( inquery() )
		return( FALSE );

	talloccnt = 0;
	tallocblk = tfreelist = NULL;
	while( tpagelist ) {
		nexttpg = tpagelist;
		tpagelist = tpagelist->next;
		free( nexttpg );
	}

	atalloccnt = 0;
	atallocblk = atfreelist = NULL;
	while( atpagelist ) {
		nextatpg = atpagelist;
		atpagelist = atpagelist->next;
		free( nextatpg );
	}

	return( TRUE );
}
