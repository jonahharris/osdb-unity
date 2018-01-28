/*
 * Copyright (C) 2002 by Lucent Technologies
 *
 */

/*
 * This file contains definitions for I/O to/from a UNITY
 * database relation.
 */
#ifndef _UDBIO
#define _UDBIO	1

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

struct urelblock {
	unsigned short attrcnt;		/* number of attr. vals used in blk */
	long seekval;			/* seekval of start of block */
	char *end;			/* last byte used within block */
	char *curloc;			/* current byte location in block */
	struct urelblock *next;		/* next block from relation */
	char data[DBBLKSIZE];		/* actual data from file */
};

/*
 * Input read structure for a relation.
 *
 * Note that the "savebuf" contains split attribute values, and any
 * fixed width attribute values as well (so they can be null terminated).
 */
struct urelio {
	unsigned short flags;		/* flags about I/O */
	short fd;			/* file descriptor for relation */
	unsigned long blkcnt;		/* how many blocks read from rel. */
	unsigned long tuplecnt;		/* how many tuples read from rel. */
	struct urelblock *readbuf;	/* read buffers for relation */
	struct urelblock *curblk;	/* curent block location in readbuf */
	struct urelblock *savebuf;	/* attr value buffers for relation */
};

/*
 * I/O flags
 */
#define UIO_EOF		0x0001		/* EOF encountered on I/O */
#define UIO_ERROR	0x0002		/* error encountered on I/O */
#define UIO_STDIN	0x0004		/* file comes from stdin */
#define UIO_NORMAL	0x0008		/* normal file */
#define UIO_PACKED	0x0010		/* packed file */

#define UIO_FILEMASK	0x001c		/* mask for getting file type above */

extern struct urelblock *newrelblk( );

#endif /*_UDBIO*/
