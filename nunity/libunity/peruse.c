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
#include "uquery.h"
#include "uerror.h"

extern char *calloc();
extern FILE *init_modify();

#define MAXPERUSE	(2 * MAXRELATION)

static struct uperuseinfo *peruselist[MAXPERUSE];
static short perusecnt;


static
closeread( ioptr )
struct urelio *ioptr;
{
	int	rc = TRUE;

	if ( ( ioptr->flags & UIO_FILEMASK ) == UIO_PACKED )
	{
		if ( ( packedclose( ioptr->fd ) ) &&
		     ( ioptr->flags & UIO_EOF ) )
		{
			ioptr->flags |= UIO_ERROR;
			set_uerror( UE_PACKEDEOF );
			rc = FALSE;
		}
	}
	else if ( ( ioptr->flags & UIO_STDIN ) == 0 )
		close( ioptr->fd );

	ioptr->fd = -1;
	ioptr->flags &= ~UIO_FILEMASK;
	ioptr->blkcnt = -1;
	ioptr->tuplecnt = -1;
	freerelblist( ioptr->readbuf );
	freerelblist( ioptr->savebuf );
	ioptr->readbuf = ioptr->savebuf = ioptr->curblk = NULL;

	return( rc );
}

static struct uperuseinfo *
peruse_alloc( )
{
	register struct uperuseinfo *perptr;

	if ( perusecnt >= MAXPERUSE ) {
		set_uerror( UE_NOMEM );
		return( NULL );
	}
	perptr = (struct uperuseinfo *)calloc( 1, sizeof( struct uperuseinfo ) );
	if ( perptr == NULL ) {
		set_uerror( UE_NOMEM );
		return( NULL );
	}

	peruselist[perusecnt] = perptr;
	perptr->relio.fd = -1;

	return( perptr );
}

struct uperuseinfo *
peruse_lock( relptr )
struct urelation *relptr;
{
	register struct uperuseinfo *perptr;

	if ( (perptr = peruse_alloc( )) == NULL )
		return( NULL );
	perptr->relptr = relptr;

	perptr->updmode = 'w';

	if ( ! start_transact( relptr, perptr->lockfile ) ) {
		free( perptr );
		return( NULL );
	}

	perusecnt++;

	return( perptr );
}
	
struct uperuseinfo *
init_peruse( relptr, mode )
struct urelation *relptr;
char *mode;
{
	register struct uperuseinfo *perptr;

	if ( (perptr = peruse_alloc( )) == NULL )
		return( NULL );
	perptr->relptr = relptr;

	perptr->updmode = *mode;

	switch( perptr->updmode ) {
	case 'a':
	case 'w':
		perptr->tmpfp = init_modify( relptr, perptr->tmptable,
					perptr->lockfile );
		if ( perptr->tmpfp == NULL) {
			free( perptr );
			return( NULL );
		}
		break;
	case 'r':
		perptr->tmpfp = NULL;
		break;
	default:
		free( perptr );
		set_uerror( UE_NOINIT );
		return( FALSE );
	}

	/*
	 * Open original data base.
	 */
	if ( ! relropen( &perptr->relio, relptr, perptr->tmpfp ) )
	{
		if ( perptr->updmode != 'r' )
			(void)end_modify( relptr, FALSE, perptr->tmpfp,
				perptr->tmptable, perptr->lockfile );
		free( perptr );

		set_uerror( UE_ROPEN );
		return( FALSE );
	}

	perusecnt++;

	return( perptr );
}

start_save( perptr, use_old )
register struct uperuseinfo *perptr;
int use_old;
{
	if ( perptr->tmpfp )
		return( TRUE );

	if ( perptr->updmode == 'r' ) {
		set_uerror( UE_NOINIT );
		return( FALSE );
	}

	perptr->tmpfp = init_modify( perptr->relptr, perptr->tmptable,
				perptr->lockfile );
	if ( perptr->tmpfp == NULL)
		return( FALSE );

	if ( use_old ) {
		/*
		 * Open original data base.
		 */
		if ( (perptr->relio.fd = open( perptr->relptr->path, 0 )) < 0 ) {
			(void)end_modify( perptr->relptr, FALSE, perptr->tmpfp,
				perptr->tmptable, NULL );
			perptr->tmpfp = NULL;
			set_uerror( UE_ROPEN );
			return( FALSE );
		}
		else {
			perptr->relio.readbuf = NULL;
			perptr->relio.curblk = NULL;
			perptr->relio.savebuf = NULL;
			perptr->relio.tuplecnt = 0;
			perptr->relio.flags = UIO_NORMAL;
			perptr->relio.blkcnt = 0;
		}
	}

	return( TRUE );
}

static
_stop_save( perptr, save_remain, tuplecnt )
register struct uperuseinfo *perptr;
int save_remain;
unsigned long *tuplecnt;
{
	int rc;

	if ( save_remain && perptr->tmpfp && perptr->relio.fd >= 0 ) {
		if ( tuplecnt ) {
			unsigned long otuplecnt;

			otuplecnt = perptr->relio.tuplecnt;
			rc = cprelationtc( &perptr->relio, perptr->tmpfp, perptr->relptr );
			*tuplecnt = perptr->relio.tuplecnt - otuplecnt;
		} else {
			rc = cprelation( &perptr->relio, perptr->tmpfp );
		}
	} else {
		if ( tuplecnt ) {
			*tuplecnt = 0;
		}
		rc = TRUE;
	}
	if ( perptr->relio.fd >= 0 ) {
		if ( ! closeread( &perptr->relio ) )
			rc = FALSE;	/* unpack error */
	}
	if ( perptr->tmpfp ) {
		if ( fclose( perptr->tmpfp ) == EOF ) {
			set_uerror( UE_WRITE );
			rc = FALSE;
		}
		perptr->tmpfp = NULL;
	}

	return( rc );
}

stop_save( perptr, save_remain )
register struct uperuseinfo *perptr;
int save_remain;
{
	return( _stop_save( perptr, save_remain, (unsigned long *)0 ) );
}

stop_save_tc( perptr, save_remain, tuplecnt )
register struct uperuseinfo *perptr;
int save_remain;
unsigned long *tuplecnt;
{
	return( _stop_save( perptr, save_remain, tuplecnt ) );
}

end_peruse( perptr, keep )
register struct uperuseinfo *perptr;
int keep;
{
	int rc, tmprc;
	int i;
	RETSIGTYPE (*sigint)(), (*sigquit)(), (*sighup)(), (*sigterm)();

	rc = TRUE;

	if ( perptr == NULL ) {
		for( i = perusecnt - 1; i >= 0; i-- ) {
			if ( ! end_peruse( peruselist[i], keep ) )
				rc = FALSE;
		}

		return( rc );
	}

	sigint = signal( SIGINT, SIG_IGN );
	sighup = signal( SIGHUP, SIG_IGN );
	sigquit = signal( SIGQUIT, SIG_IGN );
	sigterm = signal( SIGTERM, SIG_IGN );

	if ( perptr->updmode == 'r' ) {
		if ( ! closeread( &perptr->relio ) )
			rc = FALSE;
	} else {
		if ( keep && perptr->tmpfp && perptr->relio.fd >= 0 )
			rc = cprelation( &perptr->relio, perptr->tmpfp );
		if ( perptr->relio.fd >= 0 ) {
			if ( ! closeread( &perptr->relio ) )
				rc = FALSE;	/* unpack error */
		}

		tmprc = end_modify( perptr->relptr, rc ? keep : FALSE,
				perptr->tmpfp, perptr->tmptable,
				perptr->lockfile );
		if ( rc )
			rc = tmprc;
	}

	for( i = 0; i < perusecnt; i++ ) {
		if ( peruselist[i] == perptr ) {
			while( ++i < perusecnt )
				peruselist[i - 1] = peruselist[i];
			break;
		}
	}

	perusecnt--;
	free( perptr );

	(void)signal( SIGINT, sigint );	/* reset signals */
	(void)signal( SIGHUP, sighup );
	(void)signal( SIGQUIT, sigquit );
	(void)signal( SIGTERM, sigterm );

	return( rc );
}

peruse_tuple( perptr, tplptr )
register struct uperuseinfo *perptr;
struct utuple *tplptr;
{
	if ( perptr->relio.fd < 0 || tplptr->tplval == NULL ) {
		set_uerror( UE_NOINIT );
		return( FALSE );
	}

	return( readtuple( perptr->relptr, &perptr->relio, tplptr, NULL ) );
}

peruse( perptr, attrvals )
register struct uperuseinfo *perptr;
char **attrvals;
{
	struct utuple tpl;

	if ( perptr->relio.fd < 0 ) {
		set_uerror( UE_NOINIT );
		return( FALSE );
	}

	tpl.tplval = attrvals;

	return( readtuple( perptr->relptr, &perptr->relio, &tpl, NULL ) );
}

savetuple( perptr, attrvals )
struct uperuseinfo *perptr;
register char **attrvals;
{
	register struct uattribute *attrptr, *endattr;
	register int incr;

	if ( perptr->tmpfp == NULL ) {
		set_uerror( UE_NOINIT );
		return( FALSE );
	}
	attrptr = perptr->relptr->attrs;
	endattr = &perptr->relptr->attrs[perptr->relptr->attrcnt - 1];
	for( ; attrptr <= endattr; attrptr++, attrvals++ )
	{
		incr = _writeattr( perptr->tmpfp, attrptr, *attrvals,
				attrptr == endattr, TRUE );
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
