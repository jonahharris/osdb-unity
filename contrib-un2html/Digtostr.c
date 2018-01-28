static char *SCCSID="@(#)Digtostr.c	2.1.1.4";

/*
 * GP Library routine
 *
 * Writen by: R. de Heij
 * LastEditDAte="Tue May 14 14:36:51 1991"
 */

#define MAXCALLS	10

static char string[MAXCALLS][25];  /* string */
static int  i = 0;		       /* modulus counter */

char *
Digtostr(dig,len)
register int dig;			       /* Digits in decimal format */
int len;			       /* minimum length */
{
/*
 * Return pointer to character string representing dig
 * This function is equal to sprintf(to,"%ld",dig);
 * This function uses static data.
 * To make the function more flexable an internal stack of MAXCALLS is
 * created. This makes it possible to make MAXCALLS calls to the
 * function without loosing the data of the previous calls.
 * The malloc function is not used because this fucntion is called a
 * lot and than programs run out of space often.
 */

    register int    power = 1;	       /* power processing */
    register int    places = 1;	       /* places processing */
    register char  *sptr;	       /* string pointer */

    /* Allocate space */
    i = i % MAXCALLS;
    sptr = string[i];

    /* Process sign */
    if (dig < 0) {
	*sptr++ = '-';
	dig = -dig;
    }

    /* First find maximum size */
    while ((dig / power) >= 10) {
	power = power * 10;
	places++;
    }

    /* Add zeroes */
    for (; places < len; places++) {
	*sptr++ = '0';
    }

    /* Convert */
    for (; power > 0; power = power / 10) {
	*sptr++ = '0' + (dig / power);
	dig = dig % power;
    }
    *sptr = '\0';
    return (string[i++]);
}
