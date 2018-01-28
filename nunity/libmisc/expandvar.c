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
#include <pwd.h>
#include <ctype.h>

/*
 * These routines expand environment variable references, tilde references
 * (login directories), and substitution values (`cmd`) in a given string
 * (a path or variable value), and returns a pointer to the expanded result.
 * The result to can be put in a buffer supplied by the user or in a
 * malloc()'ed buffer.
 *
 * Tilde references are only recognized at the beginning of the string or
 * after a colon (':').
 *
 * To expand wildcards (* and ?), use the expand() routine in expand.c
 *
 * In expandvar(), undefined environment variables remain as $VAR in the
 * expanded output, while expandfull() has them "disappear".  Both
 * are just front-ends to the _expandvar() routine (that was the
 * original, and the expandfull() functionality was added later).
 */


#define MAXSIZE		4096	/* max length of a expanded result */

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif


extern char *getenv( );
extern struct passwd *getpwnam( );
extern char *malloc( );

char *tilde( );
char *getvar( );
char *substitute( );

char *
_expandvar( orig, varname, resultbuf, leavevar )
char *orig;
char *varname;
char *resultbuf;
char leavevar;
{
	register char *result;
	register char *fptr;	/* current spot in full */
	register char *optr;	/* current spot in orig_path */
	register char *tmpptr;	/* temporary pointer in orig */
	register short chktilde;
	register char oldch;
	register int end_brace;
	char full[MAXSIZE];

	chktilde = 1;
	optr = orig;
	full[0] = '\0';
	result = ( resultbuf == NULL || resultbuf == orig ) ? full : resultbuf;
		
	if ( varname ) {
		strcpy( result, varname );
		fptr = result + strlen( result );
		*fptr++ = '=';
	}
	else
		fptr = result;

	while( *optr && fptr < &result[MAXSIZE] ) {
		switch( *optr ) {

		case '\\':
			if ( optr[1] )
				*fptr++ = *++optr;
			++optr;
			break;

		case '$':
			tmpptr = ++optr;
			if ( isalpha( *tmpptr ) || *tmpptr == '_' ||
					*tmpptr == '{' ) {

				if ( *tmpptr == '{' ) {
					end_brace = TRUE;
					tmpptr++;
				}
				else
					end_brace = FALSE;

				for( ; isalnum( *tmpptr ) || *tmpptr == '_';
						tmpptr++ )
					;
				oldch = *tmpptr;
				*tmpptr = '\0';
				fptr = getvar( fptr, optr, &result[MAXSIZE] - fptr, leavevar );
				*tmpptr = oldch;
				if ( end_brace && *tmpptr == '}' )
					++tmpptr;
				optr = tmpptr;
			}
			else
				*fptr++ = optr[-1];
			break;

		case '~':
			if ( chktilde ) {
				for( tmpptr = ++optr; isalnum( *tmpptr ) ; tmpptr++ )
					;
				oldch = *tmpptr;
				*tmpptr = '\0';
				fptr = tilde( fptr, optr, &result[MAXSIZE] - fptr );
				*tmpptr = oldch;
				optr = tmpptr;
			}
			else
				*fptr++ = *optr++;
			break;

		case '`':
			for( tmpptr = ++optr; *tmpptr && *tmpptr != '`'; tmpptr++)
				/* skip escaped backquotes for nested substitution */
				if ( *tmpptr == '\\' ) tmpptr++;
			oldch = *tmpptr;
			*tmpptr = '\0';
			fptr = substitute( fptr, optr, &result[MAXSIZE] - fptr );
			if ( oldch ) {
				*tmpptr = oldch;
				optr = tmpptr + 1;
			}
			else
				optr = tmpptr;
			break;

		default:
			chktilde = (*optr == ':');
			*fptr++ = *optr++;
		}
	}
	*fptr = '\0';

	if ( resultbuf == NULL ) {
		result = malloc( (unsigned)(strlen( full ) + 1 ) );
		strcpy( result, full );
	}
	else if ( resultbuf == orig ) {
		strcpy( resultbuf, full );
		result = resultbuf;
	}

	return( result );
}


char *
expandvar( orig, varname, resultbuf )
char *orig;
char *varname;
char *resultbuf;
{
	return( _expandvar( orig, varname, resultbuf, TRUE ) );
}


char *
expandfull( orig, varname, resultbuf )
char *orig;
char *varname;
char *resultbuf;
{
	return( _expandvar( orig, varname, resultbuf, FALSE ) );
}

static char *
tilde( dest, logname, max )
char *dest;
char *logname;
int max;
{
	struct passwd *pwdptr;
	char *logdir;
	int len;

	if ( *logname ) {
		pwdptr = getpwnam( logname );
		logdir = pwdptr ? pwdptr->pw_dir : NULL;
	}
	else {
		logdir = getenv( "HOME" );
	}

	if ( logdir ) {
		len = strlen( logdir );
		if ( len > max )
			len = max;
		strncpy( dest, logdir, len );
	}
	else {
		*dest++ = '~';
		len = strlen( logname );
		if ( len > max )
			len = max;
		strncpy( dest, logname, len );
	}

	return( dest + len );
}

static char *
getvar( dest, varname, max, leavevar )
char *dest;
char *varname;
int max;
char leavevar;
{
	char *value;
	int len = 0;

	value = *varname ? getenv( varname ) : NULL;

	if ( value ) {
		len = strlen( value );
		if ( len > max )
			len = max;
		strncpy( dest, value, len );
	}
	else if ( leavevar ) {
		*dest++ = '$';
		len = strlen( varname );
		if ( len > max )
			len = max;
		strncpy( dest, varname, len );
	}

	return( dest + len );
}

static char *
substitute( dest, cmd, max )
char *dest;
char *cmd;
int max;
{
	FILE *fp;
	register int ch;
	register int newline;
	char escaped[MAXSIZE];
	register char *i, *j;

	/* change \` to ` */
	for ( i = cmd, j = escaped; *i != '\0'; i++, j++ )  {
		if ( *i == '\\' && *(i+1) == '`' )
			i++;
		*j = *i;
	}
	*j = '\0';

	if ( (fp = popen( escaped, "r" )) == NULL )
		return( dest );
	while( max > 0 && (ch = fgetc( fp )) != EOF ) {
		newline = ( ch == '\n' || ch == '\r' );
		*dest++ = newline ? ' ' : (char)ch;
		--max;
	}

	pclose( fp );

	if ( max > 0 && newline )
		--dest;
	*dest = '\0';

	return( dest );
}
