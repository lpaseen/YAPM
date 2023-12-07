YAPM
====

Yet Another Power Monitoring app

Basic plan on how it works (once done)

It uses 
* custom board with 12bit ADC (mcp3208) and 5.000V vref onboard to read sensor data
* one Arduino to collect basic data
* The arduino has a 1602 lcd screen with some buttons for display current info locally
* It also has a tinyRTC for time keeping
* that RTC has some onboard eeprom and a temp sensor
* 

Since the Arduino can't hold much data all data is sent out on the serial USB port.
A raspberry pi is connected to the port and receives the data for processing.
The rpi will store all values in a db and create a webpage with pretty
graphs on.
* http://energymon.techwiz.ca/
* http://grafana.techwiz.ca/public-dashboards/8b3ad5f780984947b2cb39bf2cf0eb90

Current state is pre alpha so a lot is left to do before it's even partly usable.

TODO list is to long to write down but it contains things like
*schematics:
*  change hardware to smd and jacks to stacked connectors
*  find a good way to connect the arduino
*  figure out why the midpoint isn't stable (as seen from adc), guess
   an op-amp is needed
*  maybe replace arduino with ESP or so, removing the need for rpi
*  
*arduino:
*  make library functions?
*  calculate power factor
*  verify the math
*  add digital filtering to even out values over time
*  (use eeprom to save a few more values?)
*  send out values ASAP, the buffering is not really useful.
*  
*rpi
* code to send to backend is written. csv file is stored locally.


