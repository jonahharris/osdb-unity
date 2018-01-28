/*
 * Copyright (C) 2002 by Lucent Technologies
 *
 */

#ifndef _SEEKVAL
#define _SEEKVAL

/* evaluate query and save seek values */
extern int seeksave( /* struct uquery *query */ );

/* reset seek structures */
extern void seekreset( /* void */ );

/* rewind seek structures before traversing them */
extern void seekrewind( /* void */ );

/* get number of seek values saved */
extern long getseekcnt( /* void */ );

/* see if seek value was saved */
extern int seekfind( /* long seekval */ );

/* save unchanged tuples */
extern long save_unchgd( /* struct uperuseinfo *perptr, char noop, char quiet, int (*updfunc)() */ );

#if 0
/* PSF: gcc chokes on the single quote in the next line 
updfunc's interface is:
*/
	updfunc( perptr, noop, quiet, tplptr, updcnt )
	struct uperuseinfo *perptr;
	char noop;
	char quiet;
	struct utuple *tplptr;
	long updcnt;	/* updated tuple number of tplptr, starting at 1 */
#endif

#endif
