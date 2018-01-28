/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <signal.h>
#include "db.h"

extern int uclean();

static RETSIGTYPE
intr(unused)
int unused;
{
	/* ignore new signals */
	(void) signal(SIGINT,SIG_IGN);
	(void) signal(SIGQUIT,SIG_IGN);
	(void) signal(SIGHUP,SIG_IGN);
	(void) signal(SIGTERM,SIG_IGN);

	uclean();	/* clean temp files */
	exit(1);
}

main(argc,argv)
int argc;
char *argv[];
{	
	int ret;

	if(signal(SIGINT,intr) == SIG_IGN)
		signal(SIGINT,SIG_IGN);
	if(signal(SIGQUIT,intr) == SIG_IGN)
		signal(SIGINT,SIG_IGN);
	if(signal(SIGHUP,intr) == SIG_IGN)
		signal(SIGINT,SIG_IGN);
	if(signal(SIGTERM,intr) == SIG_IGN)
		signal(SIGINT,SIG_IGN);

	ret = udelta(argc,argv);
	uclean();
	exit(ret);
}
