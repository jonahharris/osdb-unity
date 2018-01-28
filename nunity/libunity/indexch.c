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
#include <sys/types.h>
#include <sys/stat.h>
#include "urelation.h"
#include "uindex.h"

extern struct uindex *bopen();
extern char *cpdirname();
extern char *basename();


static int
openifile( pathname, filename, relstat, fp, btree )
char *pathname;
char *filename;
struct stat *relstat;
FILE **fp;
struct uindex **btree;
{
	struct stat idxstat;

	if ( stat( pathname, &idxstat ) < 0 ||
			idxstat.st_mtime < relstat->st_mtime )
		return( FALSE );

	if ( (*fp = fopen( pathname, "r" )) == NULL )
		return( FALSE );

	*filename = 'B';
	if ( (*btree = bopen( pathname)) == NULL ) {
		fclose( *fp );

		return( FALSE );
	}

	return( TRUE );
}

indexch( relname, attrname, fp, btree)
char *relname;
char *attrname;
FILE **fp;
struct uindex **btree;
{
	register char *filename, *str;
	struct stat relstat;
	char pathname[MAXPATH+4];	/* allow for "/./" or "././" prefix */

	if ( stat( relname, &relstat ) < 0 )
		return( FALSE );

	filename = cpdirname( pathname, relname );
	str = basename( relname );
	if ( filename != pathname ) {		/* relation not in cur. dir. */
		sprintf( filename, "/A%s.%s", str, attrname );

		filename++;			/* move past '/' */

#if 0
		filename[14] = '\0';	/* make sure index name is not too long */
#endif

		if ( openifile( pathname, filename, &relstat, fp, btree ) )
			return( TRUE );
	}
	else {
		sprintf( filename, "A%s.%s", str, attrname );
#if 0
		filename[14] = '\0';	/* make sure index name is not too long */
#endif
	}

	return( openifile( filename, filename, &relstat, fp, btree ) );
}
