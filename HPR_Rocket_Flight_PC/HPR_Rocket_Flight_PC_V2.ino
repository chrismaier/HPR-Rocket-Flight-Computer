//HPR Rocket Flight Computer
//Original sketch by Bryan Sparkman
//This is built for the Teensy3.5 board
//-----------Change Log------------
//V1 Final Version of the initial code
//V2 incorporates quaternion rotation
//Compatible Sentences: GPGGA, GPRMC, GNGGA, GNRMC
//--------FEATURES----------
//700Hz 3-axis digital 24G accelerometer data logging
//700Hz 1-axis digital 200G accelerometer data logging 
//700Hz 3-axis digital 2000dps gyroscope data logging
//700Hz of flight events
//700Hz of integrated speed & altitude
//100Hz of quaternion rotation
//20Hz of digital barometric data logging
//10Hz of magnetic data logging
//8Hz of GPS data logging
//5Hz of telemetry output
//Mach immune events
//Sensor Fusion based apogee event
//Barometric based main deploy event
//Optional Apogee delay
//Optional Two-Stage mode
//Audible Continuity report at startup
//Audible Post-flight status report
//Audible Battery Voltage report at startup
//Separate file for each flight
//Optional test mode for bench testing
//Built-in self-calibration mode
//Reads user flight profile from SD card
//--------UPGRADES----------
//Increase the telemetry data rate
//User adjustable radio frequency, bandwidth, & modulation
//Develop magnetic sensor fusion
//Ground-station Adjustable Radio Frequency
//------KNOWN PROBLEMS------
//need SD reading error catch
//something fishy about the radio reset and IRQ setup
//--------PINOUTS----------
//COMMUNICATION PINS:
//I2C Bus: SDA - 18, SCL - 19
//SPI Bus: CS - 10, MOSI - 11, MISO - 12, SCK -13

//OUTPUT PINS:
//Separation Charge Fire: Pin 28
//Igniter Fire: Pin 31
//Apogee Charge Fire: Pin 22 
//Main Deploy Fire: Pin 20 
//Beeper: Pin 32

//INPUT PINS:
//Separation Charge Continuity: Pin 29 
//Igniter Continuity: Pin 30
//Apogee Charge Continuity: Pin 23
//Main Deploy Continuity: Pin 21

//EEPROM ALLOCATION:
//0 - 5: maximum altitude of last flight
//6 - 7: accelBiasX
//8 - 9: accelBiasY
//10-11: accelBiasZ
//12-13: analogXbias
// 14  : resistance factor

//-------CODE START--------
#include <SdFat.h>
#include <i2c_t3.h>
#include <SPI.h>
#include <EEPROM.h>
#include <RH_RF95.h>
#include <TinyGPS++.h>

//Radio setup
#define RF95_FREQ     433.250
#define RFM95_RST     27
#define RFM95_CS      10
#define RFM95_IRQ     26

//GPS setup
#define GPSECHO  false

//Teensy 3.5 Hardware Serial for GPS
HardwareSerial HWSERIAL(Serial1);

// Assign a unique ID
RH_RF95 rf95(RFM95_CS, RFM95_IRQ);
TinyGPSPlus GPS;

SdFatSdio SD;
File outputFile;
File settingsFile;
//-----------------------------------------
//Set code version
//-----------------------------------------
const float codeVersion = 2.0; 
//-----------------------------------------
//Set defaults for user defined variables
//-----------------------------------------
char rocketName[15] = ""; 
char callSign[7]= "";
int maxAng = 45; //degrees
unsigned long setupTime = 0UL;
byte gTrigger = 209; //2.5G trigger
byte magSwitchEnable = 1;
unsigned long detectLiftoffTime = 500000UL; //0.5s
unsigned long apogeeDelay = 1000000UL; //1.0s apogee delay
int Alt_threshold = 120; //120m = 400ft
unsigned long sustainerFireDelay = 1000000UL; //1.0s
unsigned long separation_delay = 500000UL; //0.5s
byte mainDeployAlt = 153;//Up to 255m for main deploy
unsigned long rcd_time = 900000000UL; //15min
unsigned long fireTime = 500000UL;//0.5s
//-----------------------------------------
//GPIO pin mapping
//-----------------------------------------
const byte sepFpin = 28;
const byte ignFpin = 31;
const byte apogeeFpin = 22;
const byte mainFpin = 20;
const byte beepPin = 32;
const byte sepCpin = 29;
const byte ignCpin = 30;
const byte apogeeCpin = 23;
const byte mainCpin = 21;
const byte buttonGnd = 2;
const byte buttonRead = 5;
const byte magSwitchPin = 25;
//-----------------------------------------
//radio variables
//-----------------------------------------
int16_t packetnum = 0;
boolean transmitPacket = false;
unsigned long lastTX = 0UL;
unsigned long radioDelay;
unsigned long RDpreLiftoff = 5000000UL;
unsigned long RDinFlight = 200000UL;
unsigned long RDpostFlight = 10000000UL;
byte radioEvent = 0;
//-----------------------------------------
//flight events
//-----------------------------------------
boolean testMode = false;
boolean calibrationMode = false;
boolean twoStage = false;
boolean preLiftoff = true;
boolean liftoff = false;
boolean boosterBurnout = false;
boolean boosterBurnoutCheck = false;
boolean boosterSeparation = false;
boolean sustainerFireCheck = false;
boolean sustainerFire = false;
boolean apogee = false;
boolean apogeeFire = false;
boolean apogeeSeparation = false;
boolean mainDeploy = false;
boolean touchdown = false;
boolean timeOut = false;
boolean fileClose = false;
boolean rotation_OK = true;
boolean Alt_excd = false;
boolean beep = false;
boolean pyroFire = false;
boolean cont_apogee = false;
boolean cont_main = false;
boolean cont_stage = false;
boolean cont_sep = false;
boolean cont_error = false;
//-----------------------------------------
//Master timing variables
//-----------------------------------------
unsigned long timeClock = 0UL;
unsigned long timeClockPrev = 0UL;
unsigned long timeCurrent = 0UL;
unsigned long dt = 0UL;
unsigned long gdt = 0L;
unsigned long timeGyro = 0UL;
unsigned long timeGyroClock = 0UL;
unsigned long timeGyroClockPrev = 0UL;
unsigned long timeLastEvent = 0UL;
boolean checkFalseTrigger = true;
//-----------------------------------------
//digital accelerometer variables
//-----------------------------------------
const byte g = 83;
int accelBiasX =  0; 
int accelBiasY =  0;
int accelBiasZ = 0;
int analogBiasX = 0;
const int analog0Level = 13120;
const float A2D = 1.525; //1.525 = analogGain / digitalGain = 0.0183/0.012
int accelX0 = 0;
int accelY0 = 0;
int accelZ0 = 0;
int accelX;
int accelY;
int accelZ;
int16_t analogAccelX;
long analogAccelX0 = 0;
int accelNow;
int maxG = 0;
//-----------------------------------------
//Altitude & BMP180 variables
//-----------------------------------------
float Alt = 0.0;
float baseAlt = 10.0;
float maxAltitude = 0.0;
float pressure;
float temperature;
const unsigned int timeBtwnBMP = 15000; //sample once every 50ms, process takes 35ms, 50ms -35ms = 15ms
const unsigned int tmpRdTime = 4500; //need 4.5ms to read data + 1 avg cycle time -> round to nearest 10000us
const unsigned int bmpRdTime = 25500; //need 25.5ms to read data + 1 avg cycle time -> round to nearest 10000us
byte bmp_case = 1;
unsigned long bmp_counter = 1UL;
boolean bmp_flag = false;
float seaLevelPressure = 1013.25;
float pressureAvg = 0;
float pressureAvg5[5] = {0, 0, 0, 0, 0};
float pressureSum = 0.0;
byte pressurePosn = 0;
//-----------------------------------------
//Baro Reporting Variables
//-----------------------------------------
byte baroApogeePosn = 0;
int baroApogee = 0;
int baroLast5 [5] = {0, 0, 0, 0, 0};
byte baroTouchdown = 0;
byte touchdownTrigger = 5;
//-----------------------------------------
//Magnetometer Variables
//-----------------------------------------
unsigned long magTrigger = 100000UL;
unsigned long magCounter = 0UL;
long magX0 = 0;
long magY0 = 0;
long magZ0 = 0;
int magX;
int magY;
int magZ;
//-----------------------------------------
//rotation variables
//-----------------------------------------
long dx = 0L;
long dy = 0L;
long dz = 0L;
float ddx;
float ddy;
float ddz;
float pitchX0 = 0.0;
float yawY0 = 0.0;
float pitchX;
float yawY;
float rollZ;
float offVert;
int gyroBiasX = 14;
int gyroBiasY = 35;
int gyroBiasZ = 3;
int gyroX;
int gyroY;
int gyroZ;
float Quat[5];
unsigned long lastRotn = 0UL;
unsigned long rotnRate = 10000UL;//100 updates per second
const float mlnth = 0.000001;
const float degRad = 0.0174533;
const float radDeg = 57.295780;
//-----------------------------------------
//velocity calculation variables
//-----------------------------------------
float accel_vel = 0.0;
float accel_alt = 0.0;
float maxVelocity = 0.0;
//-----------------------------------------
//beeper variables
//-----------------------------------------
byte beep_counter = 0;
unsigned long beep_delay;
int beepCode = 0;
const unsigned long beep_len = 100000UL;
unsigned long timeBeepStart;
unsigned long timeLastBeep;
const unsigned long short_beep_delay = 100000UL;
const unsigned long long_beep_delay = 800000UL;
//-----------------------------------------
//Firing variables
//-----------------------------------------
byte fire_pin = 0;
unsigned long timeFireBegin;
//-----------------------------------------
//SD card writing variables
//-----------------------------------------
int strPosn = 0;
boolean syncCard = false;
const byte decPts = 2;
const byte base = 10;
char dataString[256];
byte maxAltDigits[6];
byte maxVelDigits[4];
byte voltageDigits[2];
byte altDigits = 6;
byte velDigits = 4;
byte n = 1;
int voltage = 0;
boolean reportCode = true;//true = report max altitude, false = report max velocity
byte postFlightCode = 0;
const char cs = ',';
const byte num7 = 7;
const byte num6 = 6;
const byte num5 = 5;
const byte num4 = 4;
const byte num0 = 0;
//-----------------------------------------
//GPS Variables
//-----------------------------------------
int maxGPSalt = 0;
float baseGPSalt = 0.0;
float GPSavgAlt[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
float GPSaltSum = 0.0;
byte GPSposn = 0;
boolean gpsWrite = false;
boolean gpsTransmit = false;
char liftoffLat = 'N';
char liftoffLon = 'W';
float liftoffLatitude = 0.0;
float liftoffLongitude = 0.0;
int liftoffYear = 0;
byte liftoffMonth = 0;
byte liftoffDay = 0;
byte liftoffHour = 0;
byte liftoffMin = 0;
float liftoffSec = 0.0;
long liftoffMili = 0L;
char touchdownLat = 'N';
char touchdownLon = 'W';
float touchdownLatitude = 0.0;
float touchdownLongitude = 0.0;
float touchdownAlt = 0;
byte touchdownHour = 0;
byte touchdownMin = 0;
float touchdownSec = 0.0;
long touchdownMili = 0L;
byte gpsFix = 0;
float gpsFloat;
float gpsInt;
byte gpsLat;
byte gpsLon;
float gpsLatitude;
float gpsLongitude;
//-----------------------------------------
//debug
//-----------------------------------------
long debugStart;
long debugTime;

void setup(void) {
    
 //Read the battery votage from the main parachute continuity pin
  pinMode(A7, INPUT);
  voltage = analogRead(A7)*(3.3*2.72*10/1023);
  delay(50);
  
  //Set the mode of the output pins
  pinMode(apogeeCpin, INPUT);   //cont: apogee continuity
  pinMode(mainCpin, INPUT);   //cont: main continuity
  pinMode(sepCpin, INPUT);   //cont: separation continuity
  pinMode(ignCpin, INPUT);  //cont: igniter continuity
  pinMode(sepFpin, OUTPUT); //fire: stage separation
  pinMode(ignFpin, OUTPUT); //fire: 2nd stage igniter
  pinMode(apogeeFpin, OUTPUT);  //fire: apogee
  pinMode(mainFpin, OUTPUT);  //fire: main
  pinMode(beepPin, OUTPUT);  //fire: beeper
  pinMode(buttonRead, INPUT_PULLUP);  //test mode button
  pinMode(buttonGnd, OUTPUT); //test mode button
  pinMode(magSwitchPin, INPUT); //mag switch pin
  
  //Start communication
  Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, 400000);
  SPI.begin();
  HWSERIAL.begin(115200);//115200

  //Start sensors and SD card
  SD.begin();
  beginADC(3);
  beginPressure();
  beginGyro();
  beginAccel();
  beginMag();

  //Initialize Quaternion
  Quat[1] = 1;
  Quat[2] = 0;
  Quat[3] = 0;
  Quat[4] = 0;

  //Start Radio 
  SPI.usingInterrupt(RFM95_IRQ);
  digitalPinToInterrupt(RFM95_IRQ);
  //Start the radio
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, LOW);
  //manual reset
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  //Initialize the radio
  if(!rf95.init()){digitalWrite(beepPin, HIGH);
    delay(200);
    digitalWrite(beepPin, LOW);
    delay(200);
    digitalWrite(beepPin, HIGH); 
    delay(200);
    digitalWrite(beepPin, LOW);
    delay(1000);}
  rf95.setFrequency(RF95_FREQ);
  rf95.setTxPower(15, false);//23 max setting; 20mW=13dBm, 30mW=15dBm, 50mW=17dBm, 100mW=20dBm
  radioDelay = RDpreLiftoff;

  //get the initial altitude
  getInitialAlt();

  //check if the test mode button is being held
  digitalWrite(buttonGnd, LOW);
  delay(50);
  if(digitalRead(buttonRead) == LOW){testMode = true;}

  //signal if in test-mode
  if (testMode){

    beep_counter = 0;
    beep_delay = long_beep_delay;
    while(beep_counter < 8){
      
      timeClock = micros();

      //Look for the user to release the button
      if(digitalRead(buttonRead) == HIGH){testMode = false;delay(10);}

      //Look for the user to put it into calibration mode
      if(digitalRead(buttonRead) == LOW && !testMode){
        calibrationMode = true;
        testMode = true;
        beep_counter = 8;
        digitalWrite(beepPin, LOW);
        beep = false;}

      //starts the beep
      if (!beep && timeClock - timeLastBeep > beep_delay){
          digitalWrite(beepPin, HIGH);
          timeBeepStart = timeClock;
          beep = true;
          beep_counter++;}
      
      //stops the beep
      if(beep && (timeClock - timeBeepStart > 500000UL)){
        digitalWrite(beepPin, LOW);
        timeBeepStart = 0UL;
        timeLastBeep = timeClock;
        beep = false;}
      }//end while

    //Reset variables
    beep_counter = 0;
    timeBeepStart = 0UL;
    timeLastBeep = 0UL;
    timeClock = 0UL;
    if(!calibrationMode){testMode = true;}}//end testMode

  //calibration mode
  if(calibrationMode){
    
    for (byte i = 1; i < 20; i++){
      digitalWrite(beepPin, HIGH);
      delay(250);
      digitalWrite(beepPin, LOW);
      delay(250);}

    digitalWrite(beepPin, HIGH);
    
    for (byte i = 1; i < 101; i++){
      getAccel();
      getADC0();
      accelBiasX += (accelX - g);
      accelBiasY += accelY;
      accelBiasZ += accelZ;
      analogBiasX += (analogAccelX - analog0Level - (int)(g/A2D));
      delay(300);}
      
    //calculate the bias
    accelBiasX /= 100;
    accelBiasY /= 100;
    accelBiasZ /= 100;
    analogBiasX /=100;

    //Store in EEPROM
    EEPROM.write(6,lowByte(accelBiasX+1000));
    EEPROM.write(7,highByte(accelBiasX+1000));
    EEPROM.write(8,lowByte(accelBiasY+1000));
    EEPROM.write(9,highByte(accelBiasY+1000));
    EEPROM.write(10,lowByte(accelBiasZ+1000));
    EEPROM.write(11,highByte(accelBiasZ+1000));
    EEPROM.write(12,lowByte(analogBiasX+1000));
    EEPROM.write(13,highByte(analogBiasX+1000));
    
    digitalWrite(beepPin, LOW);}//end calibration mode

  //read the bias from EEPROM
  accelBiasX = word(EEPROM.read(7),EEPROM.read(6))-1000;
  accelBiasY = word(EEPROM.read(9),EEPROM.read(8))-1000;
  accelBiasZ = word(EEPROM.read(11), EEPROM.read(10))-1000;
  //analogBiasX = word(EEPROM.read(13), EEPROM.read(12))-1000;

  //Open the settings file
  settingsFile.open("Settings.txt", O_READ);

  //Read in the user defined variables
  parseNextVariable(false);n=0;
  while (dataString[n]!='\0'){rocketName[n] = dataString[n];n++;}rocketName[n]='\0';n=0;
  parseNextVariable(false);
  while (dataString[n]!='\0'){callSign[n] = dataString[n];n++;}callSign[n]='\0';n=0;
  //magSwitchEnable = (byte)(parseNextVariable(true));
  setupTime = (unsigned long)(parseNextVariable(true)*1000UL);
  gTrigger = (byte)(parseNextVariable(true)*g);
  detectLiftoffTime = (unsigned long)(parseNextVariable(true)*1000000UL);
  apogeeDelay = (unsigned long)(parseNextVariable(true)*1000000UL);
  mainDeployAlt = (byte)(parseNextVariable(true)/3.2808) + 1;
  rcd_time = (unsigned long)(parseNextVariable(true)*1000000UL);
  fireTime = (unsigned long) (parseNextVariable(true)*1000000UL);
  n = (int)parseNextVariable(true);if(n==1){twoStage = true;}n=0;
  sustainerFireDelay = (unsigned long)(parseNextVariable(true)*1000000UL);
  separation_delay = (unsigned long)(parseNextVariable(true)*1000000UL);
  Alt_threshold = (int)(parseNextVariable(true)/3.2808);
  maxAng = (float)parseNextVariable(true)*degRad;

  //close the settings file
  settingsFile.close();
  
  //safety override of manual variables
  if (gTrigger < 1.5 * g) {gTrigger = 1.5 * g;} //min 1.5G trigger
  if (gTrigger > 5 * g) {gTrigger = 5 * g;} //max 5G trigger
  if (detectLiftoffTime < 100000UL) {detectLiftoffTime = 100000UL;} //.1s min gTrigger detection
  if (detectLiftoffTime > 1000000UL) {detectLiftoffTime = 1000000UL;} //1s max gTrigger detection
  if (apogeeDelay > 5000000UL) {apogeeDelay = 5000000UL;} //5s max apogee delay
  if (fireTime > 1000000UL) {fireTime = 1000000UL;} //1s max firing length
  if (mainDeployAlt < 91) {mainDeployAlt = 91;}//minimum of 100ft
  if (sustainerFireDelay > 8000000UL){sustainerFireDelay = 8000000UL;}//maximum 8s 2nd stage ignition delay
  if (separation_delay > 3000000UL){separation_delay = 3000000UL;}//max 3s booster separation delay after burnout
  if (Alt_threshold < 91){Alt_threshold = 91;}//minimum 100ft threshold
  if (maxAng > 90){maxAng = 90;}//maximum 90 degree off vertical
  if (rcd_time < 300000000UL){rcd_time = 300000000UL;}//min 5min of recording time
  if (fireTime < 200000UL){fireTime = 200000UL;}//min 0.2s of firing time
  if (fireTime > 1000000UL){fireTime = 1000000UL;}//max 1.0s of firing time
  if (setupTime > 60000UL) {setupTime = 60000UL;}//max 60 seconds from power-on to preflight start
  if (setupTime < 3000UL) {setupTime = 3000UL;}//min 3 seconds of setup time
  
  //Overrides for bench test mode
  if (testMode) {
    rf95.setTxPower(13, false);//10% power, or 20mW
    detectLiftoffTime = 100000UL; //0.1s
    setupTime = 3000UL; //3s startup time
    apogeeDelay = 1000000UL; //1s apogee delay
    rcd_time = 15000000UL; //15s record time
    gTrigger = 125; //1.5G trigger
    maxAltitude = 11101/3.2808;
    maxVelocity = 202/3.2808;
    RDpreLiftoff = 1000000UL;
    RDpostFlight = 1000000UL;
    radioDelay = RDpreLiftoff;
    magSwitchEnable = (byte)0;}

 // Rename the data file to FLIGHT01.txt
  dataString[0] ='F';
  dataString[1] ='L';
  dataString[2] ='I';
  dataString[3] ='G';
  dataString[4] ='H';
  dataString[5] ='T';
  dataString[6] ='0';
  dataString[7] ='1';
  dataString[8] ='.';
  dataString[9] ='t';
  dataString[10]='x';
  dataString[11]='t';
  dataString[12]='\0';
  
 //Create and open the next file on the SD card
 while (SD.exists(dataString)) {
    n++;
    if(n<10){itoa(n, dataString + 7,10);}
    else{itoa(n, dataString + 6,10);}
    dataString[8]='.';}
  outputFile = SD.open(dataString, FILE_WRITE);
  dataString[0]=(char)0;
  //Print header
  outputFile.print(rocketName);
  outputFile.print(F(" Code V"));
  outputFile.print(codeVersion);
  outputFile.print(F(","));
  outputFile.println(F("gTm,aX,aY,aZ,gX,gY,gZ,roll,pitch,yaw,vel,alt,events,fire,pin,analog,Alt,press,temp,magX,magY,magZ,lat,lon,speed,gps_alt,gps_angle,satellites,packet"));
  if(calibrationMode){
    outputFile.print(accelBiasX);outputFile.print(",");outputFile.print(accelBiasY);outputFile.print(",");
    outputFile.print(accelBiasZ);outputFile.print(",");outputFile.print(analogBiasX);}
  outputFile.sync();

  //if the magnetic switch is enabled, beep 2Hz until the magnet is sensed
  if(magSwitchEnable == 1){
    while(digitalRead(magSwitchPin) == false){
      digitalWrite(beepPin, HIGH);
      delay(250);
      digitalWrite(beepPin, LOW);
      delay(250);}}
  
  //wait for the rocket to be installed vertically
  digitalWrite(beepPin, HIGH);
  delay(setupTime);
  digitalWrite(beepPin, LOW);
  delay(500);

  //sample the sensors 100 times over 3 seconds to determine the offsets and initial values
  for (byte i = 1; i < 101; i++) { 
    //get a gyro event
    getGyro();
    getAccel();
    getADC0();
    getMag();
  
    //add up the gyro samples
    gyroBiasX += gyroX;
    gyroBiasY += gyroY;
    gyroBiasZ += gyroZ;

    //add up the accelerometer samples
    accelX0 += accelX - accelBiasX;
    accelY0 += accelY - accelBiasY;
    accelZ0 += accelZ - accelBiasZ;   

    //add up the analog accelerometer samples
    analogAccelX0 += analogAccelX;
    
    //add up the magnetometer samples
    magX0 += magX;
    magY0 += magY;
    magZ0 += magZ;

    //sample over a period of 3 seconds
    delay(30);}

  //Divide by 100 to set the average of 100 samples
  gyroBiasX /= 100;
  gyroBiasY /= 100;
  gyroBiasZ /= 100;
  (int)(analogAccelX0 /= 100);
  accelX0 /= 100;
  accelY0 /= 100;
  accelZ0 /= 100;
  magX0 /= 100;
  magY0 /= 100;
  magZ0 /= 100;
  
  //Compute the acceleromter based rotation angle
  if (accelZ0 >= 0) {yawY0 = asin(min(1, (float)accelZ0 / (float)g));}
  else {yawY0 = asin(max(-1, (float)accelZ0 / (float)g));}

  if (accelY0 >= 0) {pitchX0 = asin(min(1, (float)accelY0 / (float)g));}
  else {pitchX0 = asin(max(-1, (float)accelY0 / (float)g));}
  
  //update quaternion
  getRotn(pitchX0, yawY0, 0);

  //Calibrate the analog accelerometer to the digital one
  analogBiasX = analogAccelX0 - analog0Level - (int)(accelX0 / A2D);

  //Restart the ADC at 800SPS
  beginADC(1);
  
  //Reset the G-trigger
  gTrigger -= accelBiasX;
  
  //resample initial altitude value
  getInitialAlt();

  //check continuity
  if (digitalRead(apogeeCpin) == HIGH) {cont_apogee = true;}
  if (digitalRead(mainCpin) == HIGH) {cont_main = true;}
  if (digitalRead(sepCpin) == HIGH) {cont_sep = true;}
  if (digitalRead(ignCpin) == HIGH) {cont_stage = true;}
  
  //Report single-stage pre-flight status
  if (!twoStage){
    if (cont_main && cont_apogee) {beepCode = 3;}
    else if (cont_main){beepCode = 2;}
    else if (cont_apogee) {beepCode = 1;}
    else {beepCode = 4;}
    postFlightCode = 1;}

  //Report two-stage pre-flight status
  if (twoStage){
    if (cont_sep && cont_stage && cont_apogee && cont_main) {beepCode = 4;}
    else {
      cont_error = true;
      if (!cont_sep) {beepCode = 1;}
      else if(!cont_stage) {beepCode = 2;}
      else if (!cont_apogee) {beepCode = 3;}
      else if (!cont_main){beepCode = 5;}}}

  //set the beep delay and preflight beep code
  beep_delay = long_beep_delay;
  
  //Read main deploy setting into its beep array
  parseBeep(long(10*int(mainDeployAlt*.32808)), maxVelDigits, num4);
  //Beep out the main deployment altitude setting
  while (maxVelDigits[velDigits-1]==0){velDigits--;}  
  for(byte i = velDigits + 1; i > 0; i--){
    delay(800);
    for(byte j = maxVelDigits[i-1]; j > 0; j--){
      digitalWrite(beepPin, HIGH);
      delay(100);
      digitalWrite(beepPin, LOW);
      delay(100);}}
  velDigits = 4;
  delay(2000);

  //Write initial values into EEPROM
  //for(byte j = 0; j <6; j++){EEPROM.write(j,j);}
  
  //Beep out the last flight's altitude
  for(byte i=0;i<6;i++){maxAltDigits[i]=EEPROM.read(i);}
  while (maxAltDigits[altDigits-1]==0){altDigits--;}  
  for(byte i = altDigits + 1; i > 0; i--){
    delay(800);
    for(byte j = maxAltDigits[i-1]; j > 0; j--){
      digitalWrite(beepPin, HIGH);
      delay(100);
      digitalWrite(beepPin, LOW);
      delay(100);}}
  altDigits = 6;
  delay(2000);

  //Beep out the battery voltage
  parseBeep(voltage, voltageDigits, 2);
  for(byte i = 0; i < 2; i++){
    delay(800);
    for(byte j = voltageDigits[1-i]; j > 0; j--){
      digitalWrite(beepPin, HIGH);
      delay(100);
      digitalWrite(beepPin, LOW);
      delay(100);}}
  delay(2000);

}//end setup

void loop(void){

  //Get an acceleration event
  getAccel();

  //Capture current timestamp
  timeClockPrev = timeClock;
  timeClock = micros();
   
  if (!liftoff && accelX > gTrigger && !touchdown && !timeOut) {
    timeGyroClock = timeClockPrev; //initializes timeGyro to an appropriate value
    lastTX = 0UL;
    radioDelay = RDinFlight; //transmit packets at a faster rate
    preLiftoff = false;
    liftoff = true;
    timeLastEvent = timeCurrent;
    radioEvent = 1;
    liftoffHour = GPS.time.hour();
    liftoffMin = GPS.time.minute();
    liftoffSec = GPS.time.second();
    liftoffMili = GPS.time.centisecond();}

  if (liftoff) {

    //Get gyro values
    getGyro();
    
    //Update master gyro timing variables
    timeGyroClockPrev = timeGyroClock;
    timeGyroClock = micros();
    gdt = long(timeGyroClock - timeGyroClockPrev);
    timeGyro += (unsigned long)gdt;
    
    //Get Analog Accelerometer Data
    getADC0();
    
    //update master timing variables
    dt = timeClock - timeClockPrev;
    timeCurrent += dt;
    
    //Update the counter between end of last read and start of new read
    if(!bmp_flag){bmp_counter += dt;}

    //See if a new temp is needed
    if (!bmp_flag && bmp_counter >= timeBtwnBMP) {
      bmp_flag = true;
      bmp_case = 1;}

    if (bmp_flag) {
      //Check if a temp event should be initiated
      if (bmp_case == 1) {
        initiateTemp();
        bmp_counter = micros();
        bmp_case = 2;}

      //Check if a bmp event should be initiated
      else if (bmp_case == 2 && micros() - bmp_counter >= tmpRdTime ) {
        bmp_case = 3;
        initiatePressure(&temperature);
        bmp_counter = micros();}

      //If required, get a bmp event
      else if (bmp_case == 3 && micros() - bmp_counter >= bmpRdTime) {
        getPressure(&pressure);
        Alt = pressureToAltitude(seaLevelPressure, pressure) - baseAlt;
        if(Alt > maxAltitude && !apogee){maxAltitude = Alt;}
        bmp_case = 0;
        bmp_flag = false;
        bmp_counter = 0;
        //Baro apogee trigger
        baroApogee = int(Alt) - baroLast5[baroApogeePosn];
        baroLast5[baroApogeePosn] = int(Alt);
        baroApogeePosn++;
        if(baroApogeePosn == 5){baroApogeePosn = 0;}
        //Baro touchdown trigger
        if (mainDeploy && baroApogee == 0) {baroTouchdown ++;}
        else {baroTouchdown = 0;}}
    }//end bmp_flag

    //Get magnetometer data
    magCounter += dt;
    if (magCounter >= magTrigger){
      getMag();
      magCounter = 0;}
      
    //Eliminate analog acceleration bias
    analogAccelX -= analogBiasX;

    //Eliminate digital accceleration bias
    accelX -= accelBiasX;
    accelY -= accelBiasY;
    accelZ -= accelBiasZ;

    //Eliminate gyro bias
    gyroX -= gyroBiasX;
    gyroY -= gyroBiasY;
    gyroZ -= gyroBiasZ;

    //if the digital accelerometer is pegged, then overwrite with the calibrated analog value
    accelNow = accelX;
    if (abs(accelNow) >= 1950){accelNow = (analogAccelX-analog0Level)*A2D;}
    if (accelNow > maxG){maxG = accelNow;}
     
    //Integrate velocity, altitude, and rotation data prior to apogee
    if(!apogee || testMode){
      
      //calculate the new acceleration based velocity
      //this makes the apogee event mach immune
      accel_vel += (accelNow - g) * float(dt) * 0.11768 * mlnth;
    
      //update maximum velocity if it exceeds the previous value
      if(accel_vel > maxVelocity){maxVelocity = accel_vel;}
    
      //calculate the new acceleration based altitude
      accel_alt += accel_vel * float(dt) * mlnth;
      if(!Alt_excd && accel_alt > Alt_threshold){Alt_excd = true;}
    
      //caluclate the partial rotation
      const float deg2rad = 0.00122173;
      dx -= gyroZ * gdt;
      dy += gyroY * gdt;
      dz += gyroX * gdt;

      //if required update the rotation
      if(timeCurrent - lastRotn > rotnRate){
        
        ddx = (dx*deg2rad)*mlnth;
        ddy = (dy*deg2rad)*mlnth;
        ddz = (dz*deg2rad)*mlnth;
        
        getRotn( ddx , ddy , ddz);
        dx = 0L;
        dy = 0L;
        dz = 0L;
        lastRotn = timeCurrent;

      //Check if the max angle is exceeded
      if (!sustainerFireCheck && rotation_OK && offVert > maxAng){rotation_OK = false;}}
      
    }//end if !apogee
  
    //Check for timeout
    if (!timeOut && !pyroFire && timeCurrent > rcd_time) {
      timeOut = true;
      radioEvent = 9;
      touchdownHour = GPS.time.hour();
      touchdownMin = GPS.time.minute();
      touchdownSec = GPS.time.second();
      touchdownMili = GPS.time.centisecond();}
    
    //Check false trigger until the flight time has passed the minimum time
    if (checkFalseTrigger) {
      if (timeCurrent > detectLiftoffTime) {checkFalseTrigger = false;}
      if (accelX < gTrigger) {
        //reset the key triggers
        timeCurrent = 0UL;
        timeGyro = 0UL;
        timeLastEvent = 0UL;
        preLiftoff = true;
        liftoff = false;
        radioEvent = 0;
        boosterBurnout = false;
        baroApogee = 0;
        baroApogeePosn = 0;
        baroLast5[0] = 0;
        baroLast5[1] = 0;
        baroLast5[2] = 0;
        baroLast5[3] = 0;
        baroLast5[4] = 0;
        baroTouchdown = 0;
        accel_vel = 0;
        accel_alt = 0;
        packetnum = 0;
        radioDelay = RDpreLiftoff;
        lastTX = 0UL;}
    }//end checkFalseTrigger

    //check for booster burnout: if the x acceleration is negative
    if (!boosterBurnout && liftoff && accelX <= 0) {
      boosterBurnout = true;
      radioEvent = 2;
      boosterBurnoutCheck = true;
      timeLastEvent = timeCurrent;}
      
    //check for booster motor burp for 1 second after burnout is detected
    if (boosterBurnoutCheck){
      if(timeCurrent - timeLastEvent > 1000000UL){boosterBurnoutCheck = false;}
      else if (boosterBurnout && !testMode && accelX > 0){boosterBurnout = false; boosterBurnoutCheck = false; radioEvent = 1;}}

    //Fire separation charge if burnout is detected and time is past the separation delay
    if (!boosterSeparation && twoStage && liftoff && boosterBurnout && !checkFalseTrigger && timeCurrent - timeLastEvent > separation_delay) {
      boosterSeparation = true;
      radioEvent = 3;
      timeLastEvent = timeCurrent;
      fire_pin = sepFpin;
      timeFireBegin = timeCurrent;
      pyroFire = true;
      //Fire separation charge
      digitalWrite(fire_pin, HIGH);}

    //Fire second stage
    if (!sustainerFireCheck && twoStage && liftoff && boosterBurnout && boosterSeparation && !pyroFire && timeCurrent - timeLastEvent > sustainerFireDelay) {
      sustainerFireCheck = true;
      postFlightCode = 1;
      timeLastEvent = timeCurrent;
      //Check for staging inhibit and fire if OK
      if ((Alt_excd && rotation_OK) || testMode) {
        sustainerFire = true;
        fire_pin = ignFpin;
        timeFireBegin = timeCurrent;
        radioEvent = 4;
        pyroFire = true;
        //Fire second stage
        digitalWrite(fire_pin, HIGH);}
      else if (!rotation_OK) {postFlightCode = 3;}
      else if (!Alt_excd) {postFlightCode = 2;}
      else {postFlightCode = 4;}}
      
    //Check for apogee if the accelerometer velocity < 0 or the baroApogee is less than zero
    if (!apogee && boosterBurnout && !boosterBurnoutCheck && !pyroFire && (accel_vel < 0 || (baroApogee < 0 && accel_vel < 100))) {
      apogee = true;
      timeLastEvent = timeCurrent;}

    //Fire apgogee charge if the current time > apogeeTime + apogeeDelay
    if (!apogeeFire && apogee && timeCurrent - timeLastEvent >= apogeeDelay) {
      apogeeFire = true;
      if(!apogeeSeparation){radioEvent = 5;}
      timeLastEvent = timeCurrent;
      digitalWrite(fire_pin, LOW);
      fire_pin = apogeeFpin;
      timeFireBegin = timeCurrent;
      pyroFire = true;
      //Fire apogee charge
      digitalWrite(fire_pin, HIGH);}
      
    //Write the data to the card 3s after apogeeFire and mainDeploy in case of crash or powerloss
    if(apogeeFire && !syncCard && !testMode && timeCurrent - timeLastEvent >= 3000000UL){outputFile.sync(); syncCard = true;}

    //Detect separation after apogee
    if(!apogeeSeparation && ((!twoStage && boosterBurnout) || (twoStage)) && accel_vel < 100 && accelX > 4*g){apogeeSeparation = true; radioEvent = 6;}

    //Fire main chute charge if the baro altitude is lower than the threshold and at least 1s has passed since apogee
    if (!mainDeploy && apogee && Alt < mainDeployAlt && timeCurrent - timeLastEvent >= 1000000UL) {
      mainDeploy = true;
      timeLastEvent = timeCurrent;
      radioEvent = 7;
      digitalWrite(fire_pin, LOW);
      fire_pin = mainFpin;
      timeFireBegin = timeCurrent;
      pyroFire = true;
      //Fire main charge
      digitalWrite(fire_pin, HIGH);
      //reset the sync boolean so the card syncs again 3s after main deploy
      syncCard = false;}
      
    //Stop firing after 500 miliseconds
    if (pyroFire && timeCurrent - timeFireBegin > fireTime) {
      digitalWrite(fire_pin, LOW);
      pyroFire = false;}

    //Check for touchdown
    if (!touchdown && mainDeploy && !pyroFire && !testMode && baroTouchdown > touchdownTrigger && Alt < 46) {
      touchdown = true;
      radioEvent = 8;
      touchdownHour = GPS.time.hour();
      touchdownMin = GPS.time.minute();
      touchdownSec = GPS.time.second();
      touchdownMili = GPS.time.centisecond();}

    //debugStart = micros();
    //Write the data to a string
    //Cycle timestamp
    writeFloatData(timeCurrent, 2, num0);
    writeFloatData(timeGyro, 2, num0);
    //LSM303DLHC Digital Accel Data
    writeIntData(accelX);
    writeIntData(accelY);
    writeIntData(accelZ);
    //L3DG20H Gyro Data
    writeIntData(gyroX);
    writeIntData(gyroY);
    writeIntData(gyroZ);
    //Integrated Rotation Values
    writeIntData((int)(rollZ*radDeg*10));
    writeIntData((int)(yawY*radDeg*10));
    writeIntData((int)(pitchX*radDeg*10));
    //Integrated Speed and Altitude
    writeFloatData(accel_vel, num4, decPts);
    writeFloatData(accel_alt, num4, decPts);
    //Flight Event Flags
    writeBoolData(liftoff);
    writeBoolData(boosterBurnout);
    writeBoolData(boosterBurnoutCheck);
    writeBoolData(boosterSeparation);
    writeBoolData(sustainerFireCheck);
    writeBoolData(sustainerFire);
    writeBoolData(apogee);
    writeBoolData(apogeeFire);
    writeBoolData(mainDeploy);
    writeBoolData(touchdown);
    writeBoolData(timeOut);
    dataString[strPosn] = cs;
    strPosn++;
    writeBoolData(pyroFire);
    dataString[strPosn] = cs;
    strPosn++;
    writeIntData(fire_pin);
    //debug
    writeIntData(analogAccelX);
    //debug
    //writeFloatData(debugTime, num4, num0);
    //BMP180 Altitude data
    if (bmp_counter == 0) {
      writeFloatData(Alt, num4, decPts);
      writeFloatData(pressure, num4, decPts);
      writeFloatData(temperature, num4, 1);}
    else{dataString[strPosn]=cs;strPosn++;dataString[strPosn]=cs;strPosn++;dataString[strPosn]=cs;strPosn++;}
    //Magnetometer Data
    if (magCounter == 0){
      writeIntData(magX);
      writeIntData(magY);
      writeIntData(magZ);}
    else{dataString[strPosn]=cs;strPosn++;dataString[strPosn]=cs;strPosn++;dataString[strPosn]=cs;strPosn++;}
    //GPS Data
    if(gpsWrite){
      dataString[strPosn]=gpsLat;strPosn++;
      writeFloatData(gpsLatitude,2,6);
      dataString[strPosn]=gpsLon;strPosn++;
      writeFloatData(gpsLongitude,2,6);
      writeFloatData((float)GPS.speed.mph(),3,2);
      writeFloatData((float)GPS.altitude.meters(),3,2);
      writeFloatData((float)GPS.course.deg(),2,2);
      writeIntData((int)GPS.satellites.value());
      gpsWrite=false;}
    else{
      dataString[strPosn]=cs;strPosn++;dataString[strPosn]=cs;strPosn++;dataString[strPosn]=cs;strPosn++;
      dataString[strPosn]=cs;strPosn++;dataString[strPosn]=cs;strPosn++;dataString[strPosn]=cs;strPosn++;}
    //update the radio packet number
    if (transmitPacket){writeIntData(packetnum++);}
    else{dataString[strPosn]=cs;strPosn++;}
    //end of sample - carriage return, newline, and null value
    dataString[strPosn] = '\r';
    strPosn++;
    dataString[strPosn] = '\n';
    strPosn++;
    dataString[strPosn] = '\0';
    //debugTime += micros()-debugStart;
    // write the string to file
    outputFile.write(dataString, strPosn);
    strPosn = 0;
    
    //build and transmit the data packet
    if(transmitPacket){
      strPosn = 0;
      writeIntData(radioEvent);
      writeFloatData((float)(timeCurrent*mlnth),2,2);
      writeIntData((int)(accel_vel));
      writeIntData((int)(accel_alt));
      writeIntData((int)(Alt));
      writeIntData((int)(rollZ*10));
      writeIntData((int)(yawY*10));
      writeIntData((int)(pitchX*10));
      writeIntData(accelX);
      if(gpsTransmit){
        writeIntData((int)(GPS.altitude.meters()));
        dataString[strPosn]=gpsLat;strPosn++;
        writeFloatData(gpsLatitude,2,4);
        dataString[strPosn]=gpsLon;strPosn++;
        writeFloatData(gpsLongitude,2,4);
        gpsTransmit = false;}
      else{dataString[strPosn]=cs;strPosn++;dataString[strPosn]=cs;strPosn++;}
      dataString[strPosn] = '\0';
      rf95.send((uint8_t *)dataString, strPosn+1);
      lastTX = micros();
      strPosn = 0;
      transmitPacket = false;}
    
    //Close file at Touchdown or Timeout
    if (timeOut || touchdown) {
      //Print the initial conditions
      outputFile.println(F("Max Alt, Max Speed, baseAlt, initial pitch, initial yaw, accelX0, accelY0, accelZ0, analogX0, magX0, magY0, magZ0, gyroBiasX, gyroBiasY, gyroBiasZ, accelBiasX, accelBiasY, accelBiasZ, analogBiasX"));
      writeFloatData(long(maxAltitude*3.2808), num7, 0);
      writeFloatData(long(maxVelocity*3.2808), num5, 0);
      writeFloatData(baseAlt, num4, decPts);
      writeFloatData(pitchX0*radDeg, 2, 2);
      writeFloatData(yawY0*radDeg, 2, 2);
      writeIntData(accelX0);
      writeIntData(accelY0);
      writeIntData(accelZ0);
      writeIntData(analogAccelX0);
      writeIntData(magX0);
      writeIntData(magY0);
      writeIntData(magZ0);
      writeIntData(gyroBiasX);
      writeIntData(gyroBiasY);
      writeIntData(gyroBiasZ);
      writeIntData(accelBiasX);
      writeIntData(accelBiasY);
      writeIntData(accelBiasZ);
      writeIntData(analogBiasX);
      //end of sample - carriage return, newline, and null value
      dataString[strPosn] = '\r';strPosn++;
      dataString[strPosn] = '\n';strPosn++;
      dataString[strPosn] = '\0';
      outputFile.write(dataString, strPosn);
      
      //write out the launch time and locations
      strPosn = 0;
      outputFile.println(F("launch date, UTC time, launch altitude, launch latitude, launch longitude"));
      //Write out the GPS liftoff date
      outputFile.print(liftoffDay);outputFile.print("/");outputFile.print(liftoffMonth);outputFile.print("/20");outputFile.print(liftoffYear);outputFile.print(",");
      //Write out the GPS liftoff time
      outputFile.print(liftoffHour);outputFile.print(":");outputFile.print(liftoffMin);outputFile.print(":");outputFile.print((int)liftoffSec);outputFile.print(",");
      //Write out GPS launch location
      writeFloatData(baseGPSalt,2,1);
      dataString[strPosn]=liftoffLat; strPosn++; writeFloatData(liftoffLatitude,2,4);
      dataString[strPosn]=liftoffLon; strPosn++; writeFloatData(liftoffLongitude,2,4);
      //end of sample - carriage return, newline, and null value
      dataString[strPosn] = '\r';strPosn++;
      dataString[strPosn] = '\n';strPosn++;
      dataString[strPosn] = '\0';
      outputFile.write(dataString, strPosn);

      //Write out the GPS landing location
      strPosn = 0;
      outputFile.println(F("landing date, UTC time, landing altitude, landing latitude, landing longitude"));
      //Write out the GPS landing date
      outputFile.print(liftoffDay);outputFile.print("/");outputFile.print(liftoffMonth);outputFile.print("/20");outputFile.print(liftoffYear);outputFile.print(",");
      //Write out the GPS landing time
      outputFile.print(touchdownHour);outputFile.print(":");outputFile.print(touchdownMin);outputFile.print(":");outputFile.print((int)touchdownSec);outputFile.print(",");
      writeFloatData(touchdownAlt,2,1);
      dataString[strPosn]=touchdownLat; strPosn++; writeFloatData(touchdownLatitude,2,4);
      dataString[strPosn]=touchdownLon; strPosn++; writeFloatData(touchdownLongitude,2,4);
      //end of sample - carriage return, newline, and null value
      dataString[strPosn] = '\r';
      strPosn++;
      dataString[strPosn] = '\n';
      strPosn++;
      dataString[strPosn] = '\0';
      outputFile.write(dataString, strPosn);
      
      //write out the settings for the flight
      strPosn = 0;
      outputFile.println(F("Rocket Name, callsign, gTrigger, detectLiftoffTime, apogeeDelay, mainDeployAlt, rcdTime, fireTime, 2Stage, ignitionDelay, sepDelay, altThreshold, maxAng, seaLevelPressure"));
      outputFile.print(rocketName);outputFile.print(cs);
      outputFile.print(callSign);outputFile.print(cs);
      writeIntData(gTrigger);
      writeFloatData(detectLiftoffTime*mlnth,2,1);
      writeFloatData(apogeeDelay*mlnth,2,1);
      writeIntData((int)(10*int(mainDeployAlt*.32808)));
      writeFloatData(rcd_time*mlnth,2,0);
      writeFloatData(fireTime*mlnth,2,1);
      if(twoStage){writeIntData(1);} else{writeIntData(0);}
      writeFloatData(sustainerFireDelay*mlnth,2,1);
      writeFloatData(separation_delay*mlnth,2,1);
      writeIntData((int)(10*int(Alt_threshold*.32808)));
      writeIntData(maxAng);
      writeFloatData(seaLevelPressure,2,2);
      //end of sample - carriage return, newline, and null value
      dataString[strPosn] = '\r';
      strPosn++;
      dataString[strPosn] = '\n';
      strPosn++;
      dataString[strPosn] = '\0';
      outputFile.write(dataString, strPosn);    
      strPosn=0;
      //close the file
      outputFile.close();
      fileClose = true;
      liftoff = false;
      digitalWrite(fire_pin, LOW);
      //Set the radio transmitter to post-flight data rate
      radioDelay = RDpostFlight;
      //Read max altitude into its beep array
      parseBeep(long(maxAltitude*3.2808), maxAltDigits, num6);
      //Read max velocity into its beep array
      parseBeep(long(maxVelocity*3.2808), maxVelDigits, num4);
      //reset n, which we'll use to cycle through the reporting digits
      while (maxAltDigits[altDigits-1]==0){altDigits--;}
      while (maxVelDigits[velDigits-1]==0){velDigits--;}  
      n=altDigits;
      reportCode = true;
      //store the maximum altitude in EEPROM
      if(!testMode){for(byte i=0;i<6;i++){EEPROM.update(i,maxAltDigits[i]);}}
    }//end of timeout/touchdown check    
    
  }//end of liftoff flag

  //Code to start the beep
  if ((preLiftoff || fileClose) && !beep && timeClock - timeLastBeep > beep_delay)  {
    digitalWrite(beepPin, HIGH);
    timeBeepStart = timeClock;
    beep_counter++;
    if (beep_counter == beepCode) {
      beep_counter = 0;
      beep_delay = long_beep_delay;
      //If we are post-flight reporting, cycle through the reporting variable
      if (fileClose && postFlightCode != 1){beepCode = postFlightCode;}
      else if (fileClose && reportCode){beepCode = maxAltDigits[n-1];
        if(n==altDigits){beep_delay = 3000000UL;}
        n--;
        //switch reporting codes
        if(n==0){
          n=velDigits;
          reportCode = false;}}
      else if (fileClose && !reportCode){beepCode = maxVelDigits[n-1];
        if(n==velDigits){beep_delay = 3000000UL;}
        n--;
        //switch reporting codes
        if(n==0){
          n=altDigits;
          reportCode = true;}}}
    else {beep_delay = short_beep_delay;}
    beep = true;}

  //Code to stop the beep
  if (beep && (timeClock - timeBeepStart > beep_len)) {
    digitalWrite(beepPin, LOW);
    timeBeepStart = 0UL;
    timeLastBeep = timeClock;
    beep = false;}

  //Telemetry ground code
  if(micros() - lastTX > radioDelay){
    transmitPacket = true;
    if(preLiftoff){
      strPosn=0;
      writeIntData(radioEvent);
      for (byte j = 0; j < sizeof(rocketName); j++){
        dataString[strPosn] = rocketName[j];
        strPosn++;}
      dataString[strPosn] = cs;strPosn++;
      if(twoStage){writeIntData(beepCode);}
      else{writeIntData(beepCode + 5);}
      writeIntData(gpsFix);
      writeIntData((int)(baseAlt));
      writeIntData((int)(GPS.altitude.meters()));
      dataString[strPosn]=gpsLat;strPosn++;
      writeFloatData(gpsLatitude,2,4);
      dataString[strPosn]=gpsLon;strPosn++;
      writeFloatData(gpsLongitude,2,4);
      dataString[strPosn] = '\0';
      rf95.send((uint8_t *)dataString, strPosn+1);
      lastTX = micros();
      strPosn = 1;
      transmitPacket = false;}
    if(touchdown || timeOut){
      strPosn=0;
      writeIntData(radioEvent);
      writeIntData((int)(maxAltitude));
      writeIntData((int)(maxVelocity));
      writeIntData(maxG);
      writeIntData((int)(maxGPSalt));
      writeIntData(gpsFix);
      writeIntData((int)(GPS.altitude.meters()));
      dataString[strPosn]=gpsLat;strPosn++;
      writeFloatData(gpsLatitude,2,4);
      dataString[strPosn]=gpsLon;strPosn++;
      writeFloatData(gpsLongitude,2,4);
      dataString[strPosn] = '\0';
      rf95.send((uint8_t *)dataString, strPosn+1);
      lastTX = micros();
      strPosn = 1;
      transmitPacket = false;}
    }//end telemetry ground code
    
  //GPS Code
  while(HWSERIAL.available() > 0){GPS.encode(HWSERIAL.read());}
  if (GPS.location.isUpdated() || GPS.altitude.isUpdated()) {
        gpsFix = 1;
        gpsWrite = true;
        gpsTransmit = true;
        convertLocation();
        if(GPS.altitude.meters() > maxGPSalt){maxGPSalt = GPS.altitude.meters();}
        //capture the GPS takeoff position and correct base altitude
        if(preLiftoff){
          if(GPS.altitude.meters() > 0){
            //Correct sea level pressure with running average of 5 samples
            //GPS altitude running average
            GPSposn++;
            if(GPSposn > 4){GPSposn = 0;}
            GPSaltSum = GPSaltSum + GPS.altitude.meters() - GPSavgAlt[GPSposn];
            GPSavgAlt[GPSposn] = GPS.altitude.meters();
            baseGPSalt = GPSaltSum*0.2;
            //barometric pressure running average
            pressurePosn++;
            if(pressurePosn > 4){pressurePosn = 0;}
            pressureSum = pressureSum + pressure - pressureAvg5[pressurePosn];
            pressureAvg5[pressurePosn] = pressure;
            pressureAvg = pressureSum*0.2;
            //sea level correction
            seaLevelPressure = pressureAvg / pow((44330 - baseGPSalt)/44330, 5.254861);
            //correct baseAlt
            if(GPSavgAlt[4] != 0){
              baseAlt = pressureToAltitude(seaLevelPressure, pressureAvg);
              if(isnan(baseAlt)){baseAlt = pressureToAltitude(1013.25, pressure);}}}
          liftoffLat=gpsLat;
          liftoffLatitude=gpsLatitude;
          liftoffLon=gpsLon;
          liftoffLongitude=gpsLongitude;
          liftoffYear = GPS.date.year();
          liftoffMonth = GPS.date.month();
          liftoffDay = GPS.date.day();}
        //capture the last GPS position
        if(mainDeploy || !touchdown || !timeOut){
          touchdownLat=gpsLat;
          touchdownLatitude=gpsLatitude;
          touchdownLon=gpsLon;
          touchdownLongitude=gpsLongitude;
          touchdownAlt = GPS.altitude.meters();}}

}//end void main loop

void getInitialAlt(){
  initiateTemp();
  delay(5);
  initiatePressure(&temperature);
  delay(26);
  getPressure(&pressure);
  baseAlt = pressureToAltitude(seaLevelPressure, pressure);}//end void
  
void parseBeep(long value, byte array[], byte arrayLen){
  boolean flag = false;
  for (byte i = arrayLen; i >= 1; i--){
       array[i-1] = byte(value/pow(10,i-1));
       value -= array[i-1]*pow(10,i-1);
       if (!flag && array[i-1] > 0){flag = true;}
       if (flag && array[i-1] == 0){array[i-1] = 10;}}}//end void
       
float parseNextVariable(boolean flag){
  byte n=0;
  float dataValue;
  char c;
  n=0;
  c='\0';
  while (c != '='){c = settingsFile.read();}
  c = settingsFile.read();
  while (c != ';'){
    c = settingsFile.read();
    if(c != ';'){dataString[n]=c;}
    else{dataString[n]='\0';}
    n++;}
  dataValue = atof(dataString);
  if (flag){return dataValue;}
  else{return '\0';}}//end void
  
void writeIntData(int dataValue) {
  itoa(dataValue, dataString + strPosn, base);
  updateStrPosn();}//end void

void writeFloatData(float dataValue, byte dataLen, byte decimals) {
  dtostrf(dataValue, dataLen, decimals, dataString + strPosn);
  updateStrPosn();}//end void

void writeBoolData(boolean dataBool) { 
  if (dataBool) {dataString[strPosn] = '1';}
  else {dataString[strPosn] = '0';}
  strPosn ++;}//end void

void updateStrPosn(){
  while(dataString[strPosn]!= '\0'){strPosn++;}
  dataString[strPosn] = cs;
  strPosn++;}

void convertLocation(){
  //Convert back to NMEA format as required by the ground reciever
  //Latitude
  gpsInt = GPS.location.rawLat().deg;
  gpsFloat = GPS.location.lat();
  gpsLat = 'N';
  if(GPS.location.rawLat().negative){gpsFloat*=-1;gpsLat = 'S';}
  gpsLatitude = gpsInt*100+ 60*(gpsFloat-gpsInt);

  //Longitude
  gpsInt = GPS.location.rawLng().deg;
  gpsFloat = GPS.location.lng();
  gpsLon = 'E';
  if(GPS.location.rawLng().negative){gpsFloat*=-1;gpsLon = 'W';}
  gpsLongitude = gpsInt*100+ 60*(gpsFloat-gpsInt);}

