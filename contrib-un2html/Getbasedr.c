static char *SCCSID="@(#)Getbasedr.c	2.1.1.4";

/* 
 * GP Library routine
 *
 * LastEditDate="Wed Nov  1 12:56:57 1989"
 * Written by: R. de Heij
 */

#ifdef u3b20
#include <lla/lla.h>
#include <lla/log_sub.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#define	SEP	':'

extern void Pwd();
extern void Quit();
extern char *getenv();
extern char *strcat();
extern char *strcpy();
extern char *strrchr();
extern char *strchr();
extern char *malloc();
extern char *Strcat();
extern char *Strcpy();

static char *path;		       /* Path from $PATH */
static int pi;				/* Path index */
static int eos;				/* End Of path String */
static char rootdir[64]; 	       /* Basedir created by Set */
static int setb_tobedone = 1;	       /* Set basedir to be done */

static void
cleanup_path(p)
char *p;			       /* Path */
{
/* Remove not wanted elements from the given path 'p'
 * e.g . and ..
 */

    register char  *in = p;	       /* In pointer */
    register char  *out = p;	       /* Out pointer */

    /* G trough the path p */
    while (*in) {

	/* Check for /./ in the path, remove it */
	if (*in == '/' && *(in + 1) == '.' && *(in + 2) == '/') {
	    in += 2;
	    continue;
	}

	/* Check for /../ in the path, remove it incl previous */
	if (*in == '/' && *(in + 1) == '.' && *(in + 2) == '.' &&
		*(in + 3) == '/') {
	    in += 3;
	    *out = '\0';
	    if ((out = strrchr (p, '/')) == NULL)
		out = p;
	    continue;
	}
	*out++ = *in++;
    }
    *out = '\0';
}

static int 
getdir()
{
/*
 * Get the next directory from path.
 */

    register char   c;		       /* A character */
    register int    index = 0;	       /* Index in array */


    /* Init */
    *rootdir = '\0';

    /* More to go */
    if (eos)
	return (0);

    /* Are we at the end and we have seen a sep */
    if (path[pi] == NULL) {
	if (pi > 0 && path[pi - 1] == SEP) {
	    eos = 1;
	    (void) strcpy (rootdir, ".");
	    return (1);
	}
	else
	    return (0);
    }

    /* Walk through the path */
    while ((c = path[pi++]) != SEP && c != NULL)
	rootdir[index++] = c;
    if (c == NULL)
	eos = 1;
    if (index == 0) {
	(void) strcpy (rootdir, ".");
	return (1);
    }
    rootdir[index++] = NULL;
    return (1);
}

void
Set_basedir(flnm)
char *flnm;
{
/* 
 * Determine where flnm is located as an executable file (where function)
 * The location is saved in rootdir
 * The location of one directory higher is saved. This enables the
 * Get_basedir function to add a subdirectory. In this way subdirectories of
 * A software package can be accessed. eg. the man and bin directoru of NUTS.
 */

    register char  *sptr;	       /* String pointer */
    register int    i;		       /* index */
    struct stat st;		       /* stat bufer */
    char    savewd[256];	       /* save space for working dir */

    /* Mark function exected */
    setb_tobedone = 0;

    /* First check if a / is present */
    /* If it is present, we do not search the path, */
    /*  the prefix is the rootdir */
    if (strchr (flnm, '/') != NULL) {
	(void) strcpy (rootdir, flnm);
    }
    else {

	/* PATH to check */
	if ((path = getenv ("PATH")) == NULL || !*path)
	    Quit ("Shell variable PATH not set or NULL", 0);

	/* Init scanning */
	pi = 0;
	eos = 0;

	/* Check accessibility, and quit if found */
	while (getdir ()) {
	    (void) Strcat (rootdir, "/", flnm, 0);

	    /* Get file mode */
	    if (stat (rootdir, &st) == 0) {

		/* Is it a file? */
		if (st.st_mode & S_IFREG) {

		    /* Is it executable */
		    if (access (rootdir, 01) == 0)
			break;
		}
	    }
	}
    }

    /* Could we find a PATH, no something is wrong */
    if (*rootdir == 0)
	Quit ("Unable to determine the basedir", 0);

    /* Do we have to fill in the CWD */
    if (*rootdir != '/') {
	*savewd = 0;
	(void) Pwd (savewd);
	(void) Strcat (savewd, "/", rootdir, 0);
	(void) strcpy (rootdir, savewd);
    }

    /* remove . and .. from the path */
    cleanup_path (rootdir);

    /* Strip the filename and one directory to come to the package */
    for (i = 0; i < 2; i++) {
	sptr = strrchr (rootdir, '/');
	if (sptr != NULL)
	    *sptr = '\0';
    }
}

char *
Get_basedir(subdir)
char *subdir;
{
/* 
 * Add to the root directory the subdir. 
 */

    char   *retdir;		       /* Return path */

    if (setb_tobedone || !*rootdir)
	Quit ("Rootdir is not set", 0);
    retdir = malloc (64);
    (void) Strcpy (retdir, rootdir, "/", subdir, 0);
    return (retdir);
}
