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
#include <stdarg.h>
#include <ctype.h>
#include "uquery.h"
#include "uerror.h"

#define MAXINSERT	(4 * MAXRELATION)

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
extern char *calloc();
#endif
extern FILE *init_modify();

static struct uinsertinfo *insertlist[MAXINSERT];
static short insertcnt;

addattr( insptr, aname )
register struct uinsertinfo *insptr;
char *aname;
{
	register int i;

	if ( insptr->attrcnt < MAXATT ) {
		i = findattr( insptr->relptr->attrs,
			insptr->relptr->attrcnt, aname );
		if ( i < 0 || insptr->attrmap[i] != -1 ) {
			set_uerror( UE_UNRECATTR );
			return( FALSE );
		}

		insptr->attrmap[i] = insptr->attrcnt++;
	}

	return( TRUE );
}

static
struct uinsertinfo *
_init_insert( relptr, attrstr, tuplecnt )
struct urelation *relptr;
register char *attrstr;
unsigned long *tuplecnt;
{
	register struct uinsertinfo *insptr;
	register char *str;
	register int i;
	register char oldch;
	struct urelio relio;

	if ( insertcnt >= MAXINSERT ) {
		set_uerror( UE_NOMEM );
		return( NULL );
	}
	insertlist[insertcnt] = (struct uinsertinfo *)calloc( 1, sizeof( struct uinsertinfo ) );
	if ( insertlist[insertcnt] == NULL ) {
		set_uerror( UE_NOMEM );
		return( NULL );
	}
	insptr = insertlist[insertcnt];
	insptr->relptr = relptr;

	/*
	 * Create the attribute map, from the order given on the command
	 * to the order given in the relation itself.
	 */
	for( i = 0; i < MAXATT; i++ )
		insptr->attrmap[i] = -1;

	insptr->attrcnt = 0;
	str = strpbrk( attrstr, " \t\n" );
	while( str && insptr->attrcnt < MAXATT ) {
		oldch = *str;
		*str = '\0';
		i = findattr( relptr->attrs, relptr->attrcnt, attrstr );
		if ( i < 0 || insptr->attrmap[i] != -1 ) {
			free( insptr );
			set_uerror( UE_UNRECATTR );
			return( NULL );
		}

		insptr->attrmap[i] = insptr->attrcnt++;
		*str = oldch;
		while( *str && isspace( *str ) )
			++str;
		attrstr = str;
		str = strpbrk( attrstr, " \t\n" );
	}

	if ( insptr->attrcnt < MAXATT && *attrstr ) {
		i = findattr( relptr->attrs, relptr->attrcnt, attrstr );
		if ( i < 0 || insptr->attrmap[i] != -1 ) {
			free( insptr );
			set_uerror( UE_UNRECATTR );
			return( NULL );
		}

		insptr->attrmap[i] = insptr->attrcnt++;
	}

	insptr->tmpfp = init_modify( relptr, insptr->tmptable, insptr->lockfile );
	if ( insptr->tmpfp == NULL ) {
		free( insptr );
		return( NULL );
	}

	/*
	 * Open original data base.
	 */
	if ( ! relropen( &relio, relptr, insptr->tmpfp ) )
	{
		(void)end_modify( relptr, FALSE, insptr->tmpfp, insptr->tmptable,
			insptr->lockfile );
		free( insptr );

		set_uerror( UE_ROPEN );
		return( NULL );
	}

	/*
	 * Copy table to temp file and optionally check/count the tuples.
	 */
	if ( tuplecnt ) {
		i = cprelationtc( &relio, insptr->tmpfp, relptr );
		*tuplecnt = relio.tuplecnt;
	} else {
		i = cprelation( &relio, insptr->tmpfp );
	}
	if ( i != TRUE )
	{
		(void)relrclose( &relio );
		(void)end_modify( relptr, FALSE, insptr->tmpfp, insptr->tmptable,
			insptr->lockfile );
		free( insptr );

		return( NULL );
	}

	if ( ! relrclose( &relio ) )
	{
		/*
		 * Just in case we ever allow a packed relation to be modified,
		 * we do not know if the relation was successfully unpacked
		 * and/or if all of the data was read until the pipe is closed.
		 */
		(void)end_modify( relptr, FALSE, insptr->tmpfp, insptr->tmptable,
			insptr->lockfile );
		free( insptr );

		return( NULL );
	}

	insertcnt++;

	return( insptr );
}

struct uinsertinfo *
init_insert( relptr, attrstr )
struct urelation *relptr;
register char *attrstr;
{
	return( _init_insert( relptr, attrstr, (unsigned long *)0 ) );
}

struct uinsertinfo *
init_insert_tc( relptr, attrstr, tuplecnt )
struct urelation *relptr;
register char *attrstr;
unsigned long *tuplecnt;
{
	return( _init_insert( relptr, attrstr, tuplecnt ) );
}

end_insert( insptr, keep )
register struct uinsertinfo *insptr;
int keep;
{
	int rc;
	int i;

	RETSIGTYPE (*sigint)(), (*sigquit)(), (*sighup)(), (*sigterm)();

	if ( insptr == NULL ) {
		rc = TRUE;
		for( i = insertcnt - 1; i >= 0; i-- ) {
			if ( ! end_insert( insertlist[i], keep ) )
				rc = FALSE;
		}

		return( rc );
	}

	sigint = signal( SIGINT, SIG_IGN );
	sighup = signal( SIGHUP, SIG_IGN );
	sigquit = signal( SIGQUIT, SIG_IGN );
	sigterm = signal( SIGTERM, SIG_IGN );

	rc = end_modify( insptr->relptr, keep, insptr->tmpfp, insptr->tmptable,
			insptr->lockfile );

	for( i = 0; i < insertcnt; i++ ) {
		if ( insertlist[i] == insptr ) {
			while( ++i < insertcnt )
				insertlist[i - 1] = insertlist[i];
			break;
		}
	}

	insertcnt--;
	free( insptr );

	(void)signal( SIGINT, sigint );	/* reset signals */
	(void)signal( SIGHUP, sighup );
	(void)signal( SIGQUIT, sigquit );
	(void)signal( SIGTERM, sigterm );

	return( rc );
}

do_insert( insptr, attrvals )
register struct uinsertinfo *insptr;
register char **attrvals;
{
	register int i;
	register struct uattribute *attrptr;
	register char *val;
	register int incr;

	attrptr = insptr->relptr->attrs;
	for( i = 0; i < insptr->relptr->attrcnt; i++, attrptr++ ) {
		val = insptr->attrmap[i] >= 0 ? attrvals[insptr->attrmap[i]] : "";
		incr = _writeattr( insptr->tmpfp, attrptr, val,
				i == insptr->relptr->attrcnt - 1, TRUE );
		if ( incr < 0 )
			return( FALSE );
		else if ( incr > DBBLKSIZE )
		{
			set_uerror( UE_ATTSIZE );
			return( FALSE );
		}
	}

	return( TRUE );
}

/*VARARGS1*/
vdo_insert( struct uinsertinfo *insptr, ... )
{
	va_list args;
	register int cnt;
	char *attrvals[MAXATT];

	va_start( args, insptr );

	if ( insptr->attrcnt <= 0 ) {
		set_uerror( UE_NOINIT );
		return( FALSE );
	}

	for( cnt = 0; cnt < insptr->attrcnt; cnt++ ) {
		attrvals[cnt] = va_arg( args, char * );
		if ( attrvals[cnt] == NULL )
			attrvals[cnt] = "";
	}

	return( do_insert( insptr, attrvals ) );
}
