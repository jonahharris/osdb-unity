/*
 * Copyright (C) 2002 by Lucent Technologies
 *
 */

#define MSG_INTERNAL	0x01	/* print internal tool error message */
#define MSG_ERROR	0x02	/* print normal error message */
#define MSG_ALERT	0x03	/* print alert message */
#define MSG_WARN	0x04	/* print warning message */
#define MSG_NOTE	0x05	/* print a note to stdout */
#define MSG_USAGE	0x06	/* print out usage message */
#define MSG_QUESTION	0x07	/* print out a question */
#define MSG_ERRNOTE	0x08	/* print a note to stderr */
#define MSG_ASIS	0x09	/* Just print the msg as is to standard output */
#define MSG_ERRASIS	0x0a	/* Just print the msg as is to standard error */
#define MSG_DEBUG	0x0b	/* print debug message */
#define MSG_STRICT      0x0c    /* warn or error depending on application */

#define MSG_CONTINUE	0x10	/* continue a previous message -- */
				/* can be OR'ed with other types */
#define MSG_QUIET       0x20    /* print if in quiet mode */
                                /* can be OR'ed with other types -- */
                                /* mutually exclusive with MSG_VERBOSE */
#define MSG_VERBOSE     0x40    /* print only if in verbose mode */
                                /* can be OR'ed with other types -- */
                                /* mutually exclusive with MSG_QUIET */
#define MSG_IGNORE      0x80    /* don't print this message */
                                /* can be OR'ed with other types */


#define MSG_MASK	0x0f	/* mask for getting message type */

#define MSG_INDENT(x)	((x) << 8)	/* indentation for messages */

extern char *msg_prog();
extern int msg_errorcnt();
extern int msg_warncnt();
extern void reset_errorcnt();
extern int msg_strict();
extern int *msg_lineno();
extern char *msg_filename();
extern int msg_verbose();       /* don't print NOTE msgs w/ MSG_VERBOSE set */
extern int msg_quiet();         /* print NOTE msgs only if MSG_QUIET set */
extern int msg_normal();        /* ! verbose and ! quiet */

