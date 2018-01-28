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

#define NOCASE( ch )	(isupper( (ch) ) ? _tolower( (ch) ) : (ch))

char uc_upper[ 256 ];		/* characters mapped to upper case */
char uc_nondict[ 256 ];		/* dictionary sort to ignore character if set */
char uc_nonprint[ 256 ];	/* printable sort to ignore character if set */

int init_uc_upper = 0;		/* set to one (1) when uc_upper[] has been initialized */
int init_uc_nondict = 0;	/* set to one (1) when uc_nondict[] has been initialized */
int init_uc_nonprint = 0;	/* set to one (1) when uc_nonprint[] has been initialized */

void
init_upper_tbl()
{
	register int i;

	for ( i = 0; i < 256; i++ ) {
		uc_upper[ i ] = ( islower( i ) ? toupper( i ) : i );
	}

	/*
	 * Make sure uc_upper[ '\0' ] is set to '\0'
	 */
	uc_upper[ 0 ] = 0;

	init_uc_upper = 1;
}

void
init_nondict_tbl()
{
	register int i;

	for ( i = 0; i < 256; i++ ) {
		uc_nondict[ i ] = ( ! isalnum( i ) );
	}
	uc_nondict[ '\t' ] = uc_nondict[ ' ' ] = 0;

	/*
	 * Make sure '\0' is not ignored
	 */
	uc_nondict[ 0 ] = 0;

	init_uc_nondict = 1;
}

void
init_nonprint_tbl()
{
	register int i;

	for ( i = 0; i < 256; i++ ) {
		uc_nonprint[ i ] = ( isprint( i ) == 0 );
	}

	/*
	 * Make sure '\0' is not ignored
	 */
	uc_nonprint[ 0 ] = 0;

	init_uc_nonprint = 1;
}

nocasecmp( str1, str2 )
register char *str1;
register char *str2;
{
	register char ch1, ch2;

	if ( ! init_uc_upper ) {
		init_upper_tbl( );
	}

	while( *str1 && *str2 ) {
		/*
		ch1 = NOCASE( *str1 );
		ch2 = NOCASE( *str2 );
		*/
		ch1 = uc_upper[ (unsigned char) *str1 ];
		ch2 = uc_upper[ (unsigned char) *str2 ];
		if ( ch1 != ch2 )
			return( (int)ch1 - (int)ch2 );
		++str1;
		++str2;
	}

	return( (int)(*str1) - (int)(*str2) );
}

dictcmp( str1, str2 )
register char *str1;
register char *str2;
{
	register char ch1, ch2;

	if ( ! init_uc_nondict ) {
		init_nondict_tbl( );
	}

	/*
	 * Make sure '\0' is not ignored.
	 */
	uc_nondict[ 0 ] = 0;

	while( *str1 && *str2 ) {
		/*
		 * Get next char in both str1 and str2
		 * which are not to be ignored.
		 */
		while ( uc_nondict[ (unsigned char) *str1 ] )
			++str1;
		while ( uc_nondict[ (unsigned char) *str2 ] )
			++str2;

		ch1 = *str1;
		ch2 = *str2;

		if ( ch1 != ch2 )
			return( (int)ch1 - (int)ch2 );
		++str1;
		++str2;
	}

	return( (int)(*str1) - (int)(*str2) );
}

nocasedictcmp( str1, str2 )
register char *str1;
register char *str2;
{
	register char ch1, ch2;

	if ( ! init_uc_nondict ) {
		init_nondict_tbl( );
	}
	if ( ! init_uc_upper ) {
		init_upper_tbl( );
	}

	/*
	 * Make sure '\0' is not ignored and '\0' is always '\0'.
	 */
	uc_nondict[ 0 ] = 0;
	uc_upper[ 0 ] = 0;

	while( *str1 && *str2 ) {
		/*
		 * Get next char in both str1 and str2
		 * which are not to be ignored.
		 */
		while ( uc_nondict[ (unsigned char) *str1 ] )
			++str1;
		while ( uc_nondict[ (unsigned char) *str2 ] )
			++str2;

		ch1 = uc_upper[ (unsigned char) *str1 ];
		ch2 = uc_upper[ (unsigned char) *str2 ];

		if ( ch1 != ch2 )
			return( (int)ch1 - (int)ch2 );
		++str1;
		++str2;
	}

	return( (int)(*str1) - (int)(*str2) );
}

printcmp( str1, str2 )
register char *str1;
register char *str2;
{
	register char ch1, ch2;

	if ( ! init_uc_nonprint ) {
		init_nonprint_tbl( );
	}

	/*
	 * Make sure '\0' is not ignored.
	 */
	uc_nonprint[ 0 ] = 0;

	while( *str1 && *str2 ) {
		/*
		 * Get next char in both str1 and str2 that are not to be ignored.
		 */
		while ( uc_nonprint[ (unsigned char) *str1 ] )
			++str1;
		while ( uc_nonprint[ (unsigned char) *str2 ] )
			++str2;

		ch1 = *str1;
		ch2 = *str2;

		if ( ch1 != ch2 )
			return( (int)ch1 - (int)ch2 );
		++str1;
		++str2;
	}

	return( (int)(*str1) - (int)(*str2) );
}

nocaseprintcmp( str1, str2 )
register char *str1;
register char *str2;
{
	register char ch1, ch2;

	if ( ! init_uc_nonprint ) {
		init_nonprint_tbl( );
	}
	if ( ! init_uc_upper ) {
		init_upper_tbl( );
	}

	/*
	 * Make sure '\0' is not ignored and '\0' is always '\0'.
	 */
	uc_nonprint[ 0 ] = 0;
	uc_upper[ 0 ] = 0;

	while( *str1 && *str2 ) {
		/*
		 * Get next char in both str1 and str2 that are not to be ignored.
		 */
		while ( uc_nonprint[ (unsigned char) *str1 ] )
			++str1;
		while ( uc_nonprint[ (unsigned char) *str2 ] )
			++str2;

		ch1 = uc_upper[ (unsigned char) *str1 ];
		ch2 = uc_upper[ (unsigned char) *str2 ];

		if ( ch1 != ch2 )
			return( (int)ch1 - (int)ch2 );
		++str1;
		++str2;
	}

	return( (int)(*str1) - (int)(*str2) );
}

/*
 * copy string s2 to s1 in upper case
 */
char *
strcpyupper( s1, s2 )
register char *s1;
register char *s2;
{
	register char ch;
	char	*s0;

	s0 = s1;

	while ( *s2 ) {
		ch = *s2++;
		if ( islower( ch ) )
			ch = toupper( ch );
		*s1++ = ch;
	}
	*s1 = '\0';

	return( s0 );
}

/*
 * copy string s2 to s1 in lower case
 */
char *
strcpylower( s1, s2 )
register char *s1;
register char *s2;
{
	register char ch;
	char	*s0;

	s0 = s1;

	while ( *s2 ) {
		ch = *s2++;
		if ( isupper( ch ) )
			ch = tolower( ch );
		*s1++ = ch;
	}
	*s1 = '\0';

	return( s0 );
}
