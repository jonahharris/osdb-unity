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

static void
setdelim( delim, deflt, lastattr, last_inrel, attrterm, termch )
short *delim;
char deflt;
int lastattr;
int last_inrel;
int attrterm;
char termch;
{
	*delim = lastattr ? '\n' :
		deflt != 0 ? deflt :
		attrterm && last_inrel ? '\t' :
		attrterm ? (unsigned char)termch :
		EOF;
}

int
prtuples( output, query, result, delim )
FILE *output;
register struct uquery *query;
struct qresult *result;
char delim;
{
	char *tptr[MAXATT];
	register int i, acnt;
	register struct qnode *nodeptr;
	register struct qprojection *refptr;
	short attrdelim[MAXATT];
	int tcnt;

	if ( ! initresult( result ) )
		return( 0 );

	for( i = 0, acnt = 0; i < query->attrcnt && acnt < MAXATT; i++ ) {
		refptr = &query->attrlist[i];
		nodeptr = refptr->rel;
		switch( refptr->attr ) {
		case ATTR_ALL:
		{
			register struct uattribute *attrptr, *endattr;

			endattr = &nodeptr->rel->attrs[nodeptr->rel->attrcnt - 1];
			for( attrptr = nodeptr->rel->attrs; attrptr <= endattr && acnt < MAXATT;
					attrptr++ ) {

				setdelim( &attrdelim[acnt], delim,
					acnt == MAXATT - 1 ||
						( i == query->attrcnt - 1 &&
						attrptr == endattr ),
					attrptr == endattr,
					attrptr->attrtype == UAT_TERMCHAR,
					(char)attrptr->terminate );
				acnt++;
			}
			break;
		}
		case ATTR_RECNUM:
		case ATTR_SEEK:
			setdelim( &attrdelim[acnt], delim,
				acnt == MAXATT - 1 || i == query->attrcnt - 1,
				FALSE, TRUE, ':' );
			acnt++;
			break;
		default:	/* normal attribute */
			setdelim( &attrdelim[acnt], delim,
				acnt == MAXATT - 1 || i == query->attrcnt - 1,
				refptr->attr == nodeptr->rel->attrcnt - 1,
				nodeptr->rel->attrs[refptr->attr].attrtype == UAT_TERMCHAR,
				(char)nodeptr->rel->attrs[refptr->attr].terminate );
			acnt++;
			break;
		}
	}

	tcnt = 0;
	while( nexttuple( query, result, tptr ) ) {
		++tcnt;
		for( i = 0; i < acnt; i++ ) {
			fputs( tptr[i], output );
			if ( attrdelim[i] != EOF )
				putc( (char)attrdelim[i], output );
		}
	}

	return( tcnt );
}
