#!/bin/sh
cat <<EOM
/*
	harris - a strategy game
	Copyright (C) 2012-2013 Edward Cree

	licensed under GPLv3+ - see top of harris.c for details
	
	events.h: maps event names to ids
*/
EOM
num=0
names=""
while read name; do
	echo "#define EVENT_$name $num"
	names=$names+$name
	num=$(($num+1))
done
echo "#define NEVENTS $num"
names=${names#*+}
echo "const char *event_names[$num]={"
for i in `seq 0 $(($num-1))`; do
	name=${names%%+*}
	echo "\t\"${name}\","
	names=${names#*+}
done
echo "};"
