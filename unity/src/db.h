/*
 * Copyright (C) 2002 by Lucent Technologies
 *
 */

/* @(#)db.h	1.4 */
#include <stdio.h>
#include <sys/param.h>
#define MAXPTR 8192		/* maximum number of pointers in index */
#define NDSZ 284		/* node size in index */
#ifndef	MAXREC
#define MAXREC 16384		/* maximum record length */
#endif
#define MAXCHAR 0177		/* maximum ascii character */
#ifndef MAXATT
#define MAXATT	500		/* maximum number of attributes */
#endif
#define ANAMELEN 31		/* attribute name length */
#ifdef MAXPATHLEN
#define MAXPATH MAXPATHLEN	/* maximum path length for file */
#else
#define MAXPATH 128
#endif

#ifdef MAXNAMELEN
#define MAXFILE MAXNAMELEN	/* maximum simple file name length */
#else
#define MAXFILE 20		/* maximum simple file name length */
#endif

#ifdef pdp11
#define MAXUNAME 40		/* maximum user-friendly name */
#else
#define MAXUNAME 80		/* maximum user-friendly name */
#endif
#define MAXCMD 512		/* maximum buffer length for commands */
#define KEYLEN 128		/* maximum key value length in index */
#define ERR -1			/* error return code */
#define NLCHAR '~'		/* default new line character */

#define MAXTBL 20		/* maximum tables handled by umenu */
#define DBFILES "dbfiles"	/* name of file with list of tables for umenu */
#define OVERVIEW "overview"	/* name of overview file for umenu */

#ifndef	FALSE
#define	FALSE	0
#endif
#ifndef	TRUE
#define	TRUE	1
#endif

/* Maximum attribute value length must be less than DBBLKSIZE */
#ifndef DBBLKSIZE
#if MACHINE == ibm
#define DBBLKSIZE	10240	/* IBM/Amdahl mainframes with lots of memory */
#else	/* MACHINE != ibm */
#if MACHINE == vax
#define DBBLKSIZE	6144	/* Vaxes and other medium size machines */
#else	/* MACHINE != vax */
#define DBBLKSIZE	4096	/* default I/O block size */
#endif	/* MACHINE == vax */
#endif	/* MACHINE == ibm */
#endif	/* #ifndef DBBLKSIZE */

/* the following structures are used in the index handling programs */
struct hdr
{	int level;
	long nodeloc;
	int chksum;
};
#define HDR sizeof(struct hdr)
struct index
{	int fd;
	char *rdptr;
	char *ordptr;
	struct hdr *root;
	struct hdr *leaf;
};
#define INDEX sizeof(struct index)

/* the following defines are used in scanning the indexes */
#define END 0
#define FOUND 1
#define MISSED 2

/* the following defines are used for operator handling */
#define LLT 1
#define LLE 2
#define LEQ 3
#define LNE 4
#define LGE 5
#define LGT 6
#define REQ 7
#define RNE 8
#define LT 9
#define LE 10
#define EQ 11
#define NE 12
#define GE 13
#define GT 14

/* the following are for date (and time) processing */
#define DLT 15
#define DLE 16
#define DEQ 17
#define DNE 18
#define DGE 19
#define DGT 20

/* the following are for date only processing */
#define DOLT 21
#define DOLE 22
#define DOEQ 23
#define DONE 24
#define DOGE 25
#define DOGT 26

/* the following defines are the caseless operators */
#define	CLT 33				/* LLT + 32 */
#define	CLE 34				/* LLE + 32 */
#define	CEQ 35				/* LEQ + 32 */
#define	CNE 36				/* LNE + 32 */
#define	CGE 37				/* LGE + 32 */
#define	CGT 38				/* LGT + 32 */

struct fmt
{	int flag;
	int flen;
	char aname[ANAMELEN+1];
	char inf[4];
	int prnt;
	char justify;	/* l for left justify, the default
			   r for right
			   c for center
			   L for left but do not print by default
			   R for right
			   C for center
			*/
	char *val;
};

#define WN 1	/* fixed length field */
#define T 2	/* variable length terminated field */

/* error message numbers used by error() */

#define E_DESFOPEN 0
#define E_DATFOPEN 1
#define E_DATFWRITE 2
#define E_TEMFOPEN 3
#define E_DIRWRITE 4
#define E_INDFOPEN 5
#define E_BADLINK 6
#define E_BADUNLINK 7
#define E_EXISTS 8
#define E_BADFLAG 9
#define E_ILLATTR 10
#define E_ERR 11
#define E_SPACE 12
#define E_NONNUMERIC 13
#define	E_LOCKSTMP 14
#define	E_PACKREAD 15
#define E_GENERAL 50

/*
 * Definition of what a signal function returns.  This definition
 * helps make the code cleaner portability-wise.  The most common
 * definition is that signal functions return an int.  Some
 * implementations say they are void, however.  This definition
 * can be changed depending on the circumstances.
 *
 */
#ifndef RETSIGTYPE
#define RETSIGTYPE	int
#endif
