#!/bin/bash
for CMD in "gcc -c sysinfo.c -o sysinfo.o -W -Wall -O2" "gcc -c autoinfo.c -o autoinfo.o -W -Wall -O2" "gcc -c misc.c -o misc.o -W -Wall -O2" "gcc -c newrt.c -o newrt.o -W -Wall -O2" "gcc newrt.o sysinfo.o misc.o autoinfo.o -W -Wall -O2 -o newrtinfo"; do
	echo $CMD
	$CMD
done

exit 0

CMD="gcc rtnfo.c -o rtnfo -Wall -O2"

echo $CMD
$CMD

