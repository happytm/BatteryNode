#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <EEPROM.h>
#include <FS.h>
#include "SPIFFS.h"
#include <SPIFFSEditor.h>
#include <TinyMqtt.h>       // Thanks to https://github.com/hsaturn/TinyMqtt
#include "time.h"
#include "motionDetector.h" // Thanks to: https://github.com/paoloinverse/motionDetector_esp

struct tm timeinfo;
#define MY_TZ "EST5EDT,M3.2.0,M11.1.0" //(New York) https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

#define FIRSTTIME  false  // Define true if setting up Gateway for first time.

const char* apSSID = "ESP";
const char* apPassword = "";
const int apChannel = 7;
const int hidden = 0;                 // If hidden is 1 probe request event handling does not work ?

std::string sentTopic = "data";
std::string receivedTopic = "command";

const char* http_username = "admin";  // Web file editor interface Login.
const char* http_password = "admin";  // Web file editor interface password.

String dataFile = "/data.json";       // File to store sensor data.

int enableCSVgraphOutput = 1; // 0 disable, 1 enable // if enabled, you may use Tools-> Serial Plotter to plot the variance output for each transmitter. 
int scanInterval = 500; // in milliseconds
int motionLevel = RADAR_BOOTING; // initial value = -1, any values < 0 are errors, see motionDetector.h , ERROR LEVELS sections for details on how to intepret any errors.

//==================User configuration not required below this line ================================================

char str [256], s [70];
String ssid,password,graphData, Hour, Minute;
int device, rssi, sleepTime, sensorValues[4], sensorTypes[4]; int arraySize = 10;
float voltage;
uint8_t mac[6],receivedCommand[6],showConfig[256];
const char* ntpServer = "pool.ntp.org";
unsigned long currentMillis, lastMillis;
unsigned long epoch; 
String Epoch = String(epoch);String Loc = String(device);String V = String(voltage, 2);String S = String(rssi);String T = String(sensorValues[0]);String H = String(sensorValues[1]);String P = String(sensorValues[2]);String L = String(sensorValues[3]); 

#define MYFS SPIFFS
#define FORMAT_SPIFFS_IF_FAILED true

MqttBroker broker(1883);
MqttClient myClient(&broker);

AsyncWebServer webserver(80);
AsyncWebSocket ws("/ws");

void notifyClients(String level) {
  ws.textAll(level);
  Serial.print("Websocket message sent: ");Serial.println(level);
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

unsigned long getTime() {time_t now;if (!getLocalTime(&timeinfo)) {Serial.println("Failed to obtain time");return(0);}Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");time(&now);return now;}

void setup(){
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  delay(500);
  SPIFFS.begin();
  EEPROM.begin(512);
#if FIRSTTIME  
  // Setup device numbers and wifi Channel for remote devices in EEPROM permanantly.
  for (int i = 6; i < 256; i = i+10) {EEPROM.writeByte(i, i);EEPROM.writeByte(i+1, 107);EEPROM.writeByte(i+2, apChannel);}}
  EEPROM.writeByte(0, apChannel);EEPROM.commit();
#endif  
  EEPROM.readBytes(0, showConfig,256);for(int i=0;i<256;i++){Serial.printf("%d ", showConfig[i]);}Serial.println();
  
  startWiFi();
  startAsyncwebserver();
  startMotionDetector();
    
  configTime(0, 0, ntpServer); setenv("TZ", MY_TZ, 1); tzset(); // Set environment variable with your time zone
  epoch = getTime(); Serial.print("Epoch Time: "); Serial.println(epoch); delay(500); Epoch = String(epoch);

  broker.begin();
  myClient.setCallback(receivedMessage);
  myClient.subscribe(receivedTopic);
  myClient.subscribe(sentTopic);
        
  WiFi.onEvent(probeRequest,WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED);
  Serial.print("Waiting for probe requests ... ");
 

} // End of setup

void loop()
{
   delay(scanInterval);
   if (WiFi.waitForConnectResult() != WL_CONNECTED) {ssid = EEPROM.readString(270); password = EEPROM.readString(301);Serial.println("Wifi connection failed");Serial.print("Connect to Access Point ");Serial.print(apSSID);Serial.println(" and point your browser to 192.168.4.1 to set SSID and password" );WiFi.disconnect(false);delay(1000);WiFi.begin(ssid.c_str(), password.c_str());}
   notifyClients(String(motionLevel));
   unsigned long lastDetected;
   int lastLevel;
   if (motionLevel > 100)
   {
    lastDetected = getTime();
    lastLevel = motionLevel;
   }
   notifyClients(String(lastDetected));
   notifyClients(String(lastLevel));
   ws.cleanupClients();
   // manageSerialCommands();
    
    motionLevel = motionDetector_esp();  // if the connection fails, the radar will automatically try to switch to different operating modes by using ESP32 specific calls. 
    if (enableCSVgraphOutput == 0) {  // for the graph via serial, we just let the library do the job. 
      Serial.print("motionLevel: ");
      Serial.println(motionLevel);
    }
}  // End of loop


void probeRequest(WiFiEvent_t event, WiFiEventInfo_t info) 
{ 
  Serial.println();
  Serial.print("Probe Received :  ");for (int i = 0; i < 6; i++) {Serial.printf("%02X", info.wifi_ap_probereqrecved.mac[i]);if (i < 5)Serial.print(":");}Serial.println();
  Serial.print("Connect at IP: ");Serial.print(WiFi.localIP()); Serial.print(" or 192.168.4.1 with connection to ESP AP");Serial.println(" to monitor and control whole network");
  // Allow data from device ID ending in number 6 and voltage value between 2.4V and 3.6V.
  for (int i = 6; i < 256; i = i+10) if (info.wifi_ap_probereqrecved.mac[0] == i && (info.wifi_ap_probereqrecved.mac[1] > 120 || info.wifi_ap_probereqrecved.mac[1] < 180))
   {
    // This helps reduce interference from unknown devices from far away with weak signals.
    if (info.wifi_ap_probereqrecved.rssi > -70)  
    {
      
      device = info.wifi_ap_probereqrecved.mac[0];
      Serial.println("Contents of command data saved in EEPROM for this device: ");
      EEPROM.readBytes(0, showConfig,256);for(int i=0;i<10;i++){ 
      Serial.printf("%d ", showConfig[i+device]);}
      
      Serial.println();
      for (int i = 0; i < 6; i++) mac[i] = showConfig[i+device];   // Prepare command to be sent to remote device.
      for (int j = 0; j < 4; j++) sensorTypes[j] = showConfig[j+device+6]; // Assign sensor types to the particular device.
                    
      timeSynch();
      if (mac[1] == 0 || mac[1] == 255) {mac[0] = device; mac[1] = 107; mac[2] = apChannel; timeSynch();}
                     
      esp_err_t esp_base_mac_addr_set(uint8_t *mac);  // https://github.com/justcallmekoko/ESP32Marauder/issues/418
      Serial.print("Command sent to remote device :  "); Serial.print(mac[0]);Serial.print("/");Serial.print(mac[1]);Serial.print("/");Serial.print(mac[2]);Serial.print("/");Serial.print(mac[3]);Serial.print("/");Serial.print(mac[4]);Serial.print("/");Serial.print(mac[5]);Serial.println("/");        
          
      rssi = info.wifi_ap_probereqrecved.rssi;         
      voltage = info.wifi_ap_probereqrecved.mac[1];
      voltage = voltage * 2 / 100;
      sensorValues[0] = info.wifi_ap_probereqrecved.mac[2];sensorValues[1] = info.wifi_ap_probereqrecved.mac[3];sensorValues[2] = info.wifi_ap_probereqrecved.mac[4];sensorValues[3] = info.wifi_ap_probereqrecved.mac[5];
           
      sprintf (str, "{");sprintf (s, "\"%s\":\"%i\"", "Location", device);    strcat (str, s);sprintf (s, ",\"%s\":\"%.2f\"", "Voltage", voltage);    strcat (str, s);sprintf (s, ",\"%i\":\"%i\"", sensorTypes[0], sensorValues[0]); strcat (str, s);sprintf (s, ",\"%i\":\"%i\"", sensorTypes[1], sensorValues[1]); strcat (str, s);sprintf (s, ",\"%i\":\"%i\"", sensorTypes[2], sensorValues[2]); strcat (str, s);sprintf (s, ",\"%i\":\"%i\"", sensorTypes[3], sensorValues[3]); strcat (str, s);sprintf (s, "}"); strcat (str, s);
                    
      Serial.println("Following ## Sensor Values ## receiced from remote device  & published via MQTT: ");Serial.println(str);
      
      myClient.publish("sensor", str);
       
      graphData = ",";graphData += epoch;graphData += ",";graphData += device;graphData += ",";graphData += voltage;graphData += ",";graphData += rssi;graphData += ",";graphData += sensorTypes[0];graphData += ",";graphData += sensorValues[0];graphData += ",";graphData += sensorTypes[1];graphData += ",";graphData += sensorValues[1];graphData += ",";graphData += sensorTypes[2];graphData += ",";graphData += sensorValues[2];graphData += ",";graphData += sensorTypes[3];graphData += ",";graphData += sensorValues[3];graphData += "]";
     
      File f = SPIFFS.open(dataFile, "r+"); // See https://github.com/lorol/LITTLEFS/issues/33
      Serial.print("File size: "); Serial.println(f.size());
      f.seek((f.size()-1), SeekSet);
      Serial.print("Position: "); Serial.println(f.position());
      f.print(graphData);Serial.println();
      Serial.print("Appended to file: "); Serial.println(graphData);
      Serial.print("File size: "); Serial.println(f.size());
      f.close(); 
                    
      if (voltage < 2.50) {      // if voltage of battery gets to low, print the warning below.
         myClient.publish("Warning/Battery Low", String(device));
      }         
    }
  }    
       // In some cases send command only once.
       if (mac[1] == 105 || mac[1] == 106 || mac[1] == 108 || mac[1] == 110){for (int i = 6; i < 256; i = i+10) {EEPROM.writeByte(i+1, 107);EEPROM.writeByte(i+2, apChannel);}}
} // End of Proberequest function.


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
    EEPROM.commit();Serial.println();Serial.print("Wifi Configuration saved to EEPROM: SSID="); Serial.print(ssid);Serial.print(" & Password="); Serial.println(password);Serial.println("Restarting Gateway now...");delay(1000);
    ESP.restart();
  }
}
    
void timeSynch(){ if (mac[1] == 105 || mac[1] == 106) {return;}else{epoch = getTime();Serial.print("Epoch Time: ");Serial.println(epoch); Hour = timeinfo.tm_hour; mac[4] = Hour.toInt();Minute = timeinfo.tm_min; mac[5] = Minute.toInt();Serial.print("Time Sent to remote device ");Serial.print(device);Serial.print("  ");Serial.print(Hour); Serial.print(":"); Serial.println(Minute);}}

void startWiFi()
{
  WiFi.mode(WIFI_AP_STA);
  
  WiFi.softAP(apSSID, apPassword, apChannel, hidden);
  esp_wifi_set_event_mask(WIFI_EVENT_MASK_NONE); // This line is must to activate probe request received event handler.
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
   
    Serial.print("Connect at IP: ");Serial.print(WiFi.localIP()); Serial.print(" or 192.168.4.1");Serial.println(" to monitor and control whole network");
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
    password =request->getParam(7)->value().c_str();
  /*       
  if(p->isPost()){
    Serial.printf("Command[%s]: %s\n", p->name().c_str(), p->value()); // For debug purpose.
    }
  */
} 
  request -> send(200, "text/plain", "Command received by server successfully, please click browser's back button to get back to main page.");
  Serial.print("Command received from Browser: ");Serial.print(receivedCommand[0]);Serial.print(receivedCommand[1]);Serial.print(receivedCommand[2]);Serial.print(receivedCommand[3]);Serial.print(receivedCommand[4]);Serial.println(receivedCommand[5]);

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

void startMotionDetector(){

    motionDetector_init();  // initializes the storage arrays in internal RAM
    motionDetector_config(64, 16, 3, 3, false); 
    motionDetector_debug_via_serial(0); // show debugging information, at the simplest debugging level. Level 0 means no output. 
    motionDetector_set_debug_level(1); // set a very verbose level for operating the radar.
    
    if (enableCSVgraphOutput > 0) {      // USE THE Tools->Serial Plotter to graph the live data!
      motionDetector_enable_serial_CSV_graph_data(enableCSVgraphOutput); // output CSV data only
    }

    Serial.setTimeout(1000);
 }
