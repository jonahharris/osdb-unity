/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "db.h"
#include <ctype.h>
#ifdef NEED_TOUPPER
#define _tolower( ch )	(isupper( (ch) ) ? tolower( (ch) ) : (ch))
#define _toupper( ch )	(isupper( (ch) ) ? (ch) : toupper( (ch) ))
#endif  /* NEED_TOUPPER */

#ifndef	FALSE
#define	FALSE	0
#endif
#ifndef	TRUE
#define	TRUE	1
#endif

extern double cnvtdate();

extern int step();
static	struct stbl {
	int cval;
	char *sval;
} optbl[] =
{
	/* lexical comparisons */
	{LLT,"llt"},
	{LLE,"lle"},
	{LEQ,"leq"},
	{LNE,"lne"},
	{LGE,"lge"},
	{LGT,"lgt"},

	/* numeric comparisons */
	{LT,"lt"},
	{LE,"le"},
	{EQ,"eq"},
	{NE,"ne"},
	{GE,"ge"},
	{GT,"gt"},

	/* regular expression comparisons */
	{REQ,"req"},
	{RNE,"rne"},

	/* field comparisons */
	{LLT,"fllt"},
	{LLE,"flle"},
	{LEQ,"fleq"},
	{LNE,"flne"},
	{LGE,"flge"},
	{LGT,"flgt"},
	{LT,"flt"},
	{LE,"fle"},
	{EQ,"feq"},
	{NE,"fne"},
	{GE,"fge"},
	{GT,"fgt"},

	/* date and time comparisons */
	{DLT,"dlt"},
	{DLE,"dle"},
	{DEQ,"deq"},
	{DNE,"dne"},
	{DGE,"dge"},
	{DGT,"dgt"},
	{DLT,"fdlt"},
	{DLE,"fdle"},
	{DEQ,"fdeq"},
	{DNE,"fdne"},
	{DGE,"fdge"},
	{DGT,"fdgt"},

	/* date only comparisons */
	{DOLT,"Dlt"},
	{DOLE,"Dle"},
	{DOEQ,"Deq"},
	{DONE,"Dne"},
	{DOGE,"Dge"},
	{DOGT,"Dgt"},
	{DOLT,"fDlt"},
	{DOLE,"fDle"},
	{DOEQ,"fDeq"},
	{DONE,"fDne"},
	{DOGE,"fDge"},
	{DOGT,"fDgt"},

	/* New UNITY compatibility */

	{LT,"nlt"},
	{LE,"nle"},
	{EQ,"neq"},
	{NE,"nne"},
	{GE,"nge"},
	{GT,"ngt"},

	{LT,"fnlt"},
	{LE,"fnle"},
	{EQ,"fneq"},
	{NE,"fnne"},
	{GE,"fnge"},
	{GT,"fngt"},

	{REQ,"~"},
	{RNE,"!~"},

	{REQ,"r="},
	{REQ,"r=="},
	{RNE,"r!="},

	{LEQ,"="},
	{LEQ,"f="},

	/* caseless lexical comparisons */
	{CLT,"clt"},
	{CLE,"cle"},
	{CEQ,"ceq"},
	{CNE,"cne"},
	{CGE,"cge"},
	{CGT,"cgt"},
	{CLT,"fclt"},
	{CLE,"fcle"},
	{CEQ,"fceq"},
	{CNE,"fcne"},
	{CGE,"fcge"},
	{CGT,"fcgt"},

	/* caseless regular expressions ("rc...") */
	{REQ,"rceq"},
	{RNE,"rcne"},
	{REQ,"rc="},
	{REQ,"rc=="},
	{RNE,"rc!="},


	/*
	 * The other five (six) basic algebraic symbols in New UNITY,
	 * which default to lexical comparisons when there is
	 * no "d", "l", or "n" prefix, are being commented out
	 * until the next release of (old) UNITY when they
	 * will be changed from numerical to lexical comparison
	 * operators.  This can be done by removing the comments
	 * here (but leave the operators intact) and removing the
	 * last two "sections" of the table and also remove the warning
	 * messages by removing the extra for loops from getop().
	 *
	{LLT,"<"},
	{LGT,">"},
	{LEQ,"=="},
	{LNE,"!="},
	{LLE,"<="},
	{LGE,">="},
	 *
	 */

	/*
	 *
	{LLT,"f<"},
	{LLE,"f<="},
	{LEQ,"f=="},
	{LNE,"f!="},
	{LGE,"f>="},
	{LGT,"f>"},
	 *
	 */

	{LT,"n<"},
	{GT,"n>"},
	{EQ,"n="},
	{EQ,"n=="},
	{NE,"n!="},
	{LE,"n<="},
	{GE,"n>="},

	{LLT,"l<"},
	{LGT,"l>"},
	{LEQ,"l="},
	{LEQ,"l=="},
	{LNE,"l!="},
	{LLE,"l<="},
	{LGE,"l>="},

	{LT,"fn<"},
	{GT,"fn>"},
	{EQ,"fn="},
	{EQ,"fn=="},
	{NE,"fn!="},
	{LE,"fn<="},
	{GE,"fn>="},

	{LLT,"fl<"},
	{LGT,"fl>"},
	{LEQ,"fl="},
	{LEQ,"fl=="},
	{LNE,"fl!="},
	{LLE,"fl<="},
	{LGE,"fl>="},

	{DLT,"d<"},
	{DGT,"d>"},
	{DEQ,"d="},
	{DEQ,"d=="},
	{DNE,"d!="},
	{DLE,"d<="},
	{DGE,"d>="},

	{DLT,"fd<"},
	{DGT,"fd>"},
	{DEQ,"fd="},
	{DEQ,"fd=="},
	{DNE,"fd!="},
	{DLE,"fd<="},
	{DGE,"fd>="},

	{DOLT,"D<"},
	{DOGT,"D>"},
	{DOEQ,"D="},
	{DOEQ,"D=="},
	{DONE,"D!="},
	{DOLE,"D<="},
	{DOGE,"D>="},

	{DOLT,"fD<"},
	{DOGT,"fD>"},
	{DOEQ,"fD="},
	{DOEQ,"fD=="},
	{DONE,"fD!="},
	{DOLE,"fD<="},
	{DOGE,"fD>="},

	{CLT,"c<"},
	{CGT,"c>"},
	{CEQ,"c="},
	{CEQ,"c=="},
	{CNE,"c!="},
	{CLE,"c<="},
	{CGE,"c>="},

	{CLT,"fc<"},
	{CGT,"fc>"},
	{CEQ,"fc="},
	{CEQ,"fc=="},
	{CNE,"fc!="},
	{CLE,"fc<="},
	{CGE,"fc>="},

	/*
	 * Terminate first for-loop in getop()
	 * and increment pointer to skip over
	 * this empty (null) entry.
	 */
	{1,0},

	/* WARNING: These will change to lexical operators next release. */
	{LT,"<"},
	{LE,"<="},
	{EQ,"=="},
	{NE,"!="},
	{GE,">="},
	{GT,">"},

	/*
	 * Terminate second for-loop in getop()
	 * and increment pointer to skip over
	 * this empty (null) entry.
	 */
	{1,0},

	/* WARNING: These operator will be removed next release. */
	{EQ,"==="},
	{LNE,"!=="},
	{LNE,"!==="},
	{LGE,">>="},
	{LGE,">=="},
	{LGT,">>"},
	{LLT,"<<"},
	{LLE,"<<="},
	{LLE,"<=="},

	/* Terminate all for-loops in getop() */
	{0,0}
	};

getop(s)
char	*s;
{
	struct stbl *p;
	for(p=optbl;p->sval!=0;p++) {
		if(strcmp(s,p->sval)==0) {
			return(p->cval);
		}
	}
	/*
	 * WARNING: This second for-loop must be removed when
	 *	    the second section of optbl[] is removed.
	 */
	for(p+=p->cval;p->sval!=0;p++) {
		if(strcmp(s,p->sval)==0) {
			error(E_GENERAL,"WARNING: Use of operator '%s' will change from numeric to lexical comparison\n\t %s\n",
				s, "in the next UNITY software release to be consistent with New UNITY.");
			return(p->cval);
		}
	}
	/*
	 * WARNING: This third for-loop must be removed when
	 *	    the third section of optbl[] is removed.
	 */
	for(p+=p->cval;p->sval!=0;p++) {
		if(strcmp(s,p->sval)==0) {
			error(E_GENERAL,"WARNING: Use of operator '%s' is not supported and will be removed\n\t %s\n",
				s, "in the next UNITY software release.");
			return(p->cval);
		}
	}
	return(ERR);
}

comp(op,s,v)
int	op;
char	*s;
char	*v;
{
	return(compb(op,s,v,0,0));
}

compb(op,s,v,sibase,vibase)
int	op;
char	*s, *v;
int	sibase, vibase;
{
	double  ss, vv;
	int     syy, smm, sdd, vyy, vmm, vdd, count;

	if (op >= LT && op <= GT) {	/* numeric comparison */
		if (sibase == 0) {
			if(sscanf (s, "%lf", &ss) != 1)
				if(*s == 0)
					ss = 0;		/* Treat NULL as ZERO */
				else
					return(0);	/* not a floating point number */
		} else if (sibase >= 2 && sibase <= 36) {
#ifdef	__STDC__
			/*
			 * use unsigned conversion if available to
			 * avoid problem with maximum unsigned value
			 * since most (all) non-base 10 values are
			 * initially generated from an unsigned int
			 */
			ss = (int) strtoul(s, (char **)NULL, sibase);
#else
			ss = (int) strtol(s, (char **)NULL, sibase);
#endif
		} else if ( sibase == 1 ) {
			ss = strlen(s);
		} else {
			return(0);
		}
		if (vibase == 0) {
			if(sscanf (v, "%lf", &vv) != 1)
				if(*v == 0)
					vv = 0;		/* Treat NULL as ZERO */
				else
					return(0);	/* not a floating point number */
		} else if (vibase >= 2 && vibase <= 36) {
#ifdef	__STDC__
			/*
			 * use unsigned conversion if available to
			 * avoid problem with maximum unsigned value
			 * since most (all) non-base 10 values are
			 * initially generated from an unsigned int
			 */
			vv = (int) strtoul(v, (char **)NULL, vibase);
#else
			vv = (int) strtol(v, (char **)NULL, vibase);
#endif
		} else if ( vibase == 1 ) {
			vv = strlen(v);
		} else {
			return(0);
		}
	}
	else if (op >= DLT && op <= DGT) {	/* date and time comparison */
		ss = cnvtdate( s, TRUE );
		vv = cnvtdate( v, TRUE );
	}
	else if (op >= DOLT && op <= DOGT) {	/* date only comparison */
		ss = cnvtdate( s, FALSE );
		vv = cnvtdate( v, FALSE );
	}

	switch (op) {
	default:
		error (E_GENERAL, "comp error %d %s %s\n", op, s, v);
		return (0);
	case LLT:
		return (strcmp (s, v) < 0);
	case LLE:
		return (strcmp (s, v) <= 0);
	case LEQ:
		return (strcmp (s, v) == 0);
	case LNE:
		return (strcmp (s, v) != 0);
	case LGE:
		return (strcmp (s, v) >= 0);
	case LGT:
		return (strcmp (s, v) > 0);
	case LT:
		return (ss < vv);
	case LE:
		return (ss <= vv);
	case EQ:
		return (ss == vv);
	case NE:
		return (ss != vv);
	case GE:
		return (ss >= vv);
	case GT:
		return (ss > vv);
	case REQ:
		return (step (s, v));
	case RNE:
		return (!step (s, v));
	case DLT:
	case DOLT:
		if (ss == 0 || vv == 0)
			return (0);
		return (ss < vv);
	case DLE:
	case DOLE:
		if (ss == 0 || vv == 0)
			return (0);
		return (ss <= vv);
	case DEQ:
	case DOEQ:
		if (ss == 0 || vv == 0)
			return (0);
		return (ss == vv);
	case DNE:
	case DONE:
		if (ss == 0 || vv == 0)
			return (0);
		return (ss != vv);
	case DGE:
	case DOGE:
		if (ss == 0 || vv == 0)
			return (0);
		return (ss >= vv);
	case DGT:
	case DOGT:
		if (ss == 0 || vv == 0)
			return (0);
		return (ss > vv);
	case CLT:
		return (nocasecmp (s, v) < 0);
	case CLE:
		return (nocasecmp (s, v) <= 0);
	case CEQ:
		return (nocasecmp (s, v) == 0);
	case CNE:
		return (nocasecmp (s, v) != 0);
	case CGE:
		return (nocasecmp (s, v) >= 0);
	case CGT:
		return (nocasecmp (s, v) > 0);
	}
}

#define NOCASE( ch )	(isupper( (ch) ) ? tolower( (ch) ) : (ch))

nocasecmp( str1, str2 )
register char *str1;
register char *str2;
{
	register char ch1, ch2;

	static char uc_upper[ 256 ];	/* characters mapped to upper case */
	static int init_uc_upper;	/* uc_upper[] initialized? */

	if ( ! init_uc_upper ) {
		register int i;
		for ( i = 0; i < 256; i++ ) {
			uc_upper[ i ] = ( islower( i ) ? _toupper( i ) : i );
		}
		/*
		 * Make sure uc_upper[ '\0' ] is set to '\0'
		 */
		uc_upper[ 0 ] = 0;
		init_uc_upper = 1;
	}
	while ( *str1 && *str2 ) {
		/*
		ch1 = NOCASE( *str1 );
		ch2 = NOCASE( *str2 );
		*/
		ch1 = uc_upper[ (unsigned char) *str1 ];
		ch2 = uc_upper[ (unsigned char) *str2 ];
		if ( ch1 != ch2 )
			return( (int)ch1 - (int)ch2 );
		++str1;
		++str2;
	}

	return( (int)(*str1) - (int)(*str2) );
}
