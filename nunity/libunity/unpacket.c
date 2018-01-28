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
#include "urelation.h"
#include "udbio.h"
#include "uerror.h"

static void fillfield();
static struct uattribute *locate_attr( );
static int getline();

long
unpacket( infp, outfp, relptr, friendly, newline )
FILE *infp;
FILE *outfp;
struct urelation *relptr;
char friendly;
char newline;
{
	long lines = 1;
	long reccount = 0;
	char need_line = TRUE;
	char more_to_do = TRUE;
	register struct uattribute *attrptr, *endattr, *newattr;
	char *attname, *value;

	endattr = &relptr->attrs[ relptr->attrcnt ];
	while( more_to_do )
	{
		attrptr = relptr->attrs;
		while( attrptr < endattr )
		{
			if ( need_line )
			{
				if ( ! getline( infp, &lines,
						&attname, &value ) )
				{
					more_to_do = FALSE;
					break;
				}
				need_line = FALSE;
			}

			if ( strcmp( attname, "#####" ) == 0 )
			{
			 	/* record number line */
				need_line = TRUE;
				break;	/* go to next record */
			}

			newattr = locate_attr( attrptr, endattr, attname, friendly );
			if ( newattr == NULL )
			{
				set_uerror( UE_UNRECATTR );
				return( -lines );
			}
			else if ( newattr < attrptr )
			{
				need_line = FALSE;
				break;	/* start a new record */
			}
			else
			{
				while( attrptr < newattr )
				{
					fillfield( outfp, attrptr );
					attrptr++;
				}
			}

			if ( attrptr->attrtype == UAT_FIXEDWIDTH )
			{
				if ( strlen( value ) != attrptr->terminate )
				{
					set_uerror( UE_ATTWIDTH );
					return( -lines );
				}
				if ( _writeattrval( outfp, value, '\0', FALSE ) < 0 )
					return( -lines );

				need_line = TRUE;
			}
			else
			{	/* variable length fields */
				char lastch;

				if ( _writeattrval( outfp, value,
						attrptr->terminate,
						FALSE ) < 0 )
					return( -lines );

				/*
				 * This next section continues
				 * getting lines which begin with
				 * a tab and takes them as a
				 * continuation of the variable length
				 * field, inserting a "newline"
				 * character for each new line read.
				 */
				while ( 1 )
				{
					lastch = value[ strlen( value ) - 1 ];

					if ( ! getline( infp, &lines,
							&attname, &value ) )
					{
						more_to_do = FALSE;
						break;
					}
					if ( attname[0] != '\0' )
						break;

					if ( newline != '\0' )
						putc( newline, outfp );

					if ( _writeattrval( outfp, value,
							attrptr->terminate,
							FALSE ) < 0 )
						return( -lines );
				}

				/*
				 * If the last char is a backslash
				 * record parsing won't work.
				 */
				if ( lastch == '\\' )
				{
					set_uerror( UE_LASTCHBS );
					return( -lines );
				}

				putc( (char)attrptr->terminate, outfp );

			} /* end terminal char attribute */

			attrptr++;	/* increment to next attribute */

		} /* end for each attribute */

		if ( attrptr != relptr->attrs )
		{
			reccount++;

			for( ; attrptr < endattr; attrptr++ )
				fillfield( outfp, attrptr );

			if ( relptr->attrs[ relptr->attrcnt - 1 ].attrtype == UAT_FIXEDWIDTH )
				putc( '\n', outfp );
		}

	} /* end while( more_to_do ) */

	if ( ferror( outfp ) )
	{
		set_uerror( UE_WRITE );
		return( -lines );
	}

	return( reccount );
}

static void
fillfield( fp, attrptr )
FILE *fp;
register struct uattribute *attrptr;
{
	register int i;

	if ( attrptr->attrtype == UAT_FIXEDWIDTH )
	{
		for( i = 0; i < attrptr->terminate; i++ )
			putc( ' ', fp );
	}
	else
		putc( attrptr->terminate, fp );
}

static int
getline( infp, lines, attname, attvalue )
FILE *infp;
long *lines;
char **attname;
char **attvalue;
{
	register int len;
	register char *value;
	static char attrbuf[ DBBLKSIZE ];

	do
	{
		(*lines)++;
		if ( fgets( attrbuf, DBBLKSIZE - 1, infp ) == NULL )
			return( FALSE );

		len = strlen( attrbuf );
		if ( attrbuf[ len - 1 ] != '\n' )
		{
			/* Line too long */
			set_uerror( UE_ATTSIZE );
			return( FALSE );
		}
		attrbuf[ len - 1 ] = '\0';

	} while( attrbuf[0] == '\0' );

	*attname = attrbuf;
	value = strchr( attrbuf, '\t' );
	if ( value != NULL )
		*value++ = '\0';
	else
		value = "";
	*attvalue = value;

	return( TRUE );
}

static struct uattribute *
locate_attr( attrptr, endattr, attname, friendly )
register struct uattribute *attrptr;
register struct uattribute *endattr;
char *attname;
char friendly;
{
	for( ; attrptr < endattr; attrptr++ )
	{
		if ( strcmp( attname,
			friendly && attrptr->friendly && *attrptr->friendly ?
				attrptr->friendly : attrptr->aname ) == 0 )
		{
			return( attrptr );
		}
	}

	return( NULL );
}
