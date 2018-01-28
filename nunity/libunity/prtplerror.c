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
#include "message.h"
#include "urelation.h"

long	utplelimit;	/* tuple error message limit -
			 * ignored if utplmsgtype not set  */

int	utplmsgtype;	/* MSG_ERROR or MSG_WARN (default) */

int
prtplerror( relptr, tplptr )
struct urelation *relptr;
struct utuple *tplptr;
{
	int	msgtype;

	if ( (tplptr->flags & TPL_ERRORMSK) == 0 )
		return( FALSE );

	if ( utplmsgtype ) {
		if ( utplmsgtype == MSG_ERROR ) {
			msgtype = MSG_ERROR;
		} else {
			msgtype = MSG_WARN;
		}
		if ( utplelimit == 0 ) {
			/*
			 * tuple error report limit has
			 * already been reached so clear
			 * the error bit(s) and return.
			 */
			tplptr->flags &= ~TPL_ERRORMSK;
			return( TRUE );
		}
		/*
		 * decrement error report limit/count
		 */
		--utplelimit;
	} else {
		msgtype = MSG_WARN;
	}

	/*
	 * This first message is the only one that we can continue
	 * reading the tuple when it occurs.  So we try and figure out
	 * where the error occured in the tuple.
	 */
	if ( tplptr->flags & TPL_SYNEMBEDNL )
	{
		unsigned int badattr;
		char *attrname;

		badattr = tplptr->badattr;
		if ( tplptr->badattr == TPL_NOBADATTR )
		{
			attrname = "**unknown**";
			badattr = -1;
		}
		else
		{
			attrname = relptr->attrs[badattr].aname;
			badattr = tplptr->badattr;
		}

		prmsg( msgtype, "relation %s, tuple #%lu:\n\tattribute %s (#%d) contains embedded new-line,\n\ttuple ended at that point",
			relptr->path, tplptr->tuplenum, attrname,
			badattr + 1 );
	}
	else
	if ( tplptr->flags & TPL_SYNFWNONL )
		prmsg( msgtype, "relation %s, tuple #%lu:\n\tfinal attribute %s (#%u) is fixed-width,\n\tbut not followed by a new-line",
			relptr->path, tplptr->tuplenum,
			relptr->attrs[relptr->attrcnt - 1].aname,
			relptr->attrcnt );

	if ( tplptr->flags & TPL_SYNPREMEOF )
		prmsg( msgtype, "relation %s, tuple #%lu:\n\tpremature end-of-file encountered while reading tuple",
			relptr->path, tplptr->tuplenum );

	if ( tplptr->flags & TPL_ERRBIGATT )
		prmsg( MSG_ERROR, "relation %s, tuple #%lu:\n\tattribute value is longer than maximum I/O block size (%u),\n\tattribute truncated to fit",
			relptr->path, tplptr->tuplenum, DBBLKSIZE );

	if ( tplptr->flags & TPL_ERRREAD )
		prmsg( msgtype == MSG_ERROR ? MSG_ERROR : MSG_ALERT,
			"relation %s, tuple #%lu:\n\tread failure occurred while reading tuple",
			relptr->path, tplptr->tuplenum );

	if ( tplptr->flags & TPL_ERRNOMEM )
		prmsg( MSG_INTERNAL, "relation %s, tuple #%lu:\n\tunable to allocate memory while reading tuple",
			relptr->path, tplptr->tuplenum );

	if ( tplptr->flags & TPL_ERRUNKNOWN )
		prmsg( MSG_INTERNAL, "relation %s, tuple #%lu:\n\tunknown error occured while reading tuple",
			relptr->path, tplptr->tuplenum );

	/*
	 * Now get rid of the error flags so the error is
	 * only reported once.
	 */
	tplptr->flags &= ~TPL_ERRORMSK;

	return( TRUE );
}
