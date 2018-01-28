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
#include <ctype.h>

#define TRUE	1
#define FALSE	0

#define TABSIZE	8

char *
oneline( buf, maxsize, overflow )
register char *buf;
int maxsize;
int *overflow;
{
	register int is_word = FALSE;
	register char *endword = NULL;
	register int len;

	if ( overflow )
		*overflow = FALSE;

	len = 0;
	while( len < maxsize ) {
		switch( *buf ) {
		case '\0':
			return( buf );		/* end of string */

		case '\n':
		case '\r':
			return( is_word || endword == NULL ? buf : endword );

		case '\t':
			len += (TABSIZE - (len % TABSIZE)) - 1;
			/*
			 * We want to fall thru here.
			 */

		case ' ':
			/*
			 * This is a potential word boundary; mark it as such.
			 */
			len++;
			if ( is_word ) {
				endword = buf;
				is_word = FALSE;
			}
			break;

		default:
			if ( *buf < ' ' )
				len += 2;
			else
				len++;
			is_word = TRUE;
		}

		buf++;
	}

	if ( is_word && endword == NULL ) {
		/*
		 * The first word doesn't fit on the line.  End the line at the
		 * next white space.  If we care about overflow, we return the
		 * first character on the next line; otherwise, we find the
		 * first space and return that.
		 */
		if ( overflow )
			*overflow = TRUE;
		else
			while( *buf && ! isspace( *buf ) )
				++buf;
		return( buf );
	}

	return( endword );
}

char *
skip_space( ptr, allspace )
register char *ptr;
register int allspace;
{
	register int nlcnt = 0;

	/*
	 * Skip all white space and at most one new line char.
	 * If allspace is FALSE, we return immediately after any
	 * new-line.
	 */
	while( *ptr && isspace( *ptr ) ) {
		if ( *ptr == '\n' || *ptr == '\r' ) {
			if ( nlcnt++ > 0 )
				return( ptr );
			else if ( ! allspace )
				return( ptr + 1 );
		}
		++ptr;
	}

	return( ptr );
}

char *
skipspace( ptr )
register char *ptr;
{
	return( skip_space( ptr, TRUE ) );
}
