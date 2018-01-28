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
#include <pwd.h>
#include <errno.h>
#include "message.h"

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

extern char *basename();
extern struct passwd *getpwuid();

extern int errno;

char *prog;

main( argc, argv )
int argc;
char *argv[];
{
	char force;
	char status;
	char seen_file;
	int i;

	prog = basename( *argv );

	if ( argc < 2 )
	{
		usage();
	}

	force = FALSE;	/* normally do all the checks */
	status = 0;
	seen_file = FALSE;

	for( i = 1; i < argc; i++ )
	{
		if ( argv[i][0] == '-' )
		{
			char *option;

			for( option = &argv[i][1];
				option != NULL && *option != '\0';
				option++ )
			{
				switch( *option ) {
				case 'f':
					force = TRUE;
					break;
				default:
					prmsg( MSG_ERROR, "unrecognized option '%c'",
						*option );
					usage();
					break;
				}
			}
		}
		else
		{
			seen_file = TRUE;

			if ( ! rmulock( argv[i], force ) )
				status = 2;
		}
	}

	if ( ! seen_file )
	{
		prmsg( MSG_ERROR, "no file names given on command line" );
		usage( );
	}

	exit( status );
}

rmulock( filename, force )
char *filename;
char force;
{
	char lockfile[ 40 ];
	struct stat statbuf;
	int uid;
	char seen_error;

	if ( ! lockname( filename, lockfile, &statbuf ) )
	{
		perror( prog );
		prmsg( MSG_ERROR, "cannot stat table '%s'", filename );
		usage( );
	}

	if ( stat( lockfile, &statbuf ) < 0 )
	{
		if ( errno == ENOENT )
		{
			prmsg( MSG_ERROR, "lockfile %s for table %s does not exist",
				lockfile, filename );
		}
		else
		{
			perror( prog );
			prmsg( MSG_ERROR, "cannot stat lockfile %s for table %s",
				lockfile, filename );
		}
		return( FALSE );
	}

	seen_error = FALSE;		/* no errors yet */

	uid = geteuid( );
	if ( ! force && statbuf.st_uid != uid )
	{
		struct passwd *pwdptr;
		char uidbuf[20];
		char *owner;

		pwdptr = getpwuid( statbuf.st_uid );
		if ( pwdptr == NULL )
		{
			sprintf( uidbuf, "uid %d", statbuf.st_uid );
			owner = uidbuf;
		}
		else
			owner = pwdptr->pw_name;

		prmsg( MSG_ERROR, "lockfile %s is owned by login '%s' -- only that login can remove it",
			lockfile, owner );
		seen_error = TRUE;
	}

	if ( ! force && (statbuf.st_mode & 0777) != 0 )
	{
		prmsg( MSG_ERROR, "lockfile %s is not mode 0 (has mode %04o)",
			lockfile, statbuf.st_mode & 0777 );
		seen_error = TRUE;
	}

	if ( ! force && statbuf.st_size != 0 )
	{
		prmsg( MSG_ERROR, "lockfile %s is not size 0 (has size %ld)",
			lockfile, statbuf.st_size );
		seen_error = TRUE;
	}

	if ( seen_error )
		return( FALSE );

	/* now unlink the file */
	if ( unlink( lockfile ) < 0 )
	{
		perror( prog );
		prmsg( MSG_ERROR, "cannot unlink lockfile %s for table %s",
			lockfile, filename );
		return( FALSE );
	}
	else
	{
		prmsg( MSG_NOTE, "lockfile %s for table %s successfully removed",
			lockfile, filename );
		return( TRUE );
	}
}

usage( )
{
	prmsg( MSG_USAGE, "[-f] <unity_table>..." );
	exit( 1 );
}
