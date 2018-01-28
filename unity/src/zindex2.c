/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* This function reads the table and for each record
   writes the value of aname and the starting location of the
   record in the file to the standard output
*/
#include "db.h"

extern	char	*strrchr();

extern	FILE	*utfp1;
extern	int	end_of_tbl;

static	struct	fmt xx[MAXATT];
static	int	nattr1, endf1;
static	long	loc;

index2(argc, argv)
register int	argc;
register char	*argv[];
{
	int	i, afld1, exitcode;
	char	Dtable[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	*Ioption, *prog;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}

	Ioption = NULL;
	exitcode = 1;

	while (  argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		switch(argv[1][1]) {
		case 'I':
			Ioption = &argv[1][2];
			break;
		default:
			error(E_BADFLAG,prog,argv[1]);
			return(exitcode);
		}
		argc--;
		argv++;
	}
	if ((argc!=3) || (strcmp(argv[2],"-") == 0)) {
		if (argc == 3)
			error(E_GENERAL,
				"%s: \"table\" cannot come from standard input (\"-\")\n",
				prog);
		error(E_GENERAL,"Usage: %s [-Itable] aname table\n",prog);
		return(exitcode);
	}

	if((nattr1=mkntbl(prog,argv[2],Dtable,xx,Ioption))==ERR)
		return(exitcode);

	if (xx[nattr1-1].flag == WN)
		endf1 = 1;
	else
		endf1 = 0;

	if((afld1=setnum(xx,argv[1],nattr1))==ERR) {
		error(E_ILLATTR,prog,argv[1],Dtable);
		return(exitcode);
	}

	if ((utfp1=fopen(argv[2],"r"))==NULL) {
		error(E_DATFOPEN,prog,argv[2]);
		return(exitcode);
	}

	end_of_tbl = 0;

	for(;;) {
		loc=ftell(utfp1);
		newrec();
		for(i=0;i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
		if ((i == 0) && (end_of_tbl)) {
			fclose(utfp1);
			return(0);
		}
		else if (i < nattr1)
		{
			error( E_GENERAL, "%s: Error: record parsing failed on attribute %d of record at seek# %08ld\n",
				prog, i, loc );

			break;
		}

		if (endf1)
			if ((feof(utfp1)) || ((i = getc(utfp1)) == EOF)) {
				error( E_GENERAL,
					"%s: Error: missing newline on record at seek# %08ld\n",
					prog, loc );
				break;
			} else if (i != '\n') {
				error( E_GENERAL,
					"%s: Error: data overrun on record at seek# %08ld\n",
					prog, loc );
				break;
			}
		fprintf(stdout,"%s\01%08ld\n",xx[afld1].val, loc);
	}

	fclose(utfp1);
	return(1);
}
