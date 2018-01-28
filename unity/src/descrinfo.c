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
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>

static char description[] = "%description\n";
static char enddescription[] = "%enddescription\n";

extern	char tmpItable[];
extern	char DtmpItable[];
extern	FILE *udfp0;

FILE *
getdescrwtbl( fp, tmpfile )
FILE	*fp;
char	**tmpfile;
{
	char *curchar;
	int ch;
	int i, j;

	curchar = description;
	while ( (ch = getc( fp )) != EOF )
	{
		if ( *curchar != ch )
			break;

		if ( *++curchar == '\0' )	/* everything matched */
		{
			break;
		}
	}

	if (*curchar != '\0') {
		(void)ungetc( ch, fp );		/* put non-matching char back */
		return( NULL );
	}

	/*
	 * Define name of a temporary file based on the current process ID
	 * with special "/./" prefix so that the given (tmp) directory will
	 * be searched first when looking for the Descriptor file.
	 */
	sprintf(tmpItable, "/./tmp/noI%05u", getpid());
	/*
	 * set DtmpItable for datafile (/tmp) directory
	 */
	getfile(DtmpItable, tmpItable, 1);

	if ((udfp0 = fopen(DtmpItable, "w")) != NULL) {
		curchar = enddescription;
		i = 0;
		j = '\n';
		while ( (ch = getc( fp )) != EOF )
		{
			if (j == '\n') {
				if (*curchar == ch) {
					if (*++curchar == '\0') {
						/* everything matched */
						break;
					}
					++i;
				} else {
					for (j = 0, curchar = enddescription; j < i && *curchar != '\0'; j++, curchar++) {
						if (putc(*curchar, udfp0) == EOF) {
							fclose(udfp0);
							udfp0 = NULL;
							unlink(DtmpItable);
							DtmpItable[0] = '\0';
							tmpItable[0] = '\0';
							return(NULL);
						}
					}
					curchar = enddescription;
					i = 0;
					j = ch;
					if (putc(ch, udfp0) == EOF) {
						fclose(udfp0);
						udfp0 = NULL;
						unlink(DtmpItable);
						DtmpItable[0] = '\0';
						tmpItable[0] = '\0';
						return(NULL);
					}
				}
			} else {
				j = ch;
				if (putc(ch, udfp0) == EOF) {
					fclose(udfp0);
					udfp0 = NULL;
					unlink(DtmpItable);
					DtmpItable[0] = '\0';
					tmpItable[0] = '\0';
					return(NULL);
				}
			}
		}

		/* close the temporary descriptor file */
		if (fclose(udfp0) == EOF) {
			udfp0 = NULL;
			unlink(DtmpItable);
			DtmpItable[0] = '\0';
			tmpItable[0] = '\0';
			return(NULL);
		}
		udfp0 = NULL;

		if (*curchar == '\0') {
			/* don't write to a null pointer */
			if (tmpfile != NULL) {
				*tmpfile = tmpItable;
			}
			return(fp);	/* Success */
		}

		unlink(DtmpItable);		/* remove useless descriptor file */
		DtmpItable[0] = '\0';
		tmpItable[0] = '\0';
	}

	return(NULL);
}

FILE *
putdescrwtbl( fp, dfile )
FILE	*fp;
char	*dfile;
{
	FILE *dfp;
	char *curchar;
	int ch;

	if ((dfile == NULL) || (dfile[0] == '\0') ||
	   ((dfp = fopen(dfile, "r")) == NULL)) {
		return(NULL);
	}

	curchar = description;

	while (*curchar) {
		if (putc(*curchar++, fp) == EOF) {
			fclose(dfp);
			return(NULL);
		}
	}

	while ( (ch = getc( dfp )) != EOF )
	{
		if (putc(ch, fp) == EOF) {
			fclose(dfp);
			return(NULL);
		}
	}

	curchar = enddescription;

	while (*curchar) {
		if (putc(*curchar++, fp) == EOF) {
			fclose(dfp);
			return(NULL);
		}
	}

	fclose(dfp);

	if (fflush(fp) == EOF) {
		return(NULL);
	}

	return(fp);	/* Success */
}
