/*
 * Copyright (C) 2002 by Lucent Technologies
 *
 */

#define MAXERR 256	/* maximum errors for single record */
#define ELENGTH 10	/* length of error code */

#ifdef __STDC__
#define Mdb(V,C) printf(#V " = %" #C "\n",V)
#else
#define Mdb(V,C) printf("V = %C\n",V)
#endif

#define size(X) (strlen(X)+1)
# define max(a,b) 		(a<b ? b : a)
# define min(a,b) 		(a>b ? b : a)
# define equal(str1,str2)	!strcmp(str1,str2)
# define Mnull(X)		(*((char *)(X)) == '\0')
/**
Referenced by valcmp.y (validation compiler), val.c (validation machine),
and validate.c (validation program)

Op codes should be chosen close enough together
so that switch statement in machine() in val.c can be
compiled as a direct branch.
*/
#define Ihalt	0
#define Icat	1
#define Ior	2
#define Iand	3
#define Ieq	4
#define Ine	5
#define Ileq	6
#define Ilne	7
#define Ilt	8
#define Ile	9
#define Igt	10
#define Ige	11
#define Illt	12
#define Ille	13
#define Ilgt	14
#define Ilge	15
#define Iadd	16
#define Isub	17
#define Imul	18
#define Idiv	19
#define Ineg	20
#define Inot	21
#define ILONG	25		/* Separates shorts from longs. */
#define Idown	25
#define Ierr	26
#define Ierrp	27
#define Ipushn	28
#define Ipushs	29
#define Ipushi	30
#define Ifield	31
#define Icall	32
#define Ireg	33
