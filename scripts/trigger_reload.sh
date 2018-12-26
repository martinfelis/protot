#!/bin/bash
PIDS=$(ps a | grep $1 | egrep -v "grep|$0" | sed -s 's/^\s//' | cut -d " " -f 1)
echo "PIDs: '$PIDS'"

for p in $PIDS; do
	kill -s USR1 $p
done

