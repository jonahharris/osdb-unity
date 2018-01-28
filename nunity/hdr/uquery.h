/*
 * Copyright (C) 2002 by Lucent Technologies
 *
 */

#ifndef _UQUERY
#define _UQUERY		1

#include <stdio.h>
#include "urelation.h"
#include "udbio.h"

/*
 * This file contains the query structure definitions needed
 * to do general queries on UNITY relations.
 */

/*
 * The query structure itself.
 */
#define Q_INIT		0x01	/* flag for saying that query is initialized */
#define Q_SORT		0x02	/* is sorting needed for the query? */
#define Q_UNIQUE	0x04	/* is uniqueness checking needed for query? */
#define Q_FRIENDLY	0x08	/* use user-friendly names in proj. attrs */
#define Q_NOEXPAND	0x10	/* don't expand "all" attr-query not init'ed */
#define Q_NOCASECMP	0x20	/* all comparisons are caseless */

/*
 * This next define is for checking the version of compiled queries.
 * It attempts to gracefully prevent core dumps when queries are not
 * recompiled when the structure of queries changes.
 *
 * THIS MACRO MUST BE UPDATED WHENEVER THE STRUCTURES USED IN COMPILED
 * QUERIES ARE CHANGED!!
 *
 * The version string in uversion.c should also be updated.
 */
#define UQUERY_VERSION	0x26

/*
 * Structure defining operations performed during query evaluation.
 */
typedef enum {
	UQOP_SELECT,
	UQOP_JOIN,
	UQOP_ANTIJOIN,
	UQOP_OUTERJOIN
} UQOPERATION;

#define UQF_DONE	0x01	/* operation completed */
#define UQF_PROJECT	0x02	/* project after operation completed */
#define UQF_RELRESET	0x04	/* reset relations at this point */
#define UQF_RECNUM	0x08	/* reference to rec# attr in this oper */
#define UQF_OJDIRECT	0x10	/* directional outer-join operation */

struct uqoperation {
	UQOPERATION oper;	/* type operation from above */
	unsigned char flags;	/* operation flags from above */
	unsigned short cmpcnt;	/* # entries in cmplist */
	unsigned short maxcmp;	/* # allocated entries in cmplist */
	unsigned short nonequal;  /* # non-equality compares in join opers */
	struct qnode *node1;	/* 1st node of operation */
	struct qnode *node2;	/* 2nd node of operation */
	struct queryexpr **cmplist;	/* query expressions for oper */
	struct uqoperation *next;	/* next operation */
};

struct uquery {
	unsigned char version;	/* version - MUST BE FIRST ENTRY IN STRUCT */
	unsigned char flags;		/* Query initialized (bound) yet? */
	unsigned short nodecnt;		/* cnt of relations in query */
	unsigned short attrcnt;		/* cnt of attr in projection list */
	unsigned short sortcnt;		/* cnt of attrs used for sorting */
	int (*tplfunc)();		/* function for each result tuple */
	struct qprojection *attrlist;	/* attr projection list */
	struct uqoperation *operlist;	/* operations to do query */
	struct qnode *nodelist[MAXRELATION];	/* relations in query */
};

/*
 * Query result query.  This structure stores the result of
 * a query for traversal by the user.
 */
struct qresult {
	unsigned short flags;		/* flags for query result */
	struct uquery *query;		/* query result is for */
	struct supernode *snode;	/* result of query */
	struct joinblock *curblk;	/* cur. tuple blk in traversal */
	char *unique;			/* data for checking uniqueness */
};

#define QR_INFILE	0x0001		/* some node has tuples in file */
#define QR_RESULT	0x0002		/* query result is still allocated */

/*
 * Below here are the implementation details of query evaluation.
 */

/*
 * Structure for attribute references within a query.
 */
struct attrref {
	struct qnode *rel;	/* relation of attribute reference */
	short attr;		/* attribute index in relation */
#ifdef	WHERE
	short modifier;		/* attribute modifier */
#endif
};

/*
 * Structures for projecting and sorting the attributes of a query.
 */
/* structure for specifying sub-fields to sort on */
struct qsubsort {
	char strtfld;		/* field to start sub-sort on */
	short strtchar;		/* char in strtfld to start sub-sort on */
	char endfld;		/* field to end sub-sort on */
	short endchar;		/* char in endfld to end sub-sort on */
				/* -1 means end of field */
};

#define QP_MAXSUBSRT	4	/* max sub-fields to sort on */

/* projected attribute termination/delimiter types */
#define	QP_TERMCHAR	0	/* UAT_TERMCHAR	*/
#define	QP_FIXEDWIDTH	1	/* UAT_FIXEDWIDTH */
 
/* attribute projection and sorting info */
struct qprojection {
	struct qnode *rel;	/* node for the attribute */
	short attr;		/* attribute number */
	unsigned short prwidth;	/* width attribute shoule be displayed with */
	char *prname;		/* attribute name printed for attribute */
	char *attorval;		/* attr (list) or (new) value */
	unsigned short attrwidth; /* attribute width */
	unsigned char attrtype;	/* attribute type */
	char terminate;		/* termination character on output */
	unsigned short flags;	/* flags about attribute sorting/projection */
	unsigned char justify;	/* 'l'eft, 'r'ight, or 'c'enter */
	unsigned char subcnt;	/* number of char sub-ranges to sort on */
	unsigned short sortattr; /* index of attr to sort in this position */
	unsigned short priority; /* priority of this attr in sorting */
	char *delim;		/* delimiters for sub-field sorting */
	struct qsubsort subsort[QP_MAXSUBSRT];	/* sub-fields to sort on */
};

/*
 * Flags for projection and sorting information.
 */
#define QP_NUMERIC	0x0001	/* do numeric comparisons */
#define QP_STRING	0x0002	/* do normal string comparisons */
#define QP_CASELESS	0x0003	/* do caseless string comparisons */
#define QP_DATE		0x0004	/* do date comparison */
#define QP_BINARY	0x0005	/* do binary integer comparison */
#define QP_OCTAL	0x0006	/* do octal integer comparison */
#define QP_HEXADECIMAL	0x0007	/* do hexadecimal integer comparison */
#define	QP_DICTIONARY	0x0008	/* do dictionary string comparisons */
#define	QP_PRINTABLE	0x0009	/* do printable string comparisons */
#define	QP_NOCASEDICT	0x000a	/* do caseless dictionary string comparisons */
#define	QP_NOCASEPRINT	0x000b	/* do caseless printable string comparisons */

#define QP_SORTMASK	0x000f	/* mask for getting type of sort above */

#define QP_DESCENDING	0x0010	/* sort in descending order, not ascending */
#define QP_RMBLANKS	0x0020	/* remove blanks at start of attr value */
#define QP_NODISPLAY	0x0040	/* don't display the attribute, only sort it */
#define QP_SORT		0x0080	/* sort on this attribute */
#define QP_SORTUNIQ	0x0100	/* do not skip when checking uniqueness */
#define QP_NEWVALUE	0x0200	/* "attorval" contains new attribute value */
#define QP_NOVERBOSE	0x0400	/* do not copy friendly name from relation */

/*
 * NOTES:
 *
 *	Projected attributes with the QP_NEWVALUE flag set
 *	(":value=..." modifier) are ignored when sorting
 *	and/or checking uniqueness.
 *
 *	QP_SORTUNIQ is (to be) set when an attribute is sorted
 *	in more than one way but is not displayed for each way.
 */

/*
 * Structure for passing the projection list to the application defined
 * tuple function (tplfunc in struct uquery).
 */
struct qprojtuple {
	struct utuple *tplptr;		/* tuple being projected */
	struct qprojection *projptr;	/* projection info for attribute */
};

/*
 * Below are the predefined numbers of special attributes.
 */
#define ATTR_ALL	-2	/* project all attr's from a relation */
#define ATTR_RECNUM	-3	/* record number for each tuple */
#define ATTR_SEEK	-4	/* seek location for each tuple */

/* check if attribute is one of the special ones above */
#define ATTR_SPECIAL(attrnum)	((attrnum) < -1)

/*
 * Definitions for where-clause representation in a query.
 */

/*
 * Operation for query expressions.  There are compare operations,
 * boolean operations, and a unary "not" operation.
 */
#define OPNOT		0x0001		/* expr is not true - OR w/ any expr */
#define OPEQ		0x0002		/* first operand = second */
#define OPLT		0x0004		/* first operand < second */
#define OPGT		0x0008		/* first operand > second */
#define OPNE		(OPNOT|OPEQ)	/* first operand != second */
#define OPLE		(OPNOT|OPGT)	/* first operand <= second */
#define OPGE		(OPNOT|OPLT)	/* first operand >= second */

#define OPIN		0x0010		/* set membership comparison */
#define OPNOTIN		(OPNOT|OPIN)	/* not in set */

#define OPANTIJOIN	0x0020		/* anti-join or set difference */

#define OPAND		0x0040		/* intersection of sub-expressions */
#define OPOR		0x0080		/* union of sub-expressions */
#define OPELSE		0x0100		/* selection of only one sub-expr. */

/*
 * Below are the defines for checking to see the general type of
 * operation we have.
 */
#define ISCOMPARE(op)	((op) < OPAND)
#define ISBOOL(op)	((op) >= OPAND)
#define ISSETCMP(op)	(((op) & ~OPNOT) == OPIN)

#define NOT(op)		(op) ^= OPNOT

/*
 * Below are miscellaneous flags about the query operation type.
 */
#define SUBEXPR		0x0001	/* expr is a parenthesized sub-expr. */
#define ISATTR1		0x0002	/* first oper is attr. name */
#define ISATTR2		0x0004	/* second oper is attr. name */
#define HASELSE		0x0008	/* expr contains an else-clause */
#define HASSELECT	0x0010	/* expr contains static selections */
#define HASJOIN		0x0020	/* expr contains joins or anti-joins */
#define OPOUTERJOIN	0x0080	/* outer join done on relations */
#define OPOJDIRECT	0x0100	/* directional outer join on relations */
#define OPNOCASE	0x0200	/* operation is caseless (i.e. regexpr) */

/*
 * Below are the types of data we can work on.  Except for REGEXPR, these
 * can be combined with any of the compare operators above.  REGEXPR can only
 * be combined with OPEQ or OPNE.
 */
typedef enum {
	QCMP_STRING,	/* attr's are strings */
	QCMP_CASELESS,	/* attr's are strings, but do caseless compare */
	QCMP_REGEXPR,	/* 1st attr is string - second reg. expr. */
	QCMP_NUMBER,	/* attr's are numbers */
	QCMP_DATE,	/* attr's are dates and include optional time */
	QCMP_DATEONLY	/* attr's are dates and ignore optional time */
} QCMPTYPE;

/*
 * Below are flags used while resolving expressions against tuples.
 */
#define QE_RIGHTCHILD	0x01	/* right child matched on else-clause */
#define QE_LEFTCHILD	0x02	/* left child matched on else-clause */
#define QE_TPLLEFT	0x04	/* current tuple matched left side of else */
#define QE_TPLRIGHT	0x08	/* current tuple matched right side of else */
#define QE_TPLMATCH	0x10	/* tuple matched this comparison */
#define QE_CHECKED	0x20	/* this comparison has been checked */


struct attrreflist {
	unsigned short cnt;
	struct attrref *list;
};

union qoperand {
	struct attrref attr;	/* attribute used for operand */
	struct queryexpr *expr;	/* sub-expression of query */
	char *strval;		/* static operand - only if second operand */
	double numval;		/* static operand - only if second operand */
	long intval;		/* record number - only if first operand */
	struct attrreflist alist;	/* list of attributes */
	char **strlist;		/* string list for set comparison */
};

struct queryexpr {
	unsigned short optype;	/* operation type */
	unsigned short opflags;	/* flags about operation, tag for union */
	QCMPTYPE cmptype;	/* comparison type */
	unsigned char trvflags;	/* traversal flags for expressions */
	char modifier1;		/* elem1 comparison/attribute modifier */
	char modifier2;		/* elem2 comparison/attribute modifier */
	union qoperand elem1;	/* first operand of expression */
	union qoperand elem2;	/* second operand of expression */
	struct utupleblk *tuples; /* if OPELSE, tuples matching this clause */
	struct qnode *nodeptr;	/* if OPELSE, node of matched tuples */
};

/*
 * Structure for blocks of tuples stored in a node
 */
#define TPLBLKSIZE	126	/* number of tuples grouped together */
				/* chosen to get 4k pages in allocation */

struct utupleblk {
	struct utupleblk *next;		/* next block of tuples */
	unsigned short tuplecnt;	/* number of tuples in block */
	struct utuple *tuples[TPLBLKSIZE];	/* tuple structures themselves */
};

/*
 * The node structure.  This represents one relation instance used
 * in the query.  It is called a node because of it's role in the
 * connection graph representing the query.
 */
struct qnode {
	struct urelation *rel;		/* relation info for node */
	unsigned short flags;		/* flags for query */
	unsigned long tuplecnt;		/* total tuples retrieved from table */
	unsigned long relsize;		/* size of original relation */
	unsigned long memsize;		/* size of tuples in memory */
	struct utupleblk *tuples;	/* tuples retrieved from this table */
	struct utupleblk *nextblk;	/* next blk of tuples for traversing */
	unsigned short nexttpl;		/* next tuple within next block */
	unsigned char nodenum;		/* index of node in query */
	struct supernode *snode;	/* supernode of this node */
	struct utuple *tmptpl;		/* temporary tuple struct */
	struct urelio relio;		/* I/O info for relation if real file*/
	/*
	 * The next entry is for flags about each real attr. or the relation.
	 * We do this as a maximum possible size to avoid malloc'ing the
	 * space - the space waste is negligible as sizes are now.
	 * There's nothing wrong with going back to malloc if sizes change.
	 */
	unsigned char memattr[MAXATT];
};

/*
 * Node flags.
 */
#define N_FORCESAVE	0x0001	/* force save of tuples in addtuple() */
#define N_OPENED	0x0002	/* file for relation is opened */
#define N_PROJECT	0x0004	/* node has attributes for projection */
#define N_PROJRECNUM	0x0008	/* node project recnum */
#define N_IGNORE	0x0010	/* mark tuples as ignored on deletes */
#define N_SELFILTER	0x0020	/* do select filter in gettuple() */
#define N_INFILE	0x0040	/* node is still in a file */
#define N_JOINDONE	0x0080	/* join has been done on node */

/*
 * This next definition is one that an application can use and OR in
 * to the node's flags.  It is used when the application wants to supply
 * in-memory tuples for a query, i.e., fake the query code into thinking
 * it has already read in the tuples for a node, when they were really
 * supplied (created in memory) by the application.  The tuples will not
 * be freed or modified by the query evaluator.
 */
#define N_APPTUPLES	0x8000	/* In-memory application tuples for node */

/*
 * Flags for node's "memattr" array.
 */
#define MA_SELECT	0x01	/* attr is used in selection conditions */
#define MA_PROJECT	0x02	/* attr is in projection list for query */
#define MA_INDEX	0x04	/* attr is indexed */
#define MA_MARKER	0x08	/* simple marker for traversing attr's */

/*
 * Composite node structures - "super nodes".  These nodes represent the
 * join of two or more relations together.
 */
typedef struct utuple *supertuple[MAXRELATION];

/*
 * Define how many tuples in each join block.  Chosen to get ~4k
 * pages in allocation.
 */
#define JOINBLKSIZE	126 * MAXRELATION

struct joinblock {
	struct joinblock *next;		/* next block of super tuples */
	unsigned short blkcnt;		/* number of super tuples in block */
	short curtpl;			/* current tuple in traversal */
	struct utuple *tuples[JOINBLKSIZE];  /* super tuples for this block */
};

struct supernode {
	long node_present;		/* bitmap for which nodes present */
	struct qresult *result;		/* result for this super node */
	struct supernode *next;		/* next super node in result */
	struct joinblock *joinptr;	/* tuples of supernode */
};

/*
 * Flags about individual nodes in supernode.
 */
#define SN_INFILE	0x01		/* this node is still in a file */

/*
 * Structure containing information about joins
 */
struct joininfo {
	struct supernode *snode;	/* supernode for join result */
	int (*joinfunc)();	/* function to call for joined tuples */
	int (*addfunc1)();	/* function for good tuple of node1 */
	int (*addfunc2)();	/* function for good tuple of node2 */
	int (*delfunc1)();	/* func. for deleting tpls from node1 */
	int (*delfunc2)();	/* func. for deleting tpls from node2 */
};

/*
 * Below are low-level structures for stepping through and
 * possibly updating each tuple in a relation.
 */
struct uperuseinfo {
	char flags;
	char updmode;
	struct urelation *relptr;
	char lockfile[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char tmptable[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	FILE *tmpfp;
	struct urelio relio;
};

extern struct uperuseinfo *init_peruse( );

/*
 * Below are low-level structures for inserting information in a relation.
 */
struct uinsertinfo {
	short flags;
	struct urelation *relptr;
	char lockfile[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char tmptable[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	FILE *tmpfp;
	short attrmap[MAXATT];
	short attrcnt;
};

extern struct uinsertinfo *init_insert( );
extern struct uinsertinfo *init_insert_tc( );	/* tuple check/count */

/*
 * extern int cmp_attrval( struct qprojection *projptr, char *attr1, char *attr2, int do_subsort );
 */
extern int cmp_attrval( );

#endif /* _UQUERY */
