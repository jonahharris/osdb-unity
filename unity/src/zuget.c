/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*	this shell gets a unity file
	it gets the modified sccs file for the data and unloads it
	gets the description file
*/

#include <stdio.h>
#include "db.h"

extern FILE	*popen();
extern char *dirname();

#define RECTERM '^'
extern FILE	*utfp1;
extern char	Dtable2[], *table2;
extern char	*strcat(), *strcpy(), *strrchr();

uget(argc,argv)
int argc;
char *argv[];
{
	int c, chmodflg, i, j, k, goption;
	char	cmd[512];
	char	path[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	t2[MAXPATH+4];
	char	opt[100];
	char	*prog;
	FILE	*utfp0;

	if ((prog = strrchr(argv[0],'/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}

	if(argc == 1) {
		error(E_GENERAL, "Usage: %s %s \\\n\t%s\n", prog,
			"[-rSID] [-ccutoff] [-ilist] [-x[list] [-aseq-no.] [-k] [-e]",
			"[-s] [-b] [-g] [-t] file ...");
		return(1);
	}

	opt[0] = '\0';
	goption = 0;
	chmodflg=0;
	for(i=1; i < argc; i++) {
		if(argv[i][0] != '-')
			continue;

		/* don't allow -p, -l, -m, -n, -w options */
		switch(argv[i][1]) {
		case 'g':
			goption = 1;
		case 'e':
		case 'k':
			if(argv[i][1] != 'g')
				chmodflg=1;
			/* fall through */
		case 'r':
		case 'c':
		case 'i':
		case 'x':
		case 'a':
		case 's':
		case 'b':
		case 't':
			strcat(opt," \"");
			strcat(opt,argv[i]);
			strcat(opt,"\"");
			break;
		default:
			error(E_GENERAL,"%s: unknown option %s\n",prog,argv[i]);
			return(1);
		}
	}

	for(i=1; i < argc; i++) {
		if(argv[i][0] == '-')
			continue;

		/* get simple file name without s. */
		for(k=j=0; argv[i][k]; k++) {
			if(argv[i][k] == '/') {
				j = k + 1;
			}
		}
		if(argv[i][j] == 's' && argv[i][j+1] == '.')
			j = j + 2;	/* take off s. if any */
		for(k=0; argv[i][j] != '\0'; j++,k++)
			t2[k] = argv[i][j];
		t2[k] = '\0';
		strcpy(path,argv[i]);
		(void)dirname(path);

		if(chkaccess(t2,02) == 0) {
			error(E_GENERAL,"ERROR: writable '%s' exists.\n",
				t2);
			return(1);
		}
		unlink(t2);
		table2 = t2;	/* set up in case of break */

		strcpy(Dtable2,"D");
		strcat(Dtable2,table2);
		if(chkaccess(Dtable2,02) == 0) {
			error(E_GENERAL,"ERROR: writable '%s' exists.\n",
				Dtable2);
			Dtable2[0] = '\0';
			table2 = NULL;
			return(1);
		}
		unlink(Dtable2);

	
		sprintf(cmd,"get %s -p %s/s.%s.s", opt, path, table2);
		if(!goption) {
			if((utfp1 = fopen(table2,"w")) == NULL) {
				error(E_DATFWRITE,prog,table2);
				return(1);
			}
			if((utfp0 = popen(cmd,"r")) == NULL) {
				error(E_GENERAL,"%s:  get failed\n",prog);
				fclose(utfp1); unlink(table2);
				return(1);
			}
			while((c=getc(utfp0))!=EOF) {
				if(c=='\n')
					continue;
				if (c==RECTERM)
					c='\n';
				putc(c,utfp1);
			}
			fclose(utfp1);
			pclose(utfp0);
		}
		else {
			if(system(cmd) != 0) {
				error(E_GENERAL,"%s:  Aborted\n",prog);
				return(1);
			}
		}
		sprintf(cmd,"get %s %s/s.D%s", opt, path, table2);
		if(system(cmd) != 0) {
			error(E_GENERAL,"%s:  Aborted\n",prog);
			unlink(table2);
			return(1);
		}
		if(chmodflg) {
			chmod(table2,0666);
			chmod(Dtable2,0666);
		}
		else {
			chmod(table2,0444);
			chmod(Dtable2,0444);
		}
	}
	table2 = NULL;
	Dtable2[0] = '\0';
	return(0);
}
