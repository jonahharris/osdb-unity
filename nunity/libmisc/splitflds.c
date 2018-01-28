/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define NULL ((char *)0)

extern char *strpbrk();

splitflds( fldlist, maxflds, str, delim )
register char *fldlist[];
register char *str;
int maxflds;
char *delim;
{
	register char *next;
	register int count;
	register int rc;

	count = 0;

	while( count < maxflds - 1 && *str &&
			( next = strpbrk( str, delim ) ) != NULL ) {
		fldlist[count++] = str;
		*next = '\0';
		str = next + 1;
	}

	if ( *str ) {
		fldlist[count++] = str;
		if ( (next = strpbrk( str, delim ) ) != NULL )
			*next = '\0';
	}

	rc = count;
	while( count < maxflds )
		fldlist[count++] = NULL;

	return( rc );
}
