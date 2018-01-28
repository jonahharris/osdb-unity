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

extern struct utuple *newtuple();

static struct utuple *
nulltuple( nodeptr )
struct qnode *nodeptr;
{
	register struct utuple *tplptr;
	short i;

	tplptr = newtuple( nodeptr );
	if ( tplptr == NULL )
		return( NULL );

	tplptr->flags |= TPL_NULLTPL;
	tplptr->tuplenum = 0xffffffff;
	tplptr->lseek = 0xffffffff;

	for( i = 0; i < nodeptr->rel->attrcnt; i++ )
	{
		tplptr->tplval[i] = "";
	}

	i = nodeptr->flags;
	nodeptr->flags |= N_FORCESAVE;
	if ( ! addtuple( NULL, nodeptr, tplptr ) )
	{
		nodeptr->flags = i;
		freetuple( tplptr );
		return( NULL );
	}
	nodeptr->flags = i;

	return( tplptr );
}

struct utuple **
nullsupertpl( query, nodeptr, tplptr )
struct uquery *query;
struct qnode *nodeptr;
struct utuple *tplptr;
{
	static struct utuple *_unullstpl[ MAXRELATION ];
	short i;

	memset( _unullstpl, 0, sizeof( _unullstpl ) );

	for( i = 0; i < query->nodecnt; i++ )
	{
		if ( (nodeptr->snode->node_present & (1 << i)) != 0 &&
			i != nodeptr->nodenum )
		{
			_unullstpl[ i ] = nulltuple( query->nodelist[ i ] );
			if ( _unullstpl[ i ] == NULL )
			{
				return( NULL );
			}
		}
	}

	_unullstpl[ nodeptr->nodenum ] = tplptr;

	return( _unullstpl );
}

outerjoin( query, nodeptr, tplptr, jinfo, operptr )
struct uquery *query;
struct qnode *nodeptr;
struct utuple *tplptr;
struct joininfo *jinfo;
struct uqoperation *operptr;
{
	struct utuple *tpl1, *tpl2;
	struct supernode *snode;

	/*
	 * If the nodes are already in the same supernode, there
	 * will be no new supernode; use the previous one.
	 */
	snode = ( operptr->node1->snode == operptr->node2->snode &&
			jinfo->snode == NULL ?
				operptr->node1->snode : jinfo->snode );

	if ( nodeptr == operptr->node1 )
	{
		tpl1 = tplptr;
		tpl2 = nulltuple( operptr->node2 );
		if ( tpl2 == NULL )
			return( FALSE );
	}
	else
	{
		tpl1 = nulltuple( operptr->node1 );
		if ( tpl1 == NULL )
			return( FALSE );
		tpl2 = tplptr;
	}

	if ( ! (*jinfo->joinfunc)( query, snode,
			operptr->node1, operptr->node2, tpl1, tpl2 ) )
	{
		return( FALSE );
	}

#if 1
	if ( nodeptr == operptr->node1 )
	{
		if ( (*jinfo->addfunc1)( query, nodeptr, tplptr ) <= 0 )
			return( FALSE );
	}
	else
	{
		if ( (*jinfo->addfunc2)( query, nodeptr, tplptr ) <= 0 )
			return( FALSE );
	}
#else
	if ( (*jinfo->addfunc1)( query, operptr->node1, tpl1 ) <= 0 )
		return( FALSE );
	if ( (*jinfo->addfunc2)( query, operptr->node2, tpl2 ) <= 0 )
		return( FALSE );
#endif

	return( TRUE );			
}
