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
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#ifdef __STDC__
#include <sys/wait.h>
#else

#define WIFEXITED(stat)         ((int)((stat)&0xFF) == 0)
#define WIFSIGNALED(stat)       ((int)((stat)&0xFF) > 0 && \
                                    (int)((stat)&0xFF00) == 0)
#define WIFSTOPPED(stat)        ((int)((stat)&0xFF) == 0177 && \
                                    (int)((stat)&0xFF00) != 0)
#define WEXITSTATUS(stat)       ((int)(((stat)>>8)&0xFF))
#define WTERMSIG(stat)          ((int)((stat)&0x7F))
#define WSTOPSIG(stat)          ((int)(((stat)>>8)&0xFF))

#endif

extern FILE *fdopen();
extern char *getenv(), *basename();


/*
 * This is a backward compatible extension of the standard popen().
 * We allow file descriptor numbers to be encoded in the mode
 * string which, in read mode, will be grouped together and
 * attached to the pipe such that the parent process can catch
 * output from multiple file descriptors, e.g., from stdout and
 * stderr, or alternate file descriptors, e.g., stderr instead of
 * stdout.  The child process can also read from other popen()
 * pipes previously set up in the parent.  The special file descriptors
 * are appended to the mode string and separated by non-digits.
 * For example to read from both stdout and stderr, the mode string
 * could be "r1 2".  If the special file descriptors are omitted,
 * then only stdout is connected.
 *
 * In write mode, there is no difference between this and the standard
 * popen().  However, the code is more robust than the standard popen().
 *
 * There is also a popenv() routine provided which allows a command
 * to be executed directly, without going through the shell.  The
 * arguments are passed in an argv array, similar to execv.  This
 * routine is useful whenever shell processing is not needed, quoting
 * is a problem, or it is more convenient to pass args in an array,
 * rather than in a single string.  Other than the argument passing
 * method and no shell interpretation, popenv() supports the same
 * functionality as the popen() in this file.
 */

#define MAXFILES	20	/* maximum concurrent open files allowed */

/*
 * For each file descriptor, what's the pid associated with it?
 * This is used for knowing what pid to look for when a pipe is
 * closed, especially when multiple pipes are opened.
 */
static struct p_state {		/* state information on processes per fd */
	short state;		/* process state, see #defines below */
	short status;		/* exit status returned by wait */
	long pid;		/* pid associated with descriptor */
} p_state[ MAXFILES ] = { 0 };	/* set to all zeros */

/* constants for p_state.state */
#define NOPROCESS 0		/* no process associated with descriptor */
#define RUNNING   1		/* process running */
#define DONE	  2		/* process exited, status in p_state.status */

/*
 * Definition of what a signal function returns.  This definition
 * helps make the code cleaner for porting.  The most common
 * definition is that signal functions return an int.  The newer
 * implementations say they are void, however.  This definition
 * can be changed depending on the circumstances.
 */
#ifndef RETSIGTYPE
#define RETSIGTYPE	int
#endif


static FILE *
popenv( cmdname, argv, mode )
char *cmdname;
char **argv;
char *mode;
{
	int p[2];
	register short parentside, childside, special_fds;
	register long pid;

	if ( *mode != 'r' && *mode != 'w' )
		return( NULL );

	if( pipe( p ) < 0 )
		return( NULL );
	if ( *mode == 'r' )
	{
		parentside = p[0];
		childside = p[1];
	}
	else
	{
		parentside = p[1];
		childside = p[0];
	}

	if ( (pid = fork()) == 0 )	/* child side */
	{
		register short i;

		(void)close( parentside );

		special_fds = 0;	/* no special FDs seen, yet */
		if ( *mode == 'r' )
		{
			register char *ptr;
			register short fd, flags;

			/*
			 * Look for any other file descriptors the
			 * user wants to collectively read from.
			 */
			ptr = &mode[1];
			while( *ptr )
			{
				if ( *ptr < '0' || *ptr > '9' )
				{
					ptr++;
					continue;
				}
				/*
				 * We have a number to duplicate.  The
				 * fd must be open for writing and not
				 * be one of the pipe's fds.  We also
				 * null out the entry in p_state so
				 * the fd is not closed below.
				 */
				fd = 0;
				while ( *ptr >= '0' && *ptr <= '9' )
					fd = fd * 10 + *ptr++ - '0';
				if ( fd >= MAXFILES || fd == childside )
					continue;
				/*
				 * If the file isn't opened for writing,
				 * then ignore this descriptor.
				 */
				flags = fcntl( fd, F_GETFL, 0 );
				if ( flags == -1 || (flags & (O_WRONLY|O_RDWR)) == 0 )
					continue;

				/* duplicate the file */
				(void)close( fd );
				(void)fcntl( childside, F_DUPFD, fd );

				/*
				 * Set the pid to zero so the file won't be
				 * closed below.
				 */
				p_state[fd].pid = 0;
				special_fds = 1;
			}
		}

		if ( special_fds == 0 )
		{
			short stdio;

			/* make stdio point to pipe */
			stdio = ( *mode == 'r' ? 1 : 0 );
			if ( stdio != childside )
			{
				(void)close( stdio );
				(void)fcntl( childside, F_DUPFD, stdio );
			}
		}
		(void)close( childside );

		/*
		 * Close all pipes from previous popen's which haven't
		 * been closed yet, or requested as special FDs above.
		 */
		for( i = 0; i < MAXFILES; i++ )
		{
			if ( p_state[ i ].pid != 0  )
				(void)close( i );
		}

		(void)execvp( cmdname, argv );

		_exit( 1 );	/* exec failed */
	}

	/*
	 * This is parent side of the fork().
	 */
	if( pid == -1 )
		return( NULL );

	p_state[ parentside ].pid = pid;
	p_state[ parentside ].state = RUNNING;

	(void)close( childside );
	return( fdopen( parentside, mode ) );
}

FILE *
popen( cmd, mode )
#ifdef	__STDC__
const char *cmd, *mode;
#else
char *cmd, *mode;
#endif
{
#ifdef	__STDC__
	const char *argv[4];
#else
	char *argv[4];
#endif
	char *shell;

#if 0
/*
 * We used to be nice and use the shell of the user's choice (see
 * below), but it turned out that the 86 version of /usr/lbin/ksh
 * has a security "feature" where it resets the path if called from
 * a process where effective and real user/group ids don't match
 * (like a setuid program).  Since many elmr programs run that way,
 * this caused much customer annoyance.  Hence, we're back to just
 * /bin/sh.
 */
	shell = getenv( "SHELL" );
	if ( shell == NULL || *shell == '\0' )
#endif
		shell = "/bin/sh";

	argv[0] = basename( shell );
	argv[1] = "-c";
	argv[2] = cmd;
	argv[3] = NULL;

	return( popenv( shell, argv, mode ) );
}

int
pclose( fp )
FILE *fp;
{
	register short fd;
	register long rc;
	int status;
	RETSIGTYPE (*hstat)(), (*istat)(), (*qstat)();

	fd = fileno( fp );
	(void)fclose( fp );

	/*
	 * If this pipe was previously closed unexpectedly,
	 * just return the saved return status.
	 */
	if ( p_state[fd].state == DONE )
	{
		p_state[fd].pid = 0;
		p_state[fd].state = NOPROCESS;

		return( p_state[fd].status );
	}

	istat = signal( SIGINT, SIG_IGN );
	qstat = signal( SIGQUIT, SIG_IGN );
	hstat = signal( SIGHUP, SIG_IGN );

	/*
	 * Wait for the expected child to die.  If another child
	 * dies, we mark it as closed and continue waiting.
	 */
	while( (rc = wait( (int *) &status )) != p_state[fd].pid &&
		(rc != -1 || errno == EINTR) )
	{
		if ( rc != -1 )
		{
			register short i;

			for( i = 0; i < MAXFILES; i++ )
			{
				if ( p_state[i].pid == rc &&
					p_state[i].state == RUNNING )
				{
					p_state[i].state = DONE;
					p_state[i].status = (int) status;
					break;
				}
			}
		}
	}
	if ( rc == -1 )
		status = -1;

	(void)signal( SIGINT, istat );
	(void)signal( SIGQUIT, qstat );
	(void)signal( SIGHUP, hstat );

	p_state[fd].pid = 0;	/* mark this pipe closed */
	p_state[fd].state = NOPROCESS;

	return( (int) status );
}

#ifndef	STDIN_FILENO
#define	STDIN_FILENO	0
#endif

#ifndef	STDOUT_FILENO
#define	STDOUT_FILENO	1
#endif

int
packedopen( packedcatcmd, table )
char *packedcatcmd;
char *table;
{
	char *argv[3];
	int p[2];
	register short parentside, childside;
	register long pid;

	argv[0] = basename( packedcatcmd );
	argv[1] = table;
	argv[2] = NULL;

	if( pipe( p ) < 0 )
		return( -1 );

	/* read from child */
	parentside = p[0];
	childside = p[1];

	if ( (pid = fork()) == 0 )	/* child side */
	{
		register int fd;

		(void)close( parentside );

		/* make stdio point to pipe */
		if ( childside != STDOUT_FILENO )
		{
			(void)close( STDOUT_FILENO );
			(void)fcntl( childside, F_DUPFD, STDOUT_FILENO );
			(void)close( childside );
		}

		/* make stdin come from /dev/null */
		fd = open( "/dev/null", O_RDONLY );
		if ( fd != STDIN_FILENO )
		{
			(void)close( STDIN_FILENO );
			(void)fcntl( fd, F_DUPFD, STDIN_FILENO );
			(void)close( fd );
		}

		/*
		 * Close all pipes from previous popen's which haven't
		 * been closed after making sure we do not clobber
		 * STDIN or STDOUT.
		 */
		p_state[ STDIN_FILENO ].pid = 0;
		p_state[ STDOUT_FILENO ].pid = 0;
		for( fd = 0; fd < MAXFILES; fd++ )
		{
			if ( p_state[ fd ].pid != 0  )
				(void)close( fd );
		}

		(void)execvp( packedcatcmd, argv );

		_exit( 1 );	/* exec failed */
	}

	/*
	 * This is parent side of the fork().
	 */
	if ( pid == -1 )
		return( -1 );

	p_state[ parentside ].pid = pid;
	p_state[ parentside ].state = RUNNING;

	(void)close( childside );

	return( parentside );
}

int
packedclose( fd )
register int fd;
{
	register long rc;
	int status;
	RETSIGTYPE (*hstat)(), (*istat)(), (*qstat)();

	(void)close( fd );

	if ( fd < 0 || fd >= MAXFILES )
		return( -1 );		/* error */

	/*
	 * If this pipe was previously closed unexpectedly,
	 * just return the saved return status.
	 */
	if ( p_state[fd].state == DONE )
	{
		p_state[fd].pid = 0;
		p_state[fd].state = NOPROCESS;

		status = p_state[fd].status;

		if ( ( WIFEXITED( status ) ) &&
		     ( WEXITSTATUS( status ) == 0 ) )
			return( 0 );	/* success */
		else
			return( 1 );	/* fail    */
	}

	istat = signal( SIGINT, SIG_IGN );
	qstat = signal( SIGQUIT, SIG_IGN );
	hstat = signal( SIGHUP, SIG_IGN );

	/*
	 * Wait for the expected child to die.  If another child
	 * dies, we mark it as closed and continue waiting.
	 */
	while( (rc = wait( &status )) != p_state[fd].pid &&
		(rc != -1 || errno == EINTR) )
	{
		if ( rc != -1 )
		{
			register short i;

			for( i = 0; i < MAXFILES; i++ )
			{
				if ( p_state[i].pid == rc &&
					p_state[i].state == RUNNING )
				{
					p_state[i].state = DONE;
					p_state[i].status = status;
					break;
				}
			}
		}
	}
	if ( rc == -1 )
		status = -1;

	(void)signal( SIGINT, istat );
	(void)signal( SIGQUIT, qstat );
	(void)signal( SIGHUP, hstat );

	p_state[fd].pid = 0;	/* mark this pipe closed */
	p_state[fd].state = NOPROCESS;


	if ( ( WIFEXITED( status ) ) &&
	     ( WEXITSTATUS( status ) == 0 ) )
		return( 0 );	/* success */
	else
		return( 1 );	/* fail    */
}

FILE *
packfpopen( packedcatcmd, table )
char *packedcatcmd;
char *table;
{
	int	fd;
	FILE	*fp;

	fd = packedopen( packedcatcmd, table );
	if ( fd < 0 ) {
		return( (FILE *) NULL );
	}

	fp = fdopen( fd, "r" );
	if ( fp == (FILE *) NULL ) {
		if ( fd < MAXFILES ) {
			(void) kill( p_state[ fd ].pid, SIGKILL );
			p_state[fd].pid = 0;
			p_state[fd].status = 0;
			p_state[fd].state = NOPROCESS;
		}
		(void)close( fd );
		return( (FILE *) NULL );
	}

	return( fp );
}

int
pkclose( fp )
FILE *fp;
{
	register short fd;

	fd = fileno( fp );

	if ( ( fd >= 0 ) && ( fd < MAXFILES ) && ( p_state[ fd ].pid != 0 ) ) {
		(void) kill( p_state[ fd ].pid, SIGKILL );
	}

	return( pclose( fp ) );
}

