.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH "HIGH LEVEL FUNCTIONS" "UNITY" "" ""
.SA 1
.SH NAME
Uinthl \- High Level Function Interface
.SH SYNOPSIS
.nf
uclean()                                     /* cleanup function */

alter(argc,argv) int argc; char *argv[];     /* alter command */
asort(argc,argv) int argc; char *argv[];     /* asort command */
catalog(argc,argv) int argc; char *argv[];   /* catalog command */
compute(argc,argv) int argc; char *argv[];   /* compute command */
ddist(argc,argv) int argc; char *argv[];     /* ddist command */
delete(argc,argv) int argc; char *argv[];    /* delete command */
dis(argc,argv) int argc; char *argv[];       /* dis command */
dist(argc,argv) int argc; char *argv[];      /* dist command */
ei(argc,argv) int argc; char *argv[];        /* ei command */
et(argc,argv) int argc; char *argv[];        /* et command */
format(argc,argv) int argc; char *argv[];    /* format command */
index(argc,argv) int argc; char *argv[];     /* index command */
insert(argc,argv) int argc; char *argv[];    /* insert command */
load(argc,argv) int argc; char *argv[];      /* load command */
merge(argc,argv) int argc; char *argv[];     /* merge command */
nprintf(argc,argv) int argc; char *argv[];   /* nprintf command */
outerjoin(argc,argv) int argc; char *argv[]; /* outerjoin command */
packet(argc,argv) int argc; char *argv[];    /* packet command */
regjoin(argc,argv) int argc; char *argv[];   /* regjoin command */
setdiff(argc,argv) int argc; char *argv[];   /* setdiff command */
ustat(argc,argv) int argc; char *argv[];     /* stat command */
table(argc,argv) int argc; char *argv[];     /* table command */
tuple(argc,argv) int argc; char *argv[];     /* tuple command */
ucheck(argc,argv) int argc; char *argv[];    /* ucheck command */
uedit(argc,argv) int argc; char *argv[];     /* uedit command */
uenter(argc,argv) int argc; char *argv[];    /* uenter command */
ujoin(argc,argv) int argc; char *argv[];     /* ujoin command */
unload(argc,argv) int argc; char *argv[];    /* unload command */
unmerge(argc,argv) int argc; char *argv[];   /* unmerge command */
uprint(argc,argv) int argc; char *argv[];    /* uprint command */
uprintf(argc,argv) int argc; char *argv[];   /* uprintf command */
uselect(argc,argv) int argc; char *argv[];   /* uselect command */
.fi
.SH DESCRIPTION
The UNITY high level function interface provides a single function
call to execute each UNITY command.
The function name is same as UNITY main name (with the exception
of \fIstat\fP which is named \fIustat\fP).
Use of this interface saves overhead of \fIsystem\fP(3) function
call (the overhead of fork(2) and exec(2)).
.P
The parameters passed to the functions are in argc-argv format,
as if they were passed to the UNITY main program.
\fIargc\fP is the count of arguments in the array of character pointers.
The second argument, \fIargv\fP, is an array of character pointers which
point to the arguments for the command.
\fIargv\fP[0] should point to the name of the command being executed
(generally, used when printing error messages).
The character pointer beyond the last parameter must be set to NULL.
Unlike entering parameters using the shell interface,
special characters within parameters for the function interface should
not be escaped (i.e., they are not expanded by the shell).
.P
The return code from function is 0 if there are no errors and 1 if
an error occurs.
Most functions print error messages to the standard error output if
an error occurs.
This error printing can be turned off by setting the external
integer variable Unoerrpr to 1 (used in the \fIerror\fP() function).
.P
After EACH UNITY function call, the \fIuclean\fP() function MUST BE called.
This function closes all files opened by the UNITY function and
removes temporary and lock files,
One cannot avoid calling the \fIuclean\fP() function in hopes that the
lock file will be left around to protect other updates since the
lock file name changes for each update.
Similarly, one cannot avoid calling the \fIuclean\fP() function over several
function calls and expect all of the opened files to be closed
or temporary files removed.
For software calling UNITY functions which create a temporary file and/or
a lock file (especially the later), the calling program should
catch signals such as break and hangup and call the \fIuclean\fP() function
to clean up the files (see the example below).
.SH EXAMPLE
.nf
.sp
#include <signal.h>
extern int uclean();

static int
intr()
{
  /* ignore new signals */
  (void) signal(SIGINT,SIG_IGN);
  (void) signal(SIGQUIT,SIG_IGN);
  (void) signal(SIGHUP,SIG_IGN);
  (void) signal(SIGTERM,SIG_IGN);
  uclean();  /* uclean temp files */
  exit(1);
}

main(argc,argv)
int argc;
char *argv[];
{  
  char *myargv[10]; int ret;
  int ret;
  int (*istat)(), (*qstat)(), (*hstat)(), (*tstat)();

  /* catch signals to get rid of temp and lock files */
  if((istat = signal(SIGINT,intr)) == SIG_IGN)
	signal(SIGINT,SIG_IGN);
  if((qstat = signal(SIGQUIT,intr)) == SIG_IGN)
	signal(SIGQUIT,SIG_IGN);
  if((hstat = signal(SIGHUP,intr)) == SIG_IGN)
	signal(SIGHUP,SIG_IGN);
  if((tstat = signal(SIGTERM,intr)) == SIG_IGN)
	signal(SIGTERM,SIG_IGN);


  /* execute "alter city to paris in S where snumber leq s1" */
  myargv[0] = "alter"; myargv[1] = "city";
  myargv[2] = "to"; myargv[3] = "paris";
  myargv[4] = "in"; myargv[5] = "S";
  myargv[6] = "where"; myargv[7] = "snumber";
  myargv[8] = "leq"; myargv[9] = "s1";
  myargv[10] = NULL;

  /* do the alter command and cleanup */
  ret=alter(10,myargv);
  uclean();

  /* reset signals */
  signal(SIGINT,istat);
  signal(SIGQUIT,qstat);
  signal(SIGHUP,hstat);
  signal(SIGTERM,tstat);

  exit(ret);
}
.fi
.SH FILES
.nf
zalter.c, ... - source file names begin with z
dblib.a - the archive library file
.fi
.SH "SEE ALSO"
\fIUintll\fP(UNITY)
.SH BUGS
If output from a command is to go to the standard output,
it is always buffered in a buffer allocated using \fImalloc\fP(3).
At the end of the command, the buffer is flushed and freed,
and the output becomes unbuffered.
This will interfere with any buffering set up by the calling program.
