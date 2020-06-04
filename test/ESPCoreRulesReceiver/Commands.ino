//*********************************************************************************************
// Execute commands
//*********************************************************************************************

#define INPUT_COMMAND_SIZE          80
void ExecuteCommand(const char *Line)
{
  #if FEATURE_PLUGINS
     // first check the plugins
    String cmd = Line;
    String params = "";
    int parampos = getParamStartPos(cmd, 2);
    if (parampos != -1) {
      params = cmd.substring(parampos);
      cmd = cmd.substring(0, parampos - 1);
    }
    if (PluginCall(PLUGIN_WRITE, cmd, params)) {
      if(Settings.LogEvents)
        logger->println("OK");
      return;
    }
  #endif
  
  boolean success = false;
  char TmpStr1[80];
  TmpStr1[0] = 0;
  char Command[80];
  Command[0] = 0;
  int Par1 = 0;
  int Par2 = 0;
  int Par3 = 0;
  int Par4 = 0;

  GetArgv(Line, Command, 1);
  if (GetArgv(Line, TmpStr1, 2)) Par1 = str2int(TmpStr1);
  if (GetArgv(Line, TmpStr1, 3)) Par2 = str2int(TmpStr1);
  if (GetArgv(Line, TmpStr1, 4)) Par3 = str2int(TmpStr1);
  if (GetArgv(Line, TmpStr1, 5)) Par4 = str2int(TmpStr1);
    
  //********************************************************************************
  // Config commands
  //********************************************************************************

  if (strcasecmp_P(Command, PSTR("Config")) == 0)
  {
    success = true;
    String strLine = Line;
    String setting = parseString(strLine, 2);
    String strP1 = parseString(strLine, 3);
    String strP2 = parseString(strLine, 4);
    String strP3 = parseString(strLine, 5);
    String strP4 = parseString(strLine, 6);
    String strP5 = parseString(strLine, 7);

    if (setting.equalsIgnoreCase(F("AutoConnect"))){
      Settings.AutoConnect = (Par2 == 1);
    }

    if (setting.equalsIgnoreCase(F("Baudrate"))){
      if (Par2){
        Settings.BaudRate = Par2;
        Settings.UseSerial = 1;
        Serial.begin(Settings.BaudRate);
      }
      else
      {
        Settings.BaudRate = Par2;
        Settings.UseSerial = 0;
        pinMode(1, INPUT);
        pinMode(3, INPUT);
      }
    }

    if (setting.equalsIgnoreCase(F("DST"))){
      Settings.DST = (Par2 == 1);
    }

    if (setting.equalsIgnoreCase(F("Name"))){
      strcpy(Settings.Name, strP1.c_str());
    }

    if (setting.equalsIgnoreCase(F("Group"))){
      strcpy(Settings.Group, strP1.c_str());
    }

  #if FEATURE_I2C
    if (setting.equalsIgnoreCase(F("I2C"))){
      if(Par2 == 0){
        Wire.begin();
      }else{
        Wire.begin(Par2,Par3);
      }
    }
  #endif
  
    if (setting.equalsIgnoreCase(F("Network"))){
      char tmpString[26];
      strP1.toCharArray(tmpString, 26);
      str2ip(tmpString, Settings.IP);
      strP2.toCharArray(tmpString, 26);
      str2ip(tmpString, Settings.Subnet);
      strP3.toCharArray(tmpString, 26);
      str2ip(tmpString, Settings.Gateway);
      strP4.toCharArray(tmpString, 26);
      str2ip(tmpString, Settings.DNS);
      IPAddress ip = Settings.IP;
      IPAddress gw = Settings.Gateway;
      IPAddress subnet = Settings.Subnet;
      IPAddress dns = Settings.DNS;
    }

    if (setting.equalsIgnoreCase(F("NodeListMax"))){
      Settings.NodeListMax = Par2;
    }

    #if FEATURE_PLUGINS
    if (setting.equalsIgnoreCase(F("Plugins"))){
      int parampos = getParamStartPos(strLine, 3);
      if (parampos != -1) {
        params = strLine.substring(parampos);
        logger->println(params);
        byte plugin[PLUGIN_MAX];
        for (byte x = 0; x < PLUGIN_MAX; x++){
          plugin[x]=0;
          Plugin_Enabled[x] = false;
        }
        parseBytes(params.c_str(), ',', plugin, PLUGIN_MAX, 10);
        for (byte p = 0; p < PLUGIN_MAX; p++){
          if(plugin[p] == 0)
            break;
          if(plugin[p] != 0)
            for (byte x = 0; x < PLUGIN_MAX; x++)
              if(Plugin_id[x] == plugin[p])
                Plugin_Enabled[x] = true;
        }
      }
    }
    #endif

    if (setting.equalsIgnoreCase(F("sendARP"))){
      Settings.UseGratuitousARP = (Par2 == 1);
    }
    if (setting.equalsIgnoreCase(F("Port"))){
      Settings.Port = Par2;
    }

    if (setting.equalsIgnoreCase(F("Time"))){
      Settings.UseTime = (Par2 == 1);
    }

    if (setting.equalsIgnoreCase(F("Timezone"))){
      Settings.TimeZone = Par2;
    }

    if (setting.equalsIgnoreCase(F("TimerMax"))){
      Settings.TimerMax = Par2;
    }

    if (setting.equalsIgnoreCase(F("UserVarNumericMax"))){
      Settings.nUserVarMax = Par2;
    }
    if (setting.equalsIgnoreCase(F("UserVarStringMax"))){
      Settings.sUserVarMax = Par2;
    }

    if (setting.equalsIgnoreCase(F("WifiSSID")))
      strcpy(SecuritySettings.WifiSSID, strP1.c_str());

    if (setting.equalsIgnoreCase(F("WifiKey")))
      strcpy(SecuritySettings.WifiKey, strP1.c_str());

    if (setting.equalsIgnoreCase(F("WifiSSID2")))
      strcpy(SecuritySettings.WifiSSID2, strP1.c_str());

    if (setting.equalsIgnoreCase(F("WifiKey2")))
      strcpy(SecuritySettings.WifiKey2, strP1.c_str());

    if (setting.equalsIgnoreCase(F("WifiAPKey")))
      strcpy(SecuritySettings.WifiAPKey, strP1.c_str());

    if (setting.equalsIgnoreCase(F("WifiBSSID")))
      parseBytes(strP1.c_str(), ':', Settings.BSSID, 6, 16);
     
    if (setting.equalsIgnoreCase(F("WifiChannel")))
      Settings.WifiChannel = Par2;

    if (setting.equalsIgnoreCase(F("Rules"))){
      if(strP1.equalsIgnoreCase(F("Clock")))
        Settings.RulesClock = (Par3 == 1);
      if(strP1.equalsIgnoreCase(F("Serial")))
        Settings.RulesSerial = (Par3 == 1);
    }

    if (setting.equalsIgnoreCase(F("LogEvents"))){
      Settings.LogEvents = (Par2 == 1);
    }

  }


  //********************************************************************************
  // Operational commands
  //********************************************************************************

  if (strcasecmp_P(Command, PSTR("DeepSleep")) == 0)
  {
    success = true;
    #if defined(ESP8266)
      ESP.deepSleep(Par1 * 1000000, WAKE_RF_DEFAULT); // Sleep for set delay
    #endif
  }

  if (strcasecmp_P(Command, PSTR("Delay")) == 0)
  {
    success = true;
    delay(Par1);
  }

  #if FEATURE_I2C
  if (strcasecmp_P(Command, PSTR("I2C")) == 0)
  {
    success = true;
    byte error, address;
    for (address = 1; address <= 127; address++) {
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
      if (error == 0) {
        String log = F("I2C  : Found 0x");
        log += String(address, HEX);
        logger->println(log);
      }
    }
  }
  #endif

  #if FEATURE_TIME
    if (strcasecmp_P(Command, PSTR("NTP")) == 0)
    {
      success = true;
      unsigned long  t = updateNtp();
      if(t)
        logger->println(getTimeString(':'));
      else
        logger->println(F("No Reply!"));
    }
  #endif

  if (strcasecmp_P(Command, PSTR("ParseFromJSON")) == 0)
  {
    success = true;
    String strLine = Line;
    String varName = parseString(strLine, 2);
    String jsonName = parseString(strLine, 3);
    int jsonPos = getParamStartPos(strLine, 4);
    String jsonPayload = strLine.substring(jsonPos);
    String strJSONValue = parseJSON(jsonPayload, jsonName);
    if(strJSONValue != ""){
      if(strJSONValue[0] == '"'){
        strJSONValue.replace("\"","");
        setSvar(varName, strJSONValue);
      }else{
        setNvar(varName, strJSONValue.toFloat());
      }
    }
  }
  
  if (strcasecmp_P(Command, PSTR("Reboot")) == 0)
  {
    success = true;
    pinMode(0, INPUT);
    pinMode(2, INPUT);
    pinMode(15, INPUT);
    reboot();
  }

  if (strcasecmp_P(Command, PSTR("Reset")) == 0)
  {
    success = true;
    SPIFFS.end();
    logger->println(F("RESET: formatting..."));
    SPIFFS.format();
    logger->println(F("RESET: formatting done..."));
    if (SPIFFS.begin()){
      initFiles();
    }
    else{
      logger->println(F("RESET: FORMAT SPIFFS FAILED!"));
    }
  }
  
  if (strcasecmp_P(Command, PSTR("SendToUDP")) == 0)
  {
    success = true;
    String strLine = Line;
    String ip = parseString(strLine, 2);
    String port = parseString(strLine, 3);
    int msgpos = getParamStartPos(strLine, 4);
    String message = strLine.substring(msgpos);
    byte ipaddress[4];
    str2ip((char*)ip.c_str(), ipaddress);
    IPAddress UDP_IP(ipaddress[0], ipaddress[1], ipaddress[2], ipaddress[3]);
    WiFiUDP portUDP;
    portUDP.beginPacket(UDP_IP, port.toInt());
    #if defined(ESP8266)
      portUDP.write(message.c_str(), message.length());
    #endif
    #if defined(ESP32)
      portUDP.write((uint8_t*)message.c_str(), message.length());
    #endif
    portUDP.endPacket();
  }

  if (strcasecmp_P(Command, PSTR("Serial")) == 0)
  {
    success = true;
    Serial.begin(115200);
  }

  if (strcasecmp_P(Command, PSTR("SerialFloat")) == 0)
  {
    success = true;
    pinMode(1, INPUT);
    pinMode(3, INPUT);
  }

  if (strcasecmp_P(Command, PSTR("SerialSend")) == 0)
  {
    success = true;
    String tmpString = Line;
    tmpString = tmpString.substring(11);
    Serial.println(tmpString);
  }

  if (strcasecmp_P(Command, PSTR("SerialSendFile")) == 0)
  {
    success = true;
    String tmpString = Line;
    String fileName = parseString(tmpString, 2);
    fs::File f = SPIFFS.open(fileName, "r");
    while (f.available()){
      byte data = f.read();
      Serial.write(data);
      if(data == '\n')
        delay(100);
      delay(10);
    }
  }

  if (strcasecmp_P(Command, PSTR("SerialTelnet")) == 0)
  {
    success = true;
    Settings.SerialTelnet = Par1;
  }

  if (strcasecmp_P(Command, PSTR("Settings")) == 0)
  {
    success = true;
    char str[20];
    logger->println();

    logger->println(F("System Info"));
    IPAddress ip = WiFi.localIP();
    sprintf_P(str, PSTR("%u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);
    logger->print(F("  Name          : ")); logger->println(Settings.Name);
    logger->print(F("  IP Address    : ")); logger->println(str);
    logger->print(F("  Build         : ")); logger->print((int)BUILD); logger->println(BUILD_NOTES);
    String core = getSystemLibraryString();
    logger->print(F("  Core          : ")); logger->println(core);
    logger->print(F("  Wifi SSID     : ")); logger->println(SecuritySettings.WifiSSID);
    logger->print(F("  Wifi Key      : ")); logger->println(SecuritySettings.WifiKey);
    logger->print(F("  Wifi Channel  : ")); logger->println(Settings.WifiChannel);
    sprintf_P(str, PSTR("%02X:%02X:%02X:%02X:%02X:%02X"), Settings.BSSID[0], Settings.BSSID[1], Settings.BSSID[2], Settings.BSSID[3],Settings.BSSID[4],Settings.BSSID[5]);
    logger->print(F("  Wifi BSSID    : ")); logger->println(str);
    logger->print(F("  Wifi Enabled  : ")); logger->println(Settings.Wifi);
    logger->print(F("  Wifi Status   : ")); logger->println(WiFi.status());    
  }

#if FEATURE_RULES
  if (strcasecmp_P(Command, PSTR("TimerSet")) == 0)
  {
    if (GetArgv(Line, TmpStr1, 2)) {
      String varName = TmpStr1;
      success = true;
      if (Par2)
        setTimer(varName, millis() + (1000 * Par2));
      else
        setTimer(varName, 0L);
    }
  }

  if (strcasecmp_P(Command, PSTR("Event")) == 0)
  {
    success = true;
    String event = Line;
    event = event.substring(6);
    rulesProcessing(FILE_RULES, event);
  }

  if (strcasecmp_P(Command, PSTR("ValueSet")) == 0)
  {
    success = true;
    if (GetArgv(Line, TmpStr1, 3))
    {
      float result = 0;
      Calculate(TmpStr1, &result);
      if (GetArgv(Line, TmpStr1, 2)) {
        String varName = TmpStr1;
        if (GetArgv(Line, TmpStr1, 4))
          setNvar(varName, result, Par3);
        else
          setNvar(varName, result);
      }
    }
  }

  if (strcasecmp_P(Command, PSTR("StringSet")) == 0)
  {
    success = true;
    String sline = Line;
    int pos = getParamStartPos(sline, 3);
    if (pos != -1) {
      if (GetArgv(Line, TmpStr1, 2)) {
        String varName = TmpStr1;
        setSvar(varName, sline.substring(pos));
      }
    }
  }

  if (strcasecmp_P(Command, PSTR("StringLength")) == 0)
  {
    success = true;
    String strLine = Line;
    String varName = parseString(strLine, 2);
    String varNameSource = parseString(strLine, 3);
    String tmp = getSvar(varNameSource);
    setNvar(varName, tmp.length(),0);
  }

  if (strcasecmp_P(Command, PSTR("StringReplace")) == 0)
  {
    success = true;
    String strLine = Line;
    String varName = parseString(strLine, 2);
    String replace = parseString(strLine, 3);
    String replaceWith = parseString(strLine, 4);
    String tmp = getSvar(varName);
    tmp.replace(replace,replaceWith);
    setSvar(varName, tmp);
  }
  
  if (strcasecmp_P(Command, PSTR("StringSubstring")) == 0)
  {
    success = true;
    String strLine = Line;
    String varName = parseString(strLine, 2);
    String varNameSource = parseString(strLine, 3);
    String strStart = parseString(strLine, 4);
    String strEnd = parseString(strLine, 5);
    int startPos = 0;
    int endPos = 0;

    String tmp = getSvar(varNameSource);
    if(strStart[0] != '"'){
      startPos = strStart.toInt();
    }else{
      strStart.replace("\"","");
      startPos = tmp.indexOf(strStart);
    }

    if(strEnd == ""){
      endPos = tmp.length();
    }else{
      if(strEnd[0] != '"'){
        endPos = strEnd.toInt();
      }else{
        strEnd.replace("\"","");
        endPos = tmp.indexOf(strEnd);
      }
    }
    
    tmp = tmp.substring(startPos,endPos);
    setSvar(varName, tmp);
  }

  if (strcasecmp_P(Command, PSTR("Syslog")) == 0)
  {
    success = true;
    String log = Line;
    log = log.substring(7);
    syslog(log);
  }

  #if FEATURE_ADC_VCC
    if (strcasecmp_P(Command, PSTR("VCCRead")) == 0)
    {
      success = true;
      String strLine = Line;
      String varName = parseString(strLine, 2);
      setNvar(varName, ESP.getVcc() / 1000.0);
    }
  #endif

  if (strcasecmp_P(Command, PSTR("webPrint")) == 0)
  {
    success = true;
    String wprint = Line;
    if (wprint.length() == 8)
      printWebString = "";
    else
      printWebString += wprint.substring(9);
  }

  if (strcasecmp_P(Command, PSTR("webButton")) == 0)
  {
    success = true;
    String params = Line;
    printWebString += F("<a class=\"");
    printWebString += parseString(params, 2,';');
    printWebString += F("\" href=\"");
    printWebString += parseString(params, 3,';');
    printWebString += F("\">");
    printWebString += parseString(params, 4,';');
    printWebString += F("</a>");
  }
  
#endif

  if (strcasecmp_P(Command, PSTR("WifiConnect")) == 0)
  {
    success = true;
    String strLine = Line;
    String ssid = parseString(strLine, 2);
    String key = parseString(strLine, 3);
    strcpy(SecuritySettings.WifiSSID, ssid.c_str());
    strcpy(SecuritySettings.WifiKey, key.c_str());
    WiFi.begin(SecuritySettings.WifiSSID, SecuritySettings.WifiKey);
//    WifiInit();
  }

  if (strcasecmp_P(Command, PSTR("WifiInit")) == 0)
  {
    success = true;
    WifiInit();
  }

  if(Settings.LogEvents){
    if (success)
      logger->println("OK");
    else
      logger->println("?");
  }
}


//*********************************************************************************************
// Find positional parameter in a char string
//*********************************************************************************************

boolean GetArgv(const char *string, char *argv, int argc)
{
  int string_pos = 0, argv_pos = 0, argc_pos = 0;
  char c, d;

  while (string_pos < strlen(string))
  {
    c = string[string_pos];
    d = string[string_pos + 1];

    if       (c == ' ' && d == ' ') {}
    else if  (c == ' ' && d == ',') {}
    else if  (c == ',' && d == ' ') {}
    else if  (c == ' ' && d >= 33 && d <= 126) {}
    else if  (c == ',' && d >= 33 && d <= 126) {}
    else
    {
      argv[argv_pos++] = c;
      argv[argv_pos] = 0;

      if (d == ' ' || d == ',' || d == 0)
      {
        argv[argv_pos] = 0;
        argc_pos++;

        if (argc_pos == argc)
        {
          return true;
        }

        argv[0] = 0;
        argv_pos = 0;
        string_pos++;
      }
    }
    string_pos++;
  }
  return false;
}

