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

extern struct utuple *gettuple();

long
blkjoin( query, jinfo, operptr )
struct uquery *query;
struct joininfo *jinfo;
struct uqoperation *operptr;
{
	register struct utuple *tpl1, *tpl2;
	register int rc;
	register long joincnt, delcnt;

	rc = TRUE;
	joincnt = 0L;
	delcnt = 0L;

	if ( ! relreset( operptr->node2 ) )
		return( -1L );

	while( (tpl2 = gettuple( operptr->node2 )) != NULL ) {
		tpl2->flags &= ~(TPL_DELETE|TPL_JOINED);

		if ( ! relreset( operptr->node1 ) ) {
			rc = FALSE;
			break;
		}
		while( rc && (tpl1 = gettuple( operptr->node1 )) != NULL ) {

			if ( chkjoinlist( operptr->cmplist, operptr->cmpcnt,
					operptr->node1, operptr->node2,
					tpl1, tpl2 ) ) {
				tpl1->flags |= TPL_JOINED;
				tpl2->flags |= TPL_JOINED;
				joincnt++;
				if ( (*jinfo->joinfunc)( query, jinfo->snode,
						operptr->node1, operptr->node2,
						tpl1, tpl2 ) <= 0 )
					rc = FALSE;
			}
			if ( ! addtuple( query, operptr->node1, tpl1 ) )
				rc = FALSE;
		}

		if ( ! rc )
			break;
		else if ( uerror != UE_NOERR ) {	/* unpack error */
			rc = FALSE;
			break;
		}

		if ( (tpl2->flags & TPL_JOINED) == TPL_JOINED ||
				(tpl2->flags & TPL_DELETE) == 0 ) {
			tpl2->flags &= ~TPL_JOINED;
			if ( (*jinfo->addfunc2)( query,
					operptr->node2, tpl2 ) <= 0 ) {
				rc = FALSE;
				break;
			}
		}
		else {
			delcnt++;
			if ( (*jinfo->delfunc2)( query,
				operptr->node2, tpl2, jinfo, operptr ) <= 0 )
			{
				rc = FALSE;
				break;
			}
		}
	}

	if ( ( ! rc ) || ( uerror != UE_NOERR ) )	/* unpack error */
		return( -1L );

	/*
	 * Clean up node1 and delete any tuples that did not
	 * participate in the join.
	 */
	if ( ! relreset( operptr->node1 ) )
		return( -1L );
	while( (tpl1 = gettuple( operptr->node1 )) != NULL ) {
		if ( (tpl1->flags & TPL_JOINED) == TPL_JOINED ||
				(tpl1->flags & TPL_DELETE) == 0 ) {

			tpl1->flags &= ~(TPL_DELETE|TPL_JOINED);
			if ( (*jinfo->addfunc1)( query,
					operptr->node1, tpl1 ) <= 0 )
			{
				rc = FALSE;
				break;
			}
		}
		else {
			delcnt++;
			if ( (*jinfo->delfunc1)( query,
				operptr->node1, tpl1, jinfo, operptr ) <= 0 )
			{
				rc = FALSE;
				break;
			}
		}
	}

	/*
	 * Decide what to return.  If there was an error, return -1;
	 * otherwise, return the appropriate cnt.
	 */
	if ( ( ! rc ) || ( uerror != UE_NOERR ) )	/* unpack error */
		return( -1L );

	switch( operptr->oper ) {
	case UQOP_JOIN:
		return( joincnt );
	case UQOP_OUTERJOIN:
		return( joincnt + delcnt );
	case UQOP_ANTIJOIN:
		return( delcnt );
	default:
		return( -1L );
	}
}
