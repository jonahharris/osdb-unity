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
extern	char	*getenv();

extern	char	Dpak[],*table2;
extern	char	Dtable2[],pak[];
extern  char    tmptable[], lockfile[];
extern	FILE	*utfp1;
extern	char	Uunames[MAXATT][MAXUNAME+1];	/* in mktbl2.c */
extern	char	*strcpy();
extern	char	*strrchr();
static	int	exitcode, uoption, coption;
static	char	tabbuf[MAXPATH+4];	/* allow for "/./" or "././" prefix */

uenter(argc,argv)
char *argv[];
int argc;
{
	char	*prog,*table,newline,*ed, *Ioption;
	char	Dtable[MAXPATH+4], string[MAXCMD], pid[MAXFILE];
	char	save[MAXFILE];
	char	cmd1[MAXCMD],cmd2[MAXCMD],cmd3[MAXCMD],cmd4[MAXCMD], uopt[4];
	struct	fmt xx[MAXATT];
	int	i,flg,nattr,valopt,qoption;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}

	coption = valopt = qoption = uoption = 0;
	newline = NLCHAR;
	Ioption = table2 = NULL;
	Dtable2[0] = pak[0] = Dpak[0] = uopt[0] = '\0';
	tmptable[0] = lockfile[0] = '\0';
	exitcode = 1;

	while ( argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		switch(argv[1][1]) {
			default:
				error(E_BADFLAG,prog,argv[1]);
				return(exitcode);
			case 'n':
				newline=argv[1][2];
				if ( newline == '\n' )
				{
					error( E_GENERAL, "%s: ERROR: cannot have '\\n' as newline character\n",
						prog );
					return( exitcode );
				}
				break;
			case 'c':
			case 'q':
			case 'u':
			case 'v':
				for (i = 1; argv[1][i]; i++)
					switch (argv[1][i]) {
					case 'v':
						valopt = 1;
						break;
					case 'u':
						uoption = 1;
						strcpy(uopt,"-u");
						break;
					case 'q':
						qoption = 1;
						break;
					case 'c':
						coption = 1;
						break;
					default:
						error(E_BADFLAG,prog,&argv[1][i]);
						return(exitcode);
					}
				break;
			case 'I':
				Ioption = &argv[1][2];
				break;
		}
		argc--;
		argv++;
	}

	if(argc != 3 || strcmp(argv[1],"in")!=0) {
		error(E_GENERAL,
	"Usage: %s [-q] [-Itable] [-v] [-n[nline]] [-u] [-c] in table\n",
			prog);
		return(exitcode);
	}

	table = argv[2];
	umask(002);
	
	if((nattr = mkntbl2(prog,table,Dtable,xx,Ioption))==ERR)
		return(exitcode);

	sprintf(pid,"%d",getpid());
	sprintf(tabbuf,"table2%s",pid);
	table2 = tabbuf;
	sprintf(Dtable2,"D%s",table2);
	sprintf(save,"save%s",pid);
	sprintf(pak,"pak%s",pid);
	sprintf(Dpak,"D%s",pak);

	if(uprompt(prog,pak,xx,nattr) == exitcode)
		return(exitcode);
	if((ed = getenv("ED")) == NULL) {
		if((ed = getenv("EDITOR")) == NULL)
			ed = "ed";
	}
	sprintf(cmd1,"%s %s",ed,pak);
	flg = 1;
	while (flg) {
		error(E_GENERAL,"Enter editor to make changes (y or n) ");
		gets(string);
		switch(string[0]) {
		case 'y':
			system(cmd1);			/* ed pak$$ */
			flg = 0;
			break;
		case 'n':
			flg = 0;
			break;
		default:
			error(E_GENERAL,"Try again!\n");
			break;
		}
	}
	if(newline)
		sprintf(cmd2,"tuple -n'%c' %s -I%s %s into %s",
			newline, uopt, pak, pak, table2);
	else
		sprintf(cmd2,"tuple -n %s -I%s %s into %s",
			uopt, pak, pak, table2);
	if (Ioption) {
		char *Iprefix = "";

		if (strcmp(Ioption,table) == 0)
		{
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
		}
		sprintf(cmd3,"validate -I%s%s %s", Iprefix, Ioption, table2);
		if(qoption)
			sprintf(cmd4,"insert -q -I%s%s in %s from %s",
				Iprefix, Ioption, table, table2);
		else
			sprintf(cmd4,"insert -I%s%s in %s from %s",
				Iprefix, Ioption, table, table2);
	}
	else {
		char *tbl = table;

		/*
		 * Do not pass table name with a special prefix
		 * to validate in the -Itable option so that it
		 * does not search the data directory first.
		 */
		while ((strncmp(tbl,"/./",3) == 0) || (strncmp(tbl,"./",2) == 0))
			tbl += 2;

		sprintf(cmd3,"validate -I%s %s", tbl, table2);

		if(qoption)
			sprintf(cmd4,"insert -q in %s from %s", table, table2);
		else
			sprintf(cmd4,"insert in %s from %s", table, table2);
	}
	if(link(Dtable,Dpak)<0) {
		if(copy(prog,Dtable,Dpak) != 0) {
			error(E_GENERAL,"%s: Cannot copy %s to %s\n",prog,
				Dtable,Dpak);
			return(exitcode);
		}
	}
	while(1) {
		unlink(table2);
		unlink(Dtable2);
		if(system(cmd2) == 0) {	/* tuple -n pak$$ into table2$$ */
			if(!valopt)
				break;
			if(system(cmd3) == 0)
				break;
		}

		flg=1;
		while(flg) {
			error(E_GENERAL,
				"reenter editor to correct problem (y or n)");
			gets(string);
			switch(string[0]) {
			case 'y':
				system(cmd1);		/* ed pak$$ */
				flg=0;
				break;
			case 'n':
			error(E_GENERAL,"Entry aborted, %s unchanged\n",
				table);
				link(pak,save);
			error(E_GENERAL,"edited table is in %s\n",save);
				exitcode = 0;
				return(exitcode);
			default:
				error(E_GENERAL, "Try again!\n");
				break;
			}
		}
	}
	flg=1;
	while(flg){
		error(E_GENERAL, "Make these changes in %s (y or n) ",table);
		gets(string);
		switch(string[0]) {
		case 'y':
			if(system(cmd4)!=0) {
				link(table2,save);
				error(E_GENERAL,"%s: table left in %s\n",
					prog,save);
			}
			flg=0;
			break;
		case 'n':
			error(E_GENERAL,"Changes not made\n");
			flg=0;
			break;
		default:
			error(E_GENERAL,"Try again!\n");
			break;
		}
	}
	exitcode = 0;
	return(exitcode);
}

static int
uprompt(prog,oname,xx,nattr)
char *prog;
char *oname;
struct fmt xx[MAXATT];
int nattr;
{
	char buf[MAXREC];
	int i, j, eof,inserted=0;

	if ((utfp1=fopen(oname,"w"))==NULL) {
		error(E_DATFWRITE,prog,oname);
		return(exitcode);
	}
	error(E_GENERAL,"Terminate insertion with EOF (ctrl-d)\n");
	eof=0;
	while(!eof) {
		error(E_GENERAL,"\nNew Record\n");
		inserted++;
		for(i=0;i<nattr;i++) {
			if(uoption)
				error(E_GENERAL,"%s:\t",Uunames[i]);
			else
				error(E_GENERAL,"%s:\t",xx[i].aname);
			if (gets(buf)==NULL) {
				eof=1;
				break;
			}
			if(!coption) {
				/* check for \ or | at end of line */
				j=strlen(buf);
				while(j>0 &&
					(buf[j-1] == '\\' || buf[j-1] == '|')) {
					/* check if escaped */
					if(j>1 && buf[j-2] == '\\') {
						/* get rid of extra \ */
						buf[j-2] = buf[j-1];
						buf[j-1] = '\0';
						break;
					}
			        	if(buf[j-1] == '\\') {
						error(E_GENERAL,"\t");
						buf[j-1]='\n';
						buf[j++]='\t';
			        	}
					else
						buf[j-1]=' ';
					buf[j] = '\0';
					if(gets(&buf[j])==NULL) {
						eof=1;
						break;
					}
					j=strlen(buf);
				}
			}
			if(i==0) {
				if (fprintf(utfp1,"#####\t%d\n",inserted) < 0) {
					error(E_DATFWRITE,prog,oname);
					return(exitcode);
				}
			}
			if(uoption) {
				if (fprintf(utfp1,"%s\t%s\n",Uunames[i],buf) < 0) {
					error(E_DATFWRITE,prog,oname);
					return(exitcode);
				}
			} else {
				if (fprintf(utfp1,"%s\t%s\n",xx[i].aname,buf) < 0) {
					error(E_DATFWRITE,prog,oname);
					return(exitcode);
				}
			}
			if(eof)
				break;
		}
		if (i!=0) {
			for(;i<nattr;i++) {
				if(uoption) {
					if (fprintf(utfp1,"%s\t\n",Uunames[i]) < 0) {
						error(E_DATFWRITE,prog,oname);
						return(exitcode);
					}
				} else {
					if (fprintf(utfp1,"%s\t\n",xx[i].aname) < 0) {
						error(E_DATFWRITE,prog,oname);
						return(exitcode);
					}
				}
			}
			putc('\n',utfp1);
		}
	}
	error(E_GENERAL,"\n");
	if (fclose(utfp1) == EOF) {
		error(E_DATFWRITE,prog,oname);
		return(exitcode);
	}
	utfp1 = NULL;
	return(0);
}
