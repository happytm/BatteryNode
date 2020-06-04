//********************************************************************************************
// Init Serial port
//********************************************************************************************
void serialInit(){
  
  // If boot.txt does not contain a SSID, make sure we have serial on to get started
  if(SecuritySettings.WifiSSID[0] == 0){
    Serial.begin(115200);
    Serial.println(F("No Wifi Config!"));
  }

  // If we have a valid boot config and serial is explicitly disabled, turn pins to input
  if (bootConfig && !Settings.UseSerial) {
    pinMode(1, INPUT);
    pinMode(3, INPUT);
  }
  
  #if SERIALDEBUG
    Serial.println("");
    Serial.println("Serial init");
  #endif  
}
/********************************************************************************************\
 Get data from Serial Interface
\*********************************************************************************************/
void serial()
{
  while (Serial.available())
  {
    delay(0);
    SerialInByte = Serial.read();
    if (SerialInByte == 255) // binary data...
    {
      Serial.flush();
      return;
    }

    if (isprint(SerialInByte))
    {
      if (SerialInByteCounter < INPUT_BUFFER_SIZE) // add char to string if it still fits
        InputBuffer_Serial[SerialInByteCounter++] = SerialInByte;
    }

    if (SerialInByte == '\n')
    {
      InputBuffer_Serial[SerialInByteCounter] = 0; // serial data completed
      if(Settings.SerialTelnet && ser2netClient.connected())
      {
        ser2netClient.write((const uint8_t*)InputBuffer_Serial, strlen(InputBuffer_Serial));
        ser2netClient.write(10);
        ser2netClient.write(13);
        ser2netClient.flush();
      }

      String action = InputBuffer_Serial;

      #if FEATURE_RULES
        if (Settings.RulesSerial){
          String event = "";
          if(action.substring(0,3) == F("20;")){
            action = action.substring(6); // RFLink, strip "20;xx;" from incoming message
            event = "RFLink#" + action;
          }
          else
            event = "Serial#" + action;
          rulesProcessing(FILE_RULES, event);
        }
      #endif

      #if FEATURE_PLUGINS
        PluginCall(PLUGIN_SERIAL_IN, action, dummyString);
      #endif
      ExecuteCommand(InputBuffer_Serial);
      SerialInByteCounter = 0;
      InputBuffer_Serial[0] = 0; // serial data processed, clear buffer
    }
  }
}

