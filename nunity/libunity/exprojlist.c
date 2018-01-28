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
#include "message.h"

extern char *calloc();
extern char *strchr();

/*
 * Below are the routines for sorting the priorities for
 * attributes used in sorting.
 */
static struct uquery *cmp_query;

static int
cmp_priority( ptr1, ptr2 )
int *ptr1;
int *ptr2;
{
	register struct qprojection *proj1, *proj2;
	register unsigned short sort1, sort2;

	proj1 = &cmp_query->attrlist[ *ptr1 ];
	proj2 = &cmp_query->attrlist[ *ptr2 ];

	sort1 = (proj1->flags & (QP_SORT|QP_SORTMASK)) != 0;
	sort2 = (proj2->flags & (QP_SORT|QP_SORTMASK)) != 0;

	if ( sort1 && ! sort2 )
		return( -1 );
	else if ( ! sort1 && sort2 )
		return( 1 );

	if ( proj1->priority < proj2->priority )
		return( -1 );
	else if ( proj1->priority > proj2->priority )
		return( 1 );
	else if ( *ptr1 < *ptr2 )	/* check relative indexes */
		return( -1 );
	else
		return( 1 );
}

static void
sort_priority( query )
struct uquery *query;
{
	int i;
	int sortattr[ MAXATT ];

	cmp_query = query;

	for( i = 0; i < query->attrcnt; i++ )
		sortattr[i] = i;

	qsort( (char *)sortattr, query->attrcnt, sizeof( int ),
		cmp_priority );

	for( i = 0; i < query->attrcnt; i++ )
		query->attrlist[i].sortattr = sortattr[i];
}

exprojlist( query, friendly )
struct uquery *query;
int friendly;
{
	register struct qprojection *refptr, *newref;
	register unsigned int i, acnt, j;
	struct uattribute *attrptr;
	char allexcept[MAXATT];

	for( refptr = query->attrlist, i = 0, acnt = 0;
		acnt < MAXATT && i < query->attrcnt;
		i++, refptr++ )
	{
		/*
		 * Sorting can be specified right up front in the
		 * query flags or can be given on specific attributes.
		 * (Uniqueness cannot be specified on an attribute
		 * basis, but is a concept of the entire projected tuple.)
		 * So, the first thing we need to do is check the
		 * attributes to see if they are sorted.  This implies
		 * the query has sorting.  We then need to make a
		 * second pass over the attributes to count how many
		 * sort attributes there are.
		 */
		if ( refptr->flags & QP_SORT )
		{
			/*
			 * Sorting specified for this attribute.
			 */
			query->flags |= Q_SORT;
		}
		else if ( refptr->flags & QP_SORTMASK ) {
			/*
			 * Sorting or uniqueness was specified
			 * for this attribute.  If neither is set
			 * in the query, then we assume sorting.
			 */
			if ( (query->flags & (Q_SORT|Q_UNIQUE)) == 0 )
				query->flags |= Q_SORT;
		}

		if ( refptr->attr == ATTR_ALL )
			acnt += refptr->rel->rel->attrcnt;
		else
			acnt++;		/* nothing special */
	}
	if ( acnt == 0 )	/* no attributes projected */
	{
		query->attrcnt = 0;
		query->attrlist = NULL;
		return( TRUE );
	}
	else if ( acnt > MAXATT )
		acnt = MAXATT;

	newref = (struct qprojection *)calloc( acnt, sizeof( struct qprojection));
	if ( newref == NULL )
	{
		set_uerror( UE_NOMEM );
		return( FALSE );
	}

	refptr = query->attrlist;
	query->attrlist = newref;
	query->sortcnt = 0;
	for( i = 0, acnt = 0; acnt < MAXATT && i < query->attrcnt;
		i++, refptr++ )
	{
		if ( (query->flags & (Q_SORT|Q_UNIQUE)) == (Q_SORT|Q_UNIQUE) &&
		     (refptr->flags & QP_NODISPLAY) )
		{
			/*
			 * When sorting uniquely, non-displayed attributes do
			 * not participate in the sort (since uniqueness is
			 * determined over the projected attributes, only.)
			 * So remove the sorting criteria from any
			 * non-displayed attributes in this case
			 * unless the QP_SORTUNIQ flag was set to
			 * to indicate that more than one type of
			 * of sort was requested on this attribute.
			 */
			if ( ( refptr->flags & QP_SORTUNIQ ) == 0 ) {
				refptr->flags &= ~(QP_SORT|QP_SORTMASK);
			}
		}

		if ( refptr->attr != ATTR_ALL )
		{
			*newref++ = *refptr;
			acnt++;

			/*
			 * Adjust the sort count for the query if sorting
			 * is to be done.
			 */
			if ( (query->flags & Q_SORT) &&
				(refptr->flags & (QP_SORT|QP_SORTMASK)) )
			{
				query->sortcnt++;
			}

			continue;
		}

		/*
		 * Adjust the sort count for the query if sorting
		 * is to be done.
		 */
		if ( (query->flags & Q_SORT) &&
			(refptr->flags & (QP_SORT|QP_SORTMASK)) )
		{
			/*
			 * Only increment the sort cnt by as many
			 * attributes as will fit in the projection,
			 * i.e., if this expansion will cause us to
			 * go beyond MAXATT, then only add in the
			 * number that will fit.
			 */
			query->sortcnt +=
				( acnt + refptr->rel->rel->attrcnt <=
						MAXATT ) ?
				refptr->rel->rel->attrcnt :
				MAXATT - acnt;
		}

		/*
		 * If not all attributes are to be displayed
		 * then we need to get a list of which ones
		 * are to be marked QP_NODISPLAY.
		 */
		strncpy( allexcept, "", MAXATT );	/* initialize values */
		if ( refptr->attorval != NULL )
		{
			char	*colonptr, *p, *q;

			p = refptr->attorval;
			if ( ( colonptr = strchr( p, ':' ) ) != NULL ) {
				*colonptr = '\0';
			}
			while ( p )
			{
				if ( ( q = strchr( p, ',' ) ) != NULL ) {
					*q = '\0';
				}
				for ( j = 0, attrptr = refptr->rel->rel->attrs;
				      j < refptr->rel->rel->attrcnt && j < MAXATT; attrptr++, j++ )
				{
					if ( strcmp( p, attrptr->aname ) == 0 ) {
						allexcept[ j ] = TRUE;
						break;
					}
				}
				if ( q ) {
					*q = ',';
					p = q + 1;
				} else {
					p = NULL;
					break;
				}
			}
			if ( colonptr ) {
				*colonptr = ':';
			}
		}

		/*
		 * Now add in the new attributes.
		 */
		for( j = 0; j < refptr->rel->rel->attrcnt && acnt < MAXATT;
			j++ )
		{
			*newref = *refptr;	/* copy print width, etc */
			newref->attr = j;	/* reset attr number */

			if ( allexcept[ j ] ) {

				newref->flags |= QP_NODISPLAY;
			}
			newref->attorval = NULL;

			attrptr = &refptr->rel->rel->attrs[j];
			if ( newref->prwidth == 0 )
				newref->prwidth = attrptr->prwidth;
			if ( newref->justify == '\0' )
				newref->justify = attrptr->justify;
			if ( ( ( newref->attrtype != QP_TERMCHAR ) &&
			       ( newref->attrtype != QP_FIXEDWIDTH ) ) ||
			     ( ( newref->attrtype == QP_FIXEDWIDTH ) &&
			       ( newref->attrwidth >= DBBLKSIZE ) ) )
			{
				if ( ( attrptr->attrtype == UAT_TERMCHAR ) ||
				     ( attrptr->terminate >= DBBLKSIZE ) )
				{
					newref->attrtype = QP_TERMCHAR;
					newref->terminate =
						j == refptr->rel->rel->attrcnt - 1 ?
						'\t' : (char)attrptr->terminate;
					newref->attrwidth = 0;
				} else {
					newref->attrtype = QP_FIXEDWIDTH;
					newref->attrwidth = attrptr->terminate;
					newref->terminate = '\0';
				}
			}

			/*
			 * Set print name based on whether the friendly
			 * name is desired and there is a friendly name.
			 * The default is to use the attribute name itself.
			 */
			newref->prname = ( friendly &&
					attrptr->friendly != NULL ) ?
					attrptr->friendly :
					attrptr->aname;
			acnt++;
			newref++;
		}
	}

	query->attrcnt = acnt;

	/*
	 * Make sure the last projected attribute has a new-line as
	 * its terminator.
	 */
	i = acnt - 1;
	while( 1 )
	{
		if ( (query->attrlist[i].flags & QP_NODISPLAY) == 0 )
		{
			query->attrlist[i].terminate = '\n';
			break;
		}
		if ( i == 0 )
			break;
		i--;
	}

	/*
	 * Set up the sort order for the attributes.
	 */
	if ( query->flags & Q_SORT ) {
		/*
		 * If no sorted attributes were specified, or uniqueness
		 * is needed also, then sort on all attributes.
		 */
		if ( query->sortcnt == 0 || (query->flags & Q_UNIQUE) != 0 )
			query->sortcnt = acnt;

		sort_priority( query );
	}

	return( TRUE );
}
