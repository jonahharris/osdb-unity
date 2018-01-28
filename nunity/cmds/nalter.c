/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include "message.h"
#include "permission.h"
#include "uquery.h"


/*
 * WARNING NOTE: Any new modifiers that are unique to nalter(1)
 *		 must be added to the switch statement after the
 *		 (second) call to getaltermod() in main() so that
 *		 the modifier does not get passed on to fmkquery()
 *		 in one of the projected attribute names. You can
 *		 get this location by searching on the matching
 *		 "WARNING NOTE" string in this file.
 */
typedef enum altmodifier {
	ALT_M_NULL = 0,
	ALT_M_NUMBER,
	ALT_M_BINARY,
	ALT_M_OCTAL,
	ALT_M_HEX,
	ALT_M_LENGTH,
	ALT_M_STRING,
	ALT_M_TOLOWER,
	ALT_M_TOUPPER,
	ALT_M_TRIM,
	ALT_M_LTRIM,
	ALT_M_RTRIM
} ALTMODIFIER;

struct altermod {
	char *modifier;
	enum altmodifier mod;
	short len;
};

struct altermod modlist[] = {
	"numeric",	ALT_M_NUMBER,		2,
	"binary",	ALT_M_BINARY,		2,
	"octal",	ALT_M_OCTAL,		1,
	"hex",		ALT_M_HEX,		1,
	"length",	ALT_M_LENGTH,		3,
	"string",	ALT_M_STRING,		3,
	"tolower",	ALT_M_TOLOWER,		3,
	"toupper",	ALT_M_TOUPPER,		3,
	"trim",		ALT_M_TRIM,		2,
	"ltrim",	ALT_M_LTRIM,		3,
	"rtrim",	ALT_M_RTRIM,		3,
	NULL,		ALT_M_NULL,		0,
};

typedef enum altopcode {
	ALT_OP_NULL = 0,
	ALT_S_ASSIGN,
	ALT_N_ASSIGN,
	ALT_N_INCR,
	ALT_N_DECR,
	ALT_N_MULT,
	ALT_N_DIVIDE,
	ALT_N_MODULO,
	ALT_A_ASSIGN,
	ALT_A_NUMBER,
	ALT_A_INCR,
	ALT_A_DECR,
	ALT_A_MULT,
	ALT_A_DIVIDE,
	ALT_A_MODULO,
	ALT_A_LENGTH
} ALTOPCODE;

struct alterop {
	char *oper;
	enum altopcode op;
};

struct alterop oplist[] = {
	"to",	ALT_S_ASSIGN,
	"=",	ALT_S_ASSIGN,
	"+=",	ALT_N_INCR,
	"-=",	ALT_N_DECR,
	"*=",	ALT_N_MULT,
	"/=",	ALT_N_DIVIDE,
	"%=",	ALT_N_MODULO,

	"fto",	ALT_A_ASSIGN,
	"f=",	ALT_A_ASSIGN,
	"f+=",	ALT_A_INCR,
	"f-=",	ALT_A_DECR,
	"f*=",	ALT_A_MULT,
	"f/=",	ALT_A_DIVIDE,
	"f%=",	ALT_A_MODULO,
	NULL,	ALT_OP_NULL,
};

union alterval {
	char *strval;
	double numval;
	short attr;
};

struct alterinfo {
	short op;		/* op saying how to update attr */
	short attr;		/* attribute being modified */
	unsigned short width;	/* attribute width for print output */
	char delim;		/* attribute delimiter for print output */
	unsigned char attrtype;	/* attr termination type for print output */
	char  dmodifier;	/* destination attribute modifier */
	char  smodifier;	/* source attribute modifier */
	char  conversion;	/* %format conversion character */
	char  *format;		/* pointer to %format string */
	union alterval val;	/* new value for attribute */
};

static char *format_s_default = "%s";
static char *format_g_default = "%.11g";

#ifndef	__STDC__
extern long strtol();
extern char *strchr(), *strrchr();
#endif

extern unsigned short getegid(), geteuid();
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
extern char *calloc(), *malloc();
#endif
extern double atof();
extern char *basename();
extern char *getenv();
extern struct uquery *fmkquery();
extern struct urelation *getrelinfo();
extern void setunpackenv();
extern char *strcpylower();
extern char *strcpyupper();

RETSIGTYPE catchsig();
int alterattrs();
long not_altered();

extern char uversion[];			/* version string */

extern int  uchecktuple;	/* if set readtuple() will count/report tuple errors */
extern long utplerrors; 	/* no. of tuples read with tuple errors */
extern long utplelimit; 	/* limit on number of tuple errors reported */
extern int  utplmsgtype;	/* MSG_ERROR or MSG_WARN type tuple error message */

char *prog;
char seen_error;	/* seen error while processing query? */
char all_static;	/* all alterations static values? */
char quiet, printold, printnew, printrecnum, noop, prerecorder;
struct uperuseinfo *perptr;
long modifycnt;
long utplerrors_orig;	/* original value of utplerrors */
short altercnt;
char flddelim = '\0';
char flddelim_set = FALSE;
struct alterinfo alterlist[MAXATT];

enum altopcode
getalterop( str )
char *str;
{
	register struct alterop *altptr;

	for( altptr = oplist; altptr->oper; altptr++ ) {
		if ( strcmp( altptr->oper, str ) == 0 )
			return( altptr->op );
	}

	return( ALT_OP_NULL );
}


enum altmodifier
getaltermod( str )
char *str;
{
	register struct altermod *altptr;

	for ( altptr = modlist; altptr->modifier; altptr++ ) {
		if ( strncmp( altptr->modifier, str, altptr->len ) == 0 )
			return( altptr->mod );
	}

	return( ALT_M_NULL );
}

void
prtmodlistmsg()
{
	register struct altermod *altptr;
	register char *p, *q;
	int	i;
	char	msgbuf[1024];

	for ( i = 1, p = msgbuf, altptr = modlist; altptr->modifier; altptr++, i++ ) {
		if ( i > 6 ) {
			i = 1;
			*p++ = '\n';
			*p++ = '\t';
			*p++ = '\t';
		}
		q = altptr->modifier;
		while ( *q ) {
			*p++ = *q++;
		}
		*p++ = ' ';
	}
	*p = '\0';

	prmsg( MSG_CONTINUE, "Modifier must be one of the following:\n\t\t%s", msgbuf );
}


main( argc, argv )
int argc;
char *argv[];
{
	extern int uerror;
	char **attrnames;
	char **relnames;
	char **wherelist;
	short anamecnt, relnamecnt, wherecnt;
	register long i;
	struct urelation *modrel;
	short maxattr;
	unsigned short uid, gid;
	int queryflags = 0;
	struct uquery *query;
	short projcnt;
	char *projlist[MAXATT];
	char *unity_alter;

	prog = basename( *argv++ );

	unity_alter = getenv("UNITY_ALTER");
	/* check environment options before command line options */
	if (unity_alter != NULL)
	{
		char *p;
		char *q;

		p = unity_alter;

		while ((p != NULL) && ((q = strchr(p, '=')) != NULL)) {

			i = q++ - p;

			switch (i) {
			case 7 :
				if (strncmp(p, "prerec#", i) == 0) {
					if (*q == 'y') {
						prerecorder = TRUE;
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
	}

	for( --argc; argc > 0; argc--, argv++ )
	{
		register char *option;

		option = *argv;
		if ( *option != '-' )
		{
#ifdef DEBUG
			if ( strncmp( option, "debug=", 6 ) == 0 )
			{
				(void)set_qdebug( &option[6] );
				continue;
			}
#endif /* DEBUG */
			break;
		}

		while( *++option )
		{
			switch( *option ) {
			case 'a':
				printnew = TRUE;
				break;
			case 'i':
				queryflags ^= Q_NOCASECMP;
				break;
			case 'n':
				noop = TRUE;
				break;
			case 'o':
				printold = TRUE;
				break;
			case 'p':
				prerecorder = TRUE;
				break;
			case 'Q':		/* don't quit due to tuple errors */
				/*
				 * default to count tuple errors
				 * and make a note that we saw -Q
				 */
				utplelimit = 0;
				uchecktuple = UCHECKCOUNT;
				if ( option[1] ) {
					if ((( option[1] == '-' ) &&
					     ( isdigit( option[2] ))) ||
					    ( isdigit( option[1] ))) {

						uchecktuple = UCHECKPRINT;
						utplelimit = atoi( &option[1] );
						option = "Q";
					}
				}
				else if ( argc > 1 ) {
					char *p = argv[1];
					if ((( p[0] == '-' ) &&
					     ( isdigit( p[1] ))) ||
					    ( isdigit( p[0] ))) {
						uchecktuple = UCHECKPRINT;
						utplelimit = atoi( p );
						option = "Q";
						--argc;
						++argv;
					}
				}
				break;
			case 'q':
				quiet = TRUE;
				break;
			case 'r':
				printrecnum = TRUE;
				break;
			case 'V':
				prmsg( MSG_NOTE, "%s", uversion );
				exit( 0 );
			case 'd':
#ifdef DEBUG
				if ( strcmp( option, "debug" ) == 0 )
				{
					(void)set_qdebug( "all" );
					option = "\0\0";
					break;
				}
#endif
				if ( strncmp( option, "delim", 5 ) == 0 ) {
					if ( option[5] ) {
						cnvtbslsh( &option[5], &option[5] );
						flddelim = option[5];
						flddelim_set = TRUE;
					}
					option = "\0\0";
					break;
				}

				noop = TRUE;
				break;
			default:
				prmsg( MSG_ERROR, "unrecognized option '%c'",
					*option );
				usage( );
			}
		}
	}

	/*
	 * Set environment to handle read (unpack) of packed relations
	 * based on the UNITYUNPACK environement variable.
	 */
	setunpackenv();

	if ( ! uchecktuple ) {
		/* treat tuple errors as fatal errors */
		uchecktuple = UCHECKFATAL;
		utplmsgtype = MSG_ERROR;
		utplelimit = 1;
	} else {
		if ( ( utplelimit == 0 ) && ( quiet ) ) {
			uchecktuple = UCHECKIGNORE;
		}
		utplmsgtype = MSG_WARN;
	}
	utplerrors = 0;

	/*
	 * Get the modified attribute names.
	 */
	anamecnt = -1;
	attrnames = NULL;
	for( i = 0; i < argc; i += 3 )
	{
		if ( strcmp( argv[ i ], "in" ) == 0 )
		{
			attrnames = argv;
			anamecnt = i;		/* 0 to i-1 */
			argc -= i + 1;		/* skip keyword "in" */
			argv += i + 1;
			break;
		}
		if ( argc < i + 5 )
			continue;	/* invalid command syntax */
		if ( argv[i+1][0] == ':' ) {
			if (( argv[i+2][0] == '%' ) && ( strcmp( argv[i+2], "%=" ) ))
				i += 2;
			else
				i += 1;
		} else if (( argv[i+1][0] == '%' ) && ( strcmp( argv[i+1], "%=" ) )) {
			if ( argv[i+2][0] == ':' )
				i += 2;
			else
				i += 1;
		}
		if (( argc > i + 3 ) && ( argv[i+3][0] == ':' ))
			i += 1;		/* skip over attribute modifer */
	}

	if ( anamecnt < 0 )
	{
		if (i)
			prmsg( MSG_ERROR, "unexpected end of modified attribute list" );
		usage( );
	}
	else if ( anamecnt == 0 )
	{
		prmsg( MSG_ERROR, "no attributes listed to modify" );
		usage( );
	}

	/*
	 * Get the list of relations.
	 */
	relnamecnt = argc;
	relnames = argv;
	wherecnt = -1;
	wherelist = NULL;
	for( i = 0; i < argc; i++ )
	{
		if ( strcmp( argv[ i ], "where" ) == 0 )
		{
			relnamecnt = i;		/* 0 to i-1 */
			argc -= i;		/* don't skip "where" */
			argv += i;
			wherelist = argv;
			wherecnt = argc;
			break;
		}
	}

	if ( relnamecnt < 1  )	/* gotta have a table */
	{
		prmsg( MSG_ERROR, "no tables given on command line" );
		usage( );
	}

	if ( relnamecnt > 1 && strcmp( relnames[1], "with" ) == 0 )
	{
		if ( --relnamecnt < 2 )
		{
			prmsg( MSG_ERROR, "no tables given after 'with' keyword" );
			usage( );
		}
		for( i = 1; i < relnamecnt; i++ )
			relnames[i] = relnames[i + 1];
	}

	modrel = getrelinfo( *relnames, NULL, FALSE );
	if ( modrel == NULL )
	{
		prmsg( MSG_ERROR, "cannot get relation info for table '%s'",
			*relnames );
		usage( );
	}
	if ( ! noop && ! chkupdate( modrel->path ) )
	{
		prmsg( MSG_ERROR, "cannot update table '%s'", modrel->path );
		usage( );
	}

	projlist[0] = "1.all:nodisplay=";
	projcnt = 1;

	/*
	 * If record numbers are being printed, ask for
	 * the record number itself.  We won't use the
	 * record number as a string, but asking for it guarantees
	 * that it will be available, i.e., when an index is used
	 * on a relation, record numbers cannot be determined.
	 * Asking for the record number forces indexes to not
	 * be used.
	 */
	if ( printrecnum || prerecorder ) {
		projlist[ projcnt++ ] = "1.rec#:ascending";
		queryflags |= Q_UNIQUE;
	}

	/*
	 * Look up all attribute values;
	 */
	altercnt = 0;
	for( i = 0; i < anamecnt; i += 3 )
	{
		register char *p;
		char *attr_ptr;
		char *dmod_ptr;
		char *smod_ptr;
		char *sval_ptr;
		char *fmt_ptr;
		char smod_attr;
		ALTMODIFIER tmod;
		int attr;
		struct alterinfo *altptr;
		char numbuf[20];

		/*
		 * Save pointer to attribute so we can scan ahead
		 * and check whether or not separate parameters
		 * were given for :modifier and/or %format and
		 * bump the index (i) accordingly.  If we do not
		 * find both then we need to go back and check
		 * if either and/or both were appened to the
		 * attribute name and, if so, we need to replace
		 * the ':' or '%' with '\0' before attempting to
		 * lookup the name of the attribute.
		 */

		attr_ptr = attrnames[i];

		dmod_ptr = smod_ptr = fmt_ptr = NULL;

		p = attrnames[i+1];
		if ( *p == ':' ) {
			dmod_ptr = &p[1];
			p = attrnames[i+2];
			if (( *p == '%' ) && ( strcmp( p, "%=" ))) {
				fmt_ptr = p;
				i += 2;
			} else {
				i += 1;
			}
		} else if (( *p == '%' ) && ( strcmp( p, "%=" ))) {
			fmt_ptr = p;
			p = attrnames[i+2];
			if ( *p == ':' ) {
				dmod_ptr = &p[1];
				i += 2;
			} else {
				i += 1;
			}
		}

		/*
		 * Check if attr_ptr needs to be searched for :modifier or %format.
		 * When finished, "p" will be left pointing to the start of the
		 * %format string if it was (the first to be) appended to the
		 * attribute name such that the '%' needs to be reinserted after
		 * the attribute name lookup.  Otherwise, "p" will be null since
		 * the ':' in a ":modifer" string is always ignored and can be
		 * replaced by '\0' to termate the attribute name or %format string.
		 */
		if ( dmod_ptr == NULL ) {
			if ( fmt_ptr == NULL ) {
				p = attr_ptr;
				while ( ( *p ) && ( *p != ':' ) && ( *p != '%' ) )
					++p;
				switch ( *p ) {
				case ':':
					*p++ = '\0';
					dmod_ptr = p;
					fmt_ptr = strchr( p, '%' );
					p = NULL;
					break;
				case '%':
					fmt_ptr = p;
					dmod_ptr = strchr( p, ':' );
					if ( dmod_ptr ) {
						*dmod_ptr++ = '\0';
					}
					*p = '\0';	/* removes '%' */
					break;
				default:
					p = NULL;
					break;
				}
			} else {	/* fmt_ptr != NULL */
				p = strchr( attr_ptr, ':' );
				if ( p ) {
					*p++ = '\0';
					dmod_ptr = p;
					p = NULL;
				}
			}
		} else { /* dmod_ptr != NULL */
			if ( fmt_ptr == NULL ) {
				p = strchr( attr_ptr, '%' );
				if ( p ) {
					fmt_ptr = p;
					*p = '\0';	/* removes '%' */
				} else {
					fmt_ptr = strchr( dmod_ptr, '%' );
				}
			} else {
				p = NULL;
			}
		}

		attr = findattr( modrel->attrs, modrel->attrcnt, attr_ptr );

		if ( attr < 0 )
		{
			prmsg( MSG_ERROR, "unrecognized attribute name '%s'", attr_ptr );
			usage( );
		}

		if ( p ) {
			/*
			 * Add '%' to beginning of %format
			 * and set attr_ptr to the attribute
			 * name that is part of the relation
			 * data structure so that attribute
			 * name does not include the %format
			 * in any subsequent error printouts.
			 */
			*p = '%';
			attr_ptr = modrel->attrs[attr].aname;
		}

		if ( findalter_attr( attr ) )
		{
			prmsg( MSG_ERROR, "altered attribute '%s' given multiple times on command line", attr_ptr );
			exit( 1 );
		}

		altptr = &alterlist[ altercnt++ ];
		altptr->attr = attr;

		if ( dmod_ptr ) {
			altptr->dmodifier = (char) getaltermod( dmod_ptr );
			if ( altptr->dmodifier == (char) ALT_M_NULL )
			{
				prmsg( MSG_ERROR, "unrecognized alter attribute modifier ':%s'", dmod_ptr );
				prtmodlistmsg( );
				usage( );
			}
		}

		if ( fmt_ptr ) {
			if ( ( altptr->conversion = fmtchk( fmt_ptr ) ) != 0 ) {
				altptr->format = fmt_ptr;
			} else {
				prmsg( MSG_ERROR, "invalid format specification '%s'", fmt_ptr );
				prmsg( MSG_CONTINUE, "Must be of the form %s",
					"%[+ -#]*[0-9]*\\.{0,1}[0-9][bdiouxXeEfgGs]" );
				usage( );
			}
		}

		altptr->op = (short) getalterop( attrnames[i + 1] );

		if ( altptr->op == (short) ALT_OP_NULL ) {
			prmsg( MSG_ERROR, "unrecognized alter operation '%s'",
				attrnames[i + 1] );
			usage( );
		}

		/* if field operation then project requested attribute */
		if ( attrnames[i+1][0] == 'f' ) {
			/*
			 * check for attribute modifier
			 */
			if ( attrnames[i+3][0] == ':' ) {
				smod_ptr = attrnames[i+3];
				smod_attr = 1;	/* separate attribute and modifier args */
			} else {
				/*
				 * Check if modifier was appended to the
				 * source attribute.  Since it is valid
				 * to add attribute modifiers to projected
				 * attributes for sorting (etc.) we will
				 * only look for/at the last modifier.
				 */
				smod_ptr = strrchr( attrnames[i+2], ':' );
				smod_attr = 0;	/* combined attribute:modifier arg */
			}
			if ( smod_ptr != NULL ) {
				altptr->smodifier = getaltermod( &smod_ptr[1] );
				/*
				 * Check for erroneous modifier but do not
				 * report an error if a null ("") modifer
				 * was given since this is how the user
				 * is allowed to indicate that we are not
				 * to use any modifiers that have been
				 * added to the source attribute for use
				 * during the query/projection only.
				 *
				 * Also do not report an error for an
				 * unrecognized modifier that was appended
				 * to the source attribute since we do not
				 * recognize the same attributes that the
				 * query/projection functions recognize.
				 * By the same token, we need to remove the
				 * modifier from the end of the source
				 * attribute string if it is a modifier
				 * that is unique to us (nalter).
				 *
				 * Also, ( smod_ptr == '\0' ) can be used as
				 * a quick check to ignore any subsequent
				 * errors and perform the operation that
				 * is assigned when no modifer was given.
				 */
				switch ( (enum altmodifier) altptr->smodifier ) {
				case ALT_M_NULL:
					if ( smod_ptr[1] ) {
						if ( smod_attr ) {
							prmsg( MSG_ERROR, "unrecognized alter attribute modifier '%s'", smod_ptr );
							prtmodlistmsg( );
							usage( );
						}
					} else {
						if ( smod_attr == 0 ) {
							*smod_ptr = '\0';
						}
					}
					smod_ptr = NULL;
					break;

				/*
				 * WARNING NOTE: Modifiers unique to nalter(1)
				 *		 must be listed here!
				 */
				case ALT_M_LENGTH:
				case ALT_M_TRIM:
				case ALT_M_LTRIM:
				case ALT_M_RTRIM:
				case ALT_M_TOLOWER:
				case ALT_M_TOUPPER:

					if ( smod_attr == 0 ) {
						*smod_ptr = '\0';
						/*
						 * do not ignore errors
						 * for these modifiers
						 */
						smod_attr = 1;
					}
					++smod_ptr;
					break;

				default:		/* common attribute modifiers */
					++smod_ptr;
					break;
				}
			}
			/*
			 * In saving the new attribute name, two
			 * indexes are needed: one is the index in the
			 * projection list passed to fmkquery() and one
			 * is the index in the attributes retrieved.
			 * These are not the same because we use the
			 * "all" attribute for the attributes of the
			 * first relation.
			 *
			 * The two indexes always differ by the number
			 * of attributes in the modified relation - 1.
			 */
			projlist[ projcnt ] = attrnames[ i + 2 ];
			altptr->val.attr = projcnt + modrel->attrcnt - 1;
			projcnt++;

		} else {

			if ( attrnames[i+3][0] == ':' ) {
				prmsg( MSG_ERROR, "unexpected right operand modifier '%s' for alter operation '%s'",
					attrnames[i+3], attrnames[i+1] );
				usage( );
			}

			altptr->val.attr = -1;	/* not a field operator */
		}

		/* initialize pointer to source operand */
		sval_ptr = attrnames[ i + 2 ];


		/*
		 * check basic operation code and expand based on modifiers if appropriate
		 */
		switch ( (enum altopcode) altptr->op ) {

		case ALT_S_ASSIGN:			/* "=" or "to" */

			switch ( (enum altmodifier) altptr->dmodifier ) {
			case ALT_M_NULL:
				if ( altptr->conversion != '\0' ) {
					if ( altptr->conversion != 's' ) {
						altptr->op = (short) ALT_N_ASSIGN;
					}
				}
				break;
			case ALT_M_STRING:
				break;
			case ALT_M_NUMBER:
				altptr->op = (short) ALT_N_ASSIGN;
				break;
			case ALT_M_BINARY:
			case ALT_M_OCTAL:
			case ALT_M_HEX:
				{
					int num;

					switch ( (enum altmodifier) altptr->dmodifier ) {
					case ALT_M_BINARY:
						num = 2;
						break;
					case ALT_M_OCTAL:
						num = 8;
						break;
					case ALT_M_HEX:
						num = 16;
						break;
					}
#ifdef	__STDC__
					/*
					 * Use unsigned conversion if available to
					 * avoid problem with maximum unsigned value
					 * since most (all) non-base 10 values are
					 * initially generated from an unsigned int.
					 */
					num = (int) strtoul( sval_ptr, (char **)NULL, num );
#else
					num = (int) strtol( sval_ptr, (char **)NULL, num );
#endif
					sval_ptr = numbuf;
					sprintf( sval_ptr, "%d", num );

					altptr->op = (short) ALT_N_ASSIGN;
				}
				break;
			case ALT_M_LENGTH:
				{
					int num;

					num = strlen( sval_ptr );
					sval_ptr = numbuf;
					sprintf( sval_ptr, "%d", num );

					altptr->op = (short) ALT_N_ASSIGN;
				}
				break;
			case ALT_M_TRIM:
				{
					int	len;

					p = sval_ptr;
					while ( *p == ' ' )
						++p;
					sval_ptr = p;
					len = strlen( p );
					p += len - 1;
					while ( ( len ) && ( *p == ' ' ) ) {
						*p = '\0';
						--p;
						--len;
					}
				}
				break;
			case ALT_M_LTRIM:
				p = sval_ptr;
				while ( *p == ' ' )
					++p;
				sval_ptr = p;
				break;
			case ALT_M_RTRIM:
				{
					int	len;

					p = sval_ptr;
					len = strlen( p );
					p += len - 1;
					while ( ( len ) && ( *p == ' ' ) ) {
						*p = '\0';
						--p;
						--len;
					}
				}
				break;
			case ALT_M_TOLOWER:
				(void) strcpylower( sval_ptr, sval_ptr );
				break;
			case ALT_M_TOUPPER:
				(void) strcpyupper( sval_ptr, sval_ptr );
				break;
			default:
				prmsg( MSG_ERROR, "invalid modifier ':%s' for alter operation '%s'",
					dmod_ptr, attrnames[i+1] );
				usage( );
				break;
			}
			break;

		case ALT_N_INCR:			/* "+=" */
		case ALT_N_DECR:			/* "-=" */
		case ALT_N_MULT:			/* "*=" */
		case ALT_N_DIVIDE:			/* "/=" */
		case ALT_N_MODULO:			/* "%=" */

			switch ( (enum altmodifier) altptr->dmodifier ) {
			case ALT_M_NULL:
			case ALT_M_NUMBER:
			case ALT_M_BINARY:
			case ALT_M_OCTAL:
			case ALT_M_HEX:
				break;
			default:
				prmsg( MSG_ERROR, "invalid modifier ':%s' for alter operation '%s'",
					dmod_ptr, attrnames[i+1] );
				usage( );
				break;
			}
			break;

		case ALT_A_ASSIGN:			/* "f="  or "fto" */

			switch ( (enum altmodifier) altptr->dmodifier ) {

			case ALT_M_NULL:

				switch ( (enum altmodifier) altptr->smodifier ) {
				default:
					if ( smod_attr ) {
						prmsg( MSG_ERROR, "invalid source attribute modifier ':%s' for alter operation '%s'",
							smod_ptr, attrnames[i+1] );
						usage( );
						break;
					}

					altptr->smodifier = (char) ALT_M_NULL;

					/* FALLTHROUGH */

				case ALT_M_NULL:
					if (( altptr->conversion != '\0' ) &&
					    ( altptr->conversion != 's' )) {
						altptr->op = (short) ALT_A_NUMBER;
					}
					break;
				case ALT_M_NUMBER:
				case ALT_M_BINARY:
				case ALT_M_OCTAL:
				case ALT_M_HEX:
				case ALT_M_LENGTH:
					altptr->op = (short) ALT_A_NUMBER;
					break;
				case ALT_M_STRING:
				case ALT_M_TRIM:
				case ALT_M_LTRIM:
				case ALT_M_RTRIM:
					break;
				case ALT_M_TOLOWER:
				case ALT_M_TOUPPER:
					altptr->dmodifier = altptr->smodifier;
					altptr->smodifier = (char) ALT_M_NULL;
					break;
				}
				break;

			case ALT_M_NUMBER:

				switch ( (enum altmodifier) altptr->smodifier ) {
				default:
					if ( smod_attr ) {
						prmsg( MSG_ERROR, "invalid source attribute modifier ':%s' for numeric assignment",
							smod_ptr );
						usage( );
						break;
					}

					altptr->smodifier = (char) ALT_M_NULL;

					/* FALLTHROUGH */

				case ALT_M_NULL:
				case ALT_M_NUMBER:
				case ALT_M_BINARY:
				case ALT_M_OCTAL:
				case ALT_M_HEX:
				case ALT_M_LENGTH:
					break;
				}
				altptr->op = (short) ALT_A_NUMBER;
				break;

			case ALT_M_BINARY:
			case ALT_M_OCTAL:
			case ALT_M_HEX:

				/* p = conversion type in/for any error messages */
				switch ( (enum altmodifier) altptr->dmodifier ) {
				case ALT_M_BINARY:
					p = "binary";
					break;
				case ALT_M_OCTAL:
					p = "octal";
					break;
				case ALT_M_HEX:
					p = "hex";
					break;
				}

				switch ( (enum altmodifier) altptr->smodifier ) {
				default:
					if ( smod_attr ) {
						prmsg( MSG_ERROR, "invalid source attribute modifier ':%s' for %s conversion",
							smod_ptr, p );
						usage( );
					}

					altptr->smodifier = (char) ALT_M_NULL;

					/* FALLTHROUGH */

				case ALT_M_NULL:
					altptr->smodifier = altptr->dmodifier;
					break;

				case ALT_M_NUMBER:
				case ALT_M_BINARY:
				case ALT_M_OCTAL:
				case ALT_M_HEX:
					if ( altptr->smodifier != altptr->dmodifier ) {
						if ( smod_attr ) {
							prmsg( MSG_ERROR, "invalid source attribute modifier ':%s' for %s conversion",
								smod_ptr, p );
							usage( );
						} else {
							altptr->smodifier = altptr->dmodifier;
						}
					}
					break;
				}

				altptr->op = (short) ALT_A_NUMBER;
				break;

			case ALT_M_LENGTH:

				switch ( (enum altmodifier) altptr->smodifier ) {
				default:
					if ( smod_attr ) {
						prmsg( MSG_ERROR, "invalid source attribute modifier ':%s' for length assignment",
							smod_ptr );
						usage( );
					}

					altptr->smodifier = (char) ALT_M_NULL;

					/* FALLTHROUGH */

				case ALT_M_NULL:
					break;
				case ALT_M_STRING:
					altptr->smodifier = (char) ALT_M_NULL;
					break;
				case ALT_M_TRIM:
				case ALT_M_LTRIM:
				case ALT_M_RTRIM:
					break;
				}
				altptr->op = (short) ALT_A_LENGTH;
				break;

			case ALT_M_STRING:
			case ALT_M_TRIM:

				switch ( (enum altmodifier) altptr->smodifier ) {
				default:
					if ( smod_attr ) {
						prmsg( MSG_ERROR, "invalid source attribute modifier ':%s' for string assignment",
							smod_ptr );
						usage( );
					}

					altptr->smodifier = (char) ALT_M_NULL;

					/* FALLTHROUGH */

				case ALT_M_NULL:
					if ( altptr->dmodifier == (char) ALT_M_TRIM )
						altptr->smodifier = (char) ALT_M_TRIM;
					break;
				case ALT_M_STRING:
					if ( altptr->dmodifier == (char) ALT_M_TRIM )
						altptr->smodifier = (char) ALT_M_TRIM;
					else
						altptr->smodifier = (char) ALT_M_NULL;
					break;
				case ALT_M_TRIM:
					break;
				case ALT_M_LTRIM:
				case ALT_M_RTRIM:
					if ( altptr->dmodifier == (char) ALT_M_TRIM )
						altptr->smodifier = (char) ALT_M_TRIM;
					break;
				case ALT_M_TOLOWER:
				case ALT_M_TOUPPER:
					tmod = altptr->dmodifier;
					altptr->dmodifier = altptr->smodifier;
					altptr->smodifier = tmod;
					break;
				}
				break;

			case ALT_M_LTRIM:
			case ALT_M_RTRIM:

				switch ( (enum altmodifier) altptr->smodifier ) {
				default:
					if ( smod_attr ) {
						prmsg( MSG_ERROR, "invalid source attribute modifier ':%s' for string assignment",
							smod_ptr );
						usage( );
					}

					altptr->smodifier = (char) ALT_M_NULL;

					/* FALLTHROUGH */

				case ALT_M_NULL:
				case ALT_M_STRING:
					altptr->smodifier = altptr->dmodifier;
					break;
				case ALT_M_TRIM:
					break;
				case ALT_M_LTRIM:
				case ALT_M_RTRIM:
					if ( altptr->dmodifier != altptr->smodifier )
						altptr->smodifier = (char) ALT_M_TRIM;
					break;
				case ALT_M_TOLOWER:
				case ALT_M_TOUPPER:
					tmod = altptr->dmodifier;
					altptr->dmodifier = altptr->smodifier;
					altptr->smodifier = tmod;
					break;
				}
				break;

			case ALT_M_TOLOWER:
			case ALT_M_TOUPPER:

				switch ( (enum altmodifier) altptr->smodifier ) {
				default:
					if ( smod_attr ) {
						prmsg( MSG_ERROR, "invalid source attribute modifier ':%s' for string assignment",
							smod_ptr );
						usage( );
					}

					altptr->smodifier = (char) ALT_M_NULL;
					break;
				case ALT_M_NULL:
				case ALT_M_STRING:
				case ALT_M_TRIM:
				case ALT_M_LTRIM:
				case ALT_M_RTRIM:
					break;
				case ALT_M_TOLOWER:
				case ALT_M_TOUPPER:
					if ( altptr->smodifier != altptr->smodifier ) {
						prmsg( MSG_ERROR, "invalid source attribute modifier ':%s' for destination attribute modifier ':%s'",
						smod_ptr, dmod_ptr );
						usage( );
					}
					altptr->smodifier = (char) ALT_M_NULL;
					break;
				}
				break;

			default:
				prmsg( MSG_ERROR, "invalid modifier ':%s' for alter operation '%s'",
					dmod_ptr, attrnames[i+1] );
				usage( );
				break;
			}

			break;

		case ALT_A_INCR:			/* "f+=" */
		case ALT_A_DECR:			/* "f-=" */
		case ALT_A_MULT:			/* "f*=" */
		case ALT_A_DIVIDE:			/* "f/=" */
		case ALT_A_MODULO:			/* "f%=" */

			/* deferring error checks to "final" switch statement */
			break;
		}


		/* do final (expanded) operation code checks and update modifiers as appropriate */
		switch ( (enum altopcode) altptr->op ) {

		case ALT_S_ASSIGN:

			if ( altptr->conversion != '\0' ) {
				if ( altptr->conversion != 's' ) {
					prmsg( MSG_ERROR, "cannot use numeric type format specification in string assignment" );
					usage( );
				}
				if ( strcmp( altptr->format, "%s" ) == 0 ) {
					altptr->conversion = '\0';
				}
			}
			altptr->val.strval = sval_ptr;
			altptr->dmodifier = (char) ALT_M_NULL;
			altptr->smodifier = (char) ALT_M_NULL;
			break;

		case ALT_N_ASSIGN:
		case ALT_N_INCR:
		case ALT_N_DECR:
		case ALT_N_MULT:
		case ALT_N_DIVIDE:
		case ALT_N_MODULO:

			if ( altptr->conversion != '\0' ) {
				if ( altptr->conversion == 's' ) {
					prmsg( MSG_ERROR, "cannot use string type format specification in numeric assignment" );
					usage( );
				}
			} else {
				altptr->conversion = 'g';
			}
			if (( sval_ptr[0] == '0' ) &&
			   (( sval_ptr[1] == 'x' ) ||
			    ( sval_ptr[1] == 'X' ))) {
#ifdef	__STDC__
				/*
				 * Use unsigned conversion if available to
				 * avoid problem with maximum unsigned value
				 * since most (all) non-base 10 values are
				 * initially generated from an unsigned int.
				 */
				altptr->val.numval = (int) strtoul( sval_ptr, (char **)NULL, 16 );
#else
				altptr->val.numval = (int) strtol( sval_ptr, (char **)NULL, 16 );
#endif
			} else {
				if ( altptr->op == (short) ALT_N_MODULO ) {
					altptr->val.numval = (int) atof( sval_ptr );
				} else {
					altptr->val.numval = atof( sval_ptr );
				}
			}
			if (( altptr->val.numval == 0 ) &&
			   (( altptr->op == (short) ALT_N_DIVIDE ) ||
			    ( altptr->op == (short) ALT_N_MODULO ))) {
				prmsg( MSG_ERROR, "cannot divide attribute value by zero" );
				usage();
			}
			switch ( (enum altmodifier) altptr->dmodifier ) {
			case ALT_M_NUMBER:
			case ALT_M_BINARY:
			case ALT_M_OCTAL:
			case ALT_M_HEX:
				if ( altptr->op == (short) ALT_N_ASSIGN ) {
					altptr->dmodifier = (char) ALT_M_NULL;
				}
				break;
			case ALT_M_NULL:
			default:
				if ( altptr->op == (short) ALT_N_ASSIGN ) {
					altptr->dmodifier = (char) ALT_M_NULL;
				} else {
					altptr->dmodifier = (char) ALT_M_NUMBER;
				}
				break;
			}
			altptr->smodifier = (char) ALT_M_NULL;
			break;

		case ALT_A_ASSIGN:

			switch ( (enum altmodifier) altptr->smodifier ) {
			case ALT_M_NULL:
			case ALT_M_TRIM:
			case ALT_M_LTRIM:
			case ALT_M_RTRIM:
				break;
			case ALT_M_STRING:
				altptr->smodifier = (char) ALT_M_NULL;
				break;
			default:
				prmsg( MSG_ERROR, "invalid source attribute modifier ':%s' for string assignment",
					smod_ptr, attrnames[i+1] );
				usage( );
				break;
			}

			if ( altptr->conversion != '\0' ) {
				if ( altptr->conversion != 's' ) {
					prmsg( MSG_ERROR, "cannot use numeric type format specification in string assignment" );
					usage( );
				}
				if ( strcmp( altptr->format, "%s" ) == 0 ) {
					altptr->conversion = '\0';
				}
			}

			switch ( altptr->dmodifier ) {
			case ALT_M_TOLOWER:
			case ALT_M_TOUPPER:
				break;
			default:
				altptr->dmodifier = (char) ALT_M_NULL;
			}
			/* 
			 * The projected attribute has already been saved: altptr->val.attr
			 */
			break;

		case ALT_A_NUMBER:

			switch ( (enum altmodifier) altptr->smodifier ) {
			case ALT_M_NULL:
				altptr->smodifier = (char) ALT_M_NUMBER;
			case ALT_M_NUMBER:
			case ALT_M_BINARY:
			case ALT_M_OCTAL:
			case ALT_M_HEX:
			case ALT_M_LENGTH:
				break;
			default:
				prmsg( MSG_ERROR, "invalid source attribute modifier ':%s' for numeric assignment",
					smod_ptr, attrnames[i+1] );
				usage( );
				break;
			}

			if ( altptr->conversion != '\0' ) {
				if ( altptr->conversion == 's' ) {
					prmsg( MSG_ERROR, "cannot use string type format specification in numeric assignment" );
					usage( );
				}
			} else {
				altptr->conversion = 'g';
			}
			altptr->dmodifier = (char) ALT_M_NULL;
			/* 
			 * The projected attribute has already been saved: altptr->val.attr
			 */
			break;

		case ALT_A_LENGTH:

			if ( altptr->conversion != '\0' ) {
				if ( altptr->conversion == 's' ) {
					prmsg( MSG_ERROR, "cannot use string type format specification in numeric assignment" );
					usage( );
				}
			} else {
				altptr->conversion = 'g';
			}
			altptr->dmodifier = (char) ALT_M_NULL;
			/* 
			 * The projected attribute has already been saved: altptr->val.attr
			 */
			break;

		case ALT_A_INCR:
		case ALT_A_DECR:
		case ALT_A_MULT:
		case ALT_A_DIVIDE:
		case ALT_A_MODULO:

			switch ( (enum altmodifier) altptr->dmodifier ) {
			case ALT_M_NULL:
				altptr->dmodifier = (char) ALT_M_NUMBER;
				break;
			case ALT_M_NUMBER:
			case ALT_M_BINARY:
			case ALT_M_OCTAL:
			case ALT_M_HEX:
				break;
			default:
				prmsg( MSG_ERROR, "invalid numeric attribute modifier ':%s' for alter operation '%s'",
					dmod_ptr, attrnames[i+1] );
				usage( );
				break;
			}

			switch ( (enum altmodifier) altptr->smodifier ) {
			case ALT_M_NULL:
				altptr->smodifier = (char) ALT_M_NUMBER;
				break;
			case ALT_M_NUMBER:
			case ALT_M_BINARY:
			case ALT_M_OCTAL:
			case ALT_M_HEX:
			case ALT_M_LENGTH:
				break;
			default:
				if ( smod_attr ) {
					prmsg( MSG_ERROR, "invalid source attribute modifier ':%s' for alter operation '%s'",
						smod_ptr, attrnames[i+1] );
					usage( );
				}
				altptr->smodifier = (char) ALT_M_NUMBER;
				break;
			}

			if ( altptr->conversion != '\0' ) {
				if ( altptr->conversion == 's' ) {
					prmsg( MSG_ERROR, "cannot use string type format specification in numeric assignment" );
					usage( );
				}
			} else {
				altptr->conversion = 'g';
			}
			/* 
			 * The projected attribute has already been saved: altptr->val.attr
			 */
			break;

		default:	/* new code error - do nothing for this attribute */

			altptr->conversion = '\0';
			altptr->op = (short) ALT_OP_NULL;
			altptr->dmodifier = (char) ALT_M_NULL;
			altptr->smodifier = (char) ALT_M_NULL;
			break;
		}

		if ( altptr->format == NULL ) {
			switch ( altptr->conversion ) {
			case '\0':
			case 's':
				altptr->format = format_s_default;
				break;
			case 'g':
			default:
				altptr->format = format_g_default;
				break;
			}
		}

		if ( attrnames[i+3][0] == ':' ) {
			i += 1;
		}

	}	/* end of look up all attribute values loop */


	query = fmkquery( queryflags, relnames, relnamecnt, projlist, projcnt,
			(char **)NULL, 0, wherelist, wherecnt );
	if ( query == NULL )
	{
		prmsg( MSG_ERROR, "cannot create query for alteration" );
		exit( 1 );
	}

	uid = geteuid();
	gid = getegid();

	maxattr = 0;
	for( i = 0; i < (long) query->nodecnt; i++ )
	{
		struct urelation *relptr;

		relptr = query->nodelist[i]->rel;
		if ( relptr->attrcnt > (unsigned) maxattr )
			maxattr = relptr->attrcnt;

		if ( strcmp( relptr->path, "-" ) != 0 &&
			! chkperm( relptr->path, P_READ, uid, gid ) )
		{
			prmsg( MSG_ERROR, "cannot read file '%s'",
				relptr->path );
			usage( );
		}
	}

	(void)set_attralloc( maxattr );

	if ( printold || printnew )
	{
		register struct uattribute *attrptr;
		register struct alterinfo *altptr;

		for( i = 0, altptr = alterlist; i < altercnt; i++, altptr++ )
		{
			if ( flddelim_set ) {
				altptr->attrtype = QP_TERMCHAR;
				altptr->delim = (char)flddelim;
				altptr->width = 0;
			} else {
				attrptr = &modrel->attrs[ altptr->attr ];
				if ( attrptr->attrtype == UAT_TERMCHAR ) {
					altptr->attrtype = QP_TERMCHAR;
					altptr->width = 0;
					altptr->delim = ( altptr->attr == modrel->attrcnt - 1 ) ?
							'\t' : (char)attrptr->terminate;
				} else {
					altptr->attrtype = QP_FIXEDWIDTH;
					altptr->width = attrptr->terminate;
					altptr->delim = '\0';
				}
			}
		}
		alterlist[ altercnt - 1 ].delim = '\n';
	}

	(void)set_sig( SIGHUP, catchsig );	/* catch signals */
	(void)set_sig( SIGINT, catchsig );
	(void)set_sig( SIGQUIT, catchsig );
	(void)signal( SIGILL, catchsig );
#ifdef SIGIOT
	(void)signal( SIGIOT, catchsig );
#endif
#ifdef SIGEMT
	(void)signal( SIGEMT, catchsig );
#endif
	(void)signal( SIGFPE, catchsig );
	(void)signal( SIGBUS, catchsig );
	(void)signal( SIGSEGV, catchsig );
#ifdef SIGSYS
	(void)signal( SIGSYS, catchsig );
#endif
	(void)signal( SIGPIPE, catchsig );
	(void)set_sig( SIGTERM, catchsig );

	if ( (perptr = init_peruse( modrel, noop ? "r" : "w" )) == NULL )
	{
		(void)pruerror( );
		prmsg( MSG_ERROR, "cannot initialize update to table '%s'",
			modrel->path );
		exit( 2 );
	}

	seen_error = FALSE;
	modifycnt = 0;

	if (prerecorder)	/* Preserve Record Order */
	{
		struct qresult result;
		struct qprojtuple proj;
		struct utuple tpl;
		char *attrvals[MAXATT];
		char *attrvalsnc[MAXATT];

		if ( ! queryeval( query, &result ) ) {
			if ( uerror ) {
				(void)pruerror( );
				prmsg( MSG_ERROR, "query evaluation failed, no alterations done" );
			}
			else
				prmsg( MSG_ERROR, "due to previous errors, no alterations done" );
			(void)end_peruse( perptr, FALSE );
			exit( 2 );
		}
		if ( ! initresult( &result ) ) {
			if ( uerror ) {
				(void)pruerror( );
				prmsg( MSG_ERROR, "failed to initialize list of tuples to be altered" );
				(void)end_peruse( perptr, FALSE );
				exit( 2 );

			}
			if ( ! end_peruse( perptr, FALSE ) ) {
				if ( uerror )
					(void)pruerror( );
				prmsg( MSG_ERROR, "failed to initialize list of tuples to be altered" );
				exit( 2 );
			}
			if ( ! quiet ) {
				if ( utplerrors ) {
					prmsg( MSG_WARN, "%d tuple error%s encountered reading relation(s)",
						utplerrors, utplerrors != 1 ? "s" : "" );
				}
				prmsg( MSG_ERRNOTE, "0 records %smodified",
					noop ? "would be " : "" );
			}
			exit( 0 );
		}

		/*
		 * Since the relation that is being altered is read twice
		 * we need to save the value of utplerrors and prevent
		 * redundant tuple syntax error reports.
		 */
		if ( utplerrors ) {
			utplerrors_orig = utplerrors;
			uchecktuple = UCHECKIGNORE;
		}

		proj.projptr = NULL;
		tpl.tplval = attrvalsnc;
		tpl.tuplenum = 0;
		tpl.flags = 0;
		i = 0;		/* record sequence index */

		while( nexttuple( query, &result, attrvals ) )
		{
			proj.tplptr = result.curblk->tuples[result.curblk->curtpl];
			
			while ( peruse_tuple( perptr, &tpl ) &&
			      ( tpl.tuplenum == ++i ) )
			{
				/*******************************************
				if ( ! quiet && (tpl.flags & TPL_ERRORMSK) )
					prtplerror( perptr->relptr, &tpl );
				*******************************************/

				if ( i == proj.tplptr->tuplenum )
					break;

				if ( ! noop && ! savetuple( perptr, attrvalsnc ) )
				{
					(void)pruerror( );
					prmsg( MSG_INTERNAL, "cannot save tuple #%d for table '%s'",
						tpl.tuplenum, perptr->relptr->path );
					(void)end_peruse( perptr, FALSE );
					exit( 2 );
				}
			}
			if ( tpl.tuplenum != proj.tplptr->tuplenum ) {
				prmsg( MSG_INTERNAL, "tuple sequence error at record #%d for table '%s'",
					i, perptr->relptr->path );

				(void)end_peruse( perptr, FALSE );
				exit( 2 );
			}

			if ( ! alterattrs( attrvals, query->attrcnt, &proj ) ) {
				prmsg( MSG_ERROR, "due to previous errors, no alterations done" );
				(void)end_peruse( perptr, FALSE );
				exit( 2 );
			}
		}

		/* Make sure that an error did not cause nexttuple() to return NULL */
		if ( uerror )
		{
			(void)pruerror( );
			prmsg( MSG_ERROR, "due to previous errors, no alterations done" );
			(void)end_peruse( perptr, FALSE );
			exit( 2 );
		}

		while ( peruse_tuple( perptr, &tpl ) )
		{
			/*******************************************
			if ( ! quiet && (tpl.flags & TPL_ERRORMSK) )
				prtplerror( perptr->relptr, &tpl );
			*******************************************/

			if ( tpl.tuplenum != ++i ) {
				prmsg( MSG_INTERNAL, "tuple sequence error at record #%d for table '%s'",
					i, perptr->relptr->path );

				(void)end_peruse( perptr, FALSE );
				exit( 2 );

			}

			if ( ! noop && ! savetuple( perptr, attrvalsnc ) )
			{
				(void)pruerror( );
				prmsg( MSG_INTERNAL, "cannot save tuple #%d for table '%s'",
					tpl.tuplenum, perptr->relptr->path );
				(void)end_peruse( perptr, FALSE );
				exit( 2 );
			}
		}

		/* Make sure that an error did not cause peruse_tuple() to return NULL */
		if ( uerror )
		{
			(void)pruerror( );
			prmsg( MSG_ERROR, "due to previous errors, no alterations done" );
			(void)end_peruse( perptr, FALSE );
			exit( 2 );
		}

	} else {	/* Do not preserve record order */
		/*
		 * The alterattss() function will be called by the
		 * queryeval() done in seeksave().
		 */
		settplfunc( query, alterattrs );

		if ( ! seeksave( query, TRUE ) || seen_error )
		{
			if ( uerror ) {
				(void)pruerror( );
				prmsg( MSG_ERROR, "query evaluation failed, no alterations done" );
			}
			else
				prmsg( MSG_ERROR, "due to previous errors, no alterations done" );
			(void)end_peruse( perptr, FALSE );
			exit( 2 );
		}
		else if ( getseekcnt( ) == 0 )
		{
			if ( ! end_peruse( perptr, FALSE ) ) {
				if ( uerror )
					(void)pruerror( );
				prmsg( MSG_ERROR, "due to previous errors, no alterations done" );
				exit( 2 );
			}
			if ( ! quiet ) {
				if ( utplerrors ) {
					prmsg( MSG_WARN, "%d tuple error%s encountered reading relation(s)",
						utplerrors, utplerrors != 1 ? "s" : "" );
				}
				prmsg( MSG_ERRNOTE, "0 records %smodified",
					noop ? "would be " : "" );
			}
			exit( 0 );
		}

		/*
		 * Since the relation that is being altered is read twice
		 * we need to save the value of utplerrors and prevent
		 * redundant tuple syntax error reports.
		 */
		if ( utplerrors ) {
			utplerrors_orig = utplerrors;
			uchecktuple = UCHECKIGNORE;
		}

		i = save_unchgd( perptr, noop, TRUE, (int (*)())NULL );
		if ( i < 0 )
		{
			if ( uerror )
				(void)pruerror( );
			prmsg( MSG_ERROR, "due to previous errors, no alterations done" );
			(void)end_peruse( perptr, FALSE );
			exit( 2 );
		}
		else if ( i != getseekcnt( ) || i != modifycnt )
		{
			prmsg( MSG_INTERNAL, "cannot find all tuples destined to be altered (altered %ld, modified %ld, should have been %ld) -- alterations aborted",
				i, modifycnt, getseekcnt( ) );
			(void)end_peruse( perptr, FALSE );
			exit( 2 );
		}
	}

	(void)signal( SIGHUP, SIG_IGN );	/* ignore signals */
	(void)signal( SIGINT, SIG_IGN );
	(void)signal( SIGQUIT, SIG_IGN );
	(void)signal( SIGTERM, SIG_IGN );

	if ( ! end_peruse( perptr, i > 0 ) )
	{
		if ( uerror )
			(void)pruerror( );
		prmsg( MSG_ERROR, "cannot update table '%s' -- no modifications done",
			modrel->path );
		exit( 2 );
	}

	if ( ! quiet ) {
		if ( utplerrors ) {
			if ( utplerrors_orig == 0 )
				utplerrors_orig = utplerrors;
			prmsg( MSG_WARN, "%d tuple error%s encountered reading relation(s)",
				utplerrors_orig, utplerrors_orig != 1 ? "s" : "" );
		}
		prmsg( MSG_ERRNOTE, "%d record%s %smodified", modifycnt,
			modifycnt != 1 ? "s" : "",
			noop ? "would be " : "" );
	}

	exit( 0 );
}

usage( )
{
	prmsg( MSG_USAGE, "[-adiopqrV] [-Q [ErrorLimit]] \\" );
	prmsg( MSG_CONTINUE, "<attr> [:modifier] [%%format] [f][+|-|*|/|%%]= <value|attr> [:modifier] \\" );
	prmsg( MSG_CONTINUE, "[attr ...] in <table>[=<alt_table>] [with <table>=[<alt_table>] ...] \\" );
	prmsg( MSG_CONTINUE, "[where <where-clause>]" );
	exit( 1 );
}

badarg( arg )
char *arg;
{
	prmsg( MSG_ERROR, "unrecognized argument '%s'", arg );
	usage( );
}

set_sig( sig, func )
int sig;
RETSIGTYPE (*func)();
{
	if ( signal( sig, SIG_IGN ) != (RETSIGTYPE(*)())SIG_IGN )
		(void)signal( sig, func );
}

RETSIGTYPE
catchsig( sig )
int sig;
{
	(void)signal( sig, SIG_IGN );

	(void)end_peruse( (struct uperuseinfo *)NULL, FALSE );

	if ( sig == SIGHUP || sig == SIGINT || sig == SIGTERM || sig == SIGQUIT ){
		prmsg( MSG_ALERT, "killed by signal (%d) -- no alterations done",
			sig );

		if ( sig != SIGQUIT )
			exit( 3 );
	}

	(void)signal( sig, SIG_DFL );	/* do the normal operation */
	(void)kill( getpid( ), sig );

	exit( 2 );		/* should never reach here */
}

findalter_attr( attr )
int attr;
{
	register int i;

	for( i = 0; i < altercnt; i++ )
	{
		if ( alterlist[i].attr == attr )
			return( TRUE );
	}

	return( FALSE );
}

#ifndef MAXALTERBUF
#define MAXALTERBUF	1024	/* max space for single altered numeric val */
#endif

alterattrs( attrvals, attrcnt, projptr )
char **attrvals;
int attrcnt;
struct qprojtuple *projptr;	/* WARNING: projptr->projptr is NULL */
{
	register int i;
	register struct alterinfo *altptr;
	register char *bufptr;
	double num, snum;
	char conversion;	/*   %format conversion type	*/
	char *S;	/* pointer to end of substr char (' ')  */
	char tmpbuf[DBBLKSIZE];

	if ( printrecnum )
		printf( "%ld%c", projptr->tplptr->tuplenum,
			printold || printnew ? ':' : '\n' );
	S = NULL;
	bufptr = tmpbuf;
	for( i = 0, altptr = alterlist; i < altercnt; i++, altptr++ )
	{
		if ( printold )
			(void)writeattrvalue( stdout, attrvals[altptr->attr],
				printnew  ? '=' : altptr->delim, TRUE,
				altptr->width, altptr->attrtype );

		conversion = altptr->conversion;

		switch ( (enum altmodifier) altptr->smodifier ) {
		case ALT_M_NULL:
			break;
		case ALT_M_NUMBER:
			snum = atof( attrvals[altptr->val.attr] );
			break;
		case ALT_M_HEX:
#ifdef	__STDC__
			/*
			 * Use unsigned conversion if available to
			 * avoid problem with maximum unsigned value
			 * since most (all) non-base 10 values are
			 * initially generated from an unsigned int.
			 */
			snum = (int) strtoul( attrvals[altptr->val.attr], (char **)NULL, 16 );
#else
			snum = (int) strtol( attrvals[altptr->val.attr], (char **)NULL, 16 );
#endif
			break;
		case ALT_M_OCTAL:
#ifdef	__STDC__
			/*
			 * Use unsigned conversion if available to
			 * avoid problem with maximum unsigned value
			 * since most (all) non-base 10 values are
			 * initially generated from an unsigned int.
			 */
			snum = (int) strtoul( attrvals[altptr->val.attr], (char **)NULL, 8 );
#else
			snum = (int) strtol( attrvals[altptr->val.attr], (char **)NULL, 8 );
#endif
			break;
		case ALT_M_BINARY:
#ifdef	__STDC__
			/*
			 * Use unsigned conversion if available to
			 * avoid problem with maximum unsigned value
			 * since most (all) non-base 10 values are
			 * initially generated from an unsigned int.
			 */
			snum = (int) strtoul( attrvals[altptr->val.attr], (char **)NULL, 2 );
#else
			snum = (int) strtol( attrvals[altptr->val.attr], (char **)NULL, 2 );
#endif
			break;
		case ALT_M_LENGTH:
			snum = strlen( attrvals[altptr->val.attr] );
			break;
		case ALT_M_TRIM:
			{
				register char *p, *q;
				int	len;

				p = attrvals[altptr->val.attr];
				while ( *p == ' ' )
					++p;
				attrvals[altptr->val.attr] = p;
				len = strlen( p );
				if ( len ) {
					q = p + len - 1;
					while ( ( q >= p ) && ( *q == ' ' ) )
						--q;
					++q;
					if ( len > q - p ) {
						*q = '\0';
						S = q;
						if ( conversion == '\0' ) {
							conversion = 's';
						}
					}
				}
			}
			break;
		case ALT_M_LTRIM:
			{
				register char *p;

				p = attrvals[altptr->val.attr];
				while ( *p == ' ' )
					++p;
				attrvals[altptr->val.attr] = p;
			}
			break;
		case ALT_M_RTRIM:
			{
				register char *p, *q;
				int	len;

				p = attrvals[altptr->val.attr];
				len = strlen( p );
				if ( len ) {
					q = p + len - 1;
					while ( ( q >= p ) && ( *q == ' ' ) )
						--q;
					++q;
					if ( len > q - p ) {
						*q = '\0';
						S = q;
						if ( conversion == '\0' ) {
							conversion = 's';
						}
					}
				}
			}
			break;
		default:
			snum = 0;
			break;
		}

		switch ( (enum altmodifier) altptr->dmodifier ) {
		case ALT_M_NULL:
			break;
		case ALT_M_NUMBER:
			num = atof( attrvals[altptr->attr] );
			break;
		case ALT_M_HEX:
#ifdef	__STDC__
			/*
			 * Use unsigned conversion if available to
			 * avoid problem with maximum unsigned value
			 * since most (all) non-base 10 values are
			 * initially generated from an unsigned int.
			 */
			num = (int) strtoul( attrvals[altptr->attr], (char **)NULL, 16 );
#else
			num = (int) strtol( attrvals[altptr->attr], (char **)NULL, 16 );
#endif
			break;
		case ALT_M_OCTAL:
#ifdef	__STDC__
			/*
			 * Use unsigned conversion if available to
			 * avoid problem with maximum unsigned value
			 * since most (all) non-base 10 values are
			 * initially generated from an unsigned int.
			 */
			num = (int) strtoul( attrvals[altptr->attr], (char **)NULL, 8 );
#else
			num = (int) strtol( attrvals[altptr->attr], (char **)NULL, 8 );
#endif
			break;
		case ALT_M_BINARY:
#ifdef	__STDC__
			/*
			 * Use unsigned conversion if available to
			 * avoid problem with maximum unsigned value
			 * since most (all) non-base 10 values are
			 * initially generated from an unsigned int.
			 */
			num = (int) strtoul( attrvals[altptr->attr], (char **)NULL, 2 );
#else
			num = (int) strtol( attrvals[altptr->attr], (char **)NULL, 2 );
#endif
			break;
		case ALT_M_LENGTH:
			num = strlen( attrvals[altptr->attr] );
			break;
		case ALT_M_TOLOWER:
		case ALT_M_TOUPPER:
			{
				char		*sptr;
				unsigned	len;
				static char	tobuf[DBBLKSIZE+1];

				sptr = attrvals[altptr->val.attr];

				if ( conversion ) {
					if ( altptr->dmodifier == ALT_M_TOLOWER )
						(void) strcpylower( tobuf, sptr );
					else
						(void) strcpyupper( tobuf, sptr );
					attrvals[altptr->val.attr] = tobuf;
				} else {
					len = strlen( sptr );
					if ( bufptr + len + 1 < &tmpbuf[DBBLKSIZE] ) {
						if ( altptr->dmodifier == ALT_M_TOLOWER )
							(void) strcpylower( bufptr, sptr );
						else
							(void) strcpyupper( bufptr, sptr );
						attrvals[altptr->val.attr] = bufptr;
						bufptr += len + 1;
					} else {
						/*
						 * Don't bother converting the input string
						 * since it is too big to fit in the buffer
						 * but set the conversion and error flags so
						 * that the buffer overflow error is reported.
						 */
						conversion = 's';
						seen_error = TRUE;
					}
				}
			}
			num = 0;
			break;
		default:
			num = 0;
			break;
		}

		switch ( (enum altopcode) altptr->op ) {
		case ALT_S_ASSIGN:
			attrvals[altptr->attr] = altptr->val.strval;
			break;
		case ALT_A_ASSIGN:
			attrvals[altptr->attr] = attrvals[altptr->val.attr];
			break;
		case ALT_N_ASSIGN:
			num = altptr->val.numval;
			break;
		case ALT_A_NUMBER:
			num = snum;
			break;
		case ALT_N_INCR:
			num = num + altptr->val.numval;
			break;
		case ALT_N_DECR:
			num = num - altptr->val.numval;
			break;
		case ALT_N_MULT:
			num = num * altptr->val.numval;
			break;
		case ALT_N_DIVIDE:
			num = num / altptr->val.numval;
			break;
		case ALT_N_MODULO:
			num = (int) num % (int) altptr->val.numval;
			break;
		case ALT_A_INCR:
			num = num + snum;
			break;
		case ALT_A_DECR:
			num = num - snum;
			break;
		case ALT_A_MULT:
			num = num * snum;
			break;
		case ALT_A_DIVIDE:
			if ( snum == 0 )
			{
				conversion = '\0';
				seen_error = TRUE;
				prmsg( MSG_ERROR, "attribute update divides by zero in attribute '%s' on record #%ld -- cannot do update",
					perptr->relptr->attrs[altptr->attr].aname,
					projptr->tplptr->tuplenum );
			}
			else
			{
				num = num / snum;
			}
			break;
		case ALT_A_MODULO:
			if ( ( (int) snum ) == 0 )
			{
				conversion = '\0';
				seen_error = TRUE;
				prmsg( MSG_ERROR, "attribute update divides by zero in attribute '%s' on record #%ld -- cannot do update",
					perptr->relptr->attrs[altptr->attr].aname,
					projptr->tplptr->tuplenum );
			}
			else
			{
				num = ( (int) num ) % ( (int) snum);
			}
			break;
		case ALT_A_LENGTH:
			num = strlen( attrvals[altptr->val.attr] );
			if ( S ) {
				*S = ' ';
				S = NULL;
			}
			break;
		default:
			conversion = '\0';
			break;
		}
	
		if ( conversion ) {
			char modbuf[DBBLKSIZE];
			short len;

			switch ( conversion ) {
			case 'g':
				len = sprintf( modbuf, altptr->format, num );
				break;
			case 's':
				len = sprintf( modbuf, altptr->format, attrvals[altptr->attr] );
				break;
			case 'd':
			case 'i':
				len = sprintf( modbuf, altptr->format, (int) num );
				break;
			case 'b':
				len = sprintfb( modbuf, altptr->format, (unsigned) num );
				break;
			case 'o':
			case 'u':
			case 'x':
			case 'X':
				len = sprintf( modbuf, altptr->format, (unsigned) num );
				break;
			default:	/* some type of floating point format */
				len = sprintf( modbuf, altptr->format, num );
				break;
			}

			if ( bufptr + len + 1 < &tmpbuf[DBBLKSIZE] )
			{
				strcpy( bufptr, modbuf );
				attrvals[altptr->attr] = bufptr;
				bufptr += len + 1;
			}
			else
			{
				seen_error = TRUE;
				prmsg( MSG_INTERNAL, "record #%ld, attribute '%s': altered values consume more space (%d) than alloted (%d) for record -- attribute not updated",
					projptr->tplptr->tuplenum,
					perptr->relptr->attrs[altptr->attr].aname,
					bufptr + len + 1 - tmpbuf,
					DBBLKSIZE );
			}
		}

		if ( S ) {
			*S = ' ';
			S = NULL;
		}

		if ( printnew )
			(void)writeattrvalue( stdout, attrvals[altptr->attr],
				altptr->delim, TRUE,
				altptr->width, altptr->attrtype );
	}

	if ( ! noop && ! savetuple( perptr, attrvals ) )
	{
		(void)pruerror( );
		prmsg( MSG_INTERNAL, "tuple save failed for table '%s'",
			perptr->relptr->path );
		return( FALSE );
	}
	else
		modifycnt++;

	return( TRUE );
}

static int
fmtchk(format)
char	*format;
{
	register int		i;
	register char		*p;
	int			dot;
	int			minus;
	int			zero, leadzero;
	int			precision;
	int			width;
	int			fmterr;
	char			conversion;

	/*
	 * output format for altered field must match the regular expression:
	 *
	 *	%[+ -#]*[0-9]*\.{0,1}[0-9]*[bdiouxXeEfgGs]
	 *
	 */

	conversion = 0;

	precision = 1;	/* default precision */

	dot = minus = zero = leadzero = width = 0;

	fmterr = i = 0;

	if ((format == NULL) || (*format != '%')) {
		p = "";
		++fmterr;
	} else {
		p = format + 1;

		/*
		 * Skip over initial flags that do not change
		 * the basic width/precision check/calculation.
		 */
		if ((*p == '#') ||
		    (*p == '+') ||
		    (*p == ' ')) {
			++p;
		}
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
			precision = 0;
			zero = 0;	/* use ' ' to pad width */
			break;
		case 'b':
		case 'd':
		case 'i':
		case 'o':
		case 'u':
		case 'x':
		case 'X':
		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G':
		case 's':
			if (*p != 0) {
				++fmterr;
			}
			if (fmterr == 0) {
				conversion = i;
			}
			break;
		case '0':
			if ((!(dot)) && (!(minus))) {
				++zero;
				++leadzero;
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
			{
				register	num;

				num = i - '0';

				while (isdigit(i = *p)) {
					num = num * 10 + i - '0';
					++p;
				}
				if (num < MAXALTERBUF) {
					if (dot) {
						precision = num;
					} else {
						width = num;
					}
				} else {
					++fmterr;
				}
			}
			break;
		default:
			/* invalid format */
			++fmterr;
		}
	}

	/* were there any format errors? */
	if (fmterr) {
		return(0);
	}

	return(conversion);
}
