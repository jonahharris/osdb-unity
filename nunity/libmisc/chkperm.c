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
#ifdef	__STDC__
#include <unistd.h>		/* getgroups(2) */
#include <limits.h>		/* NGROUPS_MAX  */
#endif
#ifndef NGROUPS_MAX
#include <sys/param.h>
#define NGROUPS_MAX NGROUPS
#endif

#define OWNER		6	/* bits to shift for owner permissions */
#define GROUP		3	/* bits to shift for group permissions */

#define MODEMASK	(P_READ|P_WRITE|P_EXEC)	/* mask for only perm's in mode */

#ifndef NULL
#define NULL	((char *)0)
#endif

extern char *getenv();
extern char *strchr();

char *packedcatcmd;	/* command to read (cat) a packed file (relation) */
char *packedsuffix;	/* packed filename suffix without the dot */

/*
 * This function checks whether a given file can be opened for the given
 * mode by a user having the given uid and gid.  The routine emulates the
 * permission checking done by UNIX, if the an open were attempted on the
 * given file for a user having the given **effective** uid and gid.
 * The function returns TRUE if the operation is allowed, FALSE on errors or
 * if the operation is not allowed.  The user must be able to stat(2) the
 * given file or FALSE will always be returned.
 *
 *  This routine is essentially an enhanced version of access(2).
 * However, it can check a file with respect to effective uid/gid, rather
 * than just real uid/gid.
 *
 * The permissions are evaluated according to the following three rules:
 *
 *	1.  If the uids match, the owner permissions are used to evaluate
 *	    the operation.
 *	2.  If the uids don't match, but the gids do, the group permissions
 *	    are used to evaluate the operation.
 *	3.  If the uids and gids do not match, the "other" permissions are
 *	    used to evaluate the operation.
 *	4.  If "other" permissions fail (do not match given mode) and __STDC__
 *	    is defined then getgroups(2) is called get a list of group IDs
 *	    that the user has supplementary group access to.  If the group ID
 *	    of the given directory or file is found in the list of group IDs
 *	    the operation is evaluated based on the group permissions.  Fail
 *	    is returned if the group ID is not found in the list of group IDs.
 */
chkperm( file, mode, uid, gid )
char *file;
register ushort mode;
register ushort uid;
register ushort gid;
{
	struct stat buf;

	if ( file == NULL || *file == '\0' )
		file = ".";

	if ( stat( file, &buf ) < 0 )
	{
		if ( ( mode != P_EXIST && mode != P_READ ) ||
		     ( packedsuffix == NULL ) ||
		     ( statpacked( file, &buf ) ) )
		{
			return( 0 );
		}
	}

	if ( mode == P_EXIST )
		return( 1 );

	mode &= MODEMASK;
	if ( uid == buf.st_uid ) {
		mode <<= OWNER;
		return( (buf.st_mode & mode) == mode );
	}
	else if ( gid == buf.st_gid ) {
		mode <<= GROUP;
		return( (buf.st_mode & mode) == mode );
	}
	if ( (buf.st_mode & mode) == mode )
		return( 1 );
#if defined(__STDC__) || defined(SUPGRP)
#ifdef	NGROUPS_MAX
	{
		int	i;
#ifdef	__STDC__
		gid_t	grouplist[NGROUPS_MAX];
#else
		int	grouplist[NGROUPS_MAX];
#endif
		mode <<= GROUP;
		i = getgroups( NGROUPS_MAX, grouplist );
		while ( i > 0 ) {
			if ( buf.st_gid == grouplist[--i] )
				return( (buf.st_mode & mode) == mode );
		}
	}
#endif
#endif
	return( 0 );
}

statpacked( file, statbufptr )
char *file;
struct stat *statbufptr;
{
	int rc;
	struct stat buf;
	char packedfile[ MAXPATH + 8 ];	/* +4 for "/./" or "././" prefix */
					/* +4 for ".<suffix>"	*/

	if ( file == NULL || *file == '\0' )
		return( 1 );

	if ( statbufptr == NULL )
		statbufptr = &buf;

	if ( ( packedcatcmd ) &&
	     ( packedsuffix ) &&
	     ( *packedsuffix ) &&
	     ( strlen( packedsuffix ) <= 3 ) &&
	     ( strchr( packedsuffix, '.' ) == NULL ) &&
	     ( strchr( packedsuffix, '/' ) == NULL ) &&
	     ( strlen( file ) < MAXPATH + 4 ) )
	{
		sprintf( packedfile, "%s.%s", file, packedsuffix );

		if (( rc = stat( packedfile, statbufptr ) ) < 0 ) {
			return( rc );
		} else {
			return( 0 );
		}
	} else {
		return( 1 );
	}
}

void
setunpackenv( )
{
	register char *p, *q;
	static char command[ MAXPATH ];
	static char suffix[ 4 ];

	/* Does the user want us to look for and unpack packed relations? */
	if ( ( ( p = getenv( "UNITYUNPACK" ) ) != NULL ) &&
	     ( ( q = strchr( p, '.' ) ) != NULL ) &&
	     ( q > p ) && ( q - p < MAXPATH ) &&
	     ( q[ 1 ] != '\0' ) &&
	     ( strlen( q + 1 ) <= 3 ) &&
	     ( strchr( q + 1, '.' ) == NULL ) &&
	     ( strchr( q + 1, '/' ) == NULL ) )
	{
		*q = '\0';
		strcpy( command, p );
		packedcatcmd = command;
		*q++ = '.';	/* restore the '.' in UNITYUNPACK */
		strcpy( suffix, q );
		packedsuffix = suffix;
	} else {
		packedcatcmd = NULL;
		packedsuffix = NULL;
	}
}
