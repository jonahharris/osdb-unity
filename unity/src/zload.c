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

extern	char	*table2, Dtable2[],*stdbuf;
extern	char	*strrchr();
static	int	exitcode;

load(argc, argv)
int	argc;
char	*argv[];
{

	int	seqno, qoption, errorlimit, errors, records;
	char	*prog, *Ioption;
	char	dtable2[MAXPATH+4];	/* allow for "/./" or "././" prefix */

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}

	stdbuf = table2 = Ioption = NULL;
	Dtable2[0] = '\0';
	errorlimit = 1;
	exitcode = 1;
	qoption = 0;

	while ( argc > 1 && argv[1][0] == '-') {
		switch(argv[1][1]) {
			default:
				error(E_BADFLAG,prog,argv[1]);
				return(exitcode);
			case 'I':
				Ioption = &argv[1][2];
				break;
			case 'q':
				qoption = 1;
				break;
			case 'r':
				if (argv[1][2]) {
					if (((errorlimit = atoi(&argv[1][2])) <= 0) &&
					     (argv[1][2] != '0') && (argv[1][2] != '-')) {
						error(E_GENERAL,
							"%s: Invalid error report limit %s\n",
							prog, argv[1]);
						return(exitcode);
					}
				} else {
					errorlimit = 0;
				}
				break;
		}
		argc--;
		argv++;
	}
	if (argc != 4 || strcmp(argv[2],"to") || (strcmp(argv[3],"-") == 0)) {
		error(E_GENERAL,"Usage: %s [-q] [-rErrorLimit] [-Itable] data-file to table\n",prog);
		return(exitcode);
	}
	if (chkaccess(argv[3],00) == 0) {
		error(E_EXISTS,prog,argv[3]);
		return(exitcode);
	}
	if (Ioption)
	{
		if (strcmp(Ioption,argv[3]) == 0)
		{
			char *Iprefix = "";

			/* Search data directory first since the alternate
			 * table is the same as the data table.
			 */

			if (Ioption[0] == '/') {
				if (Ioption[1] != '.' || Ioption[2] != '/')
					Iprefix = "/.";
			} else if (Ioption[0] == '.' && Ioption[1] == '/') {
				if (Ioption[2] != '.' || Ioption[3] != '/')
					Iprefix = "./";
			} else {
				Iprefix = "././";
			}
			sprintf(dtable2,"%s%s", Iprefix, Ioption);
			Ioption = dtable2;
		}
	} else {
		/*
		 * Do not pass table name with a special prefix
		 * to ncheck() in the table description parameter
		 * so it does not search the data directory first.
		 */
		Ioption = argv[3];
		while ((strncmp(Ioption,"/./",3) == 0) || (strncmp(Ioption,"./",2) == 0))
			Ioption += 2;
	}
	if((seqno = ncheck(argv[1],Ioption,errorlimit,&errors,&records))==ERR) {
		if (records)
			error(E_GENERAL,
				"%s: check record errors (%d) - %d records left in %s\n",
				prog, errors, records, argv[1]);
		else
			error(E_GENERAL,
				"%s: Error - data left untouched in %s\n",
				prog, argv[1]);
		return(exitcode);
	}
	if (copy(prog,argv[1],argv[3]) != 0)
		return(exitcode);

	if(!qoption)
		error(E_GENERAL,"Load successful - %d records loaded.\n",seqno);
	exitcode = 0;
	return(exitcode);
}
