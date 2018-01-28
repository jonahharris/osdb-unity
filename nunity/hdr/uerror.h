/*
 * Copyright (C) 2002 by Lucent Technologies
 *
 */

#define UE_NOERR	0	/* No Error */
#define UE_NOMEM	1	/* No memory available */
#define UE_ATTRNAME	2	/* Syntax error in attribute name */
#define UE_STAT		3	/* Bad Stat on File */
#define UE_WOPEN	4	/* Open File for Writing Failed */
#define UE_ROPEN	5	/* Open File for Reading Failed */
#define UE_TMPOPEN	6	/* Open tmp File for Writing Failed */
#define UE_RELNAME	7	/* Unrecognized Relation Name */
#define UE_NOTAB	8	/* syntax error - no tab separator */
#define UE_BSEOL	9	/* syntax error - \ at end of line */
#define UE_TERMCH	10	/* syntax error - no field termination char. */
#define UE_ATTRWIDTH	11	/* syntax error - bad width */
#define UE_ATTRTYPE	12	/* syntax error - bad field type */
#define UE_PRWIDTH	13	/* syntax error - bad print width */
#define UE_JUST		14	/* syntax error - bad justify character */
#define UE_NLTERM	15	/* syntax error - no newline for last field */
#define UE_NOINIT	16	/* no initialization previously done */
#define UE_CRLOCK	17	/* cannot create lock file */
#define UE_RELLOCK	18	/* relation already locked */
#define UE_WRITE	19	/* bad write */
#define UE_READ		20	/* bad read */
#define UE_LINK		21	/* relation link/unlink failed */
#define UE_SAMEINIT	22	/* initialization different than operation */
#define UE_NUMATTR	23	/* too many attributes */
#define UE_NUMREL	24	/* too many relations */
#define UE_TOOCMPLX	25	/* where-clause too complex */
#define UE_NODESCR	26	/* no descriptor file */
#define UE_DECOMPOSE	27	/* query decompostion failed - shouldn't happen */
#define UE_EOF		28	/* premature end-fo-file */
#define UE_RECSYN	29	/* bad syntax in record */
#define UE_BADQVERS	30	/* bad query version */
#define UE_UNRECATTR	31	/* unrecognized attribute name */
#define UE_IDXFAIL	32	/* index command failed */
#define UE_DUPATTR	33	/* duplicate attribute in desc. file */
#define UE_QUERYEVAL	34	/* internal error in query evaluation */
#define UE_RECSIZE	35	/* maximum record size exceeded */
#define UE_ATTSIZE	36	/* maximum attribute size exceeded */
#define UE_NLINATTR	37	/* new-line in attribute value */
#define UE_ATTWIDTH	38	/* attr. width doesn't match descriptor file */
#define UE_HASTAB	39	/* attribute name has embedded tab */
#define UE_OLDLINK	40	/* can't link orig. table to temp file. */
#define UE_UNLINK	41	/* can't unlink orig. table */
#define UE_LASTCHBS	42	/* last char of attr value is backslash */
#define UE_TMPLOCK	43	/* temp file already has lock file in place */
#define UE_ACMPMOD	44	/* unrecognized attribute/comparison modifier */
#define UE_RPOPEN	45	/* failed to open pipe to unpack relation */
#define UE_PACKEDEOF	46	/* failed to unpack relation */
#define UE_UNPACKMOD	47	/* cannot modify packed relation */
#define UE_TPLERROR	48	/* tuple error (uchecktuple == UCHECKFATAL) */
#define	UE_STATDIR	49	/* unsucessful stat(2) on relation directory */
#define	UE_LOCKSTAT	50	/* corrupt or missing lock file */
#define	UE_NOLOCK	51	/* could not find lock file in internal lock file table */
#define	UE_LOCKSTMP	52	/* corrupt or missing lock file - changes left in tmp file */
#define UE_BSINATTR	53	/* backslash in attribute value terminated by a backslash */

extern int uerror;
