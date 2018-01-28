/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <signal.h>
#include "message.h"

/*
 * Definition of what a signal function returns.  This definition
 * helps make the code cleaner portability-wise.  The most common
 * definition is that signal functions return an int.  Some
 * implementations say they are void, however.  This definition
 * can be changed depending on the circumstances.
 *
 */
#ifndef RETSIGTYPE
#define RETSIGTYPE	int
#endif


shellcmd( path, cmd )
char *path;
char *cmd;
{
	int status, pid, w;
	RETSIGTYPE (*istat)(), (*qstat)();

	/*
	 * This is a stolen version of the system() function.
	 * The only difference is that this command sets the effective
	 * user and group id back to the real user and group id
	 * before calling the command.  This has to be done because
	 * of UNIX(SV)'s way of dealing with the id's.  (You can't
	 * restore the effective uid/gid, once you set them to the
	 * real uid/gid.)
	 *
	 * As a convenience, this command also sets the path (or any
	 * other environment variable) in the environment to what is
	 * passed in.
	 */

	if ( (pid = fork()) == 0 ) {	/* child process */
		if ( setuid( getuid( ) ) < 0 || setgid( getgid( ) ) < 0 ) {
			prmsg( MSG_INTERNAL, "cannot reset user and group id's for user command" );
			_exit( 127 );	/* cannot set uid and gid */
		}

		if ( path && *path && putenv( path ) ) {
			prmsg( MSG_INTERNAL, "cannot restore environment for user command" );
			_exit( 127 );	/* cannot set path */
		}

		(void) execl( "/bin/sh", "sh", "-c", cmd, 0 );

		prmsg( MSG_INTERNAL, "exec failed for user command" );
		_exit( 127 );
	}

	istat = signal( SIGINT, SIG_IGN );
	qstat = signal( SIGQUIT, SIG_IGN );
	while( (w = wait(&status)) != pid && w != -1 )
		;
	(void) signal( SIGINT, istat );
	(void) signal( SIGQUIT, qstat );

	return( (w == -1) ? -1 : status );
}
