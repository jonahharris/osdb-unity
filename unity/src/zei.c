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

ei(argc,argv)
char	*argv[];
int argc;
{
	int	exitcode;
	char	*prog;
	char	Dtable[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	ABname[MAXPATH+4];

	if ((prog = strrchr(argv[0],'/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	exitcode = 1;

	if(argc != 4 || strcmp(argv[2],"from")) {
		error(E_GENERAL,"Usage: %s aname from table\n",prog);
		return(exitcode);
	}

	getfile(Dtable,argv[3],0);
	sprintf(ABname,"%s.%s",Dtable,argv[1]);
	ABname[0] = 'A';
	unlink(ABname);
	ABname[0] = 'B';
	unlink(ABname);
	exitcode = 0;
	return(exitcode);
}
