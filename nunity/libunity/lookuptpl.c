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
#include "uindex.h"

extern char *calloc();

extern struct utuple *seektuple();
extern void unsaveelse();

/*
 * Structure of seek values matching tuples for a particular node.
 */ 
#define LOCBLKSIZE	126	/* so we get nice size allocations */

struct locationblk {
	short blkcnt;
	struct locationblk *next;
	long entries[LOCBLKSIZE];
};

#define LOCPAGESIZE	8

struct locationpage {
	struct locationpage *next;
	struct locationblk entries[LOCPAGESIZE];
};

static struct locationblk *freelist, *allocblk;
static short alloccnt;
static struct locationpage *pagelist;

_freealllocblks()
{
	struct locationpage *pgptr;

	if ( inquery() )
		return( FALSE );

	alloccnt = 0;
	allocblk = freelist = NULL;
	while( pagelist ) {
		pgptr = pagelist;
		pagelist = pagelist->next;
		free( pgptr );
	}

	return( TRUE );
}

static
freelocblk( blkptr )
register struct locationblk *blkptr;
{
	if ( blkptr ) {
		blkptr->next = freelist;
		freelist = blkptr;
	}
}

static
freelocblks( blkptr )
register struct locationblk *blkptr;
{
	register struct locationblk *end;

	if ( blkptr ) {
		for( end = blkptr; end->next; end = end->next )
			;
		end->next = freelist;
		freelist = blkptr;
	}
}

static struct locationblk *
newlocblk()
{
	register struct locationblk *blkptr;
	register struct locationpage *pgptr;

	if ( freelist ) {
		blkptr = freelist;
		freelist = freelist->next;
		blkptr->next = NULL;
		blkptr->blkcnt = 0;
	}
	else if ( alloccnt-- > 0 )
		blkptr = allocblk++;
	else {
		pgptr = (struct locationpage *)calloc( 1, sizeof( struct locationpage ) );
		if ( pgptr == NULL ) {
			set_uerror( UE_NOMEM );
			return( NULL );
		}

		if ( pagelist )
			pgptr->next = pagelist;
		pagelist = pgptr;
		blkptr = pgptr->entries;
		alloccnt = LOCPAGESIZE - 1;
		allocblk = &pgptr->entries[1];
	}

	return( blkptr );
}

static int
findlocation( rootblk, seekval, insert )
struct locationblk **rootblk;
register long seekval;
int insert;
{
	register struct locationblk *blkptr;
	register int i;

	for( blkptr = *rootblk; blkptr; blkptr = blkptr->next ) {
		for( i = 0; i < blkptr->blkcnt; i++ ) {
			if ( blkptr->entries[i] == seekval )
				return( TRUE );
		}

		if ( blkptr->next == NULL )
			break;		/* save value of blkptr */
	}

	if ( insert ) {
		if ( blkptr == NULL ) {
			blkptr = newlocblk();
			*rootblk = blkptr;
		}
		else if ( blkptr->blkcnt >= LOCBLKSIZE ) {
			blkptr->next = newlocblk();
			blkptr = blkptr->next;
		}

		blkptr->entries[blkptr->blkcnt++] = seekval;

		return( TRUE );
	}

	return( FALSE );	
}

static struct locationblk *
mergeblks( blk1, blk2 )
struct locationblk *blk1;
register struct locationblk *blk2;
{
	register int i;
	register struct locationblk *next;

	if ( blk1 == NULL )
		return( blk2 );
	else if ( blk2 == NULL )
		return( blk1 );

	for( ; blk2; blk2 = next ) {
		for( i = 0; i < blk2->blkcnt; i++ )
			(void)findlocation( &blk1, blk2->entries[i], TRUE );
		next = blk2->next;
		freelocblk( blk2 );
	}

	return( blk1 );
}

static struct locationblk *
pruneblks( blk1, blk2 )
struct locationblk *blk1;
register struct locationblk *blk2;
{
	register int i;
	register struct locationblk *tmpblk, *next, *resultblk;

	if ( blk1 == NULL ) {
		freelocblks( blk2 );
		return( NULL );
	}
	else if ( blk2 == NULL ) {
		freelocblks( blk1 );
		return( NULL );
	}

	resultblk = newlocblk();
	tmpblk = resultblk;
	for( ; blk2; blk2 = next ) {
		for( i = 0; i < blk2->blkcnt; i++ ) {
			if ( findlocation( &blk1, blk2->entries[i], FALSE ) ) {
				if ( tmpblk->blkcnt >= LOCBLKSIZE ) {
					tmpblk->next = newlocblk();
					tmpblk = tmpblk->next;
				}
				tmpblk->entries[tmpblk->blkcnt++] = blk2->entries[i];
			}
		}

		next = blk2->next;
		freelocblk( blk2 );
	}

	if ( resultblk->blkcnt == 0 ) {
		freelocblk( resultblk );
		resultblk = NULL;
	}

	freelocblks( blk1 );

	return( resultblk );
}

static
savetuplelocs( rootblk, fp, seekval )
register struct locationblk **rootblk;
FILE *fp;
long seekval;
{
	register struct locationblk *blkptr;
	register long readsize;
	long reccnt;

	fseek( fp, seekval, 0 );
	fread( (char *)&reccnt, sizeof( long ), 1, fp );

	if ( *rootblk == NULL )
		*rootblk = newlocblk();
	for( blkptr = *rootblk; blkptr->blkcnt >= LOCBLKSIZE;
			blkptr = blkptr->next ) {
		if ( blkptr->next == NULL ) {
			blkptr->next = newlocblk();
			blkptr = blkptr->next;
			break;
		}
	}

	reccnt *= -1;
	while( 1 ) {
		readsize = LOCBLKSIZE - blkptr->blkcnt;
		if ( reccnt < readsize )
			readsize = reccnt;

		fread( (char *)&blkptr->entries[blkptr->blkcnt],
			readsize * sizeof( long ), 1, fp );
		blkptr->blkcnt += readsize;
		if ( (reccnt -= readsize) <= 0 )
			break;

		for( ; blkptr->blkcnt >= LOCBLKSIZE; blkptr = blkptr->next ) {
			if ( blkptr->next == NULL )
				blkptr->next = newlocblk();
		}
	}
}

static
lkupindexes( nodeptr, attr, str, optype, rootblk )
register struct qnode *nodeptr;
int attr;
char *str;
unsigned char optype;
struct locationblk **rootblk;
{
	char *found;
	int status;
	long seekval;
	FILE *fp;
	struct uindex *tree;

	*rootblk = NULL;

	/*
	 * Initialize the index.
	 */
	if ( ! indexch( nodeptr->rel->path,
			nodeptr->rel->attrs[attr].aname, &fp, &tree ) )
		return( FALSE );

	/*
	 * Now get the tuples
	 */
	switch( optype ) {
	case OPNE:
	case OPNOTIN:
		status = rdindexed( tree, "", &found, &seekval );
		while( status != END ) {
			if ( strcmp( found, str ) )
				savetuplelocs( rootblk, fp, seekval );
			status = rdnext( tree, &found, &seekval );
		}
		break;
	case OPLT:
		status = rdindexed( tree, "", &found, &seekval );
		while( status != END ) {
			if ( strcmp( str, found ) >= 0 )
				break;
			savetuplelocs( rootblk, fp, seekval );
			status = rdnext( tree, &found, &seekval );
		}
		break;
	case OPLE:
		status = rdindexed( tree, "", &found, &seekval );
		while( status != END ) {
			if ( strcmp( str, found ) > 0 )
				break;
			savetuplelocs( rootblk, fp, seekval );
			status = rdnext( tree, &found, &seekval );
		}
		break;
	case OPEQ:
	case OPIN:
		status = rdindexed( tree, str, &found, &seekval );
		if ( status == FOUND )
			savetuplelocs( rootblk, fp, seekval );
		break;
	case OPGE:
		status = rdindexed( tree, str, &found, &seekval );
		while( status != END ) {
			savetuplelocs( rootblk, fp, seekval );
			status = rdnext( tree, &found, &seekval );
		}
		break;
	case OPGT:
		status = rdindexed( tree, str, &found, &seekval );
		if ( status == FOUND )
			status = rdnext( tree, &found, &seekval );
		while( status != END ) {
			savetuplelocs( rootblk, fp, seekval );
			status = rdnext( tree, &found, &seekval );
		}
		break;
	}

	fclose( fp );
	close( tree->fd );

	return( TRUE );
}

static
lookupset( nodeptr, qptr, rootblk )
register struct qnode *nodeptr;
register struct queryexpr *qptr;
struct locationblk **rootblk;
{
	register struct attrref *refptr;
	register int i, cnt;
	register char **strlist;
 	struct locationblk *tmpblk, *totalblk;

	*rootblk = NULL;
	
	cnt = qptr->elem1.alist.cnt;
	strlist = qptr->elem2.strlist;

	while( *strlist )
	{
		totalblk = NULL;

		for( i = 0, refptr = qptr->elem1.alist.list;
			i < cnt;
			i++, refptr++ )
		{
			tmpblk = NULL;
			if ( ! lkupindexes( nodeptr, refptr->attr, *strlist++,
					qptr->optype, &tmpblk ) ) {
				freelocblks( tmpblk );
				freelocblks( totalblk );
				freelocblks( *rootblk );
				return( FALSE );
			}
			if ( totalblk == NULL )
				totalblk = tmpblk;
			else if ( qptr->optype == OPNOTIN )
				totalblk = mergeblks( totalblk, tmpblk );
			else
				totalblk = pruneblks( totalblk, tmpblk );
		}
		if ( *rootblk == NULL )
			*rootblk = totalblk;
		else if ( qptr->optype == OPNOTIN )
			*rootblk = pruneblks( *rootblk, totalblk );
		else
			*rootblk = mergeblks( *rootblk, totalblk );
	}

	return( TRUE );
}

static
lookuptuples( nodeptr, qptr, rootblk )
register struct qnode *nodeptr;
register struct queryexpr *qptr;
struct locationblk **rootblk;
{
	register struct attrref *attrptr;
	register char *str;
 	struct locationblk *leftblk, *rightblk;

	if ( ISBOOL( qptr->optype ) ) {
		if ( ! lookuptuples( nodeptr, qptr->elem1.expr, &leftblk ) )
			return( FALSE );

		switch( qptr->optype ) {
		case OPELSE:
			if ( leftblk ) {
				*rootblk = leftblk;
				return( TRUE );
			}
			return( lookuptuples( nodeptr, qptr->elem2.expr,
						rootblk ) );
		case OPOR:
			if ( ! lookuptuples( nodeptr, qptr->elem2.expr,
					&rightblk ) ) {
				freelocblks( leftblk );
				return( FALSE );
			}
			*rootblk = mergeblks( leftblk, rightblk );
			return( TRUE );
		case OPAND:
			if ( ! lookuptuples( nodeptr, qptr->elem2.expr,
					&rightblk ) ) {
				freelocblks( leftblk );
				return( FALSE );
			}
			*rootblk = pruneblks( leftblk, rightblk );
			return( TRUE );
		}
	}

	/*
	 * If it's a set comparison, we must do things very differently.
	 */
	if ( ISSETCMP( qptr->optype ) ) {
		if ( qptr->elem1.alist.list->rel != nodeptr ) {
			*rootblk = NULL;
			return( TRUE );
		}
		else if ( qptr->cmptype != QCMP_STRING ) {
			*rootblk = NULL;
			return( FALSE );
		}
		return( lookupset( nodeptr, qptr, rootblk ) );
	}

	/*
	 * Expression is compare operation.  So lookup all the tuples
	 * that match the given conditions.
	 *
	 * We return an empty location block if the compare does not
	 * apply to this node.
	 */
	switch( qptr->opflags & (ISATTR1|ISATTR2) ) {
	case ISATTR1:
		str = qptr->elem2.strval;
		attrptr = &qptr->elem1.attr;
		break;
	case ISATTR2:
		str = qptr->elem1.strval;
		attrptr = &qptr->elem2.attr;
		break;
	case (ISATTR1|ISATTR2):
	default:
		*rootblk = NULL;
		return( FALSE );
	}
	if ( attrptr->rel != nodeptr ) {
		*rootblk = NULL;
		return( TRUE );
	}

	return( lkupindexes( nodeptr, attrptr->attr, str, qptr->optype,
			rootblk ) );
}

long
tuple_by_index( operptr, nodeptr, edgenum, query, addfunc )
struct uqoperation *operptr;
struct qnode *nodeptr;
int edgenum;
struct uquery *query;
int (*addfunc)();
{
	struct locationblk *rootblk;
	register int i, rc;
	register long tuplecnt;
	struct utuple *tptr;
	register struct locationblk *blkptr, *next;

	if ( ! lookuptuples( nodeptr, operptr->cmplist[edgenum], &rootblk))
		return( -1L );
	else if ( rootblk == NULL ) {
		return( 0L );
	}

	/*
	 * Read in all the tuples in the location block.
	 */
	tuplecnt = 0L;
	rc = 1;
	if ( ! relreset( nodeptr ) )
		rc = -1;
	for( blkptr = rootblk; blkptr; blkptr = next ) {
		for( i = 0; rc > 0 && i < blkptr->blkcnt; i++ ) {
			tptr = seektuple( nodeptr, blkptr->entries[i] );
			if ( tptr == NULL ) {	/* seektuple failed */
				rc = -1;
				break;
			}

			if ( operptr->cmpcnt == 1 ||
					chktuple( query, operptr, nodeptr, tptr, edgenum))
			{
				tuplecnt++;
				rc = (*addfunc)( query, nodeptr, tptr );
			}
			else if ( ! deltuple( query, nodeptr, tptr ) ) {
				rc = -1;
				break;
			}
		}
		next = blkptr->next;
		freelocblk( blkptr );
	}

	unsaveelse( operptr, nodeptr ); /* remove tuple refs saved for else's */

	return( rc < 0 ? (long) rc : tuplecnt );
}
