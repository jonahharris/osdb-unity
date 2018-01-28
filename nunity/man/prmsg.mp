.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH prmsg 3 "" "New UNITY"
.SH NAME                \" required
prmsg \- Print a formatted message.
.SH SYNOPSIS            \" required    
.nf
\fB#include "message.h"

extern char *prog;

void
prmsg( msgtype, formatstr, args, ... );
int msgtype;
char *formatstr;
char *args, ... ;\fP

char *
setmsgfmt( msgtype, msgformat )
int msgtype;
char *msgformat;
.fi
.SH DESCRIPTION         \" required - 1/2 page in length
The
.BR prmsg()
routine prints the message given in
.I formatstr
according to the type of message specified in
.IR msgtype .
The
.I formatstr
may contain any formatting constructs recognized by
.BR printf (3).
The
.I args
will be used as needed, depending on the format constructs in
.IR formatstr .
.P
Normally,
.BR prmsg ()
prints the message to
.B stderr
or
.BR stdout .
However, the routine may be redefined within applications
with other needs, such as printing the messages to a window.
.P
The external character pointer
.B prog
should be set by the application to the name of the application.
This will be used by
.BR prmsg ()
in the printed message.
.P
The
.BR setmsgfmt ()
routine allows redefinition of the message prefix for individual message types.
Each message is normally preceded by the command name and
a word or two saying what type of message it is.
The default message prefixes for each message type are explained below.
.SH PARAMETERS          \" required - if command takes parameters - alphabetical order
.TP
.I msgtype
The type of message to print.
This partially dictates the format of the message and
where the message will be printed.
Except as noted, all messages are printed to
.BR strderr .
The recognized message types are defined below, along with the
default prefix for each message:
.RS .5i
.TP
\f(CWMSG_USAGE\fP
Print a usage message for the application.
The string ``USAGE: '' is printed, followed by the program name, the message,
and a new-line.
.TP
\f(CWMSG_NOTE\fP
The message is an informative or status message for the user;
it does not represent an error message.
The message is printed to
.BR stdout .
The program name is printed, followed by ``: '', the message,
and a new-line.
.TP
\f(CWMSG_ERRNOTE\fP
The message is an informative or status message for the user,
just like \f(CWMSG_NOTE\fP,
but the message is printed to
.BR stderr .
This can be used for messages within filters where
.B stdout
is used for data.
This is the default message type if
.I msgtype
is not recognized.
.TP
\f(CWMSG_WARN\fP
The message reports a potential or non-serious error.
The program name is printed, followed by ``: WARNING: '', the message,
and a new-line.
.TP
\f(CWMSG_ALERT\fP
The message represents a non-fatal but potentially serious error.
The program name is printed, followed by ``: ALERT: '', the message,
and a new-line.
.TP
\f(CWMSG_ERROR\fP
The message represents a serious or fatal error.
The program name is printed, followed by ``: ERROR:'', the message,
and a new-line.
.TP
\f(CWMSG_INTERNAL\fP
The message represents an error in the logic or internal workings
of the application, e.g., errors that ``should not happen''
and array overflow.
The program name is printed, followed by ``: INTERNAL ERROR: '',
the message, and a new-line.
.TP
\f(CWMSG_QUESTION\fP
The message is a prompt for a question.
The message routine does not collect input for the answer, however.
The program name is printed, followed by ``: '', the message, and a space.
No new-line is printed after the message.
.TP
\f(CWMSG_DEBUG\fP
The message represents debugging information.
The program name is printed, followed by ``: DEBUG: '', the message,
and a new-line.
.TP
\f(CWMSG_ASIS\fP
Print only the message, exactly as is, to
.BR stdout .
The program name is not printed as part of the message.
This is used for messages of a form not covered by
the other message types.
.TP
\f(CWMSG_ERRASIS\fP
This is equivalent to \f(CWMSG_ASIS\fP, but the message is printed
on \f(CWstderr\fP.
.P
The following flags may be OR'ed with any of the above
message types:
.TP
\f(CWMSG_CONTINUE\fP
This flag may be passed in alone or with a message type.
If passed in without a message type,
it indicates a continuation of whatever was the last type of message.
In this case a tab is printed and then the message.
If OR'ed with other message types this flag indicates that
part of the message is still forthcoming in a subsequent call.
In this case a new-line is not printed at the end of the message.
.TP
\f(CWMSG_INDENT\fR(\fInum\fR)
Indent the message
.I num
indentation levels.
An indentation level is four spaces.
This flag is useful in cases such as recursive calls, where
the messages may be confusing or useless because
the level of recursion is not known.
.RE
.TP
.I formatstr
The message string.
This must be in a form recognized by
.BR printf (3).
.TP
.IR args ", ..."
The arguments useable by
.BR printf (3)
according to the formatting constructs in
.IR formatstr .
.TP
.I msgformat
The format of the message prefix for the given
.I msgtype.
The
.I msgformat
must be in a form recognized by
.BR printf (3)
and may contain at most one ``%s'' construct (for the command name).
No other formating constructs are allowed.
The routine does not check for compliance of this restriction.
.SH EXAMPLE             \" strongly recommended
Print an error message.
.P
.RS .5i
.nf
\f(CWprmsg( MSG_ERROR, "There has been a fatal error on file %s", "filename" );\fP
.fi
.RE
.P
Assuming that
.B prog
is set to ``program'', the output from this call would be
.P
.RS .5i
.nf
\f(CWprogram: ERROR: There has been a fatal error on file filename\fP
.fi
.RE
.SH SEE ALSO            \" strongly recommended
printf(3).
