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

static char recbuf[MAXREC],*recptr;
static char *size_msg = "getrec: maximum record size (%d) exceeded.\n";
static char *size_att = "getrec: maximum attribute size (%d) exceeded.\n";

int	end_of_tbl;	/* incremented on EOF when no data in recbuf[] */

newrec()
{
	recptr=recbuf;
}

getrec(file,fmt)
FILE *file; 
struct fmt *fmt;
{	
	static int nlinfw;	/* warn about NL in fixed width attribute */
	int i,c,eos;
	char *s;

	fmt->val = s = recptr;
	switch(fmt->flag) {
	case WN:
		for(eos=i=0;i < fmt->flen && (c=getc(file)) != EOF ;i++) {
			if ((c == '\n') && (!(eos))) {
				if (nlinfw == 0) {
					error( E_GENERAL, "getrec: WARNING: Input table contains embedded NL in fixed width attribute!\n");
					++nlinfw;
				}
			} else if (c == '\0') {
				++eos;
			}
			if(s < &recbuf[MAXREC])
				*s++ = c;
			else
			{
				if ( s == &recbuf[MAXREC] )
					error( E_GENERAL, size_msg, MAXREC );
				return( ERR );
			}
		}
		if (i < fmt->flen) {
			if ( s == &recbuf[0] )
				++end_of_tbl;
			return(ERR);
		}
		/* if first attribute in record is zero-width then check for EOF */
		if ((i == 0) && (s == &recbuf[0])) {
			if ((c=getc(file)) == EOF) {
				++end_of_tbl;
				return(ERR);
			} else {
				ungetc(c,file);
			}
		}
		break;
	case T:	
		while ( (c = getc( file )) != fmt->inf[1] )
		{
			int newch;

			switch( c ) {
			case EOF:
				if ( s != &recbuf[0] ) {
					error( E_GENERAL, "getrec: premature EOF encountered in parsing attribute value.\n" );
				} else {
					++end_of_tbl;
				}
				return( ERR );
/* #if 0 */
		/* Don't choke on embedded new-line, yet */
			case '\n':
				error( E_GENERAL, "getrec: embedded newline in attribute value.\n" );
				return( ERR );
/* #endif */
			case '\\':
				newch = getc( file );
				if ( newch != '\n'  && newch != '\0' && newch == fmt->inf[1] )
					c = newch;
				else
					(void)ungetc( newch, file );
				break;
			}

			if ( s < &recbuf[MAXREC] )
			{
				*s++ = c;
			}
			else
			{
				if ( s >= &recbuf[MAXREC] )
					error( E_GENERAL, size_msg, MAXREC );
				return( ERR );
			}
		}
		break;
	default:
		error(E_ERR,"getrec","getrec");
		return(ERR);
	}

	if( s < &recbuf[MAXREC])
	{
		if ( s - recptr >= DBBLKSIZE ) {
			error( E_GENERAL, size_att, DBBLKSIZE );
			return( ERR );
		}

		*s++='\0';
		recptr=s;
		return( 0 );
	}
	else
	{
		error( E_GENERAL, size_msg, MAXREC );
		return( ERR );
	}
}
