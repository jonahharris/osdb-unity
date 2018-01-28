static char *SCCSID="@(#)Strcpy.c	2.1.1.3";

/*
 * GP Library routine
 *
 * Writen by: R. de Heij
 * LastEditDAte="Sat Dec 19 17:31:18 1987"
 */

#include <stdio.h>
#include <varargs.h>

/*VARARGS*/
char *
Strcpy(out, va_alist)
char *out;			       /* Where to put the output */
va_dcl				       /* The list of string arguments */
{
/*
 * Copy all input words to the output string
 * This function is like sprintf(out,"%s%s%s%s",a, x,y,z);
 * The last string should be equal to NULL
 */

    va_list ap;			       /* arg list */
    char   *arg;		       /* arg pointer*/

    va_start (ap);
    while ((arg = va_arg (ap, char *)) != NULL) {
	while (*out++ = *arg++);
	out--;
    }
    va_end (ap);
    return (out);
}
