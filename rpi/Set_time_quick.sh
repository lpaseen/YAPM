#!/bin/bash
DEV=/dev/ttyACM0;[ -e "$DEV" ] && let wd=$(date +%w)+1 && echo "T$(date +%y%m%d%H%M%S)$wd" >$DEV
