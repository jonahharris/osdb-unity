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
#include <string.h>
#include "uquery.h"
#include "uerror.h"
#include "hash.h"

#ifdef	__STDC__
extern unsigned long strtoul( );
#else
extern long strtol( );
#endif

extern double atof( );
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#else
extern char *calloc( );
#endif

static int
splitflds( flds, srcattr, delim, maxatt )
char **flds;
char *srcattr;
char *delim;
int maxatt;
{
	char *ptr;
	int i;

	if ( delim == NULL || *delim == '\0' ) {
		flds[0] = srcattr;
		return( 1 );
	}

	for( i = 0; i < maxatt; i++ ) {
		flds[i] = srcattr;

		ptr = strchr( srcattr, *delim );
		if ( ptr != NULL ) {
			*ptr = '\0';
			srcattr = ptr + 1;
			if ( delim[1] != '\0' )
				delim++;
		}
		else {
			++i;
			break;
		}
	}

	flds[i] = NULL;

	return( i );
}

static void
restoreflds( flds, delim, cnt )
char **flds;
char *delim;
int cnt;
{
	int i;

	for( i = 1; i < cnt; i++ ) {
		flds[i][-1] = *delim;
		if ( delim[1] != '\0' )
			++delim;
	}
}

static char *
cpsubfld( projptr, sortptr, buf, max, fldval, fldnum, strtchar )
struct qprojection *projptr;
struct qsubsort *sortptr;
char *buf;
char *max;
char *fldval;
int fldnum;
int strtchar;
{
	int len;

	if ( buf >= max )
		return( buf );

	if ( projptr->flags & QP_RMBLANKS )
	{
		while( isspace( *fldval ) )
			fldval++;
	}

	len = strlen( fldval );
	if ( fldnum == sortptr->endfld && sortptr->endchar > 0 &&
		sortptr->endchar < len )
	{
		len = sortptr->endchar;
	}

	if ( strtchar > len )
		return( buf );

	len -= strtchar;
	if ( buf + len > max )
		len = max - buf;
	if ( len <= 0 )
		return( buf );

	strncpy( buf, &fldval[ strtchar ], len );

	return( buf + len );
}

#ifndef MAXSFLDBUF
#define MAXSFLDBUF	1024
#endif

static int
cmp_attr( projptr, tpl1, tpl2, do_subsort )
register struct qprojection *projptr;
struct utuple *tpl1[];
struct utuple *tpl2[];
int do_subsort;		/* do sub-range sorting? */
{
	register char *attr1, *attr2;
	register short cmp;
	char buf1[ MAXSFLDBUF ];	/* for sub-field comparisons */
	char buf2[ MAXSFLDBUF ];

	if ( ( projptr->flags & QP_NEWVALUE ) && ( projptr->attorval ) )
	{
		return( 0 );
	}

	if ( ATTR_SPECIAL( projptr->attr ) )
	{
		register long diff;

		if ( projptr->attr == ATTR_RECNUM )
			diff = tpl1[ projptr->rel->nodenum ]->tuplenum -
				tpl2[ projptr->rel->nodenum ]->tuplenum;
		else
			diff = tpl1[ projptr->rel->nodenum ]->lseek -
				tpl2[ projptr->rel->nodenum ]->lseek;
		if ( diff < 0 )
			cmp = -1;
		else if ( diff > 0 )
			cmp = 1;
		else
			cmp = 0;
		return( ( projptr->flags & QP_DESCENDING ) == 0 ? cmp : -cmp );
	}

	attr1 = tpl1[ projptr->rel->nodenum ]->tplval[ projptr->attr ];
	attr2 = tpl2[ projptr->rel->nodenum ]->tplval[ projptr->attr ];

	if ( projptr->flags & QP_RMBLANKS )
	{
		while( isspace( *attr1 ) )
			attr1++;
		while( isspace( *attr2 ) )
			attr2++;
	}

	if ( projptr->subcnt != 0 && do_subsort )
	{
		char *flds1[MAXATT];
		char *flds2[MAXATT];

		char *ptr1, *ptr2;
		short cnt1, cnt2, fldnum;
		short strtchar;
		register short j;
		struct qsubsort *sortptr;

		cnt1 = splitflds( flds1, attr1, projptr->delim, MAXATT );
		cnt2 = splitflds( flds2, attr2, projptr->delim, MAXATT );

		attr1 = buf1;	/* change values to use in compare */
		attr2 = buf2;

		ptr1 = buf1;
		ptr2 = buf2;
		for ( j = 0, sortptr = projptr->subsort;
			j < projptr->subcnt;
			j++, sortptr++ )
		{
			fldnum = sortptr->strtfld;
			strtchar = sortptr->strtchar;
			while( fldnum <= sortptr->endfld &&
				( fldnum < cnt1 || fldnum < cnt2 ) )
			{
				if ( fldnum < cnt1 )
				{
					ptr1 = cpsubfld( projptr, sortptr,
							ptr1,
							&buf1[MAXSFLDBUF - 1],
							flds1[fldnum],
							fldnum, strtchar );
				}

				if ( fldnum < cnt2 )
				{
					ptr2 = cpsubfld( projptr, sortptr,
							ptr2,
							&buf2[MAXSFLDBUF - 1],
							flds2[fldnum],
							fldnum, strtchar );
				}

				fldnum++;
				strtchar = 0;

			} /* end while there is some field to copy */

		} /* end for each sub range */

		*ptr1 = '\0';
		*ptr2 = '\0';

		restoreflds( flds1, projptr->delim, cnt1 );
		restoreflds( flds2, projptr->delim, cnt2 );

	} /* end if there is sub-range to sort on */

	switch( projptr->flags & QP_SORTMASK ) {
	case QP_NUMERIC:
	{
		double num1, num2;

		num1 = atof( attr1 );
		num2 = atof( attr2 );
		cmp = ( num1 < num2 ? -1 :
			num1 > num2 ? 1 :
			0 );
		break;
	}
	case QP_CASELESS:
		cmp = nocasecmp( attr1, attr2 );
		break;
	case QP_DATE:
		cmp = cmpdate( attr1, attr2 );
		break;
	case QP_BINARY:
	{
		unsigned int num1, num2;
#ifdef	__STDC__
		/*
		 * Use unsigned conversion if available to
		 * avoid problem with maximum unsigned value
		 * since most (all) non-base 10 values are
		 * initially generated from an unsigned int.
		 */
		num1 = (unsigned int) strtoul( attr1, (char **)NULL, 2 );
		num2 = (unsigned int) strtoul( attr2, (char **)NULL, 2 );
#else
		num1 = (unsigned int) strtol( attr1, (char **)NULL, 2 );
		num2 = (unsigned int) strtol( attr2, (char **)NULL, 2 );
#endif
		cmp = ( num1 < num2 ? -1 :
			num1 > num2 ? 1 :
			0 );
		break;
	}
	case QP_OCTAL:
	{
		unsigned int num1, num2;
#ifdef	__STDC__
		/*
		 * Use unsigned conversion if available to
		 * avoid problem with maximum unsigned value
		 * since most (all) non-base 10 values are
		 * initially generated from an unsigned int.
		 */
		num1 = (unsigned int) strtoul( attr1, (char **)NULL, 8 );
		num2 = (unsigned int) strtoul( attr2, (char **)NULL, 8 );
#else
		num1 = (unsigned int) strtol( attr1, (char **)NULL, 8 );
		num2 = (unsigned int) strtol( attr2, (char **)NULL, 8 );
#endif
		cmp = ( num1 < num2 ? -1 :
			num1 > num2 ? 1 :
			0 );
		break;
	}
	case QP_HEXADECIMAL:
	{
		unsigned int num1, num2;
#ifdef	__STDC__
		/*
		 * Use unsigned conversion if available to
		 * avoid problem with maximum unsigned value
		 * since most (all) non-base 10 values are
		 * initially generated from an unsigned int.
		 */
		num1 = (unsigned int) strtoul( attr1, (char **)NULL, 16 );
		num2 = (unsigned int) strtoul( attr2, (char **)NULL, 16 );
#else
		num1 = (unsigned int) strtol( attr1, (char **)NULL, 16 );
		num2 = (unsigned int) strtol( attr2, (char **)NULL, 16 );
#endif
		cmp = ( num1 < num2 ? -1 :
			num1 > num2 ? 1 :
			0 );
		break;
	}
	case QP_DICTIONARY:
		cmp = dictcmp( attr1, attr2 );
		break;
	case QP_PRINTABLE:
		cmp = printcmp( attr1, attr2 );
		break;
	case QP_NOCASEDICT:
		cmp = nocasedictcmp( attr1, attr2 );
		break;
	case QP_NOCASEPRINT:
		cmp = nocaseprintcmp( attr1, attr2 );
		break;
	case QP_STRING:
	default:
		cmp = strcmp( attr1, attr2 );
		break;
	}

	return( ( projptr->flags & QP_DESCENDING ) == 0 ? cmp : -cmp );
}

/*
 * WARNING: The cmp_attr( ) and cmp_attrval( ) functions are VERY similar
 *	    so any changes that are to be made to one probably need to be
 *	    made to both of these functions.  For performance reasons,
 *	    it was decided that that cmp_attr( ) should keep it's own copy
 *	    of the common code instead of calling cmp_attrval( ).
 */

int
cmp_attrval( projptr, attr1, attr2, do_subsort )
register struct qprojection *projptr;
register char *attr1;
register char *attr2;
int do_subsort;		/* do sub-range sorting? */
{
	register short cmp;
	char buf1[ MAXSFLDBUF ];	/* for sub-field comparisons */
	char buf2[ MAXSFLDBUF ];

	if ( ( projptr->flags & QP_NEWVALUE ) && ( projptr->attorval ) )
	{
		return( 0 );
	}

	if ( projptr->flags & QP_RMBLANKS )
	{
		while( isspace( *attr1 ) )
			attr1++;
		while( isspace( *attr2 ) )
			attr2++;
	}

	if ( projptr->subcnt != 0 && do_subsort )
	{
		char *flds1[MAXATT];
		char *flds2[MAXATT];

		char *ptr1, *ptr2;
		short cnt1, cnt2, fldnum;
		short strtchar;
		register short j;
		struct qsubsort *sortptr;

		cnt1 = splitflds( flds1, attr1, projptr->delim, MAXATT );
		cnt2 = splitflds( flds2, attr2, projptr->delim, MAXATT );

		attr1 = buf1;	/* change values to use in compare */
		attr2 = buf2;

		ptr1 = buf1;
		ptr2 = buf2;
		for ( j = 0, sortptr = projptr->subsort;
			j < projptr->subcnt;
			j++, sortptr++ )
		{
			fldnum = sortptr->strtfld;
			strtchar = sortptr->strtchar;
			while( fldnum <= sortptr->endfld &&
				( fldnum < cnt1 || fldnum < cnt2 ) )
			{
				if ( fldnum < cnt1 )
				{
					ptr1 = cpsubfld( projptr, sortptr,
							ptr1,
							&buf1[MAXSFLDBUF - 1],
							flds1[fldnum],
							fldnum, strtchar );
				}

				if ( fldnum < cnt2 )
				{
					ptr2 = cpsubfld( projptr, sortptr,
							ptr2,
							&buf2[MAXSFLDBUF - 1],
							flds2[fldnum],
							fldnum, strtchar );
				}

				fldnum++;
				strtchar = 0;

			} /* end while there is some field to copy */

		} /* end for each sub range */

		*ptr1 = '\0';
		*ptr2 = '\0';

		restoreflds( flds1, projptr->delim, cnt1 );
		restoreflds( flds2, projptr->delim, cnt2 );

	} /* end if there is sub-range to sort on */

	switch( projptr->flags & QP_SORTMASK ) {
	case QP_NUMERIC:
	{
		double num1, num2;

		num1 = atof( attr1 );
		num2 = atof( attr2 );
		cmp = ( num1 < num2 ? -1 :
			num1 > num2 ? 1 :
			0 );
		break;
	}
	case QP_CASELESS:
		cmp = nocasecmp( attr1, attr2 );
		break;
	case QP_DATE:
		cmp = cmpdate( attr1, attr2 );
		break;
	case QP_BINARY:
	{
		unsigned int num1, num2;
#ifdef	__STDC__
		/*
		 * Use unsigned conversion if available to
		 * avoid problem with maximum unsigned value
		 * since most (all) non-base 10 values are
		 * initially generated from an unsigned int.
		 */
		num1 = (unsigned int) strtoul( attr1, (char **)NULL, 2 );
		num2 = (unsigned int) strtoul( attr2, (char **)NULL, 2 );
#else
		num1 = (unsigned int) strtol( attr1, (char **)NULL, 2 );
		num2 = (unsigned int) strtol( attr2, (char **)NULL, 2 );
#endif
		cmp = ( num1 < num2 ? -1 :
			num1 > num2 ? 1 :
			0 );
		break;
	}
	case QP_OCTAL:
	{
		unsigned int num1, num2;
#ifdef	__STDC__
		/*
		 * Use unsigned conversion if available to
		 * avoid problem with maximum unsigned value
		 * since most (all) non-base 10 values are
		 * initially generated from an unsigned int.
		 */
		num1 = (unsigned int) strtoul( attr1, (char **)NULL, 8 );
		num2 = (unsigned int) strtoul( attr2, (char **)NULL, 8 );
#else
		num1 = (unsigned int) strtol( attr1, (char **)NULL, 8 );
		num2 = (unsigned int) strtol( attr2, (char **)NULL, 8 );
#endif
		cmp = ( num1 < num2 ? -1 :
			num1 > num2 ? 1 :
			0 );
		break;
	}
	case QP_HEXADECIMAL:
	{
		unsigned int num1, num2;
#ifdef	__STDC__
		/*
		 * Use unsigned conversion if available to
		 * avoid problem with maximum unsigned value
		 * since most (all) non-base 10 values are
		 * initially generated from an unsigned int.
		 */
		num1 = (unsigned int) strtoul( attr1, (char **)NULL, 16 );
		num2 = (unsigned int) strtoul( attr2, (char **)NULL, 16 );
#else
		num1 = (unsigned int) strtol( attr1, (char **)NULL, 16 );
		num2 = (unsigned int) strtol( attr2, (char **)NULL, 16 );
#endif
		cmp = ( num1 < num2 ? -1 :
			num1 > num2 ? 1 :
			0 );
		break;
	}
	case QP_DICTIONARY:
		cmp = dictcmp( attr1, attr2 );
		break;
	case QP_PRINTABLE:
		cmp = printcmp( attr1, attr2 );
		break;
	case QP_NOCASEDICT:
		cmp = nocasedictcmp( attr1, attr2 );
		break;
	case QP_NOCASEPRINT:
		cmp = nocaseprintcmp( attr1, attr2 );
		break;
	case QP_STRING:
	default:
		cmp = strcmp( attr1, attr2 );
		break;
	}

	return( ( projptr->flags & QP_DESCENDING ) == 0 ? cmp : -cmp );
}

static struct uquery *cmp_query;

static int
cmp_tuple( tpl1, tpl2 )
struct utuple *tpl1[];
struct utuple *tpl2[];
{
	register int i;
	register struct qprojection *projptr;
	register short cmp;

	projptr = cmp_query->attrlist;
	for( i = 0; i < cmp_query->sortcnt; i++, projptr++ ) {
		cmp = cmp_attr( &cmp_query->attrlist[ projptr->sortattr ],
				tpl1, tpl2, TRUE );
		if ( cmp != 0 )
			return( cmp );
	}

	return( 0 );
}

void
sort_tuples( result, blkptr )
struct qresult *result;
struct joinblock *blkptr;
{
	cmp_query = result->query;

	qsort( (char *)blkptr->tuples,
		blkptr->blkcnt / result->query->nodecnt,
		sizeof( struct utuple * ) * result->query->nodecnt,
		cmp_tuple );
}

sort_nexttpl( result, prevblk, blkptr )
struct qresult *result;
register struct joinblock **prevblk;
register struct joinblock *blkptr;
{
	register struct joinblock *nextblk;
	struct utuple **stpl;
	int nodecnt;

	cmp_query = result->query;

	nodecnt = result->query->nodecnt;

	if ( blkptr->curtpl + nodecnt >= blkptr->blkcnt )
	{
		/*
		 * We have an "all-used" block.  Move it to the end of
		 * the list.
		 */
		while( *prevblk != NULL )
			prevblk = &(*prevblk)->next;
	}
	else
	{
		stpl = &blkptr->tuples[ blkptr->curtpl + nodecnt ];

		while( (nextblk = *prevblk) != NULL )
		{
			if ( nextblk->curtpl + nodecnt >= nextblk->blkcnt )
				break;	/* insert before any used blocks */

			if ( cmp_tuple( stpl,
				&nextblk->tuples[ nextblk->curtpl + nodecnt ] ) <= 0 )
			{
				break;	/* found spot to insert */
			}

			prevblk = &nextblk->next;
		}
	}

	blkptr->next = *prevblk;
	*prevblk = blkptr;
}

int
cmp_unique( result, tpl1, tpl2 )
struct qresult *result;
struct utuple *tpl1[];
struct utuple *tpl2[];
{
	register int i;
	register struct qprojection *projptr;

	projptr = result->query->attrlist;
	for( i = 0; i < result->query->attrcnt; i++, projptr++ ) {

		if ( ( projptr->flags & (QP_NODISPLAY|QP_SORTUNIQ) ) == QP_NODISPLAY )
			continue;	/* not used for uniqueness */

		if ( cmp_attr( projptr, tpl1, tpl2, FALSE ) != 0 )
			return( TRUE );
	}

	return( FALSE );
}

/*
 * Information used to check uniqueness of tuples.
 * This is hidden from the outside world since we want
 * to be able to change the implementation.
 */
struct stplhash {
	unsigned long hashval;
	struct utuple **stpl;
	struct stplhash *next;
};

#define HSHVALBLKSIZE	170	/* hash structs per blk, to get 2K blk */

struct hashvalblk {
	struct hashvalblk *next;
	unsigned short blkcnt;
	struct stplhash entries[ HSHVALBLKSIZE ];
};

#define MAXBUCKET	512  /* buckets in hash table, must be power of 2! */
#define BUCKETMASK	(MAXBUCKET - 1)

struct attrhash {
	short nodenum;
	short attrnum;
	unsigned long (*func)();	/* func to do hashing for each attr */
	int modifier;			/* attribute/comparison/hash modifier */
};

struct unique {
	struct hashvalblk *hashlist;	/* hash table entries */
	short attrcnt;			/* # attrs in attrlist */
	struct attrhash attrlist[ MAXATT ];  /* attr numbers for projection */
	struct stplhash *hashtbl[ MAXBUCKET ];  /* hash table for uniqueness */
};

int
init_unique( result )
struct qresult *result;
{
	struct unique *uniqptr;
	struct uquery *query;
	struct attrhash *attrptr;
	struct qprojection *projptr;
	int i;

	uniqptr = (struct unique *)calloc( 1, sizeof( struct unique ) );
	if ( uniqptr == NULL )
	{
		set_uerror( UE_NOMEM );
		return( FALSE );
	}

	query = result->query;

	/* go through and find the attributes of each type of comparison */

	attrptr = uniqptr->attrlist;
	projptr = query->attrlist;
	for( i = 0; i < query->attrcnt; i++, projptr++ )
	{
		if ( ( projptr->flags & (QP_NODISPLAY|QP_SORTUNIQ) ) == QP_NODISPLAY )
			continue;

		attrptr->nodenum = projptr->rel->nodenum;
		attrptr->attrnum = projptr->attr;
		attrptr->modifier = 0;			/* default hash modifier */

		if ( ATTR_SPECIAL( projptr->attr ) )
		{
			/* seek value or record number attributes */
			attrptr->func = hash_num_attr;
		}
		else
		{
			switch( projptr->flags & QP_SORTMASK ) {
			case QP_NUMERIC:
				attrptr->func = hash_num_attr;
				break;
			case QP_CASELESS:
				attrptr->func = hash_nocase_attr;
				break;
			case QP_DATE:
				attrptr->func = hash_date_attr;
				break;
			case QP_BINARY:
				/* hash binary input number */
				attrptr->modifier = 2;
				attrptr->func = hash_num_attr;
				break;
			case QP_OCTAL:
				/* hash octal input number */
				attrptr->modifier = 8;
				attrptr->func = hash_num_attr;
				break;
			case QP_HEXADECIMAL:
				/* hash hexadecimal input number */
				attrptr->modifier = 16;
				attrptr->func = hash_num_attr;
				break;
			case QP_DICTIONARY:
				attrptr->func = hash_dict_attr;
				break;
			case QP_PRINTABLE:
				attrptr->func = hash_print_attr;
				break;
			case QP_NOCASEDICT:
				attrptr->func = hash_ucdict_attr;
				break;
			case QP_NOCASEPRINT:
				attrptr->func = hash_ucprint_attr;
				break;
			case QP_STRING:
			default:
				attrptr->func = hash_str_attr;
				break;
			}
		}

		attrptr++;
	}

	uniqptr->attrcnt = attrptr - uniqptr->attrlist;

	result->unique = (char *)uniqptr;

	return( TRUE );
}

void
end_unique( result )
struct qresult *result;
{
	struct hashvalblk *blkptr, *next;

	if ( result->unique != NULL ) {
		for( blkptr = ((struct unique *)result->unique)->hashlist;
			blkptr != NULL;
			blkptr = next )
		{
			next = blkptr->next;
			free( blkptr );
		}

		free( result->unique );
		result->unique = NULL;
	}
}

int
unique_tuple( result )
struct qresult *result;
{
	register int i;
	struct unique *uniqptr;
	unsigned long hashval;
	struct utuple **curtpl;
	struct stplhash *hashptr;
	struct hashvalblk *blkptr;
	register struct attrhash *attrptr;

	if ( result->unique == NULL )
		return( TRUE );

	uniqptr = (struct unique *)result->unique;

	curtpl = &result->curblk->tuples[ result->curblk->curtpl ];

	hashval = INIT_HASH();
	for( i = 0, attrptr = uniqptr->attrlist; i < uniqptr->attrcnt;
		i++, attrptr++ )
	{
		hashval = (attrptr->func)( hashval, curtpl[ attrptr->nodenum ],
				attrptr->attrnum, attrptr->modifier );
	}
	END_HASH( hashval );

	for( hashptr = uniqptr->hashtbl[ hashval & BUCKETMASK ];
		hashptr != NULL;
		hashptr = hashptr->next )
	{
		if ( hashptr->hashval == hashval &&
			! cmp_unique( result, curtpl, hashptr->stpl ) )
		{
			return( FALSE );	/* tuple not unique */
		}
	}

	/*
	 * The tuple is unique, so save it in the hash table.
	 */
	blkptr = uniqptr->hashlist;
	if ( blkptr == NULL || blkptr->blkcnt >= HSHVALBLKSIZE )
	{
		/* allocate a new block */
		blkptr = (struct hashvalblk *)calloc( 1, sizeof( struct hashvalblk ) );
		if ( blkptr == NULL )
		{
			set_uerror( UE_NOMEM );
			return( TRUE );		/* tuple is still unique */
		}
		blkptr->next = uniqptr->hashlist;
		uniqptr->hashlist = blkptr;
	}

	hashptr = &blkptr->entries[ blkptr->blkcnt++ ];
	hashptr->hashval = hashval;
	hashptr->stpl = curtpl;

	hashptr->next = uniqptr->hashtbl[ hashval & BUCKETMASK ];
	uniqptr->hashtbl[ hashval & BUCKETMASK ] = hashptr;

	return( TRUE );
}
