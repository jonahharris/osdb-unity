static char *SCCSID="@(#)Fopen.c	2.1.1.3";

/*
 *   GP Library routine 
 *
 *   Written by: R. de Heij 
 *   LastEditDate="Thu Apr 27 20:11:06 1989"
 */

#include <stdio.h>
#include <pwd.h>

extern void Quit();
extern unsigned short geteuid();
extern unsigned short getuid();
extern struct passwd *getpwuid();

FILE *
Fopen(flnm,type)
char *flnm;			       /* Filename to open */
char *type;			       /* type like fopen */
{
/*
 * Open file in desired mode and return pointer
 * The type is equal to the modes described in the fopen system call
 * Switch off umask function during the open. The parent directory will
 * give the mode bits now. The uid and gid of the file will be set to the
 * euid.
 */

    FILE * fptr;		       /* File pointer */
    int     euid;		       /* eff user id */
    struct passwd  *ids;	       /* Password file entry */

    /* Do the open and return proper error */
    (void) umask (02);
    if ((fptr = fopen (flnm, type)) == NULL)
	Quit ("Cannot open ", flnm, 0);

    /* If the file is written in super user mode (euid != uid) than set
       both the uid and gid to the uid of the euid */
    if (*type == 'w' || *type == 'a') {
	euid = (int) geteuid ();
	if (getuid () != euid) {
	    ids = getpwuid (euid);
	    (void) chown (flnm, ids -> pw_uid, ids -> pw_gid);
	}
    }
    return (fptr);
}
