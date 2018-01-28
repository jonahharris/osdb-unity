/******************************************************************************

                ###############################################
                # Copyright 2002-2003 Lucent Technologies Inc #
                #              All Rights Reserved            #
                ###############################################

******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * New UNITY:
 *	cc -O -s -o re_unity re_unity.c ${TOOLS}/lib/unity/lib/libmisc.a
 *
 * Old UNITY:
 *	cc -O -s -o re_unity re_unity.c ${TOOLS}/lib/unity/lib/dblib.a \
 *					${TOOLS}/lib/unity/lib/libmisc.a
 */

#include <stdio.h>
#include <string.h>

main( argc, argv )
int argc;
char *argv[];
{
	extern char *strrchr();
	extern char *regcmp();
	char	*prog, *restr;
	int	i, j;
	char	buf[1024];

	prog = strrchr( argv[0] );
	if ( prog == NULL ) {
		prog = argv[0];
	} else {
		++prog;
	}

	if ( argc != 3 ) {
		fprintf( stderr, "Usage: %s <variable> <regexp>\n", prog );
		exit(1);
	}

	restr = regcmp( argv[2], (char*)0 );	/* libmisc.a */

	if (restr == NULL) {
		fprintf( stderr, "%s: ERROR: regcmp(UNITY) returned NULL\n", prog );
		exit(-1);
	}

	i = regexplen( restr );			/* libmisc.a */

	fprintf( stdout, "%c%c \"%s\" %c%c\n", '/', '*', argv[2], '*', '/' );
	fprintf( stdout, "char %s[] = {\n", argv[1] );

	j = 0;

	while ( i ) {
		if ( j >= 12 ) {
			fprintf( stdout, "\n" );
			j = 0;
		}
		fprintf(stdout, "0%o,", (unsigned char) *restr++ );
		--i;
		++j;
	}

	fprintf( stdout, "\n0};\n" );

	exit(0);
}
