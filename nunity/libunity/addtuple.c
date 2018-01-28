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

#define TUPLEPAGESIZE	8

struct utuplepage {
	struct utuplepage *next;
	struct utupleblk entries[TUPLEPAGESIZE];
};

static struct utupleblk *freelist, *allocblk;
static short alloccnt;
static struct utuplepage *pagelist;

static
tplblkfree( blkptr )
register struct utupleblk *blkptr;
{
	register struct utupleblk *endblk;

	for( endblk = freelist; endblk; endblk = endblk->next ) {
		if ( endblk == blkptr ) {
#ifdef DEBUG
			prmsg( MSG_INTERNAL, "free'd tuple block twice" );
#endif
			return( TRUE );
		}
	}

	return( FALSE );
}

freetplblk( blkptr )
struct utupleblk *blkptr;
{
	if ( blkptr == NULL || tplblkfree( blkptr ) )
		return;

	blkptr->next = freelist;
	freelist = blkptr;
}

freetplblks( blkptr, dotuples )
struct utupleblk *blkptr;
int dotuples;
{
	register struct utupleblk *endblk;
	register int i;

	if ( blkptr == NULL || tplblkfree( blkptr ) )
		return;

	if ( dotuples ) {
		endblk = blkptr;
		while( 1 ) {
			for( i = 0; i < endblk->tuplecnt; i++ )
				freetuple( endblk->tuples[i] );
			if ( endblk->next == NULL )
				break;
			endblk = endblk->next;
		}
	}
	else
		for( endblk = blkptr; endblk->next; endblk = endblk->next )
			;

	endblk->next = freelist;
	freelist = blkptr;
}

_freealltplblks( )
{
	struct utuplepage *next;

	if ( inquery () )
		return( FALSE );

	alloccnt = 0;
	freelist = allocblk = NULL;
	while( pagelist ) {
		next = pagelist;
		pagelist = pagelist->next;
		free( next );
	}

	return( TRUE );
}

static struct utupleblk *
newtupleblk( prev )
struct utupleblk **prev;
{
	struct utupleblk *blkptr;

	if ( freelist ) {
		blkptr = freelist;
		freelist = freelist->next;
		blkptr->tuplecnt = 0;
		blkptr->next = NULL;
	}
	else if ( alloccnt-- > 0 )
		blkptr = allocblk++;
	else {
		register struct utuplepage *pgptr;

		pgptr = (struct utuplepage *)calloc( 1, sizeof( struct utuplepage));
		if ( pgptr == NULL ) {
			set_uerror( UE_NOMEM );
			return( NULL );
		}

		if ( pagelist )
			pgptr->next = pagelist->next;
		pagelist = pgptr;

		blkptr = pgptr->entries;
		allocblk = &pgptr->entries[1];
		alloccnt = TUPLEPAGESIZE - 1;
	}

	if ( prev )
		*prev = blkptr;

	return( blkptr );
}

static
alloctuple( prev, tplptr )
struct utupleblk **prev;
struct utuple *tplptr;
{
	register struct utupleblk *blkptr;

	for( blkptr = *prev; blkptr && blkptr->tuplecnt >= TPLBLKSIZE;
			blkptr = blkptr->next )
		prev = &blkptr->next;

	if ( blkptr == NULL && (blkptr = newtupleblk( prev )) == NULL )
		return( FALSE );

	blkptr->tuples[blkptr->tuplecnt++] = tplptr;

	return( TRUE );
}

addtupleref( prev, tplptr )
struct utupleblk **prev;
struct utuple *tplptr;
{
	if ( alloctuple( prev, tplptr ) ) {
		tplptr->refcnt++;

		return( TRUE );
	}
	else
		return( FALSE );
}

struct utuple *
findtuple( nodeptr, seekval )
struct qnode *nodeptr;
register long seekval;
{
	register struct utupleblk *blkptr;
	register int i;

	for( blkptr = nodeptr->tuples; blkptr; blkptr = blkptr->next ) {
		for( i = 0; i < blkptr->tuplecnt; i++ ) {
			if ( blkptr->tuples[i]->lseek == seekval )
				return( blkptr->tuples[i] );
		}
	}

	return( NULL );
}

findtplptr( nodeptr, tplptr )
struct qnode *nodeptr;
register struct utuple *tplptr;
{
	register struct utupleblk *blkptr;
	register int i;

	for( blkptr = nodeptr->tuples; blkptr; blkptr = blkptr->next ) {
		for( i = 0; i < blkptr->tuplecnt; i++ ) {
			if ( blkptr->tuples[i] == tplptr )
				return( TRUE );
		}
	}

	return( FALSE );
}

/*ARGSUSED*/
addtuple( dummy, nodeptr, tplptr )
struct uquery *dummy;	/* not used */
struct qnode *nodeptr;
struct utuple *tplptr;
{
	/*
	 * If the tuple is not from a file, but rather in memory, just go
	 * on.  However, if the force save flag is set, do it any way.
	 * This is the case where all tuples are read in before any are
	 * added to the node (e.g., hash joins).
	 */
	if ( nodeptr->relio.fd < 0 &&
			( (nodeptr->flags & N_FORCESAVE) == 0 ||
			findtplptr( nodeptr, tplptr ) ) )
		return( TRUE );

	if ( alloctuple( &nodeptr->tuples, tplptr ) ) {
		/*
		 * Don't count this tuple in the node if it's ignored.
		 */
		if ( (tplptr->flags & TPL_IGNORE) == 0 ) {
			nodeptr->tuplecnt++;
			nodeptr->memsize += tplptr->tplsize;
		}
		return( TRUE );
	}
	else
		return( FALSE );
}
