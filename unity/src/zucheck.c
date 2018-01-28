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
#include <ctype.h>
#include <string.h>

extern  char    DtmpItable[];
extern	char	*strrchr();

extern  FILE    *getdescrwtbl();

ucheck(argc, argv)
int	argc;
char	*argv[];
{
	int	exitcode;
	int	errorlimit, errors, records, roption;
	int	seqno, qoption;
	char	*prog, *Ioption;

	prog = strrchr(argv[0], '/');
	if (prog == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	qoption = roption = 0;
	exitcode = 1;
	errorlimit = -1;
	Ioption = NULL;
	DtmpItable[0] = '\0';

	while ( argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		switch(argv[1][1]) {
			default:
				error(E_BADFLAG,prog,argv[1]);
				return(exitcode);
			case 'I':
				Ioption = &argv[1][2];
				break;
			case 'n':
			case 'r':
				if (argv[1][2]) {
					if (((errorlimit = atoi(&argv[1][2])) <= 0) &&
					     (argv[1][2] != '0') && (argv[1][2] != '-')) {
						error(E_GENERAL,
							"%s: Invalid error report limit %s\n",
							prog, argv[1]);
					}
					if (!(roption))
						roption = 1;
				} else {
					if (!(roption))
						errorlimit = 0;
					roption = 2;
				}
				break;
			case 'q':
				qoption = 1;
				break;
		}
		argc--;
		argv++;
	}

	if (argc != 2 ) {
		error(E_GENERAL,"Usage: %s [-q] [-rErrorLimit] [-Itable] table\n", prog);
		return(exitcode);
	}
	if (strcmp(argv[1],"-") == 0) {
		if(!Ioption) {
			/*
			 * Check for/get description from the input file.
			 */
			if (getdescrwtbl(stdin, &Ioption) == NULL) {
				error(E_GENERAL,
					"%s: Description file not specified.\n", prog);
				return(exitcode);
			}
		}
	}

	if ((seqno = ncheck(argv[1],Ioption,errorlimit,&errors,&records))==ERR) {
		if ((records) && ( roption > qoption ))
			error(E_GENERAL,"%s: %s errors (%d) - %d records checked.\n",
				prog, argv[1], errors, records);
	} else {
		if (!qoption) {
			if (roption) {
				error(E_GENERAL,"%s: %s ok - %d records checked.\n",
					prog, argv[1], seqno);
			} else {
				error(E_GENERAL,"%s ok - %d records checked.\n",
					argv[1], seqno);
			}
		}
		exitcode = 0;
	}
	
	if (DtmpItable[0]) {
		(void) unlink(DtmpItable);	/* created by getdescrwtbl() */
		DtmpItable[0] = '\0';
	}

	return(exitcode);
}
