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

	ret = et(argc,argv);
	exit(ret);
}
