#!/bin/bash
#Purose: Stop the normal collection job and set the arduiono RTC time
# cronjob version, no output by default
#


LOGFILE=~/powermon/logs/Scheduled_time_reset.log

/home/pi/powermon/Scheduled_time_reset.sh &>>$LOGFILE

