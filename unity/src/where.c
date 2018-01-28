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
#include <signal.h>
#include <ctype.h>
#ifdef NEED_TOUPPER
#define _tolower( ch )	(isupper( (ch) ) ? tolower( (ch) ) : (ch))
#define _toupper( ch )	(isupper( (ch) ) ? (ch) : toupper( (ch) ))
#endif /* NEED_TOUPPER */

#include "db.h"

/* include the stuff from regexp.h */
extern char *compile();
extern int step();
extern int circf;

extern char *strchr();

/* Tokens from scanner */
#define T_LPAREN 1	/* Left paren */
#define T_RPAREN 2	/* Right paren */
#define T_NOT 3		/* Not (!) */
#define T_STRING 4	/* String */
#define T_NULL 5	/* Null: end of string */
#define OPNONE		0
#define OPAND		1
#define OPOR		2

struct relation {
	short	field1;		/* field number */
	short	ibase1;		/* field1 input base */
	short	field2;		/* ERR if field2 has value, */
				/* field number if comparison against field */
	short	ibase2;		/* field2 input base */
	short	rel;		/* Relational value */
	short	notflag;	/* NOT expr if true */
	char	*value;
	int	circf;		/* circumflex or not for RE */
	struct relation	*child;	/* down link */
	struct relation	*sib;	/* sibling link */
	int	op;			/* And/or operator */
};


static	struct relation *reltab, *relfree;
extern	char *malloc(),*strcpy();
static	int recordnr;
static	struct fmt *xx;
static	int nattr1;
static	char Dtable1[MAXPATH+4];	/* allow for "/./" or "././" prefix */
static	char **pp, prog[MAXPATH];
static	orlevel,andlevel,attrlevel,indx;


getcond(p,x,n,pr,dfile)
char	**p,*pr;
struct	fmt x[];
int	n;
char	*dfile;
{
	/* make variables known globally (i.e. at any level of recursion) */
	xx = x;
	nattr1 = n;
	strcpy(prog,pr);
	strcpy(Dtable1,dfile);

	/* root of tree */
	if(reltab != NULL || relfree != NULL)
		wfree();	/* free old where clause tree */
	reltab = relfree = (struct relation *)malloc(sizeof(struct relation));
	if(relfree == NULL) {
		error(E_GENERAL,"Too many conditions in where clause\n");
		kill(0,SIGINT);
	}
	reltab->op = OPNONE;
	reltab->field1 = -1;
	reltab->ibase1 = 0;
	reltab->ibase2 = 0;
	reltab->notflag = 0;
	reltab->child = (struct relation *)0;
	reltab->sib = (struct relation *)0;
	reltab->value = (char *)0;

	orlevel = andlevel = 32000;	/* deeplest level for 'and' and 'or' */
	attrlevel = -1;			/* level of first attribute */
	if (*p) {			/* where clause present */
		indx = 1;		/* indexing on first attr ok */
		if(strcmp(p[0],"where") != 0 ) {
			error(E_GENERAL,"%s: Expecting 'where' keyword.\n",
				prog);
			kill(0,SIGINT);
		}
		pp = p;
		treebuild(T_NULL,0);
		if(indx == 0 || orlevel <= attrlevel)
			indx = 0;
	}
	else
		indx = 0;
	return(indx);
}

static int
treebuild(endtoken,level)
int endtoken,level;
{
	register struct relation *rp;
	int field,token,i,j;
	char *ptr;

	for (;;) {
		rp = relfree;
		relfree = (struct relation *)malloc(sizeof(struct relation));
		if(relfree == NULL) {
			error(E_GENERAL,
				"Too many conditions in where clause\n");
			kill(0,SIGINT);
		}

		rp->field1 = -1;
		rp->field2 = ERR;
		rp->ibase1 = 0;
		rp->ibase2 = 0;
		rp->notflag = 0;
		rp->child = NULL;
		rp->sib = NULL;
		rp->value = (char *)0;
		rp->op = OPNONE;

		while ((token = scanner(rp)) == T_NOT)
			;
		switch(token) {
		case T_LPAREN:
			rp->child = relfree;
			treebuild(T_RPAREN,level+1);
			break;
		case T_STRING:
			ptr = NULL;
			if(strcmp("rec#",pp[0]) == 0) {
				/* record number comparison */
				rp->field1 = nattr1;

				if(attrlevel < 0) {
					attrlevel = level;
					indx = 0;
				}
			}
			else
			{
				ptr = strchr(pp[0],':');
				if (ptr != NULL)
					*ptr = '\0';
				i=setnum(xx,pp[0],nattr1);
				if (ptr)
					*ptr = ':';
				if (i == ERR) {
					error(E_GENERAL,
					"%s: Bad where clause, illegal attribute %s\n",
					prog,pp[0]);
					kill(0,SIGINT);
				} else {
					rp->field1 = i;

					if ((ptr == NULL) &&
					    (pp[1] != NULL) &&
					    (pp[1][0] == ':')) {
						++pp;
						ptr = pp[0];
					}
					if(attrlevel < 0) {
						attrlevel = level;
						if(pp[1] !=NULL && pp[1][0] == 'f')	/* field comp */
							indx = 0;
					}
				}
			}
			token = scanner(rp);
			if(token != T_STRING || (rp->rel=getop(pp[0])) == ERR) {
				error(E_GENERAL,
				"%s: Bad where clause, unknown operator %s\n",
				prog, (pp[0] != NULL) ? pp[0] : "(null)" );
				kill(0,SIGINT);
			}
			if (ptr) {
				++ptr;	/* look at first character past the ':' */
				switch (*ptr) {
				case 'b':	/* binary */
					if ((rp->rel >= LT) &&
					    (rp->rel <= GT)) {
						rp->ibase1 = 2;
						ptr = NULL;
					}
					break;
				case 'o':	/* octal */
					if ((rp->rel >= LT) &&
					    (rp->rel <= GT)) {
						rp->ibase1 = 8;
						ptr = NULL;
					}
					break;
				case 'h':	/* hexadecimal */
					if ((rp->rel >= LT) &&
					    (rp->rel <= GT)) {
						rp->ibase1 = 16;
						ptr = NULL;
					}
					break;
				case 'n':	/* numeric */
					if ((rp->rel >= LT) &&
					    (rp->rel <= GT)) {
						rp->ibase1 = 0;
						ptr = NULL;
					}
					break;
				case 'l':	/* length */
					if ((ptr[1] == 'e') &&
					    (ptr[2] == 'n') &&
					    (rp->rel >= LT) &&
					    (rp->rel <= GT)) {
						rp->ibase1 = 1;
						ptr = NULL;
					}
					break;
				case 's':	/* string */
				case 'a':	/* ascii */
					/*
					 * Since :modifier is advertised
					 * being for numeric attributes,
					 * just make sure that a numeric
					 * operation was not requested.
					 */
					if ((rp->rel < LT) ||
					    (rp->rel > GT)) {
						ptr = NULL;
					}
					break;
				default:
					error(E_GENERAL,
						"%s: Bad where clause, unknown attribute modifier [%s]\n",
						prog, ptr);
					kill(0,SIGINT);
				}
				if (ptr) {
					error(E_GENERAL,
						"%s: Bad where clause, attribute [%s] modifier [%s] invalid with operator [%s]\n",
						prog, xx[rp->field1].aname, ptr, pp[0]);
					kill(0,SIGINT);
				}
			}
			/*
			 * save pointer to current field operator text
			 * for caseless regular expression check below
			 */
			ptr = pp[0];
			if(*ptr == 'f')	/* field comparison operator */
				field = 1;
			else
				field = 0;
			token = scanner(rp);
			if (token != T_STRING) {
				error(E_GENERAL,
				"%s: expected relation or expression\n",prog);
				kill(0,SIGINT);
			}
			if(rp->rel == REQ || rp->rel == RNE)
			{
				char	*orig_dest;

				/* check if request for caseless regular expression */
				if (strncmp(ptr,"rc",2) == 0)
				{
					register char *src;
					register char *dest;
					register int  in_group;

					src = pp[0];
					orig_dest = malloc( 4 * strlen(src) + 1 );
					if (orig_dest == NULL) {
						error(E_GENERAL,
							"%s: Cannot allocate space for comparison value %s\n",
							prog,pp[0]);
						kill(0,SIGINT);
					}

					in_group = 0;	/* not inside [] grouping */
					dest = orig_dest;
					while( *src )
					{
						if ( isalpha( *src ) )
						{
							if ( in_group == 0 )
								*dest++ = '[';
							*dest++ = *src;
							*dest++ = islower( *src ) ? _toupper( *src ) :
									_tolower( *src );
							if ( in_group == 0 )
								*dest++ = ']';
						}
						else
						{
							*dest++ = *src;
							switch( *src )
							{
							case '\\':
								if ( src[1] )
									*dest++ = *++src;
								break;
							case '[':
								if ( in_group == 0 )
									in_group = 1;
								break;
							case ']':
								/* must not be first element of group */
								if ( in_group != 2 )
									in_group = 0;
								break;
							}
						}
						if ( in_group != 0 )
							in_group++;
						++src;
					}
					*dest = '\0';
				}
				else
				{	/* not caseless regular expression */
					orig_dest = pp[0];
				}

				/* approx count of number of [] in expression */
				for(i=j=0;orig_dest[i] != '\0'; i++)
					if(orig_dest[i] == '[')
						j++;
				/* allocate 16 bytes per [] plus
				   three times length of expression */
				j = j * 16 + 3 * i + 20;
				if((rp->value = malloc((unsigned)j)) == NULL) {
					error(E_GENERAL,
						"%s: Cannot allocate space for comparison value %s\n",
						prog,pp[0]);
					kill(0,SIGINT);
				}
				compile(orig_dest, rp->value, &rp->value[j-1], '\0');
				rp->circf = circf;

				/* free memory from caseless request if appropriate */
				if (orig_dest != pp[0])
					free(orig_dest);
			}
			else if(field) {
				char *op_ptr = ptr;	/* save operator text in case of error */
				ptr = strchr(pp[0],':');
				if (ptr != NULL)
					*ptr = '\0';
				i=setnum(xx,pp[0],nattr1);
				if (ptr)
					*ptr = ':';
				if (i == ERR) {
					if (strcmp(pp[0],"rec#") == 0) {
						rp->field2 = nattr1;
					} else {
						error(E_GENERAL,
						"%s: Bad where clause, illegal attribute %s\n",
						prog,pp[0]);
						kill(0,SIGINT);
					}
				} else {
					rp->field2 = i;
					rp->value = NULL;

					if ((ptr == NULL) &&
					    (pp[1] != NULL) &&
					    (pp[1][0] == ':')) {
						++pp;
						ptr = pp[0];
					}
					if (ptr) {
						/* look at first character past the ':' */
						++ptr;
						switch (*ptr) {
						case 'b':	/* binary */
							if ((rp->rel >= LT) &&
							    (rp->rel <= GT)) {
								rp->ibase2 = 2;
								ptr = NULL;
							}
							break;
						case 'o':	/* octal */
							if ((rp->rel >= LT) &&
							    (rp->rel <= GT)) {
								rp->ibase2 = 8;
								ptr = NULL;
							}
							break;
						case 'h':	/* hexadecimal */
							if ((rp->rel >= LT) &&
							    (rp->rel <= GT)) {
								rp->ibase2 = 16;
								ptr = NULL;
							}
							break;
						case 'n':	/* numeric */
							if ((rp->rel >= LT) &&
							    (rp->rel <= GT)) {
								rp->ibase2 = 0;
								ptr = NULL;
							}
							break;
						case 's':	/* string */
						case 'a':	/* ascii */
							/*
							 * Since :modifier is advertised
							 * being for numeric attributes,
							 * just make sure that a numeric
							 * operation was not requested.
							 */
							if ((rp->rel < LT) ||
							    (rp->rel > GT)) {
								ptr = NULL;
							}
							break;
						default:
							error(E_GENERAL,
								"%s: Bad where clause, unknown attribute modifier [%s]\n",
								prog, ptr);
							kill(0,SIGINT);
						}
						if (ptr) {
							error(E_GENERAL,
								"%s: Bad where clause, attribute [%s] modifier [%s] invalid with operator [%s]\n",
								prog, xx[rp->field1].aname, ptr, op_ptr);
							kill(0,SIGINT);
						}
					}
				}
			}
			else {
				j = strlen(pp[0]);

				/* set i to length of buffer to allocate */
				if(rp->field1 != nattr1 &&
					xx[rp->field1].flag == WN) {
					/* fixed length field */
					i = xx[rp->field1].flen;
					if(i < j)
						i = j;
				}
				else
					i = j;
				if((rp->value = malloc((unsigned)(i+1))) == NULL) {
					error(E_GENERAL,
			"%s: Cannot allocate space for comparison value %s\n",
						prog,pp[0]);
					kill(0,SIGINT);
				}
				strcpy(rp->value, pp[0]);
				if(rp->field1 != nattr1 &&
					xx[rp->field1].flag == WN &&
					j < i) {	/* pad with blanks */
					for(;j < i; j++)
						rp->value[j] = ' ';
					rp->value[j] = '\0';
				}
				/* if this is a numeric comparison then check for hex input value */
				if ((rp->rel >= LT) && (rp->rel <= GT) &&
				    (rp->value[0] == '0') &&
				   ((rp->value[1] == 'x') || (rp->value[1] == 'X'))) {
					rp->ibase2 = 16;
				}
			}
			break;
		default:
			error(E_GENERAL,
				"%s: Expected relation or expression\n",prog);
			kill(0,SIGINT);
		}
		token = scanner(rp);
		if (token == endtoken)
			return;
		if(token == T_STRING && strcmp("and",pp[0]) == 0) {
			rp->op = OPAND;
			if(level < andlevel)
				andlevel = level;
		}
		else if(token == T_STRING && strcmp("or",pp[0]) == 0) {
			rp->op = OPOR;
			if(level < orlevel)
				orlevel = level;
		}
		else {
			error(E_GENERAL,
		"%s: Bad where clause, unknown relational operator %s\n",
			prog,pp[0]);
			kill(0,SIGINT);
		}
		rp->sib = relfree;
	}
}

static int
scanner(rp)
register struct relation *rp;
{
	int token;

	pp++;					/* get next argument */
	if(*pp == 0)
		token = T_NULL;
	else if (strcmp("(",pp[0]) == 0 )
		token = T_LPAREN;
	else if(strcmp(")",pp[0]) == 0)
		token = T_RPAREN;
	else if(strcmp("!",pp[0]) ==0) {
		token = T_NOT;
		rp->notflag = !rp->notflag;
	}
	else				/* a relop, op, field, or value */
		token = T_STRING;
	return(token);
}

selct(x,r)
struct fmt x[];
{
	/* make variables known globally (i.e., at any level of recursion */
	xx = x;
	recordnr = r;

	/* call function to recursively evaluate tree */
	return(seval(reltab));
}

static int
seval(rp)
register struct relation *rp;
{
	register value;

	if (rp->field1 == -1 && rp->child == NULL)
		return(1);
	for (;;) {
		value = getval(rp);		/* get value of node */
		if (rp->sib == NULL)		/* no sibling - done */
			return(value);
		if (value && rp->op == OPOR)	/* OR operator - don't need */
			return(1);		/* to evaluate any further  */

		/* find next node to evaluate */
		if ((value && rp->op == OPAND) || (!value && rp->op == OPOR))
			rp = rp->sib;
		else {
			while (rp->op == OPAND)
				rp = rp->sib;
			if (rp->op == OPNONE || rp->sib == NULL)
				return(0);
			/* found an OR operator - get its sibling in order
			   to follow left associativity of AND and OR
			   operators and higher precedence of AND over
			   OR as in C language.
			*/
			rp = rp->sib;
		}
	}
}

static int
getval(rp)
register struct relation *rp;
{
	register int value;
	char rec[12];

	if (rp->child != NULL)
		value = seval(rp->child);
	else {
		/* set circumflex in case operator is regular expression
		   pattern match
		*/
		circf = rp->circf;
		/*
		 * check for record number comparison
		 */
		if ((rp->field1 == nattr1) || (rp->field2 == nattr1)) {
			sprintf(rec,"%d",recordnr);
			if (rp->field2 == ERR) {
				value = compb(rp->rel,rec,rp->value,0,rp->ibase2);
			} else if (rp->field2 == nattr1) {
				if (rp->field1 == nattr1)
					value = compb(rp->rel,rec,rec,0,0);
				else
					value = compb(rp->rel,xx[rp->field1].val,rec,rp->ibase1,0);
			} else {
				value = compb(rp->rel,rec,xx[rp->field2].val,0,rp->ibase2);
			}
		} else if (rp->field2 == ERR) {
				value = compb(rp->rel,xx[rp->field1].val,rp->value,rp->ibase1,rp->ibase2);
		} else {
			value = compb(rp->rel,xx[rp->field1].val,xx[rp->field2].val,rp->ibase1,rp->ibase2);
		}
	}
	if (rp->notflag)		/* negation of value */
		value = !value;
	return(value);
}

wfree()
{
	if(relfree != NULL)
		free(relfree);
	if(reltab != NULL && reltab != relfree)
		wfree1(reltab);	/* free tree - left data right traversal */
	reltab = relfree = NULL;
}

wfree1(rp)
struct relation *rp;
{
	struct relation *nrp;

	/* free one level of tree - node and all its siblings */
	if(rp == NULL)
		return;
	for(;;) {
		if(rp->child != NULL)
			wfree1(rp->child);	/* free lower levels */
		if(rp->value != NULL)
			free(rp->value);

		nrp = rp->sib;
		free(rp);

		if(nrp == NULL)		/* no sibling - done */
			return;
		rp = nrp;
	}
}
