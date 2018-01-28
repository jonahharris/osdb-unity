static char *SCCSID="@(#)Strsrch.c	2.1.1.3";

/*
 *   GP Library routine 
 *
 *   Written by: R. de Heij 
 *   LastEditDate="Sat Dec 12 20:00:05 1987"
 */

#include <stdio.h>

char *
Strsrch(line,string)
char *line;			       /* Line to search for string */
char *string;			       /* search string */
{
/*
 * Look for string in line,  if found return startpoint else NULL 
 */

    register char  *lineptr;	       /* line ptr */
    register char  *strptr;	       /* string ptr */
    int     maxlen;		       /* max startposition */

    /* Early return if empty string */
    if (!*string)
	return (line);

    /* Calc max startpos */
    maxlen = strlen (line) - strlen (string) + 1;

    /* For all start positions */
    while (maxlen-- > 0) {
	lineptr = line;
	strptr = string;

	/* As long as they are equal */
	while (*strptr++ == *lineptr++)/* If at and of string we match */
	    if (!*strptr)
		return (line);

	/* Increment startpoint */
	line++;
    }

    /* Unequal */
    return (NULL);
}
