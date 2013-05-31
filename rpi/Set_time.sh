#!/bin/bash
#
#Purpose: set the time of the arduino to same as system time

DEV=/dev/ttyACM0
[ ! -e "$DEV" ] && DEV=/dev/ttyUSB0
[ ! -e "$DEV" ] && DEV=/dev/ttyS0
SPEED=${1:-115200}
#SPEED=9600


if stty -F $DEV cs8 $SPEED ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts min 1 time 5;then
    echo "Sending a 'set time' command and then reads a few lines of what comes back"
    let wd=$(date +%w)+1
    cat $DEV|head -33 &
    JOBID=$!
    sleep 1;
    echo "T$(date +%y%m%d%H%M%S)$wd" >$DEV
#    cat $DEV|head -22
    while kill -0 $JOBID &>/dev/null;do sleep 1;done
    echo
    echo "Done"
else
    echo "stty failed for $DEV, abort"
fi
