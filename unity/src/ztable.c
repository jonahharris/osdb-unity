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

table(argc, argv)
char	*argv[];
int	argc;
{
	int	i,j,nattr1;
	char	*prog, *tblname;
	struct	fmt	xx[MAXATT];
	FILE *udfp2;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	stdbuf = table2 = NULL;
	Dtable2[0] = '\0';
	exitcode = 1;

	if (argc < 4) {
		error(E_GENERAL,"Usage: %s table aname1 atype1 [...] \n", prog);
		return(exitcode);
	}

	if ((tblname = strrchr(argv[1], '/')) == NULL) {
		tblname = argv[1];
	} else {
		++tblname;
	}

	getfile(Dtable2,tblname,0);
	if (chkaccess(Dtable2,00) == 0) {	
		error(E_EXISTS,prog,Dtable2);
		Dtable2[0] = '\0';
		return(exitcode);
	}

	if ((udfp2=fopen(Dtable2,"w")) == NULL) {
		error(E_DESFOPEN,prog,Dtable2);
		return(exitcode);
	}
	if(argc-2 > 2 * MAXATT) {
		error(E_GENERAL, "%s: Too many attributes specified\n", prog);
		return(exitcode);
	}
	for(i=2;i<argc;i+=2) {
		if ((i+1) == argc) {
			error(E_GENERAL,"%s: Attribute given without type\n", prog);
			return(exitcode);
		}
		if (fprintf(udfp2,"%s\t%s\n",argv[i],argv[i+1]) < 0) {
			error(E_DATFWRITE,prog,Dtable2);
			return(exitcode);
		}
	}
	if (fclose(udfp2) == EOF) {
		error(E_DATFWRITE,prog,Dtable2);
		return(exitcode);
	}
	udfp2 = NULL;

	if((nattr1 = mkntbl(prog,tblname,Dtable2,xx,tblname))==ERR)
		return(exitcode);

	for(i=0; i<nattr1; i++)
		for(j=0; j<nattr1; j++)
			if(i!=j && (strcmp(xx[i].aname,xx[j].aname) == 0) ) {
			error(E_GENERAL,"%s: Duplicate attribute name '%s'.\n",
				argv[0],xx[j].aname);
				return(exitcode);
			}

	Dtable2[0] = '\0';
	exitcode = 0;
	return(exitcode);
}
