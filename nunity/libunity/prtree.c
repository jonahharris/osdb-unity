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

/*
 * This file prints an expression tree showing the tree structure.
 */

#define FIELDLEN	6	/* length of each field when printing */

static char offset[1024];
static int offlen;

static void
onetree( fp, qptr, invert )
FILE *fp;
struct queryexpr *qptr;
int invert;
{
	register int len;

	len = 0;

	if ( ISCOMPARE( qptr->optype ) ) {
		prcmp( fp, qptr );
		putc( '\n', fp );
		return;
	}

	/*
	 * Check for subexpressions and negations.
	 */
	if ( qptr->optype & OPNOT )
	{
		putc( '!', fp );
		++len;
	}

	if ( qptr->optype & OPANTIJOIN )
	{
		putc( '-', fp );
		++len;
	}

	/*
	 * Print out the operation
	 */
	switch( qptr->optype & ~(OPNOT|OPANTIJOIN) ) {
	case OPOR:
		fputs( "or", fp );
		len += 2;
		break;
	case OPAND:
		fputs( "and", fp );
		len += 3;
		break;
	case OPELSE:
		fputs( "else", fp );
		len += 4;
		break;
	default:
		fputs( "???", fp );
		len += 3;
	}

	while( len++ < FIELDLEN )
		putc( '-', fp );
	strcat( offset, " |    " );	/* length must = FIELDLEN */
	offlen += FIELDLEN;

	onetree( fp, invert ? qptr->elem1.expr : qptr->elem2.expr, invert );

	offset[offlen - FIELDLEN] = '\0';   /* get rid of "  |     " */
	fprintf( fp, "%s |\n%s |----", offset, offset );
	strcat( offset, "      " );	/* length must = FIELDLEN */

	onetree( fp, invert ? qptr->elem2.expr : qptr->elem1.expr, invert );

	offlen -= FIELDLEN;
	offset[offlen] = '\0';		/* get rid of "        " */
}

void
prtree( fp, qptr, invert )
FILE *fp;
struct queryexpr *qptr;
int invert;
{
	if ( fp == NULL )
		fp = stderr;

	if ( qptr ) {
		offset[0] = '\0';
		offlen = 0;

		onetree( fp, qptr, invert );
		putc( '\n', fp );
	}
}
