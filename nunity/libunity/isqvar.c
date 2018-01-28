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

#define REGEXP_END	'\064'	/* terminating character for reg. expr. */
#define LIT_CHAR	'\024'	/* literal charater flag for reg. expr. */

int
is_reqvar( re )
register char *re;
{
	/*
	 * Check to see if the regular expression is a variable name.
	 * A variable name will start with a '$' and will contain only
	 * literal characters (since variables can only contain char's
	 * from "a-z,A-Z,0-9,_" -- none of which are significant to
	 * regular expressions).
	 */
	if ( *re != LIT_CHAR || re[1] != '$' )
		return( FALSE );
	re += 2;
	while( *re != REGEXP_END || re[1] != '\0' ) {
		if ( *re != LIT_CHAR )
			return( FALSE );
		re += 2;
	}

	return( TRUE );
}

int
is_strqvar( str )
char *str;
{
	return( str && *str == '$' );
}

int
is_ptrqvar( cmptype, strlist )
QCMPTYPE cmptype;
char **strlist;
{
	register char **listptr;
	register int cnt;

	for( cnt = 0, listptr = strlist; cnt < 2 && listptr[0]; listptr++ )
	{
		if ( listptr[0] )
		{
			if ( ++cnt == 2 )
				return( FALSE );
		}
	}

	if ( cnt != 1 )
		return( FALSE );

	if ( cmptype == QCMP_STRING && is_strqvar( strlist[0] ) )
		return( TRUE );
	else if ( cmptype == QCMP_REGEXPR && is_reqvar( strlist[0] ) )
		return( TRUE );

	return( FALSE );
}

