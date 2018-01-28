%{
#ifdef __STDC__
#include <stdlib.h>
#ifndef NO_LIBGEN
#include <libgen.h>
#endif
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifndef	WHERE
#define	WHERE
#endif
#include "uquery.h"
#include "uerror.h"
#include "message.h"

extern double atof();
#ifndef SVR4
#ifndef __STDC__
extern char *calloc(), *malloc();
#endif
#endif
#ifndef	__STDC__
extern long strtol();
extern char *strrchr();
#endif
extern char *nc_regcmp();
extern struct qnode *exprnode();
extern struct queryexpr *newqexpr();
extern struct queryexpr *expnd_setop();
extern char **parse_strlist();

static struct queryexpr *querytree;	/* final result of parse */
static char force_str, force_strlist;	/* flags for saying next arg IS string */
static char in_num_cmp;			/* flag in a numeric comparison */

static char *nomemory = "cannot allocate space for query expression";

static char nocase;		/* all comparisons caseless? */
static unsigned short queryflags;	/* query flags for special handling */

struct querycompare {
	QCMPTYPE cmptype;
	unsigned short optype;
	unsigned short opflags;
};

typedef union {		/* operand definition for yacc */
	struct attrref attr;	/* attribute used for operand */
	struct queryexpr *expr;	/* sub-expression of query */
	char *strval;		/* static string */
	double numval;		/* static string */
	struct attrreflist alist;	/* list of attributes */
	struct querycompare cmp;	/* comparison */
} YYSTYPE;
#ifdef __CYGWIN__
/* Generated file has "#ifdef YYSTYPE", but on cygwin, having a union by
 * that name is apparently not good enough.
 */
#define YYSTYPE YYSTYPE
#endif

#define T_ENDTOKEN	0		/* end-of-input token */
#define T_UNKNOWN	-1		/* some unknown value */
%}

%start	start

%left T_OR
%left T_AND
%right '!' T_ANTIJOIN
%left T_ELSE
%left T_STROPER T_ATTROPER T_NUMOPER T_REOPER T_INSET
%left '(' ')'

%token T_WHERE T_STRING T_NUMBER T_HEXSTRING T_ATTRIBUTE T_ATTRLIST T_STRLIST

%type <expr> qexpr start
%type <cmp> setoper T_INSET T_STROPER T_NUMOPER T_REOPER T_ATTROPER
%type <strval> T_STRING T_STRLIST
%type <attr> T_ATTRIBUTE
%type <numval> T_NUMBER
%type <alist> T_ATTRLIST

%%
start	:	where qexpr
		{
			querytree = $2;
		}
	;

where	:	T_WHERE
	|
	;

qexpr	:	T_ATTRIBUTE T_ATTROPER T_ATTRIBUTE
		{
			register struct queryexpr *qptr;

			if ( (qptr = newqexpr( )) == NULL ) {
				yyerror( nomemory );
				YYERROR;
			}

			qptr->optype = $2.optype;
			qptr->cmptype = $2.cmptype;
			qptr->opflags = $2.opflags | ( $1.rel != $3.rel ?
						HASJOIN : HASSELECT );
			if ( qptr->cmptype == QCMP_REGEXPR ) {
				yyerror( "cannot do field-to-field regular expression comparison" );
				YYERROR;
			}
			if ( $1.attr == ATTR_RECNUM ||
				$3.attr == ATTR_RECNUM ||
				$1.attr == ATTR_SEEK ||
				$3.attr == ATTR_SEEK )
			{
				if (( $2.cmptype == QCMP_DATE ) ||
				    ( $2.cmptype == QCMP_DATEONLY )) {
					yyerror( "cannot do date comparison on rec# or seek# field" );
					YYERROR;
				}
				qptr->cmptype = QCMP_NUMBER;
			}

			if ( qptr->cmptype == QCMP_NUMBER ) {
				if ( $1.modifier ) {
					qptr->modifier1 = $1.modifier;
					$1.modifier = 0;	/* do not copy to attr struct */
				}
				if ( $3.modifier ) {
					qptr->modifier2 = $3.modifier;
					$3.modifier = 0;	/* do not copy to attr struct */
				}
			} else {
				if (( $1.modifier ) || ( $3.modifier )) {
					yyerror( "cannot use numeric modifier with non-numeric comparison" );
					YYERROR;
				}
			}
			qptr->elem1.attr = $1;
			qptr->elem2.attr = $3;

			$$ = qptr;
		}
	|	T_ATTRLIST setoper isastrlist T_STRLIST
		{
			register struct queryexpr *qptr;
			char **strlist;

			/* go back to non-kludge mode */
			force_strlist = FALSE;

			if ( $2.optype != OPIN && $2.optype != OPNOTIN )
			{
				yyerror( "illegal set comparison" );
				YYERROR;
			}

			strlist = parse_strlist( $4, $1.cnt );
			if ( strlist == NULL )
			{
				yyerror( "syntax error in attribute value list" );
				YYERROR;
			}

			if ( $2.cmptype == QCMP_REGEXPR &&
				! cmprexplist( strlist, $2.opflags & OPNOCASE ) )
			{
				YYERROR;
			}

			if ( samerefnode( $1.list, $1.cnt ) )
			{
				/*
				 * All attributes from the same relation.
				 */
				if ( (qptr = newqexpr( )) == NULL ) {
					yyerror( nomemory );
					YYERROR;
				}

				qptr->optype = $2.optype;
				qptr->cmptype = $2.cmptype;
				qptr->opflags = $2.opflags;
				qptr->elem1.alist = $1;
				qptr->elem2.strlist = strlist;
			}
			else
			{
				/*
				 * Attributes are from different relations.
				 * We can't evaluate this directly as a
				 * set operation, so we expand it out
				 * and evaluate it that way.
				 *
				 * If we're in a cmpquery mode, we can't
				 * allow a lone query variable because
				 * this is not expandable.
				 */
				if ( (queryflags & Q_NOEXPAND) &&
					is_ptrqvar( $2.cmptype, strlist ) )
				{
					yyerror( "cannot have single query variable in set operation when attributes from multiple relations" );
					YYERROR;
				}

				qptr = expnd_setop( $1.list, $1.cnt, strlist,
						$2.cmptype, $2.opflags );
				if ( qptr == NULL )
					YYERROR;

				if ( $2.optype == OPNOTIN )
					NOT( qptr->optype );
			}

			$$ = qptr;
		}
	|	T_ATTRIBUTE setoper isastrlist T_STRLIST
		{
			register struct queryexpr *qptr;
			char **strlist;

			force_strlist = FALSE;	/* go back to non-kludge mode */

			if ( $1.modifier ) {
				if ( $2.cmptype == QCMP_NUMBER )
					yyerror( "cannot use numeric modifier with set comparison" );
				else
					yyerror( "cannot use numeric modifier with non-numeric comparison" );
				YYERROR;
			}

			strlist = parse_strlist( $4, 1 );
			if ( strlist == NULL ) {
				yyerror( "syntax error in attribute value list" );
				YYERROR;
			}

			if ( (qptr = newqexpr( )) == NULL ) {
				yyerror( nomemory );
				YYERROR;
			}

			qptr->optype = $2.optype;
			qptr->cmptype = $2.cmptype;
			qptr->opflags = $2.opflags;
			if ( qptr->optype != OPIN && qptr->optype != OPNOTIN ){
				yyerror( "illegal set comparison" );
				YYERROR;
			}
			if ( qptr->cmptype == QCMP_REGEXPR &&
					! cmprexplist( strlist, $2.opflags & OPNOCASE ) )
				YYERROR;


			qptr->elem1.alist.list = (struct attrref *)calloc( 1, sizeof( struct attrref ) );
			if ( qptr->elem1.alist.list == NULL ) {
				set_uerror( UE_NOMEM );
				yyerror( nomemory );
				YYERROR;
			}
			*qptr->elem1.alist.list = $1;
			qptr->elem1.alist.cnt = 1;
			qptr->elem2.strlist = strlist;

			$$ = qptr;
		}
	|	T_ATTRIBUTE T_STROPER isastring T_STRING
		{
			register struct queryexpr *qptr;

			force_str = FALSE;	/* go back to non-kludge mode */

			if ( $1.modifier ) {
				yyerror( "cannot use numeric modifier with non-numeric comparison" );
				YYERROR;
			}

			if ( (qptr = newqexpr( )) == NULL ) {
				yyerror( nomemory );
				YYERROR;
			}

			qptr->optype = $2.optype;
			qptr->cmptype = $2.cmptype;
			qptr->opflags = $2.opflags;
			qptr->elem1.attr = $1;
			if ( $1.attr == ATTR_RECNUM ||
				$1.attr == ATTR_SEEK )
			{
				if (( $2.cmptype == QCMP_DATE ) ||
				    ( $2.cmptype == QCMP_DATEONLY )) {
					yyerror( "cannot do date comparison on rec# or seek# field" );
					YYERROR;
				}
				qptr->cmptype = QCMP_NUMBER;
				qptr->elem2.numval = atof( $4 );
			}
			else
				qptr->elem2.strval = $4;

			$$ = qptr;
		}
	|	T_ATTRIBUTE T_REOPER isastring T_STRING
		{
			register struct queryexpr *qptr;

			force_str = FALSE;	/* go back to non-kludge mode */

			if ( $1.modifier ) {
				yyerror( "cannot use numeric modifier with non-numeric comparison" );
				YYERROR;
			}

			if ( (qptr = newqexpr( )) == NULL ) {
				yyerror( nomemory );
				YYERROR;
			}

			qptr->optype = $2.optype;
			qptr->cmptype = $2.cmptype;
			qptr->opflags = $2.opflags;
			if ( qptr->optype != OPEQ && qptr->optype != OPNE )
			{
				yyerror( "illegal regular espression comparison" );
				YYERROR;
			}
			qptr->elem1.attr = $1;
			qptr->elem2.strval = nc_regcmp( ($2.opflags & OPNOCASE) || nocase, $4, NULL );
			if ( qptr->elem2.strval == NULL )
			{
				yyerror( "syntax error in regular expression '%s'", $4 );
				YYERROR;
			}

			$$ = qptr;
		}
	|	T_ATTRIBUTE T_NUMOPER isnumcmp T_NUMBER
		{
			register struct queryexpr *qptr;

			in_num_cmp = FALSE;	/* go back to non-kludge mode */

			if ( (qptr = newqexpr( )) == NULL )
			{
				yyerror( nomemory );
				YYERROR;
			}

			if ( $1.modifier ) {
				qptr->modifier1 = $1.modifier;
				$1.modifier = 0;	/* do not copy to attr struct */
			}

			qptr->optype = $2.optype;
			qptr->cmptype = $2.cmptype;
			qptr->opflags = $2.opflags;
			qptr->elem1.attr = $1;
			qptr->elem2.numval = $4;

			$$ = qptr;
		}
	|	qexpr T_ELSE qexpr
		{
			register struct queryexpr *qptr;
			struct qnode *node1;

			if ( has_setop( $1 ) || has_setop( $3 ) )
			{
				yyerror( "sub-clauses of ELSE-operation cannot contain set operations" );
				YYERROR;
			}
			node1 = exprnode( $1 );
			if ( node1 == NULL || node1 != exprnode( $3 ) )
			{
				yyerror( "ELSE-operation contains references to multiple relations" );
				YYERROR;
			}
			if ( (qptr = newqexpr( )) == NULL ) {
				yyerror( nomemory );
				YYERROR;
			}

			qptr->optype = OPELSE;
			qptr->opflags = HASELSE|HASSELECT;
			qptr->elem1.expr = $1;
			qptr->elem2.expr = $3;

			$$ = qptr;
		}
	|	qexpr T_AND qexpr
		{
			register struct queryexpr *qptr;

			if ( (qptr = newqexpr( )) == NULL ) {
				yyerror( nomemory );
				YYERROR;
			}

			qptr->optype = OPAND;
			qptr->opflags = ($1->opflags | $3->opflags) &
						(HASELSE|HASSELECT|HASJOIN);
			qptr->elem1.expr = $1;
			qptr->elem2.expr = $3;

			$$ = qptr;
		}
	|	qexpr T_OR qexpr
		{
			register struct queryexpr *qptr;

			if ( (qptr = newqexpr( )) == NULL ) {
				yyerror( nomemory );
				YYERROR;
			}

			qptr->optype = OPOR;
			qptr->opflags = ($1->opflags | $3->opflags) &
						(HASELSE|HASSELECT|HASJOIN);
			qptr->elem1.expr = $1;
			qptr->elem2.expr = $3;

			$$ = qptr;
		}
	|	'(' qexpr ')'
		{
			/*
			 * Only have to worry about boolean operations that
			 * are parenthesized, so only do them.
			 */
			if ( ISBOOL( $2->optype ) )
				$2->opflags |= SUBEXPR;

			$$ = $2;
		}
	|	'!' qexpr
		{
			NOT( $2->optype );
			$$ = $2;
		}
	|	T_ANTIJOIN qexpr
		{
			/*
			 * This next check is a temporary restriction
			 * until I can figure out how to handle the
			 * general case.  Then I'll remove the restriction.
			 */
			if ( ISBOOL( $2->optype ) )
			{
				yyerror( "set-difference operator usable only on individual comparisons, not on arbitrary expressions" );
				YYERROR;
			}
			$2->optype ^= OPANTIJOIN;
			$$ = $2;
		}
	;

isastring :	/* empty state to say next token is coerced to a string */
		{
			force_str = TRUE;	/* allow ANYTHING in next arg */
		}
	;

isastrlist :	/* empty state to say next token is coerced to a string list */
		{
			force_strlist = TRUE;	/* allow ANYTHING in next arg */
		}
	;

isnumcmp :	/* empty state to say in a numeric comparison */
		{
			in_num_cmp = TRUE;	/* 0x strings come as numbers */
		}
	;

setoper :	T_INSET
		{
			$$ = yylval.cmp;
		}
	|	'!' T_INSET
		{
			$$ = $2;
			NOT( $$.optype );
		}
	;

%%

/*
 * Below here is the lexical analyzer for the where clause parser.
 * The lexical analyzer, is slightly modified from the a normal
 * lexical analyzer.  Instead of working on a character-by-character
 * basis, it works on an argument-by-argument basis from the command
 * line.
 */

/*
 * There are several tables in the lexical analyzer, one for keywords,
 * one for comparisons, and one for other known objects (the
 * "types" table).
 */
struct keyword {
	char *string;		/* keyword string */
	int token;		/* token type to pass back to parser */
};

/*
 * Below are the keyword tables.
 */
static struct keyword keywords[] = {
	"and",	T_AND,
	"or",	T_OR,
	"else",	T_ELSE,

	"(",	'(',
	")",	')',

	"!",	'!',
	"not",	'!',

	"-",	T_ANTIJOIN,
	"--",	T_ANTIJOIN,
	"minus",	T_ANTIJOIN,

	"where", T_WHERE,
};

#define LASTWORD &keywords[ sizeof( keywords ) / sizeof( struct keyword ) ]

struct keycompare {
	char *string;		/* comparison string */
	unsigned char optype;	/* operation type */
};

static struct keycompare cmplist[] = {
	"=",	OPEQ,
	"eq",	OPEQ,
	"==",	OPEQ,
	"~",	OPEQ,
	"!~",	OPNE,

	"!=",	OPNE,
	"<",	OPLT,
	"<=",	OPLE,
	">",	OPGT,
	">=",	OPGE,

	"in",	OPIN,

	"ne",	OPNE,
	"lt",	OPLT,
	"le",	OPLE,
	"gt",	OPGT,
	"ge",	OPGE,
};

#define LASTCMP &cmplist[ sizeof( cmplist ) / sizeof( struct keycompare ) ]

struct types {
	char *regexpr;		/* regular expression for this type */
	int token;		/* token type to pass back to parser */
};

extern char re_integer[];
extern char re_fraction[];
extern char re_decimal[];
extern char re_attribute[];
extern char re_recnum[];
extern char re_seeknum[];
extern char re_attr_mod[];
extern char re_attrlist[];
extern char re_hexstring[];

static struct types typelist[] = {
	re_attribute,	T_ATTRIBUTE,
	re_attrlist,	T_ATTRLIST,
	re_recnum,	T_ATTRIBUTE,
	re_seeknum,	T_ATTRIBUTE,
	re_attr_mod,	T_ATTRIBUTE,
	re_integer,	T_NUMBER,
	re_fraction,	T_NUMBER,
	re_decimal,	T_NUMBER,
	re_hexstring,	T_HEXSTRING,
};

#define LASTTYPE &typelist[ sizeof( typelist ) / sizeof( struct types ) ]

 
static char **arglist;		/* arguments passed in to parse */
static int maxarg;		/* argument count */
static int argindex;		/* index of current argument */
static char *curarg;		/* current argument */

static struct qnode *nodelist;	/* passed in list of relations */
static int nodecnt;		/* number of relations in nodelist */

/*
 * Lexical analyzer.
 */
int
yylex( )
{
	extern double atof();
#ifndef	LIBGEN_H
#ifndef	__STDC__
	extern char *regex();
#endif
#endif
	struct keyword *keyptr;	/* ptr for going through keywords */
	struct types *typeptr;	/* ptr for going through typelist */
	struct keycompare *cmpptr;/* ptr for going through compare list */
	int token;		/* token type from typelist */
	register char *ptr, *next;
	register unsigned int i;
	struct attrref *attrlist;

	/*
	 * Get the next argument.
	 */
	if ( ++argindex >= maxarg )
		return( T_ENDTOKEN );

	curarg = arglist[argindex];

	/*
	 * Check first if the next arg should be a string.
	 * If so, don't even bother looking at anything.
	 */
	if ( force_str ) {
		yylval.strval = curarg;
		return( T_STRING );
	}
	else if ( force_strlist ) {
		yylval.strval = curarg;
		return( T_STRLIST );
	}

	/*
	 * Now go through the keywords looking for the appropriate
	 * string.
	 */
	for( keyptr = keywords; keyptr < LASTWORD; keyptr++ )
	{
		if ( strcmp( keyptr->string, curarg ) == 0 )
			return( keyptr->token );
	}

	/*
	 * Check if this is a comparison operator.
	 *
	 * First check if this is potentially a field-to-field
	 * comparison.
	 */
	yylval.cmp.optype = 0;
	ptr = curarg;
	if ( *ptr == 'f' ) {
		yylval.cmp.opflags = ISATTR1|ISATTR2;
		++ptr;
		while( *ptr ) {
			switch( *ptr ) {
			case 'o':		/* outer join */
				yylval.cmp.opflags |= OPOUTERJOIN;
				ptr++;
				continue;
			case 'O':		/* directional outer join */
				yylval.cmp.opflags |= (OPOUTERJOIN|OPOJDIRECT);
				ptr++;
				continue;
			case '-':		/* set diff / anti-join */
				yylval.cmp.optype |= OPANTIJOIN;
				ptr++;
				continue;
			}

			break;	/* something unrecognized, yet */
		}
	}
	else
		yylval.cmp.opflags = ISATTR1|HASSELECT;

	/*
	 * Now check on the type of comparison.  This is a
	 * bit kludgy because of the various combinations of
	 * letters that could be present.
	 */
	yylval.cmp.cmptype = QCMP_NUMBER;
	token = T_NUMOPER;
	switch( *ptr ) {
	case '>':	/* string */
	case '<':
	case '=':
		yylval.cmp.cmptype = nocase ? QCMP_CASELESS : QCMP_STRING;
		token = T_STROPER;
		break;
	case 'c':
		ptr++;
		if ( *ptr == '~' )
		{
			yylval.cmp.opflags |= OPNOCASE;
			yylval.cmp.cmptype = QCMP_REGEXPR;
			token = T_REOPER;
		}
		else
		{
			yylval.cmp.cmptype = QCMP_CASELESS;
			token = T_STROPER;
		}
		break;
	case '!':	/* regular expression or string */
		if ( ptr[1] == '=' ) {
			yylval.cmp.cmptype = nocase ? QCMP_CASELESS : QCMP_STRING;
			token = T_STROPER;
		}
		else if ( ptr[1] == '~' ) {
			yylval.cmp.cmptype = QCMP_REGEXPR;
			token = T_REOPER;
		}
		break;
	case 'i':	/* possible "in" -- set comparison */
		yylval.cmp.cmptype = nocase ? QCMP_CASELESS : QCMP_STRING;
		token = T_INSET;
		break;
	case 'r':	/* regular expression */
		ptr++;
		if ( *ptr == 'c' )	/* caseless regular expression */
		{
			yylval.cmp.opflags |= OPNOCASE;
			ptr++;
		}
		/* we want to fall thru here */
	case '~':	/* regular expression */
		yylval.cmp.cmptype = QCMP_REGEXPR;
		token = T_REOPER;
		break;
	case 'n':	/* possibly numeric comparison */
		if ( ptr[1] < 'a' || ptr[1] > 'z' || strlen( ptr ) > 2 )
		{
			ptr++;
			yylval.cmp.cmptype = QCMP_NUMBER;
			token = T_NUMOPER;
		}
		break;
	case 'l':	/* possibly string comparison */
		if ( ptr[1] < 'a' || ptr[1] > 'z' || strlen( ptr ) > 2 )
		{
			ptr++;
			if ( *ptr == 'c' )
			{
				yylval.cmp.cmptype = QCMP_CASELESS;
				ptr++;
			}
			else
				yylval.cmp.cmptype = nocase ? QCMP_CASELESS : QCMP_STRING;
			token = T_STROPER;
		}
		break;
	case 'd':	/* date with optional time comparison */
		ptr++;
		yylval.cmp.cmptype = QCMP_DATE;
		token = T_STROPER;
		break;
	case 'D':	/* date without optional time comparison */
		ptr++;
		yylval.cmp.cmptype = QCMP_DATEONLY;
		token = T_STROPER;
		break;
	}

	/*
	 * If this is a field comparison, then the token type
	 * is always T_ATTROPER.
	 */
	if ( yylval.cmp.opflags & ISATTR2 )
		token = T_ATTROPER;

	/*
	 * Now look up the comparison operator.
	 * If it matches, we return.
	 */
	for( cmpptr = cmplist; cmpptr < LASTCMP; cmpptr++ )
	{
		if ( strcmp( cmpptr->string, ptr ) == 0 )
		{
			yylval.cmp.optype |= cmpptr->optype;
			/*
			 * If this is a set comparison operator,
			 * then always return T_INSET.
			 */
			return( cmpptr->optype == OPIN ? T_INSET : token );
		}
	}

	/*
	 * It's not a keyword, so try and figure out what kind of object
	 * it is by matching regular expressions.
	 */
	token = T_STRING;		/* default token type */
	for( typeptr = typelist; typeptr < LASTTYPE; typeptr++ ) {
		if ( regex( typeptr->regexpr, curarg, NULL ) ) {
			token = typeptr->token;
			break;
		}
	}
	switch( token ) {
	case T_NUMBER:
		yylval.numval = atof( curarg );	/* convert string */
		return( T_NUMBER );
	case T_HEXSTRING:
		if ( ! in_num_cmp ) {
			yylval.strval = curarg;
			return( T_STRING );
		} else {
#ifdef	__STDC__
			/*
			 * Use unsigned conversion if available to
			 * avoid problem with maximum unsigned value
			 * since most (all) non-base 10 values are
			 * initially generated from an unsigned int.
			 */
			yylval.numval = (int) strtoul( curarg, (char **)NULL, 16 );
#else
			yylval.numval = (int) strtol( curarg, (char **)NULL, 16 );
#endif
			return( T_NUMBER );
		}

	case T_ATTRLIST:
		/*
		 * Look up each attribute in the database descriptions
		 */
		ptr = curarg;
		i = 0;
		while( ptr )
		{
			if ( (ptr = strchr( ptr, ',' )) != NULL )
				ptr++;
			i++;
		}

		attrlist = (struct attrref *)calloc( i, sizeof( struct attrref ));
		if ( attrlist == NULL ) {
			set_uerror( UE_NOMEM );
			return( T_UNKNOWN );
		}

		ptr = curarg;
		i = 0;
		while( 1 ){
			next = strchr( ptr, ',' );
			if ( next )
				*next = '\0';
			if ( ! lookupattr( ptr, nodelist, nodecnt, &attrlist[i])){
				if ( next )
					*next = ',';
				free( attrlist );
				yylval.strval = curarg;
				return( T_STRING );
			}
			i++;
			if ( next ) {
				*next++ = ',';
				ptr = next;
			}
			else
				break;
		}

		yylval.alist.list = attrlist;
		yylval.alist.cnt = i;
		return( T_ATTRLIST );

	case T_ATTRIBUTE:
		/*
		 * Check if the attribute has a ":modifier" appended.
		 */
		if ( typeptr->regexpr == re_attr_mod ) {
			if (ptr = strrchr( curarg, ':'))
				*ptr = '\0';
		} else {
			ptr = NULL;
		}
		/*
		 * Look up attribute in the database descriptions.
		 */
		if ( lookupattr( curarg, nodelist, nodecnt, &yylval.attr ) ) {

			if ( ptr == NULL ) {
				/*
				 * If this is not a special attribute
				 * then check if the next argument is
				 * an attribute modifier.
				 */
				if (( yylval.attr.attr >= 0 ) &&
				    ( maxarg > argindex + 1 ) &&
				    ( arglist[argindex+1][0] == ':' )) {
					/*
					 * Advance the current argument
					 * to be the attribute modifier.
					 */
					curarg = arglist[++argindex];
					/*
					 * Set pointer to the first character
					 * of the attribute modifier.
					 */
					ptr = curarg + 1;
				}
			} else
				*ptr++ = ':';

			/* Convert numeric attribute modifier if specified. */
			if ( ptr ) {
				/* Make sure this is not a special attribute */
				if ( yylval.attr.attr >= 0 ) {
					switch ( *ptr ) {
					case 'b':	/* binary */
						if (( ptr[1] == '\0' ) ||
						    ( ptr[1] == 'i' )) {
							yylval.attr.modifier = 2;
							ptr = NULL;
						}
						break;
					case 'h':	/* hex */
						yylval.attr.modifier = 16;
						ptr = NULL;
						break;
					case 'l':	/* length */
						if (( ptr[1] == 'e' ) &&
						    ( ptr[2] == 'n' )) {
							yylval.attr.modifier = 1;
							ptr = NULL;
						}
						break;
					case 'n':	/* numeric */
						if (( ptr[1] == '\0' ) ||
						    ( ptr[1] == 'u' )) {
							yylval.attr.modifier = '\0';
							ptr = NULL;
						}
						break;
					case 'o':	/* octal */
						yylval.attr.modifier = 8;
						ptr = NULL;
						break;
					default:
						break;
					}
				}
				/*
				 * If pointer is not NULL then report error.
				 */
				if ( ptr ) {
					/* reset pointer to include the ':' */
					--ptr;
					/*
					 * Print (set) error message.
					 */
					set_uerror( UE_ACMPMOD );
					return( T_UNKNOWN );
				}
			} else {
				/*
				 * Make sure attribute modifier is not set.
				 */
				yylval.attr.modifier = '\0';
			}

			return( T_ATTRIBUTE );

		} else {
			if ( ptr )
				*ptr = ':';
		}

		/*
		 * Attribute name was unrecognized. Fall through and
		 * return a string.  Let the parser report an error, if
		 * really appropriate.
		 */
	case T_STRING:
	default:
		yylval.strval = curarg;
		return( T_STRING );
	}
}

#define CONTEXTSIZE	3

/*VARARGS1*/
yyerror( str, a, b, c, d, e, f, g, h, j, k, m )
char *str;
long a, b, c, d, e, f, g, h, j, k, m;
{
	int i, max;

	if ( str )
		prmsg( MSG_ERROR, str, a, b, c, d, e, f, g, h, j, k, m );

	if ( argindex < maxarg ) {
		prmsg( MSG_ERRASIS, "\tContext is:\n\n\t" );
		i = argindex - CONTEXTSIZE;
		if ( i < 0 )
			i = 0;
		max = argindex + 1 + CONTEXTSIZE;
		if ( max > maxarg )
			max = maxarg;
		for( ; i < max; i++ )
			prmsg( MSG_ERRASIS, i == argindex ? ">>>%s<<< " : "%s ",
				arglist[i] );
		prmsg( MSG_CONTINUE, "\n" );
	}
	else
		prmsg( MSG_ERROR, "premature end of where-clause" );
}

struct queryexpr *
fparsewhere( argc, argv, rellist, relcnt, flags )
int argc;
char *argv[];
struct qnode *rellist;
int relcnt;
unsigned short flags;
{
	arglist = argv;
	maxarg = argc;
	argindex = -1;
	curarg = NULL;

	force_str = FALSE;
	force_strlist = FALSE;
	querytree = NULL;
	nodelist = rellist;
	nodecnt = relcnt;
	nocase = (flags & Q_NOCASECMP) != 0;
	queryflags = flags;

	return( yyparse( ) == 0 ? querytree : NULL );
}

struct queryexpr *
parsewhere( argc, argv, rellist, relcnt )
int argc;
char *argv[];
struct qnode *rellist;
int relcnt;
{
	return( fparsewhere( argc, argv, rellist, relcnt, 0 ) );
}

static
has_setop( qptr )
register struct queryexpr *qptr;
{
	while( ISBOOL( qptr->optype ) ) {
		if ( ! has_setop( qptr->elem1.expr ) )
			return( FALSE );
		qptr = qptr->elem2.expr;
	}

	return( ISSETCMP( qptr->optype ) );
}

static
samerefnode( refptr, cnt )
register struct attrref *refptr;
register unsigned int cnt;
{
	while( cnt-- > 1 ) {
		if ( refptr->rel != refptr[1].rel )
			return( FALSE );
		refptr++;
	}

	return( TRUE );
}

static
cmprexplist( strlist, caseless )
register char **strlist;
int caseless;
{
	register char *ptr;
	register int rc = TRUE;

	while( *strlist )
	{
		ptr = nc_regcmp( nocase || caseless, *strlist, NULL );
		if ( ptr == NULL )
		{
			yyerror( "syntax error in regular expression '%s'",
				*strlist );
			rc = FALSE;
			*strlist++ = "";
		}
		else
			*strlist++ = ptr;
	}

	return( rc );
}

#define MAXSTRINGS	512
#define STARTGROUP	'('
#define ENDGROUP	')'
#define GRPDELIMNAME	"parenthesis"

static char **
parse_strlist( origstr, attrcnt )
char *origstr;
unsigned int attrcnt;
{
	register char *str;
	register int valcnt, groupcnt, morevalue, startvalue;
	char *allocstr;
	char **tmplist;
	char *strlist[MAXSTRINGS];

	str = malloc( (unsigned)(strlen( origstr ) + 1) );
	if ( str == NULL ) {
		set_uerror( UE_NOMEM );
		yyerror( nomemory );
		return( NULL );
	}
	strcpy( str, origstr );
	allocstr = str;		/* save value for free() on parse failure */

	valcnt = 0;		/* how many strings have been seen */
	groupcnt = 0;		/* how many groupings are pending */

	/*
	 * This main loop gets one comparison value.
	 */
	startvalue = TRUE;
	while( *str )
	{
		while( *str == STARTGROUP )
		{
			/*
			 * We're at the start of a grouping.  We finish
			 * off any current grouping with empty strings,
			 * skip the STARTGROUP, and increment the group
			 * count.
			 */
			while( valcnt < MAXSTRINGS && valcnt % attrcnt != 0 )
				strlist[valcnt++] = "";
			++str;
			++groupcnt;
		}

		if ( valcnt >= MAXSTRINGS )
		{
			yyerror( "too many set comparison values (max is %d) -- try simplifying",
				MAXSTRINGS );
			free( allocstr );
			return( NULL );
		}
		strlist[valcnt++] = str;

		morevalue = TRUE;
		startvalue = FALSE;
		while( *str && morevalue )
		{
			switch( *str ) {
			case STARTGROUP:
				/*
				 * We've found a grouping.  (We should
				 * have seen a comma, but we're ignoring
				 * that error.)  We finish off the current
				 * value and any current grouping with
				 * empty strings.  Then we act as if we've
				 * just seen a comma.
				 */
				groupcnt++;
				while( valcnt < MAXSTRINGS &&
						valcnt % attrcnt != 0 )
					strlist[valcnt++] = "";
				/* we want to fall through here */
			case ',':
				*str++ = '\0';	/* end of current value */
				morevalue = FALSE;
				startvalue = TRUE;
				break;
			case ENDGROUP:
				if ( groupcnt ) {
					*str++ = '\0';
					if ( *str == ',' ) /* skip comma */
						str++;
					--groupcnt;
					morevalue = FALSE;
					/*
					 * Fill the rest of the current
					 * grouping with empty strings.
					 * If we're on the first item of a
					 * grouping the whole grouping gets
					 * empty strings.
					 */
					while( valcnt < MAXSTRINGS &&
						    (valcnt % attrcnt != 0 ||
						    valcnt == 0 ) )
						strlist[valcnt++] = "";
				}
				else {
					yyerror( "extra closing %s in set value list '%s'",
						GRPDELIMNAME, origstr );
					free( allocstr );
					return( NULL );
				}
				break;
			case '\\': 	/* Escape the next character. */
				if ( str[1] ) {
					strcpy( str, &str[1] );
					++str;
				}
				break;
			default:
				++str;
			}
		}
	}

	if ( startvalue )	/* we got a ",", but no string for it */
	{
		if ( valcnt >= MAXSTRINGS )
		{
			yyerror( "too many set comparison values (max is %d) -- try simplifying",
				MAXSTRINGS );
			free( allocstr );
			return( NULL );
		}
		strlist[valcnt++] = "";
	}

	if ( groupcnt )
	{
		yyerror( "missing closing %s in set value list '%s'",
			GRPDELIMNAME, origstr );
		free( allocstr );
		return( NULL );
	}

	/*
	 * Copy the value list to allocated space.  We allocate
	 * enough extra space to hold any empty strings still
	 * remaining in the current grouping.
	 */
	tmplist = (char **)malloc( (unsigned)((valcnt + attrcnt + 1) * sizeof( char * )) );
	if ( tmplist == NULL ) {
		set_uerror( UE_NOMEM );
		yyerror( nomemory );
		free( allocstr );
		return( NULL );
	}
	memcpy( (char *)tmplist, (char *)strlist, valcnt * sizeof( char * ) );
	tmplist[valcnt] = NULL;

	/*
	 * Fill the rest of the current grouping with empty strings.
	 * If we're on the first item of a grouping the whole grouping
	 * gets empty strings.
	 */
	while( valcnt % attrcnt != 0 || valcnt == 0 )
		tmplist[valcnt++] = "";

	return( tmplist );
}

static struct queryexpr *
expnd_oper( attlist, attcnt, strlist, strcnt, cmptype, opflags, bool_op )
struct attrref *attlist;
unsigned short attcnt;
char **strlist;
unsigned short strcnt;
QCMPTYPE cmptype;
unsigned short opflags;
unsigned short bool_op;
{
	struct queryexpr *elem1, *elem2, *qptr;

	qptr = newqexpr();
	if ( qptr == NULL )
	{
		set_uerror( UE_NOMEM );
		yyerror( nomemory );
		return( NULL );
	}

	if ( attcnt == 1 )
	{
		qptr->optype = OPEQ;
		qptr->opflags = (opflags & OPNOCASE) | HASSELECT | ISATTR1;
		qptr->cmptype = cmptype;
		qptr->elem1.attr = *attlist;
		qptr->elem2.strval = *strlist;

		return( qptr );
	}

	if ( strcnt == attcnt )
	{
		bool_op = OPAND;

		elem1 = expnd_oper( attlist, 1, strlist, 1,
				cmptype, opflags, OPAND );
		elem2 = expnd_oper( &attlist[1], attcnt - 1,
				&strlist[1], attcnt - 1,
				cmptype, opflags, OPAND );
	}
	else
	{
		elem1 = expnd_oper( attlist, attcnt, strlist, attcnt,
				cmptype, opflags, bool_op );
		elem2 = expnd_oper( attlist, attcnt,
				&strlist[ attcnt ], strcnt - attcnt,
				cmptype, opflags, bool_op );
	}
	if ( elem1 == NULL || elem2 == NULL )
	{
		if ( elem1 )
			freeqexpr( elem1 );
		if ( elem2 )
			freeqexpr( elem2 );
		return( NULL );
	}

	qptr->optype = bool_op;
	qptr->opflags = HASSELECT;
	qptr->cmptype = QCMP_STRING;
	qptr->elem1.expr = elem1;
	qptr->elem2.expr = elem2;

	return( qptr );
}

static int
set_rel( attlist, attcnt, start )
struct attrref *attlist;
unsigned short attcnt;
unsigned short start;
{
	int i;

	for( ; start < attcnt; start++ )
	{
		for( i = start - 1; i >= 0; i-- )
		{
			if ( attlist[i].rel == attlist[start].rel )
				break;
		}
		if ( i < 0 )
			return( start );
	}

	return( -1 );
}

static struct queryexpr *
setop_onerel( attlist, attcnt, start, strlist, strcnt, cmptype, opflags )
struct attrref *attlist;
unsigned short attcnt;
unsigned short start;
char **strlist;
unsigned short strcnt;
QCMPTYPE cmptype;
unsigned short opflags;
{
	struct queryexpr *elem1, *elem2, *qptr;
	int newattcnt, i, j, attnum;
	struct attrref *newrefs;
	char **newstrs;

	newattcnt = 1;
	for( i = start + 1; i < (int) attcnt; i++ )
	{
		if ( attlist[i].rel == attlist[start].rel )
			newattcnt++;
	}

	newrefs = (struct attrref *)malloc( newattcnt * sizeof( struct attrref ) );
	newstrs = (char **)malloc( (newattcnt * (strcnt / attcnt) + 1 ) * sizeof ( char **)  );
	elem1 = newqexpr();
	if ( elem1 == NULL || newrefs == NULL || newstrs == NULL )
	{
		if ( newrefs )
			free( newrefs );
		if ( newstrs )
			free( newstrs );
		if ( elem1 )
			freeqexpr( elem1 );

		set_uerror( UE_NOMEM );
		yyerror( nomemory );
		return( NULL );
	}

	attnum = 0;
	for( i = start; i < (int)attcnt; i++ )
	{
		if ( attlist[i].rel == attlist[start].rel )
		{
			newrefs[attnum].rel = attlist[i].rel;
			newrefs[attnum].attr = attlist[i].attr;

			for( j = 0; j < (int)strcnt / (int)attcnt; j++ )
				newstrs[ attnum + j * newattcnt ] = strlist[ i + j * attcnt ];
		}
	}
	newstrs[ newattcnt * (strcnt / attcnt) ] = NULL;

	elem1->optype = OPIN;
	elem1->opflags = opflags;
	elem1->cmptype = cmptype;
	elem1->elem1.alist.list = newrefs;
	elem1->elem1.alist.cnt = newattcnt;
	elem1->elem2.strlist = newstrs;

	attnum = set_rel( attlist, attcnt, start + 1 );
	if ( attnum < 0 )
		return( elem1 );

	elem2 = setop_onerel( attlist, attcnt, attnum,
			strlist, strcnt, cmptype, opflags );
	if ( elem2 == NULL )
	{
		freeqexpr( elem1 );
		return( NULL );
	}

	qptr = newqexpr();
	if ( qptr == NULL )
	{
		freeqexpr( elem1 );
		freeqexpr( elem2 );

		set_uerror( UE_NOMEM );
		yyerror( nomemory );
		return( NULL );
	}

	qptr->optype = OPAND;
	qptr->opflags = HASSELECT;
	qptr->cmptype = QCMP_STRING;
	qptr->elem1.expr = elem1;
	qptr->elem2.expr = elem2;

	return( qptr );
}

static struct queryexpr *
expnd_setop( attlist, attcnt, strlist, cmptype, opflags )
struct attrref *attlist;
unsigned short attcnt;
char **strlist;
QCMPTYPE cmptype;
unsigned short opflags;
{
	register char **listptr;
	struct queryexpr *elem1, *elem2, *qptr;

	/* Get a pointer to the last string element */
	for( listptr = strlist; *listptr; listptr++ )
		;

	elem1 = setop_onerel( attlist, attcnt, 0, strlist,
			listptr - strlist, cmptype, opflags );
	elem2 = expnd_oper( attlist, attcnt, strlist, listptr - strlist,
			cmptype, opflags, OPOR );
	qptr = newqexpr();

	free( attlist );
	free( strlist );

	if ( qptr == NULL || elem1 == NULL || elem2 == NULL )
	{
		if ( elem1 )
			freeqexpr( elem1 );
		if ( elem2 )
			freeqexpr( elem2 );

		set_uerror( UE_NOMEM );
		yyerror( nomemory );
		return( NULL );
	}

	qptr->optype = OPAND;
	qptr->opflags = HASSELECT;
	qptr->cmptype = QCMP_STRING;
	qptr->elem1.expr = elem1;
	qptr->elem2.expr = elem2;

	return( qptr );
}
