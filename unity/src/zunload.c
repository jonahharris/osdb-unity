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

extern	char	*strrchr();
extern	char	*table2, Dtable2[],*stdbuf;
static	int	exitcode;

unload(argc,argv)
char	*argv[];
int	argc;
{
	char	*prog;

	if ((prog = strrchr(argv[0],'/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	stdbuf = table2 = NULL;
	Dtable2[0] = '\0';
	exitcode = 1;

	if( argc != 2 && (argc!=4 || strcmp(argv[2],"to")) ) {
		error(E_GENERAL,"Usage: %s table1 [ to table2 ]\n", prog);
		return(exitcode);
	}

	if(argc == 4) {
		table2 = argv[4];
		if(copy(prog,argv[1],argv[3]) == ERR)
			return(exitcode);
	}
	unlink(argv[1]);

	exitcode = 0;
	table2 = NULL;
	return(exitcode);
}
