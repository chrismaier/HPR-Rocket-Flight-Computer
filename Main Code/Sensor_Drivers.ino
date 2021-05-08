//----------------------------
//LIST OF FUNCTIONS & ROUTINES
//----------------------------

//GENERIC SENSOR FUNCTIONS
//beginAccel(): starts accelerometer
//getAccel(): gets accelerometer data
//beginGyro(): starts gyro
//getGyro(): gets gyro data
//beginHighG(): starts high-g accelerometer
//getHighG(): gets high-g accelerometer data
//setMagGain(): sets the magnetometer gain
//getMag(): gets magnetometer data
//beginBaro(): starts the barometer
//getBaro(): gets the barometer data

//GENERIC I2C FUNCTIONS
//testSensor(): gives the sensor a WHOAMI test
//readSensor(): reads the sensor data
//read16(): gets uint_16t over I2C
//readS16(): gets int_16t over I2C
//read16_LE(): gets unit_16t over I2C where the LSB is first
//read16_LE(): gets int_16t over I2C wehre the LSB is first
//read8(): gets a byte over I2C
//readS8(): gets a signed byte over I2C
//write8(): writes a byte to the slave device over I2C

//SENSOR SPECIFIC FUNCTIONS
//beginLSM303: starts sensor
//getLSM303_A(): gets accelerometer data
//getLSM303_M(): gets magnetometer data

//beginL3GD20H(): starts sensor
//getL3GS20H(): gets gyro data

//beginLSM9DS1(): starts sensor
//getLSM9DS1_A(): gets accelerometer data
//getLSM9DS1_G(): gets gyro data
//getLSM9DS1_AG(): gets accelerometer & gyro data
//getLSM9DS1_M(): gets accelerometer data

//beginH3LIS331DL(): starts sensor
//getH3LIS331DL(): gets accelerometer data

//beginADS1115(): starts ADC
//getADS1115(): gets acelerometer data from ADXL377
//writeRegister(): helper function for ADS1115

//beginADXL377(): starts Teensy ADC
//getADXL377(): gets accelerometer data from Teensy ADC

//beginBMP180(): starts sensor
//initiateTemp():
//initiatePressure():
//getPressure():

//beginBMP280(): starts sensor
//readBMP280coefficients(): gets sensor calibration data
//getBMP280(): gets sensor data

//beginBMP388(): starts sensor
//readBMP388coefficients(): gets sensor calibration data
//getBMP388(): gets sensor data

//beginMPL3115A2(): starts sensor
//readMPLbaro(): reads sensor data
//pressureToAltitude(): converts pressure to altitude

//beginMS5611(): starts sensor
//getMS5611(): gets sensor data
//CmdMS56111(): helper function to send commands
//ReadMS5611(): helper function to read data
//ConvertTempMS5611(): converts temperature
//ConvertPressMS5611(): converts pressure

//----------------------------
//Written by SparkyVT, TRA #12111, NAR #85720, L3
//Sensor Package 1: LSM303DLHC, L3GD20, BMP180, ADS1115, ADXL377
//Sensor Package 2: LSM9DS1, MPL3115A2, H3LIS331DL
//Sensor Package 3: LSM9DS1, MPL3115A2, ADXL377
//Sensor Package 4: LSM9DS1, MS5611, H3LIS331DL
//-----------Change Log------------
//26 Nov 17: Version 1 created to support Adafruit 10Dof board
//10 Nov 18: Version 2 created to support new sensor package
//30 Apr 19: Version 3 created to support MPL3115A2 after EMI problems with BMP280 & BMP388
//31 Jan 20: Version 4 created to support all coded sensors and all orientations
//29 Oct 20: Added code for MS5611 sensor
//--------Supported Sensors---------
//Accelerometers/Magnetometers:LSM303, LSM9DS1
//High-G Accelerometers: H3LIS331DL, ADS115 & ADXL377 Combo, ADXL377 & Teensy3.5 ADC combo
//Gyroscopes: L3GD20H, LSM9DS1
//Barometric: BMP180, BMP280, BMP388, MPL3115A2, MS5611

//***************************************************************************
//Generic Sensor Begin & Read Statements
//***************************************************************************
void beginAccel() {

  switch (sensors.accel) {

    case 2:
      sensors.status_LSM9DS1 = beginLSM9DS1();
      break;

    case 1:
      sensors.status_LSM303 = beginLSM303();
      break;
  }
}

void getAccel() {

  //get the sensor data
  switch (sensors.accel) {

    case 2:
      if (events.liftoff) {getLSM9DS1_AG();}
      else {getLSM9DS1_A();}
      break;

    case 1:
      getLSM303_A();
      break;
  }

  //remove bias
  accel.rawX -= accel.biasX;
  accel.rawY -= accel.biasY;
  accel.rawZ -= accel.biasZ;

  //orient sensor data
  accel.x = *accel.ptrX * *accel.ptrXdir;
  accel.y = *accel.ptrY * *accel.ptrYdir;
  accel.z = *accel.ptrZ * *accel.ptrZdir;
}

void setMagGain(boolean magSwitch){
  #define LSM9DS1_ADDRESS_MAG           (0x1E)
  #define LSM9DS1_REGISTER_CTRL_REG2_M  (0x21)
  #define LSM303_ADDRESS_MAG            (0x3C >> 1)
  #define LSM303_REGISTER_MAG_CRB_REG_M (0x01)
  #define LSM303_MAGGAIN_1_3            (0x20)
  
  switch (sensors.accel) {

    case 2: //LSM9DS1
      
      if(magSwitch){
        //Set gain to 8 Gauss
        write8(LSM9DS1_REGISTER_CTRL_REG2_M, LSM9DS1_ADDRESS_MAG, 0b00100000);
        //Set mag trigger
        magTrigger = 3000;
        //correct the bias
        mag.biasX /= 2;
        mag.biasY /= 2;
        mag.biasZ /= 2;
        //clear the buffer
        getMag();
        delay(100);
        getMag();}
      
      else{
        //Set gain to 4 Gauss
        write8(LSM9DS1_REGISTER_CTRL_REG2_M, LSM9DS1_ADDRESS_MAG, 0x00);
        //Set mag trigger
        magTrigger = 6000;
        //Reset the bias
        calUnion.calByte[0]=EEPROM.read(eeprom.magBiasX); calUnion.calByte[1]=EEPROM.read(eeprom.magBiasX+1);
        mag.biasX = calUnion.calValue;
        calUnion.calByte[0]=EEPROM.read(eeprom.magBiasY); calUnion.calByte[1]=EEPROM.read(eeprom.magBiasY+1);
        mag.biasY = calUnion.calValue;
        calUnion.calByte[0]=EEPROM.read(eeprom.magBiasZ); calUnion.calByte[1]=EEPROM.read(eeprom.magBiasZ+1);
        mag.biasZ = calUnion.calValue;
        //clear the buffer
        getMag();
        delay(100);
        getMag();}
      break;

    case 1: //LSM303
      
      if(magSwitch){
        //Set gain to 8 Gauss
        write8(LSM303_REGISTER_MAG_CRB_REG_M, LSM303_ADDRESS_MAG, 0b11100000);
        //Set mag trigger
        magTrigger = 3000;
        //correct the bias
        mag.biasX /= 2;
        mag.biasY /= 2;
        mag.biasZ /= 2;
        //clear the buffer
        getMag();
        delay(100);
        getMag();}    
        
        else{
          //Set gain to 4 Gauss
          write8(LSM303_REGISTER_MAG_CRB_REG_M, LSM303_ADDRESS_MAG, LSM303_MAGGAIN_1_3);
          //Set mag trigger
          magTrigger = 6000;
          //Reset the bias
          calUnion.calByte[0]=EEPROM.read(eeprom.magBiasX); calUnion.calByte[1]=EEPROM.read(eeprom.magBiasX+1);
          mag.biasX = calUnion.calValue;
          calUnion.calByte[0]=EEPROM.read(eeprom.magBiasY); calUnion.calByte[1]=EEPROM.read(eeprom.magBiasY+1);
          mag.biasY = calUnion.calValue;
          calUnion.calByte[0]=EEPROM.read(eeprom.magBiasZ); calUnion.calByte[1]=EEPROM.read(eeprom.magBiasZ+1);
          mag.biasZ = calUnion.calValue;
          //clear the buffer
        getMag();
        delay(100);
        getMag();}
      
      break;}}

void getMag() {

  //get the sensor data
  switch (sensors.accel) {

    case 2:
      getLSM9DS1_M();
      break;

    case 1:
      getLSM303_M();
      break;
  }

  //remove bias
  mag.rawX -= mag.biasX;
  mag.rawY -= mag.biasY;
  mag.rawZ -= mag.biasZ;

  //translate sensor data
  mag.x = *mag.ptrX * *mag.ptrXdir;
  mag.y = *mag.ptrY * *mag.ptrYdir;
  mag.z = *mag.ptrZ * *mag.ptrZdir;
}

void beginGyro() {
  if (!sensors.status_LSM9DS1) {beginL3GD20H();}
}

void getGyro() {

  //get sensor data
  switch (sensors.gyro) {

    case 2:
      if (!events.liftoff) {getLSM9DS1_G();}
      break;

    case 1:
      getL3GD20H();
      break;
  }

  //remove bias
  gyro.rawX -= gyro.biasX;
  gyro.rawY -= gyro.biasY;
  gyro.rawZ -= gyro.biasZ;

  //orient sensor data
  gyro.x = *gyro.ptrX * *gyro.ptrXdir;
  gyro.y = *gyro.ptrY * *gyro.ptrYdir;
  gyro.z = *gyro.ptrZ * *gyro.ptrZdir;
}

void beginHighG(char dataRate) {
  
  switch (sensors.highG) {

    case 1:
      sensors.status_ADS1115 = beginADS1115(dataRate);
      break;

    case 2:
      sensors.status_H3LIS331DL = beginH3LIS331DL(dataRate);
      break;

    case 3:
      if(!startADXL377){sensors.status_ADXL377 = beginADXL377();}
      break;
  }
}

void getHighG(boolean fullScale) {

  switch (sensors.highG) {

    //get sensor data
    case 1:
      getADS1115(fullScale);
      break;

    case 2:
      getH3LIS331DL(fullScale);
      break;

    case 3:
      getADXL377(fullScale);
      break;
  }

  //remove bias
  highG.rawX -= highG.biasX;
  highG.rawY -= highG.biasY;
  highG.rawZ -= highG.biasZ;

  //orient sensor data
  highG.x = *highG.ptrX * *highG.ptrXdir;
  highG.y = *highG.ptrY * *highG.ptrYdir;
  highG.z = *highG.ptrZ * *highG.ptrZdir;
}

void beginBaro() {

  switch (sensors.baro) {

    case 1:
      sensors.status_BMP180 = beginBMP180();
      break;

    case 2:
      sensors.status_MPL3115A2 = beginMPL3115A2();
      break;

    case 3:
      sensors.status_BMP280 = beginBMP280();
      break;

    case 4:
      sensors.status_BMP388 = beginBMP388();
      break;

    case 5: 
      sensors.status_MS5611 = beginMS5611();
      break;
  }
}

void getBaro() {

  switch (sensors.baro) {

    case 2:
      readMPLbaro();
      break;

    case 3:
      getBMP280();
      break;

    case 4:
      getBMP388();
      break;

    case 5: getMS5611();
    break;
  }
}

//***************************************************************************
//Generic I2C WHOAMI Sensor Test
//***************************************************************************
bool testSensor(byte address) {

  switch (pins.i2c) {

    case 0:
      Wire.beginTransmission(address);
      Wire.endTransmission();
      switch (Wire.status()) {
        case I2C_WAITING: return true; break;
        case I2C_ADDR_NAK: return false; break;
        default: return false; break;
      }
      break;

    case 1:
      Wire1.beginTransmission(address);
      Wire1.endTransmission();
      switch (Wire1.status()) {
        case I2C_WAITING: return true; break;
        case I2C_ADDR_NAK: return false; break;
        default: return false; break;
      }
      break;

    case 2:
      Wire2.beginTransmission(address);
      Wire2.endTransmission();
      switch (Wire2.status()) {
        case I2C_WAITING: return true; break;
        case I2C_ADDR_NAK: return false; break;
        default: return false; break;
      }
      break;

    default:
      Wire.beginTransmission(address);
      Wire.endTransmission();
      switch (Wire.status()) {
        case I2C_WAITING: return true; break;
        case I2C_ADDR_NAK: return false; break;
        default: return false; break;
      }
      break;
  }
}

//***************************************************************************
//Generic I2C Read Sensor Data
//***************************************************************************
void readSensor(byte address, byte reg, byte bytes) {

  //reads the data off of the specified i2c bus and places it into the buffer

  switch (pins.i2c) {

    case 0:
      Wire.beginTransmission(address);
      Wire.write(reg);
      Wire.endTransmission(I2C_NOSTOP);
      Wire.requestFrom(address, bytes);
      while (Wire.available() < bytes) {};
      for (byte i = 0; i < bytes; i++) {rawData[i] = Wire.read();}
      break;

    case 1:
      Wire1.beginTransmission(address);
      Wire1.write(reg);
      Wire1.endTransmission(I2C_NOSTOP);
      Wire1.requestFrom(address, bytes);
      while (Wire1.available() < bytes) {};
      for (byte i = 0; i < bytes; i++) {rawData[i] = Wire1.read();}
      break;

    case 2:
      Wire2.beginTransmission(address);
      Wire2.write(reg);
      Wire2.endTransmission(I2C_NOSTOP);
      Wire2.requestFrom(address, bytes);
      while (Wire2.available() < bytes) {};
      for (byte i = 0; i < bytes; i++) {rawData[i] = Wire2.read();}
      break;

    default:
      Wire.beginTransmission(address);
      Wire.write(reg);
      Wire.endTransmission(I2C_NOSTOP);
      Wire.requestFrom(address, bytes);
      while (Wire.available() < bytes) {};
      for (byte i = 0; i < bytes; i++) {rawData[i] = Wire.read();}
      break;
  }
}

//***************************************************************************
//LSM303 Accelerometer & Magnetometer
//***************************************************************************

bool beginLSM303() {

  #define LSM303_ADDRESS_ACCEL               (0x32 >> 1)
  #define LSM303_REGISTER_ACCEL_CTRL_REG4_A  (0x23)
  #define LSM303_REGISTER_ACCEL_CTRL_REG1_A  (0x20)
  
  //***************************************************************************
  //LSM303 Accelerometer
  //***************************************************************************

  if (!testSensor(LSM303_ADDRESS_ACCEL)) {
    if (settings.testMode) {Serial.println(F("LSM303 not found!"));} 
    return false;}
    
  byte id = read8(LSM303_REGISTER_ACCEL_CTRL_REG1_A | 0x80, LSM303_ADDRESS_ACCEL);
  if (id != 0x07) {
    if (settings.testMode) {Serial.println(F("LSM303 not found!"));} 
    return false;}
  if (settings.testMode) {Serial.println(F("LSM303 OK!"));}

  //set gain to 24G
  write8(LSM303_REGISTER_ACCEL_CTRL_REG4_A, LSM303_ADDRESS_ACCEL, 0x38);
  accel.gainX = accel.gainY = accel.gainZ = 0.012 * 9.80655;
  g = 84;
  accel.ADCmax = (int)(0.98*2048);

  //set data rate to 1300Hz
  write8(LSM303_REGISTER_ACCEL_CTRL_REG1_A, LSM303_ADDRESS_ACCEL, 0x97);

  //***************************************************************************
  //LSM303 Magnetometer
  //***************************************************************************

  #define LSM303_ADDRESS_MAG            (0x3C >> 1)
  #define LSM303_REGISTER_MAG_MR_REG_M  (0x02)
  #define LSM303_REGISTER_MAG_CRB_REG_M (0x01)
  #define LSM303_REGISTER_MAG_CRA_REG_M (0x00)
  #define LSM303_MAGGAIN_1_3            (0x20)
  #define LSM303_MAGRATE_15             (0x10)

  //enable magnetometer
  write8(LSM303_REGISTER_MAG_MR_REG_M, LSM303_ADDRESS_MAG, 0x00);

  //set gain
  //_lsm303Mag_Gauss_LSB_XY = 1100;
  //_lsm303Mag_Gauss_LSB_Z  = 980;
  write8(LSM303_REGISTER_MAG_CRB_REG_M, LSM303_ADDRESS_MAG, LSM303_MAGGAIN_1_3);
  float gainX = 1100.0;
  float gainY = 1100.0;
  float gainZ = 980.0;
  mag.ADCmax = 32768;

  if(mag.orientX == 'X'){mag.gainX = gainX;}
  if(mag.orientX == 'Y'){mag.gainX = gainY;}
  if(mag.orientX == 'Z'){mag.gainX = gainZ;}
  if(mag.orientY == 'X'){mag.gainY = gainX;}
  if(mag.orientY == 'Y'){mag.gainY = gainY;}
  if(mag.orientY == 'Z'){mag.gainY = gainZ;}
  if(mag.orientZ == 'X'){mag.gainZ = gainX;}
  if(mag.orientZ == 'Y'){mag.gainZ = gainY;}
  if(mag.orientZ == 'Z'){mag.gainZ = gainZ;}

  //set data rate to 30Hz
  write8(LSM303_REGISTER_MAG_CRA_REG_M, LSM303_ADDRESS_MAG, 0b00010100);
  magTime = 33333UL;

  return true;
}

void getLSM303_A() {
#define LSM303_REGISTER_ACCEL_OUT_X_L_A  (0x28)
  readSensor(LSM303_ADDRESS_ACCEL, (LSM303_REGISTER_ACCEL_OUT_X_L_A | 0x80), 6);
  //get the timestamp
  fltTime.tmClockPrev = fltTime.tmClock;
  fltTime.tmClock = micros();
  //assemble the data
  accel.rawX = (int16_t)(rawData[0] | (rawData[1] << 8)) >> 4;
  accel.rawY = (int16_t)(rawData[2] | (rawData[3] << 8)) >> 4;
  accel.rawZ = (int16_t)(rawData[4] | (rawData[5] << 8)) >> 4;
}

void getLSM303_M() {
#define LSM303_REGISTER_MAG_OUT_X_H_M  (0x03)
  readSensor(LSM303_ADDRESS_MAG, LSM303_REGISTER_MAG_OUT_X_H_M, 6);
  mag.rawX = (int16_t)(rawData[1] | ((int16_t)rawData[0] << 8));
  mag.rawY = (int16_t)(rawData[3] | ((int16_t)rawData[2] << 8));
  mag.rawZ = (int16_t)(rawData[5] | ((int16_t)rawData[4] << 8));
}

//***************************************************************************
//L3GD20 Gyroscope
//***************************************************************************
bool beginL3GD20H() {

#define L3GD20_ADDRESS           0x6B
#define GYRO_REGISTER_WHOAMI     0x0F
#define GYRO_REGISTER_CTRL_REG1  0x20
#define GYRO_REGISTER_CTRL2      0x21
#define GYRO_REGISTER_CTRL_REG4  0x23

  if (!testSensor(L3GD20_ADDRESS)) {
    if (settings.testMode) {
      Serial.println(F("L3GD20H not found!"));
    } return false;
  }
  byte id = read8(GYRO_REGISTER_WHOAMI, L3GD20_ADDRESS);
  if (id != 0xD4 && id != 0xD7) {
    if (settings.testMode) {
      Serial.println(F("L3GD20H not found!"));
    } return false;
  }
  if (settings.testMode) {
    Serial.println(F("L3GD20H OK!"));
  }

  //reset then enable
  write8(GYRO_REGISTER_CTRL_REG1, L3GD20_ADDRESS, 0x00);
  write8(GYRO_REGISTER_CTRL_REG1, L3GD20_ADDRESS, 0x0F);

  //set gain
  write8(GYRO_REGISTER_CTRL_REG4, L3GD20_ADDRESS, 0x20);
  gyro.gainX = gyro.gainY = gyro.gainZ = 0.07;
  gyro.ADCmax = 32768;

  //set data rate 800Hz
  write8(GYRO_REGISTER_CTRL_REG1, L3GD20_ADDRESS, 0xdf);

  //configure high-pass filter
  //write8(GYRO_REGISTER_CTRL2, L3GD20_ADDRESS, 0b00010001);

  return true;
}

void getL3GD20H() {
#define GYRO_REGISTER_OUT_X_L (0x28)
  readSensor(L3GD20_ADDRESS, (GYRO_REGISTER_OUT_X_L | 0x80), 6);
  //Capture current timestamp
  fltTime.gyroClockPrev = fltTime.gyroClock;
  fltTime.gyroClock = micros();
  //Assemble the data
  gyro.rawX = (int16_t)(rawData[0] | (rawData[1] << 8));
  gyro.rawY = (int16_t)(rawData[2] | (rawData[3] << 8));
  gyro.rawZ = (int16_t)(rawData[4] | (rawData[5] << 8));
}

//***************************************************************************
//LSM9DS1 Accelerometer, Gyroscope, & Magnetometer
//***************************************************************************
bool beginLSM9DS1() {

  //Addresses for the registers
  #define LSM9DS1_ADDRESS_ACCELGYRO          (0x6B)
  #define LSM9DS1_ADDRESS_MAG                (0x1E)
  #define LSM9DS1_XG_ID                      (0b01101000)
  #define LSM9DS1_MAG_ID                     (0b00111101)
  #define LSM9DS1_REGISTER_CTRL_REG1_G         (0x10)
  #define LSM9DS1_REGISTER_CTRL_REG5_XL        (0x1F)
  #define LSM9DS1_REGISTER_CTRL_REG6_XL        (0x20)
  #define LSM9DS1_REGISTER_CTRL_REG1_M         (0x20)
  #define LSM9DS1_REGISTER_CTRL_REG2_M         (0x21)
  #define LSM9DS1_REGISTER_CTRL_REG3_M         (0x22)
  #define LSM9DS1_REGISTER_CTRL_REG4_M         (0x23)
  #define LSM9DS1_REGISTER_CTRL_REG3_G         (0x12)

  //----------------------
  //GET WHO AM I REGISTERS
  //----------------------
  if (!testSensor(LSM9DS1_ADDRESS_ACCELGYRO)) {
    if (settings.testMode) {Serial.println(F("LSM9DS1 not found!"));} 
    return false;}
    
  byte id = read8(0x0F, LSM9DS1_ADDRESS_ACCELGYRO);
  if (id != 0b01101000) {
    if (settings.testMode) {Serial.println(F("LSM9DS1 not found!"));} 
    return false;}
    
  if (settings.testMode) {Serial.println(F("LSM9DS1 OK!"));}

  //----------------------
  //CONFIGURE ACCELEROMETER
  //----------------------
  //Set 16G Range, 952 Hz ODR,
  write8(LSM9DS1_REGISTER_CTRL_REG6_XL, LSM9DS1_ADDRESS_ACCELGYRO,  0b11001000);
  accel.gainX = accel.gainY = accel.gainZ = 0.000735 * 9.80655;
  accel.ADCmax = (int)(0.98*32768);
  g = 1366;

  //----------------------
  //CONFIGURE GYROSCOPE
  //----------------------
  //Set 2000dps Range, 952 Hz ODR
  write8(LSM9DS1_REGISTER_CTRL_REG1_G, LSM9DS1_ADDRESS_ACCELGYRO, 0b11011000);
  gyro.gainX = gyro.gainY = gyro.gainZ = 0.07;
  gyro.ADCmax = 32768;

  //disable high-pass filter
  //write8(LSM9DS1_REGISTER_CTRL_REG3_G, LSM9DS1_ADDRESS_ACCELGYRO, 0b00000000);

  //----------------------
  //CONFIGURE MAGNETOMETER
  //----------------------
  //Mag Temp Compensation, UltraHigh Perf, 40Hz
  write8(LSM9DS1_REGISTER_CTRL_REG1_M, LSM9DS1_ADDRESS_MAG, 0b11111000);
  //Set gain to 4 Gauss
  write8(LSM9DS1_REGISTER_CTRL_REG2_M, LSM9DS1_ADDRESS_MAG, 0x00);
  //Mag Continuous Conversion
  write8(LSM9DS1_REGISTER_CTRL_REG3_M, LSM9DS1_ADDRESS_MAG, 0x00);
  //Mag Reverse Magnetometer MSB / LSB Order, Z-Axis high-perf mode
  write8(LSM9DS1_REGISTER_CTRL_REG4_M, LSM9DS1_ADDRESS_MAG, 0b00001100);

  return true;
}//end begin

void getLSM9DS1_AG() {

  //this uses the LSM9DS1 burst read to rapidly sample the sensors
#define LSM9DS1_REGISTER_OUT_X_L_XL (0x28)
#define LSM9DS1_REGISTER_OUT_X_L_G  (0x18)
  //readSensor(LSM9DS1_ADDRESS_ACCELGYRO, 0x80 | LSM9DS1_REGISTER_OUT_X_L_XL, (byte)1);

  //readSensor(LSM9DS1_ADDRESS_ACCELGYRO, LSM9DS1_REGISTER_OUT_X_L_G, 12);

  //Capture current timestamp
  fltTime.gyroClockPrev = fltTime.gyroClock;
  fltTime.gyroClock = micros();
  fltTime.tmClockPrev = fltTime.tmClock;
  //fltTime.tmClock = micros();
  fltTime.tmClock = fltTime.gyroClock;

  readSensor(LSM9DS1_ADDRESS_ACCELGYRO, LSM9DS1_REGISTER_OUT_X_L_G, 12);
  //read the data
  gyro.rawX = (int16_t)(rawData[0] | (rawData[1] << 8));
  gyro.rawY = (int16_t)(rawData[2] | (rawData[3] << 8));
  gyro.rawZ = (int16_t)(rawData[4] | (rawData[5] << 8));
  accel.rawX  = (int16_t)(rawData[6] | (rawData[7] << 8));
  accel.rawY  = (int16_t)(rawData[8] | (rawData[9] << 8));
  accel.rawZ  = (int16_t)(rawData[10] | (rawData[11] << 8));

  //Check range
  /*if(accelX > 31500 && range < 16){

    //Set 16G Range, 952 Hz ODR,
    write8(LSM9DS1_REGISTER_CTRL_REG6_XL, LSM9DS1_ADDRESS_ACCELGYRO,  0b11001000);}*/
}

void getLSM9DS1_A() {
#define LSM9DS1_REGISTER_OUT_X_L_XL (0x28)
  readSensor(LSM9DS1_ADDRESS_ACCELGYRO, 0x80 | LSM9DS1_REGISTER_OUT_X_L_XL, 6);

  //Capture current timestamp
  fltTime.gyroClockPrev = fltTime.gyroClock;
  fltTime.gyroClock = micros();
  fltTime.tmClockPrev = fltTime.tmClock;
  fltTime.tmClock = micros();

  accel.rawX = (int16_t)(rawData[0] | (rawData[1] << 8));
  accel.rawY = (int16_t)(rawData[2] | (rawData[3] << 8));
  accel.rawZ = (int16_t)(rawData[4] | (rawData[5] << 8));
}

void getLSM9DS1_G() {
#define LSM9DS1_REGISTER_OUT_X_L_G  (0x18)
  readSensor(LSM9DS1_ADDRESS_ACCELGYRO, 0x80 | LSM9DS1_REGISTER_OUT_X_L_G, 6);

  gyro.rawX  = (int16_t)(rawData[0] | (rawData[1] << 8));
  gyro.rawY  = (int16_t)(rawData[2] | (rawData[3] << 8));
  gyro.rawZ  = (int16_t)(rawData[4] | (rawData[5] << 8));
}

void getLSM9DS1_M() {
#define LSM9DS1_REGISTER_OUT_X_L_M  (0x28)
  readSensor(LSM9DS1_ADDRESS_MAG, 0x80 | LSM9DS1_REGISTER_OUT_X_L_M, 6);

  mag.rawX  = (int16_t)(rawData[0] | (rawData[1] << 8));
  mag.rawY  = (int16_t)(rawData[2] | (rawData[3] << 8));
  mag.rawZ  = (int16_t)(rawData[4] | (rawData[5] << 8));
}

//***************************************************************************
//AXL377 High-G Analog Accelerometer w/ Teensy3.5 ADC
//***************************************************************************
bool beginADXL377() {

  //set gain
  for(byte i = 0; i < 4; i++){calFloat.calByte[i] = (byte)EEPROM.read(eeprom.ADXL377gain+i);}
  highG.gainX = highG.gainY = highG.gainZ = calFloat.val * 9.80655;
  if(settings.testMode){Serial.print("ADXL377 Gain: ");Serial.println(highG.gainX, 5);}
  //highG.gainX = highG.gainY = highG.gainZ = 9.80655/129;
  //highG.gainX = highG.gainY = highG.gainZ = 0.0158337 * 9.80655;

  //high1G = 63;// bits in 1G = 1/gain = 63
  //high1G = (int16_t)(1/highG.gainX);  
  high1G = 129;
  //sizeHighGfilter = 1;
  sizeHighGfilter = 10;

  //Start the DAC for differential mode
  //analogWriteResolution(12);
  //analogWrite(A22, 2048); //4093 = 3.3V, 2048 = 1.65V

  //Start ADC1
  //adc->setResolution(16, ADC_1); // set bits of resolution
  //adc->setConversionSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED, ADC_1);
  //adc->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_HIGH_SPEED, ADC_1);

  startADXL377 = true;

  if (settings.testMode) {Serial.println(F("ADXL377 OK!"));}
  return true;
}

void getADXL377(boolean threeAxisMode) {
  
  //measured time is 45 micros per reading
  
  //const uint16_t zeroLevel = adc->getMaxValue(ADC_0)*0.5;
  //const uint16_t zeroLevel = 0;
  const uint16_t ADCmaxValue = 65535;
  const uint16_t ADCmidValue = 32768;
  const uint32_t ADCdataRate = 2000;
  uint16_t val;
  long highGsumX = 0L;
  long highGsumY = 0L;
  long highGsumZ = 0L;
  int16_t interferenceValue = 16;
  unsigned long interferenceTime = 118020UL;
  static boolean radioInterference = false;
  static int16_t prevPacketNum = 0;
  //Is the radio transmitting?  If so, counter the interference
  //if(!radioInterference && radio.packetnum > prevPacketNum){radioInterference = true; prevPacketNum = radio.packetnum;}
  //if(radioInterference && fltTime.timeCurrent - TXnow > interferenceTime){radioInterference = false;}
  
//  //singleAxisMode = true: only samples the axis aligned to the travel of the rocket (Z-axis)
//  if(threeAxisMode){
//    
//    highG.rawX = (int16_t)(adc->adc0->analogRead(A2) - zeroLevel);
//    highG.rawY = (int16_t)(adc->adc0->analogRead(A3) - zeroLevel);
//    highG.rawZ = (int16_t)(adc->adc0->analogRead(A4) - zeroLevel);

//    highGsum = 0;
//    for(byte i = 0; i <30; i++){adc->adc0->startSingleRead(A2); highGsum += adc->adc0->readSingle();}
//    highG.rawX = (int16_t)((highGsum/30)-zeroLevel);
//
//    highGsum = 0;
//    for(byte i = 0; i <30; i++){adc->adc1->startSingleDifferential(A10, A11); highGsum -= adc->adc1->readSingle();}
//    highG.rawX = (int16_t)((highGsum/30)-zeroLevel);

//    highGsum = 0;
//    for(byte i = 0; i <30; i++){adc->adc0->startSingleRead(A3); highGsum += adc->adc0->readSingle();}
//    highG.rawY = (int16_t)((highGsum/30)-zeroLevel);
//
//    highGsum = 0;
//    for(byte i = 0; i <30; i++){adc->adc0->startSingleRead(A4); highGsum += adc->adc0->readSingle();}
//    highG.rawZ = (int16_t)((highGsum/30)-zeroLevel);
    
//    }
//    
//  else{
//    
//    if (highG.orientX == 'Z'){
//      highGsum = 0;
//      for(byte i = 0; i <30; i++){adc->adc1->startSingleDifferential(A10, A11); highGsum -= adc->adc1->readSingle();}
//      highG.rawX = (int16_t)((highGsum/30)-zeroLevel);}
//    if (highG.orientY == 'Z'){highG.rawY = (int16_t)(adc->adc0->analogRead(A3) - zeroLevel);}
//    if (highG.orientZ == 'Z'){highG.rawZ = (int16_t)(adc->adc0->analogRead(A4) - zeroLevel);}
//    
//    }

if(threeAxisMode){

  highGsumX = 0;
  highGsumY = 0;
  highGsumZ = 0;
  
  for(byte i = 0; i < 5; i++){
    highGsumX += analogRead(A2);
    highGsumY += analogRead(A3);
    highGsumZ += analogRead(A4);}
    
  highG.rawX = (int16_t)((highGsumX/5) - ADCmidValue);
  highG.rawY = (int16_t)((highGsumY/5) - ADCmidValue);
  highG.rawZ = (int16_t)((highGsumZ/5) - ADCmidValue);

  if(radioInterference){
    highG.rawX += interferenceValue;
    highG.rawY += interferenceValue;
    highG.rawZ += interferenceValue;}
  }

else{

  highGsumZ = 0;
  
  if (highG.orientX == 'Z'){
    for(byte i = 0; i < 5; i++){highGsumZ += analogRead(A2);}
    highG.rawX = (int16_t)((highGsumZ/5) - ADCmidValue);
    if(radioInterference){highG.rawX += interferenceValue;}}

  else if (highG.orientY == 'Z'){
    for(byte i = 0; i < 5; i++){highGsumZ += analogRead(A3);}
    highG.rawY = (int16_t)((highGsumZ/5) - ADCmidValue);
    if(radioInterference){highG.rawY += interferenceValue;}}

  else if (highG.orientZ == 'Z'){
    for(byte i = 0; i < 5; i++){highGsumZ += analogRead(A4);}
    highG.rawZ = (int16_t)((highGsumZ/5) - ADCmidValue);
    if(radioInterference){highG.rawZ += interferenceValue;}}}
 
}//endvoid

//***************************************************************************
//H3LIS331DL High-G Accelerometer
//***************************************************************************

bool beginH3LIS331DL(char DR) {

#define H3LIS331_ADDRESS            (0x19)
#define H3LIS331_REGISTER_CTRL_REG1 (0x20)
#define H3LIS331_REGISTER_CTRL_REG2 (0x21)
#define H3LIS331_REGISTER_CTRL_REG4 (0x23)

  if (!testSensor(H3LIS331_ADDRESS)) {
    if (settings.testMode) {
      Serial.println(F("H3LIS331DL not found!"));
    } return false;
  }
  byte id = read8(0x0F, H3LIS331_ADDRESS);
  if (id != 0b00110010) {
    if (settings.testMode) {Serial.println(F("H3LIS331DL not found!"));} 
    return false;}
  if (settings.testMode) {Serial.println(F("H3LIS331DL OK!"));}

  high1G = 21;
  highG.gainX = highG.gainY = highG.gainZ =0.049 * 9.80655;
  sizeHighGfilter = 15;

  //Set 100G scale
  write8(H3LIS331_REGISTER_CTRL_REG4, H3LIS331_ADDRESS, 0b00000000);

  //Normal Mode (0), High-Pass filter mode (01), Filter Data Select (1),
  //HP Interrupt2 (0), HP Interrupt1 (0), HPF Coeff(00)
  //write8(H3LIS331_REGISTER_CTRL_REG2, H3LIS331_ADDRESS, 0b00110000);

  if (DR == 'F') {
    //Normal Mode (001), 1000 Hz data rate (11), Enable axes (111)
    write8(H3LIS331_REGISTER_CTRL_REG1, H3LIS331_ADDRESS, 0b00111111);}
  
  if (DR == 'M') {
    //Normal Mode (001), 400 Hz data rate (10), Enable axes (111)
    write8(H3LIS331_REGISTER_CTRL_REG1, H3LIS331_ADDRESS, 0b00110111);}

  if (DR == 'S') {
    //Normal Mode (001), 100 Hz data rate (01), Enable axes (111)
    write8(H3LIS331_REGISTER_CTRL_REG1, H3LIS331_ADDRESS, 0b00101111);}

  return true;}

void getH3LIS331DL(boolean threeAxisMode) {

#define H3LIS331_REGISTER_OUT_X_L   0b10101000 //(0x28)
  //readSensor(H3LIS331_ADDRESS, H3LIS331_REGISTER_OUT_X_L, (byte)4);

  if (!threeAxisMode) {
    readSensor(H3LIS331_ADDRESS, H3LIS331_REGISTER_OUT_X_L, 2);
    highG.rawX = (int16_t)(rawData[0] | (rawData[1] << 8)) >> 4;
  }

  else {
    readSensor(H3LIS331_ADDRESS, H3LIS331_REGISTER_OUT_X_L, 6);
    highG.rawX = (int16_t)(rawData[0] | (rawData[1] << 8)) >> 4;
    highG.rawY = (int16_t)(rawData[2] | (rawData[3] << 8)) >> 4;
    highG.rawZ = (int16_t)(rawData[4] | (rawData[5] << 8)) >> 4;
  }


}

//***************************************************************************
//ADS1115 ADC interface to ADXL377
//***************************************************************************
/**************************************************************************/
/*!
    @file     Adafruit_ADS1015.cpp
    @author   K.Townsend (Adafruit Industries)
    @license  BSD (see license.txt)

    Driver for the ADS1015/ADS1115 ADC

    This is a library for the Adafruit MPL115A2 breakout
    ----> https://www.adafruit.com/products/???

    Adafruit invests time and resources providing this open source code,
    please support Adafruit and open-source hardware by purchasing
    products from Adafruit!

    @section  HISTORY

    v1.0 - First release
*/
/**************************************************************************/

#define ADS1115_ADDRESS                 (0x48)

static void writeRegister(uint8_t i2cAddress, uint8_t reg, uint16_t value) {

  switch (pins.i2c) {

    case 0:
      Wire.beginTransmission(i2cAddress);
      Wire.write((uint8_t)reg);
      Wire.write((uint8_t)(value >> 8));
      Wire.write((uint8_t)(value & 0xFF));
      Wire.endTransmission();
      break;

    case 1:
      Wire1.beginTransmission(i2cAddress);
      Wire1.write((uint8_t)reg);
      Wire1.write((uint8_t)(value >> 8));
      Wire1.write((uint8_t)(value & 0xFF));
      Wire1.endTransmission();
      break;

    case 2:
      Wire2.beginTransmission(i2cAddress);
      Wire2.write((uint8_t)reg);
      Wire2.write((uint8_t)(value >> 8));
      Wire2.write((uint8_t)(value & 0xFF));
      Wire2.endTransmission();
      break;
  }
}

bool beginADS1115(char dataRate) {

  if (!testSensor(ADS1115_ADDRESS)) {
    if (settings.testMode) {Serial.println(F("ADS1115 not found!"));} 
    return false;}
  if(settings.testMode){Serial.println(F("ADS1115 OK!"));}

  highG.gainX = highG.gainY = highG.gainZ = 0.0183 * 9.80665;
  high1G = 55;
  sizeHighGfilter = 10;

#define ADS1115_REG_CONFIG_DR_32SPS    (0x40)
#define ADS1115_REG_CONFIG_DR_800SPS   (0x00E0)
#define ADS1115_REG_CONFIG_DR_400SPS   (0x00C0)
  uint16_t rateSPS = ADS1115_REG_CONFIG_DR_32SPS;

  switch (dataRate) {
    case 'F': rateSPS = ADS1115_REG_CONFIG_DR_800SPS;
      break;

    case 'M': rateSPS = ADS1115_REG_CONFIG_DR_400SPS;
      break;

    case 'S': rateSPS = ADS1115_REG_CONFIG_DR_32SPS;
      break;
  }

  uint16_t configADS =

#define ADS1115_REG_CONFIG_CQUE_NONE    (0x0003)
    ADS1115_REG_CONFIG_CQUE_NONE    | // Disable the comparator (default val)
#define ADS1115_REG_CONFIG_CLAT_NONLAT  (0x0000)
    ADS1115_REG_CONFIG_CLAT_NONLAT  | // Non-latching (default val)
#define ADS1115_REG_CONFIG_CPOL_ACTVLOW (0x0000)
    ADS1115_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
#define ADS1115_REG_CONFIG_CMODE_TRAD   (0x0000)
    ADS1115_REG_CONFIG_CMODE_TRAD   | // Traditional comparator (default val)
    //set the sample rate
    rateSPS   |
#define ADS1115_REG_CONFIG_MODE_CONTIN  (0x0000)
    ADS1115_REG_CONFIG_MODE_CONTIN;   // Continuous mode

  //Set gain
#define ADS1115_REG_CONFIG_PGA_4_096V   (0x0200)
  configADS |= ADS1115_REG_CONFIG_PGA_4_096V;

  //Set channel
#define ADS1115_REG_CONFIG_MUX_SINGLE_0 (0x4000)
  configADS |= ADS1115_REG_CONFIG_MUX_SINGLE_0;

  // Set 'start single-conversion' bit
#define ADS1115_REG_CONFIG_OS_SINGLE    (0x8000)
  configADS |= ADS1115_REG_CONFIG_OS_SINGLE;

  // Write config register to the ADC
#define ADS1115_REG_POINTER_CONFIG      (0x01)
  writeRegister(ADS1115_ADDRESS, ADS1115_REG_POINTER_CONFIG, configADS);

  return true;
}

void getADS1115(boolean threeAxisMode) {
#define ADS1115_REG_POINTER_CONVERT     (0x00)
  int16_t zeroLevel = 13120;
  highG.rawX = readS16(ADS1115_REG_POINTER_CONVERT, ADS1115_ADDRESS) - zeroLevel;
}

//***************************************************************************
//MPL3115A2 Barometric Pressure Sensor
//***************************************************************************

boolean beginMPL3115A2() {
#define MPL3115A2_ADDRESS  0x60
#define MPL3115A2_I2C_READ 0xC1
#define MPL3115A2_I2C_WRITE 0xC0
#define MPL3115A2_WHOAMI   0x0C

  //establish contact
  //if(!testSensor(MPL3115A2_ADDRESS)){if(settings.testMode){Serial.println(F("MPL3115A2 not found!"));}return false;}
  uint8_t reply = read8(MPL3115A2_WHOAMI, MPL3115A2_ADDRESS);

  if (reply != 0xC4) {
    if (settings.testMode) {
      Serial.print(F("MPL3115A2 bad reply! "));
      Serial.println(reply);
    } return false;
  }
  if (settings.testMode) {
    Serial.println(F("MPL3115A2 OK!"));
  }

  //set the sampling frequency
  timeBtwnBaro = 35000UL;

  //SETUP Control Register
  //barometer mode: 0
  //RAW mode: 0
  //Oversampling: 18ms - 010, 34ms - 011, 66ms - 100
  //Reset: 0
  //One Shot Mode (OST): 1
  //Standby: 0
  write8(0x26, MPL3115A2_ADDRESS, 0b00011010);

  //Read the offsets from the registers
  baroPressOffset = (float)EEPROM.read(eeprom.MPL3115A2pressOffset) * 0.04;
  baroTempOffset = (int16_t)EEPROM.read(eeprom.MPL3115A2tempOffset);
  //baroPressOffset = (float)(readS8(0x2B, MPL3115A2_ADDRESS)) * 0.04; 
  //Serial.print("Press Offset: ");Serial.println(baroPressOffset, 2);
  //baroTempOffset = (int16_t)(readS8(0x2C, MPL3115A2_ADDRESS));
  //Serial.print("Temp Offset: ");Serial.println(baroTempOffset * 0.0625, 2);
  //baroAltOffset = (float)(readS8(0x2D, MPL3115A2_ADDRESS));
  //Serial.print("Alt Offset: ");Serial.println(baroAltOffset);

  return true;
}

void readMPLbaro() {

  readSensor(MPL3115A2_ADDRESS, 0x01, 5);

  uint32_t adc_P;
  adc_P = rawData[0];
  adc_P <<= 8;
  adc_P |= rawData[1];
  adc_P <<= 8;
  adc_P |= rawData[2];
  adc_P >>= 4;

  int16_t adc_T;
  adc_T = rawData[3];
  adc_T <<= 8;
  adc_T |= rawData[4];
  adc_T >>= 4;

  pressure = (float)(adc_P) * 0.0025 + baroPressOffset;
  temperature = adc_T;
  if (adc_T & 0x800) {
    adc_T |= 0xF000;
  }
  temperature = (float)(adc_T + baroTempOffset) * 0.0625;
  Alt = 44330.77 * (1.0 - pow(pressure / seaLevelPressure, 0.1902632));

  //initiate next reading
  write8(0x26, MPL3115A2_ADDRESS, 0b00011010);
}

float pressureToAltitude(float seaLevel, float atmospheric) {
  return 44330.77 * (1.0 - pow(atmospheric / seaLevel, 0.1902632));
}

//***************************************************************************
//BMP180 Pressure Sensor
//***************************************************************************
/***************************************************************************
  This is a library for the BMP180 pressure sensor

  Designed specifically to work with the Adafruit BMP180 or BMP180 Breakout
  ----> http://www.adafruit.com/products/391
  ----> http://www.adafruit.com/products/1603

  These displays use I2C to communicate, 2 pins are required to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
***************************************************************************/
#define BMP180_ADDRESS  (0x77)
#define _BMP180Mode     (3)

struct BMP180_calib_data
{
  int16_t  ac1;
  int16_t  ac2;
  int16_t  ac3;
  uint16_t ac4;
  uint16_t ac5;
  uint16_t ac6;
  int16_t  b1;
  int16_t  b2;
  int32_t  b5;
  int16_t  mb;
  int16_t  mc;
  int16_t  md;
} ;
static BMP180_calib_data _BMP180_coeffs;

boolean beginBMP180() {

#define BMP180_REGISTER_CAL_AC1 0xAA
#define BMP180_REGISTER_CAL_AC2 0xAC
#define BMP180_REGISTER_CAL_AC3 0xAE
#define BMP180_REGISTER_CAL_AC4 0xB0
#define BMP180_REGISTER_CAL_AC5 0xB2
#define BMP180_REGISTER_CAL_AC6 0xB4
#define BMP180_REGISTER_CAL_B1  0xB6
#define BMP180_REGISTER_CAL_B2  0xB8
#define BMP180_REGISTER_CAL_MB  0xBA
#define BMP180_REGISTER_CAL_MC  0xBC
#define BMP180_REGISTER_CAL_MD  0xBE

  byte id;

  //check the whoAmI register
  if (!testSensor(BMP180_ADDRESS)) {
    if (settings.testMode) {
      Serial.println(F("BMP180 not found!"));
    } return false;
  }
  id = read8(0xD0, BMP180_ADDRESS);
  if (id != 0x55) {
    if (settings.testMode) {
      Serial.println(F("BMP180 not found!"));
    } return false;
  }
  if (settings.testMode) {
    Serial.println(F("BMP180 OK!"));
  }

  //Read calibration coefficients
  _BMP180_coeffs.ac1 = readS16(BMP180_REGISTER_CAL_AC1, BMP180_ADDRESS);
  _BMP180_coeffs.ac2 = readS16(BMP180_REGISTER_CAL_AC2, BMP180_ADDRESS);
  _BMP180_coeffs.ac3 = readS16(BMP180_REGISTER_CAL_AC3, BMP180_ADDRESS);
  _BMP180_coeffs.ac4 = read16(BMP180_REGISTER_CAL_AC4, BMP180_ADDRESS);
  _BMP180_coeffs.ac5 = read16(BMP180_REGISTER_CAL_AC5, BMP180_ADDRESS);
  _BMP180_coeffs.ac6 = read16(BMP180_REGISTER_CAL_AC6, BMP180_ADDRESS);
  _BMP180_coeffs.b1 = readS16(BMP180_REGISTER_CAL_B1, BMP180_ADDRESS);
  _BMP180_coeffs.b2 = readS16(BMP180_REGISTER_CAL_B2, BMP180_ADDRESS);
  _BMP180_coeffs.mb = readS16(BMP180_REGISTER_CAL_MB, BMP180_ADDRESS);
  _BMP180_coeffs.mc = readS16(BMP180_REGISTER_CAL_MC, BMP180_ADDRESS);
  _BMP180_coeffs.md = readS16(BMP180_REGISTER_CAL_MD, BMP180_ADDRESS);

  return true;
}

void initiateTemp() {
#define BMP180_REGISTER_CONTROL         0xF4
#define BMP180_REGISTER_READTEMPCMD     0x2E
#define BMP180_REGISTER_READPRESSURECMD 0x34

  write8(BMP180_REGISTER_CONTROL, BMP180_ADDRESS, BMP180_REGISTER_READTEMPCMD);
}

void initiatePressure(float *temp) {
  int32_t  ut = 0;
  int32_t UT, X1, X2, B5;     // following ds convention
  float t;

  //Read ucompensated temperature
#define BMP180_REGISTER_TEMPDATA 0xF6
  uint16_t rt;
  rt = read16(BMP180_REGISTER_TEMPDATA, BMP180_ADDRESS);
  ut = rt;

  //Calculate true temperature
  X1 = (ut - (int32_t)_BMP180_coeffs.ac6) * ((int32_t)_BMP180_coeffs.ac5) >> 15;
  X2 = ((int32_t)_BMP180_coeffs.mc << 11) / (X1 + (int32_t)_BMP180_coeffs.md);
  _BMP180_coeffs.b5 = X1 + X2;
  t = (_BMP180_coeffs.b5 + 8) >> 4;
  t /= 10;
  *temp = t;

  //Initiate Pressure
  write8(BMP180_REGISTER_CONTROL, BMP180_ADDRESS, BMP180_REGISTER_READPRESSURECMD + (_BMP180Mode << 6));
}

void getPressure(float *pressure) {

  uint8_t  p8;
  uint16_t p16;
  int32_t  up = 0, compp = 0;
  int32_t  x1, x2, b5, b6, x3, b3, p;
  uint32_t b4, b7;

  //Read uncompensated pressure
#define BMP180_REGISTER_PRESSUREDATA 0xF6
  p16 = read16(BMP180_REGISTER_PRESSUREDATA, BMP180_ADDRESS);
  up = (uint32_t)p16 << 8;
  p8 = read8(BMP180_REGISTER_PRESSUREDATA + 2, BMP180_ADDRESS);
  up += p8;
  up >>= (8 - _BMP180Mode);

  //Calculate true pressure
  b6 = _BMP180_coeffs.b5 - 4000;
  x1 = (_BMP180_coeffs.b2 * ((b6 * b6) >> 12)) >> 11;
  x2 = (_BMP180_coeffs.ac2 * b6) >> 11;
  x3 = x1 + x2;
  b3 = (((((int32_t) _BMP180_coeffs.ac1) * 4 + x3) << _BMP180Mode) + 2) >> 2;
  x1 = (_BMP180_coeffs.ac3 * b6) >> 13;
  x2 = (_BMP180_coeffs.b1 * ((b6 * b6) >> 12)) >> 16;
  x3 = ((x1 + x2) + 2) >> 2;
  b4 = (_BMP180_coeffs.ac4 * (uint32_t) (x3 + 32768)) >> 15;
  b7 = ((uint32_t) (up - b3) * (50000 >> _BMP180Mode));

  if (b7 < 0x80000000) {
    p = (b7 << 1) / b4;
  }
  else {
    p = (b7 / b4) << 1;
  }

  x1 = (p >> 8) * (p >> 8);
  x1 = (x1 * 3038) >> 16;
  x2 = (-7357 * p) >> 16;
  compp = p + ((x1 + x2 + 3791) >> 4);

  /* Assign compensated pressure value */
  *pressure = compp / 100.0F;
}

//***************************************************************************
//BMP280 Barometric Pressure Sensor
//***************************************************************************
/***************************************************************************
  This is a library for the BMP280 pressure sensor

  Designed specifically to work with the Adafruit BMP280 Breakout
  ----> http://www.adafruit.com/products/2651

  These sensors use I2C to communicate, 2 pins are required to interface.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Kevin Townsend for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/
#define BMP280_ADDRESS                (0x77)

enum
{
  BMP280_REGISTER_DIG_T1              = 0x88,
  BMP280_REGISTER_DIG_T2              = 0x8A,
  BMP280_REGISTER_DIG_T3              = 0x8C,

  BMP280_REGISTER_DIG_P1              = 0x8E,
  BMP280_REGISTER_DIG_P2              = 0x90,
  BMP280_REGISTER_DIG_P3              = 0x92,
  BMP280_REGISTER_DIG_P4              = 0x94,
  BMP280_REGISTER_DIG_P5              = 0x96,
  BMP280_REGISTER_DIG_P6              = 0x98,
  BMP280_REGISTER_DIG_P7              = 0x9A,
  BMP280_REGISTER_DIG_P8              = 0x9C,
  BMP280_REGISTER_DIG_P9              = 0x9E,

  BMP280_REGISTER_CHIPID             = 0xD0,
  BMP280_REGISTER_VERSION            = 0xD1,
  BMP280_REGISTER_SOFTRESET          = 0xE0,

  BMP280_REGISTER_CAL26              = 0xE1,  // R calibration stored in 0xE1-0xF0

  BMP280_REGISTER_CONTROL            = 0xF4,
  BMP280_REGISTER_CONFIG             = 0xF5,
  BMP280_REGISTER_PRESSUREDATA       = 0xF7,
  BMP280_REGISTER_TEMPDATA           = 0xFA,
};

typedef struct
{
  uint16_t dig_T1;
  int16_t  dig_T2;
  int16_t  dig_T3;

  uint16_t dig_P1;
  int16_t  dig_P2;
  int16_t  dig_P3;
  int16_t  dig_P4;
  int16_t  dig_P5;
  int16_t  dig_P6;
  int16_t  dig_P7;
  int16_t  dig_P8;
  int16_t  dig_P9;

  uint8_t  dig_H1;
  int16_t  dig_H2;
  uint8_t  dig_H3;
  int16_t  dig_H4;
  int16_t  dig_H5;
  int8_t   dig_H6;
} bmp280_calib_data;

bmp280_calib_data _bmp280_calib;

boolean beginBMP280() {

  if (!testSensor(BMP280_ADDRESS)) {
    if (settings.testMode) {
      Serial.println(F("BMP280 not found!"));
    } return false;
  }
  if (read8(BMP280_REGISTER_CHIPID, BMP280_ADDRESS) != 0x58) {
    if (settings.testMode) {
      Serial.println(F("BMP280 not found!"));
    } return false;
  }
  if (settings.testMode) {
    Serial.println(F("BMP280 OK!"));
  }

  //set the sampling control rate
  timeBtwnBaro = 38100UL;
 
  delay(100);
  readBMP280Coefficients();
  //put the BMP280 into sleep mode
  write8(BMP280_REGISTER_CONTROL, BMP280_ADDRESS, 0x00);
  //Set the config register to 0.5ms standby, 16 IIR coeff,
  write8(BMP280_REGISTER_CONFIG,  BMP280_ADDRESS, 0b00010100);//16 = 0b00011100 = 0x1C, 8 = 0b00001100 = 0x0C
  //Set the control register to Ultra high resolution and sleep mode: osrr_t,osrs_p,mode
  write8(BMP280_REGISTER_CONTROL, BMP280_ADDRESS, 0b01010111);//0b00111111 = 0x3F, 0b01010100 = 0x54, 0b01011111 = 0x5F

  return true;
}

void readBMP280Coefficients() {

  _bmp280_calib.dig_T1 = read16_LE(BMP280_REGISTER_DIG_T1, BMP280_ADDRESS);
  _bmp280_calib.dig_T2 = readS16_LE(BMP280_REGISTER_DIG_T2, BMP280_ADDRESS);
  _bmp280_calib.dig_T3 = readS16_LE(BMP280_REGISTER_DIG_T3, BMP280_ADDRESS);

  _bmp280_calib.dig_P1 = read16_LE(BMP280_REGISTER_DIG_P1, BMP280_ADDRESS);
  _bmp280_calib.dig_P2 = readS16_LE(BMP280_REGISTER_DIG_P2, BMP280_ADDRESS);
  _bmp280_calib.dig_P3 = readS16_LE(BMP280_REGISTER_DIG_P3, BMP280_ADDRESS);
  _bmp280_calib.dig_P4 = readS16_LE(BMP280_REGISTER_DIG_P4, BMP280_ADDRESS);
  _bmp280_calib.dig_P5 = readS16_LE(BMP280_REGISTER_DIG_P5, BMP280_ADDRESS);
  _bmp280_calib.dig_P6 = readS16_LE(BMP280_REGISTER_DIG_P6, BMP280_ADDRESS);
  _bmp280_calib.dig_P7 = readS16_LE(BMP280_REGISTER_DIG_P7, BMP280_ADDRESS);
  _bmp280_calib.dig_P8 = readS16_LE(BMP280_REGISTER_DIG_P8, BMP280_ADDRESS);
  _bmp280_calib.dig_P9 = readS16_LE(BMP280_REGISTER_DIG_P9, BMP280_ADDRESS);
}

void getBMP280() {

  //---------------------------------------------------------------
  //burst read from the registers
  //---------------------------------------------------------------
  readSensor(BMP280_ADDRESS, BMP280_REGISTER_PRESSUREDATA, 6);

  int32_t adc_P;
  adc_P = rawData[0];
  adc_P <<= 8;
  adc_P |= rawData[1];
  adc_P <<= 8;
  adc_P |= rawData[2];
  adc_P >>= 4;

  int32_t adc_T;
  adc_T = rawData[3];
  adc_T <<= 8;
  adc_T |= rawData[4];
  adc_T <<= 8;
  adc_T |= rawData[5];
  adc_T >>= 4;

  //put the BMP280 into forced mode
  write8(BMP280_REGISTER_CONTROL, BMP280_ADDRESS, 0b10101010); //0b01010110 = 0x56

  //---------------------------------------------------------------
  //compensate and compute temperature
  //---------------------------------------------------------------

  int32_t var1, var2, t_fine;

  var1  = ((((adc_T >> 3) - ((int32_t)_bmp280_calib.dig_T1 << 1))) *
           ((int32_t)_bmp280_calib.dig_T2)) >> 11;

  var2  = (((((adc_T >> 4) - ((int32_t)_bmp280_calib.dig_T1)) *
             ((adc_T >> 4) - ((int32_t)_bmp280_calib.dig_T1))) >> 12) *
           ((int32_t)_bmp280_calib.dig_T3)) >> 14;

  t_fine = var1 + var2;

  temperature  = (t_fine * 5 + 128) >> 8;
  temperature *= .01;
  //---------------------------------------------------------------
  //Read pressure and convert
  //---------------------------------------------------------------

  int64_t var3, var4, p;

  var3 = ((int64_t)t_fine) - 128000;
  var4 = var3 * var3 * (int64_t)_bmp280_calib.dig_P6;
  var4 = var4 + ((var3 * (int64_t)_bmp280_calib.dig_P5) << 17);
  var4 = var4 + (((int64_t)_bmp280_calib.dig_P4) << 35);
  var3 = ((var3 * var3 * (int64_t)_bmp280_calib.dig_P3) >> 8) +
         ((var3 * (int64_t)_bmp280_calib.dig_P2) << 12);
  var3 = (((((int64_t)1) << 47) + var3)) * ((int64_t)_bmp280_calib.dig_P1) >> 33;

  if (var3 == 0) {
    pressure = 0;  // avoid exception caused by division by zero
  }
  else {
    p = 1048576 - adc_P;
    p = (((p << 31) - var4) * 3125) / var3;
    var3 = (((int64_t)_bmp280_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var4 = (((int64_t)_bmp280_calib.dig_P8) * p) >> 19;

    p = ((p + var3 + var4) >> 8) + (((int64_t)_bmp280_calib.dig_P7) << 4);
    pressure = (float)p / 256;
  }

  //--------------------------------------------
  //Calculate Altitude
  //--------------------------------------------

  pressure *= 0.01;

  Alt = 44330 * (1.0 - pow(pressure / (seaLevelPressure), 0.1903));
}

//***************************************************************************
//BMP388 Barometric Pressure Sensor
//***************************************************************************
#define BMP388_ADDRESS                (0x77)

enum {
  REGISTER_NVM_PAR_P11 = 0x45,
  REGISTER_NVM_PAR_P10 = 0x44,
  REGISTER_NVM_PAR_P09U = 0x43,
  REGISTER_NVM_PAR_P09L = 0x42,
  REGISTER_NVM_PAR_P08 = 0x41,
  REGISTER_NVM_PAR_P07 = 0x40,
  REGISTER_NVM_PAR_P06U = 0x3F,
  REGISTER_NVM_PAR_P06L = 0x3E,
  REGISTER_NVM_PAR_P05U = 0x3D,
  REGISTER_NVM_PAR_P05L = 0x3C,
  REGISTER_NVM_PAR_P04 = 0x3B,
  REGISTER_NVM_PAR_P03 = 0x3A,
  REGISTER_NVM_PAR_P02U = 0x39,
  REGISTER_NVM_PAR_P02L = 0x38,
  REGISTER_NVM_PAR_P01U = 0x37,
  REGISTER_NVM_PAR_P01L = 0x36,

  REGISTER_NVM_PAR_T3 = 0x35,
  REGISTER_NVM_PAR_T2U = 0x34,
  REGISTER_NVM_PAR_T2L = 0x33,
  REGISTER_NVM_PAR_T1U = 0x32,
  REGISTER_NVM_PAR_T1L = 0x31,

};

typedef struct
{
  int8_t    NVM_PAR_P11;
  int8_t    NVM_PAR_P10;
  int16_t   NVM_PAR_P09;
  int8_t    NVM_PAR_P08;
  int8_t    NVM_PAR_P07;
  uint16_t  NVM_PAR_P06;
  uint16_t  NVM_PAR_P05;
  int8_t    NVM_PAR_P04;
  int8_t    NVM_PAR_P03;
  int16_t   NVM_PAR_P02;
  int16_t   NVM_PAR_P01;

  int8_t    NVM_PAR_T3;
  uint16_t  NVM_PAR_T2;
  uint16_t  NVM_PAR_T1;

  double    PAR_P11 ;
  double    PAR_P10 ;
  double    PAR_P09 ;
  double    PAR_P08 ;
  double    PAR_P07 ;
  double    PAR_P06 ;
  double    PAR_P05 ;
  double    PAR_P04 ;
  double    PAR_P03 ;
  double    PAR_P02 ;
  double    PAR_P01 ;

  double    PAR_T3 ;
  double    PAR_T2 ;
  double    PAR_T1 ;

  float     t_lin ;

} bmp388_calib_data;

bmp388_calib_data bmp388cal;

boolean beginBMP388() {

#define BMP388_REGISTER_CHIPID 0x00

  if (!testSensor(BMP388_ADDRESS)) {
    if (settings.testMode) {
      Serial.println(F("BMP388 not found!"));
    } return false;
  }
  if (read8(BMP388_REGISTER_CHIPID, BMP388_ADDRESS) != 0x50) {
    if (settings.testMode) {
      Serial.println(F("BMP388 not found!"));
    } return false;
  }
  if (settings.testMode) {
    Serial.println(F("BMP388 OK!"));
  }

#define BMP388_REGISTER_PWR_CTRL  0x1B
#define BMP388_REGISTER_CONFIG    0x1F
#define BMP388_REGISTER_OSR       0x1C

  delay(100);

  readBMP388Coefficients();

  //put the BMP388 into sleep mode
  write8(BMP388_REGISTER_PWR_CTRL, BMP388_ADDRESS, 0x00);

  //Set the config register 4 IIR coeff: 010 = 4
  write8(BMP388_REGISTER_CONFIG,  BMP388_ADDRESS, 0b00000100);

  //Set the control register to Ultra high resolution
  //osr_p: 100 = 16x (2:0)
  //osr_t: 001 = 2x  (5:3)
  write8(BMP388_REGISTER_OSR, BMP388_ADDRESS, 0b00001100);

  //set the sampling control rate
  timeBtwnBaro = 40000UL;

  return true;
}

void readBMP388Coefficients() {

  double denominator;

  bmp388cal.NVM_PAR_P11 = readS8(REGISTER_NVM_PAR_P11, BMP388_ADDRESS);
  bmp388cal.NVM_PAR_P10 = readS8(REGISTER_NVM_PAR_P10, BMP388_ADDRESS);
  bmp388cal.NVM_PAR_P09 = readS16_LE(REGISTER_NVM_PAR_P09L, BMP388_ADDRESS);
  bmp388cal.NVM_PAR_P08 = readS8(REGISTER_NVM_PAR_P08, BMP388_ADDRESS);
  bmp388cal.NVM_PAR_P07 = readS8(REGISTER_NVM_PAR_P07, BMP388_ADDRESS);
  bmp388cal.NVM_PAR_P06 = read16_LE(REGISTER_NVM_PAR_P06L, BMP388_ADDRESS);
  bmp388cal.NVM_PAR_P05 = read16_LE(REGISTER_NVM_PAR_P05L, BMP388_ADDRESS);
  bmp388cal.NVM_PAR_P04 = readS8(REGISTER_NVM_PAR_P04, BMP388_ADDRESS);
  bmp388cal.NVM_PAR_P03 = readS8(REGISTER_NVM_PAR_P03, BMP388_ADDRESS);
  bmp388cal.NVM_PAR_P02 = readS16_LE(REGISTER_NVM_PAR_P02L, BMP388_ADDRESS);
  bmp388cal.NVM_PAR_P01 = readS16_LE(REGISTER_NVM_PAR_P01L, BMP388_ADDRESS);

  bmp388cal.NVM_PAR_T3 = readS8(REGISTER_NVM_PAR_T3, BMP388_ADDRESS);
  bmp388cal.NVM_PAR_T2 = read16_LE(REGISTER_NVM_PAR_T2L, BMP388_ADDRESS);
  bmp388cal.NVM_PAR_T1 = read16_LE(REGISTER_NVM_PAR_T1L, BMP388_ADDRESS);

  denominator = 0.00390625f;
  bmp388cal.PAR_T1 = (double)bmp388cal.NVM_PAR_T1 / denominator;

  denominator = 1073741824.0f;
  bmp388cal.PAR_T2 = (double)bmp388cal.NVM_PAR_T2 / denominator;

  denominator = 281474976710656.0f;
  bmp388cal.PAR_T3 = (double)bmp388cal.NVM_PAR_T3 / denominator;

  denominator = 36893488147419103232.0f;
  bmp388cal.PAR_P11 = (double)bmp388cal.NVM_PAR_P11 / denominator;

  denominator = 281474976710656.0f;
  bmp388cal.PAR_P10 = (double)bmp388cal.NVM_PAR_P10 / denominator;

  bmp388cal.PAR_P09 = (double)bmp388cal.NVM_PAR_P09 / denominator;

  denominator = 32768.0f;
  bmp388cal.PAR_P08 = (double)bmp388cal.NVM_PAR_P08 / denominator;

  denominator = 256.0f;
  bmp388cal.PAR_P07 = (double)bmp388cal.NVM_PAR_P07 / denominator;

  denominator = 64.0f;
  bmp388cal.PAR_P06 = (double)bmp388cal.NVM_PAR_P06 / denominator;

  denominator = 0.125f;
  bmp388cal.PAR_P05 = (double)bmp388cal.NVM_PAR_P05 / denominator;

  denominator = 137438953472.0f;
  bmp388cal.PAR_P04 = (double)bmp388cal.NVM_PAR_P04 / denominator;

  denominator = 4294967296.0f;
  bmp388cal.PAR_P03 = (double)bmp388cal.NVM_PAR_P03 / denominator;

  denominator = 536870912.0f;
  bmp388cal.PAR_P02 = ((double)bmp388cal.NVM_PAR_P02 - 16384.0f) / denominator;

  denominator = 1048576.0f;
  bmp388cal.PAR_P01 = ((double)bmp388cal.NVM_PAR_P01 - 16384.0f) / denominator;
}

void getBMP388() {

  //---------------------------------------------------------------
  //burst read from the registers
  //---------------------------------------------------------------

#define BMP388_REGISTER_PRESSUREDATA 0x04

  readSensor(BMP388_ADDRESS, BMP388_REGISTER_PRESSUREDATA, 6);

  uint32_t uncomp_press;
  /*uncomp_press = Wire2.read();
    uncomp_press <<= 8;
    uncomp_press |= Wire2.read();
    uncomp_press <<= 8;
    uncomp_press |= Wire2.read();
    uncomp_press >>= 4;*/
  uncomp_press = rawData[0];
  uncomp_press += (rawData[1] << 8);
  uncomp_press += (rawData[2] << 16);

  uint32_t uncomp_temp;
  /*uncomp_temp = Wire2.read();
    uncomp_temp <<= 8;
    uncomp_temp |= Wire2.read();
    uncomp_temp <<= 8;
    uncomp_temp |= Wire2.read();
    uncomp_temp >>= 4;*/
  uncomp_temp = rawData[3];
  uncomp_temp += (rawData[4] << 8);
  uncomp_temp += (rawData[5] << 16);

  //put the BMP388 into forced mode
  //press_en: 1 (0)
  //temp_en:  1 (1)
  //mode:     01 (5:4)
  write8(BMP388_REGISTER_PWR_CTRL, BMP388_ADDRESS, 0b00010011);

  //---------------------------------------------------------------
  //compensate and compute temperature
  //---------------------------------------------------------------

  float partial_data1;
  float partial_data2;
  float partial_data3;
  float partial_data4;
  float partial_out1;
  float partial_out2;

  partial_data1 = (float)(uncomp_temp - bmp388cal.PAR_T1);

  partial_data2 = (float)(partial_data1 * bmp388cal.PAR_T2);

  bmp388cal.t_lin = partial_data2 + (partial_data1 * partial_data1) * bmp388cal.PAR_T3;

  temperature = bmp388cal.t_lin;
  //temperature *= .01;

  //---------------------------------------------------------------
  //compensate and compute pressure
  //---------------------------------------------------------------

  partial_data1 = bmp388cal.PAR_P06 * bmp388cal.t_lin;
  partial_data2 = bmp388cal.PAR_P07 * (bmp388cal.t_lin * bmp388cal.t_lin);
  partial_data3 = bmp388cal.PAR_P08 * (bmp388cal.t_lin * bmp388cal.t_lin * bmp388cal.t_lin);
  partial_out1 = bmp388cal.PAR_P05 + partial_data1 + partial_data2 + partial_data3;

  partial_data1 = bmp388cal.PAR_P02 * bmp388cal.t_lin;
  partial_data2 = bmp388cal.PAR_P03 * (bmp388cal.t_lin * bmp388cal.t_lin);
  partial_data3 = bmp388cal.PAR_P04 * (bmp388cal.t_lin * bmp388cal.t_lin * bmp388cal.t_lin);
  partial_out2 = (float)uncomp_press * (bmp388cal.PAR_P01 + partial_data1 + partial_data2 + partial_data3);

  partial_data1 = (float)uncomp_press * (float)uncomp_press;
  partial_data2 = bmp388cal.PAR_P09 + bmp388cal.PAR_P10 * bmp388cal.t_lin;
  partial_data3 = partial_data1 * partial_data2;
  partial_data4 = partial_data3 + ((float)uncomp_press * (float)uncomp_press * (float)uncomp_press) * bmp388cal.PAR_P11;
  pressure  = partial_out1 + partial_out2 + partial_data4;

  //--------------------------------------------
  //Calculate Altitude
  //--------------------------------------------

  pressure *= 0.01;

  Alt = 44330 * (1.0 - pow(pressure / (seaLevelPressure), 0.1903));
}

//***************************************************************************
//MS5611 Barometric Pressure Sensor
//***************************************************************************
#define MS5611_ADDRESS                (0x77)

int16_t MS5611_PROM[6];
int32_t MS5611_dT;
int32_t MS5611_TEMP;

bool beginMS5611(){

  #define MS5611RdPROM 0xA2

  if (!testSensor(MS5611_ADDRESS)) {
    if (settings.testMode) {
      Serial.println(F("BMP280 not found!"));
    } return false;}

  //read PROM
  for(byte ii = 0; ii < 6; ii++){
    CmdMS5611(MS5611RdPROM + 2*ii);
    ReadMS5611(2);
    MS5611_PROM[ii] = (rawData[0] << 8) | rawData[1];}

  //get initial temperature
  CmdMS5611(0x58);
  delayMicroseconds(9040);
  ReadMS5611(3);
  ConvertTempMS5611();

  //get initial pressure
  CmdMS5611(0x48);
  
  return true;}

void getMS5611(){
  
  static byte counter = 0;
  static float prevAlt = 0;

  //Get pressure
  if(counter < 11){
    
    //D2 at 4096
    //Read pressure and issue next command
    ReadMS5611(3);
    ConvertPressMS5611();
    counter++;

    //if we have done 10 pressure readings, then initiate a temperature reading
    if(counter > 10){CmdMS5611(0x58);}
    //else issue the next pressure command
    else{CmdMS5611(0x48);}}

  //Read temp and issue command for pressure
  else if(counter == 11){
    ReadMS5611(3);
    temperature = ConvertTempMS5611();
    CmdMS5611(0x48);
    counter = 0;}
}

void CmdMS5611(byte cmd){

switch (pins.i2c) {
  
    case 0:
      Wire.beginTransmission(MS5611_ADDRESS);
      Wire.write(cmd);
      Wire.endTransmission(I2C_STOP);
      break;
    
    case 1:
      Wire1.beginTransmission(MS5611_ADDRESS);
      Wire1.write(cmd);
      Wire1.endTransmission(I2C_STOP);
      break;
      
    case 2:
      Wire2.beginTransmission(MS5611_ADDRESS);
      Wire2.write(cmd);
      Wire2.endTransmission(I2C_STOP);
      break;}
}

void ReadMS5611(byte bytes){
  
switch (pins.i2c) {

    case 0:
      Wire.requestFrom(MS5611_ADDRESS, bytes);
      while (Wire.available() < bytes) {};
      for (byte i = 0; i < bytes; i++) {rawData[i] = Wire.read();}
      break;

   case 1:
      Wire1.requestFrom(MS5611_ADDRESS, bytes);
      while (Wire1.available() < bytes) {};
      for (byte i = 0; i < bytes; i++) {rawData[i] = Wire1.read();}
      break;

   case 2:
      Wire2.requestFrom(MS5611_ADDRESS, bytes);
      while (Wire2.available() < bytes) {};
      for (byte i = 0; i < bytes; i++) {rawData[i] = Wire2.read();}
      break;}

}

float ConvertTempMS5611(){

  uint16_t C6 = MS5611_PROM[5];
  uint16_t C5 = MS5611_PROM[4];

  uint32_t D2 = (rawData[0]<<16) + (rawData[1]<<8) + rawData[2];

  MS5611_dT = D2 - (C5*256);

  MS5611_TEMP = 2000 + ((MS5611_dT*C6)/8388608);
  
  int32_t T2 = 0L;
  if(MS5611_TEMP < 2000){T2 = (MS5611_dT  * MS5611_dT)/2147483648;}

  MS5611_TEMP -= T2;
   
  float finalTemp = ((float)MS5611_TEMP)*0.01;

  return finalTemp;}

float ConvertPressMS5611(){

  uint16_t C1 = MS5611_PROM[0];
  uint16_t C2 = MS5611_PROM[1];
  uint16_t C3 = MS5611_PROM[2];
  uint16_t C4 = MS5611_PROM[3];

  int64_t OFF1 = (C2*2^16) + ((C4*MS5611_dT)/128);
  int64_t SENS = (C1*2^15) + ((C3*MS5611_dT)/256);
  
  int64_t OFF2 = 0;
  int64_t SENS2 = 0;
  if(temperature < 20.0F){
    OFF2 = 5 * (MS5611_TEMP - 2000) * (MS5611_TEMP - 2000) / 2;
    SENS2 = 5 * (MS5611_TEMP - 2000) * (MS5611_TEMP - 2000) / 4;}

  if(temperature < -15.0F){
    OFF2 += 7*(MS5611_TEMP +1500)*(MS5611_TEMP +1500);
    SENS2 += 11*(MS5611_TEMP +1500)*(MS5611_TEMP +1500)/2;}

  OFF1 -= OFF2;
  SENS -= SENS2;

  uint32_t D1 = (rawData[0] << 16) | (rawData[1] << 8) | rawData[2];
    
  int32_t p = (D1*SENS/2097152 - OFF1)/32768;

  pressure = (float)p *0.01;

  Alt = 44330 * (1.0 - pow(pressure / (seaLevelPressure), 0.1903));

  return pressure;}
  
//***************************************************************************
//Common I2C helper functions
//***************************************************************************

uint16_t read16(byte reg, byte _i2caddr) {

  uint16_t value;
  readSensor(_i2caddr, reg, 2);
  value = (rawData[0] << 8) | rawData[1];
  return value;
}

uint16_t read16_LE(byte reg, byte _i2caddr) {
  uint16_t temp = read16(reg, _i2caddr);
  return (temp >> 8) | (temp << 8);
}

int16_t readS16_LE(byte reg, byte _i2caddr) {
  return (int16_t)read16_LE(reg, _i2caddr);
}

int16_t readS16(byte reg, byte _i2caddr) {
  return (int16_t)read16(reg, _i2caddr);
}

int8_t readS8(byte reg, byte _i2caddr) {

  int8_t value;
  readSensor(_i2caddr, reg, 1);
  value = rawData[0];
  return value;
}

uint8_t read8(byte reg, byte _i2caddr) {

  uint8_t value;
  readSensor(_i2caddr, reg, (byte)1);
  value = rawData[0];
  return value;
}

void write8(byte reg, byte _i2caddr, byte value) {

  switch (pins.i2c) {

    case 0:
      Wire.beginTransmission((uint8_t)_i2caddr);
      Wire.write((uint8_t)reg);
      Wire.write((uint8_t)value);
      Wire.endTransmission(I2C_STOP);
      break;

    case 1:
      Wire1.beginTransmission((uint8_t)_i2caddr);
      Wire1.write((uint8_t)reg);
      Wire1.write((uint8_t)value);
      Wire1.endTransmission(I2C_STOP);
      break;

    case 2:
      Wire2.beginTransmission((uint8_t)_i2caddr);
      Wire2.write((uint8_t)reg);
      Wire2.write((uint8_t)value);
      Wire2.endTransmission(I2C_STOP);
      break;
  }
}
