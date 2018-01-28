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
#include <sys/types.h>
#include <sys/stat.h>

extern	char	*strcat(), *strcpy();

indexch(file0, attr0, file1, attr1, list, btree)
char	*file0, *attr0, *file1, *attr1;
struct	index **btree;
FILE	**list;
{
	char	ABname[MAXPATH+4];	/* allow for "/./" or "././" prefix */
	char	file[MAXPATH+4];
	char	attr[ANAMELEN+1], cmd[MAXCMD];
	int	i,status;
	struct	stat stat1,stat2;
	struct	index *bopen();

	strcpy(file,file0);
	strcpy(attr,attr0);
	for(i=0;i<2;i++) {
		if(file[0] == '\0' || strcmp(file,"-") == 0) {
			strcpy(file,file1);	/* get ready for second try */
			strcpy(attr,attr1);
			continue;
		}
		stat(file,&stat1);
		getfile(ABname,file,0);
		if ( strlen( ABname ) + strlen( attr ) + 2 > MAXPATH+4 )
		{
			error( E_GENERAL, "(indexch): file name too long (%d) for internal array (max %d)",
				strlen( ABname ) + strlen( attr ) + 2,
				MAXPATH+4 );
			return( ERR );
		}
		ABname[0] = 'A';
		strcat(ABname,".");
		strcat(ABname,attr);
		if(chkaccess(ABname,04)==0) {	/* check Aindexfile */
			ABname[0] = 'B';
			if(stat(ABname,&stat2) < 0 || stat2.st_mtime <
			    stat1.st_mtime) { /* needs updating */
				sprintf(cmd,"index %s in %s",attr,file);
				error(E_GENERAL, "Updating index files\n");
				if((status = system(cmd)) != 0) {
				error(E_GENERAL,
					"%s: %s failed with status = %d\n",
					"indexch","index",status);
					return(ERR);
				}
			}
			if((*btree = bopen(ABname)) == NULL) {
				error(E_GENERAL,"Open on index failed\n");
				return(ERR);
			}
			ABname[0] = 'A';
			if((*list = fopen(ABname,"r")) == NULL) {
				error(E_GENERAL,"Open on index failed\n");
				return(ERR);
			}
			return(i);
		}
		strcpy(file,file1);
		strcpy(attr,attr1);
	}
	return(ERR);
}
