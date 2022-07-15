#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <HTTPClient.h>                    
#include <ESP32httpUpdate.h>                
#include <EEPROM.h>
#include "FS.h"
#include "SPIFFS.h"
#include <SPIFFSEditor.h>
#include <TinyMqtt.h>         // Thanks to https://github.com/hsaturn/TinyMqtt
#include "time.h"
#include "motionDetector.h"                 // Thanks to https://github.com/paoloinverse/motionDetector_esp
#include <ESP32Ping.h>                      // Thanks to https://github.com/marian-craciunescu/ESP32Ping

#define FIRSTTIME     false                 // Define true if setting up Gateway for first time.
#define PINGABLE      true                 // If true use ESPPing library to detect presence of known devices.

#if PINGABLE
#include <ESP32Ping.h>                      // Thanks to https://github.com/marian-craciunescu/ESP32Ping
#endif

const char* room = "Gateway";               // Needed for devices locator.
int rssiThreshold = -50;                    // Adjust according to signal strength by trial & error.
int motionThreshold = 100;                  // Adjust the sensitivity of motion sensor.Higher the number means less sensetive motion sensor is.

int WiFiChannel = 7;                        // This must be same for all devices on network.
const char* apSSID = "ESP";                 // SoftAP SSID.
const char* apPassword = "";                // SoftAP password.
const int hidden = 0;                       // If hidden is 1 probe request event handling does not work ?

int enableCSVgraphOutput = 1;               // 0 disable, 1 enable.If enabled, you may use Tools-> Serial Plotter to plot the variance output for each transmitter.
int scanInterval = 600;                     // Interval in minutes to send data.
int pingInterval;                           // interval in minutes to ping known devices.
int motionLevel = -1;
float receivedRSSI = 0;

struct tm timeinfo;
#define MY_TZ "EST5EDT,M3.2.0,M11.1.0"      //(New York) https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

std::string sentTopic = "data";
std::string receivedTopic = "command";

const char* http_username = "admin";        // Web file editor interface Login.
const char* http_password = "admin";        // Web file editor interface password.

String dataFile = "/data.json";             // File to store sensor data.
String binFile = "http://192.168.4.1/gateway.bin";

#if PINGABLE
IPAddress deviceIP(192, 168, 0, 2);         // Fixed IP address assigned to family member's devices to be checked for their presence at home.
//IPAddress deviceIP = WiFi.localIP();
int device1IP = 2, device2IP = 3, device3IP = 4, device4IP = 5;
int device1Status, device2Status, device3Status, device4Status;
#endif   //#if PINGABLE

//==================User configuration not required below this line ================================================

#define MYFS SPIFFS
#define FORMAT_SPIFFS_IF_FAILED true

MqttBroker broker(1883);
MqttClient myClient(&broker);

AsyncWebServer webserver(80);
AsyncWebSocket ws("/ws");

char str [256], s [70];
String ssid,password,graphData, graphDataToWS, Month, Date, Year, Hour, Minute, Second;
int device, rssi, sleepTime, sensorValues[4], sensorTypes[4], commandSent;
float voltage;
uint8_t receivedCommand[6],showConfig[256];
const char* ntpServer = "pool.ntp.org";
unsigned long currentMillis, lastMillis;
unsigned long epoch; 
String Epoch = String(epoch);String Loc = String(device);String V = String(voltage, 2);String S = String(rssi);String T = String(sensorValues[0]);String H = String(sensorValues[1]);String P = String(sensorValues[2]);String L = String(sensorValues[3]); 

//int distance[4] = {10,30,40,20};  

uint8_t Command[] =                                            // Maximum limit is 1500 bytes?
{
  0x80, 0x00,                                                 //  0- 1: First byte here must be 80 for Type = Beacon.
  0x00, 0x00,                                                 //  2- 3: Can it be used to send more data to remote device?
  0x06, 0x44, 0x44, 0x44, 0x44, 0x44,                         //  4- 9: First byte here must be device ID (It will be filled automaticallywith Device ID of remote sensor sending sensor data).Second byte is Command type.Rest will be filled with command values received from web interface.
  0x06, 0x55, 0x55, 0x55, 0x55, 0x55,                         // 10-15: Month, Date, Hour, Minute & second sent to remote device for time synch.
  0x66, 0x66, 0x66, 0x66, 0x66, 0x66,                         // 16-21: Can be used to send more data to remote device.
  0x00, 0x00,                                                 // 22-23: Can it be used to send more data to remote device?
}; 

unsigned long getTime() {time_t now;if (!getLocalTime(&timeinfo)) {Serial.println("Failed to obtain time");return(0);}time(&now);return now;}
//int sort (const void * arg1, const void * arg2){int * a = (int *) arg1;int * b = (int *) arg2;if (*a > *b)return -1;if (*a < *b)return 1;return 0;}  // Make sort of array of integers to rearrange values of array in decending or ascending order.

void setup() {
  Serial.begin(115200);
  delay(500);
  
  SPIFFS.begin();
  EEPROM.begin(512);
  
#if FIRSTTIME  
  // Setup device IDs and wifi Channel for remote devices in EEPROM permanantly.
  for (int i = 6; i < 256; i = i+10) {EEPROM.writeByte(i, i);EEPROM.writeByte(i+1, 107);EEPROM.writeByte(i+2, apChannel);}}
  EEPROM.writeByte(0, WiFiChannel);EEPROM.commit();
  File f = LITTLEFS.open(dataFile, "w");
  f.print("[13,\"Epoch\",\"Location\",\"Voltage\",\"SSID\",\"Motion\",\"T1\",\"S1\",\"T2\"\"S2\",\"T3\",\"S3\",\"T4\",\"S4\"]"); // See http://davidgiard.com/2018/11/02/EmbeddingQuotesWithinACString.aspx
  f.close();
  Serial.println("Wrote first line to file: [13,\"Epoch\",\"Location\",\"Voltage\",\"SSID\",\"Motion\",\"T1\",\"S1\",\"T2\"\"S2\",\"T3\",\"S3\",\"T4\",\"S4\"]");Serial.println();
#endif  
  
  EEPROM.readBytes(0, showConfig,256);for(int i=0;i<256;i++){Serial.printf("%d ", showConfig[i]);}Serial.println();
  
  startWiFi();
  startAsyncwebserver();
    
  configTime(0, 0, ntpServer); setenv("TZ", MY_TZ, 1); tzset(); // Set environment variable with your time zone
  epoch = getTime(); Serial.print("Epoch Time: "); Serial.println(epoch); delay(500); Epoch = String(epoch);

  broker.begin();
  myClient.setCallback(receivedMessage);
  myClient.subscribe(receivedTopic);
  myClient.subscribe(sentTopic);
 
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);
  esp_wifi_set_channel(WiFiChannel, WIFI_SECOND_CHAN_NONE);
    
}

void loop() 
{ 
   pingInterval++; 
   motionDetector_set_minimum_RSSI(-80);                // Minimum RSSI value to be considered reliable. Default value is 80 * -1 = -80.
   motionLevel = motionDetector_esp();                  // if the connection fails, the radar will automatically try to switch to different operating modes by using ESP32 specific calls.

   if (pingInterval > (scanInterval * 500))             // 500 for 5 minutes if scanInterval is 600.
  {
   
  #if PINGABLE
    Serial.println("Checking to see who is at home.... ");

    int pingTime;

    deviceIP[3] = device1IP; Serial.println("Pinging IP address 2... "); if (Ping.ping(deviceIP, 5)) {pingTime = Ping.averageTime(); device1Status = (pingTime);} else {device1Status = 0;} Serial.print("Ping time in milliseconds: ");Serial.println(pingTime);
    deviceIP[3] = device2IP; Serial.println("Pinging IP address 3... "); if (Ping.ping(deviceIP, 5)) {pingTime = Ping.averageTime(); device2Status = (pingTime);} else {device2Status = 0;} Serial.print("Ping time in milliseconds: ");Serial.println(pingTime);
    deviceIP[3] = device3IP; Serial.println("Pinging IP address 4... "); if (Ping.ping(deviceIP, 5)) {pingTime = Ping.averageTime(); device3Status = (pingTime);} else {device3Status = 0;} Serial.print("Ping time in milliseconds: ");Serial.println(pingTime);
    deviceIP[3] = device4IP; Serial.println("Pinging IP address 5... "); if (Ping.ping(deviceIP, 5)) {pingTime = Ping.averageTime(); device4Status = (pingTime);} else {device4Status = 0;} Serial.print("Ping time in milliseconds: ");Serial.println(pingTime);
    pingInterval = 0;   // Data sent. Reset the ping interval counter.
  #endif   // #if PINGABLE 
}
   
 if (motionLevel > motionThreshold)        // Adjust the sensitivity of motion sensor. Higher the number means less sensetive motion sensor is.
  {Serial.print("Motion detected & motion level is: "); Serial.println(motionLevel);}  
   if (WiFi.waitForConnectResult() != WL_CONNECTED) {ssid = EEPROM.readString(270); password = EEPROM.readString(301);Serial.println("Wifi connection failed");WiFi.disconnect(false);delay(1000);WiFi.begin(ssid.c_str(), password.c_str());}
   
   if (commandSent == 1)  // Set frequency for printing serial statements.
   {  
      //qsort(distance, 4, sizeof (int), sort); for (int i = 0; i < 4; i++){Serial.print(distance[i]);if (i<3){Serial.print(",");}}Serial.println(); // Make sort of array of integers to rearrange values of array in decending or ascending order.
      Serial.println();Serial.print("Data received from remote sensor -  ");Serial.print(device);Serial.println(" is below: ");for (int i = 0; i < 23; i++) {Serial.printf("%02X", sensorValues[i]);}Serial.println();
      Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
      Serial.print("Epoch Time: ");Serial.println(epoch); Serial.print("Time Sent to remote device - ");Serial.print(device);Serial.print(" = ");Serial.print(Month + 1);Serial.print("/");Serial.print(Date);Serial.print("  ");Serial.print(Hour);Serial.print(":");Serial.print(Minute);Serial.print(":");Serial.println(Second);
      Serial.print("Connect at IP: ");Serial.print(WiFi.localIP());Serial.print(" or 192.168.4.1");Serial.println(" to monitor and control whole network");
      Serial.println("Following ## Sensor Values ## receiced from remote device  & published via MQTT: ");Serial.println(str);      
      for(int i=0;i<10;i++){Serial.print(showConfig[i+device]);} Serial.println();
      Serial.println("Received packet & sending command...... ");
      Serial.print("Unknown device's MAC received from remote device - ");Serial.print(device);Serial.print(" = "); for (int i = 10; i < 15; i++) {Serial.print(Command[i], HEX);if (i < 14)Serial.print(":");} Serial.print(" & Guest's signal strength = "); Serial.println(Command[17]);
      Serial.print("Status of known devices near device - ");Serial.print(device);Serial.print(" = "); for (int i = 18; i < 21; i++) {Serial.print(Command[i], HEX);if (i < 20)Serial.print(",");}Serial.println();
      Serial.println("Following ## Sensor Values ## receiced from remote device  & published via MQTT: ");Serial.println(str);
      Serial.print("Sent Command to remote device: "); for (int i = 4; i < 9; i++) {Serial.print(Command[i]);} Serial.println();Serial.println();
      Serial.print("Graph data size: ");Serial.println(graphDataToWS.length());
      Serial.println("Graph Data: ");Serial.println(graphDataToWS);
      
      notifyClients(graphDataToWS);  // Send data to web interface.
      //notifyClients(str);          // Send data to web interface.
      ws.cleanupClients();
      
      File f = SPIFFS.open(dataFile, "r+"); // See https://github.com/lorol/LITTLEFS/issues/33
      f.seek((f.size()-1), SeekSet);f.print(graphData);f.close();Serial.println("Sensor data saved to flash.");
      commandSent = 0;
      }
      delay(scanInterval);
}

void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) 
{ 
  // Avoid any Serial.print statements in this callback function to send command to remote devices as soon as possible.
  
  wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t*)buf;
  
  if (p->payload[0] == 0x40 && p->rx_ctrl.rssi > -70 && p->payload[10] != 0xCC) {        // Limit devices with type proberequest with SSID of more than -70 (nearest devices). Adjust according to area to be monitored.Also filter ESP devices (vendor ID starting with CC) out.
        receivedRSSI = p->rx_ctrl.rssi;
               
      // RSSI = -10nlog10(d/d0)+A0 // https://www.wouterbulten.nl/blog/tech/kalman-filters-explained-removing-noise-from-rssi-signals/#fn:2
      // https://create.arduino.cc/projecthub/deodates/rssi-based-social-distancing-af0e26
      // Following three variables must be float type.

      float RSSI_1meter = -50; // RSSI at 1 meter distance. Adjust according to your environment.Use WiFi Analyser android app from VREM Software & take average of RSSI @ 1 meter. .
      float Noise = 2;         // Try between 2 to 4. 2 is acceptable number but Adjust according to your environment.
      float Distance = pow(10, (RSSI_1meter -  receivedRSSI) / (10 * Noise)); Serial.print("Distance:  "); Serial.println(Distance);    
      Serial.print("Unknown device detected with MAC ID : "); for (int i = 10; i <= 15; i++) { /*sensorValues[i] = p->payload[i]; */Serial.print(p->payload[i], HEX); }
      Serial.print(" & RSSI : "); Serial.println(receivedRSSI);
     }
  for (int i = 6; i < 256; i = i+10) 
   {     
    if (p->payload[0] == 0x80 && p->payload[4] == i) // Hex value 80 for type - Beacon to filter out unwanted traffic.
    {
      device = p->payload[4];  // Device ID.
            
      EEPROM.readBytes(0, showConfig,256);
         
      for (int i = 0; i < 6; i++) Command[i+4] = showConfig[i+device];      // Prepare command to be sent to remote device.
      for (int j = 0; j < 4; j++) sensorTypes[j] = showConfig[j+device+6];  // Assign sensor types to the particular device.
                    
      timeSynch();
      if (Command[5] == 0 || Command[5] == 255) {Command[4] = device; Command[5] = 107; Command[6] = WiFiChannel; timeSynch();}
      esp_wifi_80211_tx(WIFI_IF_AP, Command, sizeof(Command), true);   // Avoid any Serial.print statements in this callback function to send command to remote devices as soon as possible.
      
      int remoteChannel = p->rx_ctrl.channel;
      int rssi = p->rx_ctrl.rssi;         
      voltage = p->payload[5];
      voltage = voltage * 2 / 100;
      motionLevel = p->payload[16];
      sensorValues[0] = p->payload[6];sensorValues[1] = p->payload[7];sensorValues[2] = p->payload[8];sensorValues[3] = p->payload[9];      

      sprintf (str, "{");sprintf (s, "\"%s\":\"%i\"", "Location", device);strcat(str, s);sprintf(s, ",\"%s\":\"%.2f\"", "Voltage", voltage);strcat(str, s);sprintf(s, ",\"%s\":\"%i\"", "RSSI", rssi);strcat(str, s);sprintf(s, ",\"%s\":\"%i\"", "Motion Level", motionLevel);strcat(str, s);
      sprintf(s, ",\"%s\":\"%i\"", "Father", p->payload[18]);strcat(str, s);
      sprintf(s, ",\"%s\":\"%i\"", "Mother", p->payload[19]);strcat(str, s);
      sprintf(s, ",\"%s\":\"%i\"", "Son", p->payload[20]);strcat(str, s);
      sprintf(s, ",\"%s\":\"%i\"", "Daughter", p->payload[21]);strcat(str, s);
      sprintf(s, ",\"%i\":\"%i\"", sensorTypes[0], sensorValues[0]);strcat(str, s);sprintf (s, ",\"%i\":\"%i\"", sensorTypes[1], sensorValues[1]); strcat (str, s);sprintf (s, ",\"%i\":\"%i\"", sensorTypes[2], sensorValues[2]); strcat (str, s);sprintf (s, ",\"%i\":\"%i\"", sensorTypes[3], sensorValues[3]); strcat (str, s);sprintf (s, "}"); strcat (str, s);
      myClient.publish("sensor", str);
       
      graphData = ",";graphData += epoch;graphData += ",";graphData += device;graphData += ",";graphData += voltage;graphData += ",";graphData += rssi;graphData += ",";graphData += motionLevel;graphData += ",";graphData += sensorTypes[0];graphData += ",";graphData += sensorValues[0];graphData += ",";graphData += sensorTypes[1];graphData += ",";graphData += sensorValues[1];graphData += ",";graphData += sensorTypes[2];graphData += ",";graphData += sensorValues[2];graphData += ",";graphData += sensorTypes[3];graphData += ",";graphData += sensorValues[3];graphData += "]";
      
      Epoch += "," + String(epoch);Loc += "," + String(device);V += "," + String(voltage, 2);S += "," + String(rssi);T += "," + String(sensorValues[0]);H += "," + String(sensorValues[1]);P += "," + String(sensorValues[2]);L += "," + String(sensorValues[3]);
      graphDataToWS = "[" + Epoch + "]," + "[" + Loc + "]," + "[" + V + "]," + "[" + S + "]," + "[" + T + "]," + "[" + H + "]," + "[" + P + "]," + "[" + L + "]";

      if (graphDataToWS.length() > 1000) {
      Epoch = String(epoch);
      //graphDataToWS.remove(0, 34); // Remove first 34 charachters from string.
      Loc = "";V = "";S = "";T = "";H = "";P = "";L = "";
      
      }
      
      if (voltage < 2.50) {      // if voltage of battery gets to low, print the warning below.
         myClient.publish("Warning/Battery Low", String(device));
         }
         for (int i = 0; i < 23; i++) {Command[i] = p->payload[i];}
         commandSent = 1;         
      }
   }
}

void receivedMessage(const MqttClient* source, const Topic& topic, const char* payload, size_t length)
{
  Serial.print("Received message on topic '"); Serial.print(receivedTopic.c_str());Serial.print("' with payload = ");Serial.println(payload);  
  if (receivedTopic == "command") // Each part of Mqtt commands must be 3 digits (for example: 006 for 6). 
  {
    receivedCommand[0] = atoi(&payload[0]);receivedCommand[1] = atoi(&payload[4]);receivedCommand[2] = atoi(&payload[8]);receivedCommand[3] = atoi(&payload[12]);receivedCommand[4] = atoi(&payload[16]);receivedCommand[5] = atoi(&payload[20]);
    Serial.print("Command received via MQTT: ");Serial.print(receivedCommand[0]);Serial.print(receivedCommand[1]);Serial.println(receivedCommand[2]);
    }
    saveCommand();
  }
  
void saveCommand()                                                                           
{    
    if (receivedCommand[1] == 121) // Set sensor types command received.
  {
    for (int i = 0; i < 4; i++) 
     {
      uint8_t tempSensortypes[4];
      tempSensortypes[i] = receivedCommand[i+2];
      EEPROM.writeBytes(receivedCommand[0]+6, tempSensortypes,4);
     }

    } else if (receivedCommand[1] >= 101 && receivedCommand[1] <= 120) 
      {
        
        for (int i = 0; i < 6; i++)
        { 
          uint8_t tempCommand[6];
          tempCommand[i] = receivedCommand[i];
          EEPROM.writeBytes(receivedCommand[0], tempCommand,6);
          
        }

      EEPROM.commit();Serial.println();Serial.println("Command or sensor types saved to EEPROM.");
      EEPROM.readBytes(0, showConfig,256);for(int i=0;i<256;i++){Serial.printf("%d ", showConfig[i]);}Serial.println();
      
      }
}
    
void saveWificonfig()
{ 
  if (ssid.length() > 0 || password.length() > 0) 
  {  
    EEPROM.writeString(270,ssid);EEPROM.writeString(301, password);
    EEPROM.commit();Serial.println();Serial.print("Wifi Configuration saved to EEPROM: SSID=");Serial.print(ssid);Serial.print(" & Password="); Serial.println(password);Serial.println("Restarting Gateway now...");delay(1000);
    ESP.restart();
  }
}
    
void timeSynch()
{ 
    epoch  = getTime();
    Month  = timeinfo.tm_mon;Command[10] = Month.toInt() + 1;  // January is 0.
    Date   = timeinfo.tm_mday;Command[11] = Date.toInt();
    Hour   = timeinfo.tm_hour;Command[12] = Hour.toInt();
    Minute = timeinfo.tm_min;Command[13] = Minute.toInt();
    Second = timeinfo.tm_sec;Command[14] = Second.toInt();
}

void startWiFi()
{
  WiFi.mode(WIFI_AP_STA);
  motionDetector_init(); motionDetector_config(64, 16, 3, 3, false); Serial.setTimeout(1000); // Initializes the storage arrays in internal RAM and start motion detector with custom configuration.
  esp_wifi_set_promiscuous(true); esp_wifi_set_promiscuous_rx_cb(&sniffer); esp_wifi_set_channel(WiFiChannel, WIFI_SECOND_CHAN_NONE);

  WiFi.softAP(apSSID, apPassword, WiFiChannel, hidden);
  Serial.print("AP started with name: ");Serial.println(apSSID);
  
  ssid = EEPROM.readString(270); password = EEPROM.readString(301);
  WiFi.begin(ssid.c_str(), password.c_str());
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Wifi connection failed");
    Serial.print("Connect to Access Point '");Serial.print(apSSID);Serial.println("' and point your browser to 192.168.4.1 to set SSID and password");
    WiFi.disconnect(false);
    delay(1000);
    WiFi.begin(ssid.c_str(), password.c_str());
 }
   
    Serial.print("Connect at IP: ");Serial.print(WiFi.localIP());Serial.print(" or 192.168.4.1");Serial.println(" to monitor and control whole network");
}

void startAsyncwebserver()
{
  ws.onEvent(onEvent);
  webserver.addHandler(&ws);
  
  webserver.addHandler(new SPIFFSEditor(MYFS, http_username,http_password));
  
  webserver.on("/post", HTTP_POST, [](AsyncWebServerRequest * request){
  
  int params = request->params();
  
  for(int i=0;i<params;i++){
  AsyncWebParameter* p = request->getParam(i);
    
    String input0 =request->getParam(0)->value();receivedCommand[0] =(atoi(input0.c_str()));
    String input1 =request->getParam(1)->value();receivedCommand[1] =(atoi(input1.c_str()));  
    String input2 =request->getParam(2)->value();receivedCommand[2] =(atoi(input2.c_str()));
    String input3 =request->getParam(3)->value();receivedCommand[3] =(atoi(input3.c_str())); 
    String input4 =request->getParam(4)->value();receivedCommand[4] =(atoi(input4.c_str()));
    String input5 =request->getParam(5)->value();receivedCommand[5] =(atoi(input5.c_str()));    
   
    ssid = request->getParam(6)->value().c_str();                  
    password = request->getParam(7)->value().c_str();
  /*       
  if(p->isPost()){
    Serial.printf("Command[%s]: %s\n", p->name().c_str(), p->value()); // For debug purpose.
    }
  */
} 
  request -> send(200, "text/plain", "Command received by server successfully.");
  Serial.print("Command received from web interface: ");Serial.print(receivedCommand[0]);Serial.print(receivedCommand[1]);Serial.print(receivedCommand[2]);Serial.print(receivedCommand[3]);Serial.print(receivedCommand[4]);Serial.println(receivedCommand[5]);

  saveWificonfig();
  saveCommand();
}); 
  
  webserver.serveStatic("/", MYFS, "/").setDefaultFile("index.html");
      
  webserver.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index)
      Serial.printf("UploadStart: %s\n", filename.c_str());
      Serial.printf("%s", (const char*)data);
      if(final)
      Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
  });
  
   webserver.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if(!index)
      Serial.printf("BodyStart: %u\n", total);
      Serial.printf("%s", (const char*)data);
      if(index + len == total)
      Serial.printf("BodyEnd: %u\n", total);
  });
  
  //Followin line must be added before server.begin() to allow local lan request see : https://github.com/me-no-dev/ESPAsyncWebServer/issues/726
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    webserver.begin();
}

void notifyClients(String wsData) {
  ws.textAll(wsData);
  Serial.print("Websocket message sent: ");Serial.println(wsData);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) 
{
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
    {
      
      // Process incoming message.
      data[len] = 0;
      String message = "";
      message = (char*)data;
    
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) 
{
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void gpioControl() {

  if ((EEPROM.readByte(2) >= 1 && EEPROM.readByte(2) <= 5) || (EEPROM.readByte(2) >= 12 && EEPROM.readByte(2) <= 39))
  { if (EEPROM.readByte(3) == 1) {
      digitalWrite(EEPROM.readByte(2), HIGH);
    } else if (EEPROM.readByte(2) == 0) {
      digitalWrite(EEPROM.readByte(2), LOW);
    }
    /*
      } else if (commandType == 102){
         analogWrite(EEPROM.readByte(4), EEPROM.readByte(5));

      }
      }
      /*
      } else if (receivedCommand == 105)    {
        // TO DO - write function for neopixel
    */
  }
}

void OTAupdate() {  // Receive  OTA update from bin file on Gateway's LittleFS data folder.
 
  t_httpUpdate_return ret = ESPhttpUpdate.update(binFile);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      ESP.restart();
      break;
  }
}
