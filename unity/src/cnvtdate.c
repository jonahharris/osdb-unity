/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "db.h"
#include <ctype.h>
#include <time.h>

#ifndef	__STDC__
#define	time_t	long
#endif

#ifndef	FALSE
#define	FALSE	0
#endif
#ifndef	TRUE
#define	TRUE	1
#endif

#define MINUTE	60		/* seconds in minute */
#define HOUR	(60 * MINUTE)	/* seconds in hour */
#define DAY	(24 * HOUR)	/* seconds in day */
#define MONTH	(31 * DAY)	/* max seconds in month */
#define YEAR	(12 * MONTH)	/* max seconds in year */

#define RANGESTART 60			/* # of years back to start window */

#define GETDATE \
    if (! havedate) { \
	CurrTime = time((time_t *) 0);	/* Get current time */ \
	TimeData = localtime(&CurrTime);  /* Populate date/time struct */ \
					/* Get last 2 digits of so.range */ \
	RangeStart = TimeData->tm_year%100-RANGESTART; \
					/* Get century of so.range */ \
	RangeStartCentury = TimeData->tm_year-TimeData->tm_year%100+1900; \
	if (RangeStart < 0) { 		/* If so.range is negative */\
		RangeStart += 100; 	/* Adjust so.range */\
		RangeStartCentury -= 100; /* Adjust century of so.range */\
	} \
	havedate = 1; \
    }

static int havedate = 0;		/* =1 if already got today's date */
static int RangeStart;			/* Last 2 digits of range start */
static int RangeStartCentury;		/* Century year of RangeStart */

static int dt_add2year;			/* set by cnvtdate() if date is valid */
static int dt_year;			/* set by cnvtdate() if date is valid */
static int dt_month;			/* set by cnvtdate() if date is valid */
static int dt_day;			/* set by cnvtdate() if date is valid */
static int dt_hour;			/* set by cnvtdate() if date is valid */
static int dt_minute;			/* set by cnvtdate() if date is valid */
static int dt_second;			/* set by cnvtdate() if date is valid */

struct tm *TimeData;			/* Structure for date & time data */
time_t	CurrTime;			/* Current time in seconds */

extern struct tm *localtime();		/* Function for getting dates */
extern time_t time();			/* The function that gets the time */

init_cnvtdate( )
{
	/*
	 * Reset havedate so that the current date (year) window
	 * is recalculated the next time cnvtdate() is called.
	 */
	havedate = 0;

	return( 0 );	/* in case the caller cares */
}

get_dt_buffers( add2year, year, month, day, hour, minute, second )
int	**add2year;
int	**year;
int	**month;
int	**day;
int	**hour;
int	**minute;
int	**second;
{
	if ( add2year )
		*add2year = &dt_add2year;
	if ( year )
		*year = &dt_year;
	if ( month )
		*month = &dt_month;
	if ( day )
		*day = &dt_day;
	if ( hour )
		*hour = &dt_hour;
	if ( minute )
		*minute = &dt_minute;
	if ( second )
		*second = &dt_second;
	return( 0 );
}

double
cnvtdate( date, plustime )
char *date;
int plustime;
{
	register unsigned long dtime;
	register long num;
	register char *p, *q;

	/*
	 * Initialize year to zero (0), which is never valid,
	 * so applications calling this function to parse a
	 * date or date plus time string can determine whether
	 * or not a valid date was obtained if this function
	 * fails to sucessfully parse the entire input string.
	 */
	dt_year = 0;

	/*
	 * Get date adjustment if needed for missing century.
	 */
	GETDATE;

	/*
	 * Remove any initial white space
	 * and save pointer to beginning
	 * of the first set of digits.
	 */
	p = date;
	while ( isspace( *p ) ) {
		++p;
	}
	/*
	 * We need to more or less blindly
	 * process the first set of digits
	 * and get to its terminating character
	 * to determine the expected format
	 * of the remaining subfields (if any).
	 */
	for ( num = 0, q = p; isdigit( *q ); q++ ) {
		num = num * 10 + *q - '0';
	}
	if ( p == q ) {		/* no initial digit string */
		return( 0L );
	}
	/*
	 * We processed at least one digit
	 * so now we need to check for a
	 * valid termination character
	 * and/or valid digit count and
	 * proceed accordingly.
	 */
	if (( *q == '/' ) || ( *q == '-' ))
	{
		/* MM/DD/YY or MM-DD-YY type format */
		if (( num < 1 ) || ( num > 12 )) {
			return( 0L );	/* month is out-of-range */
		}
		dt_month = num;
		/*
		 * Remove any leading white space
		 * before the number of days.
		 */
		for ( p = q + 1; isspace( *p ); p++ )
			;
		for ( num = 0; isdigit( *p ); p++ ) {
			num = num * 10 + *p - '0';
		}
		if (( num < 1 ) || ( num > 31 ) || ( *p != *q )) {
			/*
			 * Day is out-of-range or not terminated
			 * by the same character as the month was.
			 */
			return( 0L );
		}
		dt_day = num;
		/*
		 * Remove any leading white space
		 * before the number of years.
		 */
		for ( q = p + 1; isspace( *q ); q++ )
			;
		for ( num = 0, p = q; isdigit( *p ); p++ ) {
			num = num * 10 + *p - '0';
		}
		if (( num > 9999 ) ||
		   (( num <= 0 ) && (( p - q <= 0 ) || ( p - q >= 3 )))) {
			/*
			 * Note: Zero (0) is not a valid year
			 *	 if it includes the century.
			 */
			return( 0L );	/* invalid year */
		}
		/*
		 * Adjust year based on current year if needed.
		 */
		if (( num == 0 ) || (( num < 100 ) && ( p - q <= 2 ))) {
			if ( num < RangeStart ) {
				num += RangeStartCentury + 100;
			} else {
				num += RangeStartCentury;
			}
			dt_add2year = TRUE;
		} else {
			dt_add2year = FALSE;
		}
		dt_year = num;
	}
	else if ( *q == '.' )
	{
		/* DD.MM.YY type format */
		if (( num < 1 ) || ( num > 31 )) {
			return( 0L );	/* day is out-of-range */
		}
		dt_day = num;
		/*
		 * Remove any leading white space
		 * before the number of months.
		 */
		for ( p = q + 1; isspace( *p ); p++ )
			;
		for ( num = 0; isdigit( *p ); p++ ) {
			num = num * 10 + *p - '0';
		}
		if (( num < 1 ) || ( num > 12 ) || ( *p++ != '.' )) {
			return( 0L );	/* month is out-of-range */
		}
		dt_month = num;
		/*
		 * Remove any leading white space
		 * before the number of days.
		 */
		while ( isspace( *p ) ) {
			++p;
		}
		for ( num = 0, q = p; isdigit( *p ); p++ ) {
			num = num * 10 + *p - '0';
		}
		if (( num > 9999 ) ||
		   (( num <= 0 ) && (( p == q ) || ( p - q >= 3 )))) {
			/*
			 * Note: Zero (0) is not a valid year
			 *	 if it includes the century.
			 */
			return( 0L );	/* invalid year */
		}
		/*
		 * Adjust year based on current year if needed.
		 */
		if (( num == 0 ) || (( num < 100 ) && ( p - q <= 2 ))) {
			if ( num < RangeStart ) {
				num += RangeStartCentury + 100;
			} else {
				num += RangeStartCentury;
			}
			dt_add2year = TRUE;
		} else {
			dt_add2year = FALSE;
		}
		dt_year = num;
	}
	else if (( *q == '\0' ) || ( isspace( *q ) ))
	{
		int len = q - p;
		/*
		 * Use length of digit string to determine format.
		 */
		switch ( len ) {
		case  6:	/* YYMMDD	*/
		case 12:	/* YYMMDDHHMMSS	*/
			for ( num = 0, q = p + 2; p < q; p++ ) {
				num = num * 10 + *p - '0';
			}
			if ( num < RangeStart ) {
				num += RangeStartCentury + 100;
			} else {
				num += RangeStartCentury;
			}
			dt_add2year = TRUE;
			break;
		case 8:		/* YYYYMMDD	*/
		case 14:	/* YYYYMMDDHHMMSS */
			for ( num = 0, q = p + 4; p < q; p++ ) {
				num = num * 10 + *p - '0';
			}
			if ( num == 0 ) {
				return( 0L );	/* zero (0) is an invalid year */
			}
			dt_add2year = FALSE;
			break;
		default:
			return( 0L );	/* invalid date format */
		}
		dt_year = num;
		/*
		 * p and q both point to start of month digits
		 */
		for ( num = 0, q = p + 2; p < q; p++ ) {
			num = num * 10 + *p - '0';
		}
		if (( num < 1 ) || ( num > 12 )) {
			dt_month = 0;	/* invalid month */
			return( 0L );
		}
		dt_month = num;
		for ( num = 0, q = p + 2; p < q; p++ ) {
			num = num * 10 + *p - '0';
		}
		if (( num < 1 ) || ( num > 31 )) {
			dt_day = 0;	/* invalid day */
			return( 0L );
		}
		dt_day = num;
		/*
		 * If TIME digits were included then process them now.
		 */
		if ( len >= 12 ) {
			for ( num = 0, q = p + 2; p < q; p++ ) {
				num = num * 10 + *p - '0';
			}
			if ( num >= 24 ) {
				dt_hour = -1;	/* invalid hour */
				return( 0L );
			}
			dt_hour = num;
			for ( num = 0, q = p + 2; p < q; p++ ) {
				num = num * 10 + *p - '0';
			}
			if ( num >= 60 ) {
				dt_minute = -1;	/* invalid minute */
				return( 0L );
			}
			dt_minute = num;
			for ( num = 0, q = p + 2; p < q; p++ ) {
				num = num * 10 + *p - '0';
			}
			/*
			 * Seconds can be from 0-59
			 */
			if ( num >= 60 ) {
				dt_second = -1;	/* invalid second */
				return( 0L );
			}
			dt_second = num;
			/*
			 * If not EOS then only allow trailing white space.
			 */
			if ( *q ) {
				while ( isspace( *q ) ) {
					++q;
				}
				if ( *q ) {
					return( 0L );	/* trailing garbage */
				}
			}
			/*
			 * Calculate and return result.
			 */
			dtime = (unsigned long)((dt_month - 1) * MONTH);
			dtime += (unsigned long)((dt_day - 1) * DAY);
			if ( plustime ) {
				dtime += (unsigned long)((dt_hour) * HOUR);
				dtime += (unsigned long)((dt_minute) * MINUTE);
				dtime += (unsigned long)dt_second;
			}
			return( ((double) dt_year * YEAR) + dtime );
		}
	}
	else
	{
		return( 0L );	/* invalid date format */
	}
	if ( *p ) {
		/*
		 * Remove any trailing space.
		 */
		while ( isspace( *p ) ) {
			++p;
		}
	}
	if ( *p == '\0' ) {
		/*
		 * Date does not include TIME component
		 * so set time variables to zero (0) and
		 * then calculate and return the result.
		 */
		dt_hour = 0;
		dt_minute = 0;
		dt_second = 0;
		dtime = (unsigned long)((dt_month - 1) * MONTH);
		dtime += (unsigned long)((dt_day - 1) * DAY);
		return( ((double) dt_year * YEAR) + dtime );
	}
	/*
	 * We have removed the white space after the DATE
	 * portion of the date field so now q should be
	 * pointing at a string of digits which are expected
	 * to be of the form HH:MM:SS or HH.MM.SS or HHMMSS.
	 */
	for ( num = 0, q = p; isdigit( *q ); q++ ) {
		num = num * 10 + *q - '0';
	}
	if ( q == p ) {
		dt_hour = -1;	/* invalid TIME component */
		return( 0L );
	}
	else if ( q - p == 6 )
	{
		/* TIME is formatted as HHMMSS digit string */
		for ( num = 0, q = p + 2; p < q; p++ ) {
			num = num * 10 + *p - '0';
		}
		if ( num >= 24 ) {
			dt_hour = -1;	/* invalid hour */
			return( 0L );
		}
		dt_hour = num;
		for ( num = 0, q = p + 2; p < q; p++ ) {
			num = num * 10 + *p - '0';
		}
		if ( num >= 60 ) {
			dt_minute = -1;	/* invalid minute */
			return( 0L );
		}
		dt_minute = num;
		for ( num = 0, q = p + 2; p < q; p++ ) {
			num = num * 10 + *p - '0';
		}
		/*
		 * Seconds can be from 0-59
		 */
		if ( num >= 60 ) {
			dt_second = -1;	/* invalid second */
			return( 0L );
		}
		dt_second = num;
	} else {
		/*
		 * TIME should be formatted as HH:MM:SS
		 * or HH.MM.SS or HH:MM or HH.SS
		 * and num contains the HH digits.
		 */
		if (( num >= 24 ) || ( *q != ':' && *q != '.')) {
			dt_hour = -1;	/* invalid hour */
			return( 0L );
		}
		dt_hour = num;
		for ( num = 0, p = q + 1; isdigit( *p ); p++ ) {
			num = num * 10 + *p - '0';
		}
		if (( p <= q + 1 ) || ( num >= 60 )) {
			dt_minute = -1;	/* invalid minute */
			return( 0L );
		}
		dt_minute = num;
		if ( *p == *q ) {
			++p;	/* skip over ':' or '.' character */
			for ( num = 0, q = p; isdigit( *p ); p++ ) {
				num = num * 10 + *p - '0';
			}
			if (( p == q ) || ( num >= 60 )) {
				dt_second = -1;	/* invalid second */
				return( 0L );
			}
			dt_second = num;
		} else {
			dt_second = 0;
		}
		if ( *p ) {
			while ( isspace( *p ) ) {
				++p;
			}
			if ( *p ) {
				return( 0L );	/* trailing garbage */
			}
		}
	}
	/*
	 * Calculate and return result.
	 */
	dtime = (unsigned long)((dt_month - 1) * MONTH);
	dtime += (unsigned long)((dt_day - 1) * DAY);
	if ( plustime ) {
		dtime += (unsigned long)((dt_hour) * HOUR);
		dtime += (unsigned long)((dt_minute) * MINUTE);
		dtime += (unsigned long)dt_second;
	}
	return( ((double) dt_year * YEAR) + dtime );
}

