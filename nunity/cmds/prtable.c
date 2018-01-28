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

/*
 * Printing flags and data for formatting the information;
 */
extern int prflags;		/* boolean flags for printing */

#define PR_DBLSPACE	0x0001
#define PR_FOOTER	0x0002
#define PR_HEADER	0x0004
#define PR_PAGINATE	0x0008
#define PR_UNIQUE	0x0010
#define PR_ALPHABRK	0x0020
#define PR_ENUMERATE	0x0040
#define PR_BOX		0x0080
#define PR_ONPAGE	0x0100

/*
 * Default definitions
 */
#define PGWIDTH		79	/* default page width */
#define SUPERWIDE	120	/* default wide page width */
#define PGLINES		66	/* default page lines */
#define BOTTOM		4	/* blank lines at bottom of page */
#define TOP		2	/* blank lines at top of page */
#define AFTER_PGHDR	3	/* blank lines after page header */
#define GAP		2	/* blank lines between sections */
#define LNOMAX		9999	/* line number max-based on print width */
#define	LNOMSK		0x7fff	/* mask for extracting line number */
#define NUMLEN		5	/* length of line number plus space */
#define LMARGIN		0	/* default left margin for printing */

#define COLSEP		'|'	/* default column separator */
#define BOXSEP		'-'	/* default horizontal separator */
#define ROWSEP		' '	/* default char for double spacing */
#define FLDBRKCH	'~'	/* defualt new-line break character */

#define DFLT_FLDWIDTH	12	/* default field width if none given */

/*
 * Flags for table printing
 */
#define F_DUPLICATE	0x0001

#define J_LEFT		0x0010
#define J_RIGHT		0x0020
#define J_CENTER	0x0030

#define JUSTIFYMASK	0x0030		/* justification mask */

#define OLDRECBLKSIZE	64

struct oldrecblock {
	short blkcnt;
	struct oldrecblock *next;
	char *fldvals[OLDRECBLKSIZE][MAXATT];
};

struct fldinfo {
	char *name;
	short width;
	short flags;
};

struct section {
	short start;	/* starting field number for section */
	short fldcnt;	/* field cnt for section */
	struct fldinfo *fldptr;
	short width;
};

char *prog;

char *enumformat = "%4d ";	/* default format for enumerated lineno */
char *enumformatwrap = "%04d ";	/* format after enum wrap-around to zero */

int lineno;
int lmargin = LMARGIN;
int pglines = PGLINES;
int pgwidth = PGWIDTH;
char colsep = COLSEP;
char boxsep = BOXSEP;
char rowsep = ROWSEP;
char fldbrkch = '\0';
char flddelim = FLDDELIM;
int prflags = PR_HEADER|PR_FOOTER|PR_BOX;
int bottom = BOTTOM;
int top = TOP;
int gap = GAP;
int after_pghdr = AFTER_PGHDR;
int onlyNsections = 0;
int in1space = 0;

char *fldline, *headline, *footline;

int fldcnt, widthcnt, user_fldcnt;
struct fldinfo fldlist[MAXATT];

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
extern char *malloc();
#endif
extern char *basename();
extern char *getenv();

main( argc, argv )
int argc;
char **argv;
{
	int i, total;
	int curpage, curline, staticlines, dblspace;
	char *infile = NULL;
	int seccnt;
	struct section *secptr;
	register char **currec, **prevrec;
	int prevlineno;
	struct oldrecblock *oldrecs;
	struct formatio frmio;
	char *rec1[MAXATT], *rec2[MAXATT];
	struct section seclist[MAXATT];
	char *unity_prt;

	prog = basename( *argv );

	unity_prt = getenv("UNITY_PRT");

	while( --argc > 0 ) {
		register char *option;
		char *arg;

		if ( **++argv != '-' ) {
			fldlist[fldcnt++].name = *argv;
			continue;
		}
		for( option = &argv[0][1]; option && *option; option++ ) {
			switch( *option ) {
			case 'b':
				prflags |= PR_ALPHABRK;
				break;
			case 'C':
				if ( option[1] ) {
					fldline = &option[1];
					option = "\0";
				}
				else if ( argc > 0 ) {
					argc--;
					fldline = *++argv;
				}
				else {
					prmsg( MSG_ERROR, "no column heading given with -C option" );
					usage( );
				}
				break;
			case 'c':
				if ( option[1] ) {
					getwidths( option + 1, J_CENTER );
					option = "\0";
				}
				else if ( argc > 0 ) {
					getwidths( *++argv, J_CENTER );
					argc--;
				}
				else {
					prmsg( MSG_ERROR, "no width given for centered field (-c)" );
					usage();
				}
				break;
			case 'D':
				prflags |= PR_DBLSPACE;
				rowsep = option[1] ? *++option : ROWSEP;
				break;
			case 'd':
				flddelim = option[1] ? *++option : FLDDELIM;
				break;
			case 'e':
				arg = NULL;
				if ( option[1] ) {
					if ( isdigit( option[1] ) ) {
						arg = &option[1];
						option = "\0";
					}
				}
				else if ( argc > 0 && isdigit( *argv[1] ) ) {
					arg = *++argv;
					--argc;
				}
				if ( arg ) {
					lineno = atoi( arg );
					if ( lineno == 0 ) {
						/*
						 * We will allow wrap-around to zero (0)
						 * but cannot start at zero.  Otherwise,
						 * we are not consistent with prblock(1).
						 */
						lineno = 1;
					} else if ( lineno > LNOMAX ) {
						lineno %= LNOMAX+1;
						enumformat = enumformatwrap;
					}
					if ( *arg == '0' ) {
						enumformat = enumformatwrap;
					}
				}
				else	/* default for -e */
					lineno = 1;
				lineno |= ~ LNOMSK;	/* in case of wrap-around to zero */
				break;
			case 'F':
				arg = NULL;
				if ( option[1] ) {
					if ( isdigit( option[1] ) ) {
						arg = &option[1];
						option = "\0";
					}
				}
				else if( argc > 0 && isdigit( *argv[1] )){
					arg = *++argv;
					argc--;
				}
				if ( arg ) {
					user_fldcnt = atoi( arg );
					if ( user_fldcnt > MAXATT )
						user_fldcnt = MAXATT;
				}
				else {
					prmsg( MSG_ERROR, "no field count given with -F option" );
					usage();
				}
				break;
			case 'f':
				if ( option[1] ) {
					footline = &option[1];
					cnvtbslsh( footline, footline );
					option = "\0";
				}
				else if ( argc > 0 ) {
					argc--;
					footline = *++argv;
					cnvtbslsh( footline, footline );
				}
				else {
					prmsg( MSG_ERROR, "no page footer given with -f option" );
					usage();
				}
				break;
			case 'h':
				if ( option[1] ) {
					headline = &option[1];
					cnvtbslsh( headline, headline );
					option = "\0";
				}
				else if ( argc > 0 ) {
					argc--;
					headline = *++argv;
					cnvtbslsh( headline, headline );
				}
				else {
					prmsg( MSG_ERROR, "no page header given with -h option" );
					usage();
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
				}
				else {
					prmsg( MSG_ERROR, "no input file given with -i option" );
					usage();
				}
				break;
			case 'l':
				if ( option[1] ) {
					getwidths( option + 1, J_LEFT );
					option = "\0";
				}
				else if( argc > 0 ) {
					getwidths( *++argv, J_LEFT );
					argc--;
				}
				else {
					prmsg( MSG_ERROR, "no width given for left-justified field (-l)" );
					usage();
				}
				break;
			case 'L':
				arg = NULL;
				if ( option[1] ) {
					if ( isdigit( option[1] ) ) {
						arg = &option[1];
						option = "\0";
					}
				}
				else if( argc > 0 && isdigit( *argv[1] )){
					arg = *++argv;
					--argc;
				}
				if ( arg ) {
					prflags |= PR_PAGINATE|PR_ONPAGE;
					pglines = atoi( arg );
					if ( pglines < top + bottom + 4 )
						pglines = PGLINES;
				}
				else {
					prmsg( MSG_ERROR, "no line count given with -l option" );
					usage();
				}
				break;
			case 'N':
				prflags &= ~PR_PAGINATE;
				break;
			case 'n':
				fldbrkch = option[1] ? *++option : FLDBRKCH;
				break;
			case 'o':
				if ( option[1] ) {
					arg = &option[1];
					option = "\0";
				}
				else if ( argc > 0 ) {
					arg = *++argv;
					argc--;
				}
				else {
					prmsg( MSG_ERROR, "no margin offset given with -o option" );
					usage();
				}
				if ( isdigit( *arg ) )
					lmargin = atoi( arg );
				else {
					prmsg( MSG_ERROR, "unrecognized margin offset '%s'",
						arg );
					usage();
				}
				break;
			case 'p':
				prflags |= PR_PAGINATE|PR_ONPAGE;
				break;
			case 'R':
				prflags |= PR_UNIQUE;
				break;
			case 'r':
				if ( option[1] ) {
					getwidths( option + 1, J_RIGHT );
					option = "\0";
				}
				else if( argc > 0 ) {
					getwidths( *++argv, J_RIGHT );
					argc--;
				}
				else {
					prmsg( MSG_ERROR, "no width given for right-justified field (-r)" );
					usage();
				}
				break;
			case 's':
			case 'S':
				colsep = option[1] ? *++option : COLSEP;
				break;
			case 't':
				prflags &= ~(PR_HEADER|PR_FOOTER);
				break;
			case 'u':
			case 'U':
				prflags &= ~PR_BOX;
				break;
			case 'w':
			case 'W':
				arg = NULL;
				if ( option[1] ) {
					if ( isdigit( option[1] ) ) {
						arg = &option[1];
						option = "\0";
					}
				}
				else if( argc > 0 && isdigit( *argv[1] )){
					arg = *++argv;
					--argc;
				}
				if ( arg ) {
					char *p;
					pgwidth = atoi( arg );
					if ( pgwidth < 3 )
						pgwidth = PGWIDTH;
					/*
					 * If width is termiated by a '-'
					 * then only print one section or
					 * the number of reqested sections.
					 */
					if ( ( p = strchr(arg,'-') ) != NULL ) {
						++p;
						if ( ( *p ) && ( isdigit( *p ) ) ) {
							onlyNsections = atoi( p );
							if ( onlyNsections < 0 )
								onlyNsections = 1;
						} else {
							onlyNsections = 1;
						}
					}
				}
				else	/* default for -w */
					pgwidth = SUPERWIDE;
				break;
			case '?':
				usage( );
				break;
			default:
				break;	/* unrecognized args ignored */
			}
		}
	}

	if ( fldcnt == 0 )
		usage( );
	if ( user_fldcnt > 0 ) {
		while( fldcnt < user_fldcnt )
			fldlist[fldcnt++].name = "";
	}
	while( widthcnt < fldcnt ) {
		fldlist[widthcnt].width = DFLT_FLDWIDTH;
		fldlist[widthcnt++].flags = J_LEFT;
	}

	if ( (prflags & PR_BOX) == 0 ) {
		colsep = ' ';
		boxsep = ' ';
	}
	if ( lineno )	/* reduce page width for line enumeration */
		pgwidth = pgwidth - NUMLEN;
	/*
	 * Now based on the widths of the fields, decide how may sections
	 * there should be.
	 */
	secptr = seclist;
	secptr->start = 0;
	secptr->width = 1;
	secptr->fldcnt = 0;
	secptr->fldptr = fldlist;
	seccnt = 1;
	for( i = 0; i < fldcnt; i++ ) {
		if ( secptr->fldcnt > 0 &&
				secptr->width + fldlist[i].width + 1 > pgwidth ) {
			/* Make a new section. */
			seccnt++;
			secptr++;
			secptr->start = i;
			secptr->fldcnt = 0;
			secptr->width = 1;
			secptr->fldptr = &fldlist[i];
		}
		secptr->width += fldlist[i].width + 1;
		secptr->fldcnt++;
	}
	if ( onlyNsections ) {
		if ( seccnt > onlyNsections ) {
			i = seccnt;
			seccnt = onlyNsections;
			onlyNsections = i;
		} else {
			onlyNsections = 0;
		}
	}
		
	if ((prflags & (PR_FOOTER|PR_HEADER|PR_PAGINATE|PR_ONPAGE)) != (PR_FOOTER|PR_HEADER)) {
		/*
		 * Check for UNITY_PRT options that are still valid.
		 */
		if (unity_prt != NULL)
		{
			char *p;
			char *q;

			p = unity_prt;

			while ((p != NULL) && ((q = strchr(p, '=')) != NULL))
			{
				i = q++ - p;

				if ((i == 8 ) && (strncmp(p, "in1space", i) == 0)) {
					switch (*q) {
					case 'L':
					case 'l':
						in1space = 'L';
						break;
					case 'R':
					case 'r':
						in1space = 'R';
						break;
					case 'Y':
					case 'y':
						in1space = 'Y';
						break;
					case 'N':
					case 'n':
						in1space = 'N';
						break;
					default:
						in1space = '\0';
						break;
					}
				}
			}
		}
		unity_prt = NULL;
	}

	if ( seccnt > 1 ) {
		prflags |= PR_ONPAGE;
	}

	if (unity_prt != NULL) {
		char *p;
		char *q;
		int value, top_save, head_save, bottom_save, gap_save;
		int msecgap;

		top_save = top;
		head_save = after_pghdr;
		gap_save = gap;
		bottom_save = bottom;

		msecgap = FALSE;

		p = unity_prt;

		while ((p != NULL) && ((q = strchr(p, '=')) != NULL)) {

			i = q++ - p;

			switch (i) {
			case 3 :
				if ((isdigit(*q)) && (strncmp(p, "top", i) == 0)) {
					value = atoi(q);
					if (((value != 0) || (*q == '0')) &&
					    (value <= 8)) {
						top_save = value;
					}
				}
				break;
			case 4 :
				if ((isdigit(*q)) && (strncmp(p, "head", i) == 0)) {
					value = atoi(q);
					if (((value != 0) || (*q == '0')) &&
					    (value <= 4)) {
						/* one NL needed for end of header string */
						head_save = value + 1;
					}
				}
				break;
			case 6 :
				if ((isdigit(*q)) && (strncmp(p, "bottom", i) == 0)) {
					value = atoi(q);
					if (((value != 0) || (*q == '0')) &&
					    (value <= 8)) {
						bottom_save = value;
					}
				}
				break;
			case 7 :
				if ((isdigit(*q)) && (strncmp(p, "msecgap", i) == 0)) {
					value = atoi(q);
					if (((value != 0) || (*q == '0')) &&
					    (value <= 4)) {
						gap_save = value;
						msecgap = TRUE;
					}
				}
				break;
			case 8 :
				if (strncmp(p, "in1space", i) == 0) {
					switch (*q) {
					case 'L':
					case 'l':
						in1space = 'L';
						break;
					case 'R':
					case 'r':
						in1space = 'R';
						break;
					case 'Y':
					case 'y':
						in1space = 'Y';
						break;
					case 'N':
					case 'n':
						in1space = 'N';
						break;
					default:
						in1space = '\0';
						break;
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
		if ((seccnt == 1) || (msecgap == TRUE)) {
			top = top_save;
			after_pghdr = head_save;
			gap = gap_save;
			bottom = bottom_save;
		}
	}

	if ( infile != NULL )
	{
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

	/*
	 * Below is the general printing code, which is basically:
	 *
	 *	For each record
	 *	   If the lines needed for this record go beyond the page
	 *		    boundary:
	 *		Print all subsequent sections for the page.
	 *		Print out any footer.
	 *		Print out any header for next page.
	 *		Reset all counts.
	 *	   else
	 *		Print the first section of this record.
	 *		Save the portions of subsequent sections.
	 */

	currec = rec1;
	oldrecs = NULL;
	if ( prflags & PR_UNIQUE ) {
		prevrec = rec2;
		for( i = 0; i < fldcnt; i++ )
			rec2[i] = NULL;
	}

	if ( lineno )
		prevlineno = lineno;
	staticlines = hdrlines( seclist, seccnt );
	curpage = 1;
	curline = staticlines;
	dblspace = FALSE;
	top_of_page( curpage );
	pr_header( seclist, curpage );
	while( getrec( &frmio, fldcnt, flddelim, currec, TRUE ) ) {
		if ( prflags & PR_UNIQUE ) {
			if ( mark_duplicate( prevrec, currec ) == fldcnt )
				continue;  /* all duplicates - don't print */
		}

		total = 0;
		for( i = 0; i < seccnt; i++ )
			total += needlines( &currec[seclist[i].start],
					&seclist[i], dblspace );

		if ( (prflags & PR_ONPAGE) && curline > staticlines &&
			total + curline > pglines )
		{
			for( i = 1; i < seccnt; i++ ) {
				if ( lineno )
					lineno = prevlineno;
				do_gap( &seclist[i - 1] );
				pr_header( &seclist[i], curpage );
				pr_section( &seclist[i], oldrecs );
			}
			if ( dblspace ) /* no dblspace on first record */
				total -= seccnt;
			dblspace = FALSE;
			if ( lineno )
				prevlineno = lineno;

			/* Check if the record is bigger than a page. */
			while( curline > pglines ) {
				curline -= pglines;
				curpage++;
			}
			pr_footer( curpage, curline, &seclist[seccnt-1] );
			curline = staticlines;
			curpage++;
			top_of_page( curpage );
			pr_header( seclist, curpage );
			if ( prflags & PR_UNIQUE ) {
				/* new page - print all fields */
				for( i = 0; i < fldcnt; i++ )
					fldlist[i].flags &= ~F_DUPLICATE;
			}

			free_oldrecs( &frmio, oldrecs, seclist[0].fldcnt,
				fldcnt - seclist[0].fldcnt );
		}

		pr_record( currec, seclist, dblspace );
		dblspace = (prflags & PR_DBLSPACE);
		curline += total;

		if ( seccnt > 1 )
			save_record( &oldrecs, currec, seclist[0].fldcnt,
				fldcnt - seclist[0].fldcnt );

		if ( prflags & PR_UNIQUE )
		{
			register char **tmprec;

			/* save prev rec by switching bufs */
			if ( prevrec[0] )
				unusedflds(&frmio, prevrec,
					seclist[0].fldcnt );
			tmprec = currec;
			currec = prevrec;
			prevrec = tmprec;
		}
		else
			unusedflds( &frmio, currec, seclist[0].fldcnt );
	}

	for( i = 1; i < seccnt; i++ ) {
		if ( lineno )
			lineno = prevlineno;
		do_gap( &seclist[i - 1] );
		pr_header( &seclist[i], curpage );
		pr_section( &seclist[i], oldrecs );
	}
	pr_footer( curpage, curline, &seclist[seccnt - 1] );

	exit( 0 );
}

usage( )
{
	prmsg( MSG_USAGE, "[-beNpRtU] [-D<dbl_sp_char>] [-d<fld_delim>] \\" );
	prmsg( MSG_CONTINUE, "[-F<col_cnt>] [-f <pg_footer>] [-h <pg_header>] [-L<pg_length>] \\" );
	prmsg( MSG_CONTINUE, "[-n<newline_char>] [-o<margin_offset>] [-s<col_sep_char>] \\" );
	prmsg( MSG_CONTINUE, "[-W<page_width>[-<sections>]] [-c|l|r <col_widths>,...]... \\" );
	prmsg( MSG_CONTINUE, "-C <col_heading> | <col_names>..." );

	exit( 1 );
}


mark_duplicate( prevrec, currec )
char **prevrec;
char **currec;
{
	int i, j;

	i = 0;
	if ( prevrec != NULL && currec != NULL ) {
		for( ; i < fldcnt; i++ ) {

			/* check first, since both == NULL is dup */
			if ( prevrec[i] == currec[i] )
				continue;	/* a duplicate */

			if ( prevrec[i] == NULL )
				break;	/* not a duplicate */

			if ( strcmp( currec[i], prevrec[i] ) != 0)
				break;	/* not a duplicate */

			fldlist[i].flags |= F_DUPLICATE;
		}
	}

	for( j = i; j < fldcnt; j++ )
		fldlist[j].flags &= ~F_DUPLICATE;

	return( i );
}

getwidths( str, justify )
register char *str;
int justify;
{
	register int width;

	do {
		if ( ! isdigit( *str ) ) {
			prmsg( MSG_ERROR, "bad field width '%s'", str );
			usage( );
		}
		width = atoi( str );
		if ( width <= 0 )
			width = DFLT_FLDWIDTH;
		fldlist[widthcnt].width = width;
		fldlist[widthcnt++].flags = justify;
		str = strchr( str, ',' );
	} while( str++ != NULL );
}

save_record( blklist, currec, start, fldcnt )
struct oldrecblock **blklist;
register char **currec;
int start;
register int fldcnt;
{
	register struct oldrecblock *blkptr;
	register char **newrec;

	if ( *blklist == NULL )
		*blklist = (struct oldrecblock *)calloc( 1, sizeof( struct oldrecblock ) );

	for( blkptr = *blklist; blkptr->blkcnt >= OLDRECBLKSIZE;
			blkptr = blkptr->next ) {
		if ( blkptr->next == NULL )
			blkptr->next = (struct oldrecblock *)calloc( 1, sizeof( struct oldrecblock ) );
	}

	newrec = &blkptr->fldvals[blkptr->blkcnt++][start];
	currec += start;
	while( fldcnt-- )
		*newrec++ = *currec++;
}

free_oldrecs( ioptr, blkptr, start, fldcnt )
struct formatio *ioptr;
register struct oldrecblock *blkptr;
register int start;
int fldcnt;
{
	register int i;

	for( ; blkptr; blkptr = blkptr->next ) {
		for( i = 0; i < blkptr->blkcnt; i++ )
			unusedflds( ioptr, &blkptr->fldvals[i][start], fldcnt );
		blkptr->blkcnt = 0;
	}
}

pr_section( secptr, blkptr )
register struct section *secptr;
register struct oldrecblock *blkptr;
{
	register int i, dblspace;
	char **prevrec;

	dblspace = FALSE;
	prevrec = NULL;
	for( ; blkptr; blkptr = blkptr->next ) {
		for( i = 0; i < blkptr->blkcnt; i++ ) {
			if ( prflags & PR_UNIQUE )
				(void)mark_duplicate( prevrec,
						blkptr->fldvals[i] );

			pr_record( &blkptr->fldvals[i][secptr->start],
				secptr, dblspace );

			prevrec = blkptr->fldvals[i];

			dblspace = (prflags & PR_DBLSPACE);
		}
	}
}

static char *
findbrk( value, width )
char *value;
int width;
{
	register char *end;
	register int length, tmplen;

	length = strlen( value );
	tmplen = length <= width ? length : width;

	/*
	 * Look for embedded newline character.
	 */
	for( end = value; end < value + tmplen; end++ ) {
		if ( (fldbrkch && *end == fldbrkch) ||
				*end == '\n' || *end == '\r' )
			return( end );
	}

	if ( length > width ) {
		/*
		 * Look for a good point to break the field
		 */
		if ( (prflags & PR_ALPHABRK)  &&  value[width] != ' ' ) {
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
fld_print( value, width, flags )
char *value;		/* beginning of field to print */
int width;		/* maximum field width */
int flags;		/* positioning character */
{
	/*
	 * Print width characters from the string "value".  If the
	 * end of "value" is reached before printing width characters,
	 * pad the field with blanks.  If "value" cannot all be printed
	 * a pointer to the next character is returned.  In this case,
	 * the field is broken at the last occurance of the fldbrkch
	 * character or a non-alphanumeric character if specified.
	 */

	register char *end;
	register int length;

	if ( (flags & F_DUPLICATE) || value == NULL || *value == '\0' ) {
		/*
		 * Field is blank.
		 */
		while ( width-- )
			putchar( ' ' );
		return( NULL );
	}

	end = findbrk( value, width );

	/*
	 * Now handle the justification
	 */
	length = width - (end - value); /* number of spaces to be printed */
	switch ( flags & JUSTIFYMASK ) {
	case J_RIGHT:
		if ( ( length >= 1 ) && ( width >= 3 ) &&
		     ( ( in1space == 'R' ) || ( in1space == 'Y' ) ) ) {
			length -= 1;
			while ( length-- )
				putchar( ' ' );
			length = 1;
		} else {
			while ( length-- )
				putchar( ' ' );
		}
		break;
	case J_CENTER:
		for ( length = (length + 1) / 2; length > 0; length-- )
		{
			width--;
			putchar( ' ' );
		}
		length = width - (end - value);
		break;
	default:
		if ( ( length >= 1 ) && ( width >= 3 ) &&
		     ( ( in1space == 'L' ) || ( in1space == 'Y' ) ) ) {
			putchar( ' ' );
			length -= 1;
		}
	}

	/*
	 * Print the field.  Any tabs will screw up the alignment,
	 * so we convert them to spaces.
	 */
	while ( value != end ) {
		if ( *value != '\t' )
			putchar( *value );
		else
			putchar( ' ' );
		value++;
	}

	/*
	 * . . . and pad to the end with spaces.
	 */
	while ( length-- > 0 )
		putchar( ' ' );

	/*
	 * Bypass any trailing spaces, too.
	 */
	for ( ; *end == ' '; end++ )
		;

	/*
	 * Return next pointer to next character or, if none, NULL.
	 */
	if ( *end == '\n' || *end == '\r' || ( fldbrkch && *end == fldbrkch ) )
		return( end[1] ? &end[1] : NULL );
	else
		return( *end ? end : NULL );
}

static int
fldlines( value, width, flags )
register char *value;	/* beginning of field to print */
int width;		/* maximum field width */
int flags;
{
	register int linecnt;

	/*
	 * fldlines is a modified version of fld_print which just returns
	 * the number of lines need to print the field value.
	 */
	if ( (flags & F_DUPLICATE) || value == NULL || *value == '\0' )
		return( 1 );

	linecnt = 0;
	while( value && *value ) {
		linecnt++;
		value = findbrk( value, width );

		if ( *value == '\n' || *value == '\r' ||
				(fldbrkch && *value == fldbrkch) )
			value++;
		else
			for( ; *value == ' '; value++ )
				;
	}

	return( linecnt );
}

needlines( currec, secptr, dblspace )
char **currec;
struct section *secptr;
int dblspace;
{
	int i, max, linecnt;

	max = 0;
	for( i = 0; i < secptr->fldcnt; i++ ) {
		linecnt = fldlines( currec[i], secptr->fldptr[i].width,
				secptr->fldptr[i].flags );
		if ( linecnt > max )
			max = linecnt;
	}

	return( dblspace ? max + 1 : max );
}

pr_header( secptr, curpage )
struct section *secptr;
{
	int more_to_do, i;
	char *names[MAXATT];

	if ( fldline ) {
		printf( fldline, curpage );
		putchar( '\n' );
		separator( secptr->width );
		return;
	}
	else if ( (prflags & PR_HEADER) == 0 )
		return;

	for( i = 0; i < secptr->fldcnt; i++ )
		names[i] = secptr->fldptr[i].name;
	separator( secptr->width );

	/*
	 * "more_to_do" is a boolean indicating that some field
	 * has more characters to print.
	 */
	do {
		more_to_do = FALSE;
		margin( );
		enumerate( FALSE );
		for( i = 0; i < secptr->fldcnt; i++ ) {
			putchar( colsep );
			names[i] = fld_print( names[i],
					secptr->fldptr[i].width, J_CENTER );
			if ( names[i] )
				more_to_do = TRUE;
		}
		putchar( colsep );
		putchar( '\n' );
	} while( more_to_do );

	separator( secptr->width );
}

top_of_page( curpage )
int curpage;
{
	int i;

	if ( prflags & PR_HEADER ) {
		for( i = 0; i < top; i++ )
			putchar( '\n' );
		if ( headline ) {
			margin( );
			printf( headline, curpage );
			for( i = 0; i < after_pghdr; i++ )
				putchar( '\n' );
		}
	}
}

pr_footer( curpage, curline, secptr )
int curpage;
int curline;
struct section *secptr;
{
	int i;

	if ( prflags & PR_FOOTER )
		separator( secptr->width ); /* finish current section */
	if ( prflags & PR_PAGINATE ) {	/* move to bottom of page */
		while( curline++ < pglines )
		{
			putchar( '\n' );
		}
	}
	/* put out bottom page margin */
	if ( footline ) {
		margin( );
		printf( footline, curpage );
	}
	if ( prflags & (PR_PAGINATE|PR_FOOTER) ) {
		for( i = 0; i < bottom; i++ )
			putchar( '\n' );
	}
}

hdrlines( secptr, seccnt )
struct section *secptr;
int seccnt;
{
	int i, j, linecnt, max, cnt;

	/*
	 * hdrlines() is a modified version of pr_header(), do_gap(),
	 * pr_footer(), and top_of_page() which computes the number
	 * of lines needed for all header info on a page.
	 */
	if ( (prflags & PR_ONPAGE) == 0 )
		return( 0 );
	linecnt = top + bottom;	/* lines at top and bottom of page */
	linecnt += gap * (seccnt - 1);	/* gaps between each section */
	if ( prflags & PR_FOOTER )
		linecnt += seccnt; /* separator line after each section */
	if ( fldline )		/* assume two lines for field names */
		return( linecnt + 2 * seccnt );
	else if ( (prflags & PR_HEADER) == 0 )
		return( linecnt );	/* no headers for sections */

	if ( headline )			/* extra lines for page header */
		linecnt += after_pghdr;

	linecnt += 2 * seccnt;		/* separator lines in headers */
	for( i = 0; i < seccnt; i++, secptr++ ) {
		max = 0;
		for( j = 0; j < secptr->fldcnt; j++ ) {
			cnt = fldlines( secptr->fldptr[j].name,
					secptr->fldptr[j].width, J_CENTER );
			if ( cnt > max )
				max = cnt;
		}
		linecnt += max;		/* max lines needed for section */
	}

	return( linecnt );
}

do_gap( secptr )
struct section *secptr;
{
	int i;

	/*
	 * Print gap between sections and reset everything for the next
	 * section.
	 */
	if ( prflags & PR_FOOTER )
		separator( secptr->width );
	for( i = 0; i < gap; i++ )
		putchar( '\n' );
}

margin( )
{
	int i;

	for( i = 0; i < lmargin; i++ )
		putchar( ' ' );
}

enumerate( newnumber )
int newnumber;
{
	if ( lineno ) {
		if ( newnumber ) {
			lineno &= LNOMSK;
			if ( lineno == 0 ) {
				enumformat = enumformatwrap;
			}
			printf( enumformat, lineno++);
			lineno %= LNOMAX+1;
			lineno |= ~ LNOMSK;
		}
		else
			fputs( "     ", stdout );
	}
}

pr_record( currec, secptr, dblspace )
char **currec;
struct section *secptr;
int dblspace;
{
	register int i, j;
	register int more_to_do;
	char *value[MAXATT];

	for( i = 0; i < secptr->fldcnt; i++ )
		value[i] = currec[i];

	if ( dblspace ) {	/* put out double space line */
		margin( );
		enumerate( FALSE );
		putchar( colsep );
		for( j = 0; j < secptr->fldcnt; j++ )
		{
			register char ch;

			ch = (secptr->fldptr[j].flags & F_DUPLICATE) ? ' ' : rowsep;
		
			for( i = secptr->fldptr[j].width; i > 0; i-- )
				putchar( ch );
			putchar( colsep );
		}
		putchar( '\n' );
	}

	more_to_do = FALSE;
	do {
		/* print the next line of output */
		margin( );
		enumerate( more_to_do == FALSE ); /* first time through */

		more_to_do = FALSE;
		for( i = 0; i < secptr->fldcnt; i++ ) {
			putchar( colsep );
			if ( (value[i] = fld_print( value[i],
					secptr->fldptr[i].width,
					secptr->fldptr[i].flags )) != NULL )
				more_to_do = TRUE; /* more data */
		}

		putchar( colsep );
		putchar( '\n' );
	} while( more_to_do );
}

separator( w )
int w;
{
	int i;

	/*
	 * Print separator line
	 */
	margin( );
	enumerate( FALSE );

	putchar( colsep );
	for ( i = 2; i < w; i++ )	/* deduct two for column char's */
		putchar( boxsep  );
	putchar( colsep );
	putchar( '\n' );
}
