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
extern struct utuple **newsupertpl();

int
initresult( result )
struct qresult *result;
{
	struct joinblock *blkptr;
	struct joinblock *nextblk;

	if ( result->snode != NULL ) {
		/*
		 * Initialize the current tuple.
		 */
		blkptr = result->snode->joinptr;
		if ( result->query->flags & Q_SORT ) {
			/*
			 * Set all the current tuple indexes to zero.
			 * Also sort the blocks by the next tuple in
			 * each block.
			 */
			result->curblk = NULL;
			for( ; blkptr != NULL; blkptr = nextblk )
			{
				blkptr->curtpl = -result->query->nodecnt;
				nextblk = blkptr->next;
				sort_nexttpl( result, &result->curblk,
					blkptr );
			}
		}
		else {
			/* Just initialize first block */
			result->curblk = blkptr;
			blkptr->curtpl = -result->query->nodecnt;

			if ( result->query->flags & Q_UNIQUE ) {
				/*
				 * Initialize the data needed for checking
				 * uniqueness of each tuple.
				 */
				if ( ! init_unique( result ) ) {
					result->curblk = NULL;
					return( FALSE );
				}
			}
		}

		/* we have tuples if they're in a file or the first block */

		return( (result->flags & QR_INFILE) != 0 ||
				result->curblk->blkcnt != 0 );
	}
	else {
		result->curblk = NULL;
		return( FALSE );
	}
}

static int
next_sorted( result )
struct qresult *result;
{
	struct joinblock *blkptr;
	struct utuple **prevtpl;	/* previous super tuple */

	/*
	 * IMPORTANT:  All tuples are assumed to be in memory
	 * at this point.  Additionally, each join block's
	 * super tuples are sorted within the block and the
	 * join blocks are sorted by what would be the next
	 * tuple in each block.
	 *
	 * The basic algorithm here is to bubble the current block
	 * down into the sorted list of blocks, with respect to th+e
	 * next tuple in each block.  Then increment to the next
	 * tuple in the first block in the list.
	 */

	blkptr = result->curblk;
	/*
	 * See if uniqueness needs to be check and if there is
	 * a previous tuple.  We'll later use the previous tuple
	 * and compare it to the next tuple we would return to
	 * make sure they're not the same.
	 */
	prevtpl = ( (result->query->flags & Q_UNIQUE) &&
				blkptr->curtpl >= 0 ) ?
			&blkptr->tuples[blkptr->curtpl] :
			NULL;

	/*
	 * Move to the next tuple and the bubble the next
	 * tuple down in the list of blocks.  We stay in this
	 * loop when we're checking uniqueness until we find
	 * a unique tuple (i.e., the current tuple and any
	 * previous tuple are not the same).
	 */
	do {
		/* remove current block from list */
		result->curblk = blkptr->next;
		/* bubble block down */
		sort_nexttpl( result, &result->curblk, blkptr );

		/* result->curblk is now set to sorted next tuple */
		blkptr = result->curblk;
		blkptr->curtpl += result->query->nodecnt;
		if ( blkptr->curtpl >= blkptr->blkcnt )
			return( FALSE );	/* no more tuples */

	} while( prevtpl != NULL &&
		! cmp_unique( result, prevtpl,
			&blkptr->tuples[blkptr->curtpl] ) );

	return( TRUE );
}

static int
next_unsorted( result )
struct qresult *result;
{
	register int i, all_in_file;
	struct utuple **newstpl;
	struct qnode *nodeptr;
	struct joinblock *blkptr;

	/*
	 * Increment the in-file tuples and check if all relations
	 * are in file.
	 */
	all_in_file = ( result->flags & QR_INFILE );
	if ( all_in_file ) {	
		for( i = result->query->nodecnt - 1; i >= 0; i-- ) {
			nodeptr = result->query->nodelist[i];
			if ( nodeptr->snode != result->snode )
				continue;

			if ( (nodeptr->flags & N_INFILE) == 0 ) {
				all_in_file = FALSE;
				continue;
			}

			/*
			 * This node is still in a file.  So read in the
			 * next tuple.  If there isn't one, then reset the
			 * node and get the first tuple again.
			 */
			if ( (nodeptr->flags & N_OPENED) == 0 )
			{
				if ( ! relreset( nodeptr ) ||
					( nodeptr->tmptpl == NULL &&
					(nodeptr->tmptpl = newtuple( nodeptr )) == NULL ) )
				{
					return( FALSE );
				}
			}

			if ( readtuple( nodeptr->rel, &nodeptr->relio,
					nodeptr->tmptpl, NULL ) )
			{
				/*
				 * We got the next tuple from this file.
				 * There's no need to increment the curblk
				 * or curtpl, unless we're on the first
				 * tuple.  In this latter case we must
				 * continue and get the initial tuples of
				 * all in-file relations and then get the
				 * fall through to get the first super
				 * tuple of the in-memory relations.
				 */
				if ( result->curblk->curtpl < 0 )
					continue;	/* first tuple */

				result->curblk->tuples[result->curblk->curtpl + nodeptr->nodenum] = nodeptr->tmptpl;
				return( TRUE );
			}
			else {
				extern	int	uchecktuple;
					int	uchecksave;
				/*
				 * Normal termination or termination due to error?
				 */
				if ( uerror != UE_NOERR ) {
					(void)relclose( nodeptr, FALSE );
					return( FALSE );
				}

				/*
				 * No more tuples in the file.  So reset the
				 * relation, get the first tuple, and continue
				 * on with any other nodes still in a file.
				 */
				if ( ! relclose( nodeptr, FALSE ) )
					return( FALSE );	/* unpack error */

				/*
				 * Ignore (1st) tuple syntax errors
				 * that have already been reported.
				 */
				if ( ( uchecksave = uchecktuple )  != UCHECKFATAL )
					uchecktuple = UCHECKIGNORE;

				if ( ! relreset( nodeptr ) ||
					! readtuple( nodeptr->rel,
						&nodeptr->relio,
						nodeptr->tmptpl, NULL ) ) {
					uchecktuple = uchecksave;
					return( FALSE );
				}

				/* restore uchecktuple to its original state */
				uchecktuple = uchecksave;
			}
		}
	}

	blkptr = result->curblk;

	/*
	 * If this is the first tuple we've looked at and all relations
	 * are still in files, then we need to allocate a super tuple
	 * for use in putting together the projected tuples.
	 */
	if ( all_in_file && blkptr->curtpl < 0 ) {
		if ( newsupertpl( blkptr, result->query ) == NULL )
			return( FALSE );
	}

	/*
	 * All the in file nodes were reset (or not present).
	 * Move the current block and current tuple down,
	 * and reset the in file tuples.
	 */
	blkptr->curtpl += result->query->nodecnt;
	while( blkptr->curtpl >= blkptr->blkcnt )
	{
		blkptr = blkptr->next;
		result->curblk = blkptr;
		if ( blkptr == NULL ) {		/* all done */
			/*
			 * Close all the in file relations
			 */
			for( i = 0; i < result->query->nodecnt; i++ ) {
				if ( result->query->nodelist[i]->flags & N_INFILE )
					(void) relclose( result->query->nodelist[i], FALSE );
			}
			return( FALSE );
		}
		blkptr->curtpl = 0;
	}

	if ( result->flags & QR_INFILE ) {
		newstpl = &blkptr->tuples[blkptr->curtpl];
		for( i = 0; i < result->query->nodecnt; i++ ) {
			nodeptr = result->query->nodelist[i];
			if ( nodeptr->snode != result->snode )
				continue;

			if ( nodeptr->flags & N_INFILE )
				newstpl[nodeptr->nodenum] = nodeptr->tmptpl;
		}
	}

	return( TRUE );
}

/*ARGSUSED*/
nexttplptr( dummy, result )
struct uquery *dummy;	/* not used -- only for backward compatability */
struct qresult *result;
{
	if ( result->snode == NULL || result->curblk == NULL )
		return( FALSE );

	if ( result->query->flags & Q_SORT ) {	/* will check uniqueness */
		return( next_sorted( result ) );
	}
	else if ( result->query->flags & Q_UNIQUE ) {

		reset_uerror( );

		/* return next unique tuple */
		do {
			if ( ! next_unsorted( result ) ) {

				end_unique( result );
				return( FALSE );
			}
		} while( ! unique_tuple( result ) );
	
		return( uerror == UE_NOERR ? TRUE : FALSE );
	}
	else {
		return( next_unsorted( result ) );
	}
}

#define MAXTPLNUM	(MAXRELATION * 8)

setattrvals( result, attrvals, display_only )
struct qresult *result;
register char **attrvals;
char display_only;	/* get all attributes or just displayed? */
{
	register int i;
	register struct qprojection *refptr;
	register struct utuple **stplptr;	/* super-tuple from joins */
	struct uquery *query;
	static short numcnt;
	static char tuplenums[MAXTPLNUM][12];

	if ( result->snode == NULL || result->curblk == NULL ||
			result->curblk->curtpl < 0 )
		return( FALSE );

	/*
	 * Set up the attribute values in the order they're given in the
	 * projection list.
	 */
	stplptr = &result->curblk->tuples[result->curblk->curtpl];
	query = result->query;

	for( refptr = query->attrlist, i = 0; i < query->attrcnt;
			i++, refptr++ ) {

		if ( display_only && (refptr->flags & QP_NODISPLAY) )
			continue;	/* attribute not displayed */

		if ( ( refptr->flags & QP_NEWVALUE ) && ( refptr->attorval ) ) {
			*attrvals = refptr->attorval;
			++attrvals;
			continue;
		}

		switch( refptr->attr ) {
		case ATTR_RECNUM:
			if ( display_only ) {
				sprintf( tuplenums[numcnt], "%lu",
					stplptr[ refptr->rel->nodenum ]->tuplenum );
				*attrvals++ = tuplenums[numcnt];
				numcnt = (numcnt + 1) % MAXTPLNUM;
			}
			else {
				*attrvals++ = ""; /* not needed */
			}
			break;
		case ATTR_SEEK:
			if ( display_only ) {
				sprintf( tuplenums[numcnt], "%lu",
					stplptr[ refptr->rel->nodenum ]->lseek );
				*attrvals++ = tuplenums[numcnt];
				numcnt = (numcnt + 1) % MAXTPLNUM;
			}
			else {
				*attrvals++ = ""; /* not needed */
			}
			break;
		default:	/* normal attribute */
			*attrvals = stplptr[ refptr->rel->nodenum ]->tplval[refptr->attr];
			++attrvals;
			break;
		}
	}

	return( TRUE );
}

/*ARGSUSED*/
getattrvals( dummy, result, attrvals )
struct uquery *dummy;	/* not used -- only for backward compatability */
struct qresult *result;
char **attrvals;
{
	return( setattrvals( result, attrvals, TRUE ) );
}

/*ARGSUSED*/
nexttuple( dummy, result, attrvals )
struct uquery *dummy;	/* not used -- only for backward compatability */
struct qresult *result;
char **attrvals;
{
	if ( ! nexttplptr( (struct uquery *)NULL, result ) )
		return( FALSE );

	return( setattrvals( result, attrvals, TRUE ) );
}
