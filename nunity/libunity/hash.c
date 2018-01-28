/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "uquery.h"
#include "uerror.h"
#include "hash.h"

#ifdef	__STDC__
#include <stdlib.h>
#else
extern	long strtol();
#endif

extern double atof();

/*
 * xor31[] contains a long value for each possible value of a byte.
 * It is used to evaluate the result of a CRC polynomial hashing algorithm
 * a byte at a time instead of a bit at a time. If the array value found by
 * indexing with the high order byte of the shift register is xor'd with
 * the register shifted left by 8, a result is obtained equivalent to 8
 * iterations of multiplication by x modulo the polynomial 0xabcdef69.
 */
unsigned long _xor31[] = {
	0x00000000, 0x2bcdef69, 0x579bded2, 0x7c5631bb, 
	0x04fa52cd, 0x2f37bda4, 0x53618c1f, 0x78ac6376, 
	0x09f4a59a, 0x22394af3, 0x5e6f7b48, 0x75a29421, 
	0x0d0ef757, 0x26c3183e, 0x5a952985, 0x7158c6ec, 
	0x13e94b34, 0x3824a45d, 0x447295e6, 0x6fbf7a8f, 
	0x171319f9, 0x3cdef690, 0x4088c72b, 0x6b452842, 
	0x1a1deeae, 0x31d001c7, 0x4d86307c, 0x664bdf15, 
	0x1ee7bc63, 0x352a530a, 0x497c62b1, 0x62b18dd8, 
	0x27d29668, 0x0c1f7901, 0x704948ba, 0x5b84a7d3, 
	0x2328c4a5, 0x08e52bcc, 0x74b31a77, 0x5f7ef51e, 
	0x2e2633f2, 0x05ebdc9b, 0x79bded20, 0x52700249, 
	0x2adc613f, 0x01118e56, 0x7d47bfed, 0x568a5084, 
	0x343bdd5c, 0x1ff63235, 0x63a0038e, 0x486dece7, 
	0x30c18f91, 0x1b0c60f8, 0x675a5143, 0x4c97be2a, 
	0x3dcf78c6, 0x160297af, 0x6a54a614, 0x4199497d, 
	0x39352a0b, 0x12f8c562, 0x6eaef4d9, 0x45631bb0, 
	0x4fa52cd0, 0x6468c3b9, 0x183ef202, 0x33f31d6b, 
	0x4b5f7e1d, 0x60929174, 0x1cc4a0cf, 0x37094fa6, 
	0x4651894a, 0x6d9c6623, 0x11ca5798, 0x3a07b8f1, 
	0x42abdb87, 0x696634ee, 0x15300555, 0x3efdea3c, 
	0x5c4c67e4, 0x7781888d, 0x0bd7b936, 0x201a565f, 
	0x58b63529, 0x737bda40, 0x0f2debfb, 0x24e00492, 
	0x55b8c27e, 0x7e752d17, 0x02231cac, 0x29eef3c5, 
	0x514290b3, 0x7a8f7fda, 0x06d94e61, 0x2d14a108, 
	0x6877bab8, 0x43ba55d1, 0x3fec646a, 0x14218b03, 
	0x6c8de875, 0x4740071c, 0x3b1636a7, 0x10dbd9ce, 
	0x61831f22, 0x4a4ef04b, 0x3618c1f0, 0x1dd52e99, 
	0x65794def, 0x4eb4a286, 0x32e2933d, 0x192f7c54, 
	0x7b9ef18c, 0x50531ee5, 0x2c052f5e, 0x07c8c037, 
	0x7f64a341, 0x54a94c28, 0x28ff7d93, 0x033292fa, 
	0x726a5416, 0x59a7bb7f, 0x25f18ac4, 0x0e3c65ad, 
	0x769006db, 0x5d5de9b2, 0x210bd809, 0x0ac63760, 
	0x3487b6c9, 0x1f4a59a0, 0x631c681b, 0x48d18772, 
	0x307de404, 0x1bb00b6d, 0x67e63ad6, 0x4c2bd5bf, 
	0x3d731353, 0x16befc3a, 0x6ae8cd81, 0x412522e8, 
	0x3989419e, 0x1244aef7, 0x6e129f4c, 0x45df7025, 
	0x276efdfd, 0x0ca31294, 0x70f5232f, 0x5b38cc46, 
	0x2394af30, 0x08594059, 0x740f71e2, 0x5fc29e8b, 
	0x2e9a5867, 0x0557b70e, 0x790186b5, 0x52cc69dc, 
	0x2a600aaa, 0x01ade5c3, 0x7dfbd478, 0x56363b11, 
	0x135520a1, 0x3898cfc8, 0x44cefe73, 0x6f03111a, 
	0x17af726c, 0x3c629d05, 0x4034acbe, 0x6bf943d7, 
	0x1aa1853b, 0x316c6a52, 0x4d3a5be9, 0x66f7b480, 
	0x1e5bd7f6, 0x3596389f, 0x49c00924, 0x620de64d, 
	0x00bc6b95, 0x2b7184fc, 0x5727b547, 0x7cea5a2e, 
	0x04463958, 0x2f8bd631, 0x53dde78a, 0x781008e3, 
	0x0948ce0f, 0x22852166, 0x5ed310dd, 0x751effb4, 
	0x0db29cc2, 0x267f73ab, 0x5a294210, 0x71e4ad79, 
	0x7b229a19, 0x50ef7570, 0x2cb944cb, 0x0774aba2, 
	0x7fd8c8d4, 0x541527bd, 0x28431606, 0x038ef96f, 
	0x72d63f83, 0x591bd0ea, 0x254de151, 0x0e800e38, 
	0x762c6d4e, 0x5de18227, 0x21b7b39c, 0x0a7a5cf5, 
	0x68cbd12d, 0x43063e44, 0x3f500fff, 0x149de096, 
	0x6c3183e0, 0x47fc6c89, 0x3baa5d32, 0x1067b25b, 
	0x613f74b7, 0x4af29bde, 0x36a4aa65, 0x1d69450c, 
	0x65c5267a, 0x4e08c913, 0x325ef8a8, 0x199317c1, 
	0x5cf00c71, 0x773de318, 0x0b6bd2a3, 0x20a63dca, 
	0x580a5ebc, 0x73c7b1d5, 0x0f91806e, 0x245c6f07, 
	0x5504a9eb, 0x7ec94682, 0x029f7739, 0x29529850, 
	0x51fefb26, 0x7a33144f, 0x066525f4, 0x2da8ca9d, 
	0x4f194745, 0x64d4a82c, 0x18829997, 0x334f76fe, 
	0x4be31588, 0x602efae1, 0x1c78cb5a, 0x37b52433, 
	0x46ede2df, 0x6d200db6, 0x11763c0d, 0x3abbd364, 
	0x4217b012, 0x69da5f7b, 0x158c6ec0, 0x3e4181a9, 
};


unsigned long
hash_num_attr( hashval, tplptr, attrnum, modifier )
register unsigned long hashval;
struct utuple *tplptr;
short attrnum;
int modifier;
{
	double dbl;

	switch( attrnum ) {
	case ATTR_RECNUM:
		dbl = tplptr->tuplenum;
		break;
	case ATTR_SEEK:
		dbl = tplptr->lseek;
		break;
	default:	/* normal attribute */
		if ( modifier <= 0 || modifier > 36 ) {
			dbl = atof( tplptr->tplval[ attrnum ] );
		} else if ( modifier == 1 ) {
			dbl = strlen( tplptr->tplval[ attrnum ] );
		} else {
#ifdef	__STDC__
			/*
			 * Use unsigned conversion if available to
			 * avoid problem with maximum unsigned value
			 * since most (all) non-base 10 values are
			 * initially generated from an unsigned int.
			 */
			dbl = (int) strtoul( tplptr->tplval[ attrnum ], (char **)NULL, modifier );
#else
			dbl = (int) strtol( tplptr->tplval[ attrnum ], (char **)NULL, modifier );
#endif
		}
		break;
	}

	NUM_HASH( hashval, dbl );

	return( hashval );
}

/*ARGSUSED*/
unsigned long
hash_nocase_attr( hashval, tplptr, attrnum, modifier )
register unsigned long hashval;
struct utuple *tplptr;
short attrnum;
int modifier;	/* NOT USED */
{
	NOCASE_HASH( hashval, tplptr->tplval[ attrnum ] );

	return( hashval );
}

/*ARGSUSED*/
unsigned long
hash_date_attr( hashval, tplptr, attrnum, modifier )
register unsigned long hashval;
struct utuple *tplptr;
short attrnum;
int modifier;	/* NOT USED */
{
	DATE_HASH( hashval, tplptr->tplval[ attrnum ] );

	return( hashval );
}

/*ARGSUSED*/
unsigned long
hash_dateonly_attr( hashval, tplptr, attrnum, modifier )
register unsigned long hashval;
struct utuple *tplptr;
short attrnum;
int modifier;	/* NOT USED */
{
	DATE_ONLY_HASH( hashval, tplptr->tplval[ attrnum ] );

	return( hashval );
}

/*ARGSUSED*/
unsigned long
hash_dict_attr( hashval, tplptr, attrnum, modifier )
register unsigned long hashval;
struct utuple *tplptr;
short attrnum;
int modifier;	/* NOT USED */
{
	DICT_HASH( hashval, tplptr->tplval[ attrnum ] );

	return( hashval );
}

/*ARGSUSED*/
unsigned long
hash_print_attr( hashval, tplptr, attrnum, modifier )
register unsigned long hashval;
struct utuple *tplptr;
short attrnum;
int modifier;	/* NOT USED */
{
	PRINT_HASH( hashval, tplptr->tplval[ attrnum ] );

	return( hashval );
}

/*ARGSUSED*/
unsigned long
hash_str_attr( hashval, tplptr, attrnum, modifier )
register unsigned long hashval;
struct utuple *tplptr;
short attrnum;
int modifier;	/* NOT USED */
{
	STR_HASH( hashval, tplptr->tplval[ attrnum ] );

	return( hashval );
}

/*ARGSUSED*/
unsigned long
hash_ucdict_attr( hashval, tplptr, attrnum, modifier )
register unsigned long hashval;
struct utuple *tplptr;
short attrnum;
int modifier;	/* NOT USED */
{
	UCDICT_HASH( hashval, tplptr->tplval[ attrnum ] );

	return( hashval );
}

/*ARGSUSED*/
unsigned long
hash_ucprint_attr( hashval, tplptr, attrnum, modifier )
register unsigned long hashval;
struct utuple *tplptr;
short attrnum;
int modifier;	/* NOT USED */
{
	UCPRINT_HASH( hashval, tplptr->tplval[ attrnum ] );

	return( hashval );
}
