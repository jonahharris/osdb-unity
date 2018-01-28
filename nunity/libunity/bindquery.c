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
#include <sys/types.h>
#include <sys/stat.h>
#include "uquery.h"
#include "uerror.h"

static int
mapattr( attrnames, nodeptr, attrnum, do_select )
char **attrnames;
register struct qnode *nodeptr;
short *attrnum;
int do_select;
{
	register int newattrnum;

	if ( ATTR_SPECIAL( *attrnum ) )
		return( TRUE );

	newattrnum = findattr( nodeptr->rel->attrs, nodeptr->rel->attrcnt,
			attrnames[*attrnum] );
	if ( newattrnum < 0 ) {
		set_uerror( UE_UNRECATTR );
		return( FALSE );
	}

	if ( do_select )
		nodeptr->memattr[newattrnum] |= MA_SELECT;
	else {
		nodeptr->memattr[newattrnum] |= MA_PROJECT;
		nodeptr->flags |= N_PROJECT;
	}

	*attrnum = newattrnum;

	return( TRUE );
}

int
bindquery( query, exprlist, exprcnt, attrnames )
register struct uquery *query;
register struct queryexpr *exprlist;
int exprcnt;
register char **attrnames;
{
	register int i, j;
	register struct qnode *nodeptr;

	if ( query->version != UQUERY_VERSION ) {
		set_uerror( UE_BADQVERS );
		return( FALSE );
	}

	if ( query->flags & Q_INIT )
		return( TRUE );

	for( i = 0; i < query->attrcnt; i++ ) {
		/*
		 * Check for the "all" attribute.  This signifies
		 * all of the relation's attributes.
		 */
		if ( query->attrlist[i].attr == ATTR_ALL ) {
			nodeptr = query->attrlist[i].rel;
			nodeptr->flags |= N_PROJECT;
			for( j = 0; j < nodeptr->rel->attrcnt; j++ )
				nodeptr->memattr[j] |= MA_PROJECT;
		}
		else if ( ! mapattr( attrnames, query->attrlist[i].rel,
				&query->attrlist[i].attr, FALSE ) ) {
			set_uerror( UE_UNRECATTR );
			return( FALSE );
		}
	}

	if ( ! exprojlist( query, FALSE ) )  /* expand the projection list */
		return( FALSE );

	for( i = 0; i < exprcnt; i++ ) {
		register struct queryexpr *qptr;

		qptr = &exprlist[i];
		if ( ISBOOL( qptr->optype ) )
			continue;

		if ( ISSETCMP( qptr->optype ) ) {
			for( j = 0; j < qptr->elem1.alist.cnt; j++ ) {
				if ( ! mapattr( attrnames,
						qptr->elem1.alist.list[j].rel,
						&qptr->elem1.alist.list[j].attr,
						TRUE ) )
					return( FALSE );
			}
		}
		else {
			if ( ! mapattr( attrnames, qptr->elem1.attr.rel,
					&qptr->elem1.attr.attr, TRUE ) )
				return( FALSE );

			if ( (qptr->opflags & ISATTR2) &&
				! mapattr( attrnames, qptr->elem2.attr.rel,
					&qptr->elem2.attr.attr, TRUE ) )
				return( FALSE );
		}
	}

	for( i = 0; i < query->nodecnt; i++ ) {
		struct stat relstat;

		/*
		 * NOTE: We expect all packed relations to fail
		 *	 on the stat(2) call below so that no
		 *	 attempt is made to use indexes.
		 */
		nodeptr = query->nodelist[i];
		if ( strcmp( nodeptr->rel->path, "-" ) == 0 ||
				stat( nodeptr->rel->path, &relstat ) < 0 )
			continue;

		/*
		 * Check the type of device for the relation.
		 * If it's a fifo device, then indexes cannot be
		 * used on it, since it is not possible to seek.
		 */
		if ( (relstat.st_mode & S_IFMT) == S_IFIFO )
			continue;

		for( j = 0; j < nodeptr->rel->attrcnt; j++ ) {
			if ( chkindex( nodeptr->rel->path, &relstat,
					nodeptr->rel->attrs[j].aname ) )
				nodeptr->memattr[j] |= MA_INDEX;
		}
	}

	query->flags |= Q_INIT;

	return( TRUE );
}
