.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH ulock 1 "" "New UNITY"
.SA 1
.SH NAME
ulock \- Create, remove, check, or report on lock files for UNITY tables
.SH SYNOPSIS
.HP
.B ulock
[-cfq]
[-E|-l|-lock|-u|-unlock]
.I <unity_table>...
.SH DESCRIPTION
The
.B ulock
command by default reports on the status of the lock file
for each specified UNITY table when
the lock (-l) or unlock (-u) option is not specified.
The lock states include LOCKED, UNLOCKED, or PLOCKED.
The PLOCKED state, which is only available on machines
with an SVR4 based Operating System,
means that the table is locked by
a process that is still running and normally should
not be unlocked via \fIulock\fP or rm(1) since the
Operating System will automatically remove the lock
on the lock file when the process dies.
.P
As an example,
this command can be used to unlock a table after a system crash
while a table was being modified using
.BR uedit (UNITY)
or any other (UNITY) table update command.
In order to remove the lockfile for a table, the user
must be the same as the login modifying the table
when the system crashed. If the force (-f) option
is specified then any user can remove the lock file as
long as they have read/write/execute permissions for
the directory that contains the lock file for the given table.
.P
If
.B ulock
cannot remove a lock file, below are the steps a
table owner can use to get around the lockfile:
.TP
1.
Copy the table to a new temporary name.
.TP
2.
Move the original table to yet another name using the
.BR mv (1)
command.
\fIDo not remove this file until the lockfile is removed
as part of periodic system cleanup on /tmp!
Otherwise, the locked table (or any other table) may
magically become locked.\fR
.TP
3.
Move the new temporary copy of the relation back to the original
table name.
.SH PARAMETERS
.TP
.B -c
Create a descriptor file for the output information.
.TP
.B -E
Check if lock file(s) exist without printing status report.
Exit code is zero (0) when all tables are locked.
Exit code is four (4) when none of the specified tables are locked.
Exit code is three (3) when some are locked and some are unlocked.
.TP
.B -f
Force table(s) to be locked or unlocked.
When used with the unlock (-u) option,
an attempt is made to remove the lock file(s)
regardless of the lockfile's size, mode, or owner
when normal error checking would prevent it.
This option does not override normal file permissions;
it only ignores checks on the lockfile's size and mode.
.IP
When -f is specified with the lock (-l) option,
\fIulock\fP will continue to lock all tables that
were specified and not report an error if some
are already locked.
Normally, \fIulock\fP will unlock any tables
that it has locked when it encounters a table
that it cannot lock (i.e., one of the tables
was already locked).
.TP
.BR -l | -lock
Lock the specified table(s).
.TP
.B -q
Quiet \(em do not print completion status message.
.TP
.BR -u | -unlock
Unlock the specified table(s).
.SH EXIT CODES          \" strongly recommended
.so Exit.codes.h
.SH RESTRICTIONS        \" optional - includes caveats and bugs
The owner of the lockfile is the only one who can (should) remove it.
The login last modifying a data table owns the table's lockfile.
