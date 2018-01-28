/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* This function takes pairs of key value and location of the beginning
   of the record containing that key value (in sorted order).  It
   produces a file containing the negative number of records containing
   a given key followed by the locations of those records.  The key
   value and the location (in the Atable.key file) where the list (number
   and locations) for the given key starts are used to build the Btree file
   (in the Btable.key file).
*/
#include <stdio.h>
#include "db.h"

#define OLD 1
#define NEW 2
extern FILE	*popen();
extern char *calloc();

static	long 	num;
static unsigned long nvec;	/* number of entries in vec */
static	long	*vec, Aloc, Bloc, ldval;
static	char	key[KEYLEN + 1];
static	char	oldkey[KEYLEN + 1];

#define MAXLVL 13

static	union	{
	char	*string;
	struct 	hdr *header;
}	unode;
static	char	nx[MAXLVL][2*NDSZ], *maxkey();
static	int	skip, ptr[MAXLVL], Afd= -1, Bfd= -1;

extern	long	lseek();
extern	char	*strcpy(), *strcat(), *strrchr();
extern	FILE	*utfp2;

static	int	exitcode;
static char	*prog;
static char	Atable[30], Btable[30];

static int
cleanup()
{
	unlink(Atable); Atable[0] = '\0';
	unlink(Btable); Btable[0] = '\0';
	close(Afd); Afd = -1;
	close(Bfd); Bfd = -1;
	if(utfp2) {
		fclose(utfp2); utfp2 = NULL;
	}
}

index(argc, argv)
register int	argc;
register char	*argv[];
{
	int	lvl, j, cont, errors, records;
	char	cmd[520], *Ioption, *tbldesc;
	char	dtable2[MAXPATH+4];	/* allow for "/./" or "././" prefix */

	if ((prog = strrchr(argv[0], '/')) == NULL) {
		prog = argv[0];
	} else {
		++prog;
	}

	exitcode = 1;
	key[0] = oldkey[0] = '\0';
	Ioption = NULL;

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

	if((argc != 4) || (strcmp(argv[2],"in") != 0) || (strcmp(argv[3],"-") == 0)) {
		if ((argc == 4) && (strcmp(argv[3],"-") == 0))
			error(E_GENERAL,
				"%s: \"table\" cannot come from standard input (\"-\")\n",
				prog);
		error(E_GENERAL,"Usage: %s [-Itable] aname in table\n",prog);
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
			tbldesc = dtable2;
		}
		else
			tbldesc = Ioption;
	} else {
		/*
		 * Do not pass table name with a special prefix
		 * to ncheck() in the table description parameter
		 * so it does not search the data directory first.
		 */
		tbldesc = argv[3];
		while ((strncmp(tbldesc,"/./",3) == 0) || (strncmp(tbldesc,"./",2) == 0))
			tbldesc += 2;
	}
	/*
	 * Make sure input table is clean.
	 * Otherwise index2(UNITY) will fail
	 * and we will not know about the error
	 * since we are piping the output through sort(1).
	 */
	if(ncheck(argv[3],tbldesc,1,&errors,&records)==ERR) {
		if (records) {
			error(E_GENERAL,
				"%s: check record errors (%d) - %d records in %s\n",
				prog, errors, records, argv[3]);
		} else {
			error(E_GENERAL,
				"%s: check record error in %s\n",
				prog, argv[3]);
		}
		return(exitcode);
	}

	/* initialization */

	if ( vec == NULL )
	{
		vec = (long *)calloc( MAXPTR, sizeof( long ) );
		nvec = MAXPTR;
		if ( vec == (long *) NULL ) {
			error( E_GENERAL, "%s: cannot allocate memory for index value array\n",
				prog );
			exit(1);
		}
	}

	skip = sizeof(long)+2*sizeof(char);
	Bloc = NDSZ;
	for(lvl=0;lvl<MAXLVL;lvl++) {
		for(j=0 ; j<2*NDSZ ; nx[lvl][j++]=0);
		ptr[lvl]=HDR;
		unode.string = nx[lvl];
		unode.header->level=lvl;
	}

	/* get Atable.aname */
	getfile(Atable,argv[3],0);
	Atable[0] = 'A';
	strcat(Atable,".");
	strcat(Atable,argv[1]);

	/* get Btable.aname */
	strcpy(Btable,Atable);
	Btable[0] = 'B';

	unlink(Atable);
	unlink(Btable);

	if((Afd=creat(Atable,0666))==ERR) {
		error(E_DATFOPEN,prog,Atable);
		return(exitcode);
	}

	if((Bfd=creat(Btable,0666))<0) {
		error(E_DATFOPEN,prog,Btable);
		return(exitcode);
	}

	if(Ioption)
		sprintf(cmd,"index2 -I%s %s %s | sort", Ioption, argv[1],
			argv[3]);
	else
		sprintf(cmd,"index2 %s %s | sort", argv[1], argv[3]);
	if((utfp2 = popen(cmd,"r")) == NULL) {
		error(E_GENERAL,"%s: indexing failed\n",prog);
		cleanup();
		return(exitcode);
	}

	Aloc = lseek(Afd,0L,1);
	reset();
	if(getone()== EOF) {
		cleanup();
		return(0);
	}
	if ( enter() < 0 ) {
		cleanup();
		return( 1 );
	}
	strcpy(oldkey,key);
	cont = 1;
	while(cont) {
		switch(getone()) {
		case OLD:
			if ( enter() < 0 )
			{
				cleanup();
				return( 1 );
			}
			break;
		case NEW:
			if(writem() < 0) {
				cleanup();
				return(1);
			}
			reset();
			if ( enter() < 0 )
			{
				cleanup();
				return(1);
			}
			break;
		case EOF:
			if(writem() < 0) {
				cleanup();
				return(1);
			}
			cont = 0;
			break;
		default:
			error(E_GENERAL,"%s:  read error\n",prog);
			cleanup();
			return(1);
		}
	}
	if(endindex() < 0) {
		cleanup();
		return(1);
	}

	close(Afd); Afd = -1;
	close(Bfd); Bfd = -1;
	fclose(utfp2); utfp2 = NULL;

	exitcode = 0;
	return(exitcode);
}

static int
reset()
{
	num=0;
	strcpy(oldkey,key);
}

static int
writem()
{
	long		number;
	unsigned	bytes;

	number = -num;
	bytes = (unsigned)(num*sizeof(long));
	if ((write(Afd,(char *)&number,sizeof(long)) != sizeof(long)) ||
	    (write(Afd,(char *)vec,bytes) != bytes)) {
		error(E_DATFWRITE,prog,Atable);
		return(-1);
	}

	if(addrec(oldkey,Aloc,0) < 0)	/* build the btree */
		return(-1);

	Aloc = lseek(Afd,0L,1);
	return(0);
}

static int
getone()
{
	int i, c, err;

	/* get key value */
	for(i=0,err=0;(c=getc(utfp2))!='\01'&& c!=EOF;i++) {
		if(i < KEYLEN)
			key[i] = c;
		else {
			if(!err) {
				key[KEYLEN] = '\0';
				error(E_GENERAL,
					"Key beginning with %s truncated\n",
					key);
				err = 1;
			}
		}
	}
	if(c==EOF)
		return(EOF);
	if(i <= KEYLEN)
		key[i]=0;

	/* get seek value */
	fscanf(utfp2,"%ld",&ldval);

	/* get rid of newline */
	while((c=getc(utfp2))!='\n' && c!=EOF);

	if(strcmp(key,oldkey))
		return(NEW);
	else
		return(OLD);
}

static int
enter()
{
	if ( num == nvec ) {
		nvec += MAXPTR;
		vec = (long *) realloc( vec, nvec * sizeof(long) );
		if ( vec == NULL )
		{
			error( E_GENERAL, "%s: cannot re-allocate space for index value array\n",
				prog );
			return( -1 );
		}
	}

	vec[num++]=ldval;

	return( 0 );
}

static char *
maxkey(s,buf,lvl)
char *s,*buf;
int lvl;
{
	char *a,*b;

	/* find maximum key for a node */
	a = b = s+HDR;
	while(*b!=MAXCHAR && innode(b,lvl)) {
		a=b;
		while(*b++!='\0');
		b += sizeof(long);
	}
	strcpy(buf,a);
	return(b);
}

static int
addrec(s,val,lvl)
char *s;
long val;
int lvl;
{
	char	*p1=nx[lvl]+ptr[lvl];
	int	i;
	union	{
		char	*c;
		long	*l;
	}	value;

	/* add a record to the current node */
	for(; *s!='\0'; *p1++ = *s++);		/*copy key value to node*/
	*p1++ = '\0';

	value.l = &val;
	for(i=0;i<sizeof(long);i++)
		*p1++ = *value.c++;		/*seek value */

	ptr[lvl]=p1-nx[lvl];			/* increment ptr to next pos*/

	if (ptr[lvl]>=NDSZ-skip) {
		if(split(lvl) < 0)		/* node overflow */
			return(-1);
	}
	return(0);
}

static int
split(lvl)
int lvl;
{
	char buf[KEYLEN + 1],*b;
	int i;
	long oldloc;

	/* split a node on overflow */
	b=maxkey(nx[lvl],buf,lvl);	/* find maximum key for node */
	mvgbt(1,NDSZ,b,nx[lvl]+NDSZ);	/* move overflow to next node */
	ptr[lvl] += nx[lvl]+NDSZ-b;	/* reset next free node position */

	*b++ = MAXCHAR;			/* indicate end of finished node */
	*b++='\0';
	for(i=0;i<sizeof(long);i++)
		*(b+i) = *(b-skip+i);	/*loc of highest key*/

	oldloc=Bloc;
	if (wrtnode(Bloc,nx[lvl]) < 0)	/* write out finished node */
		return(-1);
	Bloc = lseek(Bfd,0L,1);		/* reset current write position*/

	mvgbt(1,NDSZ,nx[lvl]+NDSZ,nx[lvl]+HDR);	/*move overflow section
						  to pos of node written out*/
	ptr[lvl] -= NDSZ-HDR;		/* reset next free node position*/

	if (lvl+1 == MAXLVL) {
		error(E_GENERAL,"%s: Too many keys\n",prog);
		return(-1);
	}
	if(addrec(buf,oldloc,lvl+1) < 0)/* write buf to next highest entry */
		return(-1);
	return(0);
}

static int
wrtnode(locat,node)
long locat; 
char *node;
{
	/* write node to index file */
	unode.string = node;
	unode.header->chksum=0;
	unode.header->nodeloc=locat;
	unode.header->chksum=ndcheck((int *)node);
	lseek(Bfd,locat,0);
	if (write(Bfd,node,NDSZ) != NDSZ) {
		error(E_DATFWRITE,prog,Btable);
		return(-1);
	}
	return(0);
}

static int
endindex()
{
	int	lvl;
	char	buf[KEYLEN + 1];
	long	oldloc;

	/* write out partially filled nodes */
	for(lvl=0;;lvl++) {
		infin(lvl);		/* put MAXCHAR after last entry */
		if(ptr[lvl]>NDSZ) {
			if(split(lvl) < 0)
				return(-1);
		}

		if(ptr[lvl+1]<=HDR)	/* nothing on next higher index*/
			break;

		(void)maxkey(nx[lvl],buf,lvl);	/* find maximum key for node */
		oldloc = Bloc;
		if (wrtnode(Bloc,nx[lvl]) < 0)	/* write out finished node */
			return(-1);
		Bloc = lseek(Bfd,0L,1);		/* current position in file */
		if(addrec(buf,oldloc,lvl+1)< 0)	/* add to next higher index */
			return(-1);
	}

	if (wrtnode(0L,nx[lvl]) < 0)		/* write highest level index */
		return(-1);

	return(0);
}

static int
infin(lvl)
int lvl;
{
	char	*p1;
	int	j;

	/* put MAXCHAR at end of current node */
	p1=nx[lvl]+ptr[lvl];
	*p1++=MAXCHAR;
	*p1++='\0';

	for(j=0;j<sizeof(long);j++)
		*(p1+j)= *(p1+j-skip);
	ptr[lvl] += skip;
}

static int
innode(s,lvl)
char *s;
int lvl;
{
	while(*s++!='\0');
	s+= sizeof(long);
	if ( s < nx[lvl]+NDSZ-skip)
		return(1);
	else
		return(0);
}
