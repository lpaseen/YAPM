#!/bin/bash
#
#Save the BUFF part to a separate part
#

DEV=/dev/ttyACM0
SPEED=115200


stty -F $DEV cs8 $SPEED ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts min 1 time 5


LOGDIR=$(eval echo ~/powermon/logs)
[ ! -e "$LOGDIR" ] && mkdir -p $LOGDIR

PREFIX=$LOGDIR/powermon_data_
EXT=csv

rm -f /tmp/quit.flg
INSIDE=false
time cat /dev/ttyACM0|while read line;do 
    if echo "$line"|grep -q "^#BEGIN";then
	INSIDE=true
	OUTFILE=${PREFIX}$(date +%F_%H%M).$EXT
	[ ! -e "$OUTFILE" ] && echo "$line" >>$OUTFILE
	[ -n "$VERBOSE" ] && echo "$line"
	continue
    fi
    if echo "$line"|grep -q "^#END";then
	INSIDE=false
	if [ -e /tmp/quit.flg ];then
	    rm -f /tmp/quit.flg
	    break
	fi
    else # if #END
	if $INSIDE;then
	    echo "$line" >>${OUTFILE}
	else
	    if [ -e /tmp/quit.flg ];then
		rm -f /tmp/quit.flg
		break
	    fi
	fi
	[ -n "$VERBOSE" ] && echo "$line"
    fi
done
