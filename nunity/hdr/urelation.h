/*
 * Copyright (C) 2002 by Lucent Technologies
 *
 */

/*
 * This file defines the basic structures for UNITY relations.
 */
#ifndef _URELATION
#define _URELATION	1

#include <sys/param.h>

/*
 * Attribute structure definitions
 */
typedef enum {
	UAT_TERMCHAR = 0,		/* attribute ends at terminating char. */
	UAT_FIXEDWIDTH = 1		/* attribute is fixed char. width */
} UATTRTYPE;

#define ANAMELEN	31	/* maximum attribute name size */

struct uattribute {
	char aname[ANAMELEN + 1];	/* attribute name in descriptor file */
	UATTRTYPE attrtype;		/* term. ch. or fixed width */
	unsigned char flags;	/* NOT FOR UNITY USE-usable by application */
	unsigned char justify;		/* 'l'eft, 'r'ight, or 'c'enter */
	unsigned short terminate;	/* term. char. or width */
	unsigned short prwidth;		/* print width for attribute */
	char *friendly;			/* user friendly name */
};

/*
 * Relation structure definitions
 */
struct urelation {
	char *path;		/* file name of database */
	char *dpath;		/* descriptor file path -- normally not set */
	unsigned char flags;	/* NOT FOR UNITY USE-usable by application */
	unsigned char relflgs;	/* status flags for relation */
	unsigned short attrcnt;	/* number of attributes per tuple */
	struct uattribute *attrs;	/* attribute descriptions */
};

/*
 * Flags for the relation (relflags)
 */
#define UR_DESCRWTBL	0x01	/* description stored with table itself */
#define UR_RELALLOC	0x02	/* relation struct allocated w/malloc() */
#define UR_PATHALLOC	0x04	/* relation path allocated w/malloc() */
#define UR_DPATHALLOC	0x08	/* descriptor path allocated w/malloc() */
#define UR_PACKEDTBL	0x10	/* packed table opened to read description */
#define UR_NODISPLAY	0x20	/* all attrs have nodisplay print width/justification */

/*
 * Below are structures for storing tuple information for a relation
 */
struct utuple {
	char **tplval;		/* tuple value itself */
	unsigned char refcnt;	/* cnt of places each tuple matches query */
	unsigned char badattr;	/* when error occurs, first attr that is bad */
	unsigned short flags;	/* flags for marking tuples, etc. */
	unsigned short tplsize;	/* size of the tuple in chars */
	unsigned long tuplenum;	/* number of tuple in relation */
	unsigned long lseek;	/* seek location of tuple in relation */
};

#define TPL_NOBADATTR	0xff	/* tuple.badattr is not legit */

/*
 * Tuple flags.  This first set of flags is only used in queries, but
 * is included here just to keep things localized.
 */
#define TPL_JOINED	0x0001	/* tuple was joined with other tuples */
#define TPL_DELETE	0x0002	/* delete tuple even though no match */
#define TPL_ACCESSED	0x0008	/* tuple referenced during join */
#define TPL_INDIRJOIN	0x0010	/* tuple joined indirectly, don't delete */
#define TPL_INVISIBLE	0x0020	/* tuple is deleted, but still there */
				/* used with N_APPTUPLES flag of qnodes */
#define TPL_IGNORE	0x0040	/* tuple is deleted, but still there */
#define TPL_FREED	0x0080	/* tuple already freed by freetuple() */
#define TPL_NULLTPL	0x0100	/* this is the null tuple */
/*
 * The flags below are set when there are syntax errors while reading
 * a tuple.  Rather than ignoring the tuple or aborting, the code
 * applies some heuristics to recover gracefully.
 *
 * In the case of an embedded new-line, all attribute values up to the
 * next newline terminated attribute (usually the last one) are set to NULL.
 * All fixed width attributes are padded as needed.
 *
 * In the case of a premature EOF or out of memory errors, all remaining
 * attribute values are set to NULL.  All fixed width attributes are
 * padded as needed.
 *
 * In the case of a missing new-line when the last attribute is a fixed
 * width attribute, the next tuple will start at that point.
 */
#define TPL_SYNEMBEDNL	0x0200	/* syntax err - embedded \n in tuple */
#define TPL_SYNFWNONL	0x0400	/* syn. err, no \n on last fixed width attr */
#define TPL_SYNPREMEOF	0x0800	/* syn. err, premature EOF */
#define TPL_ERRNOMEM	0x1000	/* no more memory for tuple */
#define TPL_ERRREAD	0x2000	/* read error occured */
#define TPL_ERRUNKNOWN	0x4000	/* some unknown error - should never happen */
#define TPL_ERRBIGATT	0x8000	/* attribute is bigger than I/O block */

#define TPL_ERRORMSK	0xfe00	/* error mask for tuple */

/*
 * Values for uchecktuple indicating how tuple errors are to be treated
 */
#define	UCHECKIGNORE	0x00	/* ignore tuple errors			*/
#define	UCHECKCOUNT	0x01	/* count tuple errors (utplerrors)	*/
#define	UCHECKPRINT	0x02	/* print tuple errors via prtplerror()	*/
#define	UCHECKFATAL	0x03	/* treat tuple error as a fatal error	*/

/*
 * Miscelanewous definitions
 */

/*
 * MAXPATH is for buffers that hold a path to a file
 * MAXPATHLEN is the UNIX #define that says how long
 * a path system calls can cope with.
 */
#ifndef MAXPATH
#ifdef MAXPATHLEN
#define MAXPATH MAXPATHLEN
#else /* MAXPATHLEN */
#define MAXPATH		128	/* max path length for file */
#endif
#endif /* MAXPATHLEN */

/*
 * MAXATT needs to be in sync with the same #define in
 * cmds/format.h
 */
#ifndef MAXATT
#define MAXATT		500	/* max attrs in relation or projection */
#endif

/*
 * Maximum relations we can handle in any query.  This definition is
 * mainly used for data structure allocation.
 */
#ifndef MAXRELATION
#define MAXRELATION	8	/* maximum relations we can handle */
#endif

typedef char *TUPLE;

#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

/*
 * Definition of what a signal function returns.  This definition
 * helps make the code cleaner portability-wise.  The most common
 * definition is that signal functions return an int.  Some
 * implementations say they are void, however.  This definition
 * can be changed depending on the circumstances.
 */
#ifndef RETSIGTYPE
#define RETSIGTYPE	int
#endif

#endif /*_URELATION*/
