//#if !PROBEREQUEST

//********************************************************************************
// Init Wifi station
//********************************************************************************
void WifiInit() {
  String event = "";
  
  #if SERIALDEBUG
    Serial.println("Start WifiInit");
  #endif

  #if defined(ESP8266)
    WiFi.forceSleepWake();
    delay(1);
  #endif
  WiFi.mode(WIFI_STA);

  if(WifiConnect(2)){
  //}else
  //{
    WifiAPMode(true);    
  }
  #if SERIALDEBUG
    Serial.println("End Wificonnect");
  #endif

  #if SERIALDEBUG
    Serial.println("Start WebServer");
  #endif
  WebServerInit();

  // Add myself to the node list (if UnitMax is not set to zero)
  if (Settings.NodeListMax){
    IPAddress IP = WiFi.localIP();
    for (byte x = 0; x < 4; x++)
      Nodes[0].IP[x] = IP[x];
    Nodes[0].age = 0;
    *Nodes[0].nodeName = Settings.Name;
    *Nodes[0].group = Settings.Group;
  }
  
  #if SERIALDEBUG
    Serial.println("Start TelnetServer");
  #endif
  ser2netServer = new WiFiServer(23);
  ser2netServer->begin();

  #if SERIALDEBUG
    Serial.println("Start UDP Server");
  #endif

  #if FEATURE_TIME
    if(Settings.UseTime)
      if(WifiConnected())
        initTime();
  #endif

  #ifdef FEATURE_ARDUINO_OTA
    #if SERIALDEBUG
      Serial.println("Start OTA Server");
    #endif
    ArduinoOTAInit();
  #endif

  // When all things are setup, we send a Wifi#Connected event when Wifi STA is connected
  #if FEATURE_RULES
  if(Settings.Wifi){
    if(WiFi.status() == WL_CONNECTED){
      #if SERIALDEBUG
        Serial.print("Wifi Connected");        
        
      #endif
      String event = F("WiFi#Connected");
      rulesProcessing(FILE_RULES, event);
    }
  }
  #endif
}


//********************************************************************************
// Check Wifi connection status
//********************************************************************************
boolean WifiConnected(){
 if(Settings.Wifi && WiFi.status() == WL_CONNECTED)
   return true;
 return false;
}

//********************************************************************************
// Send ARP update
//********************************************************************************
void sendGratuitousARP() {
#ifndef ESP32
  netif *n = netif_list;
  while (n) {
    etharp_gratuitous(n);
    n = n->next;
  }
#endif
}


//********************************************************************************
// Set Wifi AP Mode config
//********************************************************************************
void WifiAPconfig()
{
  char ap_ssid[40];
  ap_ssid[0] = 0;
  strcpy(ap_ssid, "ESP_");
  sprintf_P(ap_ssid, PSTR("%s%s"), ap_ssid, Settings.Name);
  if(SecuritySettings.WifiAPKey[0] == 0)
    strcpy(SecuritySettings.WifiAPKey, FACTORY_APKEY);
  #if SERIALDEBUG
    Serial.print("Wifi AP config with key:");
    Serial.println(SecuritySettings.WifiAPKey);
  #endif
  WiFi.softAP(ap_ssid, SecuritySettings.WifiAPKey,wifiChannel,false);
}


//********************************************************************************
// Set Wifi AP Mode
//********************************************************************************
void WifiAPMode(boolean state)
{
  if(Settings.ForceAPMode){
    if(!AP_Mode){
      #if SERIALDEBUG
        Serial.println("Wifi AP-STA forced");
      #endif
      AP_Mode = true;
      WifiAPconfig();
      WiFi.mode(WIFI_AP_STA);
    }
    return;
  }
  
  if (state)
  {
    if(!AP_Mode){
      #if SERIALDEBUG
        Serial.println("Wifi AP-STA");
      #endif
      AP_Mode = true;
      WifiAPconfig();
      WiFi.mode(WIFI_AP_STA);
    }
  }
  else
  {
    if(AP_Mode){
      #if SERIALDEBUG
        Serial.println("Wifi STA");
      #endif
      AP_Mode = false;
      WiFi.mode(WIFI_STA);
    }
  }
}


//********************************************************************************
// Connect to Wifi AP
//********************************************************************************
boolean WifiConnect(byte connectAttempts)
{
  String log = "";
  #if SERIALDEBUG
    Serial.print("Wifi Status:");
    Serial.println(WiFi.status());
  #endif
      
  if (WiFi.status() == WL_CONNECTED){
    return true;
  }else
  {
    if (Settings.IP[0] != 0 && Settings.IP[0] != 255)
    {
      IPAddress ip = Settings.IP;
      IPAddress gw = Settings.Gateway;
      IPAddress subnet = Settings.Subnet;
      IPAddress dns = Settings.DNS;
      WiFi.config(ip, gw, subnet, dns);
    }

    if ((SecuritySettings.WifiSSID[0] != 0)  && (strcasecmp(SecuritySettings.WifiSSID, "ssid") != 0))
    {
      #if SERIALDEBUG
          Serial.print("Wificonnect:");
          Serial.print(millis());
          Serial.println(" mS:");
      #endif

      for (byte tryConnect = 1; tryConnect <= connectAttempts; tryConnect++)
      {
        #if SERIALDEBUG
          Serial.print("WifiStatus:");
          Serial.println(WiFi.status());
          Serial.print("Wificonnect:");
          Serial.println(tryConnect);
        #endif

        if (tryConnect == 1){
          if(Settings.WifiChannel == 0) 
            WiFi.begin(SecuritySettings.WifiSSID, SecuritySettings.WifiKey);
          else
            WiFi.begin(SecuritySettings.WifiSSID, SecuritySettings.WifiKey, Settings.WifiChannel, Settings.BSSID);
        }
        else
          WiFi.begin();

        for (int x = 0; x < 1000; x++)
        {
          if (WiFi.status() != WL_CONNECTED)
          {
            delay(10);
          }
          else
            break;
        }
        if (WiFi.status() == WL_CONNECTED)
        {
          #if SERIALDEBUG
            Serial.print("Wificonnected:");
            Serial.print(millis());
            Serial.println(" mS:");
          #endif
          return true;
        }
        else
        {
          #if defined(ESP8266)
            ETS_UART_INTR_DISABLE();
            wifi_station_disconnect();
            ETS_UART_INTR_ENABLE();
          #endif
          delay(1000);
        }
      }
    }
  }
  return false;
}


//********************************************************************************
// Check if we are still connected to a Wifi AP
//********************************************************************************
void WifiCheck()
{
  String log = "";

  if (WiFi.status() != WL_CONNECTED)
  {
    NC_Count++;
    if (NC_Count > 2)
    {
      WifiConnect(2);
      C_Count=0;
      if (WiFi.status() != WL_CONNECTED)
        WifiAPMode(true);
      NC_Count = 0;
    }
  }
  else
  {
    C_Count++;
    NC_Count = 0;
    if (C_Count > 2) // close AP after timeout if a Wifi connection is established...
    {
      WifiAPMode(true);
    }
  }
}

//#endif   //!PROBEREQUEST
