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
#include "uerror.h"
#include "permission.h"
#include "message.h"

extern char *cpdirname( );
extern char *basename( );

static int
idx_exist( pathname, filename )
register char *pathname;
register char *filename;
{
	int rc;

	/*
	 * An index "exists" if either of the index files exists.
	 */
	if ( chkperm( pathname, P_EXIST ) )
		return( TRUE );

	/*
	 * A<table>.<attribute> file doesn't exist.  Now check the
	 * existence of the B<table>.<attribute> file.
	 */
	*filename = 'B';
	rc = chkperm( pathname, P_EXIST );

	*filename = 'A';

	return( rc );
}

updindex( relptr, msgs )
register struct urelation *relptr;
{
	char chk_path;
	int rc;
	char *filename;
	char *attrname;
	char *cmdptr;
	char idxpath[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char cmdbuf[1024];

	/*
	 * Check permission on directory where index will be
	 * created.  We try to place the index in the relation's
	 * directory first.  Then we try the current directory.
	 */
	uid = geteuid();
	gid = getegid();

	filename = cpdirname( idxpath, relptr->path );
	if ( filename != idxpath ) {	/* not in current directory */
		chk_path = TRUE;

		if ( ! chkperm( idxpath, P_WRITE, uid, gid ) ) {
			filename = idxpath;
		}
		else {
			sprintf( cmdbuf, "cd %s; ", idxpath );
			cmdptr = cmdbuf + strlen( cmdbuf );

			*filename++ = '/';
		}
	}
	else
		chk_path = FALSE;

	if ( filename == idxpath ) {	/* check current directory */
		if ( ! chkperm( ".", P_WRITE, uid, gid ) ) {
			if ( msgs )
				prmsg( MSG_ERROR, "%scurrent directory not writeable -- cannot update indexes for relation '%s'",
					chk_path ? "relation's directory and" :
						"",
					relptr->path );
			set_uerror( UE_WOPEN );
			return( FALSE );
		}

		cmdbuf[0] = '\0';
		cmdptr = cmdbuf;
	}

	sprintf( filename, "A%s.", basename( relptr->path ) );
	attrname = filename + strlen( filename );

	/*
	 * Go through the relation's attributes and update all the indexes
	 * that need it.
	 */
	rc = TRUE;
	for( i = 0; i < relptr->attrcnt; i++ ) {

		strcpy( attrname, relptr->attrs[i].aname );
#if 0
		filename[14] = '\0';	/* max size of file name */
#endif

		chk_path = FALSE;
		if ( filename != idxpath ) {	/* relation not in cur. dir. */
			if ( idx_exist( idxpath, filename ) )
				chk_path = TRUE;
		}
		if ( ! chk_path && idx_exist( filename, filename ) )
			chk_path = TRUE;

		if ( ! chk_path )
			continue;	/* index doesn't exist */

		sprintf( cmdptr, "index %s in %s", relptr->attrs[i].aname,
			relptr->path );
		if ( system( cmdbuf ) != 0 ) {
			if ( msgs )
				prmsg( MSG_ERROR, "cannot update index on attribute %s in relation '%s'",
					relptr->attrs[i].aname, relptr->path );
			set_uerror( UE_IDXFAIL );
			rc = FALSE;
		}
	}

	return( rc );
}
