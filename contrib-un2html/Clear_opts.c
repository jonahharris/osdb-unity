static char *SCCSID="@(#)Clear_opts.c	2.1.1.3";

/*
 * GP library routine
 * Written by: R. de  Heij
 * LastEditDate="Sun Oct 16 10:01:08 1988"
 */

#include <stdio.h>

extern char *Strcat();

void
Clear_opts(opts,argcp,argvp)
char *opts;			       /* Allowed options according 'getopt' format*/
int *argcp;			       /* pointer to argc */
char **argvp[];			       /* pointer to argv */
{
/* 
 * Reset argc and argv
 * All options will be removed and argv[0] will point to the
 * first argument.
 */

    int     argc;		       /* argc */
    char  **argv;		       /* argv */
    extern int  optind;

    /* Init */
    argc = *argcp;
    argv = *argvp;

    /* Read the options, and reset pointers */
    while (getopt (argc, argv, opts) != EOF);
    *argcp = argc - optind;
    *argvp = &argv[optind];
    optind = 1;
}
