/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <ctype.h>
#include "uquery.h"
#include "message.h"

extern char *escape_char();	/* from libmisc.a */
extern char *basename();	/* from libmisc.a */
extern char *calloc();
extern char *strchr();

/*
 * Structure for printing flags.
 *
 * This appears to be defined already on some machines.
 */
#ifndef __STDC__
typedef unsigned long ulong;	/* just for abbreviation purposes */
#endif
#ifdef NO_ULONG
typedef unsigned long ulong;	/* just for abbreviation purposes */
#endif


struct flags {
	char *flagname;		/* string to print (NULL is end of table.) */
	ulong flagmask;		/* mask for determining match */
	ulong flagval;		/* value to match */
};

static struct flags qnodeflags[] = {
	"N_PROJECT",	(ulong)N_PROJECT,	(ulong)N_PROJECT,
	"N_PROJRECNUM",	(ulong)N_PROJRECNUM,	(ulong)N_PROJRECNUM,
	NULL,		0,			0,
};

static struct flags optypelist[] = {
	"OPAND",	(ulong)~OPANTIJOIN,	(ulong)OPAND,
	"OPOR",		(ulong)~OPANTIJOIN,	(ulong)OPOR,
	"OPELSE",	(ulong)~OPANTIJOIN,	(ulong)OPELSE,
	"OPEQ",		(ulong)~OPANTIJOIN,	(ulong)OPEQ,
	"OPNE",		(ulong)~OPANTIJOIN,	(ulong)OPNE,
	"OPLT",		(ulong)~OPANTIJOIN,	(ulong)OPLT,
	"OPGT",		(ulong)~OPANTIJOIN,	(ulong)OPGT,
	"OPLE",		(ulong)~OPANTIJOIN,	(ulong)OPLE,
	"OPGE",		(ulong)~OPANTIJOIN,	(ulong)OPGE,
	"OPIN",		(ulong)~OPANTIJOIN,	(ulong)OPIN,
	"OPNOTIN",	(ulong)~OPANTIJOIN,	(ulong)OPNOTIN,
	"OPANTIJOIN",	(ulong)OPANTIJOIN,	(ulong)OPANTIJOIN,
	NULL,		0,			0,
};

static struct flags cmptypelist[] = {
	"QCMP_STRING",	0xffffffff,		(ulong)QCMP_STRING,
	"QCMP_CASELESS",0xffffffff,		(ulong)QCMP_CASELESS,
	"QCMP_REGEXPR",	0xffffffff,		(ulong)QCMP_REGEXPR,
	"QCMP_NUMBER",	0xffffffff,		(ulong)QCMP_NUMBER,
	"QCMP_DATE",	0xffffffff,		(ulong)QCMP_DATE,
	"QCMP_DATEONLY",0xffffffff,		(ulong)QCMP_DATEONLY,
	NULL,		0,			0,
};

static struct flags operflags[] = {
	"ISATTR1",	(ulong)ISATTR1,		(ulong)ISATTR1,
	"ISATTR2",	(ulong)ISATTR2,		(ulong)ISATTR2,
	"HASELSE",	(ulong)HASELSE,		(ulong)HASELSE,
	"HASSELECT",	(ulong)HASSELECT,	(ulong)HASSELECT,
	"HASJOIN",	(ulong)HASJOIN,		(ulong)HASJOIN,
	"SUBEXPR",	(ulong)SUBEXPR,		(ulong)SUBEXPR,
	"OPOUTERJOIN",	(ulong)OPOUTERJOIN,	(ulong)OPOUTERJOIN,
	"OPOJDIRECT",	(ulong)OPOJDIRECT,	(ulong)OPOJDIRECT,
	"OPNOCASE",	(ulong)OPNOCASE,	(ulong)OPNOCASE,
	NULL,		0,			0,
};

static struct flags queryflags[] = {
	"Q_SORT",	(ulong)Q_SORT,		(ulong)Q_SORT,
	"Q_UNIQUE",	(ulong)Q_UNIQUE,	(ulong)Q_UNIQUE,
	"Q_FRIENDLY",	(ulong)Q_FRIENDLY,	(ulong)Q_FRIENDLY,
	"Q_NOCASECMP",	(ulong)Q_NOCASECMP,	(ulong)Q_NOCASECMP,
	NULL,		0,			0,
};

static struct flags projflags[] = {
	"QP_NUMERIC",	(ulong)QP_SORTMASK,	(ulong)QP_NUMERIC,
	"QP_STRING",	(ulong)QP_SORTMASK,	(ulong)QP_STRING,
	"QP_CASELESS",	(ulong)QP_SORTMASK,	(ulong)QP_CASELESS,
	"QP_DATE",	(ulong)QP_SORTMASK,	(ulong)QP_DATE,
	"QP_BINARY",	(ulong)QP_SORTMASK,	(ulong)QP_BINARY,
	"QP_OCTAL",	(ulong)QP_SORTMASK,	(ulong)QP_OCTAL,
	"QP_HEXADECIMAL",(ulong)QP_SORTMASK,	(ulong)QP_HEXADECIMAL,
	"QP_DICTIONARY",(ulong)QP_SORTMASK,	(ulong)QP_DICTIONARY,
	"QP_PRINTABLE",	(ulong)QP_SORTMASK,	(ulong)QP_PRINTABLE,
	"QP_NOCASEDICT",(ulong)QP_SORTMASK,	(ulong)QP_NOCASEDICT,
	"QP_NOCASEPRINT",(ulong)QP_SORTMASK,	(ulong)QP_NOCASEPRINT,
	"QP_DESCENDING",(ulong)QP_DESCENDING,	(ulong)QP_DESCENDING,
	"QP_RMBLANKS",	(ulong)QP_RMBLANKS,	(ulong)QP_RMBLANKS,
	"QP_NODISPLAY",	(ulong)QP_NODISPLAY,	(ulong)QP_NODISPLAY,
	"QP_SORT",	(ulong)QP_SORT,		(ulong)QP_SORT,
	"QP_SORTUNIQ",	(ulong)QP_SORTUNIQ,	(ulong)QP_SORTUNIQ,
	"QP_NEWVALUE",	(ulong)QP_NEWVALUE,	(ulong)QP_NEWVALUE,
	"QP_NOVERBOSE",	(ulong)QP_NOVERBOSE,	(ulong)QP_NOVERBOSE,
	NULL,		0,			0,
};

static struct flags uqoptype[] = {
	"UQOP_SELECT",		0xffffffff,	(ulong)UQOP_SELECT,
	"UQOP_JOIN",		0xffffffff,	(ulong)UQOP_JOIN,
	"UQOP_ANTIJOIN",	0xffffffff,	(ulong)UQOP_ANTIJOIN,
	"UQOP_OUTERJOIN",	0xffffffff,	(ulong)UQOP_OUTERJOIN,
	NULL,			0,		0,
};

static struct flags uqopflags[] = {
	"UQF_PROJECT",	(ulong)UQF_PROJECT,	(ulong)UQF_PROJECT,
	"UQF_RELRESET",	(ulong)UQF_RELRESET,	(ulong)UQF_RELRESET,
	"UQF_RECNUM",	(ulong)UQF_RECNUM,	(ulong)UQF_RECNUM,
	"UQF_OJDIRECT",	(ulong)UQF_OJDIRECT,	(ulong)UQF_OJDIRECT,
	NULL,		0,		0,
};

static
prflags( fp, flgptr, flags )
FILE *fp;
register struct flags *flgptr;
ulong flags;
{
	register char *prefix;

	prefix = "";
	for( ; flgptr->flagname; flgptr++ ) {
		if ( (flags & flgptr->flagmask) == flgptr->flagval ) {
			fprintf( fp, "%s%s", prefix, flgptr->flagname );
			prefix = "|";
		}
	}

	if ( *prefix == '\0' )
		fputs( "0", fp );

	putc( ',', fp );
}

static void
prrels( fp, query, qname )
FILE *fp;
struct uquery *query;
char *qname;
{
	int i;
	struct qnode *nodeptr;

	fprintf( fp, "\
/*\n\
 * Relation structures (i.e., nodes) used in the query.\n\
 */\n\
static struct qnode _r_%s[] = {\n",
		qname );

	for( i = 0; i < query->nodecnt; i++ ) {
		nodeptr = query->nodelist[i];

		fputs( "\
\t{\n\
\t\t(struct urelation *)NULL,\t/* rel */\n\
",
			fp );

		fputs( "\t\t", fp );
		prflags( fp, qnodeflags, (ulong)nodeptr->flags );
		fputs( "\t\t\t/* flags */\n", fp );

		fprintf( fp, "\
\t\t0L,\t\t\t\t/* tuplecnt */\n\
\t\t0L,\t\t\t\t/* relsize */\n\
\t\t0L,\t\t\t\t/* memsize */\n\
\t\t(struct utupleblk *)NULL,\t/* tuples */\n\
\t\t(struct utupleblk *)NULL,\t/* nextblk */\n\
\t\t0,\t\t\t\t/* nexttpl */\n\
\t\t%u,\t\t\t\t/* nodenum */\n\
\t\t(struct supernode *)NULL,\t/* snode */\n\
\t\t(struct utuple *)NULL,\t\t/* tmptpl */\n\
\t\t{\t\t\t\t/* relio */\n\
\t\t\t0,\t/* flags */\n\
\t\t\t-1,\t/* fd */\n\
\t\t},\n\
\t\t{ 0 },\t\t\t\t/* memattr */\n\
\t},\n\
",
			nodeptr->nodenum );
	}
	fputs( "};\n\n", fp );
}

static int
anameidx( namelist, namecnt, aname )
char **namelist;
int namecnt;
char *aname;
{
	register int i;

	for ( i = 0; i < namecnt; i++ ) {
		if ( strcmp( namelist[i], aname ) == 0 )
			return( i );
	}

	return( -1 );
}

static void
chkattr( fp, attrlist, cntptr, nodeptr, attr )
FILE *fp;
char **attrlist;
int *cntptr;
struct qnode *nodeptr;
int attr;
{
	register char *aname;

	if ( ! ATTR_SPECIAL( attr ) )
	{
		aname = nodeptr->rel->attrs[attr].aname;
		if ( anameidx( attrlist, *cntptr, aname ) < 0 )
		{
			fprintf( fp, "\t\"%s\",\n", aname );
			attrlist[ (*cntptr)++ ] = aname;
		}
	}
}

static void
chkexpr( fp, qptr, attrlist, attrcnt )
FILE *fp;
struct queryexpr *qptr;
char **attrlist;
int *attrcnt;
{
	while( ISBOOL( qptr->optype ) ) {
		chkexpr( fp, qptr->elem1.expr, attrlist, attrcnt );
		qptr = qptr->elem2.expr;
	}

	if ( ISSETCMP( qptr->optype ) ) {
		register struct attrref *refptr;
		register int i;

		refptr = qptr->elem1.alist.list;
		for( i = 0; i < qptr->elem1.alist.cnt; i++, refptr++ )
			chkattr( fp, attrlist, attrcnt,
				refptr->rel, refptr->attr );
		return;
	}

	if ( qptr->opflags & ISATTR1 )
		chkattr( fp, attrlist, attrcnt,
			qptr->elem1.attr.rel, qptr->elem1.attr.attr );
	if ( qptr->opflags & ISATTR2 )
		chkattr( fp, attrlist, attrcnt,
			qptr->elem2.attr.rel, qptr->elem2.attr.attr );
}

static void
prattrs( fp, query, qname, attrlist, attrcnt )
FILE *fp;
struct uquery *query;
char *qname;
char **attrlist;
int *attrcnt;
{
	int i;
	struct uqoperation *operptr;

	fprintf( fp, "\
/*\n\
 * Attribute names used in the query, either projected\n\
 * or in comparisons.\n\
 */\n\
static char *_aname_%s[] = {\n",
		qname );

	for( i = 0; i < query->attrcnt; i++ )
		chkattr( fp, attrlist, attrcnt,
			query->attrlist[i].rel, query->attrlist[i].attr );

	for( operptr = query->operlist; operptr; operptr = operptr->next )
	{
		for( i = 0; i < operptr->cmpcnt; i++ )
			chkexpr( fp, operptr->cmplist[i], attrlist, attrcnt );
	}

	fputs( "\t(char *)NULL,\n};\n\n", fp );
}

static int
findexpr( exprlist, exprcnt, qptr )
struct queryexpr **exprlist;
int exprcnt;
struct queryexpr *qptr;
{
	int i;

	for( i = 0; i < exprcnt; i++ ) {
		if ( exprlist[i] == qptr )
			return( i );
	}

	return( -1 );
}

#define REGEXP_END	'\064'	/* terminating character for reg. expr. */
#define LIT_CHAR	'\024'	/* literal charater flag for reg. expr. */

static void
pr_revar( fp, re )
FILE *fp;
register char *re;
{
	/*
	 * The reg. expr. is really a variable name.  So skip the
	 * LIT_CHAR flags and print out the variable.
	 */
	re += 2;	/* jump over LIT_CHAR and '$' */
	while( *re != REGEXP_END || re[1] != '\0' ) {
		if ( *re != LIT_CHAR )
			putc( *re, fp );
		++re;
	}
}

static void
prregexp( fp, re )
FILE *fp;
register char *re;
{
	if ( is_reqvar( re ) )
		pr_revar( fp, re );
	else
	{
		putc( '"', fp );
		cstrnprt( fp, re, regexplen( re ) );
		putc( '"', fp );
	}
}

static void
prstr( fp, str )
FILE *fp;
register char *str;
{
	if ( is_strqvar( str ) )
		fputs( &str[1], fp );	/* it's a variable name */
	else {
		putc( '"', fp );
		cstrprt( fp, str );
		putc( '"', fp );
	}
}

static void
cmtoper( fp, qname, qptr, elem, attr, attrlist, attrcnt )
FILE *fp;
char *qname;
struct queryexpr *qptr;
union qoperand *elem;
int attr;
char **attrlist;
int attrcnt;
{
	if ( qptr->opflags & attr )
	{
		fprintf( fp, "\
\t\t\t/* &_r_%s[%d], */\t/* attr.rel */\n\
\t\t\t/* %d, */\t\t/* attr.attr */\n\
",
			qname, elem->attr.rel->nodenum,
			anameidx( attrlist, attrcnt,
				elem->attr.rel->rel->attrs[elem->attr.attr].aname ) );
	}
	else
	{
		fputs( "\t\t\t/* ", fp );
		switch( qptr->cmptype ) {
		case QCMP_NUMBER:
			fprintf( fp, "%f, */\t\t/* numval */\n",
				elem->numval );
			break;
		case QCMP_REGEXPR:
			prregexp( fp, elem->strval );
			fputs( ", */\t\t/* strval */\n", fp );
			break;
		case QCMP_STRING:
		case QCMP_DATE:
		case QCMP_DATEONLY:
			prstr( fp, elem->strval );
			fputs( ", */\t\t/* strval */\n", fp );
			break;
		default:
			fputs( "**unknown type**,\n", fp );
		}
	}
}

static int
cntexpr( qptr )
struct queryexpr *qptr;
{
	int cnt;

	cnt = 1;
	while( ISBOOL( qptr->optype ) ) {
		cnt += cntexpr( qptr->elem1.expr ) + 1;
		qptr = qptr->elem2.expr;
	}

	return( cnt );
}

static void
saveexprs( fp, qname, qptr, exprlist, exprcnt, attrlist, attrcnt )
FILE *fp;
char *qname;
register struct queryexpr *qptr;
struct queryexpr **exprlist;
int *exprcnt;
char **attrlist;
int attrcnt;
{
	struct attrref *refptr;
	unsigned int refcnt;
	register int i;

	while( ISBOOL( qptr->optype ) ) {
		if ( findexpr( exprlist, *exprcnt, qptr ) >= 0 )
			return;
		exprlist[ (*exprcnt)++ ] = qptr;

		saveexprs( fp, qname, qptr->elem1.expr, exprlist, exprcnt,
			attrlist, attrcnt );
		qptr = qptr->elem2.expr;
	}

	if ( findexpr( exprlist, *exprcnt, qptr ) >= 0 )
		return;
	exprlist[ (*exprcnt)++ ] = qptr;

	if ( ! ISSETCMP( qptr->optype ) )
		return;

	fprintf( fp, "static struct attrref alist%ld[] = {\n", (long)qptr );

	refptr = qptr->elem1.alist.list;
	refcnt = qptr->elem1.alist.cnt;
	for ( i = 0; i < refcnt; i++, refptr++ ) {
		fprintf( fp, "\t&_r_%s[%d], ", qname, refptr->rel->nodenum );
		switch( refptr->attr ) {
		case ATTR_ALL:
			fputs( "ATTR_ALL,\n", fp );
			break;
		case ATTR_RECNUM:
			fputs( "ATTR_RECNUM,\n", fp );
			break;
		case ATTR_SEEK:
			fputs( "ATTR_SEEK,\n", fp );
			break;
		default:
			fprintf( fp, "%d,\n",
				anameidx( attrlist, attrcnt,
					refptr->rel->rel->attrs[refptr->attr].aname ) );
			break;
		}
	}

	fputs( "};\n\n", fp );

	if ( ! is_ptrqvar( qptr->cmptype, qptr->elem2.strlist ) )
	{
		char **strlist;

		fprintf( fp, "static char *slist%ld[] = {\n", (long)qptr );
		for( strlist = qptr->elem2.strlist; *strlist; strlist++ )
		{
			putc( '\t', fp );
			if ( qptr->cmptype == QCMP_REGEXPR )
				prregexp( fp, *strlist );
			else
				prstr( fp, *strlist );
			fputs( ",\n", fp );
		}
		fputs( "};\n\n", fp );
	}
}

static void
prexprs( fp, qname, attrlist, attrcnt, exprlist, exprcnt )
FILE *fp;
char *qname;
char **attrlist;
int attrcnt;
struct queryexpr **exprlist;
int exprcnt;
{
	register unsigned int i;
	register struct queryexpr *qptr;

	fprintf( fp, "\
/*\n\
 * Comparison expressions used in the query operations.\n\
 */\n\
static struct queryexpr _exp_%s[] = {\n",
		qname );

	for( i = 0; i < exprcnt; i++ ) {
		qptr = exprlist[i];

		fputs( "\t{\n", fp );

		fputs( "\t\t", fp );
		prflags( fp, optypelist, (ulong)qptr->optype );
		fputs( "\t\t\t\t/* optype */\n", fp );

		fputs( "\t\t", fp );
		prflags( fp, operflags, (ulong)qptr->opflags );
		fputs( "\t\t/* opflags */\n", fp );

		fputs( "\t\t", fp );
		prflags( fp, cmptypelist, (ulong)qptr->cmptype );
		fputs( "\t\t\t/* cmptype */\n", fp );

		fputs( "\t\t0,\t\t\t\t/* trvflags */\n", fp );

		fprintf( fp, "\t\t%d,\t\t\t\t/* modifier1 */\n", qptr->modifier1 );
		fprintf( fp, "\t\t%d,\t\t\t\t/* modifier2 */\n", qptr->modifier2 );

		if ( ISBOOL( qptr->optype ) )
		{
			fprintf( fp, "\
\t\t/* elem1 */\n\
\t\t\t/* &_exp_%s[%d], */\t/* expr */\n\
\t\t/* elem2 */\n\
\t\t\t/* &_exp_%s[%d], */\t/* expr */\n\
",
				qname,
				findexpr( exprlist, exprcnt,
					qptr->elem1.expr ),
				qname,
				findexpr( exprlist, exprcnt,
					qptr->elem2.expr ) );
		}
		else if ( ISSETCMP( qptr->optype ) )
		{
			fprintf( fp, "\
\t\t/* elem1 */\n\
\t\t\t/* %d, */\t\t\t/*/* alist.cnt */\n\
\t\t\t/* alist%ld, */\t\t/* alist.list */\n\
",
				qptr->elem1.alist.cnt,
				(long)qptr );

			fputs( "\
\t\t/* elem2 */\n\
\t\t\t/* ",
				fp );
			if ( is_ptrqvar( qptr->cmptype, qptr->elem2.strlist ) )
			{
				if ( qptr->cmptype == QCMP_STRING )
					prstr( fp, qptr->elem2.strlist[0] );
				else if ( qptr->cmptype == QCMP_REGEXPR )
					pr_revar( fp, qptr->elem2.strlist[0] );
			}
			else
				fprintf( fp, "slist%ld", (long)qptr );
			fputs( ", */\t\t/* strlist */\n", fp );
		}
		else
		{
			fputs( "\t\t/* elem1 */\n", fp );
			cmtoper( fp, qname, qptr, &qptr->elem1, ISATTR1,
				attrlist, attrcnt );
			fputs( "\t\t/* elem2 */\n", fp );
			cmtoper( fp, qname, qptr, &qptr->elem2, ISATTR2,
				attrlist, attrcnt );
		}

		fputs( "\t},\n", fp );
	}

	fputs( "};\n\n", fp );
}

static void
properations( fp, query, qname, exprlist, exprcnt )
FILE *fp;
struct uquery *query;
char *qname;
struct queryexpr **exprlist;
int exprcnt;
{
	int i, cnt;
	struct uqoperation *operptr;

	if ( query->operlist == NULL )
		return;

	fprintf( fp, "\
/*\n\
 * List of all the comparison expressions used in the\n\
 * query operations.\n\
 */\n\
static struct queryexpr *_cmp_%s[] = {\n",
		qname );

	for( cnt = 0, operptr = query->operlist; operptr;
		operptr = operptr->next, cnt++ )
	{
		for( i = 0; i < operptr->cmpcnt; i++ )
		{
			fprintf( fp, "\t&_exp_%s[%d],", qname,
				findexpr( exprlist, exprcnt,
					operptr->cmplist[i] ) );
			if ( i == 0 )
				fprintf( fp, "\t/* operation #%d */", cnt );
			putc( '\n', fp );
		}
	}
	fputs( "};\n\n", fp );

	fprintf( fp, "\
/*\n\
 * Query operations used to evaluate the query.\n\
 */\n\
static struct uqoperation _op_%s[] = {\n",
		qname );
	for( i = 0, cnt = 0, operptr = query->operlist; operptr;
		operptr = operptr->next, i++ )
	{
		fputs( "\t{\n", fp );

		fputs( "\t\t", fp );
		prflags( fp, uqoptype, (ulong)operptr->oper );
		fputs( "\t\t/* oper */\n", fp );

		fputs( "\t\t", fp );
		prflags( fp, uqopflags, (ulong)operptr->flags );
		fputs( "\t\t/* flags */\n", fp );

		fprintf( fp, "\
\t\t%u,\t\t\t/* cmpcnt */\n\
\t\t%u,\t\t\t/* maxcmp */\n\
\t\t%u,\t\t\t/* nonequal */\n\
\t\t&_r_%s[ %u ],\t/* node1 */\n\
",
			operptr->cmpcnt,
			operptr->cmpcnt,  /* really maxcmp */
			operptr->nonequal,
			qname, operptr->node1->nodenum );

		fputs( "\t\t", fp );
		if ( operptr->node2 == NULL )
			fputs( "(struct qnode *)NULL,", fp );
		else
			fprintf( fp, "&_r_%s[ %u ],",
				qname, operptr->node2->nodenum );
		fputs( "\t\t/* node2 */\n", fp );

		fprintf( fp, "\t\t&_cmp_%s[ %d ],\t/* cmplist */\n",
			qname, cnt );
		cnt += operptr->cmpcnt;

		fputs( "\t\t", fp );
		if ( operptr->next == NULL )
			fputs( "(struct uqoperation *)NULL,", fp );
		else
			fprintf( fp, "&_op_%s[ %u ],",
				qname, i + 1 );
		fputs( "\t/* next */\n", fp );

		fputs( "\t},\n", fp );
	}
	fputs( "};\n\n", fp );
}

static void
expr_extern( fp, qptr )
FILE *fp;
struct queryexpr *qptr;
{
	while( ISBOOL( qptr->optype ) )
	{
		expr_extern( fp, qptr->elem1.expr );
		qptr = qptr->elem2.expr;
	}

	if ( ISSETCMP( qptr->optype ) )
	{
		register char **strlist;

		/*
		 * We have to decide if there is one string list
		 * variable (an array of pointers to chars) or
		 * multiple mixes of static and dynamic values.
		 * As a non-complete hueristic, we see if there
		 * is just one entry in this string list.  If there
		 * is we assume any query variable is an array of
		 * pointers; otherwise, there may be individual
		 * query variables.  Of course, for arrays the variable
		 * must be the first entry in the list.
		 */
		strlist = qptr->elem2.strlist;
		if ( qptr->cmptype == QCMP_STRING )
		{
			if ( is_ptrqvar( qptr->cmptype, qptr->elem2.strlist ) )
			{
				fprintf( fp, "extern char *%s[];\n",
					&strlist[0][1] );
			}
			else
			{
				while( *strlist )
				{
					if ( is_strqvar( strlist[0] ) )
						fprintf( fp, "extern char %s[];\n",
							&strlist[0][1] );
					strlist++;
				}
			}
		}
		else if ( qptr->cmptype == QCMP_REGEXPR )
		{
			if ( is_ptrqvar( qptr->cmptype, qptr->elem2.strlist ) )
			{
				fputs( "extern char *", fp );
				pr_revar( fp, strlist[0] );
				fputs( "[];\n", fp );
			}
			else
			{
				while( *strlist )
				{
					if ( is_reqvar( strlist[0] ) )
					{
						fputs( "extern char ", fp );
						pr_revar( fp, strlist[0] );
						fputs( "[];\n", fp );
					}
					strlist++;
				}
			}
		}
	}
	else if ( (qptr->opflags & ISATTR2) == 0 )
	{
		register char *str;

		str = qptr->elem2.strval;
		if ( qptr->cmptype == QCMP_STRING && is_strqvar( str ) )
		{
			fprintf( fp, "extern char %s[];\n", &str[1] );
		}
		else if ( qptr->cmptype == QCMP_REGEXPR && is_reqvar( str ) )
		{
			fputs( "extern char ", fp );
			pr_revar( fp, str );
			fputs( "[];\n", fp );
		}
	}
}

void
prexterns( fp, operptr )
FILE *fp;
struct uqoperation *operptr;
{
	int i;

	fputs( "\
/*\n\
 * Below are externs for any query variables used in this query.\n\
 */\n",
		fp );

	for( ; operptr; operptr = operptr->next )
	{
		if ( operptr->oper != UQOP_SELECT )
			continue;

		for( i = 0; i < operptr->cmpcnt; i++ )
			expr_extern( fp, operptr->cmplist[i] );
	}

	putc( '\n', fp );
}

static
prprojlist( fp, qname, projptr, projcnt, attrlist, attrcnt )
FILE *fp;
char *qname;
struct qprojection *projptr;
unsigned int projcnt;
char **attrlist;
int attrcnt;
{
	register int i, j;

	fprintf( fp, "static struct qprojection _prj_%s[] = {\n",
		qname );

	for ( i = 0; i < projcnt; i++, projptr++ ) {
		fputs( "\t{\n", fp );

		fprintf( fp, "\t\t&_r_%s[ %d ],\t/* rel */\n", qname,
			projptr->rel->nodenum );

		fputs( "\t\t", fp );
		switch( projptr->attr ) {
		case ATTR_ALL:
			fputs( "ATTR_ALL,", fp );
			break;
		case ATTR_RECNUM:
			fputs( "ATTR_RECNUM,", fp );
			break;
		case ATTR_SEEK:
			fputs( "ATTR_SEEK,", fp );
			break;
		default:
			fprintf( fp, "%d,",
				anameidx( attrlist, attrcnt,
					projptr->rel->rel->attrs[projptr->attr].aname ) );
			break;
		}
		fputs( "\t\t\t/* attr */\n", fp );

		fprintf( fp, "\t\t%u,\t\t\t/* prwidth */\n",
			projptr->prwidth );

		fputs( "\t\t", fp );
		if ( projptr->prname && (projptr->flags & QP_NODISPLAY) == 0 )
			fprintf( fp, "\"%s\", ", projptr->prname );
		else
			fputs( "(char *)NULL,", fp );
		fputs( "\t\t/* prname */\n", fp );

		if ( ( projptr->attr == ATTR_ALL ) &&
		     ( projptr->attorval != NULL ) )
		{
			char *colonptr;
			if ( ( colonptr = strchr( projptr->attorval, ':') ) != NULL )
				*colonptr = '\0';
			fprintf( fp, "\t\t\"%s\",\t\t\t/* all:nodisplay= */\n", projptr->attorval );
			if ( colonptr )
				*colonptr = ':';
		} else if ( ( projptr->attorval != NULL ) &&
		      ( projptr->flags & (QP_NEWVALUE|QP_NOVERBOSE) ) )
		{
			fputs( "\t\t\"", fp );
			/* C-string printing from libmisc.a */
			cstrprt( fp, projptr->attorval );
			fputs( "\",\t\t/* attorval */\n", fp );
		} else {
			fputs( "\t\t", fp );
			fputs( "(char *)NULL,", fp );
			fputs( "\t\t/* attorval */\n", fp );
		}

		fprintf( fp, "\t\t%u,\t\t\t/* attrwidth */\n",
			projptr->attrwidth );

		fputs( "\t\t", fp );
		if ( projptr->attrtype == QP_TERMCHAR )
		{
			fputs( "QP_TERMCHAR,", fp );
		}
		else if ( projptr->attrtype == QP_FIXEDWIDTH )
		{
			fputs( "QP_FIXEDWIDTH,", fp );
		}
		else
		{
			fprintf( fp, "%u,", projptr->attrtype );
		}
		fputs( "\t\t/* attrtype */\n", fp );

		fprintf( fp, "\t\t'%s',\t\t\t/* terminate */\n",
			escape_char( projptr->terminate ) );

		fputs( "\t\t", fp );
		prflags( fp, projflags, (ulong)projptr->flags );
		fputs( "\t\t/* flags */\n", fp );

		fprintf( fp, "\t\t'%c',\t\t\t/* justify */\n",
			projptr->justify );

		fprintf( fp, "\
\t\t%u,\t\t\t/* subcnt */\n\
\t\t%u,\t\t\t/* sortattr */\n\
\t\t%u,\t\t\t/* priority */\n\
",
			projptr->subcnt, projptr->sortattr,
			projptr->priority );

		fputs( "\t\t", fp );
		if ( projptr->delim == NULL )
			fputs( "(char *)NULL,", fp );
		else {
			putc( '"', fp );
			/* C-string printing from libmisc.a */
			cstrprt( fp, projptr->delim );
			fputs( "\",", fp );
		}
		fputs( "\t\t/* delim */\n", fp );

		fputs( "\t\t{\t\t\t/* subsort[] */\n", fp );
		for( j = 0; j < projptr->subcnt; j++ ) {
			fprintf( fp, "\t\t\t{\n\
\t\t\t%d,\t/* strtfld */\n\
\t\t\t%d,\t/* strtchar */\n\
\t\t\t%d,\t/* endfld */\n\
\t\t\t%d,\t/* endchar */\n\
\t\t\t},\n",
				projptr->subsort[j].strtfld,
				projptr->subsort[j].strtchar,
				projptr->subsort[j].endfld,
				projptr->subsort[j].endchar );
		}
		if ( projptr->subcnt == 0 )
			fputs( "\t\t\t0,\t/* no sub-field sorting */\n", fp );
		fputs( "\t\t},\n", fp );

		fputs( "\t},\n", fp );
	}

	fputs( "};\n\n", fp );
}

static void
prqinfo( fp, query, qname )
FILE *fp;
struct uquery *query;
char *qname;
{
	register int i;

	fprintf( fp, "struct uquery %s = {\n", qname );

	fprintf( fp, "\t0x%x,\t\t\t/* version */\n", query->version );

	putc( '\t', fp );
	prflags( fp, queryflags, (ulong)query->flags );
	fputs( "\t\t\t/* flags */\n", fp );

	fprintf( fp, "\
\t%u,\t\t\t/* nodecnt */\n\
\t%u,\t\t\t/* attrcnt */\n\
\t%u,\t\t\t/* sortcnt */\n\
\t(int (*)())NULL,\t/* tplfunc */\n\
",
		query->nodecnt,
		query->attrcnt,
		query->sortcnt );

	putc( '\t', fp );
	if ( query->attrcnt != 0 )
		fprintf( fp, "_prj_%s,", qname );
	else
		fputs( "(struct qprojection *)NULL,", fp );
	fputs( "\t/* attrlist */\n", fp );

	putc( '\t', fp );
	if ( query->operlist == NULL )
		fputs( "(struct uqoperation *)NULL,", fp );
	else
		fprintf( fp, "_op_%s, ", qname );
	fputs( "\t/* operlist */\n", fp );

	fputs( "\t{\t\t\t/* nodelist[] */\n", fp );
	for( i = 0; i < query->nodecnt; i++ ) {
		fprintf( fp, "\t\t&_r_%s[%d],\n", qname, i );
	}
	fputs( "\t},\n", fp );

	fputs( "};\n\n", fp );
}

static void
propassign( fp, qname, exprnum, qptr, elem, attr, attrlist, attrcnt,
	exprlist, exprcnt )
FILE *fp;
char *qname;
int exprnum;
struct queryexpr *qptr;
union qoperand *elem;
int attr;
char **attrlist;
int attrcnt;
struct queryexpr **exprlist;
int exprcnt;
{
	if ( ISBOOL( qptr->optype ) ) {
		fprintf( fp, "expr = &_exp_%s[%d];\n",
			qname,
			findexpr( exprlist, exprcnt, elem->expr ) );
	}
	else if ( qptr->opflags & attr ) {
		fprintf( fp, "attr.rel = &_r_%s[%d];\n", qname,
			elem->attr.rel->nodenum );
		fprintf( fp, "\t_exp_%s[%d].elem%d.attr.attr = %d;\n",
			qname, exprnum, attr == ISATTR1 ? 1 : 2,
			anameidx( attrlist, attrcnt,
				elem->attr.rel->rel->attrs[elem->attr.attr].aname ));
	}
	else {
		switch( qptr->cmptype ) {
		case QCMP_NUMBER:
			fprintf( fp, ".numval = %f;\n", elem->numval );
			break;
		case QCMP_REGEXPR:
			fputs( "strval = ", fp );
			prregexp( fp, elem->strval );
			fputs( ";\n", fp );
			break;
		case QCMP_STRING:
		case QCMP_DATE:
		case QCMP_DATEONLY:
			fputs( "strval = ", fp );
			prstr( fp, elem->strval );
			fputs( ";\n", fp );
			break;
		default:
			fputs( "**unknown type**;\n", fp );
		}
	}
}

static void
initop( fp, qname, qptr, exprnum, attrlist, attrcnt, exprlist, exprcnt )
FILE *fp;
char *qname;
struct queryexpr *qptr;
int exprnum;
char **attrlist;
int attrcnt;
struct queryexpr **exprlist;
int exprcnt;
{
	if ( ISSETCMP( qptr->optype ) )
	{
		fprintf( fp, "\t_exp_%s[%d].elem1.alist.list = alist%ld;\n",
			qname, exprnum, (long)qptr );
		fprintf( fp, "\t_exp_%s[%d].elem1.alist.cnt = %d;\n",
			qname, exprnum, qptr->elem1.alist.cnt );
		fprintf( fp, "\t_exp_%s[%d].elem2.strlist = ",
			qname, exprnum, (long)qptr );
		if ( is_ptrqvar( qptr->cmptype, qptr->elem2.strlist ) )
		{
			if ( qptr->cmptype == QCMP_STRING )
				prstr( fp, qptr->elem2.strlist[0] );
			else if ( qptr->cmptype == QCMP_REGEXPR )
				pr_revar( fp, qptr->elem2.strlist[0] );
		}
		else
			fprintf( fp, "slist%ld", (long)qptr );
		fputs( ";\n", fp );
	}
	else
	{
		fprintf( fp, "\t_exp_%s[%d].elem1.", qname, exprnum );
		propassign( fp, qname, exprnum, qptr, &qptr->elem1, ISATTR1,
			attrlist, attrcnt, exprlist, exprcnt );

		fprintf( fp, "\t_exp_%s[%d].elem2.", qname, exprnum );
		propassign( fp, qname, exprnum, qptr, &qptr->elem2, ISATTR2,
			attrlist, attrcnt, exprlist, exprcnt );
	}
}

static int
cntrelrefs( relname, nodelist, nodecnt )
char *relname;
struct qnode **nodelist;
int nodecnt;
{
	register int i, cnt;

	cnt = 1;
	for( i = 0; i < nodecnt; i++ ) {
		if ( strcmp( relname, basename( nodelist[i]->rel->path ) ) == 0 )
			cnt++;
	}
	return( cnt );
}

char *
getrelname( path )
char *path;
{
	static char buf[20];
	register char *relname;

	relname = basename( path );
	strcpy( buf, relname );
	for( relname = buf; *relname; relname++ ) {
		if ( ! isalpha( *relname ) )
			*relname = '_';
	}

	return( buf );
}

static void
prfunc( fp, query, qname, attrlist, attrcnt, exprlist, exprcnt )
FILE *fp;
struct uquery *query;
char *qname;
char **attrlist;
int attrcnt;
struct queryexpr **exprlist;
int exprcnt;
{
	int i;
	char *relname;

	fprintf( fp, "\
/*\n\
 * Below is the initialization function for the above query.  This\n\
 * function must be called once before the above query is passed\n\
 * to the queryeval() function.\n\
 */\n\
int\n\
init%s( ",
		qname );

	for( i = 0; i < query->nodecnt; i++ ) {
		relname = getrelname( query->nodelist[i]->rel->path );
		fprintf( fp, "r%d_%s%s ", i + 1, relname,
			i != query->nodecnt - 1 ? "," : "" );
	}
	fputs( ")\n", fp );

	for( i = 0; i < query->nodecnt; i++ ) {
		relname = getrelname( query->nodelist[i]->rel->path );
		fprintf( fp, "struct urelation *r%d_%s;\n", i + 1,
			relname );
	}
	fprintf( fp, "\
{\n\
\tif ( %s.flags & Q_INIT )\n\
\t\treturn( TRUE );\n\n",
		qname );

	for( i = 0; i < exprcnt; i++ )
		initop( fp, qname, exprlist[i], i, attrlist, attrcnt,
			exprlist, exprcnt );

	putc( '\n', fp );

	for( i = 0; i < query->nodecnt; i++ ) {
		relname = getrelname( query->nodelist[i]->rel->path );
		fprintf( fp, "\t_r_%s[%d].rel = r%d_%s;\n", qname, i,
			i + 1, relname );
	}

	fprintf( fp, "\n\treturn( bindquery( &%s, ", qname );
	if ( exprcnt > 0 )
		fprintf( fp, "_exp_%s, %d,", qname, exprcnt );
	else
		fputs( "(struct queryexpr *)NULL, 0,", fp );

	fprintf( fp, "\n\t\t\t_aname_%s ) );\n}\n", qname );
}

int
genquery( fp, qname, query )
FILE *fp;
char *qname;
struct uquery *query;
{
	int exprcnt;
	struct queryexpr **exprlist;
	int attrcnt;
	char **attrlist;
	int i;
	unsigned int bufcnt;

	bufcnt = 0;
	for( i = 0; i < query->nodecnt; i++ )
		bufcnt += query->nodelist[i]->rel->attrcnt;

	attrlist = (char **)calloc( bufcnt, sizeof( *attrlist ) );
	if ( attrlist == NULL )
	{
		fputs( "*** cannot allocate space for attribute info\n", fp );
		prmsg( MSG_ERROR, "cannot allocate space for attribute info" );
		return( FALSE );
	}
	attrcnt = 0;

	prexterns( fp, query->operlist );

	prattrs( fp, query, qname, attrlist, &attrcnt );

	prrels( fp, query, qname );

	exprcnt = 0;
	if ( query->operlist == NULL )
	{
		exprlist = NULL;
	}
	else
	{
		struct uqoperation *operptr;

		bufcnt = 0;
		for( operptr = query->operlist; operptr;
			operptr = operptr->next )
		{
			for( i = 0; i < operptr->cmpcnt; i++ )
				bufcnt += cntexpr( operptr->cmplist[i] );
		}
	
		exprlist = (struct queryexpr **)calloc( bufcnt, sizeof( *exprlist ));
		if ( exprlist == NULL )
		{
			fputs( "*** cannot allocate space for query-expression info\n",
				fp );
			prmsg( MSG_ERROR, "cannot allocate space for query->expression info" );
			return( FALSE );
		}

		for( operptr = query->operlist; operptr;
			operptr = operptr->next )
		{
			for( i = 0; i < operptr->cmpcnt; i++ )
				saveexprs( fp, qname, operptr->cmplist[i],
					exprlist, &exprcnt,
					attrlist, attrcnt );
		}

		prexprs( fp, qname, attrlist, attrcnt, exprlist, exprcnt );

		properations( fp, query, qname, exprlist, exprcnt );
	}

	if ( query->attrcnt != 0 ) {
		prprojlist( fp, qname, query->attrlist, query->attrcnt,
			attrlist, attrcnt );
	}

	prqinfo( fp, query, qname );

	prfunc( fp, query, qname, attrlist, attrcnt, exprlist, exprcnt );

	if ( exprlist )
		free( exprlist );
	if ( attrlist )
		free( attrlist );

	return( TRUE );
}
