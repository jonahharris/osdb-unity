.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH setunpackenv 3 "" "New UNITY"
.SH NAME                \" required
setunpackenv \- Setup environment to read packed relations.
.SH SYNOPSIS            \" required
.nf
extern char *packedcatcmd;
extern char *packedsuffix;

void
setunpackenv()
.fi
.SH DESCRIPTION         \" required - 1/2 page in length
In order to allow New Unity primitives to read packed (compressed) relations,
application programs must initialize two global variables,
\fIpackedcatcmd\fP and \fIpackedsuffix\fP,
which define the command to be called to read (cat) the contents
of a packed relation (file) and the 1-3 character suffix (without the '.')
that is appended to the filename when the file has been packed (compressed).
.P
The \fIsetunpackenv\fP() routine reads the
\fIUNITYUNPACK\fP environment variable for a string of
the form "command.suffix" and initializes
\fIpackedcatcmd\fP to be "command" and
\fIpackedsuffix\fP to be "suffix".
The "command" string may include a path ('/' characters) but cannot contain
any dot ('.') characters.
The "suffix" string must be 1-3 characters and must not contain
a dot ('.') or slash ('/') character.
If \fIUNITYUNPACK\fP is not set or is empty or if either "command" or "suffix"
is missing or invalid, then \fIpackedcatcmd\fP and \fIpackedsuffix\fP are set to NULL.
Typical values for "\fIUNITYUNPACK\fP" are "pcat.z", "zcat.Z", and "gzcat.gz".
.P
It should be noted that packed relations are not allowed as the target
of any type of insert, update, or delete operation.
In addition, using packed relations prevents the use of attribute indexes
and also prevents any packed tuples from being validated, see validate(UNITY).
.P
\fBWARNING\fP: The \fIsetunpackenv\fP() routine should never be used
by any application what it to be installed with set-UID or set-GID permissions
since there is nothing to stop the end-user from defining \fIUNITYUNPACK\fP
to reference private commands which can give the user unauthorized access
to the owners files.
In these cases, the applications should set the two global variables,
\fIpackedcatcmd\fP and \fIpackedsuffix\fP, directly using a fully qualified
path to a trusted command for reading any packed relations.
.SH ENVIRONMENT         \" optional
.TP
UNITYUNPACK
A string of the form "command.suffix" where "command" is the the name of
a command that can be used to read (cat) the contents of a packed relation (file)
and "suffix" is the 1-3 character suffix (without the '.') that is appended to
the filename when the file has been packed (compressed).
.SH EXAMPLE             \" strongly recommended
Allow read access to relations that have been packed using compress(1)
using the offical (trusted) version of zcat(1).
.P
.RS .5i
.nf
extern char *packedcatcmd;
extern char *packedsuffix;

packedcatcmd = "/usr/bin/zcat";
packedsuffix = "Z";
.fi
.RE
.P
Allow the user to define the command and suffix that indicates that
a relation has been packed.
.P
.RS .5i
.nf
extern void setunpackenv();

setunpackenv();
.fi
.RE
.SH SEE ALSO            \" strongly recommended
pcat(1), zcat(1), nalter(1), ndelete(1), ninsert(1), nuedit(1), retrieve(1), and
udiff(1).
