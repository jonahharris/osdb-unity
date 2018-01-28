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
 * This file prints an expression tree as a readable string.
 */

static void
oneexpr( fp, qptr, do_newline )
FILE *fp;
struct queryexpr *qptr;
char do_newline;
{
	if ( ISCOMPARE( qptr->optype ) ) {
		/*
		 * Print the compare operation
		 */
		prcmp( fp, qptr );
		if ( do_newline )
			fputs( "\n\t", fp );

		return;
	}

	/*
	 * Check for negations.
	 */
	if ( qptr->optype & (OPNOT|OPANTIJOIN) ) {
		if ( qptr->optype & OPNOT )
			fputs( "! ( ", fp );
		if ( qptr->optype & OPANTIJOIN )
			fputs( "- ( ", fp );
	}
	else if ( qptr->opflags & SUBEXPR )
		fputs( "( ", fp );

	oneexpr( fp, qptr->elem1.expr, do_newline );

	/*
	 * Print out the operation
	 */
	switch( qptr->optype & ~(OPNOT|OPANTIJOIN) ) {
	case OPOR:
		fputs( "or ", fp );
		break;
	case OPAND:
		fputs( "and ", fp );
		break;
	case OPELSE:
		fputs( "else ", fp );
		break;
	default:
		fputs( "**unknown bool** ", fp );
	}

	oneexpr( fp, qptr->elem2.expr, do_newline );

	/*
	 * Check for subexpressions and negations.
	 */
	if ( (qptr->optype & (OPNOT|OPANTIJOIN)) ||
			(qptr->opflags & SUBEXPR) )
		fputs( ") ", fp );
}

void
prexpr( fp, qptr, nlflag )
FILE *fp;
struct queryexpr *qptr;
int nlflag;
{
	if ( fp == NULL )
		fp = stderr;

	if ( qptr ) {
		oneexpr( fp, qptr, nlflag );
		putc( '\n', fp );
	}
}
