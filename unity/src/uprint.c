/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern void setunpackenv();

main(argc,argv)
int argc;
char *argv[];
{	
	int ret;

	setunpackenv();
	ret = uprint(argc,argv);
	uclean();
	exit(ret);
}
