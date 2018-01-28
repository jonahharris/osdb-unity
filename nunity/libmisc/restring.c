/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <memory.h>
/*
 * Regular expression/string routines.
 * Determine the length of a compiled regular expression (the
 * output of regcmp(3)), or copy a compiled regular expression.
 *
 * Special routines are needed (as opposed to strlen() and strcpy())
 * because regular expressions can have embedded null characters
 * (and nulls mean end of string do the str*() routines).
 *
 * This code is a extremely hacked up version of the libPW regex.c
 * code; that's why the variables have such cryptic names.
 */

#define TGRP	48
#define A256	01
#define A512	02
#define A768	03
#define	NBRA	10
#define CIRCFL	32

#define	CBRA	60
#define GRP	40
#define SGRP	56
#define PGRP	68
#define EGRP	44
#define RNGE	03
#define	CCHR	20
#define	CDOT	64
#define	CCL	24
#define	NCCL	8
#define	CDOL	28
#define	CEOF	52
#define	CKET	12

#define	STAR	01
#define PLUS	02

char *advance();

regexplen( restr )
char *restr;
{
	int len = 0;

	if (*restr==CIRCFL) {
		len = 1;
	}
	return( advance( restr+len ) - restr );
}

static char *
advance(aep)
char *aep;
{
	register char *ep;

	ep = aep;
	for (;;) {
		switch(*ep++) {
	
		/* end of regular expression */
		case CEOF:
			return(ep);

		case CDOT:		/* dot - arbitrary character */
		case CDOT|PLUS:
		case CDOT|STAR:
		case CDOL:		/* dollar sign - end of string */
			break;
	
		case CCHR:		/* literal character */
		case CCHR|PLUS:
		case CCHR|STAR:
		case EGRP|STAR:		/* end of a group */
		case EGRP|PLUS:
		case EGRP:
		case GRP:
 		case CBRA:		/* beginning of group followed by $N */
			ep++;
			break;
	
		case CDOT|RNGE:		/* arbitrary character (dot) followed by a range */
		case CKET:		/* $N at end of group */
			ep += 2;
			break;

		case CCHR|RNGE:		/* character followed by a range */
			ep += 3;
			break;
	
		case CCL:		/* character class */
		case NCCL:
		case CCL|PLUS:		/* character class followed by a plus */
		case NCCL|PLUS:
		case CCL|STAR:		/* character class followed by a star */
		case NCCL|STAR:
			ep += *ep;
			break;
	
		case PGRP:		/* start of a group followd by a plus */
		case PGRP|A256:
		case PGRP|A512:
		case PGRP|A768:
		case SGRP|A768:		/* start of a group followd by a star */
		case SGRP|A512:
		case SGRP|A256:
		case SGRP:
			ep += *ep + 1;
			break;

		case CCL|RNGE:		/* character class followed by a range */
		case NCCL|RNGE:
			ep += *ep + 2;
			break;
	

		case TGRP:		/* start of a group () followed by a range */
		case TGRP|A768:
		case TGRP|A512:
		case TGRP|A256:
		case EGRP|RNGE:		/* end of a group followed by a {x,y} range */
			ep += 3;
			break;
		}
	}
}

regexpcpy( dest, src )
char *dest;
char *src;
{
	register int len;

	len = regexplen( src );
	(void) memcpy( dest, src, len );
	return( len );
}
	
