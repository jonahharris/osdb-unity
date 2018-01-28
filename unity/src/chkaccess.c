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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef	__STDC__
#include <unistd.h>		/* getgroups(2) */
#include <limits.h>		/* NGROUPS_MAX  */
#include <sys/wait.h>
#else

#define WIFEXITED(stat)		((int)((stat)&0xFF) == 0)
#define WIFSIGNALED(stat)	((int)((stat)&0xFF) > 0 && \
				    (int)((stat)&0xFF00) == 0)
#define WIFSTOPPED(stat)        ((int)((stat)&0xFF) == 0177 && \
				    (int)((stat)&0xFF00) != 0)
#define WEXITSTATUS(stat)	((int)(((stat)>>8)&0xFF))
#define WTERMSIG(stat)		((int)((stat)&0x7F))
#define WSTOPSIG(stat)		((int)(((stat)>>8)&0xFF))

#endif

/* This is defined differently on mainframes */
#ifndef NGROUPS_MAX
#include <sys/param.h>
#define NGROUPS_MAX NGROUPS
#endif
			
#include "db.h"

/*
 * Permission definitions for checking permissions on files.
 */
#define P_READ	04
#define P_WRITE	02
#define P_EXEC	01
#define P_EXIST	00

#define OWNER		6	/* bits to shift for owner permissions */
#define GROUP		3	/* bits to shift for group permissions */

#define MODEMASK	(P_READ|P_WRITE|P_EXEC)	/* mask for only perm's in mode */

#ifndef NULL
#define NULL	0
#endif

#ifndef	__STDC__
extern short geteuid(), getegid();
#endif

extern char *getenv(), *strchr();

/*
 * This function checks whether a given file can be opened for the given
 * mode by a user having the given uid and gid.  The routine emulates the
 * permission checking done by UNIX, if the an open were attempted on the
 * given file for a user having the given **effective** uid and gid.
 * The function returns 0 if the operation is allowed, -1 on errors or
 * if the operation is not allowed.  The user must be able to stat(2) the
 * given file or -1 will always be returned.
 *
 * This routine is essentially an enhanced version of access(2).
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
 *	4.  If "other" permissions fail (do not match given mode)
 *	    and __STDC__ or SUPGRP
 *	    is defined then getgroups(2) is called get a list of group IDs
 *	    that the user has supplementary group access to.  If the group ID
 *	    of the given directory or file is found in the list of group IDs
 *	    the operation is evaluated based on the group permissions.  Fail
 *	    is returned if the group ID is not found in the list of group IDs.
 */
chkaccess( file, mode )
char *file;
register ushort mode;
{
	struct stat buf;

	if ( file == (char *)NULL || *file == '\0' )
		file = ".";

	if ( stat( file, &buf ) < 0 )
		return( -1 );

	if ( mode == P_EXIST )
		return( 0 );

	mode &= MODEMASK;
	if ( geteuid() == buf.st_uid ) {
		mode <<= OWNER;
		return( (buf.st_mode & mode) == mode ? 0 : -1 );
	}
	else if ( getegid() == buf.st_gid ) {
		mode <<= GROUP;
		return( (buf.st_mode & mode) == mode ? 0 : -1 );
	}
	if ( (buf.st_mode & mode) == mode )
		return( 0 );
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
				return( (buf.st_mode & mode) == mode ? 0 : -1 );
		}
	}
#endif
#endif
	return( -1 );
}

char *packedcatcmd;
char *packedsuffix;

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

FILE *
packedopen( table )
char *table;
{
	char	cmdbuf[MAXPATH+MAXPATH+8];

	if ( statpacked( table, NULL ) != 0 ) {
		return( (FILE *)NULL );
	}
	sprintf(cmdbuf, "%s %s", packedcatcmd, table);
	return( popen( cmdbuf, "r" ) );
}

int
packedclose( fp, eof )
FILE *fp;
int  eof;
{
	int	status;

	if ( ! (eof) ) {	/* read till EOF to avoid broken pipe error */
		while ( getc(fp) != EOF )
			;
	}

	status = pclose(fp);
	if ( ( WIFEXITED( status ) ) &&
	     ( WEXITSTATUS( status ) == 0 ) )
		return( 0 );	/* success */
	else
		return( 1 );	/* fail    */
}
