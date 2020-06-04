//#######################################################################################################
//######################## Plugin 014 SI7021 I2C Temperature Humidity Sensor  ###########################
//#######################################################################################################

/*
 * Commands:
 * SI7021 <variable Temperature>,<variable Humidity>[,<custom delay>]   Read temperature and humidity values into variables, use optional custom delayvalue in mSec
*/

#ifdef USES_P014
#define P014_BUILD            6
#define PLUGIN_014
#define PLUGIN_ID_014        14

// ======================================
// SI7021 sensor 
// ======================================
#define SI7021_I2C_ADDRESS      0x40 // I2C address for the sensor
#define SI7021_MEASURE_TEMP     0xF3
#define SI7021_MEASURE_HUM      0xF5
#define SI7021_WRITE_REG        0xE6
#define SI7021_READ_REG         0xE7
#define SI7021_SOFT_RESET       0xFE

// SI7021 Sensor resolution
// default at power up is SI7021_RESOLUTION_14T_12RH
#define SI7021_RESOLUTION_14T_12RH 0x00 // 12 bits RH / 14 bits Temp
#define SI7021_RESOLUTION_13T_10RH 0x80 // 10 bits RH / 13 bits Temp
#define SI7021_RESOLUTION_12T_08RH 0x01 //  8 bits RH / 12 bits Temp
#define SI7021_RESOLUTION_11T_11RH 0x81 // 11 bits RH / 11 bits Temp
#define SI7021_RESOLUTION_MASK 0B01111110

boolean Plugin_014_init = false;
uint8_t  si7021_humidity;    // latest humidity value read
int16_t  si7021_temperature; // latest temperature value read (*100)

boolean Plugin_014(byte function, String& cmd, String& params)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_INFO:
      {
        printWebTools += F("<TR><TD>P014 - SI7021<TD>");
        printWebTools += P014_BUILD;
        break;
      }
          
    case PLUGIN_WRITE:
      {
        if (cmd.equalsIgnoreCase(F("SI7021")))
        {
          success = true;
          String varNameTemp = parseString(params,1);
          String varNameHum = parseString(params,2);
          byte customDelay = parseString(params,3).toInt();
          
          if (!Plugin_014_init)
            Plugin_014_init = Plugin_014_si7021_begin(0);

          // Read values only if init has been done okay
          if (Plugin_014_init && Plugin_014_si7021_readValues(0,customDelay) == 0) {
            setNvar(varNameTemp, si7021_temperature/100.0);
            setNvar(varNameHum, si7021_humidity);
            String log = F("SI7021 T:");
            log += si7021_temperature/100.0;
            log += F(" H:");
            log += si7021_humidity;
            logger->println(log);
          } else {
            String log = F("SI7021 Read Error!");
            logger->println(log);
          }
        break;
      }
    }
  }
  return success;
}


/* ======================================================================
Function: Plugin_014_si7021_begin
Purpose : read the user register from the sensor
Input   : user register value filled by function
Output  : true if okay
Comments: -
====================================================================== */
boolean Plugin_014_si7021_begin(uint8_t resolution)
{
  uint8_t ret;

  // Set the resolution we want
  ret = Plugin_014_si7021_setResolution(resolution);
  if ( ret == 0 ) {
    ret = true;
  } else {
    ret = false;
  }

  return ret; 
}


/* ======================================================================
Function: Plugin_014_si7021_checkCRC
Purpose : check the CRC of received data
Input   : value read from sensor
Output  : CRC read from sensor
Comments: 0 if okay
====================================================================== */
uint8_t Plugin_014_si7021_checkCRC(uint16_t data, uint8_t check)
{
  uint32_t remainder, divisor;

  //Pad with 8 bits because we have to add in the check value
  remainder = (uint32_t)data << 8; 

  // From: http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
  // POLYNOMIAL = 0x0131 = x^8 + x^5 + x^4 + 1 : http://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks
  // 0x988000 is the 0x0131 polynomial shifted to farthest left of three bytes
  divisor = (uint32_t) 0x988000;

  // Add the check value
  remainder |= check; 

  // Operate on only 16 positions of max 24. 
  // The remaining 8 are our remainder and should be zero when we're done.
  for (uint8_t i = 0 ; i < 16 ; i++) {
    //Check if there is a one in the left position
    if( remainder & (uint32_t)1<<(23 - i) ) 
      remainder ^= divisor;

    //Rotate the divisor max 16 times so that we have 8 bits left of a remainder
    divisor >>= 1; 
  }
  return ((uint8_t) remainder);
}


/* ======================================================================
Function: si7021_readRegister
Purpose : read the user register from the sensor
Input   : user register value filled by function
Output  : 0 if okay
Comments: -
====================================================================== */
int8_t Plugin_014_si7021_readRegister(uint8_t * value)
{
  uint8_t error ;

  // Request user register
  Wire.beginTransmission(SI7021_I2C_ADDRESS);
  Wire.write(SI7021_READ_REG); 
  Wire.endTransmission();
  
  // request 1 byte result  
  Wire.requestFrom(SI7021_I2C_ADDRESS, 1);
  if (Wire.available()>=1) {
      *value = Wire.read();
      return 0;
  }
  
  return 1;  
}


/* ======================================================================
Function: Plugin_014_si7021_startConv
Purpose : return temperature or humidity measured 
Input   : data type SI7021_READ_HUM or SI7021_READ_TEMP
          current config resolution
Output  : 0 if okay
Comments: internal values of temp and rh are set
====================================================================== */
int8_t Plugin_014_si7021_startConv(uint8_t datatype, uint8_t resolution, byte customDelay)
{
  long data;
  uint16_t raw ;
  uint8_t checksum,tmp;
  uint8_t error;

  //Request a reading 
  Wire.beginTransmission(SI7021_I2C_ADDRESS);
  Wire.write(datatype);
  Wire.endTransmission();

  // Max for SI7021 is 3/5/7/12 ms
  // max for HTU21D is 7/13/25/50 ms
  
  if (resolution == SI7021_RESOLUTION_11T_11RH)
    tmp = 7;
  else if (resolution == SI7021_RESOLUTION_12T_08RH)
    tmp = 13;
  else if (resolution == SI7021_RESOLUTION_13T_10RH)
    tmp = 25;
  else 
    tmp = 50;

  // Humidity fire also temp measurment so delay 
  // need to be increased by 2 if no Hold Master
  if (datatype == SI7021_MEASURE_HUM)
    tmp *=2;

  if (customDelay)
    tmp = customDelay;
    
  delay(tmp);

  if ( Wire.requestFrom(SI7021_I2C_ADDRESS, 3) < 3 ) {
    return -1;
  }

  // Comes back in three bytes, data(MSB) / data(LSB) / Checksum
  raw  = ((uint16_t) Wire.read()) << 8;
  raw |= Wire.read();
  checksum = Wire.read();

  // Check CRC of data received
  if(Plugin_014_si7021_checkCRC(raw, checksum) != 0) {
    return -1; 
  }

  // Humidity 
  if (datatype == SI7021_MEASURE_HUM) {
    // Convert value to Himidity percent 
    data = ((125 * (long)raw) >> 16) - 6;

    // Datasheet says doing this check
    if (data>100) data = 100;
    if (data<0)   data = 0;

    // save value
    si7021_humidity = (uint8_t) data;

  // Temperature
  } else  if (datatype == SI7021_MEASURE_TEMP) {
    // Convert value to Temperature (*100)
    // for 23.45C value will be 2345
    data =  ((17572 * (long)raw) >> 16) - 4685;

    // save value
    si7021_temperature = (int16_t) data;
  }

  return 0;
}


/* ======================================================================
Function: Plugin_014_si7021_readValues
Purpose : read temperature and humidity from SI7021 sensor
Input   : current config resolution
Output  : 0 if okay
Comments: -
====================================================================== */
int8_t Plugin_014_si7021_readValues(uint8_t resolution, byte customDelay)
{
  int8_t error = 0;

  // start humidity conversion
  error |= Plugin_014_si7021_startConv(SI7021_MEASURE_HUM, resolution, customDelay);

  // start temperature conversion
  error |= Plugin_014_si7021_startConv(SI7021_MEASURE_TEMP, resolution, customDelay);

  return error;
}


/* ======================================================================
Function: Plugin_014_si7021_setResolution
Purpose : Sets the sensor resolution to one of four levels 
Input   : see #define default is SI7021_RESOLUTION_14T_12RH
Output  : 0 if okay
Comments: -
====================================================================== */
int8_t Plugin_014_si7021_setResolution(uint8_t res)
{
  uint8_t reg;
  uint8_t error;

  // Get the current register value
  error = Plugin_014_si7021_readRegister(&reg);
  if ( error == 0) {
    // remove resolution bits
    reg &= SI7021_RESOLUTION_MASK ; 

    // Prepare to write to the register value
    Wire.beginTransmission(SI7021_I2C_ADDRESS);
    Wire.write(SI7021_WRITE_REG); 

    // Write the new resolution bits but clear unused before
    Wire.write(reg | ( res &= ~SI7021_RESOLUTION_MASK) ); 
    return (int8_t) Wire.endTransmission();
  } 

  return error;
}
#endif
