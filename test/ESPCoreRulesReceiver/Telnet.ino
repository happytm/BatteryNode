//********************************************************************************************
// Telnet communications
//********************************************************************************************
void telnet() {
  byte telnetByte = 0;

  if (ser2netServer->hasClient())
  {
    if (ser2netClient) ser2netClient.stop();
    ser2netClient = ser2netServer->available();
  }

  if (ser2netClient.connected())
  {
    logger = &ser2netClient;
    while (ser2netClient.available()) {
      telnetByte = ser2netClient.read();

      if (isprint(telnetByte))
      {
        if (TelnetByteCounter < INPUT_BUFFER_SIZE)
          TelnetBuffer[TelnetByteCounter++] = telnetByte;
      }
      if (telnetByte == '\n') {
        TelnetBuffer[TelnetByteCounter] = 0;
        if(Settings.SerialTelnet)
          if(TelnetBuffer[0] != ':')
            Serial.println(TelnetBuffer);
          else
            ExecuteCommand(TelnetBuffer + 1);
        else
          ExecuteCommand(TelnetBuffer);
        TelnetByteCounter = 0;
        TelnetBuffer[0] = 0;
      }
    }
  }
  else
  {
    logger = &Serial;
    if (connectionState == 1) // there was a client connected before...
    {
      connectionState = 0;
    }
  }
}

