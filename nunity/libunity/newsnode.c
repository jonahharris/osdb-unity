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

extern char *malloc();
extern char *calloc();

#define MAXSNFREE	8	/* max super nodes on free list */

#define JOINPAGESIZE	8	/* number of joinblocks per allocation */

struct joinpage {
	struct joinpage *next;
	struct joinblock entries[JOINPAGESIZE];
};

static struct joinblock *joinfree, *jallocblk;
static short jalloccnt;
static struct joinpage *jpagelist;
static struct supernode *snodefree;
static int snfreecnt;

_freealljoinblks( )
{
	register struct joinpage *next;

	if ( inquery() )
		return( FALSE );

	jalloccnt = 0;
	joinfree = jallocblk = NULL;
	while( jpagelist )
	{
		next = jpagelist;
		jpagelist = jpagelist->next;
		free( next );
	}

	return( TRUE );
}

static struct joinblock *
newjoinblock( )
{
	register struct joinblock *blkptr;

	if ( joinfree )
	{
		blkptr = joinfree;
		joinfree = joinfree->next;
	}
	else if ( jalloccnt-- > 0 )
		blkptr = jallocblk++;
	else
	{
		register struct joinpage *jpgptr;

		jpgptr = (struct joinpage *)malloc( sizeof( struct joinpage ));
		if ( jpgptr == NULL )
		{
			set_uerror( UE_NOMEM );
			return( NULL );
		}

		if ( jpagelist )
			jpgptr->next = jpagelist->next;
		jpagelist = jpgptr;

		blkptr = jpgptr->entries;
		jallocblk = &jpgptr->entries[1];
		jalloccnt = JOINPAGESIZE - 1;
	}

	(void)memset( blkptr, 0, sizeof( struct joinblock ) );

	return( blkptr );
}

struct utuple **
newsupertpl( blklist, query )
struct joinblock *blklist;
struct uquery *query;
{
	struct utuple **stpl;
	register struct joinblock *blkptr;

	blkptr = blklist;
	while( 1 )
	{
		if ( blkptr->blkcnt % query->nodecnt != 0 )
		{
			set_uerror( UE_QUERYEVAL );
			return( NULL );
		}
		if ( blkptr->blkcnt + query->nodecnt < JOINBLKSIZE )
		{
			stpl = &blkptr->tuples[ blkptr->blkcnt ];
			blkptr->blkcnt += query->nodecnt;

			return( stpl );
		}

		if ( blkptr->next )
			blkptr = blkptr->next;
		else
			break;
	}

	blkptr = newjoinblock( );
	if ( blkptr == NULL )
		return( NULL );

	blkptr->next = blklist->next;
	blklist->next = blkptr;
	blkptr->blkcnt = query->nodecnt;

	return( blkptr->tuples );
}

static
freejoinblist( blkptr )
struct joinblock *blkptr;
{
	register struct joinblock *end;

	if ( blkptr )
	{
		for( end = blkptr; end->next; end = end->next )
			;
		end->next = joinfree;
		joinfree = blkptr;
	}
}

long
snode_nodes( node1, node2 )
struct qnode *node1;
struct qnode *node2;
{
	long node_present;

	node_present = (1 << node1->nodenum) | (1 << node2->nodenum);

	if ( node1->snode != NULL )
		node_present |= node1->snode->node_present;
	if ( node2->snode != NULL )
		node_present |= node2->snode->node_present;

	return( node_present );
}

struct supernode *
newsupernode( node_present )
long node_present;
{
	register struct supernode *snode;

	if ( snodefree )
	{
		snode = snodefree;
		snode->joinptr = newjoinblock();
		if ( snode->joinptr == NULL )
			return( FALSE );

		snodefree = snodefree->next;
		snfreecnt--;
		snode->next = NULL;
	}
	else
	{
		snode = (struct supernode *)calloc( 1, sizeof( *snode ) );
		if ( snode == NULL )
		{
			set_uerror( UE_NOMEM );
			return( NULL );
		}

		snode->joinptr = newjoinblock();
		if ( snode->joinptr == NULL )
		{
			freesupernode( (struct supernode **)NULL, snode );
			return( NULL );
		}
	}

	snode->node_present = node_present;

	return( snode );
}

freesupernode( listptr, snode )
struct supernode **listptr;
register struct supernode *snode;
{
	if ( listptr )
	{
		register struct supernode *next, **prev;

		prev = listptr;
		for( next = *listptr; next; next = next->next )
		{
			if ( next == snode )
			{
				*prev = snode->next;
				break;
			}
			else
				prev = &next->next;
		}
	}

	freejoinblist( snode->joinptr );

	if ( snfreecnt >= MAXSNFREE )
		free( snode );
	else
	{
		snode->joinptr = NULL;
		snfreecnt++;
		snode->next = snodefree;
		snodefree = snode;
	}
}

_freeallsnodes( )
{
	register struct supernode *next;

	if ( inquery() )
		return( FALSE );

	snfreecnt = 0;
	while( snodefree )
	{
		next = snodefree;
		snodefree = snodefree->next;
		free( next );
	}


	return( TRUE );
}
