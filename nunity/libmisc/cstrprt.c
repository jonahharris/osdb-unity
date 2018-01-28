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

char *
escape_char( ch, pad )
char ch;
char pad;
{
	static char buf[5];

	switch( ch ) {
	case '\n':
		return( "\\n" );
	case '\r':
		return( "\\r" );
	case '\t':
		return( "\\t" );
	case '\f':
		return( "\\f" );
	case '\\':
		return( "\\\\" );
	case '\'':
		return( "\\'" );
	case '\"':
		return( "\\\"" );
	default:
		if ( isprint( ch ) )
		{
			buf[0] = ch;
			buf[1] = '\0';
		}
		else if ( ! pad )
		{
			sprintf( buf, "\\%o", ch );
		}
		else
		{
			/*
			 * We must use 3 digits in the string so that
			 * if a digit follows this char, it is not
			 * ambiguous what the string means.
			 */
			sprintf( buf, "\\%03o", ch );
		}

		return( buf );
	}
}

cstrnprt( fp, str, len )
FILE *fp;
register char *str;
register int len;
{
	for( ; len > 0; --len, str++ )
	{
		fputs( escape_char( *str, len > 1 && isdigit( str[1] ) ), fp );
	}
}

cstrprt( fp, str )
FILE *fp;
register char *str;
{
	while( *str )
	{
		fputs( escape_char( *str, isdigit( str[1] ) ), fp );
		++str;
	}
}
