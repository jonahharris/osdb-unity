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
#ifdef	ADVLOCK
#include <sys/utsname.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#include <signal.h>
#ifndef __ppc__
#include <malloc.h>
#endif 
#include "message.h"
#include "urelation.h"

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

RETSIGTYPE catch_sig();

extern char *basename();
extern char *strchr();

#ifndef __STDC__
extern struct passwd *getpwuid();
#endif

extern int errno;

int lockcount;		/* number of files in locklist */
char **locklist;	/* list of lockfiles */

char *prog;

main( argc, argv )
int argc;
char *argv[];
{
	int i, Lcount, Ncount, Ucount;
	char action;
	char force;
	char outdesc;
	char quiet;
	char status;
	char logname[20];
#ifdef	ADVLOCK
	char lockownbuf[SYS_NMLN+32];
	char lockinfo[SYS_NMLN+132];
#endif
	char lockfile[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char *lockowner;
	char *lockstatus;
	char *table;
	char *p;
	int fd;
	int mode;
	int uid;
#ifdef	ADVLOCK
	int len;
	struct utsname uts_name;
#endif
	struct passwd *pwdptr;
	struct stat statbuf;

	prog = basename( *argv );

	if ( argc < 2 )
	{
		usage( );
	}

	Lcount = 0;	/* number of locked tables */
	Ncount = 0;	/* number of tables ...... */
	Ucount = 0;	/* number of unlocked tables */
	action = 'S';	/* default action to lockfile status report */
	force = FALSE;	/* normally do all the checks */
	outdesc = FALSE;/* default to not including description with status report */
	quiet = FALSE;	/* default to print a message for each table we lock or unlock */
	status = 0;

	for ( ++argv, --argc; argc > 0 && argv[0][0] == '-'; argc--, argv++ )
	{
		register char *option;

		option = *argv;

		while ( *++option )
		{
			switch ( *option ) {
			case 'E':		/* check existence */
				action = 'E';
				break;
			case 'c':
				outdesc = TRUE;
				break;
			case 'f':
				force = TRUE;
				break;
			case 'l':		/* lock */
				action = 'L';
				if ( option[1] == 'o' )
					option = "l";
				break;
			case 'q':
				quiet = TRUE;
				break;
			case 'u':		/* unlock */
				action = 'U';
				if ( option[1] == 'n' )
					option = "u";
				break;
			default:
				prmsg( MSG_ERROR, "unrecognized option '%c'",
					*option );
				usage( );
				break;
			}
		}
	}

	if ( argc <= 0 )
	{
		prmsg( MSG_ERROR, "no table names given on command line" );
		usage( );
	}

#ifdef	ADVLOCK
	if ( uname( &uts_name ) ==  -1 ) {
		perror( prog );
		prmsg( MSG_ERROR, "cannot get system node name" );
		exit( 2 );
	}
	len = strlen( uts_name.nodename );
#endif

	if ( action == 'L' )
	{
		/*
		 * Allocate memory for a list of lockfiles to be removed
		 * in the event that we encounter an error after we have
		 * already locked some but not all of the requested tables.
		 */
		if ( ( p = malloc( sizeof(char *) * argc ) ) == NULL ) {
			prmsg( MSG_ERROR, "no memory available" );
			exit( 2 );
		} else {
			locklist = &p;
		}

		/*
		 * Catch all signals so we can remove the lock files
		 * if we fail to lock all of the requested tables.
		 */
		(void)set_sig( SIGHUP, catch_sig );
		(void)set_sig( SIGINT, catch_sig );
		(void)set_sig( SIGQUIT, catch_sig );
		(void)signal( SIGILL, catch_sig );
		(void)signal( SIGTRAP, catch_sig );
#ifdef SIGIOT
		(void)signal( SIGIOT, catch_sig );
#endif
#ifdef SIGEMT
		(void)signal( SIGEMT, catch_sig );
#endif
		(void)signal( SIGFPE, catch_sig );
		(void)signal( SIGBUS, catch_sig );
		(void)signal( SIGSEGV, catch_sig );
#ifdef SIGSYS
		(void)signal( SIGSYS, catch_sig );
#endif
		(void)signal( SIGPIPE, catch_sig );
		(void)set_sig( SIGUSR1, catch_sig );
		(void)set_sig( SIGUSR2, catch_sig );
		(void)signal( SIGALRM, SIG_IGN );
	}
	else if ( action == 'S' && outdesc == TRUE )
	{
		/*
		 * NOTE: Field widths are sized so that the user can
		 *	 pipe the output to uprint(UNITY) with a "-e"
		 *	 option for a total width of 79 characters so
		 *	 that all fields will appear in the output.
		 *	 In addition, the width of the lockfile attribute
		 *	 is "optimized" for use with /tmp based lockfiles
		 *	 since the name of this type of lockfile has no
		 *	 no direct correlation to the name of the table.
		 *	 This also allows for longer table names to fit on
		 *	 a single line without wrapping to the next line.
		 *	 For SVR4 (ADVLOCK), the owner field is 9 characters wider
		 *	 for owner to be of the form <login>@<nodename>.
		 */
		fprintf( stdout, "%%description\n%s\n%s\n%s\n%s\n%%enddescription\n",
#ifdef	ADVLOCK
			"table\tt \t28l\t", "lockfile\tt \t16l\t",
			"status\tt \t8c\t", "owner\tt\\n\t17c\t" );
#else
			"table\tt \t37l\t", "lockfile\tt \t16l\t",
			"status\tt \t8c\t", "owner\tt\\n\t8c\t" );
#endif
	}

	/* get current UID and login */
	uid = geteuid();
	if ( ( pwdptr = getpwuid( uid ) ) == NULL )
	{
		sprintf( logname, "%d", uid );
	}
	else
	{
		strcpy( logname, pwdptr->pw_name);
	}

	for ( i = 0; i < argc; i++ )
	{
		mode = 0;
		table = argv[i];
		lockowner = "-";
		lockstatus = "ERROR";

		if ( ! lockname( table, lockfile, &statbuf ) )
		{
			perror( prog );
			prmsg( MSG_ERROR, "cannot stat table '%s'", table );
			/*
			 * Since we cannot lock all of the requested tables
			 * remove any locks that we created up to this point.
			 */
			if ( action == 'L' )
				(void) rmlocks( );
			usage( );
		}

		if ( stat( lockfile, &statbuf ) < 0 )
		{
			statbuf.st_uid = 0;	/* so we can compare user with lock owner later */

			if ( errno != ENOENT )
			{
				perror( prog );
				prmsg( MSG_ERROR, "cannot stat lockfile %s for table %s",
					lockfile, table );
				if ( action == 'L' )
				{
					/*
					 * Since we cannot lock all of the requested tables
					 * remove any locks that we created up to this point.
					 */
					(void) rmlocks( );
					exit( 2 );
				}
				else if ( action == 'U' )
				{
					/*
					 * If we cannot stat(2) the file then
					 * we cannot expect to unlink(2) it.
					 */
					status = 2;
					continue;
				}
				status = 2;
			}
			else
			{
				lockstatus = "UNLOCKED";
			}
		}
		else
		{
			mode = statbuf.st_mode & 0777;
#ifdef	ADVLOCK
			lockinfo[0] = '\0';
#endif
			/*
			 * check if this is a valid lockfile
			 */
#ifdef	ADVLOCK
			if ( mode == 0 )
#else
			if ( ( mode == 0 ) && ( statbuf.st_size == 0 ) )
#endif
			{
				lockstatus = "LOCKED";
			}
#ifdef	ADVLOCK
			else if ( ( mode == 0400 ) || ( mode == 0440 ) || ( mode == 0444 ) )
			{
				lockstatus = "LOCKED";

				if ( ( fd = open( lockfile, O_RDONLY ) ) >= 0 )
				{
					unsigned st_size;
					struct flock flockbuf;

					if ( statbuf.st_size < sizeof(lockinfo) ) {
						st_size = statbuf.st_size;
					} else {
						st_size = sizeof(lockinfo) - 1;
					}

					/* read system node where lock file was created */
					if ( ( st_size >= 1 ) &&
					     ( read( fd, lockinfo, st_size ) == st_size ) )
					{
						lockinfo[st_size] = '\0';
						if ( ( ( p = strchr( lockinfo, '\n') ) != NULL ) &&
						     ( p - &lockinfo[0] <= SYS_NMLN ) ) {
							*p = '\0';
						} else {
							lockinfo[0] = '\0';
						}
					}

					flockbuf.l_type = F_RDLCK;
					flockbuf.l_whence = 0;
					flockbuf.l_start = 0;
					flockbuf.l_len = 0;
#ifndef NOL_SYSID
					flockbuf.l_sysid = 0;
#endif
					flockbuf.l_pid = 0;

					if ( fcntl( fd, F_GETLK, &flockbuf ) == 0 ) {
						if ( flockbuf.l_type == F_UNLCK ) {
#ifndef	TRUST_LOCKS
							if ( ( lockinfo[0] != '\0' ) &&
							     ( strcmp( lockinfo, uts_name.nodename ) == 0 ) )
#endif
								lockstatus = "UNLOCKED";
						} else {
							lockstatus = "PLOCKED";
						}
					}

					(void) close( fd );
				}
			}
#endif
			else if ( action == 'L' )
			{
#ifdef	ADVLOCK
				prmsg( MSG_ERROR, "lockfile %s is not mode 0000, 0400, 0440, or 0444 (has mode %04o size %ld)",
					lockfile, SYS_NMLN, mode, statbuf.st_size );
#else
				prmsg( MSG_ERROR, "lockfile %s is not mode 0 and size 0 (has mode %04o size %ld)",
					lockfile, mode, statbuf.st_size );
#endif
				/*
				 * Since we cannot lock all of the requested tables
				 * remove any locks that we created up to this point.
				 */
				(void) rmlocks( );
				exit(2);
			}
			else if ( action == 'U' && force == FALSE )
			{
#ifdef	ADVLOCK
				prmsg( MSG_ERROR, "lockfile %s is not mode 0000, 0400, 0440, or 0444 (has mode %04o size %ld)",
					lockfile, SYS_NMLN, mode, statbuf.st_size );
#else
				prmsg( MSG_ERROR, "lockfile %s is not mode 0 and size 0 (has mode %04o size %ld)",
					lockfile, mode, statbuf.st_size );
#endif
				status = 2;
			}
			/*
			 * get owner of lock file
			 */
			if ( statbuf.st_uid == uid )
			{
				lockowner = logname;
#ifdef	ADVLOCK
				if ( lockinfo[0] != '\0' ) {
					sprintf( lockownbuf, "%s@%s", lockowner, lockinfo );
					lockowner = lockownbuf;
					if ( ( action == 'U' && force == FALSE ) &&
					     ( strcmp( lockinfo, uts_name.nodename ) ) ) {
						prmsg( MSG_ERROR, "lockfile %s is owned by login '%s'",
							lockfile, lockowner );
						prmsg( MSG_CONTINUE, "only that login can remove it" );
						status = 2;
					}
				}
#endif
			}
			else
			{
				if ( ( pwdptr = getpwuid( statbuf.st_uid ) ) == NULL )
				{
					static char uidbuf[20];
					sprintf( uidbuf, "%d", statbuf.st_uid );
					lockowner = uidbuf;
				}
				else
				{
					lockowner = pwdptr->pw_name;
				}
#ifdef	ADVLOCK
				if ( lockinfo[0] != '\0' ) {
					sprintf( lockownbuf, "%s@%s", lockowner, lockinfo );
					lockowner = lockownbuf;
				}
#endif
				if ( action == 'U' && force == FALSE )
				{
					prmsg( MSG_ERROR, "lockfile %s is owned by login '%s'",
						lockfile, lockowner );
						prmsg( MSG_CONTINUE, "only that login can remove it" );
					status = 2;
				}
			}
		}

		switch ( action ) {
		case 'S':
			fprintf( stdout, "%s %s %s %s\n",
				table, lockfile, lockstatus, lockowner );
			break;
		case 'E':
			if ( *lockstatus == 'L' || *lockstatus == 'P' ) {
				++Lcount;
			} else if ( *lockstatus == 'U' ) {
				++Ucount;
			}
			break;
		case 'L':
			if ( ( *lockstatus != 'L' && *lockstatus != 'P' ) ||
			     ( force == FALSE ) ||
			     ( statbuf.st_uid != uid ) )
			{
				/* now create the lock */
#ifdef	ADVLOCK
				/*
				 * lockfile mode = 0
				 */
				if ( ( fd = lockit( lockfile, 0, &statbuf ) ) < 0 )
#else
				if ( ! mklock( table, lockfile ))
#endif
				{
					pruerror( );
					prmsg( MSG_ERROR, "cannot create lockfile %s for table %s",
						lockfile, table );
					/*
					 * Since we cannot lock all of the requested tables
					 * remove any locks that we created up to this point.
					 */
					(void) rmlocks( );
					exit( 2 );
				}
				else
				{
#ifdef	ADVLOCK
					/*
					 * UNIX advisory lock not needed when mode = 0
					 * so close the FD and remove the UNIX file lock
					 */
					(void) close( fd );
#endif
					/*
					 * Add lockfile to list of lockfiles
					 * to be removed in the event that
					 * we cannot lock all of the tables.
					 */
					if ( ( p = malloc(strlen(lockfile)+1)) == NULL )
					{
						prmsg( MSG_ERROR, "no memory available" );
						/*
						 * Since we cannot lock all of the requested tables
						 * remove any locks that we created up to this point.
						 */
						(void) rmlocks( );
						exit( 2 );
					}
					/*
					 * Add lockfile to list to be removed
					 * in the event that we are unable to
					 * to lock all of the requested tables.
					 */
					strcpy( p, lockfile );
					locklist[lockcount] = p;
					++lockcount;
				}
			}
			break;
		case 'U':
			if ( (( force == TRUE ) && ( *lockstatus != 'U' )) ||
			     (( *lockstatus == 'L' ) && ( mode == 0 ) &&
			      ( statbuf.st_uid == uid ) ) )
			{
				if ( unlink( lockfile ) < 0 )
				{
					perror( prog );
					prmsg( MSG_ERROR, "cannot unlink lockfile %s for table %s",
						lockfile, table );
					status = 2;
				}
				else
				{
					++Ucount;
				}
			}
			else if ( *lockstatus == 'U' )
			{
				++Ncount;
				if ( ( force == FALSE ) && ( *lockowner == '-' ) )
				{
					prmsg( MSG_ERROR, "lockfile %s for table %s does not exist",
						lockfile, table );
					status = 2;
				}
			}
			break;
		}
	}

	switch ( action ) {
	case 'E':
		if ( Lcount == argc )
		{
			/* all tables locked */
			status = 0;
		}
		else if ( Ucount == argc )
		{
			/* all tables unlocked */
			status = 4;
		}
		else if ( Lcount + Ucount == argc )
		{
			status = 3;
		}
		else
		{
			status = 2;
		}
		break;
	case 'S':
		if ( quiet == FALSE )
		{
			fprintf( stderr, "%s: retrieved lock status for %d table%s\n",
				prog, argc, argc == 1 ? "" : "s" );
		}
		break;
	case 'L':
		if ( quiet == FALSE )
		{
			if ( lockcount == argc ) {
				fprintf( stderr, "%s: locked %d table%s\n",
					prog, lockcount, lockcount == 1 ? "" : "s" );
			} else {
				Lcount = argc - lockcount;
				fprintf( stderr, "%s: locked %d table%s - %d table%s already locked\n",
					prog, lockcount, lockcount == 1 ? "" : "s",
					Lcount, Lcount == 1 ? "" : "s" );
			}
			/* do not allow signal to remove lockfile(s) */
			lockcount = 0;
		}
		break;
	case 'U':
		if ( quiet == FALSE )
		{
			if ( Ncount == 0 ) {
				fprintf( stderr, "%s: unlocked %d table%s\n",
					prog, Ucount, Ucount == 1 ? "" : "s" );
			} else {
				fprintf( stderr, "%s: unlocked %d table%s - %d table%s already unlocked\n",
					prog, Ucount, Ucount == 1 ? "" : "s",
					Ncount, Ncount == 1 ? "" : "s" );
			}
		}
		break;
	}

	exit( status );
}

usage( )
{
	prmsg( MSG_USAGE, "[-cfq] [-E|-l|-lock|-u|-unlock] <unity_table>..." );
	exit( 1 );
}

set_sig( sig, func )
int sig;
RETSIGTYPE (*func)();
{
	if ( signal( sig, SIG_IGN ) != (RETSIGTYPE (*)())SIG_IGN )
		(void)signal( sig, func );
}

RETSIGTYPE
catch_sig( sig )
int sig;
{
	(void)signal( sig, SIG_IGN );	/* ignore any more signals */

	(void) rmlocks( );

	(void)signal( sig, SIG_DFL );
	kill( getpid( ), sig );

	exit( 2 );	/* if signal comes quickly we'll never get here */
}

rmlocks( )
{
	while ( lockcount > 0 )
	{
		/*
		 * Do not decrement lockcount until after unlink() returns
		 * do avoid race condition with signal handler which calls
		 * this function.
		 */
		(void) unlink( locklist[ lockcount-1 ] );
		--lockcount;
	}
}
