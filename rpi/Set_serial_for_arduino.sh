#!/bin/bash
#
#
#
# from http://playground.arduino.cc//Interfacing/LinuxTTY
#
# Also see http://todbot.com/blog/2006/12/06/arduino-serial-c-code-to-talk-to-arduino/


stty -F /dev/ttyACM0 9600  -parenb -parodd cs8 -hupcl -cstopb cread clocal -crtscts \
-ignbrk brkint ignpar -parmrk -inpck -istrip -inlcr -igncr -icrnl ixon -ixoff \
-iuclc -ixany -imaxbel -iutf8 \
-opost -olcuc -ocrnl -onlcr -onocr -onlret -ofill -ofdel nl0 cr0 tab0 bs0 vt0 ff0 \
-isig -icanon iexten -echo echoe echok -echonl -noflsh -xcase -tostop -echoprt \
echoctl echoke
