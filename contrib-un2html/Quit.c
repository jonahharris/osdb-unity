static char *SCCSID="@(#)Quit.c	2.1.1.4";

/*
 * GP Library routine
 *
 * Writen by: R. de Heij
 * LastEditDAte="Thu Feb 28 14:19:00 1991"
 */

#include <stdio.h>
#include <varargs.h>
#include "LIB.h"

extern void exit();
extern int errno;
extern char *Strcpy();
extern char *Curr_date();

void (*Quit_function)() = (void(*)())0;/* Function to execute on exit */
int Quit_value = -1;		       /* exit value */
int Quit_prt = 1;		       /* print error */
char *Quit_prog = NULL;		       /* Program calling quit */
int Quit_time = 0;		       /* Print date and time on  */

/*VARARGS*/
void
Quit(va_alist)
va_dcl				       /* List of string pinters */
{
/*
 * Write all arguments to stderr,  A NL is added
 * The last argument should be NULL
 * If Quit_prt == TRUE and Quit_value > 0 the word ERROR is prepended
 * Before the exit function is called the routine stored in Quit_function
 * is called (if present). The errormessage is passed as an argument.
 * Then exit(Quit_value) is called
 * The Quit_prog variable can be used to print also the calling program
 * name in front of the error.
 * If Quit_time == TRUE the data and time is printed also.
 * The error message can only be maximum 1024 characters
 */

    va_list ap;			       /* arg list */
    char   *arg;		       /* arg pointer */
    char    mes[1024];		       /* message */
    char   *sptr = mes;		       /* pointer to write mes */
    void (*ftx) ();		       /* Function to execute */

    /* Create output message */
    va_start (ap);
    *sptr = 0;
    while (((arg = va_arg (ap, char *)) != NULL) &&
	    (sptr + strlen (arg) < &mes[1024])) {
	sptr = Strcpy (sptr, arg, 0);
    }
    va_end (ap);

    /* If there is a message */
    if (*mes) {

	/* Start with a CR to be sure it comes on a new line */
	FPUTS ("\n", stderr);

	/* Print the program name? */
	if (Quit_prog != NULL) {
	    FPUTS (Quit_prog, stderr);
	    FPUTS (": ", stderr);
	}

	/* Print ERROR if needed */
	if (Quit_prt && Quit_value != 0) {
	    FPUTS ("ERROR - ", stderr);
	}

	/* Print message */
	FPUTS (mes, stderr);

	/* Print date and time */
	if (Quit_time) {
	    FPUTS (" (", stderr);
	    FPUTS (Curr_date (1), stderr);
	    FPUTS (")", stderr);
	}

	/* Print CR */
	FPUTC ('\n', stderr);
    }

    /* Execute end function,  but reset Quit_function to prohibit */
    /*  recursive calls */
    if (Quit_function != (void (*) ()) 0) {
	ftx = Quit_function;
	Quit_function = (void (*) ()) 0;
	(void) (*ftx) (mes);
    }
    exit (Quit_value);
}
