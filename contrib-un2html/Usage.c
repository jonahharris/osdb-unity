static char *SCCSID="@(#)Usage.c	2.1.1.7";

/*
 * GP Library routine
 *
 * Written by R. de Heij
 * LastEditdate="Mon Aug  9 19:50:53 1993"
 */

#include <stdio.h>
#include "LIB.h"

extern void Set_basedir();
extern char *Strcpy();
extern char *strrchr();
extern char *Get_basedir();
extern void Quit();

extern int optind;
extern int Quit_value;
extern int Quit_prt;
extern char *Quit_prog;
extern void Print_break();

static char **inargv;
static int inargc;

/* Allowed helpchars default = -h */
char *Helpchars = "h";

static void
usage_and_quit(exitval,progname,usage)
int exitval;			       /* Exit value */
char *progname;			       /* program name */
char *usage;			       /* Usage string */
{
/* 
 * Print usage and quit
 */
    char    errmes[512];
    int i;

    Quit_value = exitval;
    Quit_prt = 0;
    if (progname != NULL) {
        (void)strcpy(errmes,"+");
        for (i=0;i<inargc;i++){
		Strcat(errmes," ",inargv[i],0);
	}
	Strcat(errmes,"\n\n",0);
	Print_break (stderr, errmes, 75);
	(void) Strcpy (errmes, "Usage: ", progname, " [-",Helpchars,"] ",
		usage, "\n", 0);
	Print_break (stderr, errmes, 75);
    }
    Quit (0);
}

void
Usage(alwopt,minarg,maxarg,usage,argc,argv,exitval)
char *argv[];			       /* argv */
char *usage;			       /* short usage string */
char *alwopt;			       /* allowed options according getopt */
int exitval;			       /* exit value */
int argc;			       /* argc */
int minarg;			       /* minimum nr of arguments */
int maxarg;			       /* maximum nr of arguments */
{
/*
 * - Print usage:
 *   if wrong nr of arguments (minarg,maxarg) or not allowed option
 *   (alwopt)
 * - Print Help:
 *   If one of arguments = one from Helpchars
 *   Look for the page on basedir/man
 * - On error exit(exitval) is executed
 * - The check for minarg/maxarg can be switched off by making
 *   min/maxarg == -1
 */

    int     c;			       /* a char */
    char    manual[256];	       /* filenam to manual dir */
    int     quit = 0;		       /* quit indic */
    char    newalwopt[256];	       /* options plus -h */
    char   *progname;		       /* prgram name */
    char    cmd[256];		       /* Command string */

    /* Save arguments */
    inargv=argv;
    inargc=argc;

    /* Strip path from name */
    if ((progname = strrchr (*argv, '/')) == NULL)
	progname = *argv;
    else
	progname++;

    /* Set Progname for Quit function */
    Quit_prog = progname;

    /* Check options */
    (void) Strcpy (newalwopt, Helpchars, alwopt, 0);
    while ((c = getopt (argc, argv, newalwopt)) != EOF) {

       /* help */
       if(strrchr(Helpchars,c)!=NULL){

		/* try help */
		Set_basedir (argv[0]);
		(void) Strcpy (manual, Get_basedir ("man"), "/", progname, ".1", 0);
		if (access (manual, 04) != 0) {
		    FPUTS ("No manual page to print\n", stderr);
		    usage_and_quit (exitval, progname, usage);
		}
		else {
		    (void) Strcpy (cmd, "nroff -T37 -man ", manual, "|pg", 0);
		    (void) system (cmd);
		    usage_and_quit (exitval, NULL, NULL);
		}
		/* NOTREACHED */
	}
	else if (c == '?')
	{
	
		/* Illegal option */
		quit = 1;
		break;
	}
    }

    /* Any wrong options */
    if (quit)
	usage_and_quit (exitval, progname, usage);

    /* Check nr of arguments */
    if (minarg != -1 && ((argc - optind) < minarg)) {
	FPUTS ("Missing arguments\n", stderr);
	usage_and_quit (exitval, progname, usage);
    }
    if (maxarg != -1 && ((argc - optind) > maxarg)) {
	FPUTS ("Redundant arguments\n", stderr);
	usage_and_quit (exitval, progname, usage);
    }

    /* Reset options reading */
    optind = 1;
}
