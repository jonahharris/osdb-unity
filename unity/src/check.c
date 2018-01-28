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

static int	attrnr,nattr1,seqno,rpterr;
static struct	fmt xx[MAXATT];

check(infile,table)
char infile[];
char table[];
{
	return(ncheck(infile,table,-1,(int *)NULL,(int *)NULL));
}

ncheck(infile,table,errlimit,errors,records)
char infile[];
char table[];
int	errlimit;
int	*errors;
int	*records;
{
	extern	FILE *packedopen();
	extern	char *packedsuffix;
	extern	int  packedclose();
	int	c, errret, packed;
	int recsize, attsize;
	FILE *fp;
	char	Dtable[MAXPATH+4];	/* allow for "/./" or "././" prefix */

	/* function to check data to be inserted into table */
	seqno = errret = packed = 0;

	/*
	 * update the static variable that controls
	 * how many times error() can be called to
	 * report any errors that are found.
	 */
	rpterr = errlimit;

	/* initialize return error and record counts */
	if (errors) *errors = 0;
	if (records) *records = 0;

	if(strcmp(infile,"-") == 0)
		fp = stdin;
	else {
		if ((fp=fopen(infile,"r"))==NULL)
		{
			if ((fp = packedopen(infile)) == NULL) {
				error(E_DATFOPEN,"ucheck",infile);
				return(ERR);
			}
			++packed;	/* reading packed table */
		}
	}
	if((nattr1=mkntbl("ucheck",infile,Dtable,xx,table))==ERR) {
		if (fp != stdin) {
			if (packed) {
				/* EOF == FALSE */
				(void)packedclose(fp,FALSE);
			} else {
				fclose(fp);
			}
		}
		return(ERR);
	}
	while((c=getc(fp))!=EOF) {
		ungetc(c,fp);
		seqno++;
		recsize = 0;
		for(attrnr=0;attrnr<nattr1;attrnr++) {
			if (xx[attrnr].flag==WN) {
				attsize = fixlen( fp );
			}
			else { /* xx[attrnr].flag==T */
				attsize = varlen( fp );
			}
			if ( attsize < 0 )
			{
				errret = ERR;
				recsize += -attsize;
				break;
			}
			else
			{
				recsize += attsize;
			}
		}

		if ( recsize > MAXREC )
		{
			if (rpterr) {
				error( E_GENERAL, "ucheck: Record %d (%d chars) is larger than maximum record length (%d)\n",
				seqno, recsize, MAXREC );
			}
			attsize = -recsize;	/* note that this record had an error */
			errret = ERR;
		}

		if (attsize < 0) {
			if (rpterr) --rpterr;
			if (errors) ++*errors;
		}
	}

	if (packed) {
		/* EOF == TRUE */
		if (packedclose(fp,TRUE) != 0) {
			error(E_PACKREAD,"ucheck",infile,packedsuffix);
			errret = ERR;
		}
	} else {
		fclose( fp );
	}

	if (records) *records = seqno;

	/* return error or number of records checked */
	return( errret == ERR ? ERR : seqno );
}

static int
fixlen( fp )
FILE *fp;
{
	int	c, j, null;

	/* checked fixed length attribute */
	for(null=j=0;j<xx[attrnr].flen;j++) {
		if (((c=getc(fp)) == EOF)  ||
		   ((c == '\n') && (null == 0))) {
			if (rpterr)
				error(E_GENERAL,
					"ucheck: Fixed width attribute (%d) contains embedded NL on record %d\n",
					attrnr, seqno);
			if ( c == '\n' )
				j++;
			return( -j );
		} else if (c == '\0') {
			++null;
		}
	}
	if (attrnr==nattr1-1) {
		if ((c=getc(fp)) != '\n') {
			if (rpterr)
				error(E_GENERAL,"ucheck: Data overflow on record %d\n",
					seqno);
			if ( c != EOF )
				j++;
			while((c=getc(fp)) != '\n' && c != EOF)
				j++;
			if ( c == '\n' )
				j++;

			return( -j );
		}
		j++;
	}
	return( j );
}

static int
varlen( fp )
FILE *fp;
{
	int	c, j, escapes;

	/* check variable length attribute */
	for ( escapes = j = 0; (c=getc(fp)) != xx[attrnr].inf[1]; j++ ) {
		if (c == '\\' ) {
			int finished = 0;
			/*
			 * Backslash escape characters are only to be removed
			 * for the delimiter (termination) character.
			 */
			do {
				c = getc(fp);
				if (c == xx[attrnr].inf[1]) {
					if ((c == '\n') || (c == '\0')) {
						/*
						 * attribute value ends with '\\'
						 */
						++j;
						++finished;
					} else {
						++escapes;
					}
				} else {
					++j;
				}
			} while (c == '\\');

			if (finished)
				break;
		}
		if (c == EOF || c == '\n') {
			if (rpterr) 
				error(E_GENERAL,
					"ucheck: Insufficient data on record %d\n",
					seqno);
			if ( c == '\n' )
				j++;
			j -= escapes;
			return( -j );
		}
	}
	if (j >= DBBLKSIZE) {
		if (rpterr)
			error(E_GENERAL,
				"ucheck: Record %d attribute %d (%d chars) is larger than maximum attribute length (%d)\n",
				seqno, attrnr, j, DBBLKSIZE - 1);
		j = j + 1 - escapes;
		return( -j );
	}
	j = j + 1 - escapes;
	return( j );
}
