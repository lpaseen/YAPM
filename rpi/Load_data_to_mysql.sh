#!/bin/bash
#Purpose:Load latest data to mysql db with help of Load_data.pl
#
#
cd ~/powermon/logs
if ls powermon_data_201?-*csv &>/dev/null;then
    echo
    echo "================================================================"
    date +%F\ %T
    mv powermon_data_201?-*csv TODO
    cd TODO
    cat powermon_data_*.csv|~/powermon/Load_data.pl
    # Update the summary table with latest news
#Not needed - I think    mysql -upowermon -penergymon powermon -e "source ~/powermon/Fillin_summarytable.sql"
    # FIXME, the files moved might been created last month
    # mv powermon_data_201?-*csv ../DONE/$(date +%Y-%m)
    for i in powermon_data_201?-*csv;do
	DSTYM=$(echo "$i" |sed 's/.*data_20\([0-9]\)/20\1/'|cut -d- -f1,2)
	mv $i ../DONE/$DSTYM/
    done
    cd ~/powermon
fi
