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

extern char *dirname(), *strcpy(), *strchr(), *getenv();
extern FILE *get2file();

static   char *defaultsearch = "cdu";

static char *
filecat(s1, s2, s3, si)
register char *s1, *s2;
char *s3;
char	*si;
{
	register char	*s;

	/* copy directory name */
	s = si;
	while(*s1)
		*s++ = *s1++;
	if(si != s)
		*s++ = '/';
	/* copy prefix, if any */
	while(*s2)
		*s++ = *s2++;
	/* copy simple file name */
	while(*s3)
		*s++ = *s3++;
	*s = '\0';
	return(*s1? ++s1: 0);
}

strcmpfile(p1, p2)
char	*p1;		/* path name of first file */
char	*p2;		/* path name of second file */
{
	/* remove any special path prefix(s) from p1 */
	if (p1) {
		while ((strncmp(p1,"/./",3) == 0) || (strncmp(p1,"./",2) == 0))
			p1 += 2;
	}
	/* remove any special path prefix(s) from p2 */
	if (p2) {
		while ((strncmp(p2,"/./",3) == 0) || (strncmp(p2,"./",2) == 0))
			p2 += 2;
	}
	/* now compare the file pathnames and return the result */
	return(strcmp(p1,p2));
}

getfile(Dtable, table, dir)
char	Dtable[];	/* output description file name */
char	table[];	/* input table name		*/
int	dir;		/* indicator of directory in which to look for dfile */
{
	int ptr,i,k;
	char *dfile;

	ptr = 0;

	/* set ptr to the character following the last / */
	for (i=0; table[i]; i++) {
		if ( table[i] == '/') {
			ptr = i + 1;
		}
	}

	switch(dir) {
	case 0:	/* Dfile from current directory */
		/* copy the simple file name */
		for (i=ptr,k=1; table[i]; i++,k++)
			Dtable[k] = table[i];
		Dtable[k] = '\0';
		Dtable[0] = 'D';	/* prefix with a D */
		return(0);

	case 1:	/* Dfile from directory containing data file */
		strcpy(Dtable,table);
		dirname(Dtable);
		if (strcmp(Dtable,".") == 0)
			k = 0;
		else {
			k = strlen(Dtable);
			Dtable[k++] = '/';
		}
		Dtable[k++] = 'D';
		for (i=ptr; table[i]; i++,k++)
			Dtable[k] = table[i];
		Dtable[k] = '\0';
		return(0);

	case 2:		/* UNITYDFILES */
	default:
		if ( get2file( dir, "D", table, Dtable, NULL ) != NULL ) {
			return(0);
		}
		return(ERR);
	}
}

/*
 *	get2file - find file in one of directories specified in pathstr
 *		   with given chkaccess permissions
 */

FILE *
get2file(type, prefix, iname, oname, amode)
int type;
char *prefix;		/* prefix to add to simple file name */
char *iname;		/* input file name */
char *oname;		/* output file name */
char *amode;		/* open mode */
{
	register int ptr;
	register char *next;
	int  currentdir, datadir, udfiles;
	char where2search[8];
	char *searchnext;
	FILE *file;

	/* If UNITYDSEARCH is not set then use default search order. */
	if ((next = getenv("UNITYDSEARCH")) == NULL) {
		next = defaultsearch;
	}

	ptr = currentdir = datadir = udfiles = 0;

	/* Was a particular directory given to search or search first? */
	switch ( type ) {
	case 0:			/* current directory */
	case 1:			/* datafile directory */
	case 2:			/* UNITYDFILES directory(s) */

		where2search[ptr++] = defaultsearch[type];
		next = NULL;
		break;

	case 'C':
	case 'c':		/* current directory */

		where2search[ptr++] = 'c';
		++currentdir;
		break;

	case 'D':
	case 'd':		/* datafile directory */

		where2search[ptr++] = 'd';
		++datadir;
		break;

	case 'U':
	case 'u':		/* UNITYDFILES directory(s) */

		where2search[ptr++] = 'u';
		++udfiles;
		break;

	case 'S':
	case 's':		/* standard search order */
	default:
		break;
	}

	if ( next != NULL ) {

		while ( *next ) {
			switch ( *next++ ) {
			case 'C':
			case 'c':
				/* only search current directory once */
				if ( currentdir ) continue;
				where2search[ptr++] = 'c';
				++currentdir;
				break;
			case 'D':
			case 'd':
				/* only search datafile directory once */
				if ( datadir ) continue;
				where2search[ptr++] = 'd';
				++datadir;
				break;
			case 'U':
			case 'u':
				/* only search UNITYDFILES once */
				if ( udfiles ) continue;
				where2search[ptr++] = 'u';
				++udfiles;
				break;
			default:
				/* ignore invalid search indicator */
				break;
			}
		}
	}

	/*
	 * In case UNITYDSEARCH was an empty string (or garbage)
	 * make sure there is at least one place to search.
	 */
	if ( ptr ) {
		where2search[ptr] = '\0';	/* terminate where2search[] string */
		searchnext = where2search;
	} else {
		searchnext = defaultsearch;
	}

	/* get the simple file name */
	for (ptr=0, next = iname; *next; next++) {
		if (*next == '/') {
			ptr = next - iname + 1;
		}
	}

	while ( *searchnext )
	{
		switch ( *searchnext++ ) {

		case 'c':
			/*
			 * check the current directory
			 */
			(void)filecat("", prefix, &iname[ptr], oname);
			if (amode) {
				if (( file = fopen(oname, amode) ) != NULL )
					return(file);
			} else {
				if ( chkaccess(oname, 00) >= 0 )
					return((FILE *)1);
			}
			break;

		case 'd':
			/*
			 * check the datafile directory
			 */
			if (ptr == 0) {
				(void)filecat("", prefix, iname, oname);
			} else {
				strcpy(oname, iname);
				/* chop everything after last '/' */
				oname[ptr-1] = '\0';
				(void)filecat(oname, prefix, &iname[ptr], oname);
			}
			if (amode) {
				if ((file = fopen(oname, amode)) != NULL)
					return(file);
			} else {
				if (chkaccess(oname, 00) >= 0)
					return((FILE *)1);
			}
			break;

		case 'u':
			/*
			 * check UNITYDFILES directory(s)
			 */
			next = getenv( "UNITYDFILES" );
			while( next && *next )
			{
				register char *colon;

				colon = strchr( next, ':' );
				if ( colon )
					*colon = '\0';
				if ( *next )
					(void)filecat(next, prefix, &iname[ptr], oname);
				else
					(void)filecat("", prefix, &iname[ptr], oname);
				if ( colon )
					*colon++ = ':';	/* put the colon back & go on */
				if (amode) {
					/* only give read permission */
					if ((file = fopen(oname, "r")) != NULL)
						return(file);
				} else {
					/* assume at least read permission is needed */
					if (chkaccess(oname, 00) >= 0)
						return((FILE *)1);
				}
				next = colon;
			}
			break;

		default:
			break;
		}
	}

	return((FILE *)NULL);
}
