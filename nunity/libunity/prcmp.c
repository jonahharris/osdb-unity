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

extern char *basename();

prattr( output, attr )
FILE *output;
struct attrref attr;
{
	fputs( basename( attr.rel->rel->path ), output );
	switch( attr.attr ) {
	case ATTR_ALL:
		fputs( ".all ", output );
		break;
	case ATTR_RECNUM:
		fputs( ".rec# ", output );
		break;
	case ATTR_SEEK:
		fputs( ".seek# ", output );
		break;
	default:	/* normal attribute */
		if ( attr.attr >= 0 && attr.attr < attr.rel->rel->attrcnt )
			fprintf( output, ".%s ", attr.rel->rel->attrs[attr.attr].aname );
		else
			fprintf( output, ".**unknown attr (%d), max (%d)**", attr.attr,
				attr.rel->rel->attrcnt );
		break;
	}
}

properand( output, opflags, cmptype, attr, elem )
FILE *output;
unsigned char opflags;
QCMPTYPE cmptype;
unsigned char attr;
union qoperand *elem;
{
	if ( opflags & attr )
		prattr( output, elem->attr );
	else {
		switch( cmptype ) {
		case QCMP_NUMBER:
			fprintf( output, "\"%g\" ", elem->numval );
			break;
		case QCMP_STRING:
		case QCMP_CASELESS:
		case QCMP_REGEXPR:
		case QCMP_DATE:
		case QCMP_DATEONLY:
			putc( '"', output );
			cstrprt( output, elem->strval );
			putc( '"', output );
			putc( ' ', output );
			break;
		default:
			fputs( "**unknown type**", output );
		}
	}
}

prcmp( output, qptr )
FILE *output;
struct queryexpr *qptr;
{
	if ( qptr->optype & OPANTIJOIN )
		fputs( " - ", output );

	if ( ISSETCMP( qptr->optype ) ) {
		register int i;
		register struct attrref *refptr;
		register char **strlist;

		if ( qptr->optype & OPNOT )
			fputs( "not ", output );

		refptr = qptr->elem1.alist.list;
		i = qptr->elem1.alist.cnt;
		while( i-- > 0 )
			prattr( output, *refptr++ );

		switch( qptr->cmptype ) {
		case QCMP_NUMBER:
			putc( 'n', output );
			break;
		case QCMP_STRING:
			break;
		case QCMP_CASELESS:
			putc( 'c', output );
			break;
		case QCMP_REGEXPR:
			putc( 'r', output );
			break;
		case QCMP_DATE:
			putc( 'd', output );
			break;
		case QCMP_DATEONLY:
			putc( 'D', output );
			break;
		default:
			fputs( "**unknown type**", output );
		}

		fputs( "in '", output );

		strlist = qptr->elem2.strlist;
		if ( qptr->elem1.alist.cnt == 1 ) {
			while( *strlist ) {
				fprintf( output, "%s%s", *strlist,
					strlist[1] ? "," : "" );
				++strlist;
			}
		}
		else {
			i = 1;
			putc( '(', output );
			while( *strlist ) {
				fprintf( output, "%s%s", *strlist,
					i++ % qptr->elem1.alist.cnt != 0 ?
						strlist[1] ? "," : "]]" :
					strlist[1] ? "),(" : ")" );
				++strlist;
			}
		}
		putc( '\'', output );
		putc( ' ', output );

		return;
	}

	properand( output, qptr->opflags, qptr->cmptype, ISATTR1,
		&qptr->elem1 );

	if ( (qptr->opflags & (ISATTR1|ISATTR2)) == (ISATTR1|ISATTR2) )
		putc( 'f', output );

	if ( qptr->opflags & OPOUTERJOIN )
	{
		if ( qptr->opflags & OPOJDIRECT )
		{
			putc( 'O', output );
		}
		else
		{
			putc( 'o', output );
		}
	}

	switch( qptr->cmptype ) {
	case QCMP_NUMBER:
		putc( 'n', output );
		break;
	case QCMP_STRING:
		break;	/* don't do anything */
	case QCMP_CASELESS:
		putc( 'c', output );
		break;
	case QCMP_REGEXPR:
		putc( 'r', output );
		break;
	case QCMP_DATE:
		putc( 'd', output );
		break;
	case QCMP_DATEONLY:
		putc( 'D', output );
		break;
	default:
		fputs( "**unknown type**", output );
	}

	switch( qptr->optype & ~OPANTIJOIN ) {
	case OPEQ:
		fputs( "= ", output );
		break;
	case OPNE:
		fputs( "!= ", output );
		break;
	case OPLT:
		fputs( "< ", output );
		break;
	case OPLE:
		fputs( "<= ", output );
		break;
	case OPGT:
		fputs( "> ", output );
		break;
	case OPGE:
		fputs( ">= ", output );
		break;
	default:
		fputs( "**unknown compare** ", output );
	}

	properand( output, qptr->opflags, qptr->cmptype, ISATTR2,
		&qptr->elem2 );
}
