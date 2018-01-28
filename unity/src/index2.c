/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

main(argc,argv)
int argc;
char *argv[];
{	
	int ret;

	ret = index2(argc,argv);
	exit(ret);
}


/* Darwin type needs the symbols defined in uclean.c */
FORCE_UCLEAN() { uclean(); }
