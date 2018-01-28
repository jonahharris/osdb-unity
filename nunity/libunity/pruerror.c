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
#include "urelation.h"
#include "uerror.h"
#include "message.h"

extern char *prog;

struct errmsg {
	int error_num;
	char do_perror;
	char internal;
	char *msg;
};

struct errmsg errlist[] = {

	UE_NOMEM,	FALSE,	TRUE,
		"cannot allocate memory",
	UE_ATTRNAME,	FALSE,	FALSE,
		"syntax error in attribute name",
	UE_STAT,	TRUE,	FALSE,
		"unsuccesful stat() on file for relation",
	UE_WOPEN,	TRUE,	FALSE,
		"open for writing on relation failed",
	UE_ROPEN,	TRUE,	FALSE,
		"open for reading on relation failed",
	UE_TMPOPEN,	TRUE,	FALSE,
		"open for writing on tmp-file Failed",
	UE_RELNAME,	FALSE,	FALSE,
		"unrecognized relation name",
	UE_NOTAB,	FALSE,	FALSE,
		"syntax error in descriptor file: no tab separator",
	UE_BSEOL,	FALSE,	FALSE,
		"syntax error in descriptor file: backslash at end of line",
	UE_TERMCH,	FALSE,	FALSE,
		"syntax error in descriptor file: unrecognized field termination char.",
	UE_ATTRWIDTH,	FALSE,	FALSE,
		"syntax error in descriptor file: attribute width unrecognized or too large",
	UE_ATTRTYPE,	FALSE,	FALSE,
		"syntax error in descriptor file: unrecognized attribute termination type",
	UE_PRWIDTH,	FALSE,	FALSE,
		"syntax error in descriptor file: unrecognized attribute print width",
	UE_JUST,	FALSE,	FALSE,
		"syntax error in descriptor file: unrecognized justification character",
	UE_NLTERM,	FALSE,	FALSE,
		"syntax error in descriptor file: no newline for last field",
	UE_NOINIT,	FALSE,	TRUE,
		"no initialization previously done",
	UE_CRLOCK,	TRUE,	FALSE,
		"cannot create lock file",
	UE_RELLOCK,	FALSE,	FALSE,
		"relation is currently locked",
	UE_WRITE,	TRUE,	FALSE,
		"unsuccessful write",
	UE_READ,	TRUE,	FALSE,
		"unsuccessful read",
	UE_LINK,	TRUE,	FALSE,
		"relation link failed -- original file contents may be in 'orig[0-9]*' file",
	UE_SAMEINIT,	FALSE,	TRUE,
		"initialization doesnt match current arguments",
	UE_NUMATTR,	FALSE,	FALSE,
		"too many attributes",
	UE_NUMREL,	FALSE,	FALSE,
		"too many relations",
	UE_TOOCMPLX,	FALSE,	FALSE,
		"where-clause too complex",
	UE_NODESCR,	FALSE,	FALSE,
		"cannot locate descriptor file",
	UE_DECOMPOSE,	FALSE,	TRUE,
		"query decomposition failed!! (SHOULD NOT HAPPEN!!)",
	UE_EOF,		FALSE,	FALSE,
		"premature end-of-file",
	UE_RECSYN,	FALSE,	FALSE,
		"syntax error in record",
	UE_BADQVERS,	FALSE,	TRUE,
		"compiled queries are out of date",
	UE_UNRECATTR,	FALSE,	FALSE,
		"unrecognized attribute name",
	UE_IDXFAIL,	FALSE,	FALSE,
		"index command failed",
	UE_DUPATTR,	FALSE,	FALSE,
		"duplicate attribute name encountered in descriptor file",
	UE_QUERYEVAL,	FALSE,	TRUE,
		"an internal error has occured while processing the query (SHOULD NOT HAPPEN!!)",
	UE_RECSIZE,	FALSE,	FALSE,
		"maximum record size exceeded",
	UE_ATTSIZE,	FALSE,	FALSE,
		"attribute size is larger than I/O block size",
	UE_NLINATTR,	FALSE,	FALSE,
		"embedded new-line in attribute value",
	UE_ATTWIDTH,	FALSE,	FALSE,
		"attribute width does not match descriptor file",
	UE_HASTAB,	FALSE,	FALSE,
		"friendly attribute name contains embedded tab",
	UE_OLDLINK,	TRUE,	FALSE,
		"link of original table to temporary file failed",
	UE_UNLINK,	TRUE,	FALSE,
		"unlink of original table failed -- original file contents in 'orig[0-9]*' file",
	UE_LASTCHBS,	FALSE,	FALSE,
		"the last character of the attribute value is a backslash (\\)",
	UE_TMPLOCK,	FALSE,	TRUE,
		"temporary file's lock file already exists - making changes would lock the table permanently",
	UE_ACMPMOD,	FALSE,	FALSE,
		"unrecognized attribute/comparison modifier",
	UE_RPOPEN,	TRUE,	FALSE,
		"failed to open pipe to unpack relation",
	UE_PACKEDEOF,	FALSE,	FALSE,
		"failed to unpack relation",
	UE_UNPACKMOD,	FALSE,	FALSE,
		"cannot modify packed relation",
	UE_TPLERROR,	FALSE,	FALSE,
		"tuple read failed due to preceding tuple error(s)",
	UE_STATDIR,	TRUE,	FALSE,
		"unsuccesful stat() on relation directory",
	UE_LOCKSTAT,	FALSE,	FALSE,
		"corrupt or missing lock file",
	UE_NOLOCK,	FALSE,	TRUE,
		"could not find lock file in internal lock file table",
	UE_LOCKSTMP,	FALSE,	FALSE,
		"corrupt or missing lock file -- modified table left in tmp file (itmpXXXXXX or newXXXXXX)",
	UE_BSINATTR,	FALSE,	FALSE,
		"embedded backslash in backslash terminated attribute value",
};

#define MAXERRNO (sizeof( errlist ) / sizeof( struct errmsg ))

static char ue_lockstmp_tmpfile[ MAXPATH+4 ];	/* allow for "/./" or "././" prefix */

void
set_ue_lockstmp( tmpfile )
char *tmpfile;
{
	if ( tmpfile ) {
		strncpy( ue_lockstmp_tmpfile, tmpfile, MAXPATH+3 );
	} else {
		*ue_lockstmp_tmpfile = '\0';
	}
	set_uerror( UE_LOCKSTMP );
}

int
pruerror( )
{
	int olderr;
	register struct errmsg *errptr;
	register int i;

	if ( uerror != UE_NOERR )
	{
		for( i = 0, errptr = errlist; i < MAXERRNO; i++, errptr++ )
		{
			if ( errptr->error_num == uerror )
			{
				if ( uerror == UE_LOCKSTMP ) {
					prmsg( MSG_ERROR, "corrupt or missing lock file" );
					if ( *ue_lockstmp_tmpfile ) {
						prmsg( MSG_CONTINUE,
							"Modified table left in %s",
							ue_lockstmp_tmpfile );
					} else {
						prmsg( MSG_CONTINUE,
							"Modified table left in tmp file %s",
							"(itmpXXXXXX or newXXXXXX)" );
					}
					*ue_lockstmp_tmpfile = '\0';
					break;
				}
				if ( errptr->do_perror )
					perror( prog );
				prmsg( errptr->internal ?
						MSG_INTERNAL : MSG_ERROR,
					errptr->msg );
				break;
			}
		}
		if ( i >= MAXERRNO )	/* error not found */
			prmsg( MSG_INTERNAL, "cannot find error message for error code '%d'",
				uerror );
	}

	olderr = uerror;
	reset_uerror( );

	return( olderr );
}
