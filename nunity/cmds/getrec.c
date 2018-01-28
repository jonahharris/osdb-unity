/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "format.h"

#ifndef NULL
#define NULL	0L
#endif

extern char *calloc();

static struct fioblock *freelist;

static struct fioblock *
newfioblk( )
{
	register struct fioblock *blkptr;

	if ( freelist ) {
		blkptr = freelist;
		freelist = freelist->next;
		blkptr->fldcnt = 0;
		blkptr->next = NULL;
	}
	else {
		blkptr = (struct fioblock *)calloc( 1, sizeof( *blkptr ) );
		if ( blkptr == NULL )
			return( NULL );
	}

	blkptr->end = blkptr->curloc = blkptr->data;

	return( blkptr );
}

freefioblk( blkptr )
register struct fioblock *blkptr;
{
	blkptr->next = freelist;
	freelist = blkptr;
}

_freeallfioblk( )
{
	register struct fioblock *blkptr;
	register struct fioblock *next;

	for( blkptr = freelist; blkptr; blkptr = next ) {
		next = blkptr->next;
		free( blkptr );
	}

	freelist = NULL;

	return( TRUE );
}

static struct fioblock *
unusedfld( blkptr, val )
register struct fioblock *blkptr;
register char *val;
{
	for( ; blkptr; blkptr = blkptr->next ) {
		if ( val < blkptr->end && val >= blkptr->data ) {
			if ( blkptr->fldcnt > 0 )
				blkptr->fldcnt--;
			return( blkptr );
		}
	}

	return( NULL );
}

char nullattr[] = "\0";		/* string for unreadable fields */

unusedflds( ioptr, fldvals, fldcnt )
struct formatio *ioptr;
register char **fldvals;
int fldcnt;
{
	register struct fioblock *list, *readlist;
	register char **endfld;

	if ( fldcnt < 1 )
		return;

	readlist = ioptr->readbuf;
	for( endfld = &fldvals[fldcnt]; fldvals < endfld; fldvals++ ) {
		if ( *fldvals == NULL || *fldvals == nullattr )
			continue;
		else if ( (list = unusedfld( readlist, *fldvals )) != NULL )
			readlist = list;
		else if ( unusedfld( ioptr->savebuf, *fldvals ) == NULL )
			(void)unusedfld( ioptr->readbuf, *fldvals );
	}
}

static struct fioblock *
findfreespace( ioptr, size )
struct formatio *ioptr;
register int size;
{
	register struct fioblock *blkptr;

	if ( size > FIOBLKSIZE ) {
		ioptr->flags |= FIO_ERROR|FIO_TOOBIG;
		return( NULL );
	}

	if ( ioptr->savebuf == NULL ) {
		if ( (ioptr->savebuf = newfioblk( )) == NULL )
			ioptr->flags |= FIO_ERROR|FIO_NOMEM;
		return( ioptr->savebuf );
	}

	for( blkptr = ioptr->savebuf; blkptr; blkptr = blkptr->next ) {
		if ( &blkptr->data[FIOBLKSIZE] - blkptr->curloc >= size )
			return( blkptr );	/* found a block it will fit in */
		else if ( blkptr->next == NULL ) {
			blkptr->next = newfioblk( );
			return( blkptr->next );
		}
	}

	return( NULL );   /* can never happen -- used to keep compiler happy */
}

static char *
copy2str( ioptr, src1, len1, src2, len2, incrcnt )
struct formatio *ioptr;
char *src1;
int len1;
char *src2;
int len2;
int incrcnt;
{
	register struct fioblock *blkptr;
	char *start;

	if ( (blkptr = findfreespace( ioptr, len1 + len2 + 1 )) == NULL )
		return( NULL );

	start = blkptr->curloc;

	if ( len1 > 0 ) {
		memcpy( blkptr->curloc, src1, len1 );
		blkptr->curloc += len1;
	}
	if ( len2 > 0 ) {
		memcpy( blkptr->curloc, src2, len2 );
		blkptr->curloc += len2;
	}

	*blkptr->curloc++ = '\0';	/* null terminate the string */
	if ( incrcnt )
		blkptr->fldcnt++;

	return( start );
}

static struct fioblock *
readnextblk( ioptr, prev )
struct formatio *ioptr;
struct fioblock *prev;
{
	register struct fioblock *blkptr;
	register int readsize;

	/*
	 * Deallocate any unused blocks in the buffer list.  An unused
	 * block is identified by an fldcnt of zero.
	 */
	freefiounused( ioptr );

	if ( (ioptr->flags & FIO_EOF) || (blkptr = newfioblk( )) == NULL )
		return( NULL );

	blkptr->curloc = blkptr->data;

	readsize = read( ioptr->fd, blkptr->data, FIOBLKSIZE );
	if ( readsize < 0 ) {
		freefioblk( blkptr );
		ioptr->flags |= FIO_ERROR|FIO_READ;
		return( NULL );
	}
	else if ( readsize == 0 ) {
		freefioblk( blkptr );
		ioptr->flags |= FIO_EOF;
		return( NULL );
	}
	else 
		ioptr->flags &= ~FIO_EOF;

	blkptr->end = &blkptr->data[readsize];

	ioptr->curblk = blkptr;

	if ( prev ) {
		blkptr->next = prev->next;
		prev->next = blkptr;
		if ( ioptr->readbuf == NULL )
			ioptr->readbuf = prev;
	}
	else {
		blkptr->next = ioptr->readbuf;
		ioptr->readbuf = blkptr;
	}

	return( blkptr );
}

freefiounused( ioptr )
register struct formatio *ioptr;
{
	register struct fioblock *blkptr, *next, **prev;

	prev = &ioptr->readbuf;
	for( blkptr = ioptr->readbuf; blkptr; blkptr = next ) {
		next = blkptr->next;
		if ( blkptr->fldcnt == 0 && blkptr != ioptr->curblk ) {
			*prev = next;
			freefioblk( blkptr );
		}
		else
			prev = &blkptr->next;
	}

	prev = &ioptr->savebuf;
	for( blkptr = ioptr->savebuf; blkptr; blkptr = next ) {
		next = blkptr->next;
		if ( blkptr->fldcnt == 0 ) {
			*prev = next;
			freefioblk( blkptr );
		}
		else
			prev = &blkptr->next;
	}
}

nullflds( fldvals, fldcnt, maxfld )
char **fldvals;
int fldcnt;
int maxfld;
{
	while( fldcnt < maxfld )
		fldvals[ fldcnt++ ] = nullattr;
}

static void
rmescape( src, delim )
register char *src;
char delim;
{
	register char *dest;

	while( *src && (*src != '\\' || src[1] != delim) )
		++src;
	dest = src;
	while( *src ) {
		if ( *src == '\\' && src[1] == delim )
			++src;
		*dest++ = *src++;
	}
	*dest = '\0';
}

getrec( ioptr, maxfld, flddelim, fldvals, incrcnt )
struct formatio *ioptr;
register int maxfld;
register char flddelim;
register char **fldvals;
int incrcnt;
{
	register char *bptr, *enddata;
	register struct fioblock *blkptr;
	register int fldcnt;
	register struct fioblock *nextblk;
	register char ch;
	int nextfld;
	char escape;
	char esclist[MAXATT];

	/*
	 * Read one tuple from an on-disk relation.
	 */
	if ( (ioptr->flags & FIO_INITDONE) == 0 || ioptr->fd < 0 ) {
		ioptr->flags |= FIO_ERROR|FIO_NOINIT;
		return( FALSE );
	}

	if ( ioptr->flags & (FIO_EOF|FIO_ERROR) )
		return( FALSE );

	if ( ioptr->readbuf == NULL ) {
		if ( (blkptr = readnextblk( ioptr, NULL )) == NULL )
			return( FALSE );
	}
	else
		blkptr = ioptr->curblk;

	fldcnt = 0;
	escape = FALSE;

	do {
		if ( fldcnt == maxfld - 1 )
			flddelim = '\n';	/* last delim is always '\n' */
		nextfld = fldcnt + 1;

		if ( blkptr->curloc >= blkptr->end ) {
			if ( (blkptr = readnextblk( ioptr, blkptr )) == NULL) {
				/*
				 * Premature end-of-file.  Ignore any current
				 * tuple.
				 */
				if ( fldcnt == 0 )
					return( FALSE );
				nullflds( fldvals, fldcnt - 1, maxfld );
				return( TRUE );
			}
		}

		esclist[fldcnt] = FALSE;
		bptr = blkptr->curloc;
		enddata = blkptr->end;

		nextblk = NULL;
		while( (ch = *bptr) != flddelim ) {
			if ( ch == '\\' ) {
				if ( ++bptr >= enddata ) {
					nextblk = readnextblk( ioptr, blkptr );
					if ( nextblk == NULL ) {
						nullflds( fldvals, fldcnt,
							maxfld );
						nextfld = maxfld;
						break;
					}
					bptr = nextblk->data;
					enddata = nextblk->end;
				}
				if ( *bptr == flddelim )
				{
					escape = TRUE;
					esclist[fldcnt] = TRUE;
				}
				++bptr;
				continue;
			}
			else if ( ch == '\n' ) {
				/*
				 * Premature end-of-line.
				 */
				++bptr;
				nullflds( fldvals, fldcnt, maxfld );
				nextfld = maxfld;
			}

			if ( ++bptr >= enddata ) {
				nextblk = readnextblk( ioptr, blkptr );
				if ( nextblk == NULL ) {
					nullflds( fldvals, fldcnt, maxfld );
					nextfld = maxfld;
					break;
				}
				bptr = nextblk->data;
				enddata = nextblk->end;
			}
		}
		if ( nextblk ) {		/* buffer overflow */
			fldvals[fldcnt] = copy2str( ioptr, blkptr->curloc,
						blkptr->end - blkptr->curloc,
						nextblk->data,
						bptr - nextblk->data,
						incrcnt );
			if ( fldvals[fldcnt] == NULL )
				fldvals[fldcnt] = nullattr;
			blkptr = nextblk;
		}
		else {
			fldvals[fldcnt] = blkptr->curloc;
			if ( incrcnt )
				blkptr->fldcnt++;
		}

		*bptr++ = '\0';
		blkptr->curloc = bptr;

		fldcnt = nextfld;
	} while( fldcnt < maxfld );

	if ( escape ) {
		for( fldcnt = 0; fldcnt < maxfld; fldcnt++ ) {
			if ( esclist[fldcnt] )
				rmescape( fldvals[fldcnt], flddelim );
		}
	}

	return( TRUE );
}

initformatio( ioptr, fd )
struct formatio *ioptr;
int fd;
{
	ioptr->fd = fd;
	ioptr->flags = fd >= 0 ? FIO_INITDONE : 0;
	ioptr->readbuf = ioptr->curblk = ioptr->savebuf = NULL;
}
