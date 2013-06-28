#!/bin/bash
#
#Purpose: set the time of the arduino to same as system time

DEV=/dev/ttyACM0
[ ! -e "$DEV" ] && DEV=/dev/ttyUSB0
[ ! -e "$DEV" ] && DEV=/dev/ttyS0
SPEED=${1:-115200}
#SPEED=9600


if stty -F $DEV cs8 $SPEED ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts min 1 time 5;then
#    echo "Sending a 'set time' command and then reads a few lines of what comes back"
#    let wd=$(date +%w)+1
#    cat $DEV|head -99|grep -B6 'New time is' &
#    JOBID=$!
#    sleep 20 # seems like atmega need a lot of time
#    echo "T$(date +%y%m%d%H%M%S)$wd" >$DEV
#    while kill -0 $JOBID &>/dev/null;do sleep 1;done
#    echo
#    echo "Done"
    echo $(date +%T)"; Seems like we have a arduino connected, activation serial interface"
    DONE=false
    cnt=100
    cat $DEV|while read line;do 
        echo $(date +%T)"; $line"
	if ! $DONE;then 
	    if echo "$line"|grep -q "^Total sampling time";then 
		echo $(date +%T)"; Sending a 'set time' command and then reads a few lines of what comes back"
		sleep 1
		let wd=$(date +%w)+1;echo "T$(date +%y%m%d%H%M%S)$wd" >$DEV
		DONE=true
	    fi
	fi
	echo "$line"|grep -q 'New time is' && break
	let cnt--
	[ $cnt -le 0 ] && break
    done
else
    echo "stty failed for $DEV, abort"
fi
