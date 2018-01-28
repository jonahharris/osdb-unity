/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "format.h"
#include "message.h"

#define MINPGWIDTH	10	/* minimum page width */
#define MINDATAWIDTH	3	/* minimum space for data printing */
#define MINLMARGIN	3	/* minimum space for field names */
#define MINPGLENGTH	10	/* minimum lines per page */

#define PGWIDTH		79	/* default page width */
#define SUPERWIDE	120	/* default wide page width */
#define PGLINES		66	/* default page lines */
#define BOTTOM		4	/* default blank lines at bottom of page */
#define LMARGIN		35	/* default left margin for printing */

#define FLDBRKCH	'\0'

struct fldinfo {
	char *name;
	short lines;
};

/*
 * Formatting flags
 */
#define FRM_PAGINATE	0x0001		/* paginate the output */
#define FRM_NEWPAGE	0x0002		/* each record on new page */
#define FRM_DBLSPACE	0x0004		/* double space between fields */
#define FRM_NOBLANK	0x0008		/* don't print empty fields */
#define FRM_ENUMERATE	0x0010		/* enumerate the records */
#define FRM_VARMARGIN	0x0020		/* variable width margin, depending */
					/* on max of field name lengths */
#define FRM_LEFTJUST	0x0040		/* left-justify field names by  */
					/* remove leading (white) space */
#define FRM_NOHYPHENS	0x0080		/* don't print the (2) hyphen lines  */

char *prog;

int recincr = 0;			/* enumerated record number increment */
int enumgap = 1;			/* blank lines after --- Record %d --- */
int offset = 0;
int lmargin = LMARGIN;
int pglines = PGLINES;
int pgwidth = PGWIDTH;
int datawidth;
char hyphoffch = ' ';
char fldbrkch = FLDBRKCH;
char flddelim = FLDDELIM;
char dblspace;
unsigned short printflags = 0;

char *enumformat = "--- Record %d ---";
char *headline, *footline;
char *infile;

int fldcnt, user_fldcnt;
struct fldinfo fldlist[MAXATT];

extern char *basename();
extern char *getenv();


main( argc, argv )
int argc;
char **argv;
{
	int curpage, curline;
	int i, total;
	int recno;
	struct formatio frmio;
	char *currec[MAXATT];
	char *unity_prb;

	prog = basename( *argv );

	unity_prb = getenv("UNITY_PRB");

	while( --argc > 0 ) {
		if ( **++argv == '-' ) {
			i = parseoption( argc, argv );
			argc -= i;
			argv += i;
		}
		else
			fldlist[fldcnt++].name = *argv;
	}

	if ( user_fldcnt > 0 ) {
		while( fldcnt < user_fldcnt )
			fldlist[fldcnt++].name = "";
	}

	if ( fldcnt <= 0 ) {
		prmsg( MSG_ERROR, "no field names given on command line" );
		usage();
	}

	if (unity_prb != NULL) {
		char *p;
		char *q;
		int value, alignlm;

		alignlm = 0;

		p = unity_prb;

		while ((p != NULL) && ((q = strchr(p, '=')) != NULL)) {

			i = q++ - p;

			switch (i) {
			case 7 :
				if (strncmp(p, "alignlm", i) == 0) {
					if (( *q == 'y' ) || ( *q == 'Y' )) {
						alignlm = 1;
					} else {
						alignlm = 0;
					}
				} else if (strncmp(p, "enumfmt", i) == 0) {
					char *commaptr;
					int len;
					commaptr = strchr( q, ',' );
					if ( commaptr ) {
						len = commaptr - q;
					} else {
						len = strlen(q);
					}
					if (( len >= 1 ) && ( len < 1024 )) {
						static char enumprbuf[1024];
						strncpy( enumprbuf, q, len );
						enumprbuf[len] = '\0';
						cnvtbslsh( enumprbuf, enumprbuf );
						enumformat = enumprbuf;
					}
				} else if (strncmp(p, "enumgap", i) == 0) {
					if (isdigit(*q)) {
						value = atoi(q);
						if (((value != 0) || (*q == '0')) &&
						    (value <= 2)) {
							enumgap = value;
						}
					}
				}
				break;
			default:
				break;
			}
			if ((p = strchr(q, ',')) != NULL) {
				++p;
			}
		}
		if ( alignlm )
			printflags |= FRM_LEFTJUST;
	}

	if ( infile ) {
		int fd;

		if ( (fd = open( infile, 0 )) < 0 ) {
			prmsg( MSG_ERROR, "cannot open input file '%s'",
				infile );
			exit( 2 );
		}
		initformatio( &frmio, fd );
	}
	else
		initformatio( &frmio, fileno( stdin ) );

	if ( printflags & FRM_LEFTJUST ) {
		char *p;

		for( i = 0; i < fldcnt; i++ ) {
			p = fldlist[i].name;
			for( total = 0; *p == ' '; p++, total++) ;
			if (( total ) && ( *p != '\0' )) {
				fldlist[i].name = p;
			}
		}
	}

	if ( printflags & FRM_VARMARGIN ) {
		total = 0;
		lmargin = 0;
		for( i = 0; i < fldcnt; i++ ) {
			total = strlen( fldlist[i].name ) + 2;
			if ( total > lmargin )
				lmargin = total;
			fldlist[i].lines = 1;
		}
	}
	else {
		for( i = 0; i < fldcnt; i++ )
			fldlist[i].lines = fldlines( fldlist[i].name, lmargin - 1 );
	}

	datawidth = pgwidth - lmargin;
	if ( datawidth < MINDATAWIDTH ) {
		datawidth = MINDATAWIDTH;
		lmargin = pgwidth - datawidth;
	}

	/*
	 * Below is the general printing code, which is basically:
	 *
	 *	For each record
	 *	   If paginating and the lines needed for this
	 *		    record go beyond the page boundary:
	 *		Print out any footer.
	 *		Print out any header for next page.
	 *		Reset all counts.
	 *	   else
	 *		Print each field of the record.
	 */
	curpage = 1;
	recno = 0;
	if ( printflags & FRM_PAGINATE ) {
		curline = pr_header( curpage );
		while( getrec( &frmio, fldcnt, flddelim, currec, FALSE ) )
		{
			total = curline + reclines( currec );
			if ( (printflags & FRM_NEWPAGE) ||
					total > pglines - BOTTOM + 1 )
			{
				if ( recno != 0 )
				{
					pr_footer( curpage, curline );
					curpage += (total + BOTTOM - 1) / pglines;
					curline = pr_header( curpage );
				}
				else
					curpage += (total + BOTTOM - 1) / pglines;
			}
			curline += pr_record( currec, ++recno );
		}
		pr_footer( curpage, curline );
	}
	else
	{
		while( getrec( &frmio, fldcnt, flddelim, currec, FALSE ) )
			(void)pr_record( currec, ++recno );
	}

	exit( 0 );
}

static char *
findbrk( value, width )
char *value;
int width;
{
	register char *end;
	register int length, tmplen;

	length = strlen( value );
	if( fldbrkch )
	{
		tmplen = length <= width ? length : width;

		/*
		 * Look for embedded newline character.
		 */
		for( end = value; end < value + tmplen; end++ ) {
			if ( *end == fldbrkch || *end == '\n')
				return( end );
		}
	}

	if ( length > width ) {
		/*
		 * Look for a good point to break the field
		 */
		if ( value[width] != ' ' ) {
			for( end = &value[width - 1]; end > value; end-- ) {
				if ( isspace( *end ) || ispunct( *end ) )
					return( end + 1 );
			}
		}
		return( value + width );
	}
	else
		return( value + length );
}

static char *
fld_print( value, width, fill )
char *value;		/* beginning of field to print */
int width;		/* maximum field width */
int fill;
{
	/*
	 * Print width characters from the string "value".  If the
	 * end of "value" is reached before printing width characters and
	 * "fill" is TRUE, pad the field with blanks.  If "value" cannot
	 * all be printed a pointer to the next character is returned.
	 * In this case, the field is broken at the last occurance of the
	 * fldbrkch character if specified or a non-alphanumeric character.
	 */
	register char *end;
	register char oldch;

	if ( value == NULL || *value == '\0' ) {
		if ( fill ) {
			while( width-- > 0 ) {
				putchar( ' ' );
			}
		}
		return( NULL );
	}

	end = findbrk( value, width );

	/*
	 * Print the field . . .
	 */
	oldch = *end;
	*end = '\0';
	fputs( value, stdout );
	*end = oldch;

	/*
	 * . . . and pad to the end with spaces.
	 */
	if ( fill ) {
		register int length;

		for( length = width - (end - value); length-- > 0; )
			putchar( ' ' );
	}

	/*
	 * Bypass any trailing spaces.
	 */
	for ( ; *end == ' '; end++ )
		;

	/*
	 * Return next pointer to next character or, if none, NULL.
	 */
	if ( *end == '\n' || ( fldbrkch && *end == fldbrkch ) )
		return( end[1] ? &end[1] : NULL );
	else
		return( *end ? end : NULL );
}

static int
fldlines( value, width )
register char *value;	/* beginning of field to print */
int width;		/* maximum field width */
{
	/*
	 * This is a modified version of fld_print() which just returns
	 * the number of lines need to print the field value.
	 */
	register int linecnt;

	linecnt = 0;
	while( value && *value ) {
		linecnt++;
		value = findbrk( value, width );

		if ( *value == '\n' || (fldbrkch && *value == fldbrkch) )
			value++;
		else
			for( ; *value == ' '; value++ )
				;
	}

	return( linecnt > 0 ? linecnt : 1 );
}

reclines( currec )
char **currec;
{
	register int i, linecnt, needlines;
	register char *value;

	linecnt = 0;
	if ( printflags & FRM_NEWPAGE ) {
		if ( printflags & FRM_ENUMERATE )
			linecnt = 2;
	}
	else if ( printflags & FRM_NOHYPHENS ) {
		if ( printflags & FRM_ENUMERATE )
			linecnt = 4;
		else
			linecnt = 2;
	}
	else
		linecnt = 4;

	for( i = 0; i < fldcnt; i++ ) {
		value = currec[i];

		if ( (printflags & FRM_NOBLANK) &&
				(value == NULL || *value == '\0') )
			continue;

		needlines = fldlines( value, datawidth );
		linecnt += needlines >= fldlist[i].lines ? needlines :
					fldlist[i].lines;

		if ( printflags & FRM_DBLSPACE )
			linecnt++;
	}

	return( linecnt );
}

pr_header( curpage )
int curpage;
{
	int linecnt;

	fputs( "\n\n", stdout );
	linecnt = 2;
	if ( headline ) {
		printf( headline, curpage );
		fputs( "\n\n\n", stdout );
		linecnt += 3;
	}

	return( linecnt );
}

pr_footer( curpage, curline )
int curpage;
int curline;
{
	for( ; curline < pglines - BOTTOM + 1; curline++ )
		putchar('\n' );

	if ( footline )
		printf( footline, curpage );

	fputs( "\n\n", stdout );
	curline += 2;

	while( curline++ < pglines )
		putchar( '\n' );
}

pr_record( currec, recno )
char **currec;
int recno;
{
	register int i, linecnt;
	register char *value, *name;

	linecnt = 0;
	if ( printflags & FRM_NEWPAGE ) {
		if ( printflags & FRM_ENUMERATE ) {
			for( i = offset; i != 0; --i )
				putchar( ' ' );
			printf( enumformat, recno + recincr );
			putchar( '\n' );
			linecnt = 1;
			for ( i = enumgap; i != 0; --i ) {
				putchar( '\n' );
				++linecnt;
			}
		}
	}
	else if ( printflags & FRM_NOHYPHENS ) {
		linecnt = 2;
		printf( "\n\n" );
		if ( printflags & FRM_ENUMERATE ) {
			for( i = offset; i != 0; --i )
				putchar( ' ' );
			printf( enumformat, recno + recincr );
			putchar( '\n' );
			++linecnt;
			for ( i = enumgap; i != 0; --i ) {
				putchar( '\n' );
				++linecnt;
			}
		}
	}
	else {
		linecnt = 4;
		putchar( '\n' );
		for( i = offset; i != 0; --i )
			putchar( hyphoffch );
		for( i = 0; i < pgwidth; i++ )
			putchar( '-' );
		putchar( '\n' );
		i = 0;
		if ( printflags & FRM_ENUMERATE ) {
			char buf[20];

			if (( offset ) && ( hyphoffch == ' ' ))
				for( i = offset; i != 0; --i )
					putchar( ' ' );
			sprintf( buf, "%d", recno + recincr );
			fputs( buf, stdout );
			putchar( ' ' );
			if (( offset ) && ( hyphoffch == '-' ))
				for( i = offset; i != 0; --i )
					putchar( '-' );
			i = strlen( buf ) + 1;
		} else {
			for( i = offset; i != 0; --i )
				putchar( hyphoffch );
			i = 0;			
		}
		for( ; i < pgwidth; i++ )
			putchar( '-' );
		fputs( "\n\n", stdout );
	}

	for( i = 0; i < fldcnt; i++ ) {
		value = currec[i];
		name = fldlist[i].name;

		if ( (printflags & FRM_NOBLANK) &&
				(value == NULL || *value == '\0') )
			continue;

		while( value || name ) {
			int j;

			for( j = offset; j != 0; --j )
				putchar( ' ' );
			name = fld_print( name, lmargin - 1, TRUE );
			putchar( ' ' );
			value = fld_print( value, datawidth, FALSE );
			putchar( '\n' );
			linecnt++;
		}

		if ( printflags & FRM_DBLSPACE ) {
			if ( dblspace ) {
				int j;

				for( j = offset; j != 0; --j )
					putchar( ' ' );
				for( j = 0; j < pgwidth; j++ )
					putchar( dblspace );
			}
			putchar( '\n' );
			linecnt++;
		}
	}

	return( linecnt );
}

parseoption( argc, argv )
int argc;
char *argv[];
{
	register char *option, *optarg;
	int extra;

	extra = 0;
	for( option = &argv[0][1]; *option; option++ ) {
		switch( *option ) {
		case 'a':
			printflags |= FRM_VARMARGIN;
			break;
		case 'b':
		case 'B':
			printflags |= FRM_NOBLANK;
			break;
		case 'D':
			printflags |= FRM_DBLSPACE;
			dblspace = '\0';
			if ( option[1] ) {
				++option;
				if ( ! isspace( *option ) )
					dblspace = *option;
			}
			break;
		case 'd':
			flddelim = option[1] ? *++option : FLDDELIM;
			break;
		case 'e':
			printflags |= FRM_ENUMERATE;
			recincr = 0;
			if ( ( option[1] ) && ( isdigit( option[1] ) ) ) {
				optarg = &option[1];
				option = "\0";
			}
			else if ( ( argc > 0 ) && ( isdigit( argv[1][0] ) ) ) {
				optarg = *++argv;
				--argc;
				++extra;
			}
			else {
				recincr = 0;
				continue;
			}
			recincr = atoi( optarg );
			if ( recincr > 0 ) {
				/* subtract one from first rec# to get increment */
				--recincr;
			} else {
				recincr = 0;
			}
			break;
		case 'F':
			user_fldcnt = 0;
			if ( option[1] ) {
				optarg = &option[1];
				option = "\0";
			}
			else if ( argc > 0 ) {
				optarg = *++argv;
				--argc;
				++extra;
			}
			else
				continue;
			
			if ( *optarg < '0' || *optarg > '9' )
				prmsg( MSG_ERROR, "unrecognized field count '%s'",
					optarg );
			else {
				user_fldcnt = atoi( optarg );
				if ( user_fldcnt > MAXATT )
					user_fldcnt = MAXATT;
			}
			break;
		case 'f':
			if ( option[1] ) {
				footline = &option[1];
				option = "\0";
				cnvtbslsh( footline, footline );
				printflags |= FRM_PAGINATE;
			}
			else if ( argc > 0 ) {
				argc--;
				extra++;
				footline = *++argv;
				cnvtbslsh( footline, footline );
				printflags |= FRM_PAGINATE;
			}
			break;
		case 'h':
			if ( option[1] ) {
				headline = &option[1];
				cnvtbslsh( headline, headline );
				option = "\0";
				printflags |= FRM_PAGINATE;
			}
			else if ( argc > 0 ) {
				argc--;
				extra++;
				headline = *++argv;
				cnvtbslsh( headline, headline );
				printflags |= FRM_PAGINATE;
			}
			break;
		case 'i':
			if ( option[1] ) {
				infile = &option[1];
				option = "\0";
			}
			else if ( argc > 0 ) {
				infile = *++argv;
				--argc;
				++extra;
			}
			break;
		case 'L':
		case 'l':
			if ( option[1] ) {
				optarg = &option[1];
				option = "\0";
			}
			else if ( argc > 0 ) {
				optarg = *++argv;
				--argc;
				++extra;
			}
			else
				continue;

			printflags |= FRM_PAGINATE;
			if ( *optarg < '0' || *optarg > '9' )
				prmsg( MSG_ERROR, "unrecognized line count '%s'",
					optarg );
			else {
				pglines = atoi( optarg );
				if ( pglines < MINPGLENGTH )
					pglines = MINPGLENGTH;
			}
			break;
		case 'n':
			fldbrkch = option[1] ? *++option : FLDBRKCH;
			break;
		case 'm':
			if ( option[1] ) {
				optarg = &option[1];
				option = "\0";
			}
			else if ( argc > 0 ) {
				optarg = *++argv;
				--argc;
				++extra;
			}
			else
				continue;

			if ( *optarg == '-' ) {
				if ( optarg[1] == '\0' ) {
					printflags |= FRM_LEFTJUST;
					continue;
				}
				if ( optarg[1] >= '0' && optarg[1] <= '9' ) {
					printflags |= FRM_LEFTJUST;
					++optarg;
				}
			}
			if ( *optarg < '0' || *optarg > '9' )
				prmsg( MSG_ERROR, "unrecognized left margin '%s'",
					optarg );
			else {
				lmargin = atoi( optarg );
				if ( lmargin < MINLMARGIN )
					lmargin = MINLMARGIN;
			}
			break;
		case 'o':
			if ( option[1] ) {
				optarg = &option[1];
				option = "\0";
			}
			else if ( argc > 0 ) {
				optarg = *++argv;
				--argc;
				++extra;
			}
			else
				continue;

			if ( *optarg == '-' ) {
				if ( optarg[1] == '\0' ) {
					printflags |= FRM_NOHYPHENS;
					continue;
				}
				if ( optarg[1] >= '0' && optarg[1] <= '9' ) {
					hyphoffch = '-';
					offset = atoi( ++optarg );
					continue;
				}
			} else {
				if ( *optarg >= '0' && *optarg <= '9' ) {
					hyphoffch = ' ';
					offset = atoi( optarg );
					continue;
				}
			}
			prmsg( MSG_ERROR, "unrecognized page offset '%s'",
				optarg );
			break;
		case 'P':
			printflags |= FRM_PAGINATE|FRM_NEWPAGE;
			break;
		case 'p':
			printflags |= FRM_PAGINATE;
			break;
		case 'w':
		case 'W':
			if ( ( option[1] ) && ( isdigit( option[1] ) ) ) {
				optarg = &option[1];
				option = "\0";
			}
			else if ( ( argc > 0 ) && ( isdigit( argv[1][0] ) ) ) {
				optarg = *++argv;
				--argc;
				++extra;
			}
			else {
				pgwidth = SUPERWIDE;	/* default for -w */
				continue;
			}

			if ( *optarg < '0' || *optarg > '9' ) {
				prmsg( MSG_ERROR, "unrecognized width '%s'",
					optarg );
				pgwidth = PGWIDTH;
			}
			else {
				pgwidth = atoi( optarg );
				if ( pgwidth < MINPGWIDTH )
					pgwidth = MINPGWIDTH;
			}
			break;
		}
	}

	return( extra );
}

usage( )
{
	prmsg( MSG_USAGE, "[-aBepP] [-d<fld_delim>] [-D[<dblspc_char>]] [-F<col_cnt>] \\" );
	prmsg( MSG_CONTINUE, "[-f<pg_footer>] [-h<pg_header>] [-L<pg_length>] [-n<newline_char>] \\" );
	prmsg( MSG_CONTINUE, "[-m<fldname_margin>] [-o<page_offset>] [-W<page_width>] \\" );
	prmsg( MSG_CONTINUE, "<field_names>..." );

	exit( 1 );
}
