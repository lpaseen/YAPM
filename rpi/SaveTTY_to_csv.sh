#!/bin/bash
#
#Save the BUFF part to a separate part
#


DEV=/dev/ttyACM0
SPEED=115200

#Check that the port is there, if not just silently exit
[ ! -e $DEV ] && exit

#Only one instance should run

[ ! -e ~/tmp ] && mkdir ~/tmp
lockfile=~/tmp/SaveTTY_to_csv.lck

me=$$
echo $me >>$lockfile
l=`wc -l ${lockfile}|awk '{print $1}'`
[ $l -gt 1 ] && echo It is $l lines left in the lockfile
winner=`head -1 $lockfile`
while [ "$me" != "$winner" -a -s ${lockfile} ];do
    [ -e /proc/$winner/cmdline ] && running=$(cat /proc/$winner/cmdline) || running=""
echo "running=>$running<"
    if ! echo "$running"|grep -q $0 ;then # seems like it's the sh process that gets the lock
	echo The winner was :$winner: but that process could not be found and is removed
	echo -e '1\nd\nw\nq\n'|ed $lockfile &>/dev/null
	winner=`head -1 $lockfile`
    else
        echo running=:$running:
        ps -ef|grep $winner|grep -v grep
        echo LOCK:$myName already out there, aborting
        exit 2
    fi
done

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
		sed -i "/$$/d" $lockfile
		[ ! -s $lockfile ] && rm -f $lockfile
		break
	    fi
	fi
	[ -n "$VERBOSE" ] && echo "$line"
    fi
done
