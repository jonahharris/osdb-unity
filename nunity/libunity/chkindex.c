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
#include "permission.h"
#include "urelation.h"

extern char *cpdirname( );
extern char *basename( );
extern unsigned short geteuid(), getegid();

static int
chkifile( pathname, filename, relstat, uid, gid )
register char *pathname;
register char *filename;
register struct stat *relstat;
unsigned short uid;
unsigned short gid;
{
	struct stat idxstat;
	int rc;

	if ( ! chkperm( pathname, P_READ, uid, gid ) )
		return( FALSE );

	/*
	 * A<table>.<attribute> file is readable.  Now check the
	 * status and permissions on B<table>.<attribute> file.
	 *
	 * Unlike official UNITY commands, the index file is not
	 * updated if it's out of date, because we don't know if the
	 * attribute is needed yet.  (We're also lazy and planning
	 * to redo indexes, anyway.)
	 */
	*filename = 'B';
	rc = ( chkperm( pathname, P_READ, uid, gid ) &&
		stat( pathname, &idxstat ) == 0 &&
		idxstat.st_mtime >= relstat->st_mtime );

	*filename = 'A';

	return( rc );
}

int
chkindex( relname, relstat, attrname )
char *relname;
struct stat *relstat;
char *attrname;
{
	char pathname[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	register char *filename, *str;
	unsigned short uid;
	unsigned short gid;

	uid = geteuid();
	gid = getegid();
	filename = cpdirname( pathname, relname );
	str = basename( relname );
	if ( filename != pathname ) {		/* relation not in cur. dir. */
		sprintf( filename, "/A%s.%s", str, attrname );

		filename++;			/* move past '/' */

#if 0
		filename[14] = '\0';	/* make sure index name is not too long */
#endif

		if ( chkifile( pathname, filename, relstat, uid, gid ) )
			return( TRUE );
	}
	else {
		sprintf( filename, "A%s.%s", str, attrname );
#if 0
		filename[14] = '\0';	/* make sure index name is not too long */
#endif
	}

	return( chkifile( filename, filename, relstat, uid, gid ) );
}
