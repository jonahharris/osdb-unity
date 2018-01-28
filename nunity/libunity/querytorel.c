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
#include "udbio.h"

extern char *malloc();

struct urelation *
querytorel( query, relptr, path, descpath, flags )
struct uquery *query;
struct urelation *relptr;
char *path;
char *descpath;
char flags;
{
	register struct qprojection *refptr;
	register struct uattribute *newattr;
	register int i, attrcnt;
	struct uattribute attrlist[ MAXATT ];

	newattr = attrlist;
	attrcnt = 0;
	for( i = 0, refptr = query->attrlist; i < query->attrcnt;
		i++, refptr++ )
	{
		if ( refptr->flags & QP_NODISPLAY )
			continue;

		if ( attrcnt >= MAXATT )
		{
			set_uerror( UE_NUMATTR );
			freeattrlist( attrlist, attrcnt );
			return( NULL );
		}
		attrcnt++;

		switch( refptr->attr ) {
		case ATTR_RECNUM:
			if ( strcmp( refptr->prname, "rec#" ) == 0 )
				refptr->prname = "recnum";
			strncpy( newattr->aname, refptr->prname, ANAMELEN );
			if ( ( refptr->attrtype == QP_FIXEDWIDTH ) &&
			     ( refptr->attrwidth < DBBLKSIZE ) ) {
				newattr->attrtype = UAT_FIXEDWIDTH;
				newattr->terminate = refptr->attrwidth;
			} else {
				newattr->attrtype = UAT_TERMCHAR;
				newattr->terminate = refptr->terminate;
			}
			newattr->justify = refptr->justify;
			newattr->friendly = "Record Number";
			newattr->prwidth = refptr->prwidth;
			break;
		case ATTR_SEEK:
			if ( strcmp( refptr->prname, "seek#" ) == 0 )
				refptr->prname = "seekval";
			strncpy( newattr->aname, refptr->prname, ANAMELEN );
			if ( ( refptr->attrtype == QP_FIXEDWIDTH ) &&
			     ( refptr->attrwidth < DBBLKSIZE ) ) {
				newattr->attrtype = UAT_FIXEDWIDTH;
				newattr->terminate = refptr->attrwidth;
			} else {
				newattr->attrtype = UAT_TERMCHAR;
				newattr->terminate = refptr->terminate;
			}
			newattr->justify = refptr->justify;
			newattr->friendly = "Seek Value";
			newattr->prwidth = refptr->prwidth;
			break;
		default:	/* normal attribute */
			*newattr = refptr->rel->rel->attrs[ refptr->attr ];
			newattr->prwidth = refptr->prwidth;
			newattr->justify = refptr->justify;
			if ( ( refptr->attrtype == QP_FIXEDWIDTH ) &&
			     ( refptr->attrwidth < DBBLKSIZE ) ) {
				newattr->attrtype = UAT_FIXEDWIDTH;
				newattr->terminate = refptr->attrwidth;
			} else {
				newattr->attrtype = UAT_TERMCHAR;
				newattr->terminate = refptr->terminate;
			}
			if ( refptr->prname != newattr->aname )
				strncpy( newattr->aname, refptr->prname,
					ANAMELEN );
			break;
		}

		/* Do we keep the original friendly attribute name ? */
		if ( refptr->flags & QP_NOVERBOSE ) {
			if ( ( ( refptr->flags & QP_NEWVALUE) == 0 ) &&
			     ( refptr->attorval ) ) {
				newattr->friendly = refptr->attorval;
			} else {
				newattr->friendly = NULL;
			}
		}

		if ( newattr->friendly != NULL )
		{
			char *oldstr;

			oldstr = newattr->friendly;
			newattr->friendly = malloc( strlen( oldstr ) + 1 );
			if ( newattr->friendly == NULL )
			{
				set_uerror( UE_NOMEM );
				freeattrlist( attrlist, attrcnt );
				return( NULL );
			}
			strcpy( newattr->friendly, oldstr );
		}

		newattr++;
	}

	if ( attrcnt == 0 )
		return( NULL );

	if ( attrlist[ attrcnt - 1 ].attrtype == UAT_TERMCHAR )
		attrlist[ attrcnt - 1 ].terminate = '\n';

	if ( relptr == NULL )
	{
		relptr = (struct urelation *)malloc( sizeof( *relptr ) );
		if ( relptr == NULL )
		{
			freeattrlist( attrlist, attrcnt );
			set_uerror( UE_NOMEM );
			return( NULL );
		}
		relptr->relflgs = flags | UR_RELALLOC;
	}
	else
		relptr->relflgs = flags;

	relptr->attrcnt = attrcnt;
	relptr->attrs = NULL;
	relptr->flags = 0;
	relptr->path = NULL;
	relptr->dpath = NULL;

	if ( path != NULL )
	{
		relptr->path = malloc( strlen( path ) + 1 );
		if ( relptr->path == NULL )
		{
			set_uerror( UE_NOMEM );
			freerelinfo( relptr );
			return( NULL );
		}
		strcpy( relptr->path, path );
	}

	if ( descpath != NULL )
	{
		relptr->dpath = malloc( strlen( descpath ) + 1 );
		if ( relptr->dpath == NULL )
		{
			set_uerror( UE_NOMEM );
			freerelinfo( relptr );
			return( NULL );
		}
		strcpy( relptr->dpath, descpath );
	}
	else
		relptr->dpath = NULL;

	newattr = (struct uattribute *)malloc( (unsigned)( attrcnt * sizeof( *newattr ) ) );
	if ( newattr == NULL )
	{
		freerelinfo( relptr );
		set_uerror( UE_NOMEM );
		return( FALSE );
	}

	relptr->attrs = newattr;
	while( --attrcnt >= 0 )
		newattr[ attrcnt ] = attrlist[ attrcnt ];

	return( relptr );
}
