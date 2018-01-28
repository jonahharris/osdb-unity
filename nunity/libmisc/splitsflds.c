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

extern char *strtok();

splitsflds( fldlist, maxflds, str, delim )
register char *fldlist[];
char *str;
int maxflds;
char *delim;
{
	register char *next;
	register int count;
	register int rc;

	count = 0;
	next = strtok( str, delim );
	while( count < maxflds && next ) {
		fldlist[count++] = next;
		next = strtok( NULL, delim );
	}

	rc = count;
	while( count < maxflds )
		fldlist[count++] = NULL;

	return( rc );
}
