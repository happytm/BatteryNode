//#######################################################################################################
//#################################### Plugin 011: Pro Mini Extender ####################################
//#######################################################################################################

/*
 * Commands:
 * extGPIO <pin>,<state>                   Set GPIO <pin> output to state
 * extGPIORead <variable>,<pin>            Read GPIO input on <pin>
 * extAnalogRead <variable>,<pin>          Read analog value from <pin>
 * extEEPROMWrite <address>,<byte>         Store byte value to EEPROM address
 * extEEPROMRead <address>                 Read a byte from EEPROM address
 * extVCRead <variable>                    Read VCC value (milliVolts)
*/ 

#ifdef USES_P011
#define P011_BUILD            6
#define PLUGIN_011
#define PLUGIN_ID_011         11

#define PLUGIN_011_I2C_ADDRESS 0x26

#define CMD_DIGITAL_WRITE  1
#define CMD_DIGITAL_READ   2
#define CMD_ANALOG_WRITE   3
#define CMD_ANALOG_READ    4

#define CMD_EEPROM_WRITE   101
#define CMD_EEPROM_READ    102
#define CMD_VCC_READ      110

boolean Plugin_011(byte function, String& cmd, String& params)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_INFO:
      {
        printWebTools += F("<TR><TD>P011 - PME<TD>");
        printWebTools += P011_BUILD;
        break;
      }
      
    case PLUGIN_WRITE:
      {
        if (cmd.equalsIgnoreCase(F("extgpio")))
        {
          byte pin = parseString(params,1).toInt();
          byte state = parseString(params,2).toInt();
          Plugin_011_Write(CMD_DIGITAL_WRITE, pin, state);
          success = true;
        }

        if (cmd.equalsIgnoreCase(F("extgpioRead")))
        {
          String varName = parseString(params,1);
          byte pin = parseString(params,2).toInt();
          long value = Plugin_011_Read(CMD_DIGITAL_READ, pin);
          setNvar(varName, value);
          logger->println(value);
          success = true;
        }

        if (cmd.equalsIgnoreCase(F("extAnalogRead")))
        {
          String varName = parseString(params,1);
          byte pin = parseString(params,2).toInt();
          long value = Plugin_011_Read(CMD_ANALOG_READ, pin);
          setNvar(varName, value);
          logger->println(value);
          success = true;
        }

        if (cmd.equalsIgnoreCase(F("extEEPROMWrite")))
        {
          byte address = parseString(params,1).toInt();
          byte value = parseString(params,2).toInt();
          Plugin_011_Write(CMD_EEPROM_WRITE, address, value);
          success = true;
        }

        if (cmd.equalsIgnoreCase(F("extEEPROMRead")))
        {
          byte address = parseString(params,1).toInt();
          long value = Plugin_011_Read(CMD_EEPROM_READ, address);
          String log = F("EEPROM:0x");
          log += String(value, HEX);
          logger->println(log);
          success = true;
        }

        if (cmd.equalsIgnoreCase(F("extVCCRead")))
        {
          String varName = parseString(params,1);
          long value = Plugin_011_Read(CMD_VCC_READ, 0);
          setNvar(varName, value);
          String log = F("VCC:");
          log += String(value);
          logger->println(log);
          success = true;
        }

        break;
      }

  }
  return success;
}


//********************************************************************************
// PME read
//********************************************************************************
int Plugin_011_Read(byte cmd, byte Par1)
{
  int value = -1;
  uint8_t address = PLUGIN_011_I2C_ADDRESS;
  Wire.beginTransmission(address);
  Wire.write(cmd);
  Wire.write(Par1);
  Wire.write(0);
  Wire.write(0);
  Wire.endTransmission();
  delay(10);  // remote unit needs some time for conversion...
  Wire.requestFrom(address, (uint8_t)0x4);
  byte buffer[4];
  #if SERIALDEBUG
    Serial.print("I2C received: ");
    Serial.println(Wire.available());
  #endif
  if (Wire.available() == 4)
  {
    for (byte x = 0; x < 4; x++)
      buffer[x] = Wire.read();
    #if SERIALDEBUG
    for (byte x = 0; x < 4; x++)
      Serial.println(buffer[x]);
    #endif
    
    value = buffer[0] + 256 * buffer[1];
  }
  return value;
}


//********************************************************************************
// PME write
//********************************************************************************
void Plugin_011_Write(byte cmd, byte Par1, byte Par2)
{
  uint8_t address = PLUGIN_011_I2C_ADDRESS;
  Wire.beginTransmission(address);
  Wire.write(cmd);
  Wire.write(Par1);
  Wire.write(Par2 & 0xff);
  Wire.write((Par2 >> 8));
  Wire.endTransmission();
}
#endif

