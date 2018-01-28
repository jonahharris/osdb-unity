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

#define BUFSIZE		4096

extern char *prog;

cprelation( ioptr, tmpfp )
struct urelio *ioptr;
FILE *tmpfp;
{
	char buf[BUFSIZE];
	int i;

	if ( ioptr->curblk ) {
		struct urelblock *blkptr;

		blkptr = ioptr->curblk;
		i = blkptr->end - blkptr->curloc;
		if ( fwrite( blkptr->curloc, 1, i, tmpfp ) != i )
		{
			set_uerror( UE_WRITE );
			return( FALSE );
		}
	}

	if ( ioptr->flags & UIO_EOF )
		return( TRUE );

	/*
	 * Copy table to temp file.
	 */
	while( 1 )
	{
		if ( ioptr->fd == 0 )	/* stdin */
		{
			i = fread( buf, 1, BUFSIZE, stdin );
			if ( ferror( stdin ) )
				i = -1;
		}
		else
			i = read( ioptr->fd, buf, BUFSIZE );

		if ( i < 0 )		/* error in the read */
		{
			set_uerror( UE_READ );
			return( FALSE );
		}
		else if ( i == 0 )	/* no more to read */
			break;

		if ( fwrite( buf, 1, i, tmpfp ) != i )
		{
			set_uerror( UE_WRITE );
			return( FALSE );
		}
	}

	return( TRUE );
}

static int
copyrelblk( ioptr, tmpfp, bufptr )
struct urelio *ioptr;
FILE *tmpfp;
char *bufptr;
{
	int i;

	if ( ioptr->flags & UIO_EOF )
		return( 0 );		/* We are already at EOF */

	if ( ioptr->fd == 0 )	/* stdin */
	{
		i = fread( bufptr, 1, BUFSIZE, stdin );
		if ( ferror( stdin ) )
			i = -1;
	}
	else
		i = read( ioptr->fd, bufptr, BUFSIZE );

	if ( i < 0 )		/* error in the read */
	{
		set_uerror( UE_READ );
		return( -1 );
	}
	else if ( i == 0 ) {	/* no more to read */
		ioptr->flags |= UIO_EOF;
		return( 0 );
	}

	if ( fwrite( bufptr, 1, i, tmpfp ) != i )
	{
		set_uerror( UE_WRITE );
		return( -1 );
	}

	return( i );
}


cprelationtc( ioptr, tmpfp, relptr )
struct urelio *ioptr;
FILE *tmpfp;
struct urelation *relptr;
{
	register char *p, *q;
	register unsigned short attrcnt;
	register struct uattribute *attrptr;
	unsigned short newattrcnt;
	unsigned int size;
	int i;
	char *pstart;
	char buf[BUFSIZE];

	if ( ioptr->fd < 0 ) {
		set_uerror( UE_NOINIT );
		return( FALSE );
	}

	i = 0;

	if ( ioptr->curblk ) {
		struct urelblock *blkptr;

		/* write the remainder of the current block */
		blkptr = ioptr->curblk;
		i = blkptr->end - blkptr->curloc;
		if ( fwrite( blkptr->curloc, 1, i, tmpfp ) != i )
		{
			set_uerror( UE_WRITE );
			return( FALSE );
		}
		p = blkptr->curloc;
	} else {
		p = buf;
	}

	reset_uerror( );        /* no errors, yet */

	q = p + i;

	/*
	 * Check and count tuples while coping to temp file.
	 */
	while( ( q > p ) || ( ( ioptr->flags & UIO_EOF ) == 0 ) )
	{
		if ( p >= q ) {
			p = buf;
			i = copyrelblk( ioptr, tmpfp, p );
			if ( i < 0 ) {
				return( FALSE );
			} else if ( i == 0 ) {
				return( TRUE );
			} else {
				q = p + i;
			}
		}

		++ioptr->tuplecnt;

		newattrcnt = 0;
		size = 0;

		/*
		 * Parse the individual attributes for the tuple.
		 */
		do {
			attrcnt = newattrcnt++;
			attrptr = &relptr->attrs[attrcnt];
	
			if ( p >= q ) {
				p = buf;
				i = copyrelblk( ioptr, tmpfp, p );
				if ( i <= 0 ) {
					/*
					 * Premature end-of-file or I/O error.
					 */
					if ( i == 0 ) {
						prmsg( MSG_ERROR,
							"relation %s, tuple #%lu:\n\tattribute %s (#%d) contains embedded new-line",
							relptr->path, ioptr->tuplecnt, attrptr->aname, attrcnt );

						set_uerror( UE_TPLERROR );
					}
					return( FALSE );
				} else {
					q = p + i;
				}
			}

			if ( attrptr->attrtype == UAT_TERMCHAR ) {
				register char termch;
	
				pstart = p;
				termch = (char)attrptr->terminate;
				while( *p != termch ) {
					if ( *p == '\\' ) {
						if ( ++p >= q ) {
							size += p - pstart;
							if ( size >= DBBLKSIZE ) {
								prmsg( MSG_ERROR,
									"relation %s, tuple #%lu:\n\tattribute %s (#%d) value is longer than maximum I/O block size (%u)",
									relptr->path, ioptr->tuplecnt, attrptr->aname, attrcnt, DBBLKSIZE );
								set_uerror( UE_ATTSIZE );
								return( FALSE );
							}
							p = buf;
							i = copyrelblk( ioptr, tmpfp, p );
							if ( i <= 0 ) {
								/*
								 * Premature end-of-file or I/O error.
								 */
								if ( i == 0 ) {
									prmsg( MSG_ERROR,
										"relation %s, tuple #%lu:\n\tattribute %s (#%d) contains embedded new-line",
										relptr->path, ioptr->tuplecnt, attrptr->aname, attrcnt );
									set_uerror( UE_TPLERROR );
								}
								return( FALSE );
							} else {
								pstart = p;
								q = p + i;
							}
						}
						/*
						 * We only want to remove escapes
						 * from the delim.
						 */
						if ( *p == termch && *p != '\n' && *p != '\0' )
						{
							if ( ++p >= q ) {
								size += p - pstart;
								if ( size >= DBBLKSIZE ) {
									prmsg( MSG_ERROR,
										"relation %s, tuple #%lu:\n\tattribute %s (#%d) value is longer than maximum I/O block size (%u)",
										relptr->path, ioptr->tuplecnt, attrptr->aname, attrcnt, DBBLKSIZE );
									set_uerror( UE_ATTSIZE );
									return( FALSE );
								}
								p = buf;
								i = copyrelblk( ioptr, tmpfp, p );
								if ( i <= 0 ) {
									/*
									 * Premature end-of-file or I/O error.
									 */
									if ( i == 0 ) {
										prmsg( MSG_ERROR,
											"relation %s, tuple #%lu:\n\tattribute %s (#%d) contains embedded new-line",
											relptr->path, ioptr->tuplecnt, attrptr->aname, attrcnt );
										set_uerror( UE_TPLERROR );
									}
									return( FALSE );
								} else {
									pstart = p;
									q = p + i;
								}
							}
						}
						continue;
					}
					else if ( *p == '\n' ) {
						/*
						 * Premature end-of-line.
						 */
						prmsg( MSG_ERROR,
							"relation %s, tuple #%lu:\n\tattribute %s (#%d) contains embedded new-line",
							relptr->path, ioptr->tuplecnt, attrptr->aname, attrcnt );
						set_uerror( UE_TPLERROR );
						return( FALSE );
					}

					if ( ++p >= q ) {
						size += p - pstart;
						if ( size >= DBBLKSIZE ) {
							prmsg( MSG_ERROR,
								"relation %s, tuple #%lu:\n\tattribute %s (#%d) value is longer than maximum I/O block size (%u)",
								relptr->path, ioptr->tuplecnt, attrptr->aname, attrcnt, DBBLKSIZE );
							set_uerror( UE_ATTSIZE );
							return( FALSE );
						}
						p = buf;
						i = copyrelblk( ioptr, tmpfp, p );
						if ( i <= 0 ) {
							/*
							 * Premature end-of-file or I/O error.
							 */
							if ( i == 0 ) {
								prmsg( MSG_ERROR,
									"relation %s, tuple #%lu:\n\tattribute %s (#%d) contains embedded new-line",
									relptr->path, ioptr->tuplecnt, attrptr->aname, attrcnt );
								set_uerror( UE_TPLERROR );
							}
							return( FALSE );
						} else {
							pstart = p;
							q = p + i;
						}
					}
				} /* end while( *p != termch */

				/* skip over term char and compute attribute size */
				size += ++p - pstart;
				if ( size > DBBLKSIZE ) {
					prmsg( MSG_ERROR,
						"relation %s, tuple #%lu:\n\tattribute %s (#%d) value is longer than maximum I/O block size (%u)",
						relptr->path, ioptr->tuplecnt, attrptr->aname, attrcnt, DBBLKSIZE );
					set_uerror( UE_ATTSIZE );
					return( FALSE );
				}
			}
			else {		/* UAT_FIXEDWIDTH */
				register int len;
				int eos;

				eos = 0;
				len = attrptr->terminate;
				if ( len >= DBBLKSIZE ) {
					prmsg( MSG_ERROR,
						"relation %s, tuple #%lu:\n\tattribute %s (#%d) value is longer than maximum I/O block size (%u)",
						relptr->path, ioptr->tuplecnt, attrptr->aname, attrcnt, DBBLKSIZE );
					set_uerror( UE_ATTSIZE );
					return( FALSE );
				}
				while( len-- > 0 ) {
					if ( p >= q ) {
						p = buf;
						i = copyrelblk( ioptr, tmpfp, p );
						if ( i <= 0 ) {
							/*
							 * Premature end-of-file or I/O error.
							 */
							if ( i == 0 ) {
								prmsg( MSG_ERROR,
									"relation %s, tuple #%lu:\n\tattribute %s (#%d) contains embedded new-line",
									relptr->path, ioptr->tuplecnt, attrptr->aname, attrcnt );
								set_uerror( UE_TPLERROR );
							}
							return( FALSE );
						} else {
							pstart = p;
							q = p + i;
						}
					}

					if ( ( *p == '\n' ) && ( ! ( eos ) ) )
					{
						static	int	nlinfw;	/* warn about NL in fixed width attribute */

						if ( nlinfw == 0 ) {
							fprintf(stderr, "%s: WARNING: Input table contains embedded NL in fixed width attribute!\n", prog );
							++nlinfw;
						}
					} else if ( *p == '\0' ) {
						++eos;
					}
					p++;
				}
			}
	
		} while( newattrcnt < relptr->attrcnt );

		/*
		 * Check if there is should be an extra new-line after all
		 * the attributes.  This will happen if that last attribute
		 * was a fixed width attribute.
		 */
		if ( relptr->attrs[relptr->attrcnt - 1].attrtype == UAT_FIXEDWIDTH )
		{
			if ( p >= q ) {
				p = buf;
				i = copyrelblk( ioptr, tmpfp, p );
				if ( i <= 0 ) {
					/*
					 * Premature end-of-file or I/O error.
					 */
					if ( i == 0 ) {
						prmsg( MSG_ERROR,
							"relation %s, tuple #%lu:\n\tfinal fixed-width attribute %s (#%d) is not followed by a new-line",
							relptr->path, ioptr->tuplecnt, attrptr->aname, relptr->attrcnt - 1 );
						set_uerror( UE_TPLERROR );
					}
					return( FALSE );
				} else {
					q = p + i;
				}
			}

			if ( *p != '\n' ) {
				prmsg( MSG_ERROR,
					"relation %s, tuple #%lu:\n\tfinal fixed-width attribute %s (#%d) is not followed by a new-line",
					relptr->path, ioptr->tuplecnt, attrptr->aname, relptr->attrcnt - 1 );
				set_uerror( UE_TPLERROR );
				return( FALSE );
			} else {
				++p;
			}
		}
	}

	return( TRUE );
}
