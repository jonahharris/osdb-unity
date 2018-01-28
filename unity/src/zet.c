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

et(argc,argv)
char	*argv[];
int argc;
{
	int	exitcode;
	char	*prog;
	char	Dtable[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	cmd[MAXCMD];
	int	i;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	exitcode = 1;

	if(argc < 2) {
		error(E_GENERAL,"Usage: %s table [table ...]\n",prog);
		return(exitcode);
	}
	for(i=1;i<argc;i++) {
		unlink(argv[i]);
		getfile(Dtable,argv[i],0);
		unlink(Dtable);
		Dtable[0] = 'V';	/* validation table */
		unlink(Dtable);
		Dtable[0] = 'W';	/* compiled validation table */
		unlink(Dtable);
		Dtable[0] = 'E';	/* error table */
		unlink(Dtable);
		sprintf(cmd,"rm -f A%s.* B%s.*",&Dtable[1],&Dtable[1]);
		system(cmd);
	}
	exitcode = 0;
	return(exitcode);
}
