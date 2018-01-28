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
#include "uquery.h"
#include "uindex.h"
#include "message.h"

extern char *basename();

char *prog;

main( argc, argv )
int argc;
char **argv;
{
	char *found;
	int status;
	long seekval;
	FILE *fp;
	struct uindex *tree;
	char *relpath, *attrname;

	prog = basename( argv[0] );
	if ( argc == 3 )
	{
		attrname = argv[1];
		relpath = argv[2];
	}
	else if ( argc == 4 && strcmp( argv[2], "in" ) == 0 )
	{
		attrname = argv[1];
		relpath = argv[3];
	}
	else
		usage();


	/*
	 * Initialize the index.
	 */
	if ( ! indexch( relpath, attrname, &fp, &tree ) ) {
		prmsg( MSG_ERROR, "cannot open index on attribute '%s' in relation '%s'",
			relpath, attrname );
		usage();
	}

	/*
	 * Now get the tuples
	 */
	status = rdindexed( tree, "", &found, &seekval );
	while( status != END ) {
		prtuplelocs( found, fp, seekval );
		status = rdnext( tree, &found, &seekval );
	}

	fclose( fp );
	close( tree->fd );

	exit( 0 );
}

usage()
{
	prmsg( MSG_USAGE, "<attribute> <relation>" );
	exit( 2 );
}

prtuplelocs( attrval, fp, seekval )
char *attrval;
FILE *fp;
long seekval;
{
	long reccnt;
	long tplseekval;

	reccnt = 0;
	fseek( fp, seekval, 0 );
	fread( (char *)&reccnt, sizeof( long ), 1, fp );
	if ( reccnt >= 0 ) {
		prmsg( MSG_ERROR, "read positive record count (%ld) for attribute value '%s'",
			reccnt, attrval );
		return;
	}

	reccnt *= -1;
	while( reccnt-- > 0 ) {
		fread( (char *)&tplseekval, sizeof( long ), 1, fp );
		prmsg( MSG_ASIS, "%s\01%08ld\n", attrval, tplseekval );
	}
}
