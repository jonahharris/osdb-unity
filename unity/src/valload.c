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
#include <sys/types.h>
#include <sys/stat.h>
extern char *calloc();

vload(file,instrp, ninstrp, datap, ndatap, linep, nlinep)
char	*file;
char **instrp, **datap;
int *ninstrp, *ndatap, *nlinep;
short **linep;
{
	/* Load object code into memory. */

	register short *obj;
	short *valobj();

	obj = valobj(file);
	*ninstrp = obj[0];
	*ndatap = obj[1];
	*nlinep = (vobjn(file) - *ninstrp - *ndatap - 2 * sizeof(short)) /
	  sizeof(short);
	*instrp = (char *)(&obj[2]);
	*datap = *instrp + *ninstrp;
	*linep = (short *)(*datap + *ndatap);
}

vwhere(line, nline, loc)
register short *line;
int nline, loc;
{
	/* Function to return line number that corresponds 
	   to instruction location.
	*/

	register int l;
	register int m = 0;

	for (l = 0; l < nline; l++) {
		if (loc <= line[l])
			break;
		if (line[m] != line[l])
			m = l;
	}
	return(m + 1);
}

static short *
valobj(name)
char	*name;
{
	long size;
	char *buf;
	int fd;
	struct stat sbuf;

	/* Returns size of file or -1 if file doesn't exist.
	*/

	if (stat(name, &sbuf) < 0)
		ftlerr("valobj:%s nonexistent",name);
	else
		size = sbuf.st_size;
	if (size > 32000)
		ftlerr("valobj:%s too big",name);
	if ((buf = calloc((unsigned)size, 1)) == NULL)
		ftlerr("valobj: no space");
	if((fd = open(name, 0)) == -1)
		ftlerr("cannot open `%s'",name);
	if(read(fd, buf, (int)size)==-1)
		ftlerr("error on read");
	if(close(fd)== -1)
		ftlerr("error on close");
	return((short *)buf);
}

static int
vobjn(name)
char	*name;
{
	struct stat sbuf;
	int	size;

	if (stat(name, &sbuf) < 0)
		size = -1;
	else
		size = sbuf.st_size;
	return(size);
}
