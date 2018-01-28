static char *SCCSID="@(#)Set_option.c	2.1.1.3";

/*
 * GP library routine
 *
 * Written by: R. de  Heij
 * LastEditDate="Wed Jan 11 18:13:59 1989"
 */

#include <stdio.h>

extern char *Strcat();
    extern int  optind;
    extern char *optarg;

int
Set_option(opts,opt,argc,argv,to)
char *opts;			       /* Allowed options according 'getopt' format */
char opt;			       /* Wanted option */
int argc;			       /* Argc */
char **argv;			       /* Argv */
char **to;			       /* Where do we put the argument */
{
/* 
 * Search the given opt, return 1 if option specified on the
 * command line else 0.
 * If a argument is given to an option, place the address in 'to'.
 * This is only done if to != NULL.
 * If no argument is given to will not be initialized, this allowes 
 * presetting of the argument; Only if an argument is given it will be
 * overwritten.
 */

    register int    c;		       /* a char */
    int     returncode = 0;	       /* returncode */

    /* Read the options, and find wanted one */
    while ((c = getopt (argc, argv, opts)) != EOF) {

	/* Found */
	if (c == opt) {

	    /* Place the address in to if wanted */
	    if (to != NULL)
		*to = optarg;
	    returncode = 1;
	}
    }

    /* Return false */
    optind = 1;
    return (returncode);
}
