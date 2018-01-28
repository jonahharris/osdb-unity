.\"
.\" Copyright (C) 2002-2003 by Lucent Technologies
.\"
.TH rmulock 1 "" "New UNITY"
.SA 1
.SH NAME
rmulock \- Remove the lock file for a UNITY table.
.SH SYNOPSIS
.HP
.B rmulock
[-f]
.I <unity_table>...
.SH DESCRIPTION
The
.B rmulock
command attempts to remove the lockfile associated with
the specified UNITY data tables.
This command is used to unlock a table after a system crash
ocurred while a table was being modified,
e.g. using
.BR uedit (UNITY).
In order to remove the lockfile for a table, the user
must be the same as the login modifying the table
when the system crashed.
.P
If
.B rmulock
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
.B -f
Force the removal of the lockfile, even though normal
error checking would prevent it.
This option does not override normal file permissions;
it only ignores checks on the lockfile's size and mode.
.SH EXIT CODES          \" strongly recommended
.so Exit.codes.h
.SH RESTRICTIONS        \" optional - includes caveats and bugs
The owner of the lockfile is the only one who can remove it.
The login last modifying a data table owns the table's lockfile.
