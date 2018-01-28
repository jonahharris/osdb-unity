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
#include "db.h"

/* types from lex */
#define BOUNDARY	0
#define UNARY		1	/* unary + or - */
#define IDENT 		2	/* String - attribute number or constant */
#define LPAREN 		3	/* Left paren */
#define RPAREN 		4	/* Right paren */
#define ADDOP		5	/* + or - */
#define MULOP		6	/* *, /, or % */
#define	FUNC		7	/* function call on single attribute */

#define OPNONE		0	/* no operator */

/* subtypes */
#define MUL	16
#define DIV	17
#define ADD	18
#define SUB	19
#define UPLUS	20
#define UMINUS	21
#define ABS	22
#define MODULO	23
#define	BASE	24	/* start of interger input base conversion */
#define	BIN	26	/* binary	(BASE + 2)		   */
#define	OCT	32	/* octal	(BASE + 8)		   */
#define	HEX	40	/* hexidecimal	(BASE + 16)		   */
#define B36	60	/* base 36	(BASE + 36)		   */
#define LEN	61	/* string length of attribute value	   */

static short type;		/* token type */
static short subtype;		/* token subtype or field id number */
#ifdef INTCOMP
static int subvtype;		/* token constant value */
#else
static double subvtype;		/* token constant value */
#endif
static short fcttype;		/* function type if token is a function */
static short prevterm;		/* previous terminal type */

/* evaluation tree node structure */
struct tree {
#ifdef INTCOMP
	int	val1;		/* left const if no left child and -1 field1 */
	int	val2;		/* right const if no right child and -1 field2*/
#else
	double	val1;		/* left const if no left child and -1 field1 */
	double	val2;		/* right const if no right child and -1 field2*/
#endif
	struct	tree	*left;	/* left child or NULL */
	struct	tree	*right;	/* right child or NULL */
	short	field1;		/* left field number or -1 */
	short	field2;		/* right field number or -1 */
	short	op;		/* operator */
};

static	struct tree *tree;	/* final evaluation tree */

static	struct tree *ntree[MAXATT];	/* final evaluation tree per attribute */

extern	char *malloc(), *strcpy();

static	struct fmt *xx;		/* structure for description file and values */
static	int nattr1;		/* number of attributes for desc file */
static	char Dtable1[MAXPATH+4];/* description file name - allow for "/./" or "././" prefix */
static	char **pp;		/* array of char pointers to tokens */
static	char prog[MAXPATH];	/* main program name */

static	int p[8][8] = {	/* precedence table

	Values:
		0: error, 1: yield, 2: takes precedence, 3:equal

	Array row and col meanings:
		0 - boundary
		1 - unary op +-
		2 - id
		3 - (
		4 - )
		5 - addop
		6 - mulop
		7 - function

		e.g., p[3][2] is precedence of left paren followed by
		identifier - yields
		Constants are stored as identifiers  and functions
		(the only one currently allowed is abs() ) are
		reduced to identifiers.
	*/
	0, 1, 1, 1, 0, 1, 1, 1,
	2, 1, 1, 1, 2, 2, 2, 1,
	2, 0, 0, 0, 2, 2, 2, 0,
	2, 1, 1, 1, 3, 1, 1, 1,
	2, 0, 0, 0, 2, 2, 2, 0,
	2, 1, 1, 1, 2, 2, 1, 1,
	2, 1, 1, 1, 2, 2, 2, 1,
	2, 0, 0, 0, 2, 2, 2, 0 };

/* stack for building eval tree */
static struct {
#ifdef INTCOMP
	int svtype;		/* floating subtype for constant=>stype = -1 */
#else
	double svtype;		/* floating subtype for constant=>stype = -1 */
#endif
	struct tree *result;	/* pointer to evaluation sub-tree */
	short ttype;		/* token type */
	short stype;		/* operator subtype or field id or -1 */
	short ftype;		/* function type */
	} stack[100];

static int topstack;		/* index for top of stack */
static int topterm;		/* index for top terminal token on stack */

uexpr(e,x,n,pr,dfile)
char	**e,*pr;
struct	fmt x[];
int	n;
char	*dfile;
{
	/* this function is called by unity programs to parse
	   expressions containing table attributes.  it handles
	   arithmetic functions (+-/*), unary +-, parentheses,
	   abs(attribute), and numeric constants.
	   the tokens are passed to uexpr() in argv[][] format
	   in variable e and are terminated by a NULL pointer.
	*/

	struct tree *expr();

	/* make variables known globally (i.e. at any level of recursion) */
	xx = x;
	nattr1 = n;
	strcpy(prog,pr);
	strcpy(Dtable1,dfile);

	if(tree != NULL)
		efree();	/* free previous expression tree */
	if (*e) {		/* expression clause present */
		pp = --e;	/* back up so correct when incremented in lex */
		tree = expr();	/* parse expr and build evaluation tree */
	}
	else
		tree = NULL;
}

nuexpr(e,x,n,pr,dfile,afld)
char	**e,*pr;
struct	fmt x[];
int	n;
char	*dfile;
int	afld;
{				/* per attribute version of uexpr() */

	struct tree *expr();

	if (afld >= 0 && afld < MAXATT) {

		/* make variables known globally (i.e. at any level of recursion) */
		xx = x;
		nattr1 = n;
		strcpy(prog,pr);
		strcpy(Dtable1,dfile);

		if (ntree[afld] != NULL)
			efreen(afld);	/* free previous expression tree */
		if (*e) {		/* expression clause present */
			pp = --e;	/* back up so correct when incremented in lex */
			ntree[afld] = expr();	/* parse expr and build evaluation tree */
		} else {
			ntree[afld] = NULL;
		}
	} else {
		error(E_GENERAL,"%s: Out-of-range attribute index passed to nuexpr()\n", prog);
		kill(getpid(),SIGINT);
	}
}

static struct tree *
getnode()
{
	/* this function gets space for a new node for the evaluation
	   tree and initializes it */

	struct tree *rp;

	rp = (struct tree *)malloc(sizeof(struct tree));	/* allocate */
	if (rp == NULL) {
		error(E_GENERAL,"%s: Failed to get memory\n", prog);
		kill(getpid(),SIGINT);
	} else {
		rp->field1 = -1;	/* invalid field number */
		rp->field2 = -1;	/* invalid field number */
		rp->val1 = 0;		/* no value */
		rp->val2 = 0;		/* no value */
		rp->left = NULL;	/* no child */
		rp->right = NULL;	/* no child */
		rp->op = OPNONE;	/* no operation */
	}
	return(rp);
}

static struct tree *
expr()
{
	/* This is the function that actually does the expression parsing.
	   It is implemented as an operator precedence parser.
	   See Principles of Compiler Design by Aho and Ullman, pg. 171
	   for sketch of this function.
	*/
	struct tree *rp;

	/* initialize stack */
	topstack = topterm = type = 0;
	stack[topstack].ttype = 0;	/* set boundary */

	lex();				/* get first token */
	for(;;) {
		if(topterm == 0 && type == OPNONE)
			break;	/* only boundary on stack and no more input */

		/* check to see that tokens can appear adjacently */
		if(p[stack[topstack].ttype][type] == 0) {
			error(E_GENERAL,"%s: Invalid expression\n",prog);
			kill(getpid(),SIGINT);
		}

		/* check precedence of topmost terminal symbol and current
		   input symbol
		*/
		switch(p[stack[topterm].ttype][type]) {
		case 1:				/* yields precedence */
		case 3:				/* equal preceedence */
			shift();		/* shift onto stack */
			lex();			/* get next token */
			break;

		case 2: 			/* takes precedence */
			do {
				reduce();	/* reduce expression
			until top stack terminal yields to terminal
			most recently popped or no more terminals on stack */
			} while(p[stack[topterm].ttype][prevterm]!=1 &&
				topterm != 0);
			break;

		default:	/* prec == 0	invalid combination */
			error(E_GENERAL,"%s: Invalid expression\n",prog);
			kill(getpid(),SIGINT);
			break;
		}
	}

	if(topstack != 1) {
		error(E_GENERAL,"%s: Invalid expression\n",prog);
		kill(getpid(),SIGINT);
	}

	if(stack[1].result == NULL) {	/* top item not a node */
		rp = getnode();		/* get a new node */

		/* set operand */
		if(stack[1].stype >= 0)	{	/* field value */
			rp->field1 = stack[1].stype;
			if(stack[1].ttype == FUNC)
				rp->op = stack[1].ftype;
		}
		else			/* constant value */
			rp->val1 = stack[1].svtype;
	}
	else				/* node value */
		rp = stack[1].result;

	return(rp);
}

static int
lex()
{
	/* function to do lexical analysis */

	char aname[40];		/* temporary storage for attribute name */
	int i;

	pp++;					/* get next token */

	if(*pp == NULL) {			/* no more tokens */
		type = OPNONE;
		return;
	}

	switch(pp[0][0]) {
	case '+':
		if(pp[0][1] != '\0') {
			error(E_GENERAL,
				"%s: '+' must appear as a separate token\n",
				prog);
			kill(getpid(),SIGINT);
		}
			
		if(type == LPAREN || type == ADDOP || type == MULOP ||
				type == BOUNDARY) {
			type = UNARY;
			subtype = UPLUS;
		}
		else {
			type = ADDOP;
			subtype = ADD;
		}
		break;
	case '-':
		if(pp[0][1] != '\0') {
			error(E_GENERAL,
				"%s: '-' must appear as a separate token\n",
				prog);
			kill(getpid(),SIGINT);
		}

		if(type == LPAREN || type == ADDOP || type == MULOP ||
				type == BOUNDARY) {
			type = UNARY;
			subtype = UMINUS;
		}
		else {
			type = ADDOP;
			subtype = SUB;
		}
		break;
	case '*':
		if(pp[0][1] != '\0') {
			error(E_GENERAL,
				"%s: '*' must appear as a separate token\n",
				prog);
			kill(getpid(),SIGINT);
		}

		type = MULOP;
		subtype = MUL;
		break;
	case '/':
		if(pp[0][1] != '\0') {
			error(E_GENERAL,
				"%s: '/' must appear as a separate token\n",
				prog);
			kill(getpid(),SIGINT);
		}

		type = MULOP;
		subtype = DIV;
		break;
	case '%':
		if(pp[0][1] != '\0') {
			error(E_GENERAL,
				"%s: '%' must appear as a separate token\n",
				prog);
			kill(getpid(),SIGINT);
		}

		type = MULOP;
		subtype = MODULO;
		break;
	case ')':
		if(pp[0][1] != '\0') {
			error(E_GENERAL,
				"%s: ')' must appear as a separate token\n",
				prog);
			kill(getpid(),SIGINT);
		}

		type = RPAREN;
		break;
	case '(':
		if(pp[0][1] != '\0') {
			error(E_GENERAL,
				"%s: '(' must appear as a separate token\n",
				prog);
			kill(getpid(),SIGINT);
		}

		type = LPAREN;
		break;
	default:
		/* check for functions */

		if(strncmp("abs(",pp[0],4) == 0 ||
			strncmp("ABS(",pp[0],4) == 0) {
			type = FUNC;
			fcttype = ABS;

			strcpy(aname,&pp[0][4]);
			i = strlen(aname);
			if(i == 0 || aname[i-1] != ')') {
				error(E_GENERAL,
					"%s: Bad abs function call %s\n",pp[0]);
				kill(getpid(),SIGINT);
			}
			aname[i-1] = '\0';
			if((subtype=setnum(xx,aname,nattr1)) == ERR) {
				error(E_ILLATTR,prog,aname,Dtable1);
				kill(getpid(),SIGINT);
			}
		}
		else if((strncmp("hex(",pp[0],4) == 0) ||
			(strncmp("HEX(",pp[0],4) == 0)) {
			type = FUNC;
			fcttype = HEX;

			strcpy(aname,&pp[0][4]);
			i = strlen(aname);
			if(i == 0 || aname[i-1] != ')') {
				error(E_GENERAL,
					"%s: Bad hex function call %s\n",pp[0]);
				kill(getpid(),SIGINT);
			}
			aname[i-1] = '\0';
			if((subtype=setnum(xx,aname,nattr1)) == ERR) {
				error(E_ILLATTR,prog,aname,Dtable1);
				kill(getpid(),SIGINT);
			}
		}
		else if((strncmp("oct(",pp[0],4) == 0) ||
			(strncmp("OCT(",pp[0],4) == 0)) {
			type = FUNC;
			fcttype = OCT;

			strcpy(aname,&pp[0][4]);
			i = strlen(aname);
			if(i == 0 || aname[i-1] != ')') {
				error(E_GENERAL,
					"%s: Bad oct function call %s\n",pp[0]);
				kill(getpid(),SIGINT);
			}
			aname[i-1] = '\0';
			if((subtype=setnum(xx,aname,nattr1)) == ERR) {
				error(E_ILLATTR,prog,aname,Dtable1);
				kill(getpid(),SIGINT);
			}
		}
		else if((strncmp("bin(",pp[0],4) == 0) ||
			(strncmp("BIN(",pp[0],4) == 0)) {
			type = FUNC;
			fcttype = BIN;

			strcpy(aname,&pp[0][4]);
			i = strlen(aname);
			if(i == 0 || aname[i-1] != ')') {
				error(E_GENERAL,
					"%s: Bad bin function call %s\n",pp[0]);
				kill(getpid(),SIGINT);
			}
			aname[i-1] = '\0';
			if((subtype=setnum(xx,aname,nattr1)) == ERR) {
				error(E_ILLATTR,prog,aname,Dtable1);
				kill(getpid(),SIGINT);
			}
		}
		else if((strncmp("len(",pp[0],4) == 0) ||
			(strncmp("LEN(",pp[0],4) == 0)) {
			type = FUNC;
			fcttype = LEN;

			strcpy(aname,&pp[0][4]);
			i = strlen(aname);
			if(i == 0 || aname[i-1] != ')') {
				error(E_GENERAL,
					"%s: Bad len function call %s\n",pp[0]);
				kill(getpid(),SIGINT);
			}
			aname[i-1] = '\0';
			if((subtype=setnum(xx,aname,nattr1)) == ERR) {
				error(E_ILLATTR,prog,aname,Dtable1);
				kill(getpid(),SIGINT);
			}
		}
		/* check for constant - treat as type identifier */
		else if( isdigit(pp[0][0]) ) {
			type = IDENT;
			subtype = -1;	/* invalid field number => subvtype */
			sscanf(pp[0],"%lf",&subvtype);
		}
		/* attribute name */
		else if( isalpha(pp[0][0]) || pp[0][0] == '_' ) {
			type = IDENT;
			if((subtype=setnum(xx,pp[0],nattr1)) == ERR) {
				error(E_ILLATTR,prog,pp[0],Dtable1);
				kill(getpid(),SIGINT);
			}
		}
		else {
			error(E_GENERAL,"%s: Bad token in expression %s\n",
				prog,pp[0]);
			kill(getpid(),SIGINT);
		}
		break;
	}
}

shift()
{
	/* add new token to stack */
	topstack = topstack + 1;

	stack[topstack].ttype = type;		/* set token type */

	/* set subtype or field id */
	if(subtype >= 0)
		stack[topstack].stype = subtype;
	else {
		stack[topstack].stype = -1;
		stack[topstack].svtype = subvtype;
	}
	stack[topstack].result = NULL;

	if(type == FUNC)
		stack[topstack].ftype = fcttype;	/* set function type */

	if(type != IDENT)
		topterm = topstack;	/* set to top terminal on stack */
}

static int
reduce()
{
	/* this function pops tokens from stack based on operation */

	int prev;				/* previous terminal index */

	prevterm = stack[topterm].ttype;	/* previous terminal type */

	switch(stack[topterm].ttype) {
	case ADDOP:
		biop();
		break;
	case MULOP:
		biop();
		break;
	case UNARY:
		unop();
		break;
	case FUNC:
		fctn();
		break;
	case RPAREN:
		prev = topterm - 1;

		/* find previous terminal */
		while(prev > 0 && 
			(stack[prev].ttype==IDENT || stack[prev].ttype==FUNC) )
			prev = prev - 1;
		if(stack[prev].ttype == LPAREN) {
			if(stack[topterm-1].ttype != IDENT &&
				stack[topterm-1].ttype != FUNC) {
				error(E_GENERAL,"%s: Invalid expression\n",
					prog);
				kill(getpid(),SIGINT);
			}
			stack[prev] = stack[topterm - 1];
			topstack = prev;
			topterm = prev - 1;
		}
		else {
			error(E_GENERAL,"%s: Invalid expression\n",prog);
			kill(getpid(),SIGINT);
		}
		break;
	default:
		error(E_GENERAL,"%s: Invalid expression\n",prog);
		kill(getpid(),SIGINT);
		break;
	}
}

biop()
{
	/* this function handles all binary operators and should be
	   stacked as    {IDENT|FUNC} operator {IDENT|FUNC}
	*/
	struct tree *node;

	if((stack[topterm-1].ttype != FUNC && stack[topterm-1].ttype != IDENT)||
	(stack[topterm+1].ttype != FUNC && stack[topterm+1].ttype != IDENT) ) {
		error(E_GENERAL,"%s: Invalid expression\n",prog);
		kill(getpid(),SIGINT);
	}

	/* get a new node */
	node = getnode();

	/* set first operand */
	if(stack[topterm-1].result == NULL) {
		if(stack[topterm-1].stype >= 0)
			node->field1 = stack[topterm-1].stype;
		else
			node->val1 = stack[topterm-1].svtype;
	}
	else
		node->left = stack[topterm-1].result;

	/* set second operand */
	if(stack[topterm+1].result == NULL) {
		if(stack[topterm+1].stype >= 0)
			node->field2 = stack[topterm+1].stype;
		else
			node->val2 = stack[topterm+1].svtype;
	}
	else
		node->right = stack[topterm+1].result;

	/* set operator type based on token sub-type */
	node->op = stack[topterm].stype;

	/* stack house keeping */
	topstack = topterm - 1;		/* reset index to top of stack */
	topterm = topterm - 2;		/* reset index to top terminal */
	stack[topstack].result = node;	/* set to point to new node */
	stack[topstack].stype = -1;	/* invalid sub-type */
	stack[topstack].ttype = IDENT;	/* treat as identifier */
}

unop()
{
	struct tree *node;

	/* this function handles all unary operators and should be
	   stacked as    operator {IDENT|FUNC}
	*/

	if(stack[topterm+1].ttype != IDENT && stack[topterm+1].ttype != FUNC) {
		error(E_GENERAL,"%s: Invalid expression\n",prog);
		kill(getpid(),SIGINT);
	}

	/* switch on unary operator type */
	switch(stack[topterm].stype) {
	case UPLUS:
		/* ignore unary plus */
		stack[topterm] = stack[topterm+1];

		break;
	case UMINUS:
		/* get a new node */
		node = getnode();

		/* set operand */
		if(stack[topterm+1].result == NULL)
			if(stack[topterm+1].stype >= 0)
				node->field1 = stack[topterm+1].stype;
			else
				node->val1 = stack[topterm+1].svtype;
		else
			node->left = stack[topterm+1].result;

		/* set operator */
		node->op = stack[topterm].stype;

		/* stack house keeping */
		stack[topterm].result = node;	/* set to point to new node */
		stack[topterm].stype = -1;	/* invalid sub-type */
		stack[topterm].ttype = IDENT;	/* treat as identifier */

		break;
	default:
		/* this should never occur */
		error(E_GENERAL,"%s: Error in unop()\n",prog);
		kill(getpid(),SIGINT);
		break;
	}

	topstack = topterm;		/* reset index to top of stack */
	topterm = topterm - 1;		/* reset index to top terminal */
}

static int
fctn()
{
	struct tree *node;

	/* get a new node */
	node = getnode();

	/* set operator value - always a field value */
	node->field1 = stack[topterm].stype;

	/* set operator - function type */
	node->op = stack[topterm].ftype;

	stack[topterm].result = node;		/* set to point to new node */
	stack[topterm].stype = -1;		/* no field id */
	stack[topterm].ttype = IDENT;		/* treat as identifier */

	topstack = topterm;			/* reset to top of stack */
	topterm = topterm - 1;			/* rest to top terminal */
}

#ifdef INTCOMP
int
#else
double
#endif
ueval(x)
struct	fmt x[];
{
#ifndef INTCOMP
	double uevaltr();
#endif

	xx = x;				/* make globally known */

	return(uevaltr(tree));		/* evaluate tree */
}

#ifdef INTCOMP
int
#else
double
#endif
nueval(x,afld)
struct	fmt x[];
int	afld;
{					/* per attribute version of ueval() */
#ifndef INTCOMP
	double uevaltr();
#endif

	xx = x;				/* make globally known */

	if ((afld >= 0) && (afld < MAXATT)) {
		if (ntree[afld] != NULL) {
			return(uevaltr(ntree[afld]));		/* evaluate tree */
		} else {
			return(0);
		}
	} else {
		error(E_GENERAL,"%s: Out-of-range attribute index passed to nueval()\n", prog);
		kill(getpid(),SIGINT);
	}
}

#ifdef INTCOMP
static int
#else
static double
#endif
uevaltr(tr)
struct tree *tr;
{
#ifdef INTCOMP
	int l,r,ret;
#else
	double l,r,ret;
#endif

	l = r = 0;
	/* get value of left side */
	if(tr->left != NULL)
		l = uevaltr(tr->left);
	else if(tr->field1 >= 0) {

		/* check if base conversion was requested */
		if(((tr->op) >= (short)BASE+2) && ((tr->op) <= (short)B36)) {
#ifdef __STDC__
			/*
			 * use unsigned conversion if available to
			 * avoid problem with maximum unsigned value
			 * since most (all) non-base 10 values are
			 * initially generated from an unsigned int
			 */
			l = (int) strtoul(xx[tr->field1].val, (char **)NULL, (tr->op - BASE));
#else
			l = (int) strtol(xx[tr->field1].val, (char **)NULL, (tr->op - BASE));
#endif
		} else if (tr->op == LEN) {
			l = strlen(xx[tr->field1].val);
		} else {
			l = 0;	/* in case sscanf(3S) does not give it a value */
#ifdef INTCOMP
			sscanf(xx[tr->field1].val,"%d",&l);
#else
			sscanf(xx[tr->field1].val,"%lf",&l);
#endif
		}
	} else
		l = tr->val1;

	/* get value of right side */
	if(tr->right != NULL)
		r = uevaltr(tr->right);
	else if(tr->field2 >= 0) {
		r = 0;	/* in case sscanf(3S) does not give it a value */
#ifdef INTCOMP
		sscanf(xx[tr->field2].val,"%d",&r);
#else
		sscanf(xx[tr->field2].val,"%lf",&r);
#endif
	} else
		r = tr->val2;

	ret = l;	/* default return value is value of left side */
	switch(tr->op) {
	case ADD:
		ret = ret + r;
		break;
	case SUB:
		ret = ret - r;
		break;
	case MUL:
		ret = ret * r;
		break;
	case DIV:
		if(r == 0) {
			error(E_GENERAL,"%s: 0 divide\n",prog);
			ret = 0;
		}
		else
			ret = ret / r;
		break;
	case MODULO:
		if(r == 0) {
			error(E_GENERAL,"%s: 0 divide\n",prog);
			ret = 0;
		}
		else
#ifdef INTCOMP
			ret = ret % r;
#else
			ret = (double)((int)ret % (int)r);
#endif
		break;
	case UMINUS:
		ret = -ret;
		break;
	case ABS:
		if(ret < 0)
			ret = -ret;
		break;
	case OPNONE:
	case BIN:
	case HEX:
	case OCT:	
	case LEN:	
		break;
	default:
		error(E_GENERAL,"%s: Bad operator during evaluation\n",prog);
		break;
	}
	return(ret);
}

efree()
{
	if(tree != NULL)
		efree1(tree);		/* free tree */
	tree = NULL;
	return;
}

efreen(afld)
int	afld;
{
	register int i;

	if (afld < 0 || afld > MAXATT) {
		error(E_GENERAL,"%s: Out-of-range attribute index passed to efreen()\n", prog);
		kill(getpid(),SIGINT);
	} else if (afld == MAXATT) {
		i = 0;
	} else {
		i = afld;
	}

	do {
		if (ntree[i] != NULL) {
			efree1(ntree[i]);		/* free tree */
			ntree[i] = NULL;
		}
	} while (++i < afld);
}

static int
efree1(tr)
struct tree *tr;
{
	if(tr->left != NULL)
		efree1(tr->left);

	if(tr->right != NULL)
		efree1(tr->right);
	free(tr);
	return;
}
