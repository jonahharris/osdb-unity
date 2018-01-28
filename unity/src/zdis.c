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
static	char *prog;
static	char *nunitydsearch = "UNITYDSEARCH=ucd";
extern	char *malloc(), *strrchr(), *getenv();

extern	char DtmpItable[];
extern	char *packedsuffix;
extern	int  packedclose();
extern	FILE *getdescwtbl();
extern	FILE *packedopen();

extern FILE *utfp1;

dis(argc,argv)
char	*argv[];
int argc;
{
	char Dtable[MAXPATH+4];		/* allow for "/./" or "././" prefix */
	char *Ioption, *p;
	struct fmt xx[MAXATT];
	int i, noption, qoption, rc, exitcode;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}

	DtmpItable[0] = '\0';
	rc = qoption = 0;
	exitcode = 1;
	Ioption = NULL;

	while (  argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		switch(argv[1][1]) {
		case 'I':
			Ioption = &argv[1][2];
			break;
		case 'n':
		case 'q':
			for (i = 1; argv[1][i]; i++)
				switch (argv[1][i]) {
				case 'n':
					++noption;
					break;
				case 'q':
					++qoption;
					break;
				default:
					error(E_BADFLAG,prog,argv[1]);
					return(exitcode);
				}
			break;
		default:
			error(E_BADFLAG,prog,argv[1]);
			return(exitcode);
		}
		argc--;
		argv++;
	}

	if(argc < 2 || argc > 3 ||
		(argc == 3 && *argv[2] != 'I' && *argv[2] != 'D' &&
			strncmp(argv[2],"des",3) != 0 &&
			strcmp(argv[2],"data") != 0)) {
		error(E_GENERAL, "Usage: %s [-n] [-q] [-Itable] table [{Dpath|Itable|des|data}]\n",
			prog);
		return(exitcode);
	}

	if ((!(Ioption)) && (strcmp(argv[1], "-") == 0)) {
		(void) getdescrwtbl(stdin, &Ioption);
	}

	if(argc == 2 || strcmp(argv[2],"data") != 0) {

		if ((!(Ioption)) && (strcmp(argv[1],"-") == 0)) {
			error(E_GENERAL,
				"%s:  No description file specified\n",
				prog);
			return(exitcode);
		}

		/*
		 * Check if request for New UNITY D-file search sequence
		 * and verify, using the opposite logic that mkntbl() uses
		 * to specify the data directory (first), that this is not
		 * such a (data directory) request and that UNITYDSEARCH
		 * is NULL or "" before setting UNITYDSEARCH to the search
		 * order that New Unity defaults to using when UNITYDSEARCH
		 * is NULL or "".
		 */
		if ((noption) &&
		   (((p = getenv("UNITYDSEARCH")) == NULL) ||
		    ((*p == '\0') &&
		     ((Ioption == NULL) || (*Ioption == '\0') ||
		      ((strncmp(Ioption,"/./",3) != 0) &&
		       (strncmp(Ioption,"././",4) != 0) &&
		       (strcmp(argv[1],Ioption) != 0)))))) {
			putenv(nunitydsearch);
		}

		/* need to get description file name and print */
		if (mkntbl(prog,argv[1],Dtable,xx,Ioption) < 0) {
			if (DtmpItable[0]) {
				(void) unlink(DtmpItable);
				DtmpItable[0] = '\0';
			}
			return(exitcode);
		}

		if ((argc > 2) && (*argv[2] == 'D')) {
			if (DtmpItable[0] == '\0')
				rc = fprintf(stdout, "%s\n", Dtable);
		} else if ((argc > 2) && (*argv[2] == 'I')) {
			if ((p = strrchr(Dtable, '/')) == NULL) {
				rc = fprintf(stdout, "././%s\n", &Dtable[1]);
			} else {
				char *prefix = "";

				if (Dtable[0] == '/') {
					if (Dtable[1] != '.' || Dtable[2] != '/')
						prefix = "/.";
				} else if (Dtable[0] == '.' && Dtable[1] == '/') {
					if (Dtable[2] != '.' || Dtable[3] != '/')
						prefix = "./";
				} else {
					prefix = "././";
				}
				*++p = '\0';
				rc = fprintf(stdout, "%s%s%s\n", prefix, Dtable, &p[1]);
				*p = '/';
			}
		} else {
			if (!(qoption)) 
				fprintf(stdout, "The table description is:\n");
			else if (argc == 2)
				fprintf(stdout, "%%description\n");

			if (catfn(Dtable,FALSE) == ERR) {
				if (DtmpItable[0]) {
					(void) unlink(DtmpItable);
					DtmpItable[0] = '\0';
				}
				return(exitcode);
			}

			if ((qoption) && (argc == 2))
				rc = fprintf(stdout, "%%enddescription\n");
		}
	}

	if (DtmpItable[0]) {
		/* no longer need temporary description file created by getdescrwtbl() */
		(void) unlink(DtmpItable);
		DtmpItable[0] = '\0';
	}

	/* check for previous errors return by fprintf(3S) */
	if (rc < 0) {
		error(E_GENERAL, "%s: output error\n",prog);
		return(exitcode);
	}

	if (argc == 2 || strcmp(argv[2],"data") == 0) {
		if (!(qoption))
			fprintf(stdout, "The data for the table is:\n");
		if (catfn(argv[1],(packedsuffix != NULL))== ERR)
			return(exitcode);
	}

	exitcode = 0;
	return(exitcode);
}

static int
catfn(file,packed)
char	*file;
int	packed;	/* check for packed table when set */
{
	register int c;
	int	status = 0;

	if(strcmp(file,"-") == 0)
		utfp1 = stdin;
	else {
		if ((utfp1 = fopen(file, "r")) == NULL) {
			if ((packed == FALSE) ||
			   ((utfp1 = packedopen(file)) == NULL)) {
				error(E_GENERAL, "%s: cannot open %s\n",prog,file);
				status = ERR;
				return(status);
			}
		} else {
			packed = FALSE;
		}
	}
	while ((c = getc(utfp1)) != EOF)
		if (putchar(c) == EOF && ferror (stdout)) {
			error(E_GENERAL, "%s: output error\n",prog);
			status = ERR;
			break;
		}

	if (utfp1 != stdin) {
		if (packed) {
			if (status == ERR) {
				/* EOF == FALSE */
				(void)packedclose(utfp1,FALSE);
			} else {
				if (packedclose(utfp1,TRUE) != 0) {
					error(E_PACKREAD, prog, file, packedsuffix);
					status = ERR;
				}
			}
		} else {
			if (fclose(utfp1) != 0) {
				error(E_GENERAL, "%s: close error\n",prog);
				status = ERR;
			}
		}
	}
	utfp1 = NULL;
	return(status);
}
