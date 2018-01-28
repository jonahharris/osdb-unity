static char *SCCSID="@(#)Printbrk.c	2.1.1.5";

/*
 * GP Library routine
 *
 * Written by R. de Heij
 * LastEditdate="Fri Oct 21 15:41:07 1994"
 */

#include <stdio.h>
#include "LIB.h"

extern char *Strcpy();
extern char *strrchr();
extern char *strncpy();

/* String to use when a line is ended */
char *Print_brk_str = "\n\t";

void
Print_break (outfptr, message, len)
FILE *outfptr;			       /* Output file pointer */
char *message;			       /* message to print */
int len;			       /* max length of one line */
{
/* 
 * Print the string to the stream but take care of max linelen of len
 */

    char    toprint[256];	       /* Line to print */
    char   *space;		       /* Pointer to last space */

    /*  Are we done */
    if (strlen (message) <= len) {
	FPUTS (message, outfptr);
	return;
    }

    /* copy initial size */
    (void) strncpy (toprint, message, len);
    toprint[len] = '\0';

    /* Break on space */
    space = strrchr (toprint, ' ');
    if (space != NULL)
	*space = '\0';

    /* Print it */
    FPUTS (toprint, outfptr);
    FPUTS (Print_brk_str, outfptr);
    Print_break (outfptr, message + strlen (toprint) + 1, len);
}
