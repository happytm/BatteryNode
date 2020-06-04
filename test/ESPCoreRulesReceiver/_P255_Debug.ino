//#######################################################################################################
//#################################### Plugin 255: Debug ################################################
//#######################################################################################################

/*
 * Commands:
 * analogRead <variable>,<pin>                   Read analog value into variable
 * analogDebug <pin>                             Show analog value every second on telnet session
*/

#ifdef USES_P255
#define P255_BUILD            7
#define PLUGIN_255
#define PLUGIN_ID_255         255

boolean Plugin_255(byte function, String& cmd, String& params)
{
  boolean success = false;
  static int8_t  PinDebug = -1;

  switch (function)
  {
    case PLUGIN_INFO:
      {
        printWebTools += F("<TR><TD>P255 - Debug<TD>");
        printWebTools += P255_BUILD;
        break;
      }
          
    case PLUGIN_WRITE:
      {
        if (!cmd.startsWith("db-"))
          return false;

        if (cmd.equalsIgnoreCase(F("db-Dir")))
        {
          success = true;
          logger->println();
          #if defined(ESP8266)
            Dir dir = SPIFFS.openDir("");
            while (dir.next())
            {
              String log = dir.fileName();
              log += ":";
              File f = dir.openFile("r");
              log += f.size();
              logger->println(log);
            }
          #endif
          #if defined(ESP32)
            File root = SPIFFS.open("/");
            File file = root.openNextFile();
            while (file)
            {
              if (!file.isDirectory()) {
                String log = file.name();
                log += ":";
                log += file.size();
                logger->println(log);
              }
              file = root.openNextFile();
            }
          #endif
        }

        if (cmd.equalsIgnoreCase(F("db-File")))
        {
          success = true;
          String fileName = parseString(params, 1);
          logger->println();
          fs::File f = SPIFFS.open(fileName, "r");
          if (f)
          {
            while (f.available())
              logger->write(f.read());
            f.close();
          }
        }
        
        if (cmd.equalsIgnoreCase(F("db-Vars")))
        {
          success = true;
          if(mallocOK){
            logger->print(F("Numeric (max="));
            logger->print(Settings.nUserVarMax);
            logger->println(F("):"));
            for (byte x = 0; x < Settings.nUserVarMax; x++) {
              if (*nUserVar[x].Name != "") {
                logger->print(*nUserVar[x].Name);
                logger->print(" : ");
                logger->println(nUserVar[x].Value);
              }
            }
            logger->print(F("\nString (max="));
            logger->print(Settings.sUserVarMax);
            logger->println(F("):"));
            for (byte x = 0; x < Settings.sUserVarMax; x++) {
              if (*sUserVar[x].Name != "") {
                logger->print(*sUserVar[x].Name);
                logger->print(" : ");
                logger->println(*sUserVar[x].Value);
              }
            }
            logger->print(F("\nTimers (max="));
            logger->print(Settings.TimerMax);
            logger->println(F("):"));
            for (byte x = 0; x < Settings.TimerMax; x++) {
              if (*RulesTimer[x].Name != "") {
                logger->print(*RulesTimer[x].Name);
                logger->print(" : ");
                logger->println(RulesTimer[x].Value);
              }
            }
          }
        }
          
        if (cmd.equalsIgnoreCase(F("db-Registers")))
        {
          success = true;
          for(byte x=0; x < PLUGIN_FASTCALL_MAX; x++){
            if(corePluginCall_ptr[x] != 0){
              logger->print(x);
              logger->print(":");
              logger->println((int)corePluginCall_ptr[x]);
            }
          }
        }
        if (cmd.equalsIgnoreCase(F("db-Nodes")))
        {
          for (byte x = 0; x < Settings.NodeListMax; x++)
            {
              String node = "";
              node += x;
              node += " ";
              node += *Nodes[x].group;
              node += " - ";
              node += *Nodes[x].nodeName;
              node += " -  ";
              node += Nodes[x].IP[0];
              node += " - ";
              node += Nodes[x].age;
              logger->println(node);
            }
        }
        if (cmd.equalsIgnoreCase(F("db-WifiOff")))
        {
          success = true;
          WiFi.mode( WIFI_OFF );
          #if defined(ESP8266)
            WiFi.forceSleepBegin();
          #endif
          delay(1);
        }
        if (cmd.equalsIgnoreCase(F("db-WifiClear")))
        {
          success = true;
          #if defined(ESP8266)
            ESP.eraseConfig();
            ESP.reset();
          #endif
        }
        if (cmd.equalsIgnoreCase(F("db-WifiConnectPersistent")))
        {
          success = true;
          String ssid = parseString(params, 1);
          String key = parseString(params, 2);
          WiFi.persistent(true);
          WiFi.begin(ssid.c_str(),key.c_str());
        }
        if (cmd.equalsIgnoreCase(F("db-WifiDisConnect")))
        {
          success = true;
          WiFi.disconnect();
        }
        if (cmd.equalsIgnoreCase(F("db-WifiStatus")))
        {
          success = true;
          Serial.println(WiFi.status());
        }
        if (cmd.equalsIgnoreCase(F("db-WifiAPMode")))
        {
          success = true;
          byte state = parseString(params, 1).toInt();
          byte force = parseString(params, 2).toInt();
          Settings.ForceAPMode = (force == 1);
          char ap_ssid[40];
          ap_ssid[0] = 0;
          strcpy(ap_ssid, "ESP");
          //sprintf_P(ap_ssid, PSTR("%s%s"), ap_ssid, Settings.Name);
          WiFi.softAP(ap_ssid, SecuritySettings.WifiAPKey,6,false);
          WifiAPMode(state == 1);          
        }
        
        #if SERIALDEBUG
        if (cmd.equalsIgnoreCase(F("db-DebugLevel")))
        {
          success = true;
          int Par1 = parseString(params, 1).toInt();
          //debugLevel = Par1;
        }
        #endif

        break;
      }
  }
  return success;
}
#endif
