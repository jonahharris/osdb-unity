static char *SCCSID="@(#)Curr_date.c	2.1.1.4";

/*
 *   GP Library routine 
 *
 *   Written by: R. de Heij 
 *   LastEditDate="Sat Feb 20 23:22:32 1999"
 */

#include <time.h>

extern long time();
extern struct tm *localtime();
extern char *Strcpy();
extern char *strcpy();
extern char *Digtostr();

static char date[20];	       /* created date */

char *
Curr_date(type)
int type;			       /* Type of date to return */
{
/*
 * Return current date as a string according type:
 * Type == 1: YY-MM-DD HH:MM
 * Type == 2: MMDDHHMM
 * Type == 3: YYMMDD
 * Type == 4: YYMMDDHHMMSS
 * Type == 5: MMDDHHMMSS
 * Type == 6: MM/DD/YY
 * Type == 7: YYYYMMDD
 * This function uses static data, so next call destroys info of this call!!
 */

    struct tm  *tbuf;		       /* Time buffer */
    long    clock;		       /* clock count */
    char    yyyy[5];		       /* year YYYY */
    char    yy[3];		       /* year */
    char    mm[3];		       /* month */
    char    dd[3];		       /* day */
    char    HH[3];		       /* hour */
    char    MM[3];		       /* minute */
    char    SS[3];		       /* second */

    /* Init */
    clock = time ((long *) 0);
    tbuf = localtime (&clock);
    (void) strcpy (yy, Digtostr ((tbuf -> tm_year % 100), 2));
    (void) strcpy (yyyy, Digtostr (((tbuf -> tm_year)+1900), 4));
    (void) strcpy (mm, Digtostr (tbuf -> tm_mon + 1, 2));
    (void) strcpy (dd, Digtostr (tbuf -> tm_mday, 2));
    (void) strcpy (HH, Digtostr (tbuf -> tm_hour, 2));
    (void) strcpy (MM, Digtostr (tbuf -> tm_min, 2));
    (void) strcpy (SS, Digtostr (tbuf -> tm_sec, 2));

    /* Create output */
    switch (type) {
	case 1: 
	    (void) Strcpy (date, yy, "-", mm, "-", dd, " ", HH, ":", MM, 0);
	    break;
	case 2: 
	    (void) Strcpy (date, mm, dd, HH, MM, 0);
	    break;
	case 3: 
	    (void) Strcpy (date, yy, mm, dd, 0);
	    break;
	case 4: 
	    (void) Strcpy (date, yy, mm, dd, HH, MM, SS, 0);
	    break;
	case 5: 
	    (void) Strcpy (date, mm, dd, HH, MM, SS, 0);
	    break;
	case 6: 
	    (void) Strcpy (date, mm, "/", dd, "/", yy, 0);
	    break;
	case 7: 
	    (void) Strcpy (date, yyyy, mm, dd, 0);
	    break;
    }
    return (date);
}
