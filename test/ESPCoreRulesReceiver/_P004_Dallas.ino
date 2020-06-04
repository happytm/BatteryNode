//#######################################################################################################
//#################################### Plugin 004: TempSensor Dallas DS18B20  ###########################
//#######################################################################################################

/*
 * Commands:
 * Dallas <variable>,<pin>                   Read temperature value into variable
*/

#ifdef USES_P004
#define P004_BUILD            6
#define PLUGIN_004
#define PLUGIN_ID_004         4

uint8_t Plugin_004_DallasPin;

boolean Plugin_004(byte function, String& cmd, String& params)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_INFO:
      {
        printWebTools += F("<TR><TD>P004 - Dallas<TD>");
        printWebTools += P004_BUILD;
        break;
      }
      
    case PLUGIN_WRITE:
      {
        if (cmd.equalsIgnoreCase(F("Dallas")))
        {
          success = true;
          String varName = parseString(params,1);
          byte pin = parseString(params,2).toInt();
          pinMode(pin, OUTPUT);
          dallas(varName, pin);
        }
        break;
      }

  }
  return success;
}

uint8_t DallasPin;
uint8_t DS_read(void)
{
  uint8_t bitMask;
  uint8_t r = 0;
  uint8_t BitRead;

  for (bitMask = 0x01; bitMask; bitMask <<= 1)
  {
    pinMode(DallasPin, OUTPUT);
    digitalWrite(DallasPin, LOW);
    delayMicroseconds(3);

    pinMode(DallasPin, INPUT); // let pin float, pull up will raise
    delayMicroseconds(10);
    BitRead = digitalRead(DallasPin);
    delayMicroseconds(53);

    if (BitRead)
      r |= bitMask;
  }
  return r;
}

void DS_write(uint8_t ByteToWrite)
{
  uint8_t bitMask;

  pinMode(DallasPin, OUTPUT);
  for (bitMask = 0x01; bitMask; bitMask <<= 1)
  { // BitWrite
    digitalWrite(DallasPin, LOW);
    if (((bitMask & ByteToWrite) ? 1 : 0) & 1)
    {
      delayMicroseconds(5);// Dallas spec.= 5..15 uSec.
      digitalWrite(DallasPin, HIGH);
      delayMicroseconds(55);// Dallas spec.= 60uSec.
    }
    else
    {
      delayMicroseconds(55);// Dallas spec.= 60uSec.
      digitalWrite(DallasPin, HIGH);
      delayMicroseconds(5);// Dallas spec.= 5..15 uSec.
    }
  }
}

uint8_t DS_reset()
{
  uint8_t r;
  uint8_t retries = 125;

  pinMode(DallasPin, INPUT);
  //do  {  // wait until the wire is high... just in case
  //  if (--retries == 0) return 0;
  //  delayMicroseconds(2);
  //} while ( !digitalRead(DallasPin));

  pinMode(DallasPin, OUTPUT); digitalWrite(DallasPin, LOW);
  delayMicroseconds(492); // Dallas spec. = Min. 480uSec. Arduino 500uSec.
  pinMode(DallasPin, INPUT); //Float
  delayMicroseconds(40);
  r = !digitalRead(DallasPin);
  delayMicroseconds(420);
  return r;
}

boolean dallas(String varName, byte Pin)
{
  static byte Call_Status = 0x00; // Each bit represents one relative port. 0=not called before, 1=already called before.
  boolean success = false;
  int DSTemp;                           // Temperature in 16-bit Dallas format.
  byte ScratchPad[12];                  // Scratchpad buffer Dallas sensor.
  byte RelativePort = Pin - 1;

  DallasPin = Pin;

//  noInterrupts();
  //while (!(bitRead(Call_Status, RelativePort)))
  //{
  //  // if this is the very first call to the sensor on this port, reset it to wake it up
  //  boolean present = DS_reset();
  //  bitSet(Call_Status, RelativePort);
  //}
  boolean present = DS_reset(); DS_write(0xCC /* rom skip */); DS_write(0x44 /* start conversion */);
//  interrupts();

  if (present)
  {
    delay(800);     // neccesary delay

//    noInterrupts();
    DS_reset(); DS_write(0xCC /* rom skip */); DS_write(0xBE /* Read Scratchpad */);

    digitalWrite(DallasPin, LOW);
    pinMode(DallasPin, INPUT);

    for (byte i = 0; i < 9; i++)            // copy 8 bytes
      ScratchPad[i] = DS_read();
//    interrupts();

    DSTemp = (ScratchPad[1] << 8) + ScratchPad[0];
    setNvar(varName, (float(DSTemp) * 0.0625));
    success = true;
  }
  return success;
}
#endif

