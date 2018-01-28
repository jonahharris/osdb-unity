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
	ret = dis(argc,argv);
	exit(ret);
}

/* Darwin type needs the symbols defined in uclean.c */
FORCE_UCLEAN() { uclean(); }
