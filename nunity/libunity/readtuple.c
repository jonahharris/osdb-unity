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
#include "uquery.h"
#include "urelation.h"
#include "uerror.h"
#include "udbio.h"
#include "message.h"
#ifdef DEBUG
#include "qdebug.h"
#endif

/*
 * Below is the minimum space that must be free before
 * we'll try to fill up an existing block instead of
 * allocating a new block.  This is in essence a minimum
 * read size.
 */
#define MINFREESPACE	50

extern	char *prog;

struct urelblock *
readioblk( ioptr, seekval )
struct urelio *ioptr;
long seekval;
{
	register struct urelblock *blkptr;
	register int readsize;

	if ( (ioptr->flags & UIO_EOF) || (blkptr = newrelblk( )) == NULL )
		return( NULL );

	blkptr->seekval = seekval;
	blkptr->curloc = blkptr->data;

	if ( ioptr->fd == 0 )	/* stdin */
	{
		readsize = fread( blkptr->data, 1, DBBLKSIZE, stdin );
		if ( ferror( stdin ) )
			readsize = -1;
	}
	else
		readsize = read( ioptr->fd, blkptr->data, DBBLKSIZE );

	if ( readsize < 0 )
	{
		freerelblk( blkptr );
		ioptr->flags |= UIO_ERROR;
		set_uerror( UE_READ );
		return( NULL );
	}
	else if ( readsize == 0 )
	{
		freerelblk( blkptr );
		ioptr->flags |= UIO_EOF;
		return( NULL );
	}
	else
		ioptr->flags &= ~UIO_EOF;

	blkptr->end = &blkptr->data[readsize];

	ioptr->curblk = blkptr;

	return( blkptr );
}

struct urelblock *
readnextblk( ioptr, prevblk )
struct urelio *ioptr;
struct urelblock *prevblk;
{
	register struct urelblock *blkptr;
	register long seekval;
	register int readsize;

	if ( prevblk ) {
		seekval = prevblk->seekval + (prevblk->end - prevblk->data);
		blkptr = prevblk->next;
	}
	else {
		seekval = 0;
		blkptr = ioptr->readbuf;
	}

#ifdef DEBUG
	if ( _qdebug & QDBG_IO ) {
		prmsg( MSG_DEBUG, "reading next ioblock 0x%lx, seek %ld",
			ioptr, seekval );
	}
#endif

	if ( blkptr && blkptr->seekval <= seekval &&
			seekval < blkptr->seekval + (blkptr->end - blkptr->data) )
		return( blkptr );

	if ( prevblk && (readsize = DBBLKSIZE - (prevblk->end - prevblk->data)) >= MINFREESPACE ) {
		/*
		 * The current block is not full, so use the same block
		 * and read a partial block.  This case occurs most
		 * often when the relation is from stdin, but it can
		 * also happen with named pipes.
		 */
		if ( ioptr->flags & UIO_EOF )
			return( NULL );

		if ( ioptr->fd == 0 )	/* stdin */
		{
			readsize = fread( prevblk->end, 1, readsize, stdin );
			if ( ferror( stdin ) )
				readsize = -1;
		}
		else
			readsize = read( ioptr->fd, prevblk->end, readsize );

		if ( readsize < 0 )
		{
			ioptr->flags |= UIO_ERROR;
			set_uerror( UE_READ );
			return( NULL );
		}
		else if ( readsize == 0 )
		{
			ioptr->flags |= UIO_EOF;
			return( NULL );
		}
		else 
			ioptr->flags &= ~UIO_EOF;

		prevblk->curloc = prevblk->end;
		prevblk->end = &prevblk->end[readsize];

		ioptr->curblk = prevblk;

		return( prevblk );
	}

	/*
	 * Any previous block is full.  Read a new full block.
	 */
	if ( (blkptr = readioblk( ioptr, seekval )) == NULL )
		return( NULL );

	if ( prevblk ) {
		blkptr->next = prevblk->next;
		prevblk->next = blkptr;
		if ( ioptr->readbuf == NULL )
			ioptr->readbuf = prevblk;
	}
	else {
		blkptr->next = ioptr->readbuf;
		ioptr->readbuf = blkptr;
	}

	return( blkptr );
}

freeunusedblks( ioptr )
struct urelio *ioptr;
{
	register struct urelblock *blkptr, *next, **prev;

	prev = &ioptr->readbuf;
	for( blkptr = ioptr->readbuf; blkptr; blkptr = next ) {
		next = blkptr->next;
		if ( blkptr->attrcnt == 0 && blkptr != ioptr->curblk ) {
			*prev = next;
			freerelblk( blkptr );
		}
		else
			prev = &blkptr->next;
	}

	prev = &ioptr->savebuf;
	for( blkptr = ioptr->savebuf; blkptr; blkptr = next ) {
		next = blkptr->next;
		if ( blkptr->attrcnt == 0 ) {
			*prev = next;
			freerelblk( blkptr );
		}
		else
			prev = &blkptr->next;
	}
}

static struct urelblock *
findfreespace( blklist, size )
struct urelblock **blklist;
register int size;
{
	register struct urelblock *blkptr;

	if ( size > DBBLKSIZE )
	{
		set_uerror( UE_ATTSIZE );
		return( NULL );
	}

	if ( *blklist == NULL ) {
		if ( (*blklist = newrelblk( )) == NULL )
			return( NULL );
		else
			return( *blklist );
	}

	for( blkptr = *blklist; blkptr; blkptr = blkptr->next ) {
		if ( &blkptr->data[DBBLKSIZE] - blkptr->curloc >= size )
			return( blkptr );  /* found a block it will fit in */
		else if ( blkptr->next == NULL ) {
			blkptr->next = newrelblk( );
			return( blkptr->next );
		}
	}

	return( NULL );  /* can never happen -- used to keep compiler happy */
}

static char nullattr[] = "";

char *
copy2str( blklist, src1, len1, src2, len2, padlen, incrcnt, tplptr )
struct urelblock **blklist;
char *src1;
int len1;
char *src2;
int len2;
int padlen;
int incrcnt;
struct utuple *tplptr;
{
	struct urelblock *blkptr;
	char *start;

	if ( len1 + len2 + 1 > DBBLKSIZE )
	{
		if ( len1 > DBBLKSIZE - 1 )
		{
			len1 = DBBLKSIZE - 1;
			len2 = 0;
		}
		else
			len2 = DBBLKSIZE - 1 - len1;

		tplptr->flags |= TPL_ERRBIGATT;
	}

	blkptr = findfreespace( blklist, len1 + len2 + 1 );
	if ( blkptr == NULL )
	{
		tplptr->flags |= TPL_ERRNOMEM;
		return( nullattr );
	}

	start = blkptr->curloc;

	if ( len1 > 0 )
	{
		if ( padlen <= len2 )
		{
			memcpy( blkptr->curloc, src1, len1 );
			blkptr->curloc += len1;
		}
		else
		{
			memcpy( blkptr->curloc, src1, len1 + len2 - padlen );
			blkptr->curloc += len1 + len2 - padlen;
		}
	}
	if ( len2 - padlen > 0 )
	{
		memcpy( blkptr->curloc, src2, len2 - padlen );
		blkptr->curloc += len2 - padlen;
	}

	while( padlen-- > 0 )		/* pad string with spaces */
		*blkptr->curloc++ = ' ';

	*blkptr->curloc++ = '\0';	/* null terminate the string */
	if ( incrcnt )
		blkptr->attrcnt++;

	return( start );
}

char *
copystr( blklist, src, len, padlen, incrcnt, tplptr )
struct urelblock **blklist;
char *src;
int len;
int padlen;
int incrcnt;
struct utuple *tplptr;
{
	struct urelblock *blkptr;
	char *start;

	if ( len > DBBLKSIZE - 1 )
	{
		len = DBBLKSIZE - 1;

		tplptr->flags |= TPL_ERRBIGATT;
	}

	blkptr = findfreespace( blklist, len + 1 );
	if ( blkptr == NULL )
	{
		tplptr->flags |= TPL_ERRNOMEM;
		return( nullattr );
	}

	start = blkptr->curloc;

	if ( len - padlen > 0 )
	{
		memcpy( blkptr->curloc, src, len - padlen );
		blkptr->curloc += len - padlen;
	}
	while( padlen-- > 0 )		/* pad string with space */
		*blkptr->curloc++ = ' ';

	*blkptr->curloc++ = '\0';	/* null terminate the string */
	if ( incrcnt )
		blkptr->attrcnt++;

	return( start );
}

unusedtuple( ioptr, attrvals, attrcnt )
struct urelio *ioptr;
char **attrvals;
unsigned int attrcnt;
{
	struct urelblock *readlist;
	register struct urelblock *blkptr;
	register int found;
	register char *val;
	char **endattr;

	readlist = ioptr->readbuf;
	for( endattr = &attrvals[attrcnt]; attrvals < endattr; attrvals++ )
	{
		if ( (val = *attrvals) == NULL || val == nullattr )
			continue;
		found = FALSE;

		for( blkptr = readlist; blkptr; blkptr = blkptr->next ) {
			if ( val < blkptr->end && val >= blkptr->data ) {
				blkptr->attrcnt--;
				readlist = blkptr;
				found = TRUE;
				break;
			}
		}
		if ( found )
			continue;

		for( blkptr = ioptr->savebuf; blkptr; blkptr = blkptr->next ) {
			if ( val < blkptr->end && val >= blkptr->data ) {
				blkptr->attrcnt--;
				readlist = blkptr;
				found = TRUE;
				break;
			}
		}
		if ( found )
			continue;

		for( blkptr = ioptr->readbuf; blkptr != readlist;
				blkptr = blkptr->next ) {
			if ( val < blkptr->end && val >= blkptr->data ) {
				blkptr->attrcnt--;
				readlist = blkptr;
				break;
			}
		}
	}
}

static
setbadattr( tplptr, relptr, badattr, error, can_continue )
struct utuple *tplptr;
struct urelation *relptr;
unsigned short badattr;	/* attr where error occured */
unsigned short error;	/* the error that occured */
int can_continue;	/* can we keep going after this error? */
{
	struct uattribute *attrptr;	/* current attribute */

	tplptr->flags |= error;
	switch( uerror ) {
	case UE_NOMEM:
		tplptr->flags |= TPL_ERRNOMEM;
		break;
	case UE_READ:
		tplptr->flags |= TPL_ERRREAD;
		break;
	}
	if ( tplptr->badattr == TPL_NOBADATTR )
		tplptr->badattr = badattr;

	for( attrptr = &relptr->attrs[badattr]; badattr < relptr->attrcnt;
		attrptr++, badattr++ )
	{
		tplptr->tplval[badattr] = nullattr;
		if ( can_continue && attrptr->attrtype == UAT_TERMCHAR &&
				attrptr->terminate == '\n' )
		{
			badattr++;	/* move past this attribute */
			break;
		}
	}

	return( badattr );
}

int	uchecktuple;
long	utplerrors;

readtuple( relptr, ioptr, tplptr, attrflags )
struct urelation *relptr;
struct urelio *ioptr;
struct utuple *tplptr;
unsigned char *attrflags;
{
	extern	int	utplmsgtype;
	extern	long	utplelimit;
	register char *bptr, *enddata;
	register struct urelblock *blkptr;
	register struct urelblock *nextblk;
	register struct uattribute *attrptr;
	register unsigned short attrcnt;
	char **attrvals;
	unsigned short newattrcnt;
	unsigned int size;	/* size of tuple */
	char *attrstart;	/* start of each attribute */
	char escape;		/* backslash in tuple? */
	char esclist[MAXATT];	/* backslash in each attribute? */

	/*
	 * Read one tuple from an on-disk relation.
	 */
	if ( ioptr->fd < 0 ) {
		set_uerror( UE_NOINIT );
		return( FALSE );
	}

	reset_uerror( );	/* no errors, yet */
	tplptr->flags = 0;
	tplptr->badattr = TPL_NOBADATTR;

	if ( ioptr->flags & UIO_ERROR )	/* add UIO_EOF? */
		return( FALSE );

	if ( ioptr->readbuf == NULL ) {
		if ( (blkptr = readnextblk( ioptr,
					(struct urelblock *)NULL )) == NULL )
			return( FALSE );
	}
	else
	{
		/*
		 * Deallocate any unused blocks in the buffer list.
		 * An unused block is identified by an attrcnt of zero.
		 */
		freeunusedblks( ioptr );

		blkptr = ioptr->curblk;
	}

	tplptr->lseek = blkptr->seekval + (blkptr->curloc - blkptr->data);
	tplptr->tuplenum = ++ioptr->tuplecnt;

	attrvals = tplptr->tplval;
	newattrcnt = 0;
	size = 0;
	escape = FALSE;

	/*
	 * Parse the individual attributes for the tuple.
	 */
	do {
		attrcnt = newattrcnt++;
		attrptr = &relptr->attrs[attrcnt];

		if ( blkptr->curloc >= blkptr->end ) {
			if ( (blkptr = readnextblk( ioptr, blkptr )) == NULL) {
				/*
				 * Premature end-of-file or memory error.
				 */
				if ( attrcnt == 0 )
					return( FALSE );
				(void)setbadattr( tplptr, relptr,
						attrcnt - 1, TPL_SYNPREMEOF,
						FALSE );
				if ( uchecktuple ) {
					if ( uchecktuple == UCHECKFATAL )
					{
						/* make sure ERROR report is printed */
						int msgtype = utplmsgtype;
						utplmsgtype = MSG_ERROR;
						if (utplelimit == 0)
							++utplelimit;
						(void)prtplerror( relptr, tplptr );
						utplmsgtype = msgtype;
						set_uerror( UE_TPLERROR );
						return( FALSE );
					}
					++utplerrors;
					if ( uchecktuple != UCHECKCOUNT ) {
						(void)prtplerror( relptr, tplptr );
					}
				}
				return( TRUE );
			}
		}

		esclist[attrcnt] = '\0';

		attrstart = bptr = blkptr->curloc;
		enddata = blkptr->end;
		nextblk = NULL;

		if ( attrptr->attrtype == UAT_TERMCHAR ) {
			register char termch;

			termch = (char)attrptr->terminate;
			while( *bptr != termch ) {
				if ( *bptr == '\\' ) {
					if ( ++bptr >= enddata ) {
						if ( nextblk != NULL )
						{
							set_uerror( UE_ATTSIZE );
							newattrcnt = setbadattr(
									tplptr,
									relptr,
									attrcnt,
									TPL_ERRBIGATT,
									FALSE );
							break;
						}
						nextblk = readnextblk( ioptr,
								blkptr );
						if ( nextblk == NULL ) {
							/*
							 * No more blocks or
							 * we're out of memory.
							 */
							newattrcnt = setbadattr(
									tplptr,
									relptr,
									attrcnt,
									TPL_SYNPREMEOF,
									FALSE );
							break;	/* end attr */
						}
						bptr = nextblk->curloc;
						enddata = nextblk->end;
					}
					/*
					 * We only want to remove escapes
					 * from the delim.
					 */
					if ( *bptr == termch && *bptr != '\n' && *bptr != '\0' )
					{
						if ( ++bptr >= enddata ) {
							if ( nextblk != NULL )
							{
								set_uerror( UE_ATTSIZE );
								newattrcnt = setbadattr(
										tplptr,
										relptr,
										attrcnt,
										TPL_ERRBIGATT,
										FALSE );
								break;
							}
							nextblk = readnextblk( ioptr,
									blkptr );
							if ( nextblk == NULL ) {
								/*
								 * No more blocks or
								 * we're out of memory.
								 */
								newattrcnt = setbadattr(
										tplptr,
										relptr,
										attrcnt,
										TPL_SYNPREMEOF,
										FALSE );
								break;	/* end attr */
							}
							bptr = nextblk->curloc;
							enddata = nextblk->end;
						}

						if ( attrflags == NULL ||
							attrflags[attrcnt] )
						{
							escape = TRUE;
							esclist[attrcnt] = termch;
						}
					}
					continue;
				}
				else if ( *bptr == '\n' ) {
					/*
					 * Premature end-of-line.
					 */
					newattrcnt = setbadattr( tplptr,
							relptr, attrcnt,
							TPL_SYNEMBEDNL, TRUE );
					break;	/* end this attribute */
				}

				if ( ++bptr >= enddata ) {
					if ( nextblk != NULL )
					{
						set_uerror( UE_ATTSIZE );
						newattrcnt = setbadattr(
								tplptr,
								relptr,
								attrcnt,
								TPL_ERRBIGATT,
								FALSE );
						break;
					}
					nextblk = readnextblk( ioptr, blkptr );
					if ( nextblk == NULL ) {
						/*
						 * No more blocks or we're
						 * out of memory.
						 */
						newattrcnt = setbadattr(
								tplptr,
								relptr,
								attrcnt,
								TPL_SYNPREMEOF,
								FALSE );
						break;	/* end this attr */
					}
					bptr = nextblk->curloc;
					enddata = nextblk->end;
				}
			} /* end while( *bptr != termch */

			/* compute the attribute size */
			if ( nextblk && nextblk != blkptr ) {
				/* buffer overflow */
				size += (bptr - nextblk->data) +
					(blkptr->end - blkptr->curloc);
			}
			else {
				size += bptr - attrstart + 1;
			}

			if ( attrflags && attrflags[attrcnt] == '\0' )
			{
				/* don't save the attr */
				attrvals[attrcnt] = NULL;
				if ( nextblk )
					blkptr = nextblk;
			}
			else if ( nextblk && nextblk != blkptr )
			{
				/* buffer overflow */
				attrvals[attrcnt] = copy2str( &ioptr->savebuf,
							blkptr->curloc,
							blkptr->end - blkptr->curloc,
							nextblk->data,
							bptr - nextblk->data,
							0,
							attrflags != NULL,
							tplptr );
				blkptr = nextblk;
			}
			else {
				attrvals[attrcnt] = attrstart;
				if ( attrflags )
					blkptr->attrcnt++;
			}

			*bptr++ = '\0';		/* skip termch */
			blkptr->curloc = bptr;
		}
		else {		/* UAT_FIXEDWIDTH */
			register int len;
			int remain, left, eos;

			remain = len = attrptr->terminate;
			left = eos = 0;
			while( len-- > 0 ) {
				if ( bptr >= enddata ) {
					/*
					 * Attribute value crossed block
					 * boundary.
					 */
					if ( nextblk != NULL )
					{
						set_uerror( UE_ATTSIZE );
						newattrcnt = setbadattr(
								tplptr,
								relptr,
								attrcnt,
								TPL_ERRBIGATT,
								FALSE );
						break;
					}
					nextblk = readnextblk( ioptr, blkptr );
					if ( nextblk == NULL ) {
						/*
						 * No more blocks or we're
						 * out of memory.
						 */
						newattrcnt = setbadattr(
								tplptr,
								relptr,
								attrcnt,
								TPL_SYNPREMEOF,
								FALSE );
						break;	/* end this attr */
					}
					left = len + 1;
					remain -= left;
					bptr = nextblk->curloc;
					enddata = nextblk->end;
				}

				if ( ( *bptr == '\n' ) && ( ! ( eos ) ) )
				{
					static	int	nlinfw;	/* warn about NL in fixed width attribute */
#if	0
					/*
					 * Premature end-of-line.
					 */
					newattrcnt = setbadattr( tplptr,
							relptr, attrcnt,
							TPL_SYNEMBEDNL, TRUE );
					break;	/* end this attribute */
#endif
					if ( nlinfw == 0 ) {
						fprintf(stderr, "%s: WARNING: Input table contains embedded NL in fixed width attribute!\n", prog );
						++nlinfw;
					}
				} else if ( *bptr == '\0' ) {
					++eos;
				}
				bptr++;
			}
			size += attrptr->terminate + 1;

			if ( attrflags && attrflags[attrcnt] == '\0' )
			{
				attrvals[attrcnt] = NULL;
				if ( nextblk )
					blkptr = nextblk;
			}
			else if ( nextblk == NULL || nextblk == blkptr )
			{
				attrvals[attrcnt] = copystr( &ioptr->savebuf,
							attrstart,
							(int)attrptr->terminate,
							len > 0 ? len + 1 : 0,
							attrflags != NULL,
							tplptr );
			}
			else
			{
				/* buffer overflow */
				attrvals[attrcnt] = copy2str( &ioptr->savebuf,
							blkptr->curloc,
							remain,
							nextblk->data,
							left,
							len > 0 ? len + 1 : 0,
							attrflags != NULL,
							tplptr );
				blkptr = nextblk;
			}
			blkptr->curloc = bptr;
		}

	} while( newattrcnt < relptr->attrcnt );

	/*
	 * Check if there is should be an extra new-line after all
	 * the attributes.  This will happen if that last attribute
	 * was a fixed width attribute.
	 */
	if ( relptr->attrs[relptr->attrcnt - 1].attrtype == UAT_FIXEDWIDTH )
	{
		if ( blkptr->curloc >= blkptr->end )
		{
			blkptr = readnextblk( ioptr, blkptr );
			if ( blkptr == NULL )
				tplptr->flags |= TPL_SYNPREMEOF;
		}
		if ( blkptr && *blkptr->curloc == '\n' )
			++blkptr->curloc;
		else
			tplptr->flags |= TPL_SYNFWNONL;
	}

	if ( escape )	/* remove backslashes from the attribute values */
	{
		for( attrcnt = 0; attrcnt < relptr->attrcnt; attrcnt++ )
		{
			if ( esclist[attrcnt] != '\0' )
				rmescape( attrvals[attrcnt], esclist[attrcnt]);
		}
	}

	if ( ( uchecktuple ) && ( tplptr->flags & TPL_ERRORMSK ) ) {
		if ( uchecktuple == UCHECKFATAL )
		{
			/* make sure ERROR report is printed */
			int msgtype = utplmsgtype;
			utplmsgtype = MSG_ERROR;
			if (utplelimit == 0)
				++utplelimit;
			(void)prtplerror( relptr, tplptr );
			utplmsgtype = msgtype;
			set_uerror( UE_TPLERROR );
			return( FALSE );
		}
		++utplerrors;
		if ( uchecktuple != UCHECKCOUNT ) {
			(void)prtplerror( relptr, tplptr );
		}
	}

	return( TRUE );
}

rmescape( src, delim )
register char *src;
char delim;
{
	register char *dest;

	while( *src && (*src != '\\' || src[1] != delim) )
		++src;
	dest = src;
	while( *src )
	{
		if ( *src == '\\' && src[1] == delim )
			++src;
		*dest++ = *src++;
	}
	*dest = '\0';
}
