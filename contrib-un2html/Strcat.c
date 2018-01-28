static char *SCCSID="@(#)Strcat.c	2.1.1.3";

/*
 * GP Library routine
 *
 * Writen by: R. de Heij
 * LastEditDAte="Mon May  9 19:44:48 1988"
 */

#include <stdio.h>
#include <varargs.h>

/*VARARGS*/
char *
Strcat(out, va_alist)
register char *out;		       /* Where to put het oputput */
va_dcl				       /* list of string arguments */
{
/*
 * Append all input words to the output string
 * This function is like sprintf(out,"%s%s%s%s",out, x,y,z);
 * The last string should be equal to NULL
 */

    va_list ap;			       /* Arg list */
    register char  *arg;	       /* arg pointer */

    va_start (ap);
    while (*out)		       /* to the end */
	out++;
    while ((arg = va_arg (ap, char *)) != NULL) {
	while (*out++ = *arg++);
	out--;
    }
    va_end (ap);
    return (out);
}
