/*
 * Alpha 3, added time
 * Alpha 4, added temperature
 * Alpha 5, started buffering
 * powermon3 alpha1, external adc over spi
 * powermon3 alpha2, major code cleanup
 * powermon3 alpha3, changed to do fixed number of cycles instead of look for zero crossings
 *
 *
 *
 * $Id: powermon3_alpha3.ino 127 2013-05-23 13:29:34Z peters $
 *
 */


// i2C/TWI lib, for date
#include <Wire.h>
#define DS1307_I2C_ADDRESS 0x68

// LCD Panel
#include <LiquidCrystal.h>
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

//For DS18S20 temp
#include <OneWire.h>
OneWire  ds(17);  // Initiate temp reading

//To talk to ADC and io mux
#include <SPI.h>
#define maxChannels 8
#define ADCcs 3 // MCP3208 spi-CS is on pin 3
#define MUXcs 2 // spi io->mux, CS on pin 2

//
unsigned long Vcc;
unsigned long stepV;
unsigned int  ch_LOWval[maxChannels];
unsigned int  ch_HIGHval[maxChannels];
unsigned long ch_samplingtime[maxChannels];
unsigned long valuesummary[maxChannels];
       double ch_sumV[maxChannels];
       double ch_sumI[maxChannels];
byte lastkey=1;
byte dispmode=1;
float tempC;

unsigned int MAXsamples; // How many samples required to cover 200ms (=12cycles at 50Hz or 10 at 60Hz)

// #define buffersize 40
#define buffersize 15
typedef struct valrec{
  unsigned long sampledate;  
  unsigned long sampletime;
  byte reported;
  int tempC;
  double mV[maxChannels];
};

valrec buffmV[buffersize];
int buffhead=0;

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;

extern int __heap_start, *__brkval; 

//max number of samples required to cover one cycle
//at 60Hz I can collect 75 samples on a ATmega328
//using mcp3208 I can get >45000 samples per second
//#define MAXsamples 1000
// min samples to collect before starting to check if crossed 0
// 75 for one cycle so this should way past peak but not yet at crossing
//#define MINsamples 100

// Bits of accuracy for the ADC
#define ADCbits 12
// 
const int ADCmax=pow(2,ADCbits);
const int ADChalf=ADCmax/2;

unsigned int ADCmidpoint=ADCmax/2; // will be set everytime readVoltage is executed

// CT: Voltage depends on current, burden resistor, and turns
//SCT-016
//0.01-120A
//3000:1, current only out
#define CT1_BURDEN_RESISTOR      46.2
#define CT1_TURNS                3000
// CT: Amps it's rated for
#define CT1_MAXAMPS              120
// CT: Volts given at max amps
#define CT1_MAXVOLT              2.5


// SCT-013-030
// 0-30A
// 1800:1
// Burden resistor: 62ohm
// 0-1V out

#define CT2_BURDEN_RESISTOR      62
#define CT2_TURNS                1800
// CT: Amps it's rated for
#define CT2_MAXAMPS              30
// CT: Volts given at max amps
#define CT2_MAXVOLT              1.0

// #define DEBUGTX
// #define DEBUGRX
//#define DEBUGVOLT
 
//Calibration coeficients
//These need to be set in order to obtain accurate results
//Set the above values first and then calibrate futher using normal calibration method described on how to build it page.
//double ICAL     = 1.046;
//double I_RATIO      = (long double)CT_TURNS / CT_BURDEN_RESISTOR * 5 / 1024 * ICAL;

//###############################################################################
unsigned int freeRam () {
//  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
//------------------------------------------------------------------------------
void fillStack() {
  extern int __bss_end;
//  extern uint8_t* __brkval;
  uint8_t* bgn = (uint8_t*)(__brkval ? __brkval : &__heap_start);
  uint8_t* end = (uint8_t*)&bgn - 10;
  while (bgn < end) *bgn++ = 0X55;
}
//------------------------------------------------------------------------------
unsigned int stackUnused() {
  uint8_t* bgn = (uint8_t*)(__brkval ? __brkval : &__heap_start);
  uint8_t* end = (uint8_t*)&bgn - 10;
  uint8_t* tmp = bgn;
  while (tmp < end && *tmp == 0X55) tmp++;
  return  tmp - bgn;
}

//###############################################################################
// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
  return ( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
  return ( (val/16*10) + (val%16) );
}

//###############################################################################
// 1) Sets the date and time on the ds1307
// 2) Starts the clock
// 3) Sets hour mode to 24 hour clock

// Assumes you're passing in valid numbers

void setDateDs1307(byte second,        // 0-59
byte minute,        // 0-59
byte hour,          // 1-23
byte dayOfWeek,     // 1-7
byte dayOfMonth,    // 1-28/29/30/31
byte month,         // 1-12
byte year)          // 0-99
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.write(decToBcd(second));    // 0 to bit 7 starts the clock
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));     
  Wire.write(decToBcd(dayOfWeek));
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.write(0x10); // sends 0x10 (hex) 00010000 (binary) to control register - turns on square wave
  Wire.endTransmission();
}

//###############################################################################
// Gets the date and time from the ds1307
void getDateDs1307(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year)
{
  // Reset the register pointer
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);

  // A few of these need masks because certain bits are control bits
  *second     = bcdToDec(Wire.read() & 0x7f);
  *minute     = bcdToDec(Wire.read());
  *hour       = bcdToDec(Wire.read() & 0x3f);  // Need to change this if 12 hour am/pm
  *dayOfWeek  = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month      = bcdToDec(Wire.read());
  *year       = bcdToDec(Wire.read());
}

//###############################################################################
// Read the keys on the lcd panel
byte readKey(){
  byte key=0;
  unsigned int x;

  x = analogRead (0); // the lcd panel use analog0 for key

  if (x < 100) {
    key=1; // Right
  }
  else if (x < 200) {
    key=2; // Up
  }
  else if (x < 400){
    key=3; //Down
  }
  else if (x < 600){
    key=4; //Left
  }
  else if (x < 800){
    key=5; //Select
  } // else it's no key pressed and 0 is returned
  return key;
} // readKey

//###############################################################################
unsigned int adcRead(byte channel){
  int hibit,ret,ret2;
  
  uint8_t tx[] = {
    //  00000sgc   ccxxdddd   DDDDDDDD'
    0b00000110,0b00000000,0b00000000,
  };
  uint8_t rx[] = {
    //  00000sgc   ccx0dddd   DDDDDDDD'
    0,0,0
  };
  hibit=(channel&4)>>2;
  //  channel=channel&3;
  tx[0]+=hibit;
  tx[1]+=(channel&3)<<6;
  tx[2]=0;
  
  
  //need to handle the mux also, used for any channel over 6
  //but we deal with that later
  
#ifdef DEBUGTX
  Serial.print("TX Channel # ");
  Serial.print(channel,DEC);
  Serial.print(",");
  Serial.print(tx[0],HEX);
  Serial.print(" ");
  Serial.print(tx[1],HEX);
  Serial.print(" ");
  Serial.println(tx[2],HEX);
#endif
  // take the SS pin low to select the chip:
  digitalWrite(ADCcs,LOW);
  rx[0]=SPI.transfer(tx[0]);
  rx[1]=SPI.transfer(tx[1]);
  rx[2]=SPI.transfer(tx[2]);
  digitalWrite(ADCcs,HIGH);
  
#ifdef DEBUGRX
  Serial.print("RX Channel # ");
  Serial.print(channel,DEC);
  Serial.print(",");
  if (rx[0]<16) Serial.print("0");
  Serial.print(rx[0],HEX);
  Serial.print(" ");
  if (rx[1]<16) Serial.print("0");
  Serial.print(rx[1],HEX);
  Serial.print(" ");
  if (rx[2]<16) Serial.print("0");
  Serial.print(rx[2],HEX);
  Serial.print(" = ");
  Serial.println(((rx[1] & 0x0f)<<8)+rx[2],DEC);
  //    Serial.println();
#endif

  return ((rx[1] & 0x0f)<<8)+rx[2];
} // adcRead


//###############################################################################
double readVolt(byte channel,double RATIO){
  unsigned int _sample;
  unsigned int _samples;
  unsigned int _value;
  unsigned int _MinVal;
  unsigned int _MaxVal;
  unsigned int _midpoint;
  
  // http://openenergymonitor.org/emon/buildingblocks/measuring-voltage-with-an-acac-power-adapter
  //
  //  adapter output voltage = mains voltage x transformer ratio
  //  input pin voltage = adapter output voltage / 11
  //      RATIO=input pin voltage
  //  counts = (input pin voltage / 3.3) x 1024
  //      (sample/Vref)*ADCmax
  //  Vmains = count x a constant
  //
  //    In software, in order to convert the count back to a meaningful voltage, it has to be multiplied by a calibration constant.
  //
  // peak-voltage-output = R1 / (R1 + R2) x peak-voltage-input = 10k / (10k + 100k) x 12.7V = 1.15V
  // 

  //find the peak value (we just need max voltage, no need for fancy rms
  
  _MinVal=ADCmax-1;
  _MaxVal=1;
  // follow it to min value
  for (_sample=0;_sample<MAXsamples;_sample++){
    _value=adcRead(channel);
    _MinVal=min(_MinVal,_value);
    _MaxVal=max(_MaxVal,_value);
  }

  ADCmidpoint=((_MaxVal-_MinVal)/2)+_MinVal;
  ch_LOWval[channel]=_MinVal;
  ch_HIGHval[channel]=_MaxVal;

#ifdef DEBUGVOLT
  Serial.print("ReadVolt; samples, min/max/mid: ");
  Serial.print(_sample);
  Serial.print(",");
  Serial.print(_MinVal);
  Serial.print("/");
  Serial.print(_MaxVal);
  Serial.print("/");
  Serial.println(ADCmidpoint);
#endif

  ch_sumV[channel]=max(_MaxVal-ADCmidpoint,0)*stepV/RATIO; // Main voltage (110-120V AC)
  ch_sumI[channel]=max(_MaxVal-ADCmidpoint,0)*stepV; // mV measured
  valuesummary[channel]=_MaxVal; // raw max value

  return ch_sumV[channel];
} //readVolt


//###############################################################################
double readrmsI(byte channel,byte CT_AMPS,double CT_VOLT){
  unsigned int sample;
  int value;
  unsigned long _sum;
  unsigned int _sample;
  unsigned int _value;
  unsigned int _MinVal;
  unsigned int _MaxVal;
  unsigned int _midpoint;
  
  // current constant = (100 / 0.050) / 18 = 111.11
  // a constant = current constant x (3.3 / 1024)
  // Isupply = count x a constant
  // 
  // secondary current = primary current / transformer ratio
  // input pin voltage = secondary current x burden resistance
  // counts = (input pin voltage / 3.3) x 1024

  _sum=0;
  _MinVal=ADCmax-1;
  _MaxVal=1;  
  for (_sample=0;_sample<MAXsamples;_sample++){
    _value=adcRead(channel)-ADCmidpoint;
    _MinVal=min(_MinVal,_value);
    _MaxVal=min(_MaxVal,_value);
    _sum+=_value*_value;
  }

  ch_LOWval[channel]=_MinVal;
  ch_HIGHval[channel]=_MaxVal;

//rms
  ch_sumV[channel]=sqrt((double(_sum)/_sample)*stepV*stepV)/1000; // milliV
  ch_sumI[channel]=ch_sumV[channel]*CT_AMPS/CT_VOLT/1000;
  //  samples[channel]=_sample;
  valuesummary[channel]=_sum;
  return ch_sumI[channel];
} // readrmsI

//###############################################################################
void CheckKeys(){
  byte key;
  
  key=readKey();
  if ( key > 0 && key != lastkey){
    if ( key == 1 ) { // right
      dispmode++;
      if (dispmode > 5) dispmode=5;
    } else if (key == 4) { // left
      dispmode--;
      if ( dispmode<1) dispmode=1;
    }
    lastkey=key;
    lcd.setCursor(15,0);
    lcd.print(dispmode);
  } else if ( key == 0) { // key was released, get ready for next key
    lastkey=key;
  }
} // CheckKeys


//###############################################################################
// Read temp from the first found sensor (should only be one)

float GetTempC(){
  byte i;
  byte data[12];
  byte addr[8];
  byte present = 0;
  byte type_s;
  
  ds.search(addr);
  if (OneWire::crc8(addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return -1;
  }
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      type_s = 1;
      break;
    case 0x28:
      type_s = 0;
      break;
    case 0x22:
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return -1;
  }
  ds.reset();
  ds.select(addr);
  ds.write(0x44,1);         // start conversion, with parasite power on at the end
  // this is from the RTC=no parasite power
  // Other setups should check power type during setup()
  // delay(1000);
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }
  // convert the data to actual temperature

//  unsigned int raw = (data[1] << 8) | data[0];
  int raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // count remain gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
    } else {
    unsigned char t_mask[4] = {0x7, 0x3, 0x1, 0x0};
    byte cfg = (data[4] & 0x60) >> 5;
    raw &= ~t_mask[cfg];
    }
  tempC = (float)raw / 16.0;
} // GetTempC


//###############################################################################
void printValue(int i){
  byte channel;
  Serial.print("BUFF,");  
  Serial.print(i,DEC);  
  Serial.print(",");
  Serial.print(buffmV[i].sampledate,DEC);  
  Serial.print(",");  
  Serial.print(buffmV[i].sampletime,DEC);  
  Serial.print(",");  
  Serial.print(buffmV[i].tempC,DEC);
  Serial.print(",");  
  Serial.print(buffmV[i].reported,DEC);
  for (channel=0;channel<maxChannels;channel++){
    Serial.print(",");  
    Serial.print(buffmV[i].mV[channel]);
  }
    Serial.print(",");
    Serial.print(freeRam(), DEC);
    Serial.print(",");
    Serial.println(stackUnused(),DEC);
//  Serial.println();
} // printValue

//###############################################################################
void dumpBuff(boolean all,boolean mark){
  byte channel;
  boolean header=false;
  
  for (buffhead=0;buffhead<buffersize;buffhead++){
    if (all || buffmV[buffhead].reported==0) {
      if (buffmV[buffhead].sampletime==0) continue; // skip empty slots
      if (!header){
        Serial.println("#BEGIN dump buffer");
        header=true;
      }
      printValue(buffhead);
      if (mark) buffmV[buffhead].reported++;
    }
  }
  if (header) Serial.println("#END dump buffer");
}

//###############################################################################
/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n' || inChar == '\\' || inputString.length() >= 20){ // Buffer full, work on it
        stringComplete = true; 
        break;
    }
  }
}
//###############################################################################

void CheckSerial(){
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

  serialEvent();
  if (stringComplete){
    Serial.print("# string complete: ");
    Serial.println(inputString);
    if (inputString.startsWith("T")) { //Set time
;/*      year=byte(inputString.substring(1,2));
      month=byte(inputString.substring(3,4));
      dayOfMonth=byte(inputString.substring(5,6));
      hour=byte(inputString.substring(7,8));
      minute=byte(inputString.substring(9,10));
      second = byte(inputString.substring(11,12));
      dayOfWeek=byte(inputString.substring(13,13));
      setDateDs1307(second, minute, hour, dayOfWeek, dayOfMonth, month, year);
      */
    } else if (inputString.startsWith("D")) { // dump any new data and mark it viewed
      dumpBuff(false,true);
    } else if (inputString.startsWith("A")) { // dump all data and mark it viewed
      dumpBuff(true,true);
    } else if (inputString.startsWith("S")) { // show all data but do NOT mark it viewed
      dumpBuff(true,false);
    }
    // clear the string:
    inputString = "";
    stringComplete = false;    
  }
} // CheckSerial
//###############################################################################
//###############################################################################
//###############################################################################
//###############################################################################
void setup()
{
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

  // i2c, date
  Wire.begin();

  // Change these values to what you want to set your clock to.
  // You probably only want to set your clock once and then remove
  // the setDateDs1307 call.

  year = 13;
  month = 2;
  dayOfMonth = 1;
  hour = 14;
  minute = 2;
  second = 0;
  dayOfWeek = 6;
  //  setDateDs1307(second, minute, hour, dayOfWeek, dayOfMonth, month, year);

  //to track memory usage
  fillStack();


  // set the slaveSelectPin as an output:
  pinMode (ADCcs, OUTPUT);
  digitalWrite(ADCcs,HIGH);

  // initialize SPI:
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  // SPI.setClockDivider(SPI_CLOCK_DIV16); // 16MHZ div by 2,4,..,128
  SPI.setClockDivider(SPI_CLOCK_DIV4); // 16MHZ div by 2,4,..,128
  SPI.begin();

  Vcc=5000; //external prec vref
  stepV=(Vcc*1000)/ADCmax; // stepV = milliVolt*1000 per ADC step

  Serial.begin(115200);
//  Serial.begin(9600);
  Serial.print("Buffer size: ");
  Serial.print(buffersize);
  Serial.write(", stackUnused: ");
  Serial.print(stackUnused());
  Serial.write(", freeRam: ");
  Serial.println(freeRam());
  
  Serial.print("#channel");
  Serial.print(",");
  Serial.print("date");
  Serial.print(",");
  Serial.print("stepV");
  Serial.print(",");
  Serial.print("tempC");
  Serial.print(",");
  Serial.print("sum");
  Serial.print(",");
  Serial.print("samples");
  Serial.print(",");
  Serial.print("sqrt(valuesummary/samples)");
  Serial.print(",");
  Serial.print("sum-mV");
  Serial.print(",");
  Serial.println("sum-A");
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.clear();
} // setup

// ############################################################################
void loop()
{

  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;

  unsigned long sampledate,sampletime; // date & time the sample was done
  unsigned long samplingtime,timethen,timeend,timenow,csamplingtime,ctimethen,ctimenow; // used to calculate sampling time in ms
  byte channel;
  byte lineVoltage=120;
  unsigned int i;
  float MainVolt;
  byte prevkey;

  
  /* DEBUG
  Serial.println("");
  Serial.write("ADC :");
  for (i=0;i<maxChannels;i++){
    Serial.print(adcRead(i));
    if (i != maxChannels-1) Serial.write(",");
  }
  Serial.println("");
  */

  //  Serial.print("Calibrating");

  // calibrate, Calculate how many samples that can be done in 200ms
  MAXsamples=0;
  timethen =  millis();
  timeend=timethen+211; // stop after 200ms+some overhead
  timenow = millis();
  while (timenow<timeend){
    MAXsamples++;
    i=adcRead(0);
    timenow = millis();
  }


#ifdef DEBUG
  samplingtime=timenow - timethen;
  Serial.println();
  Serial.print("Collected ");
  Serial.print(MAXsamples);
  Serial.print(" samples in ");
  Serial.print(samplingtime);
  Serial.println("ms");
#endif

  // Get current date & time
  getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
  // YYMMDDHHMMSS
  sampledate=(long)year*10000+(long)month*100+dayOfMonth;
  sampletime=(long)hour*10000+minute*100+second;

  // Read a DS18S20 on the clock board
  GetTempC();
    
/*
  lcd.setCursor(0,0);
  lcd.print("sampling data   ");
  lcd.setCursor(0,1);
  lcd.print(Vcc);
  lcd.print(":");
  lcd.print(stepV);
  lcd.print("                ");
//  delay(1000);
//  delay(10000);
*/

  timethen =  millis();
  for (channel=0;channel<maxChannels;channel++){
    switch (channel) {
    case 0:
      ctimethen =  millis();
      //Figure out the RATIO by
      // ch_sumI[0]/current line voltage*1000
      ch_sumV[channel]=readVolt(channel,9.57); // Channel,RATIO+calibration (mainvolt/measurevolt, 120VAC->9VAC &  470k/47k=120/9/(470/47)=133.33+CAL+rms)
      ctimenow =  millis();
      buffmV[buffhead].mV[channel]=ch_sumV[channel]; // really volts on main in this case
      ch_samplingtime[channel]=ctimenow-ctimethen;
      MainVolt=int(ch_sumV[0]/100);
      MainVolt=MainVolt/10;
      break; 
    case 1:
    case 2:
      ctimethen =  millis();
      ch_sumI[channel]=readrmsI(channel,120,2.5); // Channel,cycles,CT_MAXAMPS,CT_MAXVOLT
      ctimenow =  millis();
      buffmV[buffhead].mV[channel]=ch_sumI[channel];
      ch_samplingtime[channel]=ctimenow-ctimethen;
      break;
    default:
      ctimethen =  millis();
      ch_sumI[channel]=readrmsI(channel,30,1.0); // Channel,cycles,CT_MAXAMPS,CT_MAXVOLT
      ctimenow =  millis();
      buffmV[buffhead].mV[channel]=ch_sumI[channel];
      ch_samplingtime[channel]=ctimenow-ctimethen;
    }
    buffmV[buffhead].sampledate=sampledate;
    buffmV[buffhead].sampletime=sampletime;
    buffmV[buffhead].tempC=tempC*10; // temp * 10 to stay integer
    buffmV[buffhead].reported=0;
  }
  buffhead++;
  if (buffhead == buffersize) {
    dumpBuff(true,false);
    buffhead=0;  
  }

  timenow = millis();
  samplingtime=timenow - timethen;

  Serial.println();
  Serial.print("Total sampling time:");
  Serial.print(samplingtime);
  Serial.println(" milliseconds");

//Data collected, lets show it
  lcd.clear();
  lcd.setCursor(0,0);
  if (dispmode==1){
    // YYYYMMDD HHMMSS
    // <watt.1>/<watt.2>
    lcd.print("20");
    lcd.print(year, DEC);
    if (month<10){lcd.print("0");}
    lcd.print(month, DEC);
    if (dayOfMonth<10) lcd.print("0");
    lcd.print(dayOfMonth, DEC);
    lcd.print(" ");
    if  (hour<10) lcd.print("0"); 
    lcd.print(hour, DEC);
    if  (minute<10) lcd.print("0"); 
    lcd.print(minute, DEC);
    if (second<10) lcd.print("0");
    lcd.print(second, DEC);
    lcd.setCursor(0,1); // next line
//    lcd.print("l1:");
//    lcd.print(ch_sumI[1]*ch_sumV[0],DEC);
//    lcd.print("l2:");
    //    lcd.print(ch_sumI[2]*ch_sumV[0],DEC);
    lcd.print(MainVolt,DEC);
    lcd.setCursor(6,1); // need to strip off all decimals
    lcd.print("V ");
    lcd.print(tempC);
    lcd.print("C         ");
  } else if (dispmode==2){
    // <lowcnt.0>/<highcnt.0>/midpoint
    // <sumV.0>/<sumI.0>
    lcd.print(ch_LOWval[0]);
    lcd.print("/");
    lcd.print(ch_HIGHval[0]);
    lcd.print("/");
    lcd.print((ch_HIGHval[0]-ch_LOWval[0])/2+ch_LOWval[0]);
    lcd.setCursor(0,1); // next line
    lcd.print(ch_sumV[0]/1000);
    lcd.print("V/");
    lcd.print(ch_sumI[0]);
    lcd.print("");
  } else if (dispmode==3){
    // <vcc>/stepV
    // <sumV.0>/<sumI.0>
    lcd.print("vcc");
    lcd.print(Vcc);
    lcd.print(" stepV");
    lcd.print(stepV);
    lcd.setCursor(0,1); // next line
    lcd.print(valuesummary[0]);
    lcd.print(":");
    //    lcd.print(samples[0]);
  } else if (dispmode==4){
    // <sumI.0> <Watt.0>
    // <sumI.1> <Watt.1>
    lcd.print("0:");
    lcd.print(ch_sumI[0]);
    lcd.print("A ");
    lcd.print(ch_sumI[0]*lineVoltage);
    lcd.print("W");
    lcd.setCursor(0,1); // next line
    lcd.print("1:");
    lcd.print(ch_sumI[1]);
    lcd.print("A ");
    lcd.print(ch_sumI[1]*lineVoltage);
    lcd.print("W");
  } else if (dispmode==5){
    // <volt.0> s<MAXsamples> <TotSampleTime>
    // <sumV.0>/<sumI.0>
    lcd.print((int)ch_sumI[0]/1000,DEC);
    lcd.print(".");
    lcd.print((int)ch_sumI[0]%1000,DEC);
    lcd.print(" s");
    lcd.print(MAXsamples);
    lcd.print(" t");
    lcd.print(samplingtime);
    lcd.setCursor(0,1); // next line
    lcd.print(ch_sumV[0]);
    lcd.print("mV/");
    lcd.print(ch_sumI[0]);
    lcd.print("A");
  }

  lcd.setCursor(15,0);
  lcd.print(dispmode);
  
  //################################################################  
  for (channel=0;channel<maxChannels;channel++){
    Serial.print(channel);
    Serial.print(",");  
    Serial.print("20");
    Serial.print(year, DEC);
    Serial.print("-");
    if (month<10){Serial.print("0");}
    Serial.print(month, DEC);
    Serial.print("-");
    if (dayOfMonth<10){Serial.print("0");}
    Serial.print(dayOfMonth, DEC);
    Serial.print(" ");
    if (hour <10){Serial.print("0"); }
    Serial.print(hour, DEC);// convert the byte variable to a decimal number when being displayed
    Serial.print(":");
    if (minute<10){Serial.print("0");}
    Serial.print(minute, DEC);
    Serial.print(":");
    if (second<10){Serial.print("0");}
    Serial.print(second, DEC);
    Serial.print(",");
    //
    //    Serial.print(stepV);
    //    Serial.print(",");
    Serial.print(tempC);
    Serial.print(",");
    //
    Serial.print(ch_samplingtime[channel]);
    Serial.print(",");
    Serial.print(valuesummary[channel]);
    Serial.print(",");
    //    Serial.print(sqrt(double(valuesummary[channel])/double(samples[channel])));
    //    Serial.print(",");
    Serial.print(ch_sumV[channel]);
    Serial.print(",");
    Serial.print(ch_sumI[channel]);
    Serial.print(",");
//    Serial.print(buffhead);
//    Serial.print(",");
    Serial.print(freeRam(), DEC);
    Serial.print(",");
    Serial.println(stackUnused(),DEC);
  }
  
   
  CheckKeys();
  CheckSerial();

  for (i=0;i<50;i++){ // Wait for around 5 seconds
    prevkey=lastkey;
    CheckKeys();
    if (prevkey==lastkey) {
      delay(100);
    } else {
      lcd.clear();
      lcd.setCursor(15,0);
      lcd.print(dispmode);
    }
  }
}

/*
### Set mode for emacs
### fooLocal Variables: ***
### foomode: shellc ***
### fooEnd: ***
*/
