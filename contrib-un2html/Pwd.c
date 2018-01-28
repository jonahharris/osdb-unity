static char *SCCSID="@(#)Pwd.c	2.1.1.7";

/*
 * Print working (current) directory
 *      Stolen from pwd.c
 * 
 * Updated by: R. de Heij
 * LastEditDate="Tue Oct 27 12:24:10 1998"  
 */

#ifdef u3b20
#include <lla/lla.h>
#include <lla/log_sub.h>
#endif

#ifdef sun

Pwd(path)
char *path;
{
/* 
 * Call the Sun Getwd function
 * The data is written in path.
 */

    (void) getwd (path);
}

#else

#include	<stdio.h>
#include	<sys/param.h>
#include	<sys/signal.h>
#include	<sys/types.h>
#if !defined(__ppc__)
#include	<sys/sysmacros.h>
#endif
#include	<sys/stat.h>
#if defined(__CYGWIN__)
#include 	<sys/dirent.h>
#define direct dirent
#else
#include	<sys/dir.h>
#endif

extern char *strcpy();
extern void Quit();

static struct	stat	d, dd;
static struct	direct	dir;
static char	dot[]	 = ".";
static char	dotdot[] = "..";
static char	name[512];

static FILE	*file;
static int	off;

static  void
prname(path)
char *path;
{
/* 
 * Copy name into path
 */

    *path++ = '/';
    if (off < 0)
	off = 0;
    name[off] = '\0';
     (void) strcpy (path, name);
}

static void
cat()
{
/* 
 * output name
 */

    register    i,
                j;

    i = -1;
    while ((dir.d_name[++i] != 0) && (i < 14));
    if ((off + i + 2) > 511)
	Quit ("Too long path", 0);
    for (j = off + 1; j >= 0; --j)
	name[j + i + 1] = name[j];
    off = i + off + 1;
    name[i] = '/';
    for (--i; i >= 0; --i)
	name[i] = dir.d_name[i];
}

void
Pwd(path)
char *path;
{
/* 
 * Copy CWD in path
 */

    off = -1;
    for (;;) {
	if (stat (dot, &d) < 0) {
	    Quit ("Cannot stat .", 0);
	}
	if ((file = fopen (dotdot, "r")) == NULL) {
	    Quit ("Cannot open ..", 0);
	}
	if (fstat (fileno (file), &dd) < 0) {
	    Quit ("Cannot stat ..", 0);
	}
	if (chdir (dotdot) < 0) {
	    Quit ("Cannot chdir to ..", 0);
	}
#ifndef u3b20
	    if(d.st_dev == dd.st_dev)
#else
	    if(d.st_devid.dev_mdctptr == dd.st_devid.dev_mdctprt &&
			d.st_devid.dev_partno == dd.st_devid.dev_partno )
#endif
	    {
	    if (d.st_ino == dd.st_ino) {
		prname (path);
		(void) fclose (file);
		if (chdir (path) < 0) {
		    Quit ("Cannot chdir to ", path, 0);
		}
		return;
	    }
	    do
		if (fread ((char *) & dir, sizeof (dir), 1, file) < 1) {
		    Quit ("Read error in ..", 0);
		}
	    while (dir.d_ino != d.st_ino);
	}
	else
	    do {
		if (fread ((char *) & dir, sizeof (dir), 1, file) < 1) {
		    Quit ("Read error in ..", 0);
		}
		(void) stat (dir.d_name, &dd);
	    } 
#ifndef u3b20
		while(dd.st_ino != d.st_ino || d.st_dev != dd.st_dev);
#else
		while( dd.st_ino != d.st_ino ||
			(d.st_devid.dev_mdctptr != dd.st_devid.dev_mdctprt ||
				d.st_devid.dev_partno != dd.st_devid.dev_partno ));
#endif
	(void) fclose (file);
	cat ();
    }
}

#endif
