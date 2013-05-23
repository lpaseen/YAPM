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

Since the Arduino can't hold much data all data is sent out on the serial USB port
a raspberry pi is connected to the port and taks in the data for processing
the rpi will store all values in a db and create a webpage with pretty graphs on

