/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define NULL	((char *)0)

extern char *strpbrk();
extern int strspn();

cntsflds( str, delim )
char *str;
char *delim;
{
	register char *next;
	register int count;

	count = 1;
	while( *str && ( next = strpbrk( str, delim ) ) != NULL )
	{
		count++;
		str = next + strspn( next, delim );
	}

	return( count );
}
