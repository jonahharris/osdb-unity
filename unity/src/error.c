/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdarg.h>
#include "db.h"

static	struct	mess	{
	int	errnum;
	char 	*errmess;
} mess[] = {
	E_DESFOPEN,	"%s: Cannot create/open the description file %s\n",
	E_DATFOPEN,	"%s: Cannot create/open the table %s\n",
	E_DATFWRITE,	"%s: Cannot write/append to the table %s\n",
	E_TEMFOPEN,	"%s: Cannot create/open the temporary table %s\n",
	E_DIRWRITE,	"%s: No write permission in directory %s\n",
	E_INDFOPEN,	"%s: Cannot create/open the index tree %s\n",
	E_ILLATTR,	"%s: %s is an illegal attribute for description file %s\n",
	E_ERR,		"%s: Error in %s\n",
	E_SPACE,	"%s: Out of space from alloc\n",
	E_NONNUMERIC,	"%s: On record %d, %s is non-numeric - %s\n",
	E_BADLINK,	"%s: link failed. Table in %s.\n",
	E_BADUNLINK,	"%s: cannot unlink table %s.\nUpdated table left in %s.\n",
	E_EXISTS,	"%s: %s already exists.\n",
	E_BADFLAG,	"%s: Unknown flag\n",
	E_LOCKSTMP,	"%s: Corrupt or missing lock file for table %s.\nUpdated table left in %s.\n",
	E_PACKREAD,	"%s: Failure reading packed table %s.%s\n",
	E_GENERAL,	""		/* must be the last entry for sentry */
};
int Unoerrpr = 0;			/* 1 => no error printing to stderr */

/*VARARGS1*/
error( int errnum, ... )
{
	int	i;
	char *format;
	va_list args;

	if ( Unoerrpr )
		return;

	format = NULL;
	va_start( args, errnum );
	if ( errnum == E_GENERAL)
		format = va_arg( args, char * );
	else
	{
		for( i = 0; mess[i].errnum != E_GENERAL; i++ )
		{
			if ( mess[i].errnum == errnum )
			{
				format = mess[i].errmess;
				break;
			}
		}
	}

	if ( format != NULL )
		vfprintf( stderr, format, args );

	va_end( args );
}
