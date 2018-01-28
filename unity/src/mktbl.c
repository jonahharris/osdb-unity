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
#include <ctype.h>

extern char *strcpy(), *strchr();

extern FILE *get2file();

_mktbl(prog, table, Dtable, fmt, friendly, fcnt, altdesc )
char	*prog;
char	*table;
char	*Dtable;
struct	fmt	*fmt;
char friendly[][ MAXUNAME + 1 ];
int fcnt;			/* count of entries in friendly */
char	*altdesc;		/* alternate table description  */
{
	int	c, i, nattr, retcode, brflg;
	FILE	*t1;
	struct	fmt *p;
	char	justify;
	char	buffer[BUFSIZ];

	/*
	 * If an alternate descriptor has been given
	 * that includes a "/./" (full) or "././" (relative)
	 * path prefix of if the description and table names
	 * are the same, then look for the descriptor in the
	 * given directory first.  Otherwise, just stick with
	 * the default search.
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
	if((t1=get2file(c, "D", table, Dtable, "r")) == NULL) {
		getfile(Dtable,table,0);
		error(E_DESFOPEN,prog,Dtable);
		return(ERR);
	}
	retcode = 0;
	brflg = 0;
	for(nattr=0,p=fmt; (c=getc(t1))!=EOF ;nattr++,p++) {
		while(c == '#' || c == '\n') {	/* ignore lines beginning with # */
			if(c == '#') {
				/* get rest of line */
				while( (c=getc(t1)) != EOF && c != '\n');
			}
			/* at end of line or end of file */
			if(c == EOF || (c=getc(t1)) == EOF) {
				brflg = 1;	/* end of file */
				break;
			}
		}
		if(brflg)
			break;
		if(nattr == MAXATT) {
			error(E_GENERAL,
			"%s: too many attributes in description %s.\n",prog,
			Dtable);
			fclose(t1);
			return(ERR);
		}
		ungetc(c,t1);		/* unget character just gotten	*/

		p->justify = 'l';	/* left justify as the default	*/
		p->prnt = 0;		/* print width 0 as the default	*/

		/* get attribute name */
		if(fscanf(t1,"%s",buffer) == EOF)
			break;		/* no more attributes */
		if (strlen(buffer) > ANAMELEN) {
			error(E_GENERAL,
	"%s: Warning: attribute name, %s, too long in description %s.\n",
				prog,buffer,Dtable);
				/* not a fatal error */
		}
		if( !isalpha(buffer[0]) && buffer[0] != '_' ) {
			error(E_GENERAL,
"%s: Attribute name %s does not begin with a letter or underscore in description %s.\n",
				prog,buffer,Dtable);
			retcode = ERR;
		}
		p->aname[0] = buffer[0];	/* transfer first char */
		for(i=1;buffer[i] != '\0' && i < ANAMELEN;i++) {
			if(!isalnum(buffer[i]) && buffer[i] != '_' ) {
				error(E_GENERAL,
	"%s: Illegal character %c in attribute name %s in description %s.\n",
				prog,buffer[i],buffer,Dtable);
				retcode = ERR;
			}
			p->aname[i] = buffer[i];
		}
		p->aname[i] = '\0';
		if ( nattr < fcnt )	/* default is aname */
			strcpy( friendly[nattr], p->aname );

		/* get tab separator */
		if ((c=getc(t1)) != '\t') {
			error(E_GENERAL,
	"%s: Missing tab separator after attribute %s in description %s.\n",
				prog,p->aname,Dtable);
			retcode = ERR;
			nattr--; p--;
			for(;c!= '\n' && c!= EOF; c=getc(t1));/* next line */
			continue;
		}

		/* get attribute description */
		for(i=0;(c=getc(t1)) != '\n' && c != EOF;i++) {
			/* check case where T field with tab terminator */
			if (c=='\t' && i>1)
				break;
			buffer[i]=c;
		}
		buffer[i]='\0';

		constr(buffer);		/* convert escape sequences */
		switch(buffer[0]) {
		case 'w':
			p->flag=WN;
			p->flen=atoi(&buffer[1]);
			/* ignore characters after field length */
			p->val=0;
			p->inf[0] = '\0';
			if (p->flen >= DBBLKSIZE) {
				error(E_GENERAL,
	"%s: Fixed-width attribute %s length (%d) exceeds maximum allowed length (%d) in description %s.\n",
					prog, p->aname, p->flen, DBBLKSIZE - 1, Dtable);
				retcode = ERR;
				nattr--; p--;
				for(;c!= '\n' && c!= EOF; c=getc(t1));/* next line */
				continue;
			}
			break;
		case 't':
			p->flag=T;
			p->flen=0;
			p->val=0;
			sprintf(p->inf,"t%c",buffer[1]);
			/* remaining characters ignored */
			break;
		default:
			error(E_GENERAL,
	"%s: Unknown input format %c for attribute %s in description %s.\n",
				prog,buffer[0],p->aname,Dtable);
			retcode = ERR;
			nattr--; p--;
			for(;c!= '\n' && c!= EOF; c=getc(t1));/* next line */
			continue;
		}

		/* get print width and/or justification if they exist */
		if(c== '\t') {
			justify = '\0';
			c = getc(t1);
			if(c != '\t' && c != EOF)
			{
				while ( c == ' ' ) {
					c = getc(t1);	/* remove leading space(s) */
				}
				switch ( c ) {
				case 'c':
				case 'C':
				case 'l':
				case 'L':
				case 'r':
				case 'R':
					justify = c;
					c = getc(t1);
					break;
				default:
					break;
				}
				if ( isdigit( c ) )
				{
					int i = c - '0';
					while ( 1 ) {
						c = getc(t1);
						if ( ( c != EOF ) && ( isdigit( c ) ) ) {
							i = 10 * i + c - '0';
						} else {
							break;
						}
					}
					if ( i >= 0 ) {
						p->prnt = i;
					}
					if ( justify == '\0' ) {
						switch ( c ) {
						case 'c':
						case 'C':
						case 'l':
						case 'L':
						case 'r':
						case 'R':
							justify = c;
							c = getc(t1);
							break;
						default:
							break;
						}
					}
				}
				while ( c == ' ' ) {
					c = getc(t1);	/* remove trailing space(s) */
				}
				if ( justify != '\0' ) {
					p->justify = justify;
				}
			}
			switch ( c ) {
			case '\n':	/* no user friendly name specified */
				break;
			case '\t':	/* user friendly name specified */
				break;
			default:
				error(E_GENERAL,
"%s: invalid justification character %c for attribute %s in description %s.\n",
				prog,c,p->aname,Dtable);
				for(;c!= '\n' && c!= EOF; c=getc(t1));
				retcode = ERR;
			}
		}
		if(c == '\t') {
			/* remainder of line is user friendly name */
			for(i=0, c=getc(t1);c!= '\n' && c!='\t' && c!= EOF;
				c=getc(t1)) {
				if ( nattr < fcnt && i < MAXUNAME )
					friendly[nattr][i++] = c;
			}
			if ( i > 0 && nattr < fcnt )
			{
				if ( i < MAXUNAME )
					friendly[nattr][i] = '\0';
				else
					friendly[nattr][MAXUNAME] = '\0';
			}
		}
		/* ignore remaining characters on line */
		for(;c!= '\n' && c!= EOF; c=getc(t1));
	}
	fclose(t1);
	if(nattr == 0) {
		error(E_GENERAL,
		"%s: no attributes or invalid description in description %s.\n",
		prog,Dtable);
		return(ERR);
	}
	if(fmt[nattr-1].flag == T && fmt[nattr-1].inf[1] != '\n') {
		error(E_GENERAL,
		"%s: final terminator in description %s must be a newline\n",
		prog,Dtable);
		return(ERR);
	}
	if(retcode == ERR)
		return(ERR);
	else
		return(nattr);
}

static int
constr(s)
char *s;
{
	char	*p=s, c;
	int	i;

	while ( (c = *s++) != '\0') {
		if (c!='\\') {
			*p++=c;
			continue;
		}
		switch(c = *s++) {
		default:
			*p++=c;
			break;
		case 'n': 
			*p++='\n'; 
			break;
		case 't': 
			*p++='\t'; 
			break;
		case 'a': 
			*p++='\a'; 
			break;
		case 'b': 
			*p++='\b'; 
			break;
		case 'r': 
			*p++='\r'; 
			break;
		case 'v': 
			*p++='\v'; 
			break;
		case 'f': 
			*p++='\f'; 
			break;
		case '\\': 
			*p++='\\'; 
			break;
		case '\'': 
			*p++='\''; 
			break;
		case '0': 
		case '1': 
		case '2': 
		case '3':
		case '4': 
		case '5': 
		case '6': 
		case '7':
			i=c-'0';
			while (isdigit(c = *s++))
				i=8*i+c-'0';
			*p++=i;
			s--;
			break;
		}
	}
	*p = '\0';
}

mkntbl(prog, table, Dtable, fmt, altdesc )
char	*prog;
char	*table;
char	*Dtable;
struct	fmt	*fmt;
char	*altdesc;
{
	return( _mktbl( prog, table, Dtable, fmt, NULL, 0, altdesc ) );
}

/*
 * mktbl() is being replaced by mkntbl() and should be removed sometime
 */

mktbl(prog, table, Dtable, fmt )
char	*prog;
char	*table;
char	*Dtable;
struct	fmt	*fmt;
{
	return( _mktbl( prog, table, Dtable, fmt, NULL, 0, NULL ) );
}
