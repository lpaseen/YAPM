#!/bin/bash
#Purpose:Load latest data to mysql db with help of Load_data.pl
#
#
cd ~/powermon/logs
if ls powermon_data_2013-*csv &>/dev/null;then
    mv powermon_data_2013-*csv TODO
    cd TODO
    cat powermon_data_*.csv|~/powermon/Load_data.pl
    mv powermon_data_2013-*csv ../DONE/
    cd ~/powermon
fi
