/*
 * Copyright (C) 2002 by Lucent Technologies
 *
 */

%{
%}
/**
Intralisting Validation Compiler.
Uses yacc(1).

The validation compiler is run separately from the validator
and it puts object code on the W file.
While the compiler is running,
the line memory is on a temporary file ("L<process>").
After compilation,
"Wfile" looks like this:

<byte count of instructions (short)>
<byte count of data (short)>
<instructions>
<data>
<lines>

*/
%token V_NEG V_NOT V_LEQ V_LNE V_LLT V_LLE V_LGT V_LGE V_EQ
%token V_NE V_LT V_LE V_GT V_GE V_REG V_RRG
%token V_OR V_AND V_CAT V_ADD V_SUB V_MUL V_DIV NAME STR NUM V_ERR
%left  V_OR
%left  V_AND
%left  V_EQ V_NE V_LEQ V_LNE V_REG V_RRG
%left  V_LT V_GT V_LE V_GE V_LLT V_LGT V_LLE V_LGE
%left  V_CAT
%left  V_ADD V_SUB
%left  V_MUL V_DIV
%left  V_NEG V_NOT
%{
static char sccsid[] = "@(#)valcmp.y	1.12";
#ifdef __STDC__
#include <stdlib.h>
#endif
#include <stdio.h>
#include "db.h"
#include "val.h"
#include <ctype.h>

extern char *mktemp(),*strcpy(), *getenv(), *dirname();
extern FILE *get2file();
static FILE *Xpgmiop;			/* For validation table. */
static FILE *Xobjiop;			/* Object code. */
static FILE *Xlineiop;			/* Line memory. */
static unsigned Xinstrloc;		/* Instruction location counter. */
static int Xlinenum;			/* Validation table line number. */
static int Xdownlist[50];		/* Locations of incomplete DOWNs. */
#define DLSIZE sizeof(Xdownlist)/sizeof(Xdownlist[0])
static int Xlev;			/* Indent level of current val. cond. */
static int Xprelev;			/* Indent level of prev. val. cond. */
static int Xpreerrcode = -1;		/* -1 = start of table
				    	0 = previous val. cond. had no err code
				    	1 = previous val. cond. had err code */
static int Xi;				/* Scratch variable. */
static int Xerrstr;			/* Index of error code string. */
static int Xlopt;			/* 1 => list option. */
static int Xerrflg=0;			/* 1 => err has occurred during comp */
static struct fmt xx[MAXATT];		/* field characteristics */
static char *prog;
static int Xnfields;			/* number of fields */
static char Dtable[MAXPATH+4];		/* name of description file */
static char Vtable[MAXPATH+4];		/* name of validation file */
static char Wname[MAXPATH+4];		/* name of object file */
					/* allow for "/./" or "././" prefix (+4) */

/**
Xmne array contains mnemonics. Indexed by op code as defined in val.h
*/
static char *Xmne[] = {"HALT",
		"CAT",
		"OR",
		"AND",
		"EQ",
		"NE",
		"LEQ",
		"LNE",
		"LT",
		"LE",
		"GT",
		"GE",
		"LLT",
		"LLE",
		"LGT",
		"LGE",
		"ADD",
		"SUB",
		"MUL",
		"DIV",
		"NEG",
		"NOT",
		"","","",
		"DOWN",
		"ERR",
		"ERRP",
		"PUSHN",
		"PUSHS",
		"PUSHI",
		"FIELD",
		"CALL",
		"REG"
	};

main(argc,argv)
int argc;
char *argv[];
{
	yyinit(argc,argv);	/* initialization */
	if(yyparse())
		exit(1);	/* parser error detected */
	yyaccpt();
}

%}


%%
tree	: treex
	{
/**
At this point entire table has been parsed.
After finishing off the last validation condition,
we issue a HALT and make all incomplete DOWNs reference it.
*/
		if (Xpreerrcode == -1)
			ftlerr("empty table");
		else if (Xpreerrcode)
			linstr(Ierr, storestr());
		else
			err("must end with error code");
		for (Xi=0; Xi<=DLSIZE && Xdownlist[Xi]; Xi++) {
			fseek(Xobjiop, (long)(Xdownlist[Xi]+5), 0);
			putshort(Xinstrloc, Xobjiop);
		}
		fseek(Xobjiop, 0L, 2);
		sinstr(Ihalt);
	}
	;


treex	: treex node
	| node
	;


node	: indent expr '\n'
	{
		Xpreerrcode = 0;
	}
	| indent expr tabs NAME '\n'
	{
		Xpreerrcode = 1;
/**
We don't know whether to issue an V_ERR or V_ERRP,
because we don't know whether the next line is indented,
requiring a DOWN.
So we save an index to the error code and put it out
while processing the next validation condition.
*/
		Xerrstr = storestr();
	}
	| indentx '\n'
/**
In case of error we skip to the next newline and assume things
are OK from there.
*/
	| error '\n'
	{
		yyerrok;
	}
	;


indent	: indentx
	{
/**
Three possible sequences,
depending on current indent, previous indent
and whether previous validation condition had an error code:

1) V_ERR eptr	2) V_ERRP eptr	3) DOWN loc
		   DOWN loc

DOWNs are are handled as described in the manual page.
*/
		if (Xpreerrcode != -1)
			if (Xlev > Xprelev) {
				if (Xlev>Xprelev+1)
					err("indented too much");
				if (Xpreerrcode)
					linstr(Ierrp, Xerrstr);
				if (Xdownlist[Xprelev]!=0)
					ftlerr("downlist");
				Xdownlist[Xprelev] = Xinstrloc;
				linstr(Idown, 0);
			}
			else if (Xpreerrcode)
				linstr(Ierr, Xerrstr);
			else
				err("prev line no error code or bad indent");
		Xprelev = Xlev;
		if (Xlev >= DLSIZE)
			err("indentation too deep");
		for (Xi=Xlev; Xi<=DLSIZE && Xdownlist[Xi]; Xi++) {
			fseek(Xobjiop, (long)(Xdownlist[Xi] + 5), 0);
			putshort(Xinstrloc, Xobjiop);
			Xdownlist[Xi] = 0;
		}
		fseek(Xobjiop, 0L, 2);
	}
	;


indentx	: indentx '\t'
	{
		Xlev++;
	}
	|
	{
		Xlev = 0;
	}
	;


tabs	: tabs '\t'
	| '\t'
	;


expr	: expr V_CAT expr		{ sinstr(Icat); }
	| expr V_OR  expr		{ sinstr(Ior ); }
	| expr V_AND expr		{ sinstr(Iand); }
	| expr V_EQ  expr		{ sinstr(Ieq ); }
	| expr V_NE  expr		{ sinstr(Ine ); }
	| expr V_LEQ expr		{ sinstr(Ileq); }
	| expr V_LNE expr		{ sinstr(Ilne); }
	| expr V_LT  expr		{ sinstr(Ilt ); }
	| expr V_LE  expr		{ sinstr(Ile ); }
	| expr V_GT  expr		{ sinstr(Igt ); }
	| expr V_GE  expr		{ sinstr(Ige ); }
	| expr V_LLT expr		{ sinstr(Illt); }
	| expr V_LLE expr		{ sinstr(Ille); }
	| expr V_LGT expr		{ sinstr(Ilgt); }
	| expr V_LGE expr		{ sinstr(Ilge); }
	| expr V_ADD expr		{ sinstr(Iadd); }
	| expr V_SUB expr		{ sinstr(Isub); }
	| expr V_MUL expr		{ sinstr(Imul); }
	| expr V_DIV expr		{ sinstr(Idiv); }
	| V_SUB expr %prec V_NEG	{ sinstr(Ineg); }
	| V_NOT expr		{ sinstr(Inot); }
	| expr V_REG STR		{ linstr(Ireg, storereg(V_REG)); }
	| expr V_RRG STR		{ linstr(Ireg, storereg(V_RRG)); }
	| '(' expr ')'
	| STR			{ linstr(Ipushs, storestr()); }
	| NUM			{ linstr(Ipushn, storenum()); }
	| NAME			{ linstr(Ifield, fieldid()); }
	| fcn '(' args ')'
	{
		linstr(Ipushi, $3);
		linstr(Icall, $1);
	}
	;


fcn	: NAME			{ $$ = storestr(); }
	;


args	: args ',' expr		{ $$ = $1 + 1; }
	| expr			{ $$ = 1; }
	|			{ $$ = 0; }
	;
%%


/**
Function to issue short instructions.
*/
static int
sinstr(in)
register int in;
{
	if (in < 0 || in > 127)
		ftlerr("sinstr: bad instr");
	putc(in, Xobjiop);
	if (++Xinstrloc > 32760)
		ftlerr("table too big");
}

/**
Function to issue long instructions.
*/
static int
linstr(in, operand)
register int in, operand;
{
	sinstr(in);
	if (operand < 0)
		ftlerr("linstr: bad operand");
	putshort(operand, Xobjiop);
	Xinstrloc += 2;
}


/**
Execution starts here.
*/
static int
yyinit(argc, argv)
register int argc;
register char **argv;
{
	extern char *strrchr();
	static char lfile[8] = "LXXXXXX";
	char	tmp[MAXPATH+4];		/* allow for "/./" or "././" prefix */
	char	*Ioption, *t;
	int	c;

	if ((prog = strrchr(argv[0],'/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	Ioption = NULL;
	argv++; argc--;

	while( argc > 0 && argv[0][0] == '-' && argv[0][1] != '\0') {
		switch(argv[0][1]) {
		case 'I':
			Ioption = &argv[0][2];
			break;
		case 'l':
			Xlopt = 1;	/* print listing on finish */
			break;
		default :
			error(E_GENERAL,
			"Usage: %s [-l] [-Itable] fname [outfname]\n",prog);
			exit(1);
		}
		argv++; argc--;
	}

	if(argc != 1 && argc != 2) {
		error(E_GENERAL,"Usage: %s [-l] [-Itable] table [outfname]\n",
			prog);
		exit(1);
	}

	if((Xnfields=mkntbl(prog, argv[0], Dtable, xx, Ioption)) == ERR)
		exit(1);

	if(close(creat(mktemp(lfile), 0666))==-1) {
		error(E_GENERAL,"Cannot write line file %s\n",lfile);
		exit(1);
	}
	Xlineiop = fdopen(open(lfile, 2), "w");
	unlink(lfile);
	/* determine object file name */
	if(argc == 1) { /* default is simple file name prefixed with 'W' */
		getfile(Wname,argv[0],0);
		Wname[0] = 'W';
	}
	else
		strcpy(Wname,argv[1]);
	if((Xobjiop = fopen(Wname, "w")) == NULL) {
		error(E_GENERAL,"Cannot write validation object file %s\n",
			Wname);
		exit(1);
	}
	putshort(0, Xobjiop);
	putshort(0, Xobjiop);

	/*
	 * If an alternate descriptor has been given
	 * that includes a "/./" (full) or "././" (relative)
	 * path prefix or if the alternate table name is the
	 * same as the table name then look for the descriptor
	 * in the given directory first.
	 * Otherwise, just stick with the default search.
	 *
	 * NOTE: The following search order check is
	 *       the same one that mkntbl() does!
	 */
	if ((Ioption) && (*Ioption)) {
		if ((strncmp(Ioption, "/./", 3) == 0) ||
		    (strncmp(Ioption, "././", 4) == 0) ||
		    (strcmp(argv[0], Ioption) == 0)) {
			c = 'd';
		} else {
			c = 's';	/* use standard search order */
		}
		t = Ioption;
	} else {
		t = argv[0];
		c = 's';	/* use standard search order */
	}

	/* validation file name is simple file name prefixed with 'V' */
	if ((Xpgmiop=get2file(c, "V", t, Vtable, "r")) == NULL) {
		getfile(Vtable,t,0);
		Vtable[0] = 'V';
		error(E_GENERAL, "Cannot read validation table %s\n", Vtable);
		exit(1);
	}
}

/**
Lexical analyzer begins here.
Xop contains characters which can appear in an operator.
Xops contains the actual operators such that operator
Xops[i] corresponds to token Xotype[i].
Xops and Xotype could be in a structure instead,
but initializing structures is a pain in the neck.
When Xops is searched sequentially from the beginning to
find an operator,
the second appearance of "-" (V_NEG) will never be found;
it is there because yyerror() searches Xotype to get an index into
Xops for printing diagnostics.
*/
char Xop[] = "*/+-<>=$!&|#%";
char *Xops[] = {"!","==","!=","$<","$<=","$>","$>=","#==","#!=","<","<=",">",
	     ">=","|","&","$","+","-","-","*","/","%","%%",0};
int Xotype[] = {V_NOT,V_LEQ,V_LNE,V_LLT,V_LLE,V_LGT,V_LGE,V_EQ,V_NE,
		V_LT,V_LE,V_GT,V_GE,V_OR,V_AND,V_CAT,V_ADD,V_SUB,V_NEG,
		V_MUL,V_DIV,V_REG,V_RRG,0};

/**
Function called by yacc.
Yacc external yylval isn't used;
tokens are left in Xstrbuf.
This function returns token number or 0 at end of file.
*/
static int
yylex()
{
	register int c;
	register int i;

	while (c=getch()) {
		if (c == ' ')
			continue;
		if (c == '"')
			return(chrstr());
		if (c == '/' && peekch() == '*') {
			comment();
			continue;
		}
		if (isalpha(c) || c=='_') return(ident());
		if (isdigit(c))
			return(numb());
		for (i=0; Xop[i]; i++)
		if (c == Xop[i]) return(oper());
		return(c);
	}
	return(0);
}


/**
Function to pick up an operator.
*/
static int
oper()
{
	register int c;
	char x[10];
	register int i, j;

	x[i=0] = lastch();
	while (1) {
		c = getch();
		for (j=0; Xop[j] && Xop[j]!=c; j++)
			;
		if (!Xop[j])
			break;
		if (i < sizeof(x) - 2)
			x[++i] = c;
	}
	x[i+1] = '\0';
	pushch(c);
	if (i == sizeof(x) - 2)
		return(err("op too long: %s", x));
	for (i=0; Xops[i]; i++)
		if (equal(Xops[i], x))
			return(Xotype[i]);
	return(err("unknown op: %s", x));
}


static char Xstrbuf[256], *Xstrp;/* Place for strings, names and numbers. */


/**
Function to pick up string constants.
Most of it is concerned with the \ escapes.
*/
static int
chrstr()
{
	register int c;
	register int state;

	Xstrp = Xstrbuf;
	state = 0;

	while (c=getch())
		switch (state) {
		case 0:
			if (c == '"') {
				putstr('\0');
				return(STR);
			}
			if (c == '\n')
				return(err("nonterminated string"));
			if (c == '\\')
				state = 1;
			else
				putstr(c);
			continue;
		case 1:
			if (c>='0' && c<='7') {
				putstr(c);
				state = 2;
				continue;
			}
			switch (c) {
			case 'b':
				putstr('\b');
				break;
			case 'n':
				putstr('\n');
				break;
			case 'r':
				putstr('\r');
				break;
			case 't':
				putstr('\t');
				break;
			default:
				putstr(c);   
				break;
			}
			state = 0;
			continue;
		case 2:
			if (c>='0' && c<='7') {
				putstr(c);
				state = 3;
				continue;
			}
			putstr('\0');
			putstr(octal(Xstrp -= 2));
			pushch(c);
			state = 0;
			continue;
		case 3:
			if (c>='0' && c<='7') {
				putstr(c);
				state = 4;
				continue;
			}
			putstr('\0');
			putstr(octal(Xstrp -= 3));
			pushch(c);
			state = 0;
			continue;
		case 4:
			putstr('\0');
			putstr(octal(Xstrp -= 4));
			pushch(c);
			state = 0;
			continue;
		}
	ftlerr("nonterminated string");
}


/**
Function to pick up identifiers (field names
and error codes).
*/
static int
ident()
{
	register int c;

	Xstrp = Xstrbuf;
	putstr(lastch());
	while (isalnum(c=getch()) || c=='_') putstr(c);
	pushch(c);
	putstr('\0');
	return(NAME);
}


/**
Function to pick up numbers.
Numbers with decimal points are handled by this function,
but not by see storenum() or by the machine.
Having the code here
catches a possible error and also points the way for later
enhancement to allow real numbers.
*/
static int
numb()
{
	register int c;
	register int state;

	Xstrp = Xstrbuf;
	putstr(lastch());
	state = 0;
	while (c=getch()) {
		switch (state) {
		case 0:
			if (isdigit(c)) {
				putstr(c);
				continue;
			}
			if (c == '.') {
				putstr(c);
				state = 1;
				continue;
			}
			break;
		case 1:
			if (isdigit(c)) {
				putstr(c);
				continue;
			}
			break;
		}
		break;
	}
	pushch(c);
	putstr('\0');
	return(NUM);
}


/**
Function to skip over comments.
*/
static int
comment()
{
	register int c;

	getch();
	while (c=getch())
		if (c=='*' && peekch()=='/') {
			getch();
			return;
		}
	ftlerr("nonterminated comment");
}


/**
Function to put characters into Xstrbuf.
*/
static int
putstr(c)
int c;
{
	static errmode;

	if (Xstrp == Xstrbuf)
		errmode = 0;
	if (errmode)
		return;
	if (Xstrp == &Xstrbuf[sizeof(Xstrbuf)]) {
		Xstrbuf[sizeof(Xstrbuf)-1] = '\0';
		errmode = 1;
		err("long string, number or name: %s", Xstrbuf);
		return;
	}
	*Xstrp++ = (char)c;
}
/**
End of lexical analyzer.
*/


/**
Functions storenum(), storestr(), storereg() and fieldid()
process numbers, strings, regular expressions and field references,
respectively, picked up by the lexical analyzer.
The first three put data into data memory (Xstore) and return
an index to it.
*/

static char Xstore[5000];	/* Data memory + constant descriptors. */
static char *Xstdata = Xstore;	/* Ptr to next unused byte in data memory. */

/**
Array of descriptors for constants in data memory builds backwards from
end of Xstore.
For each we keep type (NUM, STR or V_REG) and ptr into data memory
part of Xstore.
This array is searched sequentially before putting any constant
into data memory to avoid duplicating one that is already there.
*/

/* Cast prevents warnings but causes compiler errors on 11/70 USG UNIX 3.0 */

#define CONDESCBEG ((struct Condesc *)&Xstore[sizeof(Xstore)-sizeof(*Xcptr)])

struct Condesc {
	int Ctype;
	char *Cptr;
} *Xcptr = CONDESCBEG;

/**
Function to store numbers in data memory.
Numbers are stored on a long boundary address.
*/
static int
storenum()
{
	register struct Condesc *p;
	register char *s;
	long n;
	int i;

	n = 0;
	s = Xstrbuf;
	while (*s >= '0' && *s <= '9')
		n = 10*n + *s++ - '0';

	if(*s) {
		err("conversion of %s", Xstrbuf);
		return(0);
	}

	for (p = CONDESCBEG; p>Xcptr; p--)
		if (p->Ctype == NUM && *((long *)(p->Cptr)) == n)
			return(p->Cptr - Xstore);
	i= (unsigned long)Xstdata % sizeof(long);
	if ( i ) {
		while( i++ != sizeof( long ) )
			*Xstdata++ = '\0';
	}
	Xcptr->Ctype = NUM;
	Xcptr->Cptr = Xstdata;
	*((long *)Xstdata) = n;
	if ((unsigned)(Xstdata += sizeof(n)) > (unsigned)Xcptr)
		ftlerr("constant overflow");
	return(Xcptr--->Cptr - Xstore);
}


/**
Function to store strings in data memory.
They can be stored at an even or odd address.
*/
static int
storestr()
{
	register struct Condesc *p;

	for (p = CONDESCBEG; p>Xcptr; p--)
		if (p->Ctype==STR && equal(p->Cptr, Xstrbuf))
			return(p->Cptr - Xstore);
	Xcptr->Ctype = STR;
	Xcptr->Cptr = Xstdata;
	strcpy(Xstdata, Xstrbuf);
	if ((unsigned)(Xstdata += strlen(Xstrbuf) + 1) > (unsigned)Xcptr)
		ftlerr("constant overflow");
	return(Xcptr--->Cptr - Xstore);
}


/**
Function to compile and store regular expressions.
The %% operator is handled by modifying the text of
the regular expression;
that is,
if R is a regular expression,
then
S%%"R" is equivalent to S%"R( R)*".
All regular expressions are surrounded by ^ and $.
*/
static int
storereg(op)
int op;
{
	register struct Condesc *p;
	char *code;
	register char *a, *b;
	int i;
	extern int __i_size;	/* defined in regcmp.c - length of compiled
				   regular expression */
	char sbuf[2*sizeof(Xstrbuf)+25], *regcmp();

	if (op == V_REG)
		sprintf(sbuf, "^%s$", Xstrbuf);
	else
		sprintf(sbuf, "^%s( %s)*$", Xstrbuf, Xstrbuf);
	if((code = regcmp(sbuf, 0)) == 0) {
		sprintf(sbuf,"Illegal regular expression: %s",Xstrbuf);
		ftlerr(sbuf);
	}
	for (p = CONDESCBEG; p>Xcptr; p--)
		if (p->Ctype == V_REG && !memcmp(p->Cptr, code, __i_size))
			return(p->Cptr - Xstore);
	Xcptr->Ctype = V_REG;
	Xcptr->Cptr = Xstdata;

	/* move value */
	a = code; b = Xstdata;
	for(i=0; i<__i_size; i++)
		*b++ = *a++;

	free(code);
	if ((unsigned)(Xstdata += __i_size) > (unsigned)Xcptr)
		ftlerr("constant overflow");
	return(Xcptr--->Cptr - Xstore);
}

/**
Function to return field ID of field reference.
*/
static int
fieldid()
{
	register int ret;

	if((ret=setnum(xx,Xstrbuf,Xnfields)) != ERR)
		return(ret);

	err("field %s not in %s", Xstrbuf,Dtable);
	return(-1);
}

/**
Function to convert octal to binary.
*/
static int
octal(s)
char s[];
{
	register int n;
	register char *d;

	n = 0;
	for (d=s; *d; d++)
		n = 8*n + *d-'0';
	return(n);
}

/**
The next group of functions implement character input from
the validation table.
Four functions are for use by the rest of the program:

getch() returns the next character.

peekch() peeks at the next character (but it will
  still be returned by getch()).

lastch() returns the last character returned by getch().

pushch() pushes a character back onto the input to
  be gotten with getch().

Not all permutations are allowed;
for example,
it is illegal to use pushch() immediately after peekch().

These functions also handle lines continued with \,
bumping Xlinenum, and writing the L-file.
The reason why the line numbers don't correspond exactly to
the instructions is that the L-file is written to
as soon as the first character of a line is read,
but V_ERR, V_ERRP and DOWN instructions aren't
issued until the indent level of that line is known.
These three instructions all belong to the previous validation
condition,
not to the one that caused the L-file to be written to.
It would be quite complicated to fix this and not really
worth it since these three instructions cannot generate a
machine diagnostic unless things are unbelievably fouled up,
and even then all that's wrong is that the line number is off by one.
*/
#define EMTY -10
static int Xlastchar = EMTY;
static int Xpeekchar = EMTY;

static int
getch()
{
	register int c;

	if ((c=grabch()) == '\\' && peekch() == '\n') {
		grabch();
		while ((c=grabch()) == ' ' || c == '\t')
			;
	}
	return(c);
}

static int
grabch()
{
	if (Xpeekchar != EMTY) {
		Xlastchar = Xpeekchar;
		Xpeekchar = EMTY;
		return(Xlastchar);
	}
	return(Xlastchar=getchr());
}

static int
lastch()
{
	return(Xlastchar);
}

static int
peekch()
{
	if (Xpeekchar != EMTY)
		return(Xpeekchar);
	return(Xpeekchar=getchr());
}

static int
pushch(c)
int c;
{
	if (Xpeekchar != EMTY)
		ftlerr("bad push");
	Xpeekchar = c;
}


static int Xprevchar = '\n';

static int
getchr()
{
	register int c;
	static int eof;

	if (eof)
		return(0);
	while ((c=getc(Xpgmiop)) != EOF) {
		if (Xprevchar == '\n') {
			if (c == '@') {
				while ((c=getc(Xpgmiop)) != EOF && c != '\n')
					;
				break;
			}
			Xlinenum++;
			putshort(Xinstrloc, Xlineiop);
		}
		return(Xprevchar=c);
	}
	eof = 1;
	return(0);
}

/**
The next group of functions handle errors.
There are three categories of errors:

1. Errors detected by yacc,
for which it calls yyerror().

2 Errors detected by the code in this file
that are specific to the validation module
which are not serious enough to cause compilation
to be aborted.
The function err() is called for these.

3. Errors detected by code in this file and other validation files
are too serious to proceed with compilation.
The function ftlerr() is called for these.

Compilation continues past errors of category 1 or 2,
but an error of category 3 will stop it.
It is easier to treat an error as category 3 than 1 or 2,
and moving an error from the former categories to the latter ones
is generally beneficial.

In order to continue compilation
when the lexical analyzer detects an error,
after calling err() it must return something to yacc.
It returns the token V_ERR which,
since it doesn't appear in the grammar,
causes yacc to detect a syntax error
and call yyerror().
But yyerror() just returns if the
reason it was called is that the token V_ERR was read.
*/

err(a, b, c, d, e, f, g, h, i, j, k)
char *a;
int b, c, d, e, f, g, h, i, j, k;
{
	char mesg[100];

	sprintf(mesg, a, b, c, d, e, f, g, h, i, j, k);
	error(E_GENERAL,"ERROR at %d: %s\n", Xlinenum, mesg);
	Xerrflg = 1;
	return(V_ERR);
}

ftlerr(a, b, c, d, e, f, g, h, i, j, k)
char *a;
int b, c, d, e, f, g, h, i, j, k;
{
	err(a, b, c, d, e, f, g, h, i, j, k);
	error(E_GENERAL, "Compilation aborted.\n");
	exit(1);
}

static int
yyerror(s)
char s[];
{
	extern int yychar;
	register int i;

	Xerrflg = 1;
	if (yychar == V_ERR)
		return;
	error(E_GENERAL,"ERROR @%d: %s", Xlinenum, s);
	if (yychar == '\0') {
		error(E_GENERAL," -- unexpected end\n");
		return;
	}
	error(E_GENERAL," on or about: ");
	if (yychar >= 0400)
		switch (yychar) {
		case NAME:
		case NUM:
			error(E_GENERAL,Xstrbuf);
			break;
		case STR:
			error(E_GENERAL,"\"", Xstrbuf, "\"");
			break;
		default:
			for (i=0; Xotype[i] && Xotype[i]!=yychar; i++)
				;
			if (Xotype[i])
				error(E_GENERAL,Xops[i]);
			else
				error(E_GENERAL,"<unknown token>");
		}
	else
		switch (yychar) {
		case '\t':
			error(E_GENERAL,"<tab>");
			break;
		case '\n':
			error(E_GENERAL,"<newline>");
			break;
		default:
			error(E_GENERAL,"%c", yychar);
			break;
		}
	error(E_GENERAL,"\n");
}


/**
At this point compilation is over.
We combine the instruction,
data and line memories to produce the object code file.
*/
static int
yyaccpt()
{
	register int c, ni, nd;

	if ((ni = Xinstrloc) & 1) {
		ni++;
		putc(0, Xobjiop);
	}
	c = (Xstdata - Xstore) % sizeof(long);
	if(c != 0)
		nd = Xstdata - Xstore + sizeof(long) - c;
	else
		nd = Xstdata - Xstore;
	fseek(Xobjiop, 0L, 0);
	putshort(ni, Xobjiop);
	putshort(nd, Xobjiop);
	fseek(Xobjiop, 0L, 2);
	fwrite(Xstore, 1, nd, Xobjiop);
	fflush(Xlineiop);
	if((Xlineiop = fdopen(fileno(Xlineiop), "r"))==NULL)
		ftlerr("error in fdopen");
	fseek(Xlineiop, 0L, 0);
	while ((c = getc(Xlineiop)) != EOF)
		putc(c, Xobjiop);
	fclose(Xobjiop);
	if (Xlopt)
		list();
	exit(Xerrflg);
}


/**
Function to print
listing.
*/
static int
list()
{
	int w, ninstr, ndata, nline;
	union {
		char c[2];
		short s;
	} stoc;
	short word, *line;
	register int i, lnum;
	char *instr, *data;

	vload(Wname,&instr, &ninstr, &data, &ndata, &line, &nline);
	lnum = 0;
	for(i=0; i<ninstr; i++) {
		if ((w = vwhere(line, nline, i)) != lnum)
			fprintf(stdout,"%4d", lnum = w);
		if(((unsigned int)instr[i])>127)
			err("instr range");
		fprintf(stdout,"\t%4d\t%s", i, Xmne[instr[i]]);
		if (instr[i] >= ILONG) {
			stoc.c[0] = instr[++i];
			stoc.c[1] = instr[++i];
			word = stoc.s;
			switch(instr[i-2]) {
			case Ifield:
				fprintf(stdout,"\t%s\n", xx[word].aname);
				break;
			case Idown:
				fprintf(stdout,"\t%d\n", word);
				break;
			case Ipushi:
				fprintf(stdout,"\t#%d\n", word);
				break;
			case Ierr:
			case Ierrp:
			case Icall:
			case Ipushs:
				fprintf(stdout,"\t\"%s\"\n", &data[word]);
				break;
			case Ipushn:
				fprintf(stdout,"\t=%ld\n",
					*((long *)&data[word]));
				break;
			default:
				fprintf(stdout,"\t@%d\n", word);
			}
		}
		else
			fprintf(stdout,"\n");
	}
}

static int
putshort(n, iop)
short n;
register FILE *iop;
{
	register char *p = (char *)&n;
	register char *pstop = (char *)(&n + 1);

	for (; p < pstop; p++)
		putc(*p, iop);
}

/* portable version of memcmp() since some systems don't have it */
static int
memcmp(s1, s2, n)
register char *s1, *s2;
register int n;
{
	int diff;

	if (s1 != s2)
		while (--n >= 0)
			if (diff = *s1++ - *s2++)
				return (diff);
	return (0);
}
