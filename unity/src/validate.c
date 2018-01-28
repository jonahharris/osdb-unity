/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <setjmp.h>
#include "db.h"
#include "val.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <ctype.h>

static int exitcode;
static jmp_buf env;
extern char *strcpy(), *getenv(),  *strcat(), *strrchr();
extern void exit();
extern void setunpackenv();
extern FILE *get2file();
extern FILE *packedopen();
extern int packedclose();
extern char *packedsuffix;
static FILE *packed;
static FILE *utfp3;
static char *prog;
static int noEfile, inerror;
extern int end_of_tbl;

main(argc, argv)
char	**argv;
int	argc;
{
	FILE	*utfp1;
	struct	fmt xx[MAXATT];
	char	Wtable[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	Ename[MAXPATH+4];
	char	*Errors[256], *m, *Ioption;
	int	Errcnt, nattr1, i, record,errflg;
	char	*table;
	char	*tblsrch();

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}
	record = errflg = 0;
	noEfile = inerror = 0;
	packed = (FILE *)NULL;
	Ioption = NULL;
	argv++; argc--;
	exitcode = 1;
	while( argc > 0 && argv[0][0] == '-' && argv[0][1] != '\0') {
		switch(argv[0][1]) {
		case 'f':
			/* this option remains for upward compatibility -
			   see -I option */
			Ioption = argv[1];
			argv++; argc--;
			break;
		case 'I':
			Ioption = &argv[0][2];
			break;
		default:
			error(E_GENERAL,
			"Usage: %s [-Itable] table1\n", prog);
			exit(exitcode);
		}
		argv++; argc--;
	}

	if(argc > 1) {
		error(E_GENERAL, "Usage: %s [-Itable] table1\n",
			prog);
		exit(exitcode);
	}

	setunpackenv();

	/* for upward compatibility, allow no arguments - indicates
	   reading from standard input */
	if(argc == 0 || strcmp(argv[0],"-") == 0) {	/* input from stdin */
		if(!Ioption) {
			error(E_GENERAL,"%s: No description file specified.\n",
				prog);
			error(E_GENERAL,
				"Usage: %s [-Itable] table1\n",
				prog);
			return(exitcode);
		}
		utfp1 = stdin;
		table = "-";
	}
	else {
		if ((utfp1 = fopen(argv[0],"r")) == NULL )
		{ 
			if ((utfp1 = packedopen(argv[0])) == NULL) {
				error(E_DATFOPEN,prog,argv[0]);
				exit(exitcode);
			}
			packed = utfp1;	/* reading packed table */
		}
		table = argv[0];
	}

	/* set up based on input table name and/or alternate description */
	setup(table,Ioption,xx,Wtable,Ename,&nattr1);

	end_of_tbl = 0;

	for(;;) { 
		setjmp(env);	/* save environment in case an error
				   occurs in machine - reset to here
				*/
		inerror = 0;
		record++;	/* tuple count */
		newrec(); 	/* get new tuple */
		for(i=0;i<nattr1 && getrec(utfp1,&xx[i])!=ERR; i++);
		if ((i == 0) && (end_of_tbl))
			break;
		else if (i<nattr1) {
			error( E_GENERAL,
				"%s: Error: record parsing failed on attribute %d of rec# %d in %s\n",
				prog, i, record, table );
			if (packed)
				(void)packedclose(packed,FALSE);
			exit(exitcode);
		}
		if(xx[nattr1-1].flag == WN)
			if ((feof(utfp1)) || ((i = getc(utfp1)) == EOF)) {
				error( E_GENERAL,
					"%s: Error: missing newline on rec# %d in %s\n",
					prog, record, table );
				if (packed)
					(void)packedclose(packed,FALSE);
				exit(exitcode);
			} else if (i != '\n') {
				error( E_GENERAL,
					"%s: Error: data overrun on rec# %d in %s\n",
					prog, record, table );
				if (packed)
					(void)packedclose(packed,FALSE);
				exit(exitcode);
			}

		val(xx,Errors,&Errcnt,Wtable);/* validate tuple */
		if(Errcnt > 0) {
			error(E_GENERAL,"\n");
			/* print out record */
			for(i=0;i<nattr1;i++) {
				if(xx[i].flag == WN)
					xx[i].val[xx[i].flen]= '\0';
				error(E_GENERAL,xx[i].val);
				if(xx[i].flag == T)
					error(E_GENERAL,&xx[i].inf[1]);
			}
			if (xx[nattr1-1].flag == WN)
				error(E_GENERAL,"\n");
		}
		inerror = 1;	/* used in tblsrch */
		for(i=0;i<Errcnt;i++) {
			if(!noEfile &&
			!Mnull(m = tblsrch(Ename,"CODE",Errors[i],"MSG")))
				/* print error message */
				error(E_GENERAL,"Record %d: %s\n",record,m);
			else
				/* print error code */
				error(E_GENERAL,"Record %d: error %s\n",
					record,Errors[i]);
		}
		if(Errcnt > 0)
			errflg = 2;	/* a validation error has occurred */
	}
	if (packed) {
		/* EOF == TRUE */
		if (packedclose(packed,TRUE) != 0) {
			error(E_PACKREAD,prog,table,packedsuffix);
			exit(exitcode);
		}
	}
	exit(errflg);
}

static int
setup(table,altdesc,xx,Wtable,Etable,nattr1)
char *table;		/* name of table - input */
char *altdesc;		/* alternate table description - input */
struct fmt *xx;		/* record structure - input/output */
char *Wtable;		/* name of validation object file - output */
char *Etable;		/* name of error source file - output */
int *nattr1;		/* number of attributes in file - output */
{
	/* do setup for program
	   1. read description file
	   2. compile validation table if necessary
	   3. get error table name if it exists
	*/
	char	Dtable[MAXPATH+4],Vtable[MAXPATH+4],tmp[MAXPATH+4], tmp2[10];
	struct	stat stat1,stat2,stat3;
	int	status, needcmp, ptr, c, i;

	/* take care of description file */
	if((*nattr1 = mkntbl(prog,table,Dtable,xx,altdesc))==ERR)
		exit(exitcode);
	if (stat(Dtable,&stat1) < 0) {
		error(E_GENERAL,"%s: Cannot stat(2) description file %s\n",
			prog,Dtable);
		if (packed)
			(void)packedclose(packed,FALSE);
		exit(exitcode);
	}

	/*
	 * If an alternate descriptor has been given
	 * that includes a "/./" (full) or "././" (relative)
	 * path prefix or if the alternate table name is the
	 * same as the table name then look for the descriptor
	 * in the given directory first.
	 * Otherwise, just stick with the default search.
	 *
	 * NOTE: The following search order check is
	 *       the same one that mkntbl() does!
	 */
	if ((altdesc) && (*altdesc)) {
		if ((strncmp(altdesc, "/./", 3) == 0) ||
		    (strncmp(altdesc, "././", 4) == 0) ||
		    (strcmp(table, altdesc) == 0)) {
			c = 'd';
		} else {
			c = 's';	/* use standard search order */
		}
		table = altdesc;
	} else {
		c = 's';	/* use standard search order */
	}

	if (get2file(c, "V", table, Vtable, (char *)NULL) == NULL ||
	    stat(Vtable,&stat2) < 0) { 
		getfile(Vtable,table,0);
		Vtable[0] = 'V';
		error(E_GENERAL,"%s: Cannot open validation file %s\n",
			prog,Vtable);
		if (packed)
			(void)packedclose(packed,FALSE);
		exit(exitcode);
	}
	needcmp = 0;
	if (get2file(c, "W", table, Wtable, (char *)NULL) == NULL ||
	    stat(Wtable,&stat3) < 0) { 
		/* can't find Wtable - need to create */
		getfile(Wtable, table, 0);
		Wtable[0] = 'W';
		needcmp = 1;
	}

	if(needcmp || stat3.st_mtime < stat2.st_mtime ||
		stat3.st_mtime < stat1.st_mtime) {
		/* validation object file needs updating  - update
		   into temporary and then link to actual so that
		   if break occurs, will not have invalid object file
		*/
		ptr = 0;
		/* set ptr to the character following the last / */
		for (i=0; Wtable[i]; i++) {
			if ( Wtable[i] == '/') {
				ptr = i + 1;
			}
		}
		strcpy(tmp,Wtable);
		tmp[ptr+9] = '\0';
		sprintf(tmp2,"%d",getpid());
		strcat(tmp,tmp2);
		if(fork()==0) {
			error(E_GENERAL, "Compiling validation table\n");
			if (altdesc)
			{
				altdesc -= 2;	/* Include the "-I" prefix */
				execlp("valcmp","valcmp",altdesc,table,tmp,0);
			} else {
				execlp("valcmp","valcmp",table,tmp,0);
			}
			error(E_GENERAL,"%s: Cannot exec %s\n",prog,"valcmp");
			exit(exitcode);
		}
		wait(&status);
		if (status!=0) {
			error(E_GENERAL, "%s: %s failed with status = %d\n",
				prog,"valcmp",status);
			unlink(Wtable);
			unlink(tmp);
			if (packed)
				(void)packedclose(packed,FALSE);
			exit(exitcode);
		}
		error(E_GENERAL,"\n");
		unlink(Wtable);
		link(tmp,Wtable);
		unlink(tmp);
	}
	if (get2file(c, "E", table, Etable, (char *)NULL) == NULL ||
	    stat(Etable,&stat2) < 0) { 
		getfile(Etable,table,0);
		Etable[0] = 'E';
		noEfile = 1;
	}
	return(1);
}

preturn()
{
	/* on error in machine, it calls this function which
	   jumps back to previous sane position in main program
	*/
	longjmp(env,0);
}

static struct fmt zz[MAXATT];
static char myrecbuf[MAXREC],*myrecptr;
static char nullbuf[2] = "";

char *
tblsrch(tbl,keycol, value, retcol)
char *tbl;
char *value;
char *keycol;
char *retcol;
{	
	int endf1, found, nattr1, afld1, afld2, i;
	FILE *packed3;
	char Dtbl[MAXPATH+4];

	packed3 = (FILE *)NULL;
	if((utfp3 = fopen(tbl,"r")) == NULL) {
		if ((inerror) || ((utfp3 = packedopen(tbl)) == NULL))
			return(nullbuf);
		packed3 = utfp3;
	}
	if(inerror) {	/* error table */
		afld1 = 0;
		afld2 = 1;
		nattr1 = 2;
		zz[0].flag = T;
		strcpy(zz[0].aname,"CODE");
		strcpy(zz[0].inf,"t\t");
		zz[1].flag = T;
		strcpy(zz[1].aname,"MSG");
		strcpy(zz[1].inf,"t\n");
	}
	else {
		if((nattr1 = mkntbl(prog,tbl,Dtbl,zz,NULL)) == ERR) {
			if (packed3)
				(void)packedclose(packed3,FALSE);
			else
				fclose(utfp3);
			utfp3 = NULL;
			return(nullbuf);
		}
		if((afld1 = setnum(zz,keycol,nattr1)) == ERR) {
			if (packed3)
				(void)packedclose(packed3,FALSE);
			else
				fclose(utfp3);
			utfp3 = NULL;
			return(nullbuf);
		}
		if((afld2 = setnum(zz,retcol,nattr1)) == ERR) {
			if (packed3)
				(void)packedclose(packed3,FALSE);
			else
				fclose(utfp3);
			utfp3 = NULL;
			return(nullbuf);
		}
	}
	if(zz[nattr1 - 1].flag == WN)
		endf1 = 1;
	else
		endf1 = 0;

	found = 0;
	for(;;) {
		/* get next record */
		myrecptr=myrecbuf;
		for(i=0; i<nattr1 && getmyrec(utfp3,&zz[i])!=ERR; i++);
		if(i<nattr1)
			break;
		if(endf1) 
			while(!feof(utfp3) && getc(utfp3) != '\n');
		if(strcmp(zz[afld1].val,value) == 0) {
			found = 1;
			break;
		}
	}
	if (packed3)
		(void)packedclose(packed3,FALSE);
	else
		fclose(utfp3);
	utfp3 = NULL;
	if(found)
		return(zz[afld2].val);
	else
		return(nullbuf);
}

static int
getmyrec(file,fmt)
FILE *file; 
struct fmt *fmt;
{	
	static int nlinfw;	/* warn about NL in fixed width attribute */
	int i,c,eos;
	char *s;

	fmt->val = s = myrecptr;
	switch(fmt->flag) {
	case WN:
		for(eos=i=0;i < fmt->flen && (c=getc(file)) != EOF ;i++)
			if ((c == '\n') && (!(eos))) {
				/*
				if (nlinfw == 0) {
					error( E_GENERAL, "getrec: WARNING: Input table contains embedded NL in fixed width attribute!\n");
					++nlinfw;
				}
				*/
			} else if (c == '\0') {
				++eos;
			}
			if(s != &myrecbuf[MAXREC])
				*s++ = c;
			else {
				return(-1);
			}
		/* if first attribute in record is zero-width then check for EOF */
		if ((i == 0) && (s == &myrecbuf[0])) {
			if ((c=getc(file)) == EOF) {
				return(-1);
			} else {
				ungetc(c,file);
			}
		}
		if(i < fmt->flen)
			return(-1);
		break;
	case T:	
		while ( (c=getc(file)) != fmt->inf[1] && (c != EOF))
		{
			int newch;

			switch( c ) {
			case EOF:
				/*
				if ( s != myrecptr ) {
					error( E_GENERAL, "getrec: premature EOF encountered in parsing attribute value.\n" );
					return(-1);
				}
				*/
				return(-1); /* end of tbl */
			case '\n':
				/*
				error( E_GENERAL, "getrec: embedded newline in attribute value.\n" );
				*/
				return(-1);
			case '\\':
				newch = getc( file );
				if ( newch != '\n'  && newch != '\0' && newch == fmt->inf[1] )
					c = newch;
				else
					(void)ungetc( newch, file );
				break;
			}

			if(s != &myrecbuf[MAXREC]) {
				*s++ = c;
			} else {
				return(-1);
			}
		}
		if(c != fmt->inf[1])
			return(-1);
		break;
	default:
		return(-1);
	}
	if(s < &myrecbuf[MAXREC])
	{
		if ( s - myrecptr >= DBBLKSIZE ) {
			/*
			error( E_GENERAL, size_att, DBBLKSIZE );
			*/
			return(-1);
		}
		*s++='\0';
		myrecptr=s;
		return(0);
	} else {
		/*
		error( E_GENERAL, size_msg, MAXREC );
		*/
		return(-1);
	}
}
