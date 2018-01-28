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

/* this is another version of mktbl() which has an additional parameter:
   an array of character strings which is to hold "user friendly names"
   associated with each attribute.  these are assumed to be stored in
   the description file after the print information, preceded by a tab.
 */
char	Uunames[MAXATT][MAXUNAME+1];

mkntbl2(prog, table, Dtable, fmt, altdesc)
char	*prog;
char	*table;
char	*Dtable;
struct	fmt	*fmt;
char	*altdesc;
{
	return( _mktbl( prog, table, Dtable, fmt, Uunames, MAXATT, altdesc ) );
}

/*
 * mktbl2() is being replaced by mkntbl2() and should be removed sometime
 */

mktbl2(prog, table, Dtable, fmt)
char	*prog;
char	*table;
char	*Dtable;
struct	fmt	*fmt;
{
	return( _mktbl( prog, table, Dtable, fmt, Uunames, MAXATT, NULL ) );
}
