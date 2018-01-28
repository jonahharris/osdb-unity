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

char *
cnvtbschar( src, dest )
char *src;
char *dest;
{
	short i;
	short cnt;

	switch( *src ) {
	case '\0':
		*dest = '\\';
		break;	/* don't increment src past null terminator */
	case 'n':
		*dest = '\n';
		src++;
		break;
	case 't':
		*dest = '\t';
		src++;
		break;
	case 'a':
		*dest = '\a';
		src++;
		break;
	case 'b':
		*dest = '\b';
		src++;
		break;
	case 'r':
		*dest = '\r';
		src++;
		break;
	case 'f':
		*dest = '\f';
		src++;
		break;
	case 'v':
		*dest = '\v';
		src++;
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
		i = 0;	/* octal number */
		cnt = 1;
		do {
			i = 8 * i + *src++ - '0';
		} while ( isdigit( *src ) && *src < '8' && cnt++ < 3 );
		*dest = (char) i;
		break;
	default:
		*dest = *src++;
		break;
	}

	return( src );
}

cnvtbslsh( startdest, src )
char *startdest;
register char *src;
{
	register char *dest;

	for( dest = startdest; *src; dest++ ) {
		if ( *src == '\\' )
			src = cnvtbschar( src + 1, dest );
		else
			*dest = *src++;
	}

	*dest = '\0';	/* null terminate the string */

	return( dest - startdest );
}
