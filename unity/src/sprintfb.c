/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * sprintf(3C) type output for binary data (unsigned integer)
 * similar to %o and %x type conversions where format must
 * start with '%' and end with 'b' (e.g., "%5.4b").
 *
 * If an invalid (unrecognized) or null ("") format is given,
 * then the default format of "%b" will be used.
 */
#include <stdio.h>
#include <ctype.h>

#ifndef	MAXBSTR
#define	MAXBSTR	1024
#endif

#ifdef	__STDC__
int
sprintfb( char *s, char *format, unsigned number )
#else
int
sprintfb( s, format, number )
char		*s;
char		*format;
unsigned	number;
#endif
{
	register int		i;
	register char		*p;
	register char		*q;
	register unsigned	binary;
	int			dot;
	int			minus;
	int			zero;
	int			prec;
	int			width;
	int			len;
	int			fmterr;
	char			bstr[sizeof(unsigned)*8+1];

	prec = 1;	/* default precision */

	dot = minus = zero = width = len = 0;

	fmterr = i = 0;

	/* Make sure format is not a null pointer. */
	if (format != NULL) {

		p = format;

		/* Make sure format starts with '%' */
		if (*p == '%') {

			++p;

			/*
			 * Ignore initial flags that do not apply
			 * to "d,i,o,u,x,X" (and "b") type conversions.
			 */
			if ((*p == '#') || 
			    (*p == '+') ||
			    (*p == ' ')) {
				++p;
			}

		} else {
			++fmterr;
		}
	} else {
		/* format is a NULL pointer */	
		p = "";
	}
	
	while (*p) {
		i = *p++;
		switch (i) {
		case '-':
			if (minus) {
				++fmterr;
			}
			++minus;
			zero = 0;	/* use ' ' to pad width */
			break;
		case '.':
			if (dot) {
				++fmterr;
			}	
			++dot;
			prec = 0;
			zero = 0;	/* use ' ' to pad width */
			break;
		case 'b':
			if (*p != 0) {
				++fmterr;
			}
			break;
		case '0':
			if ((!dot) && (!(minus))) {
				++zero;
			}
			/* FALLTHROUGH */
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			{	register	num;

				num = i - '0';

				while (isdigit(i = *p)) {
					num = num * 10 + i - '0';
					++p;
				}
				if (num < MAXBSTR) {
					if (dot) {
						prec = num;
					} else {
						width = num;
					}
				}
			}
			break;
		default:
			/* invalid format */
			++fmterr;
		}
	}

	if (i != 'b') {
		++fmterr;
	}

	/*
	 * If format specification error or none given
	 * then default to simple binary string.
	 */
	if (fmterr) {
		width = 0;
		prec = 1;
	}
		
	p = bstr + sizeof(bstr) - 1;

	*p = 0;

	binary = number;

	while (binary) {
		*(--p) = '0' + (binary & 1);
		binary = binary >> 1;
	}

	if ((*p == 0) && (prec)) {
		*(--p) = '0';
	}

	len = bstr + sizeof(bstr) - p - 1;

	q = s;

	/* Start out with padding if requested and needed */
	if ((width) && (!(minus))) {
		if ((len < width) && (prec < width)) {
			if (len < prec) {
				i = width - prec;
			} else {
				i = width - len;
			}
			if (zero) {
				while (i) {
					*q++ = '0';
					--i;
				}
			} else {
				while (i) {
					*q++ = ' ';
					--i;
				}
			}
		}
	}
	/* add precision */
	for(i = prec - len; i > 0; i--) {
		*q++ = '0';
	}
	/* add binary digit string */
	for (i = len; i > 0; i--) {
		*q++ = *p++;
	}
	/* end with padding if request and needed */
	if ((width) && (minus)) {
		if ((len < width) && (prec < width)) {
			if (len < prec) {
				i = width - prec;
			} else {
				i = width - len;
			}
			while (i) {
				*q++ = ' ';
				--i;
			}
		}
	}

	*q = 0;

	i = q - s;

	return(i);
}
