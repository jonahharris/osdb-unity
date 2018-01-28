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


extern struct utuple *newtuple();
extern struct urelblock *readioblk();
extern long lseek();

struct utuple *
seektuple( nodeptr, seekval )
register struct qnode *nodeptr;
register long seekval;
{
	register struct urelio *ioptr;
	register struct urelblock *blkptr;
	struct urelblock *prevblk;
	struct utuple *tplptr;
	long blkseekval;

	/*
	 * We have a real on-disk relation, but we're doing things in
	 * random order.
	 */
	ioptr = &nodeptr->relio;
	if ( ioptr->fd < 0 ) {
		set_uerror( UE_NOINIT );
		return( NULL );
	}

	ioptr->flags &= ~UIO_EOF;

	prevblk = NULL;
	for( blkptr = ioptr->readbuf; blkptr; blkptr = blkptr->next ) {
		if ( blkptr->seekval <= seekval &&
				seekval < blkptr->seekval + (blkptr->end - blkptr->data) ) {
		}
		prevblk = blkptr;
	}

	if ( blkptr == NULL ) {		/* block not found */
		blkseekval = ( seekval / DBBLKSIZE ) * DBBLKSIZE;
		if ( lseek( ioptr->fd, blkseekval, 0 ) < 0L ) {
			set_uerror( UE_READ );
			return( NULL );
		}
		if ( (blkptr = readioblk( ioptr, blkseekval )) == NULL )
			return( NULL );
		if ( prevblk ) {
			blkptr->next = prevblk->next;
			prevblk->next = blkptr;
		}
		else {
			blkptr->next = ioptr->readbuf;
			ioptr->readbuf = blkptr;
		}
	}

	ioptr->curblk = blkptr;
	blkptr->curloc = blkptr->data + (seekval - blkptr->seekval);

	if ( (tplptr = newtuple( nodeptr )) == NULL )
		return( NULL );

	if ( ! readtuple( nodeptr->rel, ioptr, tplptr, nodeptr->memattr ) ) {
		freetuple( tplptr );
		return( NULL );
	}

	tplptr->tuplenum = -1;	/* don't have a legal tuple number */

	return( tplptr );
}
