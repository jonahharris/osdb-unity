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
#include <stdarg.h>
#include "message.h"

extern char *prog;

#define INDENTSIZE	4

/*
 * control where output goes
 */
static FILE *fp;			/* normal output file */
static FILE *msgfp;			/* special output file */

/*
 * Control how MSG_STRICT message are printed.
 * TRUE means error, FALSE means warning
 */
static char force_strict;

#define CHKMSGMODE( mode, type )        \
                ( ((mode) == MSG_QUIET && ((type) & MSG_QUIET) == 0) || \
                ((mode) != MSG_VERBOSE && ((type) & MSG_VERBOSE) != 0) )

/*
 * Filename and line number to put in messages
 */
static char *filename;
static int *lineno;

/*
 * Counts of errors and warnings printed
 */
static unsigned short errorcnt;
static unsigned short warncnt;


static char msgmode;	    /* quiet or verbose */

/*
 ***************************************************************
 */

/*
 * Change where messages print
 */
FILE *
setmsgoutput( f )
FILE *f;
{
	FILE *oldf;

	oldf = msgfp;
	msgfp = f;

	return( oldf );
}

/*
 * Set the meaning of strict.
 */
msg_strict( value )
char value;
{
        char tmp;

        tmp = force_strict;
        force_strict = value;

        return( tmp );
}

/*
 * Control the error counts.
 *
 * reset_errcnt() 	reset counts to zero
 * msg_errorcnt()	gets error count
 * msg_warncnt()	gets warning counts
 */
void
reset_errorcnt()
{
        errorcnt = 0;
        warncnt = 0;
}

msg_errorcnt()
{
        return( errorcnt );
}

msg_warncnt()
{
        return( warncnt );
}

/*
 * Control whether MSG_QUIET and MSG_VERBOSE messages
 * are printed.  The functions return TRUE if already
 * in the given mode.
 */
msg_quiet( )
{
        char tmp;

        tmp = msgmode;
        msgmode = MSG_QUIET;

        return( tmp == MSG_QUIET );
}

msg_verbose( )
{
        char tmp;

        tmp = msgmode;
        msgmode = MSG_VERBOSE;

        return( tmp == MSG_VERBOSE );
}

msg_normal( )
{
        char tmp;

        tmp = msgmode;
        msgmode = 0;

        return( tmp == 0 );
}

/*
 * Set up program name, file name, and line number
 * for messages.
 */
char *
msg_prog( name )
char *name;
{
        char *old;

        old = prog;
        prog = name;

        return( old );
}

char *
msg_filename( name )
char *name;
{
        char *old;

        old = filename;
        filename = name;

        return( old );
}

int *
msg_lineno( num )
int *num;
{
        int *old;

        old = lineno;
        lineno = num;

        return( old );
}

/*
 * Default message formats:
 */
static char *fmt_internal = "%s: INTERNAL ERROR: ";
static char *fmt_error = "%s: ERROR: ";
static char *fmt_alert = "%s: ALERT: ";
static char *fmt_warn = "%s: WARNING: ";
static char *fmt_note = "%s: ";
static char *fmt_usage = "USAGE: %s ";
static char *fmt_question = "%s: ";
static char *fmt_asis = "";
static char *fmt_debug = "%s: DEBUG: ";
static char *fmt_continue = "\t";
static char *fmt_default = "";

char *
setmsgfmt( msgtype, format )
int msgtype;
char *format;
{
	char *oldval;

	if ( format == NULL )
		format = "";

	/*
	 * If a strict message, force to warning or error
	 * based on force_strict.
	 */
        if ( ((msgtype) & MSG_MASK) == MSG_STRICT ) {
                (msgtype) = ((msgtype) & ~MSG_MASK) |
                        ((force_strict) ? MSG_ERROR : MSG_WARN);
        }

	switch( msgtype & MSG_MASK ) {
	case MSG_INTERNAL:
		oldval = fmt_internal;
		fmt_internal = format;
		break;
	case MSG_ERROR:
		oldval = fmt_error;
		fmt_error = format;
		break;
	case MSG_ALERT:
		oldval = fmt_alert;
		fmt_alert = format;
		break;
	case MSG_WARN:
		oldval = fmt_warn;
		fmt_warn = format;
		break;
	case MSG_NOTE:
	case MSG_ERRNOTE:
		oldval = fmt_note;
		fmt_note = format;
		break;
	case MSG_USAGE:
		oldval = fmt_usage;
		fmt_usage = format;
		break;
	case MSG_QUESTION:
		oldval = fmt_question;
		fmt_question = format;
		break;
	case MSG_ERRASIS:
	case MSG_ASIS:
		oldval = fmt_asis;
		fmt_asis = format;
		break;
	case MSG_DEBUG:
		oldval = fmt_debug;
		fmt_debug = format;
		break;
	default:
		if ( msgtype & MSG_CONTINUE ) {
			oldval = fmt_continue;
			fmt_continue = format;
		}
		else {
			oldval = fmt_default;
			fmt_default = format;
		}
		break;
	}

	return( oldval );
}


/*VARARGS2*/
vprmsg( msgtype, format, args )
int msgtype;
char *format;
va_list args;
{
	int indent;
	char *errfmt;
	char endchar;
	static int oldmsgtype;

	indent = (msgtype >> 8);

	/*
	 * If a strict message, force to warning or error
	 * based on force_strict.
	 */
        if ( ((msgtype) & MSG_MASK) == MSG_STRICT ) {
                (msgtype) = ((msgtype) & ~MSG_MASK) |
                        ((force_strict) ? MSG_ERROR : MSG_WARN);
        }

	switch( msgtype & MSG_MASK ) {
	case MSG_INTERNAL:
                errorcnt++;
		errfmt = fmt_internal;
		endchar = '\n';
		fp = stderr;
		break;
	case MSG_ERROR:
                errorcnt++;
		errfmt = fmt_error;
		endchar = '\n';
		fp = stderr;
		break;
	case MSG_ALERT:
                warncnt++;
		errfmt = fmt_alert;
		endchar = '\n';
		fp = stderr;
		break;
	case MSG_WARN:
                warncnt++;
		errfmt = fmt_warn;
		endchar = '\n';
		fp = stderr;
		break;
	case MSG_NOTE:
                if ( CHKMSGMODE( msgmode, msgtype ) )
                {
                        oldmsgtype = msgtype;   /* for MSG_CONTINUE */
                        return;
                }
		errfmt = fmt_note;
		endchar = '\n';
		fp = stdout;
		break;
	case MSG_ERRNOTE:
                if ( CHKMSGMODE( msgmode, msgtype ) )
                {
                        oldmsgtype = msgtype;   /* for MSG_CONTINUE */
                        return;
                }
		errfmt = fmt_note;
		endchar = '\n';
		fp = stderr;
		break;
	case MSG_USAGE:
                errorcnt++;
		errfmt = fmt_usage;
		endchar = '\n';
		fp = stderr;
		break;
	case MSG_QUESTION:
		errfmt = fmt_question;
		endchar = ' ';
		fp = stderr;
		break;
	case MSG_ERRASIS:
		errfmt = fmt_asis;
		endchar = 0;
		fp = stderr;
		break;
	case MSG_ASIS:
		errfmt = fmt_asis;
		endchar = 0;
		fp = stdout;
		break;
	case MSG_DEBUG:
		errfmt = fmt_debug;
		endchar = '\n';
		fp = stderr;
		break;
	default:
                if ( msgtype & MSG_CONTINUE )
                {
                        if ( oldmsgtype & MSG_IGNORE )
                                return;	/* don't print the message */

                        switch( oldmsgtype & MSG_MASK ) {
                        case MSG_NOTE:
                        case MSG_ERRNOTE:
                                if ( CHKMSGMODE( msgmode, oldmsgtype ) )
                                {
                                        return;
                                }
                                break;
                        }

			errfmt = fmt_continue;
			if ( fp == NULL )
				fp = stderr;
		}
		else {
			errfmt = fmt_default;
			fp = stderr;
		}
		endchar = '\n';
		break;
	}

        oldmsgtype = msgtype;

        if ( msgtype & MSG_IGNORE )
                return;         /* don't print the message */

	if ( msgfp )		/* redirect output to special file */
		fp = msgfp;

	if ( indent > 0 )
		fprintf( fp, "%*s", indent * INDENTSIZE, "" );

	if ( format && *format ) {
		fprintf( fp, errfmt, prog ? prog : "" );

                if ( (msgtype & MSG_MASK) != MSG_USAGE )
                {
                        if ( filename )
                                fprintf( fp, "%s%s ", filename,
                                        lineno ? "" : ":" );
                        if ( lineno )
                                fprintf( fp, "(line %u): ", *lineno );
                }

                vfprintf( fp, format, args );

		if ( endchar )
			putc( endchar, fp );		
	}
}

prmsg( int msgtype, char *format, ... )
{
        va_list args;

        va_start( args, format );
        vprmsg( msgtype, format, args );
        va_end( args );
}

