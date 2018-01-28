/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*	uadmin does an admin on a unity data and description file
	the data file is first transformed to a file appropriate for
	sccs (e.g., data lines > 500 split) and named ${file}.s
	the sccs files created are $path/s.D${file} $path/s.${file}.s

	udelta adds a new delta  to sccs files for a unity file.
	both the data fie and description files are updated.
*/

#include <stdio.h>
#include "db.h"

extern FILE	*popen();
extern char *dirname();

#define RECTERM '^'
#define LINEMAX 500
extern FILE	*utfp1,*utfp2;
extern char	Dtable2[];
extern	char	*strcpy(), *strcat(), *strrchr(), *malloc();

static	FILE	*utfp0;	/* can't do an fclose on this */
static	long	location;
static	int	exitcode, admin;
static	int	count, lines;
static	char	*prog;
static	char	opt[100];

uadmin(argc,argv)
int argc;
char *argv[];
{
	int i;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}

	if(argc == 1) {
		error(E_GENERAL, "Usage: %s %s \\\n\t%s\n", prog,
			"[-rrel] [-t[name]] [-fflag[flag-val]] [-alogin]",
			"[-m[mrlist]] [-y[comment]] file ...");
		return(1);
	}

	opt[0] = '\0';
	for(i=1; i < argc; i++) {
		if(argv[i][0] != '-')
			continue;
		switch(argv[i][1]) {
		case 'r':
		case 't':
		case 'f':
		case 'a':
		case 'm':
		case 'y':
			strcat(opt," \"");
			strcat(opt, argv[i]);
			strcat(opt, "\"");
			break;
		default:
			error(E_GENERAL,"%s:  Unknown option %s",
				prog,argv[i]);
			return(1);
		}
	}
	admin = 1;
	return(update(argc,argv));
}

udelta(argc,argv)
int argc;
char *argv[];
{
	int	i;

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}

	if(argc == 1) {
		error(E_GENERAL, "Usage: %s %s \\\n\t%s\n", prog,
			"[-rSID] [-s] [-n] [-glist] [-m[mrlist]] [-y[comment]]",
			"[-p] file ...");
		return(1);
	}

	opt[0] = '\0';
	for(i=1; i < argc; i++) {
		if(argv[i][0] != '-')
			continue;
		switch(argv[i][1]) {
		case 'r':
		case 's':
		case 'n':
		case 'g':
		case 'm':
		case 'p':
		case 'y':
			strcat(opt," \"");
			strcat(opt, argv[i]);
			strcat(opt, "\"");
			break;
		default:
			error(E_GENERAL,"%s:  Unknown option %s",
				prog,argv[i]);
			return(1);
		}
	}
	admin = 0;
	return(update(argc,argv));
}

static
update(argc,argv)
int argc;
char *argv[];
{
	char	cmd[512];
	char	path[MAXPATH+4];
	char	table1[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	int	nattr1, i, j, k;
	char	Dtable1[MAXPATH+4];
	struct	fmt xx[MAXATT];

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
			table1[k] = argv[i][j];
		table1[k] = '\0';
		strcpy(path,argv[i]);
		(void)dirname(path);

		if(chkaccess(table1,04) != 0) {
			error(E_DATFOPEN,prog,table1);
			return(1);
		}
		strcpy(Dtable1,"D");
		strcat(Dtable1,table1);
		if(chkaccess(Dtable1,04) != 0) {
			error(E_DESFOPEN,prog,Dtable1);
			return(1);
		}

		/* Dtable2 gets cleaned up by uclean() */
		strcpy(Dtable2,table1);
		strcat(Dtable2,".s");
		if(chkaccess(Dtable2,00) == 0) {
			error(E_GENERAL,"%s: %s already exists\n", prog,
				Dtable2);
			Dtable2[0] = '\0';	/* Don't remove */
			return(1);
		}

		if(!admin) {
			/* for udelta - s. files must exist */
			sprintf(cmd,"%s/s.%s.s",path,table1);
			if(chkaccess(cmd,04) != 0) {
				error(E_GENERAL,"%s: Cannot read %s\n",
					prog, cmd);
				return(1);
			}
			sprintf(cmd,"%s/s.D%s",path,table1);
			if(chkaccess(cmd,04) != 0) {
				error(E_GENERAL,"%s: Cannot read %s\n",
					prog, cmd);
				return(1);
			}
		}
	
		if((nattr1 = mkntbl(prog,table1,Dtable1,xx,table1))==ERR)
			return(exitcode);
	
		if((utfp1 = fopen(table1,"r")) == NULL ) {
			error(E_DATFOPEN,prog,table1);
			return(exitcode);
		}
		if((utfp2 = fopen(Dtable2,"w")) == NULL ) {
			error(E_DATFWRITE,prog,Dtable2);
			fclose(utfp2); utfp2 = NULL;
			return(exitcode);
		}
		sprintf(cmd,"index2 -I%s %s %s | sort", table1, xx[0].aname, table1);
	
		if((utfp0 = popen(cmd,"r")) == NULL) {
			error(E_GENERAL,"%s:  sort failed\n",prog);
			fclose(utfp1); utfp1 = NULL;
			fclose(utfp2); utfp2 = NULL;
			return(exitcode);
		}
		lines=count=1;
		while(getone() != EOF) {	/* get next location */
			fseek(utfp1,location,0);/* seek to record location */
			newrec(); 			/* get record */
			for(i=0;i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
			if(i < nattr1)
				break;
			if(writerec(utfp2,xx,nattr1) != 0) {
				fclose(utfp1); utfp1 = NULL;
				fclose(utfp2); utfp2 = NULL;
				unlink(Dtable2); Dtable2[0] = '\0';
				pclose(utfp0);
				return(exitcode);
			}
		}
		fclose(utfp1); utfp1 = NULL;
		fclose(utfp2); utfp2 = NULL;
		pclose(utfp0);

		if(admin) {
			sprintf(cmd,
			"admin -i%s.s %s %s/s.%s.s && admin -i%s %s %s/s.%s",
			table1, opt, path, table1,
			Dtable1, opt, path, Dtable1);
		}
		else {
			sprintf(cmd,
			"delta %s %s/s.%s.s %s/s.D%s",
			opt, path, table1, path, table1);
		}
	
		if(system(cmd) != 0) {
			error(E_GENERAL,"%s:  Aborted\n", prog);
			unlink(Dtable2); Dtable2[0] = '\0';
			return(1);
		}
		unlink(table1);
		unlink(Dtable1);
		unlink(Dtable2); Dtable2[0] = '\0';
	}
	return(0);
}

static int
getone()
{
	int c;

	/* throw away key value */
	for(;(c=getc(utfp0))!='\01'&& c!=EOF;);
	if(c==EOF)
		return(EOF);

	/* get seek location */
	fscanf(utfp0,"%ld",&location);

	/* get rid of newline */
	while((c=getc(utfp0))!='\n' && c!=EOF);
	return(0);
}

static int
writerec(file, xx, nattr)
FILE *file;
struct fmt xx[];
{
	int i, j, c;

	for(i=0;i<nattr;i++) {
		if(xx[i].flag == WN)
			xx[i].val[xx[i].flen]= '\0';
		for(j=0; (c=xx[i].val[j]) != '\0' ; j++) {
			if(myputc(c,file) != 0)
				return(1);
		}
		if(xx[i].flag == T) {
			if(myputc(xx[i].inf[1],file) != 0)
				return(1);
		}
	}
	if(xx[nattr-1].flag == WN) {
		if(myputc('\n',file) != 0)
			return(1);
	}
	return(0);
}

myputc(c,file)
int c;
FILE *file;
{
	if(c==RECTERM) {
		error(E_GENERAL,
			"%s: Record terminator char %c found in line %d\n",
			prog, c, lines);
		return(1);
	}
	if(count==LINEMAX) {
		if (putc('\n',file) == EOF) {
			error(E_GENERAL,
				"%s: I/O write failure at line %d\n",
				prog, lines);
			return(1);
		}
		count=1;
	}
	if(c=='\n') {
		if (putc(RECTERM,file) == EOF) {
			error(E_GENERAL,
				"%s: I/O write failure at line %d\n",
				prog, lines);
			return(1);
		}
		count=1;
		lines++;
	}
	else
		count++;

	if (putc(c,file) == EOF) {
		error(E_GENERAL,
			"%s: I/O write failure at line %d\n",
			prog, lines);
		return(1);
	}
	return(0);
}
