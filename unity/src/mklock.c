/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "db.h"
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef	ADVLOCK

#include <sys/utsname.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

struct LOCKREC {	
    char fn[MAXPATH+4];		/* Full path name to lock file allowing for "/./" or "././" prefix */
    int fd;			/* File descriptor of lock file */
    struct stat	buf;		/* output from fstat(2) on lock file */
};

static struct LOCKREC *Lockrec; /* Pointer to array structs */
static int Numlockrec;		/* Number of locked files */

#define NUMLOCKREC 2		/* Allocate room for this many lock
				 * file entries per allocation call.
				 */

static struct utsname uts_name;	/* unix system/node name for lock file */

#endif

#ifndef	SEEK_SET
#define	SEEK_SET	0
#endif

#define RETRY		10
#define SLEEPTIME	3

static	int	lockit();

extern	int	errno;

mklock(file,prog,lockfile)
char *file, *prog, *lockfile;
{
	struct	stat	buf;
#ifdef	ADVLOCK
	mode_t	mode;			/* mode for lock file */
	int	fd;
	int	i;
	int	lckrecidx;		/* lock file index into Lockrec[] */
	int	rc;
#else
	int	i,try,ino,dev;
#endif
#if ( defined(ADVLOCK) ) || ( ! defined(TMPLOCK) )
	char *ptr;
#endif
	char lockpath[ MAXPATH+4 ];	/* allow for "/./" or "././" prefix */
				  	/* tmp location for lock file until */
				  	/* we're ready to return successfully */

	lockfile[0] = '\0';	/* no lock file now */

#if ( defined(ADVLOCK) ) || ( ! defined(TMPLOCK) )
	ptr = strrchr( file, '/' );
	if ( ptr == NULL ) {
		strcpy( lockpath, "L" );
		strcat( lockpath, file );
#ifdef	ADVLOCK
		rc = stat( ".", &buf);
#endif
	} else {
		*ptr = '\0';
		strcpy( lockpath, file );
		strcat( lockpath, "/L" );
		strcat( lockpath, ptr+1 );
#ifdef	ADVLOCK
		rc = stat( file, &buf );
#endif
		*ptr = '/';
	}
#endif

#ifdef	ADVLOCK

	/*
	 * check return code from stat(2) of the table directory
	 * which is needed so we can set the correct lock file mode
	 */
	if ( rc != 0 ) {
		perror( prog );
		error( E_GENERAL, "%s: Cannot stat(2) lock file directory.\n", prog );
		return(ERR);
	}
	if ( ( buf.st_mode & ( S_IWOTH | S_IXOTH ) ) == ( S_IWOTH | S_IXOTH ) ) {
		mode = S_IRUSR | S_IRGRP | S_IROTH;
	} else if ( ( buf.st_mode & ( S_IWGRP | S_IXGRP ) ) == ( S_IWGRP | S_IXGRP ) ) {
		mode = S_IRUSR | S_IRGRP;
	} else {
		mode = S_IRUSR;
	}

	/*
	 * For SVR4 machines (ADVLOCK) use the locking routine
	 * from CMS which creates a lockfile and puts a UNIX
	 * advisory lock on it.  This works reliably over NFS.
	 */

	/*
	 * First we must make sure that we have a place to save the lockfile name,
	 * its file descriptor, and output from fstat(2) before attempting to
	 * make the lock file etc. so we do not have to worry about undoing the
	 * the lock file in the event that realloc(3C) fails to allocate memory.
	 * We need to do this since when the file is unlocked, we must first close it.
	 * However closing requires the file's file descriptor, but only the file's name
	 * is passed to the unlockit routine.  This apparent design flaw is from
	 * the fact that the old version (i.e., pre advisory lock) of the lock/unlock
	 * routine needed only the file name).  Because of the large embedded base
	 * of unlockit() calls,  it was decided to just keep the file descriptor info
	 * here and do a lookup on it based on the file name when a request is
	 * made to unlock the file.  The list of locked files is expected to be
	 * quit small, so there is no efficiency issue to be concerned with.
	 * The fstat(2) buffer (space) is needed to allow for the fact that more
	 * than one path can/could be used to reference the lock file.  Therefore,
	 * an fstat(2) is done after creating the lock to get/save the inode etc.
	 * in order to find the entry later if/when attempting to use a different
	 * path later when attempting to unlock the table.
	 */
	for ( lckrecidx = 0; lckrecidx < Numlockrec; lckrecidx++ ) {
		if ( Lockrec[lckrecidx].fn[0] == 0 ) {
	        	break;			/* Found an empty slot */
		}
	}
	if ( lckrecidx == Numlockrec ) {	/* Need to allocate more space */
		Numlockrec += NUMLOCKREC;
		Lockrec = (struct LOCKREC *) realloc( (void*) Lockrec, sizeof(struct LOCKREC)*Numlockrec );
		if ( Lockrec == NULL ) {
			error( E_SPACE, prog );
	        	return(ERR);
	    	}
		for ( i = lckrecidx; i < Numlockrec; i++ ) {	/* Init new entries */
			Lockrec[i].fn[0] = 0;
			Lockrec[i].fd = -1;
		}
	}
	Lockrec[lckrecidx].buf.st_dev = 0;
	Lockrec[lckrecidx].buf.st_ino = 0;
	Lockrec[lckrecidx].buf.st_ctime = 0;
	Lockrec[lckrecidx].buf.st_mtime = 0;

	/* create lock file with UNIX advisory file lock */
	if ( ( fd = lockit( lockpath, mode, &Lockrec[lckrecidx].buf, prog ) ) < 0 ) {
		return(ERR);
	}

	/* save the name and file descriptor of the lock file that we just locked */
	strncpy( Lockrec[lckrecidx].fn, lockpath, MAXPATH+3 );
	Lockrec[lckrecidx].fn[MAXPATH+3] = 0;
	Lockrec[lckrecidx].fd = fd;

	strcpy( lockfile, lockpath );
	return(0);

#else	/* ADVLOCK */

	try = RETRY;
	while(try--) {
#ifdef TMPLOCK
		/* must be careful since the inode will change if
		   one of the unity update commands commits between
		   the stat and the creat
		*/
		if(stat(file,&buf) != 0)	/* caught in middle of commit */
			continue;
		sprintf(lockpath,"%s%d%d","/tmp/",
			(int)buf.st_ino,(int)buf.st_dev);
#endif
		if ((i=creat(lockpath,0)) >= 0 ) {
			close(i);

#ifdef TMPLOCK
			/* check to see that file has not changed */
			ino = (int)buf.st_ino;
			dev = (int)buf.st_dev;
			if(stat(file,&buf) == 0 && (int)buf.st_ino == ino &&
					(int)buf.st_dev == dev) {
				strcpy( lockfile, lockpath );
				return(0);	/* everything okay */
			}

			/* data file changed - need different lock file */
			unlink(lockpath);
#else
			strcpy( lockfile, lockpath );
			return(0);	/* everything okay */
#endif
		}
		sleep(SLEEPTIME);	/* wait and try again */
		error(E_GENERAL, "%s: Data file %s locked. Retrying.\n",
			prog,file);
	}
	if(errno == EACCES) {
		error( E_GENERAL, "%s: Lock file (%s) already exists for table '%s'\n",
			prog, lockpath, file );
		error( E_GENERAL, "Determine who owns lock file and try again.\n");
	}
	else {
		error( E_GENERAL, "%s: Cannot create lock file (%s) for table '%s'\n",
			prog, lockpath, file );
	}

	return(ERR);

#endif	/* else ADVLOCK */

}

cklock( lockfile )
char *lockfile;
{
#ifdef ADVLOCK
	struct stat buf;	/* stat(2) buffer */
	int	i;

	/*
	 * stat(2) the lock file so that device/inode/ctime/mtime
	 * can be used to validate the status of the lock file and
	 * to allow the lock file to be referenced by any valid path.
	 */
	if ( stat( lockfile, &buf ) != 0 ) {
		buf.st_ctime = 0;
		buf.st_mtime = 0;
	}
	for ( i = 0; i < Numlockrec; i++ ) {
		if ( strcmp( lockfile, Lockrec[i].fn ) == 0 ) {
			if ( ( Lockrec[i].buf.st_ctime == buf.st_ctime ) &&
			     ( Lockrec[i].buf.st_mtime == buf.st_mtime ) &&
			     ( Lockrec[i].buf.st_size == buf.st_size ) &&
			     ( Lockrec[i].buf.st_dev == buf.st_dev ) &&
			     ( Lockrec[i].buf.st_ino == buf.st_ino ) ) {
				/*
				 * Valid lock file
				 */
				return( 0 );
			} else {
				/*
				 * lock file has been tampered with
				 */
				return( ERR );
			}
		} else {
			if ( ( buf.st_ctime != 0 ) && ( buf.st_mtime != 0 ) &&
			     ( Lockrec[i].buf.st_ctime == buf.st_ctime ) &&
			     ( Lockrec[i].buf.st_mtime == buf.st_mtime ) &&
			     ( Lockrec[i].buf.st_size == buf.st_size ) &&
			     ( Lockrec[i].buf.st_dev == buf.st_dev ) &&
			     ( Lockrec[i].buf.st_ino == buf.st_ino ) ) {
			     	/*
				 * alternate path reference to valid lock file
				 */
	 			return( 0 );
			}
		}
	}

	return( ERR );
#else
	return( 0 );
#endif
}

rmlock( lockfile )
char *lockfile;
{
#ifdef ADVLOCK
	int	i;
	int	rc;
	int	errno_save;
	struct stat buf;	/* stat(2) buffer */

	/*
	 * Just in case the caller is using a different path to the lock file
	 * we will stat(2) the lock file so that device/inode/ctime can be used
	 * (if needed) in place of path to find the requested lock file entry.
	 */
	if ( stat( lockfile, &buf ) != 0 ) {
		buf.st_ctime = 0;
		buf.st_mtime = 0;
	}
	for ( i = 0; i < Numlockrec; i++ ) {
		if ( ( strcmp( lockfile, Lockrec[i].fn ) == 0 ) ||
		   ( ( buf.st_ctime != 0 ) && ( buf.st_mtime != 0 ) &&
		     ( Lockrec[i].buf.st_ctime != 0 ) &&
		     ( Lockrec[i].buf.st_mtime != 0 ) &&
		     ( Lockrec[i].buf.st_dev == buf.st_dev ) &&
		     ( Lockrec[i].buf.st_ino == buf.st_ino ) ) ) {		/* Got it */
			errno = 0;
			rc = unlink( lockfile );		/* Remove it */
			errno_save = errno;
			(void) close( Lockrec[i].fd );		/* Close it */
			Lockrec[i].fn[0] = 0;			/* Free the entry */
			Lockrec[i].fd = -1;
			Lockrec[i].buf.st_ctime = 0;
			Lockrec[i].buf.st_mtime = 0;
			errno = errno_save;
	 		return( rc );
		}
	}
#endif
	return( unlink( lockfile ) );
}

#ifdef	ADVLOCK

/******************************************************************************

	Routine	lockit
	    The lockit() function is used by all UNITY commands to lock and
	    unlock reations on SVR4 (ADVLOCK) machines.  The code is from ECMS.

	    Method

	        UNIX advisory locking is used (i.e., the UNIX lockf(3) routine).
	        The UNIX advisory locking mechanism provides the following
	        advantages over the old method:

        	    1) Lock is automatically removed when a process exits,
        	       even if the process is killed or aborts, so
        	       inadvertent locks cannot be left around.

        	    2) Supported by NFS, so lock honored by all CPUs on which
        	       file system is mounted.

	Description:
	    Lock/unlock an UNITY database file using UNIX advisory locking
	    (i.e., the lockf(3) routine) to place an advisory lock on the
	    lock file after creating the given lock file.  The lock file
	    is created with the specified mode which defaults to 0400 if
	    not set to 0000, 0400, 0440, or 0444.  RETRY indicates how
	    many attempts should be made to create the lock file if/when
	    another process has the table/file locked.

	    If the rmlock() and/or unlink() routines is not called (e.g.,
	    because the program aborts or is killed), the advisory lock is
	    automatically removed from the lock file.  However the file on
	    which the lock is placed is left around in this case and will
	    be removed the next time the file is to be locked (unless it
	    was created with mode 0000 which requires unlink(2) to remove it).

	Arguments:
	    arg1	char * to file to create and place an advisory lock
	    arg2	mode_t containing the file creation mode for the lock file
	    arg3	struct stat * buffer to fstat(2) the locked lock file

	Environment Variables:
	    NONE

	Files:
	    As defined by arg1

	Return Status:
	    -1 (error) or File Descriptor to the lock file

*******************************************************************************/

#define	LOCKPERM	0400	/* default lock file permissions if invalid mode */

static int
lockit( lockfile, mode, buf, prog )
char *lockfile;
mode_t mode;
struct stat *buf;
char *prog;
{
	int nodelen;
	int lockfd;			/* File descriptor of lock file */
	int retrycount;
	char	lockinfo[SYS_NMLN+12];	/* buffer to read node and pid from lock file */
	mode_t	omask;			/* original umask(2) */
	struct	flock flockbuf;		/* fcntl(2) record lock buffer */
	struct	stat sbuf, statbuf;	/* fstat(2) buffers */
	unsigned retestcount;		/* lock file retest counter */

	/* get unix system/node name for the lock file */
	(void) uname( &uts_name );
	nodelen = strlen( uts_name.nodename );

	/*
	 * First create the file on which we're to place the advisory lock.  If
	 * that file already exists, then we will test it to see of another process
	 * has it locked.  If the lock file exists and is not locked by another
	 * process then we will retest it after sleeping a few seconds in order
	 * to allow for/avoid race conditions with an existing process that may
	 * be in the middle of locking the file after having created it.
	 * If its already locked, we make RETRY attempts at SLEEPTIME second
	 * intervals before we tell the caller it's locked.
	 */

	switch (mode) {
	case 0000:
	case 0400:
	case 0440:
	case 0444:
		break;
	default:
		mode = LOCKPERM;
	}

	retrycount = RETRY;
	retestcount = 0;
	statbuf.st_dev = 0;
	statbuf.st_ino = 0;
	statbuf.st_ctime = 0;
	statbuf.st_mtime = 0;

	while ( 1 ) {
		errno = 0;
		/* allow lock file to be created with the requested permissions */
		omask = umask( 02 );
		lockfd = open( lockfile, O_RDWR|O_CREAT|O_EXCL, mode );
		/* restore original umask(2) */
		(void) umask( omask );
		if ( lockfd < 0 ) {
			if ( errno == EEXIST ) {	/* Lock file already exists */
				errno = 0;
		    		/* attempt to simply open the file to get FD for lockf(3) call */
				if ( ( ( lockfd = open( lockfile, O_RDONLY ) ) >= 0 ) &&
				       ( fstat( lockfd, &sbuf ) == 0 ) ) {
					flockbuf.l_type = F_RDLCK;
					flockbuf.l_whence = 0;
					flockbuf.l_start = 0;
					flockbuf.l_len = 0;
#ifndef NOL_SYSID
					flockbuf.l_sysid = 0;
#endif
					flockbuf.l_pid = 0;
					if ( ( fcntl( lockfd, F_GETLK, &flockbuf ) == 0 ) &&
					     ( flockbuf.l_type == F_UNLCK ) ) {
						/*
						 * Susposedly not locked by another process
						 */
						if ( ( sbuf.st_dev == statbuf.st_dev) &&
						     ( sbuf.st_ino == statbuf.st_ino) &&
						     ( sbuf.st_ctime == statbuf.st_ctime ) &&
						     ( sbuf.st_mtime == statbuf.st_mtime ) ) {
							/*
							 * If we keep getting the same
							 * unlocked lock file then
							 * it should be safe for us
							 * to cleanup unused lock file
							 * if we can place a read lock
							 * on it and if we are on the
							 * same system where the lock file
							 * was originally created since
							 * we cannot trust the lock status
							 * with some NFS file systems.
							 * However, in order to avoid
							 * potential race conditions
							 * with another process that may
							 * also be trying to cleanup this
							 * same old lock file, we need to
							 * go back to sleep for a few
							 * seconds before attempting to
							 * create a new lock file after
							 * removing the old lock file
							 * so we do not want to remove
							 * the old lock file if the
							 * retry count will not let us
							 * get back to the top of the
							 * loop to have a shot at creating
							 * a new lock file.
							 */
							if ( ( retrycount > 1 ) &&
#ifndef	TRUST_LOCKS
							     ( sbuf.st_size >= nodelen + 1 ) &&
#endif
							     ( ++retestcount >= 3 ) ) {
								flockbuf.l_type = F_RDLCK;
								flockbuf.l_whence = 0;
								flockbuf.l_start = 0;
								flockbuf.l_len = 0;
#ifndef NOL_SYSID
								flockbuf.l_sysid = 0;
#endif
								flockbuf.l_pid = 0;
#ifdef	TRUST_LOCKS
								if ( fcntl( lockfd, F_SETLK, &flockbuf ) == 0 ) {
#else
								if ( ( fcntl( lockfd, F_SETLK, &flockbuf ) == 0 ) &&
								     ( read( lockfd, lockinfo, nodelen + 1) == nodelen + 1 ) &&
								     ( lockinfo[ nodelen ] == '\n' ) &&
								     ( strncmp( lockinfo, uts_name.nodename, nodelen ) == 0 ) ) {
#endif
									retestcount = 0;
									statbuf.st_ctime = 0;
									statbuf.st_mtime = 0;
									(void) unlink( lockfile );
								}
							}
						} else {
							/*
							 * Save lock file device, inode, and creation time
							 * so that we can retest, after sleeping a few seconds
							 * in order to avoid potential race conditions, to see
							 * if this lock file is in fact no longer valid.
							 */
							statbuf = sbuf;
							retestcount = 0;
						}
					}
		    		}
				if ( lockfd >= 0 ) {
					(void) close( lockfd );
				}
			} else {
				perror( prog );
	        		error( E_GENERAL, "%s: Cannot create lock file %s\n", prog, lockfile ); 
	    			return(-1);
			}
		} else {
			char	infobuf[sizeof(lockinfo)];
			unsigned	infolen;
			/*
			 * lock the entire file and write the
			 * sytem (node) name terminated with
			 * a new-line into the lock file
			 */
			sprintf( lockinfo, "%s\n%u\n", uts_name.nodename, getpid() );
			infolen = strlen( lockinfo );
			infobuf[ infolen ] = '\0';	/* for strcmp() after read() */
			flockbuf.l_type = F_WRLCK;
			flockbuf.l_whence = 0;
			flockbuf.l_start = 0;
			flockbuf.l_len = 0;
#ifndef NOL_SYSID
			flockbuf.l_sysid = 0;
#endif
			flockbuf.l_pid = 0;
			if ( ( fcntl( lockfd, F_SETLK, &flockbuf ) == 0 ) &&
			     ( write( lockfd, lockinfo, infolen ) == (int) infolen ) &&
			     ( fstat( lockfd, &sbuf ) == 0 ) &&
			     ( sbuf.st_size == infolen ) &&
			     ( lseek( lockfd, 0, SEEK_SET ) != -1 ) &&
			     ( read( lockfd, infobuf, infolen ) == (int) infolen ) &&
			     ( strcmp( infobuf, lockinfo) == 0 ) &&
			     ( stat( lockfile, buf ) == 0 ) &&
			     ( buf->st_size == sbuf.st_size ) &&
			     ( buf->st_ctime == sbuf.st_ctime ) &&
			     ( buf->st_mtime == sbuf.st_mtime ) &&
			     ( buf->st_dev == sbuf.st_dev ) &&
			     ( buf->st_ino == sbuf.st_ino ) ) {
				/*
				 * sucessfully locked - return FD to the lock file
				 */
				return(lockfd);
			}

			/* cleanup before next attempt to lock it */
			(void) unlink( lockfile );
			(void) close( lockfd );
		}

		if ( --retrycount > 0 ) {
			sleep( SLEEPTIME );
		} else {
			error( E_GENERAL, "%s: Table is currently locked by lock file %s\n", prog, lockfile );
			return(-1);
		}
	}
}

#endif	/* ADVLOCK */
