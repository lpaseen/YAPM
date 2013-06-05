#!/bin/bash
#
#Purpose: Load collected data to mysql db


~/powermon/Load_data_to_mysql.sh >>~/powermon/Load_data_to_mysql_$(date +%A).log 2>&1
#Remove tomorrows file so we start clean then
rm -f ~/powermon/Load_data_to_mysql_$(date +%A -d tomorrow).log
