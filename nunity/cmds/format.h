/*
 * Copyright (C) 2002 by Lucent Technologies
 *
 */

/*
 * Size definitions
 */
#ifndef MAXATT
#define MAXATT		500
#endif

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif

#define FLDDELIM	':'	/* default field delimiter */

/*
 * I/O read structure for formatting input.  This is used to
 * speed up the reads.
 *
 * Note that the "savebuf" attribute values that cross block boundaries.
 */
struct formatio {
	unsigned short flags;		/* flags about I/O */
	short fd;			/* file descriptor for I/O */
	struct fioblock *readbuf;	/* read buffer */
	struct fioblock *curblk;	/* curent block location in readbuf */
	struct fioblock *savebuf;	/* split value buffer */
};

/*
 * I/O flags
 */
#define FIO_EOF		0x0001		/* EOF encountered on I/O */
#define FIO_ERROR	0x0002		/* error encountered on I/O */
#define FIO_READ	0x0004		/* read failed */
#define FIO_NOMEM	0x0008		/* memory allocation failed */
#define FIO_TOOBIG	0x0010		/* record is too big */
#define FIO_NOINIT	0x0020		/* I/O has not been initialized */
#define FIO_INITDONE	0x1000		/* I/O initialization done */

/*
 * I/O Block definition
 */
#define FIOBLKSIZE	4096

struct fioblock {
	short fldcnt;			/* number of fld. vals used in block */
	char *end;			/* last byte used within block */
	char *curloc;			/* current byte location in block */
	struct fioblock *next;		/* next block from input */
	char data[FIOBLKSIZE];		/* actual data from file */
};
