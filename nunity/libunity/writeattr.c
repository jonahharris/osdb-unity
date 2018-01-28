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
#include "urelation.h"
#include "uquery.h"
#include "udbio.h"
#include "uerror.h"

extern char *prog;
extern char *strchr();

static	int	nlinfw;	/* warn about NL in fixed width attribute */

writeattrvalue( fp, value, delim, prdelim, width, attrtype )
FILE *fp;
register char *value;
char delim;
char prdelim;
unsigned short width;
unsigned char attrtype;
{
	register int	size;

	switch (attrtype) {
	case QP_TERMCHAR:	/* TERM CHAR */
		return( _writeattrval( fp, value, delim, prdelim) >= 0);
	case QP_FIXEDWIDTH:	/* FIXED WIDTH */
		if ( width >= DBBLKSIZE )
		{
			set_uerror( UE_ATTSIZE );
			return( 0 );
		}
		if ( value == NULL ) {
			value = "";
		} else {
			register char *p;

			if ( ( nlinfw == 0 ) && ( ( p = strchr( value, '\n' ) ) != NULL ) ) {
				if ( p - value < (int)width ) {
					fprintf( stderr, "%s: WARNING: Output table contains embedded NL in fixed width attribute!\n", prog );
					++nlinfw;
				}
			}
		}
		if ( delim != '\0' && prdelim ) {
			size = fprintf( fp, "%-*.*s%c", width, width, value, delim );
		} else {
			size = fprintf( fp, "%-*.*s", width, width, value );
		}
		if ( size < 0 ) {
			set_uerror( UE_WRITE );
			return( 0 );
		}
		return( 1 );
	default:
		if ( delim != '\0' ) {
			return( _writeattrval( fp, value, delim, prdelim) >= 0);
		} else {
			return( _writeattrval( fp, value, delim, FALSE) >= 0);
		}
	}
}

writeattrval( fp, val, delim, prdelim )
FILE *fp;
register char *val;
char delim;
char prdelim;
{
	prdelim = ( ( delim ) || ( prdelim == TRUE ) ) ? TRUE : FALSE;
	return( _writeattrval( fp, val, delim, prdelim ) >= 0 );
}

_writeattrval( fp, val, delim, prdelim )
FILE *fp;
register char *val;
char delim;
char prdelim;	/* Print delimiter? */
{
	register int size = 0;

	if ( val == NULL )
		val = "";

	while( *val )
	{
		if ( *val == '\n' )
		{
			set_uerror( UE_NLINATTR );
			return( -1 );
		}
		else if ( *val == '\\' && prdelim && val[1] == '\0' )
		{
			if ( delim != '\n' && delim != '\0' ) {
				set_uerror( UE_LASTCHBS );
				return( -1 );
			}
		}
		else if ( *val == delim )
		{
			if ( *val == '\\' ) {
				set_uerror( UE_BSINATTR );
				return( -1 );
			}
			(void)putc( '\\', fp );
			size++;
		}
		(void)putc( *val, fp );
		val++;
		size++;
	}
	if ( prdelim )
	{
		(void)putc( delim, fp );
		size++;
	}

	if ( size > DBBLKSIZE )
	{
		set_uerror( UE_ATTSIZE );
		return( -1 );
	}

	if ( ferror( fp ) )
	{
		set_uerror( UE_WRITE );
		return( -1 );
	}

	return( size );
}

_writeattr( fp, attrptr, val, last, prdelim )
FILE *fp;
register struct uattribute *attrptr;
register char *val;
int last;
char prdelim;
{
	register int size;

	if ( val == NULL )
		val = "";

	if ( attrptr->attrtype == UAT_FIXEDWIDTH )
	{
		if ( attrptr->terminate >= DBBLKSIZE )
		{
			set_uerror( UE_ATTSIZE );
			return( -1 );
		} else {
			register char *p;

			if ( ( nlinfw == 0 ) && ( ( p = strchr( val, '\n' ) ) != NULL ) ) {
				if ( p - val < (int)attrptr->terminate ) {
					fprintf( stderr, "%s: WARNING: Output table contains embedded NL in fixed width attribute!\n", prog );
					++nlinfw;
				}
			}
		}

		size = fprintf( fp, last ? "%-*.*s\n" : "%-*.*s",
				attrptr->terminate, attrptr->terminate,
				val );
		if ( size < 0 )
			set_uerror( UE_WRITE );

		return( size );
	}
	else
		return( _writeattrval( fp, val, (char)attrptr->terminate,
					prdelim ) );
}

writeattr( fp, attrptr, val, last )
FILE *fp;
register struct uattribute *attrptr;
register char *val;
int last;
{
	return( _writeattr( fp, attrptr, val, last, TRUE ) >= 0 );
}
