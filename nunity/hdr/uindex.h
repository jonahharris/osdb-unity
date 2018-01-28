/*
 * Copyright (C) 2002 by Lucent Technologies
 *
 */

/*
 * Below are the structures used for indexes.  It is copied straight
 * from UNITY's "db.h" file.
 */
#ifndef _UINDEX
#define _UINDEX	1

#define UI_MAXKEYLEN	128	/* maximum key length in index */
#define UI_MAXCHAR	0177	/* maximum ascii character */

/*
 * Size of the key space in each node of the B-file.
 * This size accounts for enough room for two (or more)
 * null-terminated, maximum length keys and their seek
 * values; and a null-terminated UI_MAXCHAR string and
 * its seek value.
 */
#define UI_KEYSIZE					\
	(2 * ( UI_MAXKEYLEN + 1 + sizeof( long ) ) +	\
	2 + sizeof( long ))

/*
 * Structures used for indexes
 */
/*
 * Header information for each node.
 */
struct unodehdr {
	int level;		/* level of this node */
	long nodeloc;		/* node location in B file */
	int chksum;		/* check sum for node */
};

/*
 * Structure of each node in the B-file.
 */
struct uidxnode {
	struct unodehdr hdr;	/* node header */
	char keys[ UI_KEYSIZE ]; /* space for keys and seek values */
};

/*
 * Size of a node in the B-file.
 */
#define UI_NODESIZE (sizeof( struct uidxnode ))

/*
 * Information about an index.
 */
struct uindex {
	int fd;			/* file descriptor for B file */
	char *rdptr;
	char *ordptr;
	struct unodehdr *root;	/* root node - always in memory */
	struct unodehdr *leaf;	/* current leaf node */
};

/* the following defines are used in scanning the indexes */
#define END	0
#define FOUND	1
#define MISSED	2

#endif /* _UINDEX */
