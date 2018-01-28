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
#include <signal.h>
#include <errno.h>
#include "urelation.h"
#include "uerror.h"
#include "permission.h"

extern char *cpdirname(), *mktemp(), *strcpy(), *strpbrk();

start_transact( relptr, lockfile )
struct urelation *relptr;
char *lockfile;
{
	int fd, rc;
	char *str;
	RETSIGTYPE (*sigint)(), (*sigquit)(), (*sighup)(), (*sigterm)();
	char tempfile[ MAXPATH+4 ];	/* allow for "/./" or "././" prefix */

#ifndef TMPLOCK
	/*
	 * When not using locks in /tmp, lock names are
	 * based on the relation name, so check for a lock
	 * first.
	 * If we don't check first, we could come in while
	 * another process is doing it's unlink/link.
	 */
	rc = mklock( relptr->path, lockfile );
	if ( ! rc )
		return( rc );
#endif

	/*
	 * If packed relations are allowed then do not allow
	 * any transations when a packed relation is present.
	 */
	if ( statpacked( relptr->path, NULL ) == 0 ) {
		set_uerror( UE_UNPACKMOD );
		return( 0 );
	}

	/*
	 * Create the table if necessary.
	 */
	if ( ! chkperm( relptr->path, P_EXIST ) )
	{
		/*
		 * Got some touchy code coming up; ignore all the signals.
		 * No interruptions allowed while we're linking and
		 * unlinking the files.
		 */
		sigint = signal( SIGINT, SIG_IGN );
		sighup = signal( SIGHUP, SIG_IGN );
		sigquit = signal( SIGQUIT, SIG_IGN );
		sigterm = signal( SIGTERM, SIG_IGN );

		/*
		 * We don't try to create the relation directly
		 * as this could clobber some else's table if
		 * they're in the link/unlink phase of a commit.
		 * Instead, we create a temporary file and try
		 * to link it to the relation name.  This is an
		 * atomic operation and will not destroy the relation
		 * if it does exist when the creat() is called.
		 */
		str = cpdirname( tempfile, relptr->path );
		if ( str != tempfile )
			*str++ = '/';
		strcpy( str, "newXXXXXX" );	/* get temp file name */
		(void)mktemp( tempfile );

		rc = TRUE;

		if ( (fd = creat( tempfile, 0664 )) < 0 )
		{
			set_uerror( UE_WOPEN );
			rc = FALSE;
		}
		else
		{
			(void)close( fd );

#if 0
			/*
			 * Before we actually attempt to create the table,
			 * we pause to allow anyone else who's in the
			 * link/unlink stage of a commit to finish.
			 * This is just a kludge to help narrow the window.
			 */
			sleep( 3 );
#endif

			errno = 0;
			if ( link( tempfile, relptr->path ) != 0 &&
				errno != EEXIST )
			{
				/*
				 * There was an error in linking the file
				 * and it was not because the file exists
				 * already.
				 */
				set_uerror( UE_LINK );
				rc = FALSE;
			}
		}

		(void)unlink( tempfile );

		(void)signal( SIGINT, sigint );	/* reset signals */
		(void)signal( SIGHUP, sighup );
		(void)signal( SIGQUIT, sigquit );
		(void)signal( SIGTERM, sigterm );

		if ( ! rc )
			return( FALSE );
	}

#ifdef	TMPLOCK
	return( mklock( relptr->path, lockfile ) );
#else
	return( TRUE );
#endif
}

FILE *
init_modify( relptr, tempfile, lockfile )
struct urelation *relptr;
char *tempfile;
char *lockfile;
{
	char *str;
	FILE *tmpfp;

	/*
	 * Check the permissions on directory of table.  This is done by
	 * trying to create the temporary copy of the database.
	 */
	while( 1 )
	{
#ifndef NOHOLDLOCK
		char oldtemp[ MAXPATH+4 ];	/* allow for "/./" or "././" prefix */
#endif
		char tmplock[ MAXPATH+4 ];	/* allow for "/./" or "././" prefix */
		struct stat buf;

		str = cpdirname( tempfile, relptr->path );
		if ( str != tempfile )
			*str++ = '/';
		strcpy( str, "itmpXXXXXX" );	/* get temp file name */
		(void)mktemp( tempfile );

		errno = 0;	/* Need to explicitly clear from any earlier problem */

		if ( (tmpfp = fopen( tempfile, "w" )) == NULL )
		{
			tempfile[0] = '\0';
			set_uerror( UE_TMPOPEN );
			return( NULL );
		}

#ifndef TMPLOCK
		/*
		 * If we're not using inode-based locks in /tmp, then
		 * the itmp* file can't be created already locked, so
		 * just pop out of the loop.
		 */
		break;
#endif
		/*
		 * See if the lockfile already exists for the
		 * temporary file.  If it does, but we can unlink
		 * the lockfile, don't worry about it.
		 */
		if ( lockname( tempfile, tmplock, &buf ) &&
			( stat( tmplock, &buf ) < 0 ||
			unlink( tmplock ) == 0 ) )
		{
			break;
		}

		/*
		 * The lock file already exists for the
		 * temporary table.  If we use this file,
		 * the table will immediately be locked upon
		 * commit.  So we can't use this temp file.
		 *
		 * The #ifdefs here are for 2 alternatives to
		 * handle the situation.  One is to just return
		 * an error and let the user clean it up.  The
		 * other is to create an empty file to occupy
		 * the inode and then go get a new temp file name.
		 */
		(void)fclose( tmpfp );

#ifdef NOHOLDLOCK
		set_uerror( UE_TMPLOCK );
		(void)unlink( tempfile );
		tempfile[0] = '\0';

		return( NULL );
#else
		str = cpdirname( oldtemp, tempfile );
		if ( str != tempfile )
			*str++ = '/';
		strcpy( str, "LOCKXXXXXX" );
		(void)mktemp( oldtemp );
		if ( link( tempfile, oldtemp ) < 0 || unlink( tempfile ) < 0 )
		{
			set_uerror( UE_TMPLOCK );

			(void)unlink( tempfile );
			tempfile[0] = '\0';

			return( NULL );
		}
#endif
	}

	if ( lockfile[0] == '\0' && ! start_transact( relptr, lockfile ) )
	{
		(void)fclose( tmpfp );
		(void)unlink( tempfile );
		tempfile[0] = '\0';

		return( NULL );
	}

	return( tmpfp );
}

end_modify( relptr, keep, tmpfp, tempfile, lockfile )
struct urelation *relptr;
int keep;
FILE *tmpfp;
char *tempfile;
char *lockfile;
{
	char resetmode, rc;
	struct stat statbuf;
	RETSIGTYPE (*sigint)(), (*sigquit)(), (*sighup)(), (*sigterm)();

	/*
	 * Got some touchy code coming up, ignore all the signals.
	 * No interruptions allowed, while we're linking and unlinking
	 * the old and new tables.
	 */
	sigint = signal( SIGINT, SIG_IGN );
	sighup = signal( SIGHUP, SIG_IGN );
	sigquit = signal( SIGQUIT, SIG_IGN );
	sigterm = signal( SIGTERM, SIG_IGN );

	if ( tmpfp && fclose( tmpfp ) == EOF && keep ) {
		set_uerror( UE_WRITE );
		keep = FALSE;
		rc = FALSE;
	}
	else
		rc = TRUE;

	if ( ! keep )
	{
		/*
		 * Remove the updated database.
		 */
		if ( tempfile && *tempfile )
			(void)unlink( tempfile );
	}
	else if ( tempfile && *tempfile )
	{
		char origfile[ MAXPATH+4 ];	/* allow for "/./" or "././" prefix */

		/*
		 * Get the stats on the original table.
		 */
		if ( stat( relptr->path, &statbuf ) < 0 )
		{
			/*
			 * Original table not there; don't
			 * change ownership of file.
			 */
			resetmode = FALSE;
		}
		else
			resetmode = TRUE;

#ifdef	ADVLOCK
		/*
		 * If table is susposed to be locked then
		 * check that the lock file still exists
		 * and that it has not been tampered with.
		 */
		if ( lockfile ) {
			if ( ! cklock( lockfile ) )
			{
				extern void set_ue_lockstmp( );

				/* remove lockfile */
				(void) rmlock( lockfile , TRUE );

				/* reset signals */
				(void)signal( SIGINT, sigint );
				(void)signal( SIGHUP, sighup );
				(void)signal( SIGQUIT, sigquit );
				(void)signal( SIGTERM, sigterm );

				/*
				 * set UE_LOCKSTMP error and
				 * save name of the tempfile
				 */
				set_ue_lockstmp( tempfile );

				return( FALSE );
			}
		}
#endif

		/*
		 * Remove the old table and link the new table to the
		 * old table's name:
		 *
		 *  1. Link the original table to a temporary table
		 *     (origfile).
		 *  2. Unlink the original table.
		 *  3. Link the new temporary table (tmptable) to the
		 *     original table path name.
		 *  4. Remove the new temporary table (tmptable) and
		 *     the old temporary table (origfile).
		 *
		 * This procedure avoids the (unlikely) case where the
		 * new table has been deleted out from under us by some
		 * one else.
		 */
#ifdef	__STDC__
		if ( resetmode )
		{
			/*
			 * Reset all the pertinent stats on the new
			 * table to that of the old table.
			 */
			chmod( tempfile, (int)statbuf.st_mode );
			chown( tempfile, (int)statbuf.st_uid, (int)statbuf.st_gid );
		}
		if ( rename( tempfile, relptr->path ) != 0 ) {
			set_uerror( UE_LINK );
			rc = FALSE;
		}
#else
		cpdirname( origfile, relptr->path );
		strcat( origfile, "/origXXXXXX" );
		mktemp( origfile );
		if ( link( relptr->path, origfile ) != 0 )
		{
			set_uerror( UE_OLDLINK );
			rc = FALSE;
		}
		else if ( unlink( relptr->path ) != 0 )
		{
			set_uerror( UE_UNLINK );
			rc = FALSE;
		}
		else if ( link( tempfile, relptr->path ) != 0 )
		{
			set_uerror( UE_LINK );
			rc = FALSE;
		}
		else
		{
			/*
			 * Everything linked in okay.
			 */
			(void)unlink( tempfile );
			(void)unlink( origfile );

			if ( resetmode )
			{
				/*
				 * Reset all the pertinent stats on the new
				 * table to that of the old table.
				 */
				chmod( relptr->path, (int)statbuf.st_mode );
				chown( relptr->path, (int)statbuf.st_uid,
					(int)statbuf.st_gid );
			}
		}
#endif
	}

	if ( lockfile ) {
		/* should not fail unless internal error */
		if ( ! rmlock( lockfile , FALSE ) )
			rc = FALSE;
	}

	(void)signal( SIGINT, sigint );	/* reset signals */
	(void)signal( SIGHUP, sighup );
	(void)signal( SIGQUIT, sigquit );
	(void)signal( SIGTERM, sigterm );

	return( rc );
}
