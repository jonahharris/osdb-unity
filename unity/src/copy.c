/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include	<sys/types.h>
#include	<sys/stat.h>
#include	"db.h"

extern unsigned short getuid();

copy(prog,source, target)
char *prog, *source, *target;
{
	char	fbuf[BUFSIZ];
	int 	from, to, ct, oflg;
	struct	stat s1, s2;

	setuid( (int)getuid() );
	if (stat(source, &s1) < 0) {
		error(E_DATFOPEN,prog,source);
		return(ERR);
	}
	if ((s1.st_mode & S_IFMT) == S_IFDIR) {
		error(E_GENERAL, "%s : <%s> directory\n", prog, source);
		return(ERR);
	}
	s2.st_mode = S_IFREG;
	if (stat(target, &s2) >= 0) {
		if ((s2.st_mode & S_IFMT) == S_IFDIR) {
			error(E_GENERAL, "%s : <%s> directory\n", prog, target);
			return(ERR);
		}
		if (stat(target, &s2) >= 0) {
			if (s1.st_dev==s2.st_dev && s1.st_ino==s2.st_ino) {
				error(E_GENERAL,
					"%s: %s and %s are identical\n",
					 prog, source, target);
				return(ERR);
			}
		}
	}

	if((from = open(source, 0)) < 0) {
		error(E_DATFOPEN,prog,source);
		return(ERR);
	}
	oflg = chkaccess(target, 0) == -1;
	if((to = creat (target, 0666)) < 0) {
		error(E_DATFOPEN,prog,target);
		return(ERR);
	}
	while((ct = read(from, fbuf, BUFSIZ)) > 0)
		if(ct < 0 || write(to, fbuf, (unsigned)ct) != ct) {
			error(E_GENERAL, "%s: bad copy to %s\n", prog, target);
			if((s2.st_mode & S_IFMT) == S_IFREG)
				unlink(target);
			return(ERR);
		}
	close(from), close(to);
	if (oflg)
		chmod(target, (int)s1.st_mode);
	return(0);
}
