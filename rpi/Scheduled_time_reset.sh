#!/bin/bash
#Purose: Stop the normal collection job and set the arduiono RTC time
#
#

#First stop the collection job

echo Stopping the collection job
touch /tmp/quit.flg
for i in {1..100};do # Bail out after 100 sec, it may not be running
    [ ! -e /tmp/quit.flg ] && break
    date
    sleep 1
done

echo Set the time
~/powermon/Set_time.sh
#
#Start up the collection job again
[ -e /dev/ttyACM0 ] && nohup ~/powermon/SaveTTY_to_csv.sh &>/dev/null </dev/null &
