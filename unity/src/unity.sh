#!/bin/sh
#
# UNITY environment change:
#	"-r" - creates restricted environment
#	"-v" - prints version and exits
#

: ${TOOLS:=`logdir exptools`} ${UNITYDFILES:=$TOOLS/lib/unity/lib}

UNITYBIN=$TOOLS/lib/unity/bin	# don't export

PS1="unity> "

export PATH PS1 TOOLS SHELL UNITYDFILES

case "$1" in
-v )
	echo "UNITY version 2.6"
	echo 
	echo "Copyright (C) 2002 Lucent Technologies"
 	echo "UNITY comes with ABSOLUTELY NO WARRANTY; for details see"
	echo "$TOOLS/lib/unity/COPYING"
	echo "This is free software, and you are welcome to redistribute it"
	echo "under certain conditions; see $TOOLS/lib/unity/COPYING"

	exit 0
	;;
-r )
	PATH=:/rbin:/usr/rbin:/bin:/usr/bin:$UNITYBIN
	SHELL=rsh
	shift
	;;
* )
	PATH=$PATH:$UNITYBIN
	: ${SHELL:=/bin/sh}
	;;
esac

if [ $# != 0 ]; then
	exec "$SHELL" -c "$*"
else
	exec "$SHELL"
fi
