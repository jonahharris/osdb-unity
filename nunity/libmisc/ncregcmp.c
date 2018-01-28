/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#ifdef NEED_TOUPPER
#define _tolower( ch )  (isupper( (ch) ) ? tolower( (ch) ) : (ch))
#define _toupper( ch )  (isupper( (ch) ) ? (ch) : toupper( (ch) ))
#endif

extern char *malloc();
extern char *regcmp();

char *
nc_regcmp( nocase, src )
int nocase;
register char *src;
{
	register char *dest;
	register int in_group;
	char *orig_dest;

	if ( ! nocase )
		return( regcmp( src, (char *)0 ) );

	orig_dest = malloc( 4 * strlen( src ) + 1 );
	if ( orig_dest == (char *)0 )
		return( (char *)0 );

	in_group = 0;	/* not inside [] grouping */
	dest = orig_dest;
	while( *src )
	{
		if ( isalpha( *src ) )
		{
			if ( in_group == 0 )
				*dest++ = '[';
			*dest++ = *src;
			*dest++ = islower( *src ) ? _toupper( *src ) :
					_tolower( *src );
			if ( in_group == 0 )
				*dest++ = ']';
		}
		else
		{
			*dest++ = *src;
			switch( *src )
			{
			case '\\':
				if ( src[1] )
					*dest++ = *++src;
				break;
			case '[':
				if ( in_group == 0 )
					in_group = 1;
				break;
			case ']':
				/* must not be first element of group */
				if ( in_group != 2 )
					in_group = 0;
				break;
			}
		}
		if ( in_group != 0 )
			in_group++;
		++src;
	}

	*dest = '\0';

	dest = regcmp( orig_dest, (char *)0 );

	free( orig_dest );

	return( dest );
}
