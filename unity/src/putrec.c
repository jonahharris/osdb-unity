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

static int  nlinfw;	/* warn about NL in fixed width attribute */
static char *nlfw_att = "putrec: WARNING: Embedded newline in fixed width attribute.\n";
static char *size_att = "putrec: Maximum attribute size (%d) exceeded.\n";

int
putrec(file, fmt)
FILE	*file; 
struct	fmt *fmt;
{	
	int	i;
	register char	t, *s=fmt->val;
	register int	notnull;

	switch(fmt->flag) {
	case T:
		t = fmt->inf[1];
		notnull = (*s != '\0');
		while(*s!= 0)
		{
			if ( *s == '\n' ) {
				error( E_GENERAL,
					"putrec: Embedded newline in attribute value.\n" );
				/*
				 * We cannot return an error so just terminate the current
				 * field to prevent database corruption and return.
				 */
				putc( t, file );
				return( s - fmt->val + 1 );
			} else if ( *s == t )
			{
				putc( '\\', file );
				if (t == '\\') {
					error( E_GENERAL,
						"putrec: Embedded backslash in backslash terminated attribute value.\n" );
					/*
					 * We cannot return an error because too many
					 * places to check but at least we can preserve
					 * the integrity of the attribute (field) that
					 * was just written by not writing the remaining
					 * characters of the attribute value.
					 */
					return( s - fmt->val + 1 );
				}
				++notnull;
			}
			putc( *s, file );
			s++;
		}
		if (notnull)
		{
			if ((s[-1] == '\\') && (t != '\n') && (t != '\0'))
			{
				error( E_GENERAL, "putrec: Cannot have backslash as last character of an attribute value.\n" );
				/*
				 * We can't return an error because too many
				 * places to check.
			 	*/
			}
			i = s - fmt->val;
			if (notnull + i > DBBLKSIZE) {
				error( E_GENERAL, size_att, DBBLKSIZE );
			}
		}
		putc( t, file );
		return( s - fmt->val + 1 );

	case WN:
		for(notnull=i=0;i<fmt->flen;i++) {
			t = *s++;
			if ((t == '\n') && (notnull == 0)) {
				if (nlinfw == 0) {
					/* print warning about embedded NL */
					error( E_GENERAL, nlfw_att );
					++nlinfw;
				}
			} else if (t == '\0') {
				++notnull;
			}
			putc(t, file);
		}

		if (i >= DBBLKSIZE) {
			error( E_GENERAL, size_att, DBBLKSIZE );
		}

		return( fmt->flen );
	}
	/* somthing to return */
	return(0);
}

/*
 * Define a variable that the application
 * can initialize/check to see how many
 * '\\' characters were inserted into the
 * output record if/when the attribute
 * value contains field termination char.
 */
int	term_escapes;	/* initialized by application */

int
putnrec(file, fmt)
FILE	*file; 
struct	fmt *fmt;
{	
	int	i;
	register char	t, *s=fmt->val;
	register int	notnull;

	switch(fmt->flag) {
	case T:
		t = fmt->inf[1];
		notnull = (*s != '\0');
		while(*s!= 0)
		{
			if ( *s == '\n' ) {
				error( E_GENERAL,
					"putrec: Embedded newline in attribute value.\n" );
				/*
				 * Terminate the current field to
				 * prevent database corruption
				 * and return error indication.
				 */
				putc( t, file );
				i = s - fmt->val + 1;
				return( -i );
			} else if ( *s == t )
			{
				if (putc( '\\', file ) == EOF) {
					return(ERR);
				}
				if (t == '\\') {
					error( E_GENERAL,
						"putrec: Embedded backslash in backslash terminated attribute value.\n" );
					/* 
					 * We can preserve the integrity of the attribute
					 * (field) that was just written by not writing
					 * the remaining characters of the attribute value.
					 */
					i = s - fmt->val + 1;
					return( -i );	/* return error */
				}
				++notnull;
				++term_escapes;
			}
			if (putc( *s, file ) == EOF) {
				return(ERR);
			}
			s++;
		}
		if (putc( t, file ) == EOF) {
			return(ERR);
		}
		/*
		 * How many characters were written
		 * not including the term_escapes?
		 */
		i = s - fmt->val + 1;

		if (notnull)
		{
			if ((s[-1] == '\\') && (t != '\n') && (t != '\0'))
			{
				error( E_GENERAL, "putrec: Cannot have backslash as last character of an attribute value\n" );
				/*
				 * Return regative character count which
				 * will never be the same as ERR (-1).
				 */
				return(-i);
			}
			if (notnull + i - 1 > DBBLKSIZE) {
				error( E_GENERAL, size_att, DBBLKSIZE );
				return(-i);
			}
		}
		return( i );

	case WN:
		for(notnull=i=0;i<fmt->flen;i++) {
			t = *s++;
			if ((t == '\n') && (notnull == 0)) {
				if (nlinfw == 0) {
					/* print warning about embedded NL */
					error( E_GENERAL, nlfw_att );
					++nlinfw;
				}
			} else if (t == '\0') {
				++notnull;
			}
			if (putc(t, file) == EOF) {
				return(ERR);
			}
		}
		if (i >= DBBLKSIZE) {
			error( E_GENERAL, size_att, DBBLKSIZE );
			return( -i );
		}
		return( i );
	}
	/* somthing to return */
	return(0);
}
