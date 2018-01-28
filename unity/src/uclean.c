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
#include <signal.h>

char	Dtable2[MAXPATH+4], *table2, *stdbuf;
char	Dpak[MAXPATH+4], pak[MAXPATH+4];	/* allow for "/./" or "././" prefix */
char	DtmpItable[MAXPATH+4], tmpItable[MAXPATH+4];
char	tmptable[MAXPATH+4], lockfile[MAXPATH+4];
FILE	*udfp0, *utfp0, *utfp1, *udfp1, *utfp2, *udfp2;	/* file ptrs for tbl and desc */

uclean()
{
	if(utfp0 != NULL) {
		if(utfp0 != stdout) {
			if(utfp0 != stdin)
				fclose(utfp0);
		}
		else {
			fflush(stdout);
			if(stdbuf)
				setbuf(stdout,NULL);
		}
		utfp0 = NULL;
	}
	if(utfp1 != NULL) {
		if(utfp1 != stdout) {
			if(utfp1 != stdin)
				fclose(utfp1);
		}
		else {
			fflush(stdout);
			if(stdbuf)
				setbuf(stdout,NULL);
		}
		utfp1 = NULL;
	}
	if(utfp2 != NULL) {
		if(utfp2 != stdout) {
			if(utfp2 != stdin)
				fclose(utfp2);
		}
		else {
			fflush(stdout);
			if(stdbuf)
				setbuf(stdout,NULL);
		}
		utfp2 = NULL;
	}
	if(udfp0 != NULL) {
		fclose(udfp0);
		udfp0 = NULL;
	}
	if(udfp1 != NULL) {
		fclose(udfp1);
		udfp1 = NULL;
	}
	if(udfp2 != NULL) {
		fclose(udfp2);
		udfp2 = NULL;
	}
	if(stdbuf) {
		free(stdbuf);
		stdbuf = NULL;
	}
	if(table2) {
		unlink(table2);
		table2 = NULL;
	}
	if(Dtable2[0] != '\0') {
		unlink(Dtable2);
		Dtable2[0] = '\0';
	}

	if(tmptable[0] != '\0') {
		unlink(tmptable);
		tmptable[0] = '\0';
	}
	if(lockfile[0] != '\0') {
		rmlock(lockfile);
		lockfile[0] = '\0';
	}

	if(pak[0] != '\0') {
		unlink(pak);
		pak[0] = '\0';
	}
	if(Dpak[0] != '\0') {
		unlink(Dpak);
		Dpak[0] = '\0';
	}
	/*
	 * NOTE: tmpItable[] is defined together with DtmpItable[]
	 *	 for user convience and tmpItable[] is not removed
	 *	 since it is only needed (intended) to be used as
	 *	 the name of the descriptor file for mkntbl() and
	 *	 udfp0 is used to create DtmpItable[] by getdescrwtbl().
	 */
	if(DtmpItable[0] != '\0') {
		unlink(DtmpItable);
		DtmpItable[0] = '\0';
	}
	/*
	 * reset current date (year) for call to cnvtdate()
	 */
	init_cnvtdate( );

	return;
}
