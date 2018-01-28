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
#include "format.h"
#include "message.h"

extern char *basename();
extern char *malloc();

extern char *cnvtbschar();

enum FMT_TYPE {
	FMT_FLDVALUE,
	FMT_FLDNAME,
	FMT_RECNUM,
	FMT_LITERAL,
};

struct fldformat {
	char *format;
	char **str;
	enum FMT_TYPE fmttype;
};

char *prog;
long recnum;	/* record number */

main( argc, argv )
int argc;
char *argv[];
{
	char flddelim;
	char nlchar;
	char do_middle;
	char repeat;
	char rmvblanks;		/* remove trailing blanks */
	register int i, formcnt;
	int fldcnt;
	int namecnt;
	char *header = "";
	char *footer = "";
	char *middle = "";
	char *format;
	char *infile = NULL;
	struct formatio frmio;
	char *strptrs[MAXATT];
	struct fldformat formlist[2 * MAXATT];
	char *currec[MAXATT];
	char *fldnames[MAXATT];

	flddelim = FLDDELIM;
	nlchar = '\0';
	repeat = FALSE;
	rmvblanks = FALSE;
	fldcnt = 0;
	namecnt = 0;
	format = NULL;

	prog = basename( *argv++ );

	for( --argc; argc > 0; argc--, argv++ ) {
		register char *option;
		char *arg;

		if ( **argv != '-' ) {
			if ( format == NULL )
				format = *argv;
			else if ( namecnt < MAXATT )
				fldnames[ namecnt++ ] = *argv;
			continue;
		}
		for( option = &argv[0][1]; option && *option; option++ ) {
			switch( *option ) {
			case 'b':
				rmvblanks = TRUE;
				break;
			case 'd':
				if ( option[1] )
					flddelim = *++option;
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
					fldcnt = atoi( arg );
					if ( fldcnt > MAXATT )
						fldcnt = MAXATT;
				}
				else {
					prmsg( MSG_ERROR, "no field count given with -F option" );
					usage();
				}
				break;
			case 'f':
				if ( option[1] ) {
					footer = &option[1];
					option = "\0";
				}
				else if ( argc > 0 ) {
					argc--;
					footer = *++argv;
				}
				else {
					prmsg( MSG_ERROR, "no footer given with -f option" );
					usage();
				}
				break;
			case 'h':
				if ( option[1] ) {
					header = &option[1];
					option = "\0";
				}
				else if ( argc > 0 ) {
					argc--;
					header = *++argv;
				}
				else {
					prmsg( MSG_ERROR, "no header given with -h option" );
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
			case 'm':
				if ( option[1] ) {
					middle = &option[1];
					option = "\0";
				}
				else if ( argc > 0 ) {
					argc--;
					middle = *++argv;
				}
				else {
					prmsg( MSG_ERROR, "no divider string given with -m option" );
					usage();
				}
				break;
			case 'n':
				if ( option[1] )
					nlchar = *++option;
				break;
			case 'R':
				repeat = TRUE;
				break;
			}
		}
	}

	while( namecnt < fldcnt && namecnt < MAXATT )
		fldnames[ namecnt++ ] = "";

	if ( fldcnt == 0 || format == NULL )
		repeat = FALSE;		/* can't repeat non-existent format */

	formcnt = chkformat( format, strptrs, currec, &fldcnt, formlist,
			repeat, fldnames, namecnt );
	if ( formcnt < 0 )
		exit( 1 );	/* bad format */

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

	cnvtbslsh( header, header );
	cnvtbslsh( footer, footer );
	cnvtbslsh( middle, middle );

	fputs( header, stdout );
	do_middle = FALSE;
	recnum = 0;
	while( getrec( &frmio, fldcnt, flddelim, currec, FALSE ) )
	{
		register struct fldformat *frmptr;

		recnum++;
		if ( rmvblanks )
		{
			register char *ptr;
			register char *str;
			register int i;

			for( i = 0; i < fldcnt; i++ )
			{
				str = currec[i];
				ptr = str + strlen( str ) - 1;
				while( ptr >= str && *ptr == ' ' )
					*ptr-- = '\0';
			}
		}

		if ( do_middle )
			fputs( middle, stdout );
		if ( nlchar )
			conv_newline( currec, fldcnt, nlchar );
		for( i = 0, frmptr = formlist; i < formcnt; i++, frmptr++ )
			printf( frmptr->format, *frmptr->str );
		do_middle = TRUE;
	}
	fputs( footer, stdout );

	exit( 0 );
}

usage( )
{
	prmsg( MSG_USAGE, "[-bR] [-d<delimiter>] [-F<field_cnt>] [-f<footer>] \\" );
	prmsg( MSG_CONTINUE, "[-h<header>] [-m<middle-string>] [-n<newline_char>] \\" );
	prmsg( MSG_CONTINUE, "[<format_string>]" );
	exit( 1 );
}

getformat( buf, fptr, fmt_type )
char *buf;
register char *fptr;
enum FMT_TYPE *fmt_type;
{
	register char *bptr;
	register int len, first, period;

	len = 0;
	bptr = buf;
	while( *fptr && ! isalpha( *fptr ) ) {
		len++;
		*bptr++ = *fptr++;
	}

	if ( *fptr == '\0' )
		return( -1 );	/* no conversion character */
	else {
		switch( *fptr ) {
		case 'd':
		case 'o':
		case 'u':
		case 'x':
		case 'X':
			*fmt_type = FMT_RECNUM;
			/* force everything to a long value */
			*bptr++ = 'l';
			*bptr++ = *fptr;
			*bptr = '\0';
			++len;
			break;
		case 's':
			*fmt_type = FMT_FLDVALUE;
			*bptr++ = *fptr;
			*bptr = '\0';
			++len;
			break;
		case 'n':
			*fmt_type = FMT_FLDNAME;
			*bptr++ = 's';
			*bptr = '\0';
			++len;
			break;
		default:
			/* still need to save the conversion in buf */
			*bptr++ = *fptr;
			*bptr = '\0';
			prmsg( MSG_ERROR, "unsupported conversion character '%c'", *fptr);
			return( -1 );
		}
	}

	first = TRUE;
	period = FALSE;
	for( bptr = buf; *bptr; bptr++ ) {
		switch( *bptr ) {
		case '-':
			if ( ! first ) {
				prmsg( MSG_ERROR, "embedded '-' in format" );
				return( -1 );
			}
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 's':
		case 'l':
		case 'd':
		case 'o':
		case 'u':
		case 'x':
		case 'X':
			break;
		case '.':
			if( period ) {
				prmsg( MSG_ERROR, "multiple periods in format" );
				return( -1 );
			}
			period = TRUE;
			break;
		default:
			prmsg( MSG_ERROR, "unrecognized character '%c' in format",
				*bptr );
			return( -1 );
		}
		first = FALSE;
	}

	return( len );
}

chkformat( format, staticstrs, dynamicstrs, dynstrcnt, formlist,
	repeat, fldnames, namecnt )
char *format;
char **staticstrs;
char **dynamicstrs;
int *dynstrcnt;
struct fldformat *formlist;
char repeat;
char **fldnames;
int namecnt;
{
	register struct fldformat *frmptr;
	register char *bptr, *fptr;
	register int seen_literal;
	register int scnt;	/* staticstrs index */
	register int dcnt;	/* dynamicstrs index */
	register int ncnt;	/* field name count */
	register int i;

	if ( format == NULL || *format == '\0' ) {
		if ( *dynstrcnt == 0 ) {
			prmsg( MSG_ERROR, "either a format string or a field count must be given on the command line" );
			usage( );
		}
		format = "";
	}

	i = strlen( format );
	bptr = malloc( 2 * i );

	fptr = format;
	seen_literal = FALSE;
	staticstrs[0] = bptr;
	ncnt = 0;
	scnt = 0;
	dcnt = 0;
	frmptr = formlist;
	while( *fptr )
	{
		switch( *fptr ) {
		case '\\':
			fptr = cnvtbschar( fptr + 1, bptr );
			bptr++;
			seen_literal = TRUE;
			break;
		case '%':
			++fptr;
			if ( *fptr == '\0' )
			{
				seen_literal = TRUE;
				*bptr++ = '%';
			}
			else if ( *fptr == '%' )
			{
				seen_literal = TRUE;
				*bptr++ = *fptr++;
			}
			else
			{
				/*
				 * We have the start of a format in the
				 * string.  We store away any literal that
				 * there is and then get the format string.
				 */
				if ( seen_literal )
				{
					*bptr++ = '\0';
					frmptr->format = "%s";
					frmptr->str = &staticstrs[scnt++];
					frmptr->fmttype = FMT_LITERAL;
					frmptr++;
					seen_literal = FALSE;
				}
				frmptr->format = bptr;
				*bptr++ = '%';
				i = getformat( bptr, fptr, &frmptr->fmttype );
				if ( i < 0 )
				{
					prmsg( MSG_ERROR, "unrecognized conversion format '%s'", bptr );
					return( -1 );
				}
				switch( frmptr->fmttype ) {
				case FMT_RECNUM:
					frmptr->str = (char **)&recnum;

					/* add in the 'l' appended to format */
					bptr++;
					break;
				case FMT_FLDVALUE:
					if ( *dynstrcnt > 0 && dcnt >= *dynstrcnt ) {
						prmsg( MSG_ERROR, "number of string substitutions is greater than number of fields (%d)",
							*dynstrcnt );
						return( -1 );
					}
					frmptr->str = &dynamicstrs[dcnt++];
					break;
				case FMT_FLDNAME:
					frmptr->str = &fldnames[ ncnt++ ];
					break;
				}
				frmptr++;
				bptr += i + 1;
				fptr += i;
				staticstrs[scnt] = bptr; /* set next literal */
			}
			break;
		default:
			*bptr++ = *fptr++;
			seen_literal = TRUE;
			break;
		}
	}

	/*
	 * Finish off any literal string that remains.
	 */
	if ( seen_literal )
	{
		*bptr++ = '\0';
		frmptr->format = "%s";
		frmptr->str = &staticstrs[scnt];
		frmptr->fmttype = FMT_LITERAL;
		frmptr++;
	}

	if ( *dynstrcnt <= 0 )
		*dynstrcnt = dcnt;
	else if ( repeat && frmptr != formlist )
	{
		int origfcnt, num;

		origfcnt = frmptr - formlist;
		while( dcnt < *dynstrcnt )
		{
			num = (frmptr - formlist) % origfcnt;
			frmptr->format = formlist[ num ].format;
			switch( formlist[ num ].fmttype ) {
			case FMT_LITERAL:
				frmptr->str = formlist[num].str;
				break;
			case FMT_FLDVALUE:
				frmptr->str = &dynamicstrs[dcnt++];
				break;
			case FMT_RECNUM:
				frmptr->str = (char**)&recnum;
				break;
			case FMT_FLDNAME:
				frmptr->str = &fldnames[ncnt++];
				break;
			}
			frmptr++;
		}
		/*
		 * Finish off any literal strings that remain.
		 */
		num = (frmptr - formlist) % origfcnt;
		while( num != 0 && num < origfcnt )
		{
			if ( formlist[ num ].fmttype == FMT_LITERAL )
			{
				frmptr->format = formlist[ num ].format;
				frmptr->str = formlist[ num ].str;
				frmptr->fmttype = FMT_LITERAL;
				frmptr++;
			}
			++num;
		}
	}
	else
	{
		while( dcnt < *dynstrcnt )
		{
			frmptr->format = "%s\n";
			frmptr->str = &dynamicstrs[dcnt++];
			frmptr->fmttype = FMT_FLDVALUE;
			frmptr++;
		}
	}

	if ( ncnt > namecnt )
	{
		prmsg( MSG_ERROR, "more field name constructs given in format string (%d) than field names on command line (%d)",
			ncnt, namecnt );
		return( -1 );
	}

	return( frmptr - formlist );
}

conv_newline( fldlist, fldcnt, nlchar )
register char **fldlist;
register int fldcnt;
register char nlchar;
{
	register int i;
	register char *str;

	for( i = 0; i < fldcnt; i++ ) {
		for( str = fldlist[i]; *str; str++ ) {
			if ( *str == nlchar )
				*str = '\n';
		}
	}
}
