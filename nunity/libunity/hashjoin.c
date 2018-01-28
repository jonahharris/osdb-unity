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
#include <ctype.h>
#include "uquery.h"
#include "uerror.h"
#include "hash.h"

#ifdef DEBUG
#include "message.h"
#include "qdebug.h"
#endif

extern char *calloc();
extern struct utuple *gettuple();
extern unsigned long init_hash( );
extern unsigned long strhash( );
extern unsigned long datehash( );	/* no longer used ??? */
extern unsigned long nocasehash( );
extern unsigned long numhash( );
extern unsigned long end_hash( );

/*
 * buckets for hash table - must be power of 2
 */
#define MAXBUCKET	1024
#define BUCKETMASK	(MAXBUCKET - 1)

#define MAXCOMPARE	20	/* max compares of each type */

struct tplhash {
	unsigned long hashval;
	struct utuple *tplptr;
	struct tplhash *next;
};

#ifdef DEBUG
static void
prhashtbl( hashtbl, maxhash )
register struct tplhash **hashtbl;
int maxhash;
{
	register struct tplhash *hash1, *hash2;
	register int i, cnt;

	prmsg( MSG_DEBUG, "hash table contents:" );
	for( i = 0; i < maxhash; i++ )
	{
		if ( hashtbl[i] == NULL )
			continue;

		cnt = 0;
		for( hash2 = hashtbl[i]; hash2; hash2 = hash1 )
		{
			for( hash1 = hash2->next;
				hash1 && hash1->hashval == hash2->hashval;
				hash1 = hash1->next )
			{
				cnt++;
			}
		}
		prmsg( MSG_ERRASIS, "\tBucket %d: %d entr%s ", i, cnt,
			cnt == 1 ? "y" :"ies: " );
		for( hash2 = hashtbl[i]; hash2; hash2 = hash1 )
		{
			cnt = 0;
			for( hash1 = hash2->next;
				hash1 && hash1->hashval == hash2->hashval;
				hash1 = hash1->next )
			{
				cnt++;
			}
			prmsg( MSG_ERRASIS, " (%d@%08lx)", cnt,
				hash2->hashval );
		}
		prmsg( MSG_ERRASIS, "\n" );
	}
}
#endif

static struct tplhash *
findhash( hashptr, hashval )
register struct tplhash *hashptr;
register unsigned long hashval;
{
	for( ; hashptr != NULL && hashptr->hashval <= hashval;
		hashptr = hashptr->next )
	{
		if ( hashptr->hashval == hashval )
			return( hashptr );
	}

	return( NULL );
}

static void
find_cmptype( operptr, attrs1, attrs2, funclist, acmod1, acmod2 )
struct uqoperation *operptr;
short *attrs1;
short *attrs2;
unsigned long (**funclist)();
char *acmod1;		/* attribute/compare modifiers for attrs1 */
char *acmod2;		/* attribute/compare modifiers for attrs2 */
{
	register int i;
	register struct queryexpr *qptr;

	for( i = 0; i < operptr->cmpcnt - operptr->nonequal; i++, qptr++ )
	{
		qptr = operptr->cmplist[i];

		if ( operptr->node1 == qptr->elem1.attr.rel )
		{
			*attrs1++ = qptr->elem1.attr.attr;
			*attrs2++ = qptr->elem2.attr.attr;
			*acmod1++ = qptr->modifier1;
			*acmod2++ = qptr->modifier2;
		}
		else
		{
			*attrs2++ = qptr->elem1.attr.attr;
			*attrs1++ = qptr->elem2.attr.attr;
			*acmod2++ = qptr->modifier1;
			*acmod1++ = qptr->modifier2;
		}

		switch( qptr->cmptype ) {
		case QCMP_CASELESS:
			*funclist++ = hash_nocase_attr;
			break;
		case QCMP_NUMBER:
			*funclist++ = hash_num_attr;
			break;
		case QCMP_DATE:
			*funclist++ = hash_date_attr;
			break;
		case QCMP_DATEONLY:
			*funclist++ = hash_dateonly_attr;
			break;
		case QCMP_STRING:
		default:
			*funclist++ = hash_str_attr;
			break;
		}
	}
}

long
hashjoin( query, jinfo, operptr )
struct uquery *query;
struct joininfo *jinfo;
struct uqoperation *operptr;
{
	register struct utuple *tpl1, *tpl2;
	register int i, rc;
	register long joincnt, delcnt;
	register unsigned long hashval;
	unsigned long (*funclist[ MAXCOMPARE ])();
	short attrs1[MAXCOMPARE];
	short attrs2[MAXCOMPARE];
	char  acmod1[MAXCOMPARE];	/* attribute/compare modifiers for attrs1 */
	char  acmod2[MAXCOMPARE];	/* attribute/compare modifiers for attrs2 */
	struct tplhash *hashtbl[MAXBUCKET];

	if ( operptr->cmpcnt > MAXCOMPARE )
	{
		set_uerror( UE_QUERYEVAL );
		return( -1L );
	}

	rc = TRUE;
	joincnt = 0L;
	delcnt = 0L;

	memset( hashtbl, NULL, sizeof( hashtbl ) );

	find_cmptype( operptr, attrs1, attrs2, funclist, acmod1, acmod2 );

	if ( ! relreset( operptr->node1 ) )
		return( -1L );
	if ( operptr->node1->relio.fd >= 0 )  /* force save in addtuple() */
		operptr->node1->flags |= N_FORCESAVE;

	while( (tpl1 = gettuple( operptr->node1 )) != NULL )
	{
		hashval = INIT_HASH();
		for( i = 0; i < operptr->cmpcnt - operptr->nonequal; i++ )
		{
			hashval = (*funclist[i])( hashval, tpl1, attrs1[ i ], acmod1[ i ] );
		}
		END_HASH( hashval );

		if ( ! savehash( &hashtbl[ hashval & BUCKETMASK ], tpl1,
				hashval ) )
		{
			return( -1L );
		}
	}

#ifdef DEBUG
	if ( _qdebug & QDBG_HASH )
		prhashtbl( hashtbl, MAXBUCKET );
#endif

	if ( ( uerror != UE_NOERR ) || ! relreset( operptr->node2 ) )
		return( -1L );

	if ( operptr->node2->relio.fd >= 0 )  /* force save in addtuple() */
		operptr->node2->flags |= N_FORCESAVE;
	rc = TRUE;
	while( rc && (tpl2 = gettuple( operptr->node2 )) != NULL )
	{
		register struct tplhash *hashptr;

		hashval = INIT_HASH();
		for( i = 0; i < operptr->cmpcnt - operptr->nonequal; i++ )
		{
			hashval = (*funclist[i])( hashval, tpl2, attrs2[ i ], acmod2[ i ] );
		}
		END_HASH( hashval );

		hashptr = findhash( hashtbl[ hashval & BUCKETMASK ], hashval );
		while ( rc && hashptr != NULL )
		{
			tpl1 = hashptr->tplptr;

			if ( chkjoinlist( operptr->cmplist, operptr->cmpcnt,
					operptr->node1, operptr->node2,
					tpl1, tpl2 ) )
			{
				tpl1->flags |= TPL_JOINED|TPL_ACCESSED;
				tpl2->flags |= TPL_JOINED|TPL_ACCESSED;
				joincnt++;
				if ( (*jinfo->joinfunc)( query, jinfo->snode,
						operptr->node1, operptr->node2,
						tpl1, tpl2 ) <= 0 )
				{
					rc = FALSE;
				}
			}
			hashptr = findhash( hashptr->next, hashval );
		}

		if ( ! rc )
			break;

		if ( (tpl2->flags & TPL_JOINED) == TPL_JOINED ||
			(tpl2->flags & (TPL_DELETE|TPL_ACCESSED)) == TPL_ACCESSED )
		{
			tpl2->flags &= ~(TPL_JOINED|TPL_ACCESSED);
			if ( (*jinfo->addfunc2)( query,
					operptr->node2, tpl2 ) <= 0 )
			{
				rc = FALSE;
				break;
			}
		}
		else
		{
			delcnt++;
			if ( (*jinfo->delfunc2)( query,
					operptr->node2, tpl2, jinfo,
					operptr ) <= 0 )
			{
				rc = FALSE;
				break;
			}
		}
	}

	if ( ! rc )
		return( ( uerror != UE_NOERR ) ? -1L : joincnt );
	else if ( uerror != UE_NOERR )
		return( -1L );

	/*
	 * Clean up the hash table and delete any tuples from node1
	 * that did not participate in the join.
	 */
	for( i = 0; i < MAXBUCKET; i++ )
	{
		register struct tplhash *hashptr;

		for( hashptr = hashtbl[i]; hashptr != NULL;
			hashptr = hashptr->next )
		{
			tpl1 = hashptr->tplptr;

			if ( (tpl1->flags & TPL_JOINED) == TPL_JOINED ||
					(tpl1->flags & (TPL_DELETE|TPL_ACCESSED)) == TPL_ACCESSED )
			{
				tpl1->flags &= ~(TPL_JOINED|TPL_ACCESSED);
				if ( (*jinfo->addfunc1)( query,
						operptr->node1, tpl1 ) <= 0 )
				{
					rc = FALSE;
				}
			}
			else
			{
				delcnt++;
				if ( (*jinfo->delfunc1)( query,
						operptr->node1, tpl1,
						jinfo, operptr ) <= 0 )
				{
					rc = FALSE;
				}
			}
		}
	}

	operptr->node1->flags &= ~N_FORCESAVE;
	operptr->node2->flags &= ~N_FORCESAVE;

	/*
	 * Decide what to return.  If there was an error, return -1;
	 * otherwise, return the appropriate cnt.
	 */
	if ( ! rc )
		return( -1L );

	switch( operptr->oper ) {
	case UQOP_JOIN:
		return( joincnt );
	case UQOP_OUTERJOIN:
		return( joincnt + delcnt );
	case UQOP_ANTIJOIN:
		return( delcnt );
	default:
		return( -1L );
	}
}

#define HSHVALBLKSIZE	341	/* hash structs per blk, to get 4K blk */

struct hashvalblk {
	struct hashvalblk *next;
	struct tplhash entries[ HSHVALBLKSIZE ];
};

static struct hashvalblk *roothash, *curhash;
static short curhshcnt;

static
init_tplhash()
{
	curhash = roothash;
	curhshcnt = 0;
}

static
savehash( saveptr, tplptr, hashval )
register struct tplhash **saveptr;
struct utuple *tplptr;
unsigned long hashval;
{
	register struct tplhash *hashptr;

	if ( curhash == NULL || curhshcnt >= HSHVALBLKSIZE )
	{
		struct hashvalblk **prevloc;

		if ( curhash != NULL )
		{
			prevloc = &curhash->next;
			curhash = *prevloc;
		}
		else
		{
			prevloc = &roothash;
		}

		if ( curhash == NULL )
		{
			curhash = (struct hashvalblk *)calloc( 1, sizeof( *curhash ) );
			if ( curhash == NULL )
			{
				set_uerror( UE_NOMEM );
				return( FALSE );
			}
			*prevloc = curhash;
		}
		curhshcnt = 0;
	}

	hashptr = &curhash->entries[ curhshcnt++ ];
	hashptr->hashval = hashval;
	hashptr->tplptr = tplptr;

	for( ; *saveptr != NULL && (*saveptr)->hashval < hashval;
		saveptr = &(*saveptr)->next )
	{
		;
	}

	hashptr->next = *saveptr;
	*saveptr = hashptr;

	return( TRUE );
}
