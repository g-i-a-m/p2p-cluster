#!/bin/sh
ulimit -c unlimited
bFirstStartup=1
exe=coturn_proxy
export LD_LIBRARY_PATH=./
while true; do
	server=`ps aux | grep ${exe} | grep -v grep`
		if [ ! "$server" ]; then
			if [ $bFirstStartup -eq 1 ]; then
				bFirstStartup=0
				echo "${exe} process started..."
			else
				echo "${exe} process not exist, now started..."
			fi
			nohup /usr/bin/${exe} -c 1 &
			sleep 10
		fi
	sleep 5
done
