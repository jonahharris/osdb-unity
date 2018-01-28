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

static char *
getcnt( str )
char *str;
{
	register char *endptr;

	endptr = str + strlen( str );
	while( endptr > str && isdigit( endptr[-1] ) )
		--endptr;

	return( endptr );
}

char *
exprange( prevstr, newstr, buf )
char *prevstr;
char *newstr;
char *buf;
{
	static short strcnt, maxcnt;
	register char *strptr, *bufptr, *cntptr;

	/*
	 * If there is no new range name given, continue to increment the
	 * current range.
	 */
	if ( newstr == NULL || *newstr == '\0' ) {
		if ( prevstr && *prevstr && strcnt < maxcnt ) {
			sprintf( buf, "%s%d", prevstr, ++strcnt );
			return( buf );
		}
		else {
			*buf = '\0';
			return( NULL );
		}
	}

	/*
	 * We have a new range.  First, copy the range name up until
	 * the first range indicator (i.e., '-' char).
	 */
	bufptr = buf;
	strptr = newstr;
	while( *strptr && *strptr != '-' )
		*bufptr++ = *strptr++;
	*bufptr = '\0';

	/*
	 * Make the first item name.  This is either the full name given
	 * in the range name or the previous name passed into the command.
	 * If there is a new name, we copy it to the previous name.
	 */
	cntptr = getcnt( buf );
	if ( *cntptr == '\0' ) {
		*buf = '\0';
		return( NULL );	/* no number given */
	}

	strcnt = atoi( cntptr );
	maxcnt = 0;
	if ( cntptr > buf ) {
		strncpy( prevstr, buf, cntptr - buf );
		prevstr[ cntptr - buf ] = '\0';
	}
	else if ( prevstr && *prevstr ) {
		if ( *strptr ) {	/* handle the '-' character */
			*strptr = '\0';
			sprintf( buf, "%s%d", prevstr, strcnt );
			*strptr = '-';
		}
		else
			sprintf( buf, "%s%d", prevstr, strcnt );
	}
	else {
		*buf = '\0';
		return( NULL );		/* no previous or current name */
	}

	if ( *strptr ) {
		cntptr = getcnt( ++strptr );
		/*
		 * If the prefix is given again, it must be the
		 * same as the original one.  If not, return NULL.
		 */
		if ( cntptr != strptr &&
				strncmp( prevstr, strptr, cntptr - strptr ) != 0 ) {
			*buf = 0;
			return( NULL );
		}
		else if ( *cntptr )
			maxcnt = atoi( cntptr );	/* end of the range */
	}

	return( buf );
}
