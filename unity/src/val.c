/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdarg.h>
#include <time.h>
#include "db.h"
#include "val.h"

extern char *strcpy(), *strncpy(), *strchr(), *strrchr();
extern char *getenv();
extern void free(), exit();

static char *ntos(), *valloc();
static long ston();

static int  Xdtop;			/* Size of data memory. */
static char *Xdata;			/* Ptr to data memory. */
static int  Xitop;			/* Size of instruction memory. */
static char *Xinstr;			/* Ptr to instruction memory. */
static int  Xpc;			/* Program counter (instr. loc.). */
static int  Xlines;			/* Number of lines in val. table. */
static short *Xline;			/* Line memory (array of locs). */
struct fmt *Xfmt;			/* structure containing data */
static char Errors[MAXERR][ELENGTH+1];	/* errors for single record */
static char **Errorp;
static int *Errcnt;
static char *Xfcn;			/* Name of function being CALLed. */


/**
The following declarations and macros define the stack.
*/
struct stack {
	short Stype;
	union {
		long Snum;
		char *Str;
	} Sdata;
};
struct stack Xst[200];
#define STACKSZ sizeof(Xst)/sizeof(Xst[0])
#define STR	1
#define NUM	2
static int Xsp;			/* Stack pointer. */
static char of[] = "stack overflow";	/* C doesn't pack literals. */
static char uf[] = "stack underflow";

#define Mpops (Xsp<0?(char *)err(uf):\
(Xst[Xsp].Stype==STR?Xst[Xsp--].Sdata.Str:ntos(Xst[Xsp--].Sdata.Snum)))
#define Mpopn (Xsp<0?err(uf):\
(Xst[Xsp].Stype==NUM?Xst[Xsp--].Sdata.Snum:ston(Xst[Xsp--].Sdata.Str)))
#define Mtopn (Xsp<0?err(uf):\
(Xst[Xsp].Stype==NUM?Xst[Xsp].Sdata.Snum:ston(Xst[Xsp].Sdata.Str)))
#define Mpushn(N) (Xsp>=STACKSZ-1?err(of):\
(Xst[++Xsp].Stype=NUM,Xst[Xsp].Sdata.Snum=(N)))
#define Mpushs(S) (Xsp>=STACKSZ-1?(char *)err(of):\
(Xst[++Xsp].Stype=STR,Xst[Xsp].Sdata.Str=(S)))


/**
Macro to get a word from instruction memory.
Used to get operands of long instructions, which
may be at an odd address.
*/
static short Xword;
#define Mword (((char *)&Xword)[0]=Xinstr[++Xpc],\
((char *)&Xword)[1]=Xinstr[++Xpc],Xword)


/**
Macros to handle binary operators.
Variables a, b, x and y are internal to machine().
*/
#define Mbin(X) b=Mpopn;a=Mpopn;Mpushn(a X b);
#define Mcmp(X) y=Mpops;x=Mpops;Mpushn(strcmp(x,y) X 0);vfree(x);vfree(y);

static int err( char *format, ... );

/**
Execution begins here.
*/
val(fmt,errors,errcnt,Wname)
struct	fmt	*fmt;
char	**errors;
int	*errcnt;
char	*Wname;
{
	static int first = 1;

	if (first) {
		first = 0;
		vload(Wname,&Xinstr, &Xitop, &Xdata, &Xdtop, &Xline, &Xlines);
	}
	Xfmt = fmt;
	Errorp = errors;
	Errcnt = errcnt;
	*Errcnt = 0;
	machine();
}


/**
Function to execute instructions.
Currently,
over 99% of the machine's time is spent here.
Any changes made should keep it that way.
The three mach*() functions used to be a single function,
but it was broken up because the newer C compilers seem to
have a more limited capacity than the older ones.
Hence there is some additional overhead in calling mach1() and mach2().
*/
static int
machine()
{
	long a, b;

	Xsp = 0;
	for (Xpc=0; ; Xpc++) {
		switch (Xinstr[Xpc]) {
		case Ihalt:
			return;
		case Icat:
			cat();
			continue;
		case Ior:
			Mbin(||);
			continue;
		case Iand:
			Mbin(&&);
			continue;
		case Ieq:
			Mbin(==);
			continue;
		case Ine:
			Mbin(!=);
			continue;
		case Ilt:
			Mbin(<);
			continue;
		case Ile:
			Mbin(<=);
			continue;
		case Igt:
			Mbin(>);
			continue;
		case Ige:
			Mbin(>=);
			continue;
		case Iadd:
		case Isub:
		case Imul:
		case Idiv:
		case Ileq:
		case Ilne:
		case Illt:
		case Ille:
		case Ilgt:
		case Ilge:
		case Ineg:
		case Inot:
			mach1(Xinstr[Xpc]);
			continue;
		case Idown:
		case Ierr:
		case Ierrp:
		case Ipushn:
		case Ifield:
		case Ipushs:
		case Ireg:
		case Ipushi:
		case Icall:
			mach2(Xinstr[Xpc]);
			continue;
		default:
			err("bad instr: %d", Xinstr[Xpc]);
		}
	}
}

static int
mach1(instr)
register int instr;
{
	register char *x, *y;
	long a, b;

	switch (instr) {
	case Iadd:
		Mbin(+);
		return;
	case Isub:
		Mbin(-);
		return;
	case Imul:
		Mbin(*);
		return;
	case Idiv:
		Mbin(/);
		return;
	case Ileq:
		Mcmp(==);
		return;
	case Ilne:
		Mcmp(!=);
		return;
	case Illt:
		Mcmp(<);
		return;
	case Ille:
		Mcmp(<=);
		return;
	case Ilgt:
		Mcmp(>);
		return;
	case Ilge:
		Mcmp(>=);
		return;
	case Ineg:
		a=Mpopn;
		Mpushn(-a);
		return;
	case Inot:
		a=Mpopn;
		Mpushn(a==0);
		return;
	default:
		err("bad instr: %d", Xinstr[Xpc]);
	}
}

static int
mach2(instr)
register int instr;
{
	register int x;
	register char *y;
	union {
		long l;
		char c[ sizeof( long ) ];
	} val;

	switch (instr) {
	case Idown:
		if (!Mpopn)
			Xpc = Mword-1;
		else
			Xpc += 2;
		return;
	case Ierr:
		if (!Mpopn)
			invalid(&Xdata[Mword]);
		else
			Xpc += 2;
		return;
	case Ierrp:
		if (!Mtopn)
			invalid(&Xdata[Mword]);
		else
			Xpc += 2;
		return;
	case Ipushn:
		y = &Xdata[Mword];
		val.c[0] = y[0];
		val.c[1] = y[1];
		val.c[2] = y[2];
		val.c[3] = y[3];
		Mpushn( val.l );
		return;
	case Ifield:
		Mpushs(Xfmt[Mword].val);
		return;
	case Ipushs:
		Mpushs(&Xdata[Mword]);
		return;
	case Ireg:
		x = regex(&Xdata[Mword], y = Mpops);
		vfree(y);
		Mpushn(x != 0);
		return;
	case Ipushi:
		Mpushn(Mword);
		return;


	case Icall:
		Xfcn = y = &Xdata[Mword];
		if (equal(y, "substr"))
			fsubstr();
		else if (equal(y, "size"))
			fsize();
		else if (equal(y, "tblsearch"))
			ftblsearch();
		else if (equal(y, "tblsrch"))
			ftblsearch();
		else if (equal(y, "print"))
			fprint();
		else if (equal(y, "date"))
			fdate();
		else if (equal(y, "time"))
			ftime();
		else if (equal(y, "getenv"))
			fgetenv();
		else if (equal(y, "chkaccess"))
			faccess();
		else if (equal(y, "index"))
			findex();
		else if (equal(y, "rindex"))
			frindex();
		else if (equal(y, "system"))
			fsystem();
		else if (equal(y, "pipe"))
			fpipe();
		else err("unknown fcn: %s", y);
		return;
	default:
		err("bad instr: %d", Xinstr[Xpc]);
	}
}


/**
Function for the CAT instruction.
The machine isn't as well suited for this type
of string operation as it could be,
but we do the best we can without getting
too complicated.
It is expensive to determine the size of the strings
to be concatenated,
so we guess that an allocation of THRESH will do the job.
If this guess is wrong we guess 2*THRESH and try again,
and so on.
If valloc() worked differently this wouldn't be necessary;
we could obtain the starting address,
write the string and then tell valloc() how big it was.
If CAT turns out to be a big time-consumer, changes along
these lines should be made.
*/
#define THRESH 64

static int
cat()
{
	register char *s, *r, *rstop;
	char *x, *y, *a;
	int amt;

	y = Mpops;
	x = Mpops;
	for (amt=THRESH; ; amt+=THRESH) {
		r = (a = valloc(amt)) - 1;
		rstop = r + amt;
		s = x - 1;
		while (r<rstop && (*++r = *++s));
		if (*r == '\0') {
			r--;
			s = y - 1;
			while (r<rstop && (*++r = *++s));
			if (*r == '\0')
				break;
		}
		vfree(a);
	}
	vfree(x);
	vfree(y);
	Mpushs(a);
}


/**
The next group of functions implement validation language functions.
In order to add a new function one must:
1. Add a test for it in machine() under the CALL instruction.
2. Add a function for it here.
3. Update the manual page.
*/

static int
fprint()
{
	register char *s;

	argchk(1, 1);
	s = Mpops;
	printf("%s\n", s);
	vfree(s);
	Mpushn(1);
}

static int
fsystem()
{
	register char *s;
	register int rc;

	argchk(1, 1);
	s = Mpops;
	rc = system( s);
	vfree(s);
	Mpushn(rc);
}

static int
fgetenv()
{
	register char *var, *value, *rvalue;

	argchk(1, 1);
	var = Mpops;
	value = getenv( var );
	if ( (value == NULL) || (strlen(value)==0) ) {
		rvalue = valloc(1);
		rvalue[0] = '\0';
	} else {
		rvalue = valloc( strlen(value) + 1);
		strcpy( rvalue, value );
	}
	vfree( var );
	Mpushs( rvalue );
}

static int
fpipe()
{
	register char *cmd, *result;
	register FILE *fp;

	argchk(1, 1);
	cmd = Mpops;
	fp = popen( cmd, "r" );
	result = valloc( 256 );
	result[0] = '\0';
	fgets( result, 256, fp );
	fclose( fp );
	vfree( cmd );
	Mpushs( result );
}

static int
faccess()
{
	register char *file;
	register int mode, rc;

	argchk(2, 2);
	mode = Mpopn;
	if ((mode > 7) || (mode < 0) )
		err( "bad chkaccess mode %d", mode );
	file = Mpops;
	rc = chkaccess( file, mode );
	vfree( file );
	Mpushn( rc );
}

static int
findex()
{
	register char *string, *search, *match;
	register int pos;

	argchk(2, 2);
	search = Mpops;
	string = Mpops;
	match = strchr( string, *search );
	if ( match == NULL )
		pos = 0;
	else	
		pos = match - string + 1;
	vfree( string );
	vfree( search );
	Mpushn( pos );
}

static int
frindex()
{
	register char *string, *search, *match;
	register int pos;

	argchk(2, 2);
	search = Mpops;
	string = Mpops;
	match = strrchr( string, *search );
	if ( match == NULL )
		pos = 0;
	else	
		pos = match - string + 1;
	vfree( string );
	vfree( search );
	Mpushn( pos );
}

static int
fsubstr()
{
	register int len;
	char *str;
	int strsize, start;
	char *sub;

	if (argchk(2, 3) == 2)
		len = 32767;
	else
		len = Mpopn;
	start = Mpopn;
	str = Mpops;

#if 0
	if ( len < 0)
		err("bad 3rd arg to substr: %d", len);
	if (start < 1)
		err("bad 2nd arg to substr: %d", start);
#else
	if ( len <= 0 || start < 1 )
	{
		/* Result is an empty string */
		vfree( str );
		sub = valloc( 1 );
		sub[0] = '\0';
		Mpushs( sub );
		return;
	}
#endif
	strsize = strlen(str);
	start = min(start, strsize+1);
	len = min(len, strsize-start+1);
	sub = valloc(len+1);

	/* move value */
	strncpy( sub, &str[start-1], len );
	sub[len] = '\0';

	vfree(str);
	Mpushs(sub);
}

static int
fsize()
{
	register char *s;

	argchk(1, 1);
	s = Mpops;
	Mpushn(strlen(s));
	vfree(s);
}

static int
fdate()
{
	static char d[7], t[7];

	argchk(0, 0);
	dttm(d, t);
	Mpushs(d);
}

static int
ftime()
{
	static char d[7], t[7];

	argchk(0, 0);
	dttm(d, t);
	Mpushs(t);
}

static int
ftblsearch()
{
	register char *a1, *a2, *a3;
	char *a4;
	char *tblsrch();

	argchk(4, 4);
	a4 = Mpops;
	a3 = Mpops;
	a2 = Mpops;
	a1 = Mpops;

	Mpushs(tblsrch(a1, a2, a3, a4));

	vfree(a1);
	vfree(a2);
	vfree(a3);
	vfree(a4);
}

/**
Function used by CALL functions to check number of arguments.
Actual number must be greater than or equal to mn and
less than or equal to mx.
Actual number is returned.
Variable Xfcn is set up in machine().
*/
static int
argchk(mn, mx)
register mn, mx;
{
	long a;

	if ((a=Mpopn)<mn || a>mx)
		err("wrong number of args to fcn \"%s\"", Xfcn);
	return(a);
}


/**
Function called from machine() to issue error codes.
*/
static
invalid(e)
char e[];
{
	if(*Errcnt >= MAXERR)
		return;
	strncpy(Errors[*Errcnt],e,ELENGTH);
	Errors[*Errcnt][ELENGTH] = '\0';	/* make sure null terminated */
	*Errorp++ = Errors[*Errcnt];
	*Errcnt = *Errcnt + 1;
}

/**
Functions to convert from number to string (ntos()) and
from string to number (ston()).
Ntos() can be called twice before it reuses its static buffer,
which is just great for binary operators.
If it is used elsewhere caution should be taken;
the safest thing is to copy the returned string to
a caller-controlled buffer.
*/
static char *
ntos(n)
long n;
{
	static char buf[2][25], bufsw;

	bufsw = !bufsw;
	sprintf(buf[bufsw], "%ld", n);
	return(buf[bufsw]);
}


static long
ston(string)
char string[];
{
	long n;
	register char *s;

	n = 0;
	s = string;
	while(*s >= '0' && *s <= '9')
		n = 10 * n + *s++ - '0';
	if (*s) {
		err("conversion of \"%s\"", string);
	}
	vfree(string);
	return(n);
}


/**
The next group of functions handle errors.
They fall into three categories:

1. Errors detected by this file or other files specific to
   the validation module from which the machine can recover sufficiently
   to at least accept another set of field-value pairs,
   even though the new set may result in the same error.
   Function err() is called for these.
2. Errors from which the machine can't recover or is afraid to recover.
   Function ftlerr() is called for these.
3. Errors detected by the utility library, for which the ftlerr()
   here subsumes the one in the utility library.
*/

static int
/*VARARGS1*/
err( char *format, ... )
{
	va_list args;
	char s[100];

	va_start( args, format );
	vsprintf( s, format, args );
	va_end( args );

	error(E_GENERAL,"VAL ERROR: %s\n", s);
	sprintf(s, "%d", vwhere(Xline, Xlines, Xpc));
	error(E_GENERAL,"VALLINE: %s\n",s);
	preturn();	/* return to calling routine via longjmp() */
}

/*VARARGS1*/
ftlerr( char *format, ... )
{
	va_list args;
	char msg[200];

	if ( format && *format )
	{
		va_start( args, format );
		vsprintf( msg, format, args );
		va_end( args );

		error(E_GENERAL, "VAL ERROR: %s\n", msg);
	}
	exit(1);
}


/**
Interface functions to malloc() and free()
that allow extraneous free()s to be ignored.
*/
static char *Xvlow, *Xvhi;

static char *
valloc(n)
register int n;
{
	register char *a;
	char *malloc();

	if((a = malloc((unsigned)n)) == 0)
		ftlerr("error in malloc");
	if (Xvlow == 0)
		Xvlow = a;
	if (a > Xvhi)
		Xvhi = a;
	return(a);
}

static int
vfree(a)
register char *a;
{
	if (Xvlow <= a && a <= Xvhi)
		free(a);
}

static int
dttm(d,t)
register char *d, *t;
{
	/**
	Function to store date in first arg (YYMMDD) and
	time in second arg (HHMMSS).
	*/
	long tm,time();
	register struct tm *v;
/*	char mo[3], da[3], ho[3], mi[3], se[3];
 *	char *two();
 */
	time(&tm);
	v = localtime(&tm);
/* sprintf(d,"%d%s%s",v->tm_year,two(v->tm_mon+1,mo),two(v->tm_mday,da)); */
/* sprintf(t,"%s%s%s",two(v->tm_hour,ho),two(v->tm_min,mi),two(v->tm_sec,se)); */
	sprintf( d,"%d%02d%02d", v->tm_year, v->tm_mon+1, v->tm_mday );
	sprintf( t,"%02d%02d%02d", v->tm_hour, v->tm_min, v->tm_sec );
}
/*
static char *
two(n,r)
register int n;
register char *r;
{
	sprintf(r, "%02d", n);
	return(r);
}
*/
