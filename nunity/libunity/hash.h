/*
 * Copyright (C) 2002 by Lucent Technologies
 *
 */

#include <stdio.h>
#include <ctype.h>

extern unsigned long hash_str_attr( );
extern unsigned long hash_dict_attr( );
extern unsigned long hash_nocase_attr( );
extern unsigned long hash_print_attr( );
extern unsigned long hash_ucdict_attr( );
extern unsigned long hash_ucprint_attr( );
extern unsigned long hash_date_attr( );
extern unsigned long hash_dateonly_attr( );
extern unsigned long hash_num_attr( );

/*
 * xor31[] contains a long value for each possible value of a byte.
 * It is used to evaluate the result of a CRC polynomial hashing algorithm
 * a byte at a time instead of a bit at a time. If the array value found by
 * indexing with the high order byte of the shift register is xor'd with
 * the register shifted left by 8, a result is obtained equivalent to 8
 * iterations of multiplication by x modulo the polynomial 0xabcdef69.
 */
extern unsigned long _xor31[];

/*
 * Unity Comparison tables for used for caseless, dictionary, and printable
 * types of sorting along with the flags that are used to indicate if the
 * table has been initialized or not.
 */
extern char uc_upper[256];	/* characters mapped to upper case */
extern char uc_nondict[256];	/* dictionary sort to ignore character if set */
extern char uc_nonprint[256];	/* printable sort to ignore character if set */

extern int init_uc_upper;	/* set to one (1) when uc_upper[] has been initialized */
extern int init_uc_nondict;	/* set to one (1) when uc_nondict[] has been initialized */
extern int init_uc_nonprint;	/* set to one (1) when uc_nonprint[] has been initialized */

extern void init_upper_tbl( );		/* initializes uc_upper[] */
extern void init_nondict_tbl( );	/* initializes uc_nondict[] */
extern void init_nonprint_tbl( );	/* initializes uc_nonprint[] */

/* hashing function */
#define HASH( val, data ) \
	(val) = _xor31[ (val) >> 24] ^ ((((val) & 0x7fffff) << 8) | (data))


#define INIT_HASH()	0x31b2c3d4

#define END_HASH( hashval )					\
{	/* spin through the hashing a few more times */		\
	HASH( hashval, 0 );					\
	HASH( hashval, 0 );					\
	HASH( hashval, 0 );					\
	HASH( hashval, 0 );					\
}

#define STR_HASH( hashval, str )				\
{								\
	register char *_ptr;					\
								\
	_ptr = (str);						\
								\
	while( *_ptr ) {					\
		HASH( (hashval), *_ptr );			\
		_ptr++;						\
	}							\
}

#define NOCASE( ch )	(isupper( (ch) ) ? _tolower( (ch) ) : (ch))

#define NOCASE_HASH( hashval, str )				\
{								\
	register char *_ptr;					\
								\
	if ( ! init_uc_upper ) {				\
		init_upper_tbl( );				\
	}							\
								\
	_ptr = (str);						\
								\
	while( *_ptr ) {					\
		HASH((hashval),uc_upper[(unsigned char)*_ptr]);	\
		_ptr++;						\
	}							\
}

#define DICT_HASH( hashval, str )				\
{								\
	register char *_ptr;					\
								\
	if ( ! init_uc_nondict ) {				\
		init_nondict_tbl( );				\
	}							\
								\
	_ptr = (str);						\
								\
	while( *_ptr ) {					\
		/* Ignore if non-dictionary character */	\
		if( uc_nondict[ (unsigned char) *_ptr ] ) {	\
			_ptr++;					\
			continue;				\
		}						\
		HASH( (hashval), *_ptr );			\
		_ptr++;						\
	}							\
}

#define PRINT_HASH( hashval, str )				\
{								\
	register char *_ptr;					\
								\
	if ( ! init_uc_nonprint ) {				\
		init_nonprint_tbl( );				\
	}							\
								\
	_ptr = (str);						\
								\
	while( *_ptr ) {					\
		/* Ignore if non-printable character */		\
		if( uc_nonprint[ (unsigned char) *_ptr ] ) {	\
			_ptr++;					\
			continue;				\
		}						\
		HASH( (hashval), *_ptr );			\
		_ptr++;						\
	}							\
}

#define UCDICT_HASH( hashval, str )				\
{								\
	register char *_ptr;					\
								\
	if ( ! init_uc_upper ) {				\
		init_upper_tbl( );				\
	}							\
	if ( ! init_uc_nondict ) {				\
		init_nondict_tbl( );				\
	}							\
								\
	_ptr = (str);						\
								\
	while( *_ptr ) {					\
		/* Ignore if non-dictionary character */	\
		if( uc_nondict[ (unsigned char) *_ptr ] ) {	\
			_ptr++;					\
			continue;				\
		}						\
		HASH((hashval),uc_upper[(unsigned char)*_ptr]);	\
		_ptr++;						\
	}							\
}

#define UCPRINT_HASH( hashval, str )				\
{								\
	register char *_ptr;					\
								\
	if ( ! init_uc_upper ) {				\
		init_upper_tbl( );				\
	}							\
	if ( ! init_uc_nonprint ) {				\
		init_nonprint_tbl( );				\
	}							\
								\
	_ptr = (str);						\
								\
	while( *_ptr ) {					\
		/* Ignore if non-printable character */		\
		if( uc_nonprint[ (unsigned char) *_ptr ] ) {	\
			_ptr++;					\
			continue;				\
		}						\
		HASH((hashval),uc_upper[(unsigned char)*_ptr]);	\
		_ptr++;						\
	}							\
}

#define NUM_HASH( hashval, dblnum )				\
{								\
	union {							\
		double dbl;					\
		char ch[ sizeof( double ) ];			\
	} u;							\
								\
	u.dbl = dblnum;						\
								\
	/* ASSUMPTION: sizeof( double ) == 8 */			\
	HASH( (hashval), u.ch[ 0 ] );				\
	HASH( (hashval), u.ch[ 1 ] );				\
	HASH( (hashval), u.ch[ 2 ] );				\
	HASH( (hashval), u.ch[ 3 ] );				\
	HASH( (hashval), u.ch[ 4 ] );				\
	HASH( (hashval), u.ch[ 5 ] );				\
	HASH( (hashval), u.ch[ 6 ] );				\
	HASH( (hashval), u.ch[ 7 ] );				\
}

extern double cnvtdate();

#define DATE_HASH( hashval, str )				\
{								\
	union {							\
		double dbl;					\
		char ch[ sizeof( double ) ];			\
	} u;							\
								\
	u.dbl = cnvtdate( (str), TRUE );			\
								\
	/* ASSUMPTION: sizeof( double ) == 8 */			\
	HASH( (hashval), u.ch[ 0 ] );				\
	HASH( (hashval), u.ch[ 1 ] );				\
	HASH( (hashval), u.ch[ 2 ] );				\
	HASH( (hashval), u.ch[ 3 ] );				\
	HASH( (hashval), u.ch[ 4 ] );				\
	HASH( (hashval), u.ch[ 5 ] );				\
	HASH( (hashval), u.ch[ 6 ] );				\
	HASH( (hashval), u.ch[ 7 ] );				\
}

#define DATE_ONLY_HASH( hashval, str )				\
{								\
	union {							\
		double dbl;					\
		char ch[ sizeof( double ) ];			\
	} u;							\
								\
	u.dbl = cnvtdate( (str), FALSE );			\
								\
	/* ASSUMPTION: sizeof( double ) == 8 */			\
	HASH( (hashval), u.ch[ 0 ] );				\
	HASH( (hashval), u.ch[ 1 ] );				\
	HASH( (hashval), u.ch[ 2 ] );				\
	HASH( (hashval), u.ch[ 3 ] );				\
	HASH( (hashval), u.ch[ 4 ] );				\
	HASH( (hashval), u.ch[ 5 ] );				\
	HASH( (hashval), u.ch[ 6 ] );				\
	HASH( (hashval), u.ch[ 7 ] );				\
}
