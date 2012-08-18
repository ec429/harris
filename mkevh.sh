#!/bin/sh
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
