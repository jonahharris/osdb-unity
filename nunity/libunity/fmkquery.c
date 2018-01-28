/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <ctype.h>
#include "uquery.h"
#include "uerror.h"
#include "message.h"

extern struct queryexpr *fparsewhere();
extern struct uquery *fbldquery();
extern struct urelation *getrelinfo();
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
extern char *calloc();
#endif

static int
findsortatt( projptr, projcnt, sortinfo )
struct qprojection *projptr;
int projcnt;
struct qprojection *sortinfo;
{
	int i, j;
	int rc;

	rc = -1;

	for( i = 0; i < projcnt; i++, projptr++ )
	{
		if ( ( projptr->rel == sortinfo->rel ) &&
		     ( projptr->attr == sortinfo->attr ) &&
		   ((( projptr->flags & QP_NEWVALUE) == 0 ) ||
		     ( projptr->attorval == NULL )))
		{
			/*
			 * Found the attribute.
			 * Now we must check that it
			 * has not already been used for sorting.
			 */
			if ( projptr->priority < sortinfo->priority )
			{
				/*
				 * This attribute as already been used for sorting.
				 * If the attribute is to be displayed then update
				 * the return code to indicate that the attribute
				 * was found but it was already marked for sorting.
				 */
				if ( ! ( projptr->flags & QP_NODISPLAY ) )
					rc = -2;
				continue;
			}
			/*
			 * Found this attribute.  Merge the sort info.
			 * Do not set or clear QP_NODISPLAY or any other
			 * flag bits that are not related to sorting.
			 */
			projptr->flags = ( ( projptr->flags & ~(QP_SORTMASK|QP_DESCENDING|QP_RMBLANKS) ) |
					( QP_SORT ) |
					( sortinfo->flags & (QP_SORTMASK|QP_DESCENDING|QP_RMBLANKS) ) );
			projptr->subcnt = sortinfo->subcnt;
			projptr->priority = sortinfo->priority;
			projptr->delim = sortinfo->delim;
			for( j = 0; j < sortinfo->subcnt; j++ )
				projptr->subsort[j] = sortinfo->subsort[j];

			/*
			 * If this attribute is marked nodisplay
			 * but is projected to be displayed somewhere
			 * and is not the special ``all'' attribute
			 * then we need to indicate that any uniqueness
			 * checks should not skip this attribute even
			 * though it is marked nodisplay.
			 */
			if ( ( projptr->flags & QP_NODISPLAY ) &&
			     ( projptr->attr != ATTR_ALL ) ) {
				if ( rc == -2 ) {
					projptr->flags |= QP_SORTUNIQ;
				} else {
					struct qprojection *saveqptr = projptr;

					for ( ++projptr, j = i + 1; j < projcnt; j++, projptr++ )
					{
						if ( ( projptr->rel == sortinfo->rel ) &&
						     ( projptr->attr == sortinfo->attr ) &&
						    (( projptr->flags & QP_NODISPLAY ) == 0 ) &&
						   ((( projptr->flags & QP_NEWVALUE ) == 0 ) ||
						     ( projptr->attorval == NULL )))
						{
							saveqptr->flags |= QP_SORTUNIQ;
							break;
						}
					}
				}
			}

			return( i );
		}
	}

	return( rc );
}

struct uquery *
fmkquery( flags, relnames, relcnt, attrnames, anamecnt, sortlist, sortcnt,
	where, wherecnt )
int flags;
char **relnames;
int relcnt;
char **attrnames;
int anamecnt;
char **sortlist;
int sortcnt;
char **where;
int wherecnt;
{
	register int i;
	struct urelation *rellist;	/* relation information */
	struct qnode *nodelist;		/* node space for query */
	struct queryexpr *qptr;		/* query expr. from where-clause */
	struct uquery *query;		/* compiled version of query */
	char seen_stdin;		/* seen '-' in relnames? */
	int attrcnt;			/* attribute count */
	int lastsort;			/* last sorted attribute index */
	unsigned short sort_priority;	/* priority of each sorted attr */
	struct qprojection attrlist[MAXATT];	/* projected attributes */

	if ( relcnt <= 0 )	/* Gotta have a relation */
	{
		prmsg( MSG_ERROR, "no relations given" );
		return( NULL );
	}

	nodelist = (struct qnode *)calloc( (unsigned)relcnt, sizeof( *nodelist ) );
	rellist = (struct urelation *)calloc( (unsigned)relcnt, sizeof( *rellist));
	if ( nodelist == NULL || rellist == NULL )
	{
		prmsg( MSG_INTERNAL, "cannot allocate space for relation information" );
		if ( rellist )
			free( rellist );
		if ( nodelist )
			free( nodelist );
		return( NULL );
	}

	/*
	 * Get all the information about each relation.  Allow only one
	 * reference to stdin in the list.  Relation information is also
	 * plugged into each query node for the query routines.
	 */
	seen_stdin = FALSE;
	for( i = 0; i < relcnt; i++ )
	{
		if ( ! getrelinfo( relnames[i], &rellist[i], FALSE ) )
		{
			(void)pruerror( );
			prmsg( MSG_ERROR, "cannot get descriptor info for table '%s'",
				relnames[i] );
			free( rellist );
			free( nodelist );
			return( NULL );
		}

		if ( strcmp( rellist[i].path, "-" ) == 0 )
		{
			if ( seen_stdin )
			{
				prmsg( MSG_ERROR, "only one standard input relation is allowed" );
				free( rellist );
				free( nodelist );
				return( NULL );
			}
			seen_stdin = TRUE;
		}

		nodelist[i].rel = &rellist[i];	/* save relation in qnode */
		nodelist[i].nodenum = (unsigned char) i;
	}

	/*
	 * If there was a list of attributes to project, find them and 
	 * make sure they are all legit.  If there were no attributes listed,
	 * list all attributes from all relations.  Also handle the case where
	 * the NULL attribute is listed.  This basically just tells the
	 * existance of a tuple, but nothing else.
	 */
	if ( anamecnt == 1 && ( *attrnames == NULL || **attrnames == '\0' ) )
		attrcnt = 0;		/* NULL attribute */
	else if ( anamecnt > 0 )
	{
		/* Look up each attribute. */
		register struct qprojection *projptr;

		projptr = attrlist;
		for( i = 0, attrcnt = 0; i < anamecnt; i++ )
		{
			if ( strcmp( attrnames[i], "as" ) == 0 )
			{
				if ( i == 0 )
				{
					prmsg( MSG_ERROR, "no attribute name given for renaming" );
					free( rellist );
					free( nodelist );
					return( NULL );
				}
				else if ( i == anamecnt - 1 )
				{
					prmsg( MSG_ERROR, "no new name given for attribute %s",
						attrnames[i-1] );
					free( rellist );
					free( nodelist );
					return( NULL );
				}
				else if ( projptr[-1].attr == ATTR_ALL)
				{
					prmsg( MSG_ERROR, "cannot rename the \"all\" attribute" );
					free( rellist );
					free( nodelist );
					return( NULL );
				}

				projptr[-1].prname = attrnames[++i];
				continue;
			}
			else if ( *attrnames[i] == ':' ) /* attr modifiers */
			{
				if ( i == 0 )
				{
					prmsg( MSG_ERROR, "no attribute name given before attribute modifiers" );
					free( rellist );
					free( nodelist );
					return( NULL );
				}

				if ( ! attr_modifier( &projptr[-1],
						attrnames[i], TRUE ) )
				{
					free( rellist );
					free( nodelist );
					return( NULL );
				}
				continue;
			}

			if ( attrcnt >= MAXATT )
			{
				prmsg( MSG_ERROR, "too many projected attributes specified when making query -- max is %d",
					MAXATT );
				free( rellist );
				free( nodelist );
				return( NULL );
			}

			if ( ! lookupprojattr( attrnames[i], nodelist, relcnt,
					projptr, flags & Q_FRIENDLY ) )
			{
				prmsg( MSG_ERROR, "unrecognized projected attribute name '%s'",
					attrnames[i] );
				free( rellist );
				free( nodelist );
				return( NULL );
			}

			projptr++;
			attrcnt++;

		} /* end for each projected attribute string */
	}
	else	/* project all non-zero length attributes */
	{
		register struct uattribute *attrptr;
		register int j;

		attrcnt = 0;
		for( i = 0; i < relcnt; i++ )
		{
			for( j = 0, attrptr = rellist[i].attrs;
					attrcnt < MAXATT &&
						j < rellist[i].attrcnt;
					j++, attrptr++ )
			{
				if ( ( attrptr->prwidth != 0 ) && ( ! isupper( attrptr->justify ) ) )
				{
					attrlist[attrcnt].flags = 0;
					attrlist[attrcnt].rel = &nodelist[i];
					attrlist[attrcnt].attr = j;
					attrlist[attrcnt].prwidth = attrptr->prwidth;
					attrlist[attrcnt].justify = attrptr->justify;
					attrlist[attrcnt].prname = (flags & Q_FRIENDLY) &&
							attrptr->friendly ?
						attrptr->friendly :
						attrptr->aname;
					if ( ( attrptr->attrtype == UAT_TERMCHAR ) ||
					     ( attrptr->terminate >= DBBLKSIZE ) ) {
						attrlist[attrcnt].terminate =
							j == rellist[i].attrcnt - 1 ?
							'\t' :
							(char)attrptr->terminate;
						attrlist[attrcnt].attrwidth = 0;
					} else {
						attrlist[attrcnt].attrtype = QP_FIXEDWIDTH;
						attrlist[attrcnt].attrwidth = attrptr->terminate;
						attrlist[attrcnt].terminate = '\0';
					}
					attrlist[attrcnt].attorval = NULL;
					attrcnt++;

				} /* end if print width != 0 */
			} /* end for each attribute */
		} /* end for each relation */

		if ( attrcnt == 0 )
		{
			/*
			 * All attributes have an explicit zero length,
			 * so give them all attributes period.  We do this
			 * for backward compatability with old UNITY.
			 */
			for( i = 0; i < relcnt && attrcnt < MAXATT; i++ )
			{
				attrlist[attrcnt].flags = 0;
				attrlist[attrcnt].rel = &nodelist[i];
				attrlist[attrcnt].attr = ATTR_ALL;
				attrlist[attrcnt].prwidth = 0;
				attrlist[attrcnt].prname = NULL;
				attrlist[attrcnt].attorval = NULL;
				attrlist[attrcnt].attrwidth = 0;
				attrlist[attrcnt].attrtype = -1;
				attrlist[attrcnt].terminate = '\0';
				attrlist[attrcnt].justify = '\0';
				attrcnt++;

			} /* end for each relation */
		} /* end if all attributes have zero print width */
	} /* end else project all non-zero length attributes */

	/*
	 * Now save the sorted attributes in the projection list.
	 */
	lastsort = -1;
	sort_priority = 0;
	for( i = 0; i < sortcnt; i++ )
	{
		struct qprojection sortattr;

		if ( *sortlist[i] == ':' ) /* attr modifiers */
		{
			prmsg( MSG_ERROR, "no attribute name given before attribute modifiers in sort attributes" );
			free( rellist );
			free( nodelist );
			return( NULL );
		}

		if ( ! lookupprojattr( sortlist[i], nodelist, relcnt,
				&sortattr, FALSE ) )
		{
			prmsg( MSG_ERROR, "unrecognized sort attribute name '%s'",
				sortlist[i] );
			free( rellist );
			free( nodelist );
			return( NULL );
		}

		while( i+1 < sortcnt && sortlist[i+1][0] == ':' )
		{
			++i;
			if ( ! attr_modifier( &sortattr,
					sortlist[i], TRUE ) )
			{
				free( rellist );
				free( nodelist );
				return( NULL );
			}
		}

		if ( sortattr.flags & QP_NEWVALUE )
		{
			prmsg( MSG_ERROR, "cannot sort attributes with ':value=' modifier" );
			free( rellist );
			free( nodelist );
			return( NULL );
		}

		/*
		 * Clear out any other flag bits
		 * that are not used for sorting
		 * including attorval which is set
		 * for all:nodisplay=...
		 */
		sortattr.flags &= (QP_SORTMASK|QP_DESCENDING|QP_RMBLANKS|QP_NODISPLAY|QP_SORT);
		sortattr.attorval = NULL;

		/*
		 * Save the sort priority, independent of where
		 * the attribute appears in the projection list.
		 */
		sortattr.priority = sort_priority++;

		/*
		 * Merge the sort attribute info with the projected
		 * attribute info if this attribute was also
		 * projected; otherwise, we'll add a new "no-display"
		 * attribute.
		 */
		lastsort = findsortatt( attrlist, attrcnt, &sortattr );
		if ( lastsort < 0 )
		{
			/*
			 * This attribute was not projected or it has already
			 * been used as one of the sorted by attributes.
			 * Add it as an additional, non-displayed attribute.
			 */
			if ( attrcnt >= MAXATT )
			{
				prmsg( MSG_ERROR, "too many projected attributes specified when making query -- max is %d",
					MAXATT );
				free( rellist );
				free( nodelist );
				return( NULL );
			}

			attrlist[ attrcnt ] = sortattr;
			attrlist[ attrcnt ].flags |= QP_NODISPLAY|QP_SORT;

			/*
			 * If the attribute was projected (and to be displayed)
			 * then we need to indicate that this attribute must
			 * also be considered when checking for uniquess even
			 * though it is not to be displayed.
			 */
			if ( lastsort == -2 ) {
				attrlist[ attrcnt ].flags |= QP_SORTUNIQ;
			}

			lastsort = attrcnt;

			attrcnt++;
		}

	} /* end for each sort attribute string */

	if ( wherecnt > 0 )	/* Parse any where clause. */
	{
		qptr = fparsewhere( wherecnt, where, nodelist, relcnt, flags );
		if ( qptr == NULL )
		{
			(void)pruerror( );
			prmsg( MSG_ERROR, "unsuccessful parse for where-clause" );
			free( rellist );
			free( nodelist );
			return( NULL );
		}
	}
	else
		qptr = NULL;

	/*
	 * Build the query itself.  The attribute list will be
	 * malloc()'ed as part of expprojlist().  We don't have to
	 * worry that it's just space on the stack.
	 */
	query = fbldquery( nodelist, relcnt, attrlist, attrcnt, qptr, flags );
	if (  query == NULL )
	{
		(void)pruerror( );
		prmsg( MSG_ERROR, "query compilation failed" );
		free( rellist );
		free( nodelist );
		return( NULL );
	}

	return( query );
}
