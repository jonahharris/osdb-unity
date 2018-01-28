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
#include <stdarg.h>

/*VARARGS1*/
FILE *
starttable( char *options, int cnt,  ... )
{
	register int len;
	register int just;	/* really a character */
	register char *strptr, *name;
	va_list arg;
	char cmdline[1024];

	va_start( arg, options );

	if ( cnt <= 0 )
		return( NULL );

	fflush( stdout );
	sprintf( cmdline, "prtable %s", options );
	strptr = &cmdline[strlen( cmdline )];

	while( cnt-- > 0 ) {
		len = va_arg( arg, int );
		just = va_arg( arg, int );
		name = va_arg( arg, char * );
		sprintf( strptr, " -%c%d \"%s\"", just, len, name );
		strptr += strlen( strptr );
	}

	return( popen( cmdline, "w" ) );
}

endtable( fp )
FILE *fp;
{
	return( pclose( fp ) );
}
