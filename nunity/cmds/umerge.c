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
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include "uerror.h"
#include "uquery.h"
#include "message.h"
#include "permission.h"

/*
 * Definitions for saying what the next type of clause (output
 * redirection or where conditions) is on the command line.
 */
#define CL_FROM		1	/* from table(s) next on cmd line */
#define CL_SORT		2	/* list of attributes to sort on */
#define	CL_OUTPUT	3	/* output table (in/into/onto) */

#define	SPECIALBUFLEN	12	/* length of buffer required for special attribute values */

typedef enum {
	IN_TABLE = 0,
	INTO_TABLE = 1,
	ONTO_TABLE = 2
} OutputMode;

struct mattrinfo {
	char *destval;		/* contant value of destination attribute */
	short sattrnum;		/* source attribute number (index) */
	unsigned short	width;	/* destination attribute width */
	short dattridx;		/* destination attribute index - reverse mapping */
	char special;		/* special source attribute (rec#, seek#, REC#) */
};

extern struct queryexpr *fparsewhere();
extern struct uquery *fbldquery();
extern struct urelation *getrelinfo();
extern struct uperuseinfo *init_peruse();
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
extern char *calloc();
#endif
extern char *basename();
extern long atol();
extern void setunpackenv();

RETSIGTYPE catch_sig();

extern char uversion[];			/* version string */

extern int  uqpnvalue;		/* set to QP_NEWVALUE to allow "value=" attribute modifier */
extern int  uchecktuple;	/* if set readtuple() will count/report tuple errors */
extern long utplerrors; 	/* no. of tuples read with tuple errors */
extern long utplelimit; 	/* limit on number of tuple errors reported */
extern int  utplmsgtype;	/* MSG_ERROR or MSG_WARN type tuple error message */

char *prog;

static struct urelation relinfo;	/* relation infor for insertions */
static struct uquery *query;		/* compiled version of query */
static struct uquery *usort;		/* compiled version of usort to compare tuples */
static struct qresult result;		/* query result */
static struct uperuseinfo *perptr;	/* perusal info for output table */
static struct mattrinfo *mapinfo;	/* query to perusal attribute mapping info */
static char queryflags;			/* flags for query */
static char typemlimit;			/* type of merge limit (exact or at most) */
static int mergelimit;			/* number of records to be merged */
static int retrievelimit;		/* maximum recK# or recM# or recQ# set */
static int retrieveKlimit;		/* maximum recK# to be retrieved when set */
static int retrieveMlimit;		/* maximum recM# to be retrieved when set */
static int retrieveQlimit;		/* maximum recQ# to be retrieved when set */
static int delimcnt;			/* number of displayed attrs */
static unsigned long origtplcnt;	/* number of tuples in original output table */
static unsigned long querytplcnt;	/* number of tuples retrieved from query */
static unsigned long keytplcnt;		/* number of tuples queried with unique keys */
static unsigned long mergetplcnt;	/* number of tuples merged */
static unsigned long deltplcnt;		/* number of tuples deleted (replaced) */
static unsigned long tplcnt;		/* number of tuples in the updated output table */

static char RECnumfmt[8];		/* print format for REC# in RECnumbuf[] */
static char recKnumfmt[8];		/* print format for rec# in recKnumbuf[] */
static char recMnumfmt[8];		/* print format for rec# in recMnumbuf[] */
static char recQnumfmt[8];		/* print format for rec# in recQnumbuf[] */
static char RECnumbuf[SPECIALBUFLEN];	/* buffer to print REC# (output table rec#) */
static char recKnumbuf[SPECIALBUFLEN];	/* buffer to print recK# (unique key rec#) */
static char recMnumbuf[SPECIALBUFLEN];	/* buffer to print recM# (merge rec#) */
static char recQnumbuf[SPECIALBUFLEN];	/* buffer to print recQ# (query rec#) */
static char recnumlen;			/* precision (length) for rec# */
static char seeknumlen;			/* precision (length) for seek# */
static char *attorvals[MAXATT];		/* verbose attribute name for pseudo attributes */
static char **dattrvals[2];		/* destination attrvals for retrieved tuples */
static char *wspbuf;			/* buffer for white space needed for fixed width attrs */
static char *prwbuf[2];			/* buffer for printing fixed width attribute values */
static char prbufsel;			/* flag to indicate which print buffer is selected for use */
static short *Rec2attr;			/* list of (output) attributes that reference REC# */
static short RecAttrCount;		/* number of (output attributes that reference REC# */

static char noop = FALSE;		/* do not modify output table */
static char quiet = FALSE;
static char replace = FALSE;		/* replace current (position) of matching tuples */
static char caseless = FALSE;		/* add caseless sort modifier where appropriate */
static char rmblanks = FALSE;		/* add rmblanks sort modifier where appropriate */
static char zerowidth = TRUE;		/* include zero width attributes */
static char create_descr = FALSE;	/* create description with output table */
static char tbl_exists = FALSE;		/* set if output table already exists */
static char key_match_err = FALSE;	/* return error if keys match and not doing a replace */
static char key_nomatch_err = FALSE;	/* return error if no matching key and doing a replace */
static char prtExit4err = TRUE;		/* print exit code 4 (-E option) error messages */
static char prtExit5err = TRUE;		/* print exit code 5 (-e or -m) error messages */
static char prtExit6err = TRUE;		/* print exit code 6 (rec# seek) error messages */
static char key_compare = FALSE;	/* attributes to be compared are keys (i.e., unique) */
static unsigned short Unique = 0;	/* Check for uniqueness when merging tuples */
static unsigned short sort_keys;	/* number of key sort attributes */
static long merge_recnum = -1;		/* merge tuples based on rec# in output table */
static short proj2attr[MAXATT];		/* mapping of projection index to query output attr# */

static char flddelim = '\0';
static char flddelim_set = FALSE;
static short delimiteropt = 0;
static char termdelimiter[256];
static char notdelimiter = FALSE;
static char widthdelimiter = FALSE;
static short debug = 0;

static int exit_code;

static void
usage( )
{
	prmsg( MSG_USAGE, "[-cEinqRsuUVz] [-d<delim>] [-P<ExitCode>] \\" );
	prmsg( MSG_CONTINUE, "[-M {blanks|caseless}] [-Q[<ErrorLimit>]] \\" );
	prmsg( MSG_CONTINUE, "[{-e|-m} RecordCount] [-r[{K|M|Q}#]<RecordLimit>] \\" );
	prmsg( MSG_CONTINUE, "[-F[REC#]<precision>] [-f[{K|M|Q|r|s}#]<precision>]\\" );
	prmsg( MSG_CONTINUE, "[<attr>[:<modifiers>...] [as <newattr>] ...] \\" );
	prmsg( MSG_CONTINUE, "[{keyed|sorted} [by <attr>[:<modifiers>...] ... [ALL]]] \\" );
	prmsg( MSG_CONTINUE, "from <table>[=<alt_table>]... in <table>[=<alt_table>] \\" );
	prmsg( MSG_CONTINUE, "[{keyed|sorted} [by {<attr>[:<modifiers>...] ... | rec# <recnum>}]] \\" );
	prmsg( MSG_CONTINUE, "[where <where-clause>]" );

	exit( 1 );
}

set_sig( sig, func )
int sig;
RETSIGTYPE (*func)();
{
	if ( signal( sig, SIG_IGN ) != (RETSIGTYPE (*)())SIG_IGN )
		(void)signal( sig, func );
}

RETSIGTYPE
catch_sig( sig )
int sig;
{
	(void)signal( sig, SIG_IGN );	/* ignore any more signals */

	(void)end_peruse( (struct uperuseinfo *)NULL, FALSE );

	if ( sig == SIGHUP || sig == SIGINT || sig == SIGTERM || sig == SIGQUIT ) {
		prmsg( MSG_ALERT, "killed by signal (%d) -- no alterations done",
			sig );

		if ( sig != SIGQUIT )
			exit( 3 );
	}

	(void)signal( sig, SIG_DFL );	/* do the normal operation */
	kill( getpid( ), sig );

	exit( 2 );	/* if signal comes quickly we'll never get here */
}

static
init_map( query, relptr, mapptr )
register struct uquery *query;
register struct urelation *relptr;
struct mattrinfo **mapptr;
{
	register int	i;
	register int	j;
	register struct mattrinfo *mapinfo;
	int		nodisplay;
	unsigned	buflen;
	unsigned	len;
	short		rec2attr[MAXATT];

	/*
	 * Allocate memory for mapping table.
	 */
	dattrvals[0] = (char **)calloc( (unsigned)relptr->attrcnt, sizeof( char * ) );
	dattrvals[1] = (char **)calloc( (unsigned)relptr->attrcnt, sizeof( char * ) );
	mapinfo = (struct mattrinfo *)calloc( (unsigned)relptr->attrcnt, sizeof(struct mattrinfo) );
	if (( dattrvals[0] == (char **)NULL ) ||
	    ( dattrvals[1] == (char **)NULL ) ||
	    ( mapinfo == (struct mattrinfo *)NULL )) {
		set_uerror( UE_NOMEM );
		return( FALSE );
	} else {
		*mapptr = mapinfo;
	}

	/*
	 * Initialize table for mapping source to
	 * destination attributes to be merged.
	 */
	buflen = 0;		/* length of buffer required to print fixed width attrs */
	len = 0;		/* maximum fixed width attribute length */
	RecAttrCount = 0;	/* number of attributes that reference REC# */
	for ( i = 0; i < (int) relptr->attrcnt; i++ )
	{
		mapinfo[i].dattridx = -1;
		mapinfo[i].destval = NULL;
		mapinfo[i].sattrnum = -1;
		mapinfo[i].special = '\0';
		if ( relptr->attrs[i].attrtype == UAT_FIXEDWIDTH ) {
			mapinfo[i].width = relptr->attrs[i].terminate;
			if ( mapinfo[i].width > len ) {
				len = mapinfo[i].width;
			}
			buflen += mapinfo[i].width + 1;
		} else {
			mapinfo[i].width = 0;
		}
	}

	/*
	 * Allocate buffer of white space for fixed width
	 * fields that do not have a source attribute.
	 */
	if ( len ) {
		wspbuf = calloc( (unsigned)(len + 1), sizeof( char ) );
		if ( wspbuf == NULL ) {
			set_uerror( UE_NOMEM );
			return( FALSE );
		}
		for ( i = 0; i < len; i++ ) {
			wspbuf[i] = ' ';
		}
		wspbuf[i] = '\0';
	}

	/*
	 * Map source to destination attributes
	 * that are not marked nodisplay.
	 */
	for ( nodisplay = i = 0; i < (int) query->attrcnt; i++ )
	{
		if ( ( query->attrlist[i].flags & QP_NODISPLAY ) == 0 )
		{
			j = findattr( relptr->attrs, relptr->attrcnt, query->attrlist[i].prname );
			if ( ( j < 0 ) || ( mapinfo[j].sattrnum != -1 ) ) {
				set_uerror( UE_UNRECATTR );
				return( FALSE );
			}
			mapinfo[j].sattrnum = i - nodisplay;
			mapinfo[i-nodisplay].dattridx = j;	/* reverse index */
			if ( query->attrlist[i].attr < 0 )
			{
				if ( query->attrlist[i].attorval == RECnumbuf ) {
					mapinfo[j].special = 'R';	/* REC# */
					rec2attr[RecAttrCount++] = j;
				} else if (query->attrlist[i].attorval == recKnumbuf ) {
					mapinfo[j].special = 'K';	/* recK# */
				} else if (query->attrlist[i].attorval == recMnumbuf ) {
					mapinfo[j].special = 'M';	/* recM# */
				} else if (query->attrlist[i].attorval == recQnumbuf ) {
					mapinfo[j].special = 'Q';	/* recQ#  */
				} else if (query->attrlist[i].attr == ATTR_RECNUM ) {
					/* rec# from query */
					mapinfo[j].special = 'r';
				} else  {
					/* seek# from query */
					mapinfo[j].special = 's';
				}
				if ( mapinfo[j].width == 0 ) {
					buflen += SPECIALBUFLEN; /* buffer length for rec# or seek# */
				}
			}
		}
		else
		{
			++nodisplay;
		}
	}

	/*
	 * Initialize destination value for all attributes
	 * are not being mapped from a source attribute.
	 */
	for ( j = 0; j < (int) relptr->attrcnt; j++ )
	{
		if ( mapinfo[j].sattrnum == -1 ) {
			if ( mapinfo[j].width ) {
				mapinfo[j].destval = &wspbuf[ len - mapinfo[j].width ];
				buflen -= mapinfo[j].width + 1;
			} else {
				mapinfo[j].destval = "";
			}
		}
	}

	if ( RecAttrCount ) {
		/*
		 * Allocate memory for Rec2attr[] list.
		 */
		Rec2attr = (short *)calloc( (unsigned)RecAttrCount, sizeof( short ) );
		if ( Rec2attr == (short *)NULL ) {
			set_uerror( UE_NOMEM );
			return( FALSE );
		}
		/*
		 * Copy rec2attr[] to Rec2attr[]
		 */
		for ( i = 0; i < RecAttrCount; i++ ) {
			Rec2attr[i] = rec2attr[i];
		}
	}

	if ( buflen ) {
		/*
		 * Allocate memory to toggle between two different buffers
		 * for copy fixed-width or special attribute values.
		 */
		prbufsel = 0;
		prwbuf[0] = calloc( buflen, sizeof( char ) );
		prwbuf[1] = calloc( buflen, sizeof( char ) );
		if (( prwbuf[0] == NULL ) || ( prwbuf[1] == NULL )) {
			set_uerror( UE_NOMEM );
			return( FALSE );
		}
	}

	return( TRUE );
}

static void
map_output( sattrvals, dattrvals, dattrcnt, prwbuf, mapinfo )
char	**sattrvals;
char	**dattrvals;
unsigned dattrcnt;
char	*prwbuf;
register struct mattrinfo *mapinfo;
{
	register int	i;
	register int	j;
	register unsigned short width;
	register char	*p;

	p = prwbuf;

	for ( i = 0; i < (int) dattrcnt; i++ )
	{
		j = mapinfo[i].sattrnum;

		if ( j >= 0 ) {
			if ( ( width = mapinfo[i].width ) != 0 ) {
				dattrvals[i] = p;
				if ( ( recnumlen > 0 ) && ( mapinfo[i].special == 'r' ) )
				{
					int	len;
					char	buf[SPECIALBUFLEN];
					len = recnumlen - strlen( sattrvals[ j ] );
					if ( ( len < 0 ) || ( len >= SPECIALBUFLEN ) ) {
						len = 0;
					}
					strncpy( &buf[len], sattrvals[ j ], SPECIALBUFLEN - len );
					while ( len > 0 ) {
						buf[--len] = '0';
					}
					sprintf( p, "%-*.*s", width, width, buf );
				}
				else if ( ( seeknumlen > 0 ) && ( mapinfo[i].special == 's' ) )
				{
					int	len;
					char	buf[SPECIALBUFLEN];
					len = seeknumlen - strlen( sattrvals[ j ] );
					if ( ( len < 0 ) || ( len >= SPECIALBUFLEN ) ) {
						len = 0;
					}
					strncpy( &buf[len], sattrvals[ j ], SPECIALBUFLEN - len );
					while ( len > 0 ) {
						buf[--len] = '0';
					}
					sprintf( p, "%-*.*s", width, width, buf );
				}
				else
				{
					sprintf( p, "%-*.*s", width, width, sattrvals[ j ] );
				}
				p += width + 1;
			} else if ( mapinfo[i].special ) {
				dattrvals[i] = p;
				if ( ( recnumlen > 0 ) && ( mapinfo[i].special == 'r' ) )
				{
					int len;
					len = recnumlen - strlen( sattrvals[ j ] );
					if ( ( len < 0 ) || ( len >= SPECIALBUFLEN ) ) {
						len = 0;
					}
					strncpy( &p[len], sattrvals[ j ], SPECIALBUFLEN - len );
					while ( len > 0 ) {
						p[--len] = '0';
					}
				}
				else if ( ( seeknumlen > 0 ) && ( mapinfo[i].special == 's' ) )
				{
					int len;
					len = seeknumlen - strlen( sattrvals[ j ] );
					if ( ( len < 0 ) || ( len >= SPECIALBUFLEN ) ) {
						len = 0;
					}
					strncpy( &p[len], sattrvals[ j ], SPECIALBUFLEN - len );
					while ( len > 0 ) {
						p[--len] = '0';
					}
				} else
				{
					strncpy( p, sattrvals[ j ], SPECIALBUFLEN );
				}
				p[ SPECIALBUFLEN - 1 ] = '\0';
				p += SPECIALBUFLEN;
			} else {
				dattrvals[i] = sattrvals[ j ];
			}
		} else {
			dattrvals[i] = mapinfo[i].destval;
		}
	}
}

static char **
readnexttuple( )
{
	register int i, j;
	register struct qprojection *projptr;
	char **nattrvals;
	char **oattrvals;
	char *prbuf;
	char *sattrvals[MAXATT];

	if ( retrievelimit ) {
		if ( ( retrieveKlimit ) && ( keytplcnt >= retrieveKlimit ) ) {
			return( (char **)NULL );
		}
		if ( ( retrieveMlimit ) && ( mergetplcnt >= retrieveMlimit ) ) {
			return( (char **)NULL );
		}
		if ( ( retrieveQlimit ) && ( querytplcnt >= retrieveQlimit ) ) {
			return( (char **)NULL );
		}
	}

	/*
	 * Update pseudo attributes REC# ...
	 * before mapping query attributes to
	 * output table attributes.
	 */
	sprintf( RECnumbuf, RECnumfmt, (unsigned long)(tplcnt + 1));
	sprintf( recKnumbuf, recKnumfmt, (unsigned long)(keytplcnt + 1) );
	sprintf( recMnumbuf, recMnumfmt, (unsigned long)(mergetplcnt + 1) );

	if ( ( sort_keys ) && ( querytplcnt ) ) {
		oattrvals = dattrvals[ prbufsel & 1 ];
	} else {
		oattrvals = (char **)NULL;
	}
	prbufsel = ( prbufsel + 1 ) & 1;
	nattrvals = dattrvals[ prbufsel & 1 ];
	prbuf = prwbuf[ prbufsel & 1 ];

	while ( nexttuple( query, &result, sattrvals ) )
	{
		++querytplcnt;
		sprintf( recQnumbuf, recQnumfmt, (unsigned long)(querytplcnt) );

		map_output( sattrvals, nattrvals, relinfo.attrcnt, prbuf, mapinfo );

		if ( oattrvals )
		{
			for ( i = 0; ( i < (int) sort_keys ) && ( oattrvals ); i++ )
			{
				j = query->attrlist[i].sortattr;

				projptr = &query->attrlist[j];

				if ( ( proj2attr[j] >= 0 ) &&
				   ( ( projptr->flags & QP_NEWVALUE ) == 0 ) )
				{
					j = mapinfo[proj2attr[j]].dattridx;

					if ( cmp_attrval( projptr, oattrvals[j], nattrvals[j], TRUE ) ) {
						oattrvals = (char **)NULL;
					}
				}
			}
		}

		if ( ! oattrvals )
		{
			++keytplcnt;
			return( nattrvals );
		}
	}

	return( (char **)NULL );
}

static
save_original( tuplecnt )
register unsigned long tuplecnt;
{
	struct utuple tpl;
	char *sattrvals[MAXATT];

	tpl.tplval = sattrvals;

	while( peruse_tuple( perptr, &tpl ) )
	{
		++origtplcnt;

		if ( ! savetuple( perptr, sattrvals ) )
		{
			(void)pruerror( );
			prmsg( MSG_ERROR, "failure saving tuple" );
			return( FALSE );
		}

		++tplcnt;

		if ( tuplecnt )
		{
			--tuplecnt;
			if ( tuplecnt == 0 )
			{
				return( TRUE );
			}
		}
	}

	if ( uerror != UE_NOERR )
	{
		(void)pruerror( );
		prmsg( MSG_ERROR, "failure reading tuple from orignal table" );
		return( FALSE );
	}
	return( TRUE );
}

static int
append_output( )
{
	char **sattrvals;

	while ( sattrvals = readnexttuple( ) )
	{
		++mergetplcnt;
		if ( perptr ) {
			if ( ! savetuple( perptr, sattrvals ) )
			{
				(void)pruerror( );
				prmsg( MSG_ERROR, "failure saving query tuple# %lu as rec# %lu in output table",
					querytplcnt, (unsigned long)(tplcnt+1) );
				return( FALSE );
			}
		}
		++tplcnt;
	}
	if ( uerror != UE_NOERR )
	{
		(void)pruerror( );
		prmsg( MSG_ERROR, "failure reading next query result tuple" );
		return( FALSE );
	}

	return( TRUE );
}

static int
merge_tuples( )
{
	register int i, j, rc;
	register struct qprojection *projptr;
	int	eof;
	char	**nattrvals;
	char	**sattrvals;
	struct utuple tpl;
	char	*oattrvals[MAXATT];

	tpl.tplval = oattrvals;

	eof = FALSE;	/* not end of original tuples */

	if ( ! peruse_tuple( perptr, &tpl ) ) {
		if ( uerror != UE_NOERR )
		{
			(void)pruerror( );
			prmsg( MSG_ERROR, "failure reading tuple from orignal table" );
			return( FALSE );
		}
		eof = TRUE;
	} else {
		++origtplcnt;
		eof = FALSE;    /* not end of original tuples */
	}

	if ( ! ( nattrvals = readnexttuple( ) ) ) {
		if ( uerror != UE_NOERR )
		{
			(void)pruerror( );
			prmsg( MSG_ERROR, "failure reading next query result tuple" );
			return( FALSE );
		}
	}

	while ( nattrvals && ! eof )
	{
		for ( rc = i = 0; i < (int) usort->sortcnt && rc == 0; i++ )
		{
			projptr = &usort->attrlist[usort->attrlist[i].sortattr];
			j = projptr->attr;
			rc = cmp_attrval( projptr, oattrvals[j], nattrvals[j], TRUE );
		}

		if ( rc == 0 ) {
			if ( key_compare ) {
				if ( replace ) {
					sattrvals = nattrvals;
					++deltplcnt;
					++mergetplcnt;
				} else {
					if ( key_match_err ) {
						if ( prtExit4err ) {
							prmsg( MSG_ERROR, "duplicate key at rec# %lu in original table", origtplcnt );
						}
						exit_code = 4;
						return( FALSE );
					}
					sattrvals = oattrvals;
				}
			} else {
				if ( replace ) {
					sattrvals = nattrvals;
					++mergetplcnt;
					rc = 1;
				} else {
					sattrvals = oattrvals;
					rc = -1;
				}
			}
		} else {
			if ( rc > 0 ) {
				if ( ( key_nomatch_err ) && ( key_compare ) && ( replace ) ) {
					if ( prtExit4err ) {
						prmsg( MSG_ERROR, "no matching key for rec# %lu from input query", querytplcnt );
					}
					exit_code = 4;
					return( FALSE );
				}
				sattrvals = nattrvals;
				++mergetplcnt;
			} else {
				sattrvals = oattrvals;
			}
		}

		/*
		 * save selected tuple
		 */
		if ( ! savetuple( perptr, sattrvals ) )
		{
			(void)pruerror( );
			prmsg( MSG_ERROR, "failure saving tuple" );
			return( FALSE );
		}
		++tplcnt;

		if ( rc <= 0 )
		{
			/*
			 * read next tuple from original table
			 */
			if ( peruse_tuple( perptr, &tpl ) )
			{
				++origtplcnt;
			} else {
				if ( uerror != UE_NOERR )
				{
					(void)pruerror( );
					prmsg( MSG_ERROR, "failure reading tuple from orignal table" );
					return( FALSE );
				}
				eof = TRUE;
			}
		}

		if ( rc >= 0 )
		{
			/*
			 * read next tuple retrieved by the query
			 */
			if ( ! ( nattrvals = readnexttuple( ) ) ) {
				if ( uerror != UE_NOERR )
				{
					(void)pruerror( );
					prmsg( MSG_ERROR, "failure reading next query result tuple" );
					return( FALSE );
				}
			}
		}
		else
		{
			/*
			 * update pseudo attribute REC# if referenced by
			 * the tuple that is still waiting to be merged.
		 	*/
			if ( RecAttrCount )
			{
				register char *p;
				register struct mattrinfo *map_ptr;

				sprintf( RECnumbuf, RECnumfmt, (unsigned long)(tplcnt + 1) );

				for ( i = 0; i < RecAttrCount; i++ )
				{
					j = Rec2attr[i];
					p = nattrvals[j];
					map_ptr = &mapinfo[j];
					if ( map_ptr->width ) {
						sprintf( p, "%-*.*s",
							map_ptr->width,
							map_ptr->width,
							RECnumbuf );
					} else {
						strcpy( p, RECnumbuf );
					}
				}
			}

		}
	}

	if ( nattrvals )
	{
		if ( ( key_nomatch_err ) && ( key_compare ) && ( replace ) ) {
			if ( prtExit4err ) {
				prmsg( MSG_ERROR, "no matching key for rec# %lu from input query", querytplcnt );
			}
			exit_code = 4;
			return( FALSE );
		}

		while ( nattrvals )
		{
			/*
			 * merge new tuple
			 */
			++mergetplcnt;
			if ( ! savetuple( perptr, nattrvals ) )
			{
				(void)pruerror( );
				prmsg( MSG_ERROR, "failure saving tuple" );
				return( FALSE );
			}
			++tplcnt;
	
			if ( ! ( nattrvals = readnexttuple( ) ) ) {
				if ( uerror != UE_NOERR )
				{
					(void)pruerror( );
					prmsg( MSG_ERROR, "failure reading next query result tuple" );
					return( FALSE );
				}
			}
		}
	}

	if ( ! eof )
	{
		unsigned long tuplecnt;

		if ( ! savetuple( perptr, oattrvals ) )
		{
			(void)pruerror( );
			prmsg( MSG_ERROR, "failure saving tuple" );
			return( FALSE );
		}
		++tplcnt;

		/*
		 * save any unread tuples from the original table
		 */
		tuplecnt = 0;
		if ( ! stop_save_tc( perptr, TRUE, &tuplecnt) )
		{
			(void)pruerror( );
			prmsg( MSG_ERROR, "failure saving unread tuples from original table" );
			return( FALSE );
		}
		origtplcnt += tuplecnt;
		tplcnt += tuplecnt;
	}

	return( TRUE );
}

static int
merge_recnums( )
{
	int	eof;
	char	**nattrvals;
	unsigned long tuplecnt;
	struct utuple tpl;
	char	*oattrvals[MAXATT];

	tpl.tplval = oattrvals;

	eof = FALSE;	/* not end of original tuples */

	while ( nattrvals = readnexttuple( ) )
	{
		if ( ! eof ) {
			if ( peruse_tuple( perptr, &tpl ) )
			{
				++origtplcnt;
			}
			else
			{
				if ( uerror != UE_NOERR )
				{
					(void)pruerror( );
					prmsg( MSG_ERROR, "failure reading tuple from orignal table" );
					return( FALSE );
				}
				eof = TRUE;
			}
		}

		if ( ( eof ) || ( replace ) )
		{
			if ( ! eof ) {
				++deltplcnt;
			} else {
				if ( ( key_nomatch_err ) && ( replace ) && ( key_compare ) ) {
					if ( prtExit4err ) {
						prmsg( MSG_ERROR, "no matching key for rec# %lu from input query", querytplcnt );
					}
					exit_code = 4;
					return( FALSE );
				}
			}
			++mergetplcnt;
			if ( ! savetuple( perptr, nattrvals ) )
			{
				(void)pruerror( );
				prmsg( MSG_ERROR, "failure saving tuple" );
				return( FALSE );
			}
		} else {
			if ( ! eof ) {
				if ( ( key_match_err ) && ( key_compare ) ) {
					if ( prtExit4err ) {
						prmsg( MSG_ERROR, "duplicate key at rec# %lu in original table", origtplcnt );
					}
					exit_code = 4;
					return( FALSE );
				}
				if ( ! savetuple( perptr, oattrvals ) )
				{
					(void)pruerror( );
					prmsg( MSG_ERROR, "failure saving tuple" );
					return( FALSE );
				}
			}
		}
		++tplcnt;
	}
	if ( uerror != UE_NOERR )
	{
		(void)pruerror( );
		prmsg( MSG_ERROR, "failure reading next query result tuple" );
		return( FALSE );
	}

	if ( ! eof )
	{
		/*
		 * save any unread tuples from the original table
		 */
		if ( ! stop_save_tc( perptr, TRUE, &tuplecnt) )
		{
			(void)pruerror( );
			prmsg( MSG_ERROR, "failure saving unread tuples from original table" );
			return( FALSE );
		}
		origtplcnt += tuplecnt;
		tplcnt += tuplecnt;
	}

	return( TRUE );
}

static void
set_attr_all( projptr, nodeptr )
register struct qprojection *projptr;
register struct qnode *nodeptr;
{
	projptr->rel = nodeptr;
	projptr->attr = ATTR_ALL;
	projptr->flags = 0;
	projptr->priority = 0xffff;
	projptr->sortattr = 0;
	projptr->attorval = NULL;
	projptr->delim = NULL;
	projptr->subcnt = 0;
	projptr->prwidth = 0;
	projptr->prname = NULL;
	projptr->justify = '\0';
	projptr->terminate = '\0';
	projptr->attrwidth = 0;
	projptr->attrtype = -1;
}

static int
findsortatt( projptr, projcnt, sortinfo )
struct qprojection *projptr;
int projcnt;
struct qprojection *sortinfo;
{
	int i, j;
	int rc;

	rc = -1;

	for( i = 0; i < projcnt; i++, projptr++ )
	{
		if ( ( projptr->rel == sortinfo->rel ) &&
		     ( projptr->attr == sortinfo->attr ) &&
		   ((( projptr->flags & QP_NEWVALUE) == 0 ) ||
		     ( projptr->attorval == NULL )))
		{
			/*
			 * Found the attribute.
			 * Now we must check that it
			 * has not already been used for sorting.
			 */
			if ( projptr->priority < sortinfo->priority )
			{
				/*
				 * This attribute as already been used for sorting.
				 * If the attribute is to be displayed then update
				 * the return code to indicate that the attribute
				 * was found but it was already marked for sorting.
				 */
				if ( ! ( projptr->flags & QP_NODISPLAY ) )
					rc = -2;
				continue;
			}
			/*
			 * Found this attribute.  Merge the sort info.
			 * Do not set or clear QP_NODISPLAY or any other
			 * flag bits that are not related to sorting.
			 */
			projptr->flags = ( ( projptr->flags & ~(QP_SORTMASK|QP_DESCENDING|QP_RMBLANKS) ) |
					( QP_SORT ) |
					( sortinfo->flags & (QP_SORTMASK|QP_DESCENDING|QP_RMBLANKS) ) );
			projptr->subcnt = sortinfo->subcnt;
			projptr->priority = sortinfo->priority;
			projptr->delim = sortinfo->delim;
			for( j = 0; j < (int) sortinfo->subcnt; j++ )
				projptr->subsort[j] = sortinfo->subsort[j];

			/*
			 * If this attribute is marked nodisplay
			 * but is projected to be displayed somewhere
			 * and is not the special ``all'' attribute
			 * then we need to indicate that any uniqueness
			 * checks should not skip this attribute even
			 * though it is marked nodisplay.
			 */
			if ( ( projptr->flags & QP_NODISPLAY ) &&
			     ( projptr->attr != ATTR_ALL ) ) {
				if ( rc == -2 ) {
					projptr->flags |= QP_SORTUNIQ;
				} else {
					struct qprojection *saveqptr = projptr;

					for ( ++projptr, j = i + 1; j < projcnt; j++, projptr++ )
					{
						if ( ( projptr->rel == sortinfo->rel ) &&
						     ( projptr->attr == sortinfo->attr ) &&
						    (( projptr->flags & QP_NODISPLAY ) == 0 ) &&
						   ((( projptr->flags & QP_NEWVALUE ) == 0 ) ||
						     ( projptr->attorval == NULL )))
						{
							saveqptr->flags |= QP_SORTUNIQ;
							break;
						}
					}
				}
			}

			return( i );
		}
	}

	return( rc );
}

/*
 * Customized version of fmkquery(3)
 */

static struct uquery *
makequery( flags, relnames, relcnt, attrnames, anamecnt, sortlist, sortcnt, where, wherecnt )
int flags;
char **relnames;
int relcnt;
char **attrnames;
int anamecnt;
char **sortlist;
int sortcnt;
char **where;
int wherecnt;
{
	register int i;
	struct urelation *rellist;	/* relation information */
	struct qnode *nodelist;		/* node space for query */
	struct queryexpr *qptr;		/* query expr. from where-clause */
	struct uquery *query;		/* compiled version of query */
	char seen_stdin;		/* seen '-' in relnames? */
	int attrcnt;			/* attribute count */
	int thisattr;			/* current attribute name index */
	int lastsort;			/* last sorted attribute index */
	unsigned short sort_priority;	/* priority of each sorted attr */
	short all_att_first[MAXRELATION];	/* first attr to replace "all" */
	short all_att_last[MAXRELATION];	/* last attr + 1 to replace "all" */
	char all_sorted[MAXRELATION];		/* "all" used in sorted by clause */
	struct qprojection attrlist[MAXATT];	/* projected attributes */

	if ( relcnt <= 0 )	/* Gotta have a relation */
	{
		prmsg( MSG_ERROR, "no relations given" );
		return( NULL );
	}

	if ( relcnt > MAXRELATION )	/* Too many relations */
	{
		prmsg( MSG_ERROR, "too many relations given - maximum of %d relations", MAXRELATION );
		return( NULL );
	}

	nodelist = (struct qnode *)calloc( (unsigned)relcnt, sizeof( *nodelist ) );
	rellist = (struct urelation *)calloc( (unsigned)relcnt, sizeof( *rellist));
	if ( nodelist == NULL || rellist == NULL )
	{
		prmsg( MSG_INTERNAL, "cannot allocate space for relation information" );
		if ( rellist )
			free( rellist );
		if ( nodelist )
			free( nodelist );
		return( NULL );
	}

	/*
	 * Get all the information about each relation.  Allow only one
	 * reference to stdin in the list.  Relation information is also
	 * plugged into each query node for the query routines.
	 */
	seen_stdin = FALSE;
	for( i = 0; i < relcnt; i++ )
	{
		all_att_first[i] = -1;
		all_att_last[i] = -1;
		all_sorted[i] = FALSE;

		if ( ! getrelinfo( relnames[i], &rellist[i], FALSE ) )
		{
			(void)pruerror( );
			prmsg( MSG_ERROR, "cannot get descriptor info for table '%s'",
				relnames[i] );
			free( rellist );
			free( nodelist );
			return( NULL );
		}

		if ( strcmp( rellist[i].path, "-" ) == 0 )
		{
			if ( seen_stdin )
			{
				prmsg( MSG_ERROR, "only one standard input relation is allowed" );
				free( rellist );
				free( nodelist );
				return( NULL );
			}
			seen_stdin = TRUE;
		}

		nodelist[i].rel = &rellist[i];	/* save relation in qnode */
		nodelist[i].nodenum = (unsigned char) i;   /* save relidx */
	}

	/*
	 * If there was a list of attributes to project, find them and 
	 * make sure they are all legit.  If there were no attributes listed,
	 * list all attributes from all relations.
	 */
	if ( anamecnt > 0 )
	{
		/* Look up each attribute. */
		register struct qprojection *projptr;
		register char	*p;
		char	pseudo_attr;

		projptr = attrlist;
		for( i = 0, attrcnt = 0; i < anamecnt; i++ )
		{
			thisattr = i;
			p = attrnames[i];

			if ( *p == ':' )
			{
				prmsg( MSG_ERROR, "modifier '%s' must be prefixed by an attribute name", p );
				free( rellist );
				free( nodelist );
				return( NULL );
			}

			if ( attrcnt >= MAXATT )
			{
				prmsg( MSG_ERROR, "too many projected attributes specified when making query -- max is %d",
					MAXATT );
				free( rellist );
				free( nodelist );
				return( NULL );
			}

			if ((( p[0] == 'R' ) &&
			     ( p[1] == 'E' ) &&
			     ( p[2] == 'C' ) &&
			     ( p[3] == '#' ) &&
			     ( p[4] == '\0' || p[4] == ':' )) ||
			    (( p[0] == 'r' ) &&
			     ( p[1] == 'e' ) &&
			     ( p[2] == 'c' ) &&
			     ( p[3] == 'K' || p[3] == 'M' || p[3] == 'Q' ) &&
			     ( p[4] == '#' ) &&
			     ( p[5] == '\0' || p[5] == ':' )))
			{
				if ( p[0] == 'R' ) {
					/* REC# */
					pseudo_attr = 'R';
					p += 4;
				} else {
					/* recX# */
					pseudo_attr = p[3];
					p += 5;
				}

				/* use 1.rec# as a place holder for pseudo attributes */
				if ( ! lookupprojattr( "1.rec#", nodelist, relcnt, projptr, FALSE ) )
				{
					prmsg( MSG_INTERNAL, "unrecognized projected attribute name '1.rec#'" );
					free( rellist );
					free( nodelist );
					return( NULL );
				}
				if ( ( *p == ':' ) &&
				     ( ! attr_modifier( projptr, p, TRUE ) ) )
				{
					free( rellist );
					free( nodelist );
					return( NULL );
				}
			} else {

				pseudo_attr = '\0';

				if ( ! lookupprojattr( p, nodelist, relcnt, projptr, FALSE ) )
				{
					prmsg( MSG_ERROR, "unrecognized projected attribute name '%s'", p );
					free( rellist );
					free( nodelist );
					return( NULL );
				}
			}

			while ( ( i < anamecnt - 1 ) && ( attrnames[i+1][0] == ':' ) )
			{
				if ( ! attr_modifier( projptr, attrnames[++i], TRUE ) )
				{
					free( rellist );
					free( nodelist );
					return( NULL );
				}
			}

			if ( i < anamecnt - 1 )
			{
				p = attrnames[i+1];

				if ( ( p[0] == 'a' ) && ( p[1] == 's' ) && ( p[2] == '\0' ) )
				{
					i += 2;

					if ( i >= anamecnt )
					{
						prmsg( MSG_ERROR, "no new name given for attribute %s",
							attrnames[thisattr] );
						free( rellist );
						free( nodelist );
						return( NULL );
					}
					else if ( projptr->attr == ATTR_ALL )
					{
						prmsg( MSG_ERROR, "cannot rename the \"all\" attribute" );
						free( rellist );
						free( nodelist );
						return( NULL );
					}

					projptr->prname = attrnames[i];
				}
			}

			if ( ( pseudo_attr ) && ( ( projptr->flags & QP_NODISPLAY ) == 0 ) )
			{
				if ( strcmp( projptr->prname, "rec#" ) == 0 )
				{
					prmsg( MSG_ERROR, "no new name given for attribute %s", attrnames[thisattr] );
					free( rellist );
					free( nodelist );
					return( NULL );
				}
				/*
				 * If pseudo attribute was not assigned a value
				 * by the use then assign it to the appropriate
				 * pseudo attribute buffer.  As as result, it the
				 * user specified a friendly attribute name then
				 * we will use attorvals[] to (temporily) save the
				 * friendly attribute name since projptr->attorval
				 * must be used for the pseudo attribute buffer
				 * (value) which will also disable any attempt to
				 * sort on the pseudo attribute.
				 *
				 * Note: We are assuming that attrvals[] has been
				 *	 initialized to NULL for all entries so
				 *	 that (here) we only need to update it
				 *	 for the pseudo attributes.
				 */
				if ( ( projptr->flags & QP_NEWVALUE ) == 0 )
				{
					if ( ( projptr->attorval ) &&
					     ( projptr->flags & QP_NOVERBOSE ) ) {
						attorvals[attrcnt] = projptr->attorval;
					}

					projptr->flags |= ( QP_NEWVALUE | QP_NOVERBOSE );
					switch ( pseudo_attr ) {
					case 'R':
						projptr->attorval = RECnumbuf;
						break;
					case 'K':
						projptr->attorval = recKnumbuf;
						break;
					case 'M':
						projptr->attorval = recMnumbuf;
						break;
					case 'Q':
					default:
						projptr->attorval = recQnumbuf;
						break;
					}
				}
			}

			if ( projptr->attr == ATTR_ALL )
			{
				int  j;
				int  allcnt;
				int  relidx;
				char skipattr[MAXATT];
				struct qprojection saveattr;
				struct uattribute *attrptr;

				relidx = projptr->rel->nodenum;

				if ( all_att_first[ relidx ] != -1 )
				{
					prmsg( MSG_ERROR, "duplicate reference to \"all\" attribute" );
					free( rellist );
					free( nodelist );
					return( NULL );
				} else {
					all_att_first[ relidx ] = attrcnt;
					all_att_last[ relidx ] = attrcnt;
				}

				if ( ( projptr->flags & QP_NODISPLAY ) == QP_NODISPLAY ) {
					allcnt = 0;
				} else {
					allcnt = rellist[relidx].attrcnt;
				}

				for ( j = 0; j < allcnt; j++ ) {
					if ( ( zerowidth ) ||
					   ( ( rellist[relidx].attrs[j].prwidth != 0 ) &&
					     ( ! isupper( rellist[relidx].attrs[j].justify ) ) ) ) {
						skipattr[j] = FALSE;
					} else {
						skipattr[j] = TRUE;
					}
				}

				/* Check for all:nodisplay=attr(s) */
				if ( projptr->attorval != NULL )
				{
					char *colonptr, *p, *q;

					p = projptr->attorval;
					if ( ( colonptr = strchr( p, ':' ) ) != NULL ) {
						*colonptr = '\0';
					}
					while ( p )
					{
						if ( ( q = strchr( p, ',' ) ) != NULL ) {
							*q = '\0';
						}
						for ( j = 0, attrptr = rellist[relidx].attrs;
						      j < allcnt && j < MAXATT;
						      attrptr++, j++ )
						{
							if ( strcmp( p, attrptr->aname ) == 0 ) {
								skipattr[j] = TRUE;
								break;
							}
						}
						if ( q ) {
							*q = ',';
							p = q + 1;
						} else {
							p = NULL;
							break;
						}
					}
					if ( colonptr ) {
						*colonptr = ':';
					}
				}

				saveattr = *projptr;

				for ( j = 0, attrptr = rellist[relidx].attrs; j < allcnt; j++, attrptr++ )
				{
					if ( skipattr[j] )
						continue;

					if ( attrcnt >= MAXATT )
					{
						prmsg( MSG_ERROR, "too many projected attributes specified when making query -- max is %d",
							MAXATT );
						free( rellist );
						free( nodelist );
						return( NULL );
					}

					*projptr = saveattr;

					projptr->attr = j;
					projptr->prname = attrptr->aname;
					projptr->attorval = NULL;

					if ( projptr->prwidth == 0 )
						projptr->prwidth = attrptr->prwidth;
					if ( projptr->justify == '\0' )
						projptr->justify = attrptr->justify;

					if (( projptr->attrtype == -1 ) || ( projptr->attrtype >= 255 ))
					{
						if ( ( attrptr->attrtype == UAT_TERMCHAR ) ||
						     ( attrptr->terminate >= DBBLKSIZE ) ) {
							projptr->attrtype = QP_TERMCHAR;
							if ( j == rellist[relidx].attrcnt - 1 ) {
								projptr->terminate = '\t';
							} else {
								projptr->terminate = (char)attrptr->terminate;
							}
							projptr->attrwidth = 0;
						} else {
							projptr->attrtype = QP_FIXEDWIDTH;
							projptr->attrwidth = attrptr->terminate;
							projptr->terminate = '\0';
						}

					}
					projptr++;
					attrcnt++;
				}
				all_att_last[ relidx ] = attrcnt;
			} else {
				projptr++;
				attrcnt++;
			}

		} /* end for each projected attribute string */
	}
	else	/* project all attributes in all relations */
	{
		register struct uattribute *attrptr;
		register int j;

		attrcnt = 0;
		for( i = 0; i < relcnt; i++ )
		{
			for( j = 0, attrptr = rellist[i].attrs;
					j < (int) rellist[i].attrcnt;
					j++, attrptr++ )
			{
				if ( ( zerowidth ) ||
				   ( ( attrptr->prwidth != 0 ) && ( ! isupper( attrptr->justify ) ) ) )
				{
					if ( attrcnt >= MAXATT )
					{
						prmsg( MSG_ERROR, "too many projected attributes specified when making query -- max is %d",
							MAXATT );
						free( rellist );
						free( nodelist );
						return( NULL );
					}

					attrlist[attrcnt].flags = 0;
					attrlist[attrcnt].rel = &nodelist[i];
					attrlist[attrcnt].attr = j;
					attrlist[attrcnt].prwidth = attrptr->prwidth;
					attrlist[attrcnt].justify = attrptr->justify;
					attrlist[attrcnt].priority = 0xffff;
					attrlist[attrcnt].prname = (flags & Q_FRIENDLY) &&
							attrptr->friendly ?
						attrptr->friendly :
						attrptr->aname;
					if ( ( attrptr->attrtype == UAT_TERMCHAR ) ||
					     ( attrptr->terminate >= DBBLKSIZE ) ) {
						attrlist[attrcnt].terminate =
							j == rellist[i].attrcnt - 1 ?
							'\t' :
							(char)attrptr->terminate;
						attrlist[attrcnt].attrwidth = 0;
					} else {
						attrlist[attrcnt].attrtype = QP_FIXEDWIDTH;
						attrlist[attrcnt].attrwidth = attrptr->terminate;
						attrlist[attrcnt].terminate = '\0';
					}
					attrlist[attrcnt].attorval = NULL;
					attrcnt++;

				} /* end if print width != 0 */
			} /* end for each attribute */
		} /* end for each relation */

	} /* end else project all attributes in all relations */

	/*
	 * Now save the sorted attributes in the projection list.
	 */
	lastsort = -1;
	sort_priority = 0;
	for( i = 0; i < sortcnt; i++ )
	{
		register char *p;
		int	allcnt;
		int	relidx;
		char	pseudo_attr;
		struct qprojection sortattr;

		p = sortlist[i];

		if ( *p == ':' ) /* attr modifiers */
		{
			prmsg( MSG_ERROR, "no attribute name given before attribute modifiers in sort attributes" );
			free( rellist );
			free( nodelist );
			return( NULL );
		}

		pseudo_attr = '\0';

		if ( ( ( p[0] == 'A' ) && ( p[1] == 'L' ) && ( p[2] == 'L' ) ) &&
		     ( ( p[3] == '\0' ) || ( p[3] == ':' ) ) )
		{
			if ( ( strncmp( p, "ALL:nodisplay=", 14 ) == 0 ) ||
			     ( ! lookupprojattr( p, nodelist, relcnt, &sortattr, FALSE ) ) )
			{
				pseudo_attr = 'A';

				set_attr_all( &sortattr, &nodelist[0] );

				if ( ( p[3] ) && ( ! attr_modifier( &sortattr, &p[3], TRUE ) ) )
				{
					/* message already printed */
					free( rellist );
					free( nodelist );
					return( NULL );
				}
			}
		} else {
			if ( ! lookupprojattr( p, nodelist, relcnt, &sortattr, FALSE ) )
			{
				prmsg( MSG_ERROR, "unrecognized sort attribute name '%s'", p );
				free( rellist );
				free( nodelist );
				return( NULL );
			}
		}

		while( ( i + 1 < sortcnt ) && ( sortlist[i+1][0] == ':' ) )
		{
			p = sortlist[++i];
			if ( ! attr_modifier( &sortattr, p , TRUE ) )
			{
				free( rellist );
				free( nodelist );
				return( NULL );
			}
		}

		if ( sortattr.flags & QP_NEWVALUE )
		{
			prmsg( MSG_ERROR, "cannot sort attributes with ':value=' modifier" );
			free( rellist );
			free( nodelist );
			return( NULL );
		}

		/*
		 * Clear out any other flag bits
		 * that are not used for sorting
		 * including attorval which is set
		 * for all:nodisplay=...
		 */
		sortattr.flags &= (QP_SORTMASK|QP_DESCENDING|QP_RMBLANKS|QP_NODISPLAY|QP_SORT);
		sortattr.attorval = NULL;

		if ( sortattr.attr != ATTR_ALL )
		{
			/*
			 * Save the sort priority, independent of where
			 * the attribute appears in the projection list.
			 */
			sortattr.priority = sort_priority++;

			/*
			 * Merge the sort attribute info with the projected
			 * attribute info if this attribute was also
			 * projected; otherwise, we'll add a new "no-display"
			 * attribute.
			 */
			lastsort = findsortatt( attrlist, attrcnt, &sortattr );
			if ( lastsort < 0 )
			{
				/*
				 * This attribute was not projected or it has already
				 * been used as one of the sorted by attributes.
				 * Add it as an additional, non-displayed attribute.
				 */
				if ( attrcnt >= MAXATT )
				{
					prmsg( MSG_ERROR, "too many projected attributes specified when making query -- max is %d",
					MAXATT );
					free( rellist );
					free( nodelist );
					return( NULL );
				}

				attrlist[ attrcnt ] = sortattr;
				attrlist[ attrcnt ].flags |= QP_NODISPLAY|QP_SORT;

				/*
				 * If the attribute was projected (and to be displayed)
				 * then we need to indicate that this attribute must
				 * also be considered when checking for uniquess even
				 * though it is not to be displayed.
				 */
				if ( lastsort == -2 ) {
					attrlist[ attrcnt ].flags |= QP_SORTUNIQ;
				}

				lastsort = attrcnt;

				attrcnt++;
			}
		}
		else if ( pseudo_attr == 'A' )
		{
			register struct qprojection *projptr;
			int j, k;

			/*
			 * sort on all attributes not marked nodisplay
			 * which have not been used for sorting yet.
			 */
			for ( projptr = &attrlist[0], k = attrcnt; k != 0; k--, projptr++ )
			{
				if ( ( ( projptr->flags & QP_NODISPLAY ) == 0 ) &&
				     ( ( projptr->flags & QP_NEWVALUE ) == 0 ) &&
				       ( projptr->priority >= sort_priority ) )
				{
					/*
					 * Merge the sort info.
					 * Do not set or clear any flag bits
					 * that are not related to sorting.
					 */
					projptr->flags = ( ( projptr->flags & ~(QP_SORTMASK|QP_DESCENDING|QP_RMBLANKS) ) |
							( QP_SORT ) |
							( sortattr.flags & (QP_SORTMASK|QP_DESCENDING|QP_RMBLANKS) ) );
					projptr->subcnt = sortattr.subcnt;
					projptr->priority = sort_priority++;
					projptr->delim = sortattr.delim;
					for ( j = 0; j < (int) sortattr.subcnt; j++ )
						projptr->subsort[j] = sortattr.subsort[j];

				}
			}

			/*
			 * Note that the special attribute "all"
			 * is no longer a valid sort attribute
			 * for any relation used in the query.
			 */
			for ( relidx = 0; relidx < relcnt; relidx++ ) {
				all_sorted[relidx] = TRUE;
			}
		}
		else
		{
			int	attr;
			int	dolookup;

			/*
			 * Get relidx and expand "all" if not already expanded.
			 */
			relidx = sortattr.rel->nodenum;

			if ( all_sorted[relidx] == TRUE )
			{
				prmsg( MSG_ERROR, "duplicate reference to \"all\" in sorted by clause" );
				free( rellist );
				free( nodelist );
				return( NULL );
			}
			else
			{
				all_sorted[relidx] = TRUE;
			}

			if ( all_att_first[relidx] >= 0 )
			{
				/*
				 * Lookup previously expanded attributes
				 * that are included in the projection list.
				 */
				dolookup = TRUE;
				allcnt = all_att_last[relidx];
				attr = all_att_first[relidx];
			}
			else
			{
				/*
				 * "all" was not expanded previously
				 */
				dolookup = FALSE;
				allcnt = rellist[relidx].attrcnt;
				attr = 0;
			}

			while ( attr < allcnt )
			{
				if ( dolookup ) {
					sortattr.attr = attrlist[attr].attr;
				} else {
					sortattr.attr = attr;
				}

				/*
				 * Save the sort priority, independent of where
				 * the attribute appears in the projection list.
				 */
				sortattr.priority = sort_priority++;

				/*
				 * Merge the sort attribute info with the projected
				 * attribute info if this attribute was also
				 * projected; otherwise, we'll add a new "no-display"
				 * attribute.
				 */
				lastsort = findsortatt( attrlist, attrcnt, &sortattr );
				if ( lastsort < 0 )
				{
					/*
					 * This attribute was not projected or it has already
					 * been used as one of the sorted by attributes.
					 * Add it as an additional, non-displayed attribute.
					 */
					if ( attrcnt >= MAXATT )
					{
						prmsg( MSG_ERROR, "too many projected attributes specified when making query -- max is %d",
						MAXATT );
						free( rellist );
						free( nodelist );
						return( NULL );
					}

					attrlist[ attrcnt ] = sortattr;
					attrlist[ attrcnt ].flags |= QP_NODISPLAY|QP_SORT;

					/*
					 * If the attribute was projected (and to be displayed)
					 * then we need to indicate that this attribute must
					 * also be considered when checking for uniquess even
					 * though it is not to be displayed.
					 */
					if ( lastsort == -2 ) {
						attrlist[ attrcnt ].flags |= QP_SORTUNIQ;
					}

					lastsort = attrcnt;

					attrcnt++;
				}

				++attr;
			}
		}

		if ( ( sort_keys == 0xffff ) && ( sortcnt > i + 3 ) )
		{
			/*
			 * Check for list of sorted by attributes.
			 */
			p = sortlist[i+1];
			if ( ( p[0] == 's' ) &&
			     ( p[1] == 'o' ) &&
			     ( p[2] == 'r' ) &&
			     ( p[3] == 't' ) &&
			     ( p[4] == 'e' ) &&
			     ( p[5] == 'd' ) &&
			     ( p[6] == '\0' ) &&
			     ( sort_priority >= 1 ) &&
			     ( strcmp( sortlist[i+2], "by" ) == 0 ) )
			{
				sort_keys = sort_priority;
				i += 2;	/* adjust for "sorted" and "by" */
			}
		}
	} /* end for each sort attribute string */

	if ( ( sort_keys == 0xffff ) && ( sort_priority >= 1 ) ) {
		sort_keys = sort_priority;
	}

	if ( wherecnt > 0 )	/* Parse any where clause. */
	{
		qptr = fparsewhere( wherecnt, where, nodelist, relcnt, flags );
		if ( qptr == NULL )
		{
			(void)pruerror( );
			prmsg( MSG_ERROR, "unsuccessful parse for where-clause" );
			free( rellist );
			free( nodelist );
			return( NULL );
		}
	}
	else
		qptr = NULL;

	/*
	 * Build the query itself.  The attribute list will be
	 * malloc()'ed as part of expprojlist().  We don't have to
	 * worry that it's just space on the stack.
	 */
	query = fbldquery( nodelist, relcnt, attrlist, attrcnt, qptr, flags );
	if (  query == NULL )
	{
		(void)pruerror( );
		prmsg( MSG_ERROR, "query compilation failed" );
		free( rellist );
		free( nodelist );
		return( NULL );
	}

	return( query );
}

static struct uquery *
makeusort( relptr, sortlist, sortcnt, query, unique )
struct urelation *relptr;
char **sortlist;
int sortcnt;
struct uquery *query;
int unique;
{
	register struct qprojection *projptr;
	register char	*p;
	register int	i;
	int		j;
	int		queryflags;
	unsigned short	maxattr;
	unsigned short	projcnt;
	unsigned short	sort_priority;
	struct uquery	*usort;			/* output query for comparing tuples */
	struct qnode	*nodelist;		/* node space for query */
	struct qprojection attrlist[MAXATT];	/* projected attributes */
	struct qprojection sortattr;		/* attribute to be used for sorting */

	queryflags = Q_SORT;
	if ( unique )
		queryflags ^= Q_UNIQUE;
	sort_priority = 0;

	nodelist = (struct qnode *)calloc( (unsigned) 1, sizeof( *nodelist ) );
	if ( nodelist == NULL )
	{
		prmsg( MSG_INTERNAL, "cannot allocate space for relation information" );
		return( NULL );
	}

	nodelist[0].rel = relptr;

	maxattr = relptr->attrcnt;

	/* project all attributes in the relation */
	projptr = attrlist;
	for ( projcnt = 0, i = 0; i < (int) maxattr; i++ )
	{
		if ( ! lookupprojattr( relptr->attrs[i].aname, nodelist, 1, projptr, FALSE ) )
		{
			prmsg( MSG_INTERNAL, "unable to lookup projected attribute name '%s'" );
			free( nodelist );
			return( NULL );
		}
		projptr++;
		projcnt++;
	}

	if ( query )
	{		/* Sort by same attributes given for the query */
		for( i = 0; i < (int) query->sortcnt; i++ )
		{
			if ( ( sort_keys ) && ( i >= (int) sort_keys ) )
				break;	/* other sort attributes do not apply */

			projptr = &query->attrlist[ query->attrlist[i].sortattr ];

			if ( projptr->priority >= 0xffff )
				break;	/* do not include attributes added for uniqueness */

			if ( ( ! lookupprojattr( projptr->prname, nodelist, 1, &sortattr, FALSE ) ) ||
			     ( sortattr.attr < 0 ) )
			{
				prmsg( MSG_ERROR, "unrecognized tuple compare attribute name '%s'",
					projptr->prname );
				free( nodelist );
				return( NULL );
			}

			/*
			 * Copy sorting information but do not
			 * include any flag bits that are not
			 * used for sorting including attorval
			 * which is set for all:nodisplay=...
			 */
			sortattr.flags = (QP_SORTMASK|QP_DESCENDING|QP_RMBLANKS) & projptr->flags;
			sortattr.attorval = NULL;
			sortattr.subcnt = projptr->subcnt;
			sortattr.delim = projptr->delim;
			for ( j = 0; j < (int) sortattr.subcnt; j++ ) {
				sortattr.subsort[j] = projptr->subsort[j];
			}

			/*
			 * Merge the sort attribute info with the projected
			 * attribute info if this is the first time this
			 * attribute has been included in the list.
			 */
			if ( attrlist[ sortattr.attr ].priority > sort_priority )
			{
				projptr = &attrlist[ sortattr.attr ];

				projptr->flags = QP_SORT | sortattr.flags;
				projptr->subcnt = sortattr.subcnt;
				projptr->delim = sortattr.delim;
				for ( j = 0; j < (int) sortattr.subcnt; j++ )
					projptr->subsort[j] = sortattr.subsort[j];
			}
			else
			{
				if ( projcnt >= MAXATT ) {
					prmsg( MSG_ERROR, "too many projected attributes -- max is %d", MAXATT );
					free( nodelist );
					return( NULL );
				}

				attrlist[ projcnt ] = sortattr;
				projptr = &attrlist[ projcnt++ ];

				/*
				 * Mark attribute nodisplay but do not skip
				 * when checking uniqueness.
				 */
				projptr->flags |= QP_SORT | QP_NODISPLAY | QP_SORTUNIQ;
			}

			projptr->priority = sort_priority++;
		}
	}
	else
	{
		for ( i = 0; i < sortcnt; i++ )
		{
			p = sortlist[i];
			if ( *p == ':' ) {
				prmsg( MSG_ERROR, "modifer '%s' must be prefixed by an attribute name", p );
				free( nodelist );
				return( NULL );
			}

			if ( ( ( p[0] == 'A' ) && ( p[1] == 'L' ) && ( p[2] == 'L' ) ) &&
			     ( ( p[3] == '\0' ) || ( p[3] == ':' ) ) )
			{
				if ( ! lookupprojattr( p, nodelist, 1, &sortattr, FALSE ) )
				{
					set_attr_all( &sortattr, &nodelist[0] );

					if ( ( p[3] ) && ( ! attr_modifier( &sortattr, &p[3], TRUE ) ) )
					{
						/* message already printed */
						free( nodelist );
						return( NULL );
					}
				}
			} else {
				if ( ( ! lookupprojattr( p, nodelist, 1, &sortattr, FALSE ) ) ||
				     ( ( sortattr.attr < 0 ) && ( sortattr.attr != ATTR_ALL ) ) )
				{
					prmsg( MSG_ERROR, "unrecognized tuple compare attribute name '%s'", p );
					free( nodelist );
					return( NULL );
				}
			}

			while ( ( i + 1 < sortcnt ) && ( sortlist[i+1][0] == ':' ) )
			{
				p = sortlist[++i];

				if ( ! attr_modifier( &sortattr, p, TRUE ) )
				{
					/* message already printed */
					free( nodelist );
					return( NULL );
				}
			}

			/*
			 * Clear out any flag bits that are not
			 * used for sorting including attorval
			 * which is set for all:nodisplay=...
			 */
			sortattr.flags &= (QP_SORTMASK|QP_DESCENDING|QP_RMBLANKS);
			sortattr.attorval = NULL;

			/*
			 * Merge the sort attribute info with the projected
			 * attribute info if this is not a special attribute
			 * and of this is the first time this attribute has
			 * been included in the sorted by list.
			 */
			if ( ( sortattr.attr >= 0 ) &&
			     ( attrlist[ sortattr.attr ].priority > sort_priority ) )
			{
				projptr = &attrlist[ sortattr.attr ];

				projptr->flags = QP_SORT | sortattr.flags;
				projptr->subcnt = sortattr.subcnt;
				projptr->priority = sort_priority++;
				projptr->delim = sortattr.delim;
				for ( j = 0; j < (int) sortattr.subcnt; j++ )
					projptr->subsort[j] = sortattr.subsort[j];
				continue;
			}

			if ( sortattr.attr != ATTR_ALL )
			{
				if ( projcnt >= MAXATT ) {
					prmsg( MSG_ERROR, "too many projected attributes -- max is %d", MAXATT );
					free( nodelist );
					return( NULL );
				}

				attrlist[ projcnt ] = sortattr;
				projptr = &attrlist[ projcnt++ ];

				projptr->flags |= QP_SORT | QP_NODISPLAY;
				projptr->priority = sort_priority++;

				/*
				 * If normal attribute then set flag
				 * then this attribute should not be
				 * skipped when checking uniqueness.
				 */
				if ( sortattr.attr >= 0 )
					projptr->flags |= QP_SORTUNIQ;
			}
			else
			{
				register int attridx;

				/*
				 * When ATTR_ALL (all or ALL) is specified,
				 * then all (normal) attributes that
				 * have not been sorted will be sorted
				 * in order using the sort information
				 * appended to the special "all" attribute.
				 */
				for ( projptr = attrlist, attridx = 0; attridx < (int) maxattr; attridx++, projptr++ )
				{
					if ( projptr->priority < sort_priority )
						continue;

					projptr->flags = QP_SORT | sortattr.flags;
					projptr->subcnt = sortattr.subcnt;
					projptr->priority = sort_priority++;
					projptr->delim = sortattr.delim;
					for ( j = 0; j < (int) sortattr.subcnt; j++ )
						projptr->subsort[j] = sortattr.subsort[j];
				}
			}
		}

		if ( ( sort_priority == 0 ) && ( sortcnt != 0 ) ) {
			prmsg( MSG_ERROR, "no attributes in tuple merge \"by\" clause" );
			free( nodelist );
			return( NULL );
		}
	}

	/* If no sort attributes were given then sort on all atributes */
	if ( sort_priority == 0 )
	{
		for ( projptr = &attrlist[0], i = 0; i < (int) maxattr; i++, projptr++ )
		{
			projptr->flags = QP_SORT;
			projptr->priority = sort_priority++;
		}
	}

	usort = fbldquery( nodelist, 1, attrlist, projcnt, (struct queryexpr *)NULL, queryflags );

	if ( usort == NULL )
	{
		(void)pruerror( );
		prmsg( MSG_ERROR, "cannot create query for merging tuples" );
		free( nodelist );
		return( NULL );
	}

	projcnt = usort->attrcnt;

	if ( caseless || rmblanks )
	{
		register unsigned short	flags;

		for ( i = 0; i < (int) projcnt; i++ ) {
			if ( ( queryflags & Q_UNIQUE ) || ( usort->sortcnt == 0 ) ||
			     ( usort->attrlist[i].priority != 0xffff ) ) {
				flags = usort->attrlist[i].flags;
				switch ( flags & QP_SORTMASK ) {
				case QP_CASELESS:
				case QP_NOCASEDICT:
				case QP_NOCASEPRINT:
					if ( rmblanks )
						flags |= QP_RMBLANKS;
					break;
				case QP_DICTIONARY:
					if ( caseless ) {
						flags &= ~QP_SORTMASK;
						flags |= QP_NOCASEDICT;
					}
					if ( rmblanks )
						flags ^= QP_RMBLANKS;
					break;
				case QP_PRINTABLE:
					if ( caseless ) {
						flags &= ~QP_SORTMASK;
						flags |= QP_NOCASEPRINT;
					}
					if ( rmblanks )
						flags |= QP_RMBLANKS;
					break;
				case QP_STRING:
				case 0:
					if ( caseless ) {
						flags &= ~QP_SORTMASK;
						flags |= QP_CASELESS;
					}
					if ( rmblanks )
						flags |= QP_RMBLANKS;
					break;
				default:
					break;
				}
				usort->attrlist[i].flags = flags;
			}
		}
	}

	return( usort );
}

static
check_merge_limit( )
{
	if ( ( mergelimit ) && ( mergelimit != (int) mergetplcnt ) &&
	   ( ( mergetplcnt > (unsigned long) mergelimit ) || ( typemlimit == 'e' ) ) )
	{
		if ( prtExit5err )
		{
			if ( typemlimit == 'e' ) {
				prmsg( MSG_ERROR, "query returned %lu record%s when exactly %d record%s needed",
					mergetplcnt, mergetplcnt == 1 ? "" : "s",
					mergelimit, mergelimit == 1 ? " was" : "s were" );
			} else {
				prmsg( MSG_ERROR, "query returned %lu record%s when at most %d record%s needed", 
					mergetplcnt, mergetplcnt == 1 ? "" : "s",
					mergelimit, mergelimit == 1 ? " was" : "s were" );
			}
		}

		exit_code = 5;

		return( FALSE );
	}

	return( TRUE );
}

static void
print_results( )
{
	if ( ! quiet ) {
		if ( utplerrors ) {
			prmsg( MSG_WARN, "%ld tuple error%s encountered reading relation(s)",
				utplerrors, utplerrors != 1 ? "s" : "" );
		}
		if ( ( replace ) && ( key_compare ) )
		{
			mergetplcnt -= deltplcnt;

			if ( noop ) {
				prmsg( MSG_ERRNOTE, "%lu record%s would be merged and %lu record%s replaced - %lu total",
					mergetplcnt, mergetplcnt != 1 ? "s" : "",
					deltplcnt, deltplcnt != 1 ? "s" : "", tplcnt );
			} else {
				prmsg( MSG_ERRNOTE, "%lu record%s merged and %lu record%s replaced - %lu total",
					mergetplcnt, mergetplcnt != 1 ? "s" : "",
					deltplcnt, deltplcnt != 1 ? "s" : "", tplcnt );
			}
		}
		else
		{
			if ( noop ) {
				prmsg( MSG_ERRNOTE, "%lu record%s would be merged - %lu total",
					mergetplcnt, mergetplcnt != 1 ? "s" : "", tplcnt );
			} else {
				prmsg( MSG_ERRNOTE, "%lu record%s merged - %lu total",
					mergetplcnt, mergetplcnt != 1 ? "s" : "", tplcnt );
			}
		}
	}
}

main( argc, argv )
int argc;
char *argv[];
{
	register int i;
	int cnt;
	int clause;		/* what is next clause on cmd line? */
	OutputMode mode;	/* into, onto, or in <table> */
	register int curarg;
	char *outfile;		/* output file */
	char *outdescr;		/* output file descriptor */
	char *equalsign;	/* ptr to "=<alt_table>" part of output file */
	int maxattr;
	char **relnames;
	int relcnt;		/* relation count */
	int attrcnt;		/* attribute count */
	char **attrnames;	/* attr names as arguments */
	int cmpcnt;		/* tuple compare attribute count */
	char **cmpnames;	/* tuple compare attribute names */
	int sortcnt;		/* sort attribute count */
	char **sortnames;	/* sort attribute names */
	char use_sortlist;	/* compare tuples using the sort attribute names */

	prog = basename( *argv );	/* save the program name */

	/*
	 * Parse any command-line options given.
	 */
	if ( argc < 2 )
		usage( );

	for( curarg = 1; curarg < argc; curarg++ )
	{
		register char *option;

		option = argv[curarg];

		/*
		 * Look for a non-flag or the standard input
		 * file name "-" or "-=<relation>".  This will
		 * signal the end of flag parsing.
		 */
		if ( *option != '-' || option[1] == '\0' ||
			option[1] == '=' )
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

		while( option && *++option )
		{
			char *p;

			switch( *option )
			{
			case 'c':
				create_descr = TRUE;
				break;
			case 'd':
#ifdef DEBUG
				if ( strcmp( option, "debug" ) == 0 ) {
					(void)set_qdebug( "sel,expr,join" );
					option = NULL;
				}
				else
#endif
				if ( option[1] ) {
					++delimiteropt;
					cnvtbslsh( &option[1], &option[1] );
					if ( flddelim_set ) {
						for ( i = 1; i == 1 || option[i]; i++ )
							termdelimiter[(option[i] & 0xff)] = TRUE;
					} else {
						flddelim = option[1];
						if ( option[2] == '!' )
							notdelimiter = TRUE;
						flddelim_set = TRUE;
					}
					option = NULL;
				} else {
					if ( flddelim_set ) {
						++delimiteropt;
						widthdelimiter = TRUE;
					} else {
						++debug;
					}
				}
				break;
			case 'E':
				key_match_err = TRUE;
				key_nomatch_err = TRUE;
				break;
			case 'F':
			case 'f':
				i = option[0];
				if ( option[1] ) {
					p = &option[1];
					option = NULL;
				}
				else if ( argc > 0 ) {
					p = argv[++curarg];
				}
				else {
					p = NULL;
				}
				if ( p )
				{
					char *q;
					char opt;

					if ( ( q = strrchr( p, '#' ) ) != NULL )
					{
						*q = '\0';

						if ( i == 'F' ) {
							if ( ( strcmp( p, "R" ) == 0 ) ||
							     ( strcmp( p, "REC" ) == 0 ) ) {
								opt = 'F';
							} else {
								opt = '\0';
							}
						} else {
							if ( q == p + 1 ) {
								switch ( *p ) {
								case 'k':
								case 'K':
								case 'm':
								case 'M':
								case 'q':
								case 'Q':
								case 'r':
								case 's':
									opt = *p;
									break;
								default:
									opt = '\0';
								}
							} else if ( strcmp( p, "recK" ) == 0 ) {
								opt = 'K';
							} else if ( strcmp( p, "recM" ) == 0 ) {
								opt = 'M';
							} else if ( strcmp( p, "recQ" ) == 0 ) {
								opt = 'Q';
							} else if ( strcmp( p, "rec" ) == 0 ) {
								opt = 'r';
							} else if ( strcmp( p, "seek" ) == 0 ) {
								opt = 's';
							} else {
								opt = '\0';
							}
						}

						*q++ = '#';

						if ( opt == '\0' ) {
							prmsg( MSG_ERROR, "unrecognized -%c argument prefix '%s'", (char) i, p );
							if ( i != 'F' )
								prmsg( MSG_CONTINUE, "prefix must be \"K#\" or \"M#\" or \"Q#\" or \"r#\" or \"s#\"" );
							usage( );
						} else {
							i = opt;
						}
						cnt = atoi( q );
					} else {
						cnt = atoi( p );
					}
				} else {
					cnt = 0;
				}
				if (( cnt <= 0 ) || ( cnt > 10 ))
				{
					if ( i != 'F' ) {
						i = 'f';
					}
					prmsg( MSG_ERROR, "precision of record number for -%c option must be between 1 and 10", (char) i );
					usage( );
				} else {
					switch ( i ) {
					case 'F':
						sprintf( RECnumfmt, "%%0%dlu", cnt );
						break;
					case 'k':
					case 'K':
						sprintf( recKnumfmt, "%%0%dlu", cnt );
						break;
					case 'm':
					case 'M':
						sprintf( recMnumfmt, "%%0%dlu", cnt );
						break;
					case 'q':
					case 'Q':
						sprintf( recQnumfmt, "%%0%dlu", cnt );
						break;
					case 'r':
						if ( cnt >= 2 ) {
							recnumlen = cnt;
						} else {
							recnumlen = 0;
						}
						break;
					case 's':
						if ( cnt >= 2 ) {
							seeknumlen = cnt;
						} else {
							seeknumlen = 0;
						}
						break;
					case 'f':
					default:
						if ( cnt >= 2 ) {
							recnumlen = cnt;
							seeknumlen = cnt;
						} else {
							recnumlen = 0;
							seeknumlen = 0;
						}
						sprintf( recKnumfmt, "%%0%dlu", cnt );
						sprintf( recMnumfmt, "%%0%dlu", cnt );
						sprintf( recQnumfmt, "%%0%dlu", cnt );
						break;
					}
				}
				break;
			case 'i':		/* all caseless compares */
				queryflags ^= Q_NOCASECMP;
				break;
			case 'n':
				noop = TRUE;	/* do not modify output table */
				break;
			case 'M':
				if ( option[1] ) {
					p = &option[1];
				} else if ( argc > 1 ) {
					p = argv[++curarg];
				} else {
					usage( );
				}
				if ( *p == ':' )
					++p;
				if ( strcmp( p, "caseless" ) == 0 ) {
					caseless = TRUE;
				} else if ( ( strcmp( p, "blanks" ) == 0 ) ||
					    ( strcmp( p, "noblanks" ) == 0 ) ) {
					rmblanks = TRUE;
				} else {
					prmsg( MSG_ERROR, "invalid -M '%s' attribute modifier", p );
					usage( );
				}
				option = "M";
				break;
			case 'P':	/* for given exit code, do not print error messages */
				if ( option[1] ) {
					p = &option[1];
					option = NULL;
				} else if ( curarg < argc - 2 ) {
					p = argv[++curarg];
					option = NULL;
				} else {
					p = "usage";
				}
				if ( ( p[0] == '4' ) && ( p[1] == '\0' ) ) {
					prtExit4err = FALSE;
				} else if ( ( p[0] == '5' ) && ( p[1] == '\0' ) ) {
					prtExit5err = FALSE;
				} else if ( ( p[0] == '6' ) && ( p[1] == '\0' ) ) {
					prtExit6err = FALSE;
				} else {
					prmsg( MSG_ERROR, "parameter for -P option must be \"4\" or \"5\" or \"6\"" );
					usage( );
				}
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
						option = NULL;
					}
				}
				else if ( curarg < argc - 2 ) {
					p = argv[ curarg + 1 ];
					if ((( p[0] == '-' ) &&
					     ( isdigit( p[1] ))) ||
					    ( isdigit( p[0] ))) {
						uchecktuple = UCHECKPRINT;
						utplelimit = atoi( p );
						option = NULL;
						++curarg;
					}
				}
				break;
			case 'q':		/* don't print tuple count */
				quiet = TRUE;
				break;
			case 'R':
				replace = TRUE;
				break;
			case 'e':		/* exact merge limit */
			case 'm':		/* most (maximum) merge limit */
				typemlimit = option[0];
				if ( ( option[1] ) && ( isdigit( option[1] ) ) ) {
					mergelimit = atoi( &option[1] );
					option = NULL;
				}
				else if ( ( option[1] == '\0' ) &&
				          ( curarg < argc - 2 ) &&
				          ( isdigit( *argv[ curarg + 1 ] ) ) ) {
					mergelimit = atoi( argv[ curarg + 1] );
					option = NULL;
					++curarg;
				}
				else {
					prmsg( MSG_ERROR, "'%c' option requires numeric argument",
						*option );
					usage( );
				}
				if ( mergelimit <= 0 ) {
					prmsg( MSG_ERROR, "unrecognized record count for -e or -m option" );
					usage( );
				}
				break;
			case 'r':		/* record limit */
				if ( option[1] )
				{
					p = &option[1];
					option = NULL;
				}
				else if ( ( option[1] == '\0' ) &&
					  ( curarg < argc - 2 ) )
				{
					p = argv[++curarg];
					option = NULL;
				}
				else {
					prmsg( MSG_ERROR, "'%c' option requires record limit argument",
						*option );
					usage( );
				}
				if (( p[0] == 'r' ) &&
				    ( p[1] == 'e' ) &&
				    ( p[2] == 'c' ) &&
				    ( p[3] != '\0' ) &&
				    ( p[4] == '#' )) {
					p += 3;
				}
				if (( p[0] != '\0' ) && ( p[1] == '#' )) {
					i = p[0];
					if ( i == 'k' || i == 'K' ) {
						i = 'K';
					} else if ( i == 'm' || i == 'M' ) {
						i = 'M';
					} else if ( i == 'q' || i == 'Q' ) {
						i = 'Q';
					} else {
						prmsg( MSG_ERROR, "unrecognized -r argument prefix '%s'", p );
						prmsg( MSG_CONTINUE, "prefix must be either \"Q#\" or \"K#\" or \"M#\"" );
						usage( );
					}
					p += 2;
				} else {
					i = 0;
				}
				if ( ( p ) && ( *p ) && ( isdigit( *p ) ) ) {
					retrievelimit = atoi( p );
				} else {
					retrievelimit = 0;
				}
				if ( retrievelimit <= 0 ) {
					prmsg( MSG_ERROR, "unrecognized record count for -r option" );
					usage( );
				}
				switch ( i ) {
				case 'K':
					retrieveKlimit = retrievelimit;
					break;
				case 'M':
					retrieveMlimit = retrievelimit;
					break;
				case 'Q':
					retrieveQlimit = retrievelimit;
					break;
				default:
					retrieveKlimit = retrievelimit;
					retrieveMlimit = retrievelimit;
					retrieveQlimit = retrievelimit;
					break;
				}
				break;
			case 's':		/* sort the output */
				queryflags ^= Q_SORT;
				break;
			case 'u':
				queryflags ^= Q_UNIQUE;
				break;
			case 'U':
				++Unique;
				break;
			case 'z':
				zerowidth = FALSE;
				break;
			case 'V':
				prmsg( MSG_NOTE, "%s", uversion );
				exit( 0 );
			default:
				prmsg( MSG_ERROR, "unrecognized option '%c'",
					*option );
				usage( );
			}
		}
	}

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
	 * Set environment to handle read (unpack) of packed relations
	 * based on the UNITYUNPACK environement variable.
	 */
	setunpackenv();

	/*
	 * Allow "value=..." attribute modifier to be recognized.
	 */
	uqpnvalue = QP_NEWVALUE;

	/*
	 * If there is a list of attributes to project, find them and 
	 * save them away for later use.
	 */
	attrnames = &argv[curarg];
	for ( clause = 0, attrcnt = 0; curarg < argc; attrcnt++, curarg++ )
	{
		if ( strcmp( argv[ curarg ], "from" ) == 0 )
		{
			clause = CL_FROM;
			++curarg;		/* skip "from" keyword */
			break;		/* end of any attribute list so break out */
		}
		else if ( ( curarg + 1 < argc ) &&
		        ( ( strcmp( argv[curarg], "sorted" ) == 0 ) ||
		          ( strcmp( argv[curarg], "keyed" ) == 0 ) ) )
		{
			if ( strcmp( argv[ curarg + 1 ], "by" ) == 0 )
			{
				if ( argv[curarg][0] == 'k' ) {
					/*
					 * Need to treat sorted by attributes
					 * as a list of key attributes.
					 */
					sort_keys = 0xffff;
				}
				clause = CL_SORT;
				queryflags ^= Q_SORT;
				curarg += 2;		/* skip "sorted by" keywords */
				break;		/* end of any attribute list so break out */
			}
			else if ( strcmp( argv[ curarg + 1 ], "from" ) == 0 )
			{
				if ( argv[curarg][0] == 'k' ) {
					/*
					 * Add uniqueness check to query.
					 * We do not want to check ALL
					 * attributes for uniqueness
					 * after/outside the query.
					 */
					queryflags ^= Q_UNIQUE;
				}
				clause = CL_FROM;
				queryflags ^= Q_SORT;
				curarg += 2;            /* skip "sorted from" keywords */
				break;		/* end of any attribute list so break out */
			}
		}
	}

	if ( attrcnt == 0 ) {
		attrnames = NULL;
	}

	if ( clause == CL_SORT )
	{
		sortnames = &argv[curarg];
		for ( sortcnt = 0; curarg < argc; sortcnt++, curarg++ )
		{
			if ( strcmp( argv[curarg], "from" ) == 0 )
			{
				clause = CL_FROM;
				++curarg;	/* skip "from" keyword */
				break;	/* end of sorted by attribute list so break out */
			}
		}
		if ( sortcnt == 0 ) {
			prmsg( MSG_ERROR, "no attributes given for \"sorted by\" clause" );
			usage( );
		}
	} else {
		sortnames = NULL;
		sortcnt = 0;
	}

	cmpcnt = 0;
	cmpnames = (char **)NULL;
	use_sortlist = FALSE;

	/*
	 * Parse the list of relation names.
	 */
	relnames = &argv[curarg];
	for ( relcnt = 0; curarg < argc; relcnt++, curarg++ )
	{
		if ( strcmp( argv[curarg], "in" ) == 0 ) {
			clause = CL_OUTPUT;
			mode = IN_TABLE;
			break;
		}
		else if ( strcmp( argv[curarg], "into" ) == 0 ) {
			clause = CL_OUTPUT;
			cmpcnt = -1;	/* do not compare tuples being merged */
			mode = INTO_TABLE;
			break;
		}
		else if ( strcmp( argv[curarg], "onto" ) == 0 ) {
			clause = CL_OUTPUT;
			cmpcnt = -1;	/* do not compare tuples being merged */
			mode = ONTO_TABLE;
			break;
		}
	}

	if ( relcnt == 0 ) {	/* Gotta have a relation */
		prmsg( MSG_ERROR, "no relations given on command line" );
		usage( );
	} else {
		++curarg;	/* skip over in[to]/onto keyword */
	}

	if (( clause != CL_OUTPUT ) || ( curarg >= argc )) {
		prmsg( MSG_ERROR, "no output table given on command line" );
		usage( );
	}

	outfile = argv[curarg++];
	outdescr = equalsign = strchr( outfile, '=' );
	if ( outdescr )
		*outdescr++ = '\0';	/* get rid of '=' */

	if (( outfile[0] == '\0' ) ||
	   (( outfile[0] == '-' ) && ( outfile[1] == '\0' ))) {
		prmsg( MSG_ERROR, "invalid output table: %s", outfile );
		usage( );
	}

	if ( chkperm( outfile, P_EXIST ) ) {
		if ( mode == INTO_TABLE ) {
			prmsg( MSG_ERROR, "output file '%s' already exists", outfile );
			exit( 2 );
		} else {
			create_descr = FALSE;
			tbl_exists = TRUE;
		}
	} else {
		if ( create_descr )
		{
			char *p, *q;
			char path[MAXPATH+4];

			/*
			 * When -c option is specified then
			 * create descriptor with the output
			 * table regardless of whether or
			 * not an alternate table (descriptor)
			 * was specified.
			 */
			p = outfile;
			if ( ( q = strrchr( p, '/' ) ) == NULL ) {
				path[0] = 'D';
				strcpy( &path[1], p );
			} else {
				i = ++q - p;
				strncpy( path, p, (unsigned) i );
				path[i++] = 'D';
				strcpy( &path[i], q );
			}
			if ( chkperm( path, P_EXIST ) ) {
				prmsg( MSG_ERROR, "output file descriptor '%s' already exists", path );
				exit( 2 );
			}
		}
	}

	/*
	 * See if there is a sorted or keyed clause
	 * for comparing the tuples to be merged.
	 */
	if ( curarg < argc ) {
		if ( ( strcmp( argv[curarg], "sorted" ) == 0 ) ||
		     ( strcmp( argv[curarg], "keyed" ) == 0 ) )
		{
			if ( cmpcnt == -1 ) {
				prmsg( MSG_ERROR, "\"%s\" clause for merging tuples is invalid when using \"%s <table>\"",
					argv[curarg], mode == INTO_TABLE ? "into" : "onto" );
				usage( );
			}

			if ( argv[curarg++][0] == 'k' ) {
				key_compare = TRUE;
			}

			/*
			 * check for "by" attribute list
			 */
			if ( ( curarg < argc ) && ( strcmp( argv[curarg], "by" ) == 0 ) )
			{
				++curarg;
				if ( ( curarg < argc ) && ( strcmp( argv[curarg], "rec#" ) == 0 ) ) {
					++curarg;
					if ( ( curarg < argc ) && ( isdigit( argv[curarg][0] ) ) ) {
						merge_recnum = atol( argv[curarg++] );
					}
					if ( ( merge_recnum < 0 ) || ( ( merge_recnum == 0 ) &&
					   ( ( replace ) || ( key_compare ) ) ) )
					{
						if ( merge_recnum == 0 ) {
							prmsg( MSG_ERROR, "rec# 0 is not valid when used as a key or with the -R (replace) option" );
						}
						usage( );
					}
				} else {
					/*
					 * Get list of attributes to be compared.
					 */
					cmpnames = &argv[curarg];
					for ( cmpcnt = 0; curarg < argc; cmpcnt++, curarg++ ) {
						if ( strcmp( argv[curarg], "where" ) == 0 )
							break;	/* do not skip over "where" */
					}
					if ( cmpcnt == 0 ) {
						usage( );
					}
				}
			} else {
				/*
				 * Compare tuples to be merged using the same
				 * attribute list that was used for sorting
				 * the retrieved tuples.
				 */
				use_sortlist = TRUE;
			}
		}
	}

	/*
	 * See if there is a where-clause.
	 */
	if ( curarg < argc ) {
		if ( strcmp( argv[curarg], "where" ) != 0 )
			usage( );	/* unrecognized cmd line */
	}

	/*
	 * Now do the real work.  First find which tuples apply (from the
	 * where-clause), and then print out the appropriate information.
	 */
	query = makequery( queryflags, relnames, relcnt, attrnames, attrcnt,
			sortnames, sortcnt, &argv[ curarg ], argc - curarg );
	if (  query == NULL ) {
		/* message already printed */
		exit( 1 );
	}

	/*
	 * Check permissions on each relation and see how many
	 * attributes each has.
	 */
	maxattr = 0;
	for( i = 0; i < (int) query->nodecnt; i++ )
	{
		if ( strcmp( query->nodelist[i]->rel->path, "-" ) != 0 &&
			! chkperm( query->nodelist[i]->rel->path, P_READ,
				(unsigned short)geteuid(),
				(unsigned short)getegid() ) )
		{
			prmsg( MSG_ERROR, "cannot read file '%s'",
				query->nodelist[i]->rel->path );
			usage( );
		}
		if ( query->nodelist[i]->rel->attrcnt > (unsigned) maxattr )
			maxattr = query->nodelist[i]->rel->attrcnt;
	}
	(void)set_attralloc( maxattr );

	if ( caseless || rmblanks )
	{
		register unsigned short	flags;

		for ( i = 0; i < (int) query->attrcnt; i++ ) {
			if ( ( queryflags & Q_UNIQUE ) || ( query->sortcnt == 0 ) ||
			     ( query->attrlist[i].priority != 0xffff ) ) {
				flags = query->attrlist[i].flags;
				switch ( flags & QP_SORTMASK ) {
				case QP_CASELESS:
				case QP_NOCASEDICT:
				case QP_NOCASEPRINT:
					if ( rmblanks )
						flags |= QP_RMBLANKS;
					break;
				case QP_DICTIONARY:
					if ( caseless ) {
						flags &= ~QP_SORTMASK;
						flags |= QP_NOCASEDICT;
					}
					if ( rmblanks )
						flags ^= QP_RMBLANKS;
					break;
				case QP_PRINTABLE:
					if ( caseless ) {
						flags &= ~QP_SORTMASK;
						flags |= QP_NOCASEPRINT;
					}
					if ( rmblanks )
						flags |= QP_RMBLANKS;
					break;
				case QP_STRING:
				case 0:
					if ( caseless ) {
						flags &= ~QP_SORTMASK;
						flags |= QP_CASELESS;
					}
					if ( rmblanks )
						flags |= QP_RMBLANKS;
					break;
				default:
					break;
				}
				query->attrlist[i].flags = flags;
			}
		}
	}

	delimcnt = 0;
	for( i = query->attrcnt - 1; i >= 0; i-- )
	{
		if ( (query->attrlist[i].flags & QP_NODISPLAY) == 0 )
		{
			/*
			 * If we have a field delimiter, set the terminate
			 * character to it on all but the last projected
			 * attribute.  (The last projected attribute always
			 * has a new-line as it's terminator.)
			 */
			if ( flddelim_set ) {
				if ( delimcnt == 0 ) {
					if ( ( query->attrlist[i].attrtype == QP_FIXEDWIDTH ) &&
					   ( ( delimiteropt == 1 ) ||
					   ( ( delimiteropt >= 2 ) &&
					     ( widthdelimiter != notdelimiter ) ) ) ) {
						query->attrlist[i].attrtype = QP_TERMCHAR;
						query->attrlist[i].attrwidth = 0;
						query->attrlist[i].terminate = '\n';
					}
				} else {
					if ( delimiteropt == 0 ) {
						query->attrlist[i].terminate = (char)flddelim;
					} else if ( delimiteropt == 1 ) {
						query->attrlist[i].attrtype = QP_TERMCHAR;
						query->attrlist[i].attrwidth = 0;
						query->attrlist[i].terminate = (char)flddelim;
					} else {
						if ( query->attrlist[i].attrtype == QP_TERMCHAR ) {
							if ( termdelimiter[query->attrlist[i].terminate] != notdelimiter )
								query->attrlist[i].terminate = (char)flddelim;
						} else {
							if ( widthdelimiter != notdelimiter ) {
								query->attrlist[i].attrtype = QP_TERMCHAR;
								query->attrlist[i].attrwidth = 0;
								query->attrlist[i].terminate = (char)flddelim;
							}
						}
					}
				}
			}
			delimcnt++;
		}
	}

	if ( ( delimcnt == 0 ) || ( query->attrcnt == 0 ) )
	{
		prmsg( MSG_ERROR, "all projected attributes are marked 'nodisplay'" );
		usage( );
	}

	/*
	 * We need to pass a pointer to the '=' (if present)
	 * in the original output file specified on the
	 * command line so that the entire "table[=<alt_table>]"
	 * can be passed to getrelinfo() so that it can determine
	 * whether or not the user wants the data directory to
	 * be searched first for the output description.
	 */
	if ( equalsign )
		*equalsign = '=';

	if ( getrelinfo( outfile, &relinfo, FALSE ) == NULL )
	{
		if ( tbl_exists == TRUE ) {
			(void)pruerror();
			prmsg( MSG_ERROR, "cannot get descriptor info for output table '%s'",
				outfile );
			exit( 2 );
		}

		/* remove the "=<alt_table>" from the output file name if present */
		if ( equalsign )
			*equalsign = '\0';

		/*
		 * Must make our own descriptor file.
		 */
		if ( querytorel( query, &relinfo, outfile, outfile, 0 ) == NULL )
		{
			(void)pruerror();
			prmsg( MSG_ERROR, "cannot convert projected attributes to relation" );
			exit( 2 );
		}

		/*
		 * If any friendly attribute names were given
		 * for any renamed pseudo attributes (rec# or REC#)
		 * then we must add them add them to relinfo now
		 * before we attempt to write a new descriptor.
		 */
		for ( cnt = i = 0; i < (int) query->attrcnt && cnt < (int) relinfo.attrcnt; i++ )
		{
			if ( query->attrlist[i].flags & QP_NODISPLAY )
				continue;

			if ( attorvals[i] )
			{
				if ( relinfo.attrs[cnt].friendly == NULL )
					relinfo.attrs[cnt].friendly = attorvals[i];
			}

			++cnt;
		}

		create_descr = TRUE;
	}
	else
	{
		/* remove the "=<alt_table>" from the output file name if present */
		if ( equalsign )
			*equalsign = '\0';

		if ( create_descr )
			relinfo.path = outfile;
	}

	/*
	 * Initialize source to destination attribute mapping
	 * and get length of buffer required to print any
	 * source attributes that map to a fixed-width destination.
	 */
	if ( ! init_map( query, &relinfo, &mapinfo ) ) {
		(void)pruerror( );
		prmsg( MSG_ERROR, "failed to map query to output table" );
		exit( 2 );
	}

	/*
	 * Compile query to sort (compare) tuples being merged if needed.
	 */
	if ( ( cmpcnt >= 0 ) && ( merge_recnum < 0 ) )
	{
		if ( Unique ) {
			/*
			 * If uniqueness was requested and
			 * we are sorting tuples then set
			 * the key compare flag to indicate
			 * that we need to discard matching
			 * tuples.  Otherwise, we need to
			 * reset the Unique flag so that
			 * only the specified list of key
			 * attributes get checked for uniqueness.
			 */
			if ( ( key_compare ) || ( mode != IN_TABLE ) ) {
				Unique = 0;
			} else {
				/*
				 * Ignore key error flags that may
				 * be set if the -E option was used
				 * since we are setting the key compare
				 * flag when the user did not use
				 * the "keyed [by]" clause.
				 */
				key_compare = TRUE;
				replace = FALSE;
				key_match_err = FALSE;
				key_nomatch_err = FALSE;
			}
		}

		if ( use_sortlist == TRUE ) {
			usort = makeusort( &relinfo, (char **)NULL, 0, query, (int) Unique );
		} else {
			usort = makeusort( &relinfo, cmpnames, cmpcnt, (struct uquery *)NULL, (int) Unique );
		}

		if ( usort == NULL ) {
			/* message already printed */
			exit( 1 );
		}
	}

	/*
	 * Setup mapping from projected attr# to displayed output attr#
	 */
	for ( cnt = curarg = i = 0; i < (int) query->attrcnt; i++ )
	{
		if ( query->attrlist[i].flags & QP_NODISPLAY ) {
			if ( query->attrlist[i].flags & QP_SORTUNIQ ) {
				++cnt;
			}
			proj2attr[i] = -1;
		} else {
			proj2attr[i] = curarg++;
		}
	}
	if ( cnt )
	{
		for ( i = 0; i < (int) query->attrcnt; i++ )
		{
			if ( ( query->attrlist[i].flags & (QP_NODISPLAY|QP_SORTUNIQ) ) == (QP_NODISPLAY|QP_SORTUNIQ) )
			{
				for ( curarg = 0; curarg < (int) query->attrcnt; curarg++ )
				{
					if ( ( proj2attr[curarg] >= 0 ) &&
					     ( query->attrlist[curarg].attr == query->attrlist[i].attr ) &&
					     ( query->attrlist[curarg].rel == query->attrlist[i].rel ) )
					{
						proj2attr[i] = proj2attr[curarg];
						--cnt;
						break;
					}
				}
			}
		}
		if ( cnt )
		{
			prmsg( MSG_INTERNAL, "failed to find displayed attr# for multiple sort attr#" );
			exit( 2 );
		}
	}

	if ( sort_keys )
	{
		if ( query->sortcnt < sort_keys ) {
			sort_keys = query->sortcnt;
		}
		for ( i = 0; i < (int) sort_keys; i++ )
		{
			curarg = query->attrlist[i].sortattr;

			if ( query->attrlist[curarg].priority >= 0xffff )
				break;

			if ( proj2attr[curarg] < 0 )
			{
				int relidx;
				int attrnum;
				char *aname;

				relidx = query->attrlist[curarg].rel->nodenum;
				attrnum = query->attrlist[curarg].attr;

				if ( attrnum < 0 ) {
					if ( attrnum == ATTR_SEEK ) {
						aname = "seek#";
					} else {
						aname = "rec#";
					}
				} else {
					aname = query->nodelist[relidx]->rel->attrs[attrnum].aname;
				}

				prmsg( MSG_ERROR, "cannot sort by new \"value\" or \"nodisplay\" for key attribute '%d.%s'",
					relidx + 1, aname );
				usage( );
			}
			else
			{
				if ( query->attrlist[curarg].flags & QP_NEWVALUE )
					break;

				/*
				 * If a special (rec# or seek#) attribute is being sorted
				 * then make sure numeric type of sorting is specified
				 * since we will being comparing the output attribute
				 * (string) values instead of unsigned long integers.
				 */
				if ( ATTR_SPECIAL( query->attrlist[curarg].attr ) ) {
					query->attrlist[curarg].flags &= ~(QP_SORTMASK);
					query->attrlist[curarg].flags |= QP_NUMERIC;
					query->attrlist[curarg].subcnt = 0;
				}
			}
		}

		if ( i != (int) sort_keys )
		{
			prmsg( MSG_ERROR, "failed to find all keyed by attributes in query output attribute list" );
			usage( );
		}
	}

	if ( RECnumfmt[0] == '\0' ) {
		sprintf( RECnumfmt, "%%lu" );
	}
	if ( recKnumfmt[0] == '\0' ) {
		sprintf( recKnumfmt, "%%lu" );
	}
	if ( recMnumfmt[0] == '\0' ) {
		sprintf( recMnumfmt, "%%lu" );
	}
	if ( recQnumfmt[0] == '\0' ) {
		sprintf( recQnumfmt, "%%lu" );
	}

	if ( ( debug ) && ( noop ) )
	{
		char attrbuf[12];
		char dattrbuf[12];
		char special;

		fprintf(stderr, "query attrcnt = %u\n", query->attrcnt);
		fprintf(stderr, "query key cnt = %u\n", sort_keys);
		fprintf(stderr, "query sortcnt = %u\n", query->sortcnt);
		for (i = 0; i < (int) query->attrcnt; i++) {
			if ( proj2attr[i] >= 0 ) {
				sprintf(attrbuf, "%-3d", (int) proj2attr[i]);
			} else {
				sprintf(attrbuf, "%-3s", "");
			}
			fprintf(stderr, "attr[%03d] = %-3d", i, query->attrlist[i].attr);
			fprintf(stderr, " sortattr = %-3u", query->attrlist[i].sortattr);
			fprintf(stderr, " priority = %-5u", query->attrlist[i].priority);
			fprintf(stderr, " flags = 0x%04x", query->attrlist[i].flags);
			fprintf(stderr, " subcnt = %u", query->attrlist[i].subcnt);
			fprintf(stderr, " attr# = %s", attrbuf);
			fprintf(stderr, " prname = %s\n", query->attrlist[i].prname);
		}
		if ( usort )
		{
			fprintf(stderr, "usort attrcnt = %u\n", usort->attrcnt);
			fprintf(stderr, "usort sortcnt = %u\n", usort->sortcnt);
			for (i = 0; i < (int) usort->attrcnt; i++) {
				fprintf(stderr, "attr[%03d] = %-3d", i, usort->attrlist[i].attr);
				fprintf(stderr, " sortattr = %-3u", usort->attrlist[i].sortattr);
				fprintf(stderr, " priority = %-5u", usort->attrlist[i].priority);
				fprintf(stderr, " flags = 0x%04x", usort->attrlist[i].flags);
				fprintf(stderr, " subcnt = %u", usort->attrlist[i].subcnt);
				fprintf(stderr, " prname = %s\n", usort->attrlist[i].prname);
			}
		}
		fprintf(stderr, "table attrcnt = %u\n", relinfo.attrcnt);
		for (i = 0; i < (int) relinfo.attrcnt; i++) {
			if (mapinfo[i].sattrnum >= 0) {
				sprintf(attrbuf, "%-3d", mapinfo[i].sattrnum);
			} else {
				sprintf(attrbuf, "%-3s", "");
			}
			if (mapinfo[i].dattridx >= 0) {
				sprintf(dattrbuf, "%-3d", mapinfo[i].dattridx);
			} else {
				sprintf(dattrbuf, "%-3s", "");
			}
			if (mapinfo[i].special) {
				special = mapinfo[i].special;
			} else {
				special = ' ';
			}
			fprintf(stderr, "attr[%03d] sattrnum = %s dattridx = %s special = %c aname = %s\n",
				i, attrbuf, dattrbuf, special, relinfo.attrs[i].aname);
		}
	}

	exit_code = 2;	/* default exit status for failure after this point */

	/*
	 * Catch all signals so we don't leave any DB's dangling with
	 * lock files still around.
	 */
	(void)set_sig( SIGHUP, catch_sig );
	(void)set_sig( SIGINT, catch_sig );
	(void)set_sig( SIGQUIT, catch_sig );
	(void)signal( SIGILL, catch_sig );
	(void)signal( SIGTRAP, catch_sig );
#ifdef SIGIOT
	(void)signal( SIGIOT, catch_sig );
#endif
#ifdef SIGEMT
	(void)signal( SIGEMT, catch_sig );
#endif
	(void)signal( SIGFPE, catch_sig );
	(void)signal( SIGBUS, catch_sig );
	(void)signal( SIGSEGV, catch_sig );
#ifdef SIGSYS
	(void)signal( SIGSYS, catch_sig );
#endif
	(void)signal( SIGPIPE, catch_sig );
	(void)set_sig( SIGUSR1, catch_sig );
	(void)set_sig( SIGUSR2, catch_sig );
	(void)signal( SIGALRM, SIG_IGN );

	if ( ( ! noop ) || ( tbl_exists ) )
	{
		perptr = init_peruse( &relinfo, "w" );
		if ( perptr == NULL ) {
			(void)pruerror( );
			prmsg( MSG_ERROR, "initialization for perusing '%s' failed", outfile );
			exit( exit_code );
		}
	}

	/*
	 * Note: We do not want to create an output descriptor until
	 *	 we have sucessfully locked the table so that we do
	 *	 not create a descriptor if we cannot update (create)
	 *	 the table.
	 */

	if ( ( create_descr ) && ( ! noop ) && ( ! writedescr( &relinfo, NULL ) ) )
	{
		(void)pruerror( );
		prmsg( MSG_ERROR, "cannot write descriptor information for file '%s'",
			relinfo.path );
		exit( exit_code );
	}

	if ( ! key_compare )
	{
		key_match_err = FALSE;
		key_nomatch_err = FALSE;
	}

	tplcnt = querytplcnt = keytplcnt = mergetplcnt = origtplcnt = 0;

	if ( ! queryeval( query, &result ) )
	{
		(void)pruerror( );
		prmsg( MSG_ERROR, "query evaluation failed" );
		prmsg( MSG_ERROR, "due to previous errors, no updates were done" );
		if ( perptr ) {
			(void)end_peruse( perptr, FALSE );
		}
		exit( exit_code );
	}

	if ( ! initresult( &result ) )
	{
		if ( uerror != UE_NOERR ) {
			(void)pruerror( );
			prmsg( MSG_ERROR, "failed to initialize list of tuples to be merged" );
			prmsg( MSG_ERROR, "due to previous errors, no updates were done" );
			if ( perptr ) {
				(void)end_peruse( perptr, FALSE );
			}
			exit( exit_code );
		}

		if ( ( ! quiet ) && ( perptr ) )
		{

			if ( uchecktuple != UCHECKFATAL ) {
				/* treat output table tuple errors as fatal errors */
				uchecktuple = UCHECKFATAL;
				utplmsgtype = MSG_ERROR;
				utplelimit = utplerrors + 1;
			}

			if ( ! stop_save_tc( perptr, TRUE, &origtplcnt ) ) {
				if ( uerror != UE_NOERR ) {
					(void)pruerror( );
				}
				prmsg( MSG_ERROR, "failed to read or save existing tuples in output file" );
				prmsg( MSG_ERROR, "due to previous errors, no updates were done" );
				(void)end_peruse( perptr, FALSE );
				exit( exit_code );
			}

			tplcnt = origtplcnt;
		}

		if ( ! check_merge_limit( ) ) {
			if ( ( prtExit5err ) || ( exit_code != 5 ) ) {
				prmsg( MSG_ERROR, "due to previous errors, no updates were done" );
			}
			if ( perptr ) {
				(void)end_peruse( perptr, FALSE );
			}
			exit( exit_code );
		}

		if ( ( perptr ) && ( ! end_peruse( perptr, FALSE ) ) ) {
			if ( uerror != UE_NOERR ) {
				(void)pruerror( );
			}
			prmsg( MSG_ERROR, "failure terminating perusal of output table" );
			prmsg( MSG_ERROR, "due to previous errors, no updates were done" );
			exit ( exit_code );
		}

		print_results( );

		exit( 0 );
	}

	if ( uchecktuple != UCHECKFATAL ) {
		/* treat output table tuple errors as fatal errors */
		uchecktuple = UCHECKFATAL;
		utplmsgtype = MSG_ERROR;
		utplelimit = utplerrors + 1;
	}


	/*
	 * Add calls to various functions to do the work here.
	 */
	if ( ( mode == INTO_TABLE ) || ( mode == ONTO_TABLE ) || ( ( ! tbl_exists ) && ( noop ) ) )
	{
		if ( ( mode == IN_TABLE ) && ( key_nomatch_err ) && ( replace ) && ( key_compare ) )
		{
			if ( prtExit4err ) {
				prmsg( MSG_ERROR, "no matching key for rec# 1 from input query" );
				prmsg( MSG_ERROR, "due to previous errors, no updates were done" );
			}
			(void)freeresult( &result );
			if ( perptr )
				(void)end_peruse( perptr, FALSE );
			exit_code = 4;
			exit( exit_code );
		}

		if ( perptr )
		{
			if ( ! save_original( (unsigned long)0 ) )
			{
				prmsg( MSG_ERROR, "due to previous errors, no updates were done" );
				(void)freeresult( &result );
				(void)end_peruse( perptr, FALSE );
				exit( exit_code );
			}

			if ( ( mode == INTO_TABLE ) && ( origtplcnt != 0 ) )
			{
				prmsg( MSG_ERROR, "output file '%s' already exists", outfile );
				(void)freeresult( &result );
				(void)end_peruse( perptr, FALSE );
				exit( exit_code );
			}
		}

		if ( ! append_output( ) )
		{
			prmsg( MSG_ERROR, "due to previous errors, no updates were done" );
			(void)freeresult( &result );
			if ( perptr ) {
				(void)end_peruse( perptr, FALSE );
			}
			exit( exit_code );
		}
	}
	else if ( merge_recnum >= 0 )
	{
		unsigned long count;

		/*
		 * merge tuple based on specific rec# position
		 */

		count = (unsigned long) merge_recnum;

		if ( ( count ) && ( ( key_compare == TRUE ) ||
		   ( ( replace ) && ( key_compare == FALSE ) ) ) ) {
			count -= 1;
		}

		/*
		 * Save existing tuples until we are at
		 * the required record position.
		 */
		if ( count ) {
			if ( ! save_original( count ) ) {
				prmsg( MSG_ERROR, "due to previous errors, no updates were done" );
				if ( perptr ) {
					(void)end_peruse( perptr, FALSE );
				}
				exit( exit_code );
			}
			if ( origtplcnt != count ) {
				if ( prtExit6err ) {
					prmsg( MSG_ERROR, "cannot seek to position of rec# %lu in table that has %lu %s",
						count, origtplcnt, origtplcnt == 1 ? "record" : "records" );
					prmsg( MSG_ERROR, "due to previous errors, no updates were done" );
				}
				if ( perptr ) {
					(void)end_peruse( perptr, FALSE );
				}
				exit_code = 6;
				exit( exit_code );
			}
		}

		if ( key_compare == FALSE )
		{
			/* sort by rec# */

			/*
			 * insert tuples obtained from the query
			 */
			if ( ! append_output( ) )
			{
				prmsg( MSG_ERROR, "due to previous errors, no updates were done" );
				(void)freeresult( &result );
				if ( perptr ) {
					(void)end_peruse( perptr, FALSE );
				}
				exit( exit_code );
			}

			/*
			 * Save any unread tuples from the original table.
			 */
			if ( perptr )
			{
				count = 0;

				if ( ! stop_save_tc( perptr, TRUE, &count ) ) {
					(void)pruerror( );
					prmsg( MSG_ERROR, "failure saving unread tuples from original table" );
					prmsg( MSG_ERROR, "due to previous errors, no updates were done" );
					(void)end_peruse( perptr, FALSE );
					exit( exit_code );
				}
				origtplcnt += count;
				tplcnt += count;
			}
		}
		else
		{
			/* keyed by rec# */

			if ( ! merge_recnums( ) )
			{
				if ( ( exit_code != 4 ) || ( prtExit4err ) ) {
					prmsg( MSG_ERROR, "due to previous errors, no updates were done" );
				}
				if ( perptr ) {
					(void)end_peruse( perptr, FALSE );
				}
				exit( exit_code );
			}
		}
	}
	else
	{
		/*
		 * merge tuples based on comparision of tuple attr values
		 */
		if ( ! merge_tuples( ) )
		{
			if ( ( exit_code != 4 ) || ( prtExit4err ) ) {
				prmsg( MSG_ERROR, "due to previous errors, no updates were done" );
			}
			if ( perptr ) {
				(void)end_peruse( perptr, FALSE );
			}
			exit( exit_code );
		}
	}

	if ( ! freeresult( &result ) )
	{
		(void)pruerror( );
		prmsg( MSG_ERROR, "failure freeing query result" );
		prmsg( MSG_ERROR, "due to previous errors, no updates were done" );
		if ( perptr ) {
			(void)end_peruse( perptr, FALSE );
		}
		exit( exit_code );
	}

	if ( ! check_merge_limit( ) ) {
		if ( ( prtExit5err ) || ( exit_code != 5 ) ) {
			prmsg( MSG_ERROR, "due to previous errors, no updates were done" );
		}
		if ( perptr ) {
			(void)end_peruse( perptr, FALSE );
		}
		exit( exit_code );
	}

	if ( perptr ) {
		if ( ! end_peruse( perptr, ( ( noop ) || ( ! mergetplcnt ) )  ? FALSE : TRUE ) ) {
			(void)pruerror();
			prmsg( MSG_ERROR, "cannot finish updating '%s'", outfile );
			prmsg( MSG_ERROR, "due to previous errors, no updates were done" );
			exit( exit_code );
		}
	}

	print_results( );

	exit( 0 );
}

