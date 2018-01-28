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
#include <string.h>
#include "urelation.h"
#include "qdebug.h"

struct qdebug {
	char *str;
	int val;
};

static struct qdebug debuglist[] = {
	"hash",		QDBG_HASH,
	"expr",		QDBG_EXPR,
	"select",	QDBG_SELECT,
	"sel",		QDBG_SELECT,
	"join",		QDBG_JOIN,
	"io",		QDBG_IO,
};

#define MAXDEBUG	(sizeof( debuglist ) / sizeof( struct qdebug ))

int _qdebug;

int
set_qdebug( debugstr )
char *debugstr;
{
	struct qdebug *debugptr;
	char *ptr;
	char found;
	int rc;

	if ( debugstr == NULL || *debugstr == '\0' ) {
		_qdebug = 0;
		return( TRUE );
	}
	if ( strcmp( debugstr, "all" ) == 0 ) {
		_qdebug = -1;	/* set all flags */
		return( TRUE );
	}

	rc = TRUE;
	while( 1 ) {
		ptr = strchr( debugstr, ',' );
		if ( ptr )
			*ptr = '\0';
		found = FALSE;
		for( debugptr = debuglist; debugptr < &debuglist[MAXDEBUG];
				debugptr++ ) {
			if ( strcmp( debugptr->str, debugstr ) == 0 ) {
				found = TRUE;
				_qdebug |= debugptr->val;
				break;
			}
		}
		if ( ! found )
			rc = FALSE;
		if ( ptr ) {
			*ptr = ',';	/* put back the comma */
			debugstr = ptr + 1;
		}
		else
			break;
	}

	return( rc );
}
