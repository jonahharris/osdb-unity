#!/bin/sh

#
# Copyright (C) 2002 by Lucent Technologies
#

#
#	@(#)uman.sh	1.3
cd $TOOLS/lib/unity/man
if [ $# -ne 0 ]; then
	for PAGE
	do
		nroff -man -T37 SAmacro $PAGE.U
	done | col
else
	for PAGE in *.U; do
		[ "`wc -l < $PAGE`" -ne 2 ] && nroff -man -T37 SAmacro $PAGE
	done | col
fi
