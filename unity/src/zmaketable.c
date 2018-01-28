/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include "db.h"

extern char *strrchr();

extern char Dtable2[];

maketable(argc,argv)
int argc;
char *argv[];
{
	char *prog, *table, cmd[80];
	char Dtable1[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char Vtable1[MAXPATH+4], Etable1[MAXPATH+4], Wtable1[MAXPATH+4];
	FILE *utfp1;
	int exitcode, voption;
	struct fmt xx[MAXATT];

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	exitcode = 1;
	voption = 0;
	while(argc > 1 && argv[1][0] == '-') {
		switch(argv[1][1]) {
			default:
				error(E_BADFLAG,prog,argv[1]);
				return(exitcode);
			case 'v':
				voption = 1;
				break;
		}
		argc--;
		argv++;
	}
	if(argc == 1 ) {
		error(E_GENERAL,"Usage: %s [-v] table\n",prog);
		return(exitcode);
	}

	if ((table = strrchr(argv[1], '/')) == NULL) {
		table = argv[1];
	} else {
		++table;
	}

	getfile(Dtable1,table,0);
	
	if(chkaccess(Dtable1,00) == 0) {
		error(E_GENERAL,"%s: Descriptor file %s already exists.\n",prog,
			Dtable1);
		return(exitcode);
	}
	
	sprintf(Dtable2,"D%s",Dtable1);
	if((utfp1 = fopen(Dtable2,"w")) == NULL) {
		error(E_DATFWRITE,prog,Dtable2);
		return(exitcode);
	}
	fprintf(utfp1,"ATTRIBUTE\tt\\t\t11c\tAttribute Name\n");
	fprintf(utfp1,"TYPE\tt\\t\t3c\tAttribute Type/Delimiter\n");
	fprintf(utfp1,"PRINT_FORM\tt\\t\t3r\tPrint Format\n");
	fprintf(utfp1,"VERBOSENAME\tt\\n\t3r\tVerbose Name\n");
	if (fclose(utfp1) == EOF) {
		error(E_DATFWRITE,prog,Dtable2);
		return(exitcode);
	}
	utfp1 = NULL;
	if(voption) {
		strcpy(Vtable1,Dtable2);
		Vtable1[0] = 'V';
		if((utfp1 = fopen(Vtable1,"w")) == NULL) {
			error(E_DATFWRITE,prog,Vtable1);
			return(exitcode);
		}

		fprintf(utfp1, "ATTRIBUTE %% \"[a-zA-Z][a-zA-Z0-9_]{0,10}\"\t\t\te1\n");
		fprintf(utfp1, "!(TYPE %% \"w[0-9]{1,3}\")\n");
		fprintf(utfp1, "\tTYPE %% \"t.{1,2}\"\t\t\t\t\te2\n");
		fprintf(utfp1, "PRINT_FORM != \"\"\n");
		fprintf(utfp1, "\tPRINT_FORM %% \"[0-9]{1,4}[lrc]{0,1}\"\t\t\te3\n");
		fprintf(utfp1, "VERBOSENAME != \"\"\n");
		fprintf(utfp1, "\tVERBOSENAME %% \"[^\t]{1,40}\"\t\t\te4\n");
		if (fclose(utfp1) == EOF) {
			error(E_DATFWRITE,prog,Vtable1);
			unlink(Vtable1);
			return(exitcode);
		}
		utfp1 = NULL;

		strcpy(Etable1,Dtable2);
		Etable1[0] = 'E';
		if((utfp1 = fopen(Etable1,"w")) == NULL) {
			error(E_DATFWRITE,prog,Etable1);
			unlink(Vtable1);
			return(exitcode);
		}
		fprintf(utfp1,
			"e1\tInvalid attribute name.\n");
		fprintf(utfp1,
			"e2\tType must be \"w<width>\" or \"t<terminator>\".\n");
		fprintf(utfp1,
			"e3\tPrint format must be \"<width>{lcw}\"\n");
		fprintf(utfp1,
			"e4\tVerbose name must be less than 40 characters with no tabs.\n");
		if (fclose(utfp1) == EOF) {
			error(E_DATFWRITE,prog,Etable1);
			unlink(Etable1);
			unlink(Vtable1);
			return(exitcode);
		}
		utfp1 = NULL;

		strcpy(Wtable1,Vtable1);
		Wtable1[0] = 'W';

		sprintf(cmd,"uenter -v -u -n -c -I././%s in %s",Dtable1,Dtable1);
	}
	else
		sprintf(cmd,"uenter -u -n -c -I././%s in %s",Dtable1,Dtable1);
	if (system(cmd) != 0) {
		error(E_GENERAL, "%s: Data entry failed\n", prog);
	} else {
		int	i, j, nattr1, retcode;
		if ((nattr1 = mkntbl(prog,table,Dtable1,xx,table)) != ERR) {
			retcode = 0;
			for (i=0; i<nattr1; i++)
				for (j=i+1; j<nattr1; j++)
					if (strcmp(xx[i].aname,xx[j].aname) == 0) {
						error(E_GENERAL,
							"%s: Duplicate attribute name '%s'.\n",
							prog, xx[i].aname);
						retcode = 1;
					}
			exitcode = retcode;
		}
	}
	unlink(Dtable2);
	if(voption) {
		unlink(Etable1);
		unlink(Vtable1);
		unlink(Wtable1);
	}
	return(exitcode);
}
