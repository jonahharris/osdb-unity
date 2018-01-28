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
#include "urelation.h"
#include "uerror.h"

int
packet( fp, relptr, attrvals, recnum, friendly, newline )
FILE *fp;
struct urelation *relptr;
char **attrvals;
long recnum;
char friendly;
char newline;
{
	register int i;
	register struct uattribute *attrptr;
	register char *ptr;

	fprintf( fp, "#####\t%lu\n", recnum );
	for( i = 0, attrptr = relptr->attrs; i < relptr->attrcnt;
		i++, attrptr++ )
	{
		ptr = friendly && attrptr->friendly && *attrptr->friendly ?
				attrptr->friendly : attrptr->aname;
		while( *ptr )
		{
			if ( *ptr == '\t' )
			{
				set_uerror( UE_HASTAB );
				return( FALSE );
			}
			putc( *ptr++, fp );
		}
		putc( '\t', fp );

		if ( attrptr->attrtype == UAT_TERMCHAR && newline != '\0' )
		{
			for( ptr = attrvals[i]; *ptr; ptr++ )
			{
				if ( *ptr == newline )
				{
					putc( '\n', fp );
					putc( '\t', fp );
				}
				else
					putc( *ptr, fp );
			}
		}
		else
			fputs( attrvals[i], fp );

		putc( '\n', fp );
	}
	putc( '\n', fp );

	if ( ferror( fp ) )
	{
		set_uerror( UE_WRITE );
		return( FALSE );
	}

	return( TRUE );
}
