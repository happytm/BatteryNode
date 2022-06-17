#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <EEPROM.h>
#include "FS.h"
#include "SPIFFS.h"
#include <SPIFFSEditor.h>
#include <TinyMqtt.h>         // Thanks to https://github.com/hsaturn/TinyMqtt
#include "time.h"

#define FIRSTTIME  false      // Define true if setting up Gateway for first time.

int WiFiChannel = 7;          // This must be same for all devices on network.
const char* apSSID = "ESP";   // SoftAP SSID.
const char* apPassword = "";  // SoftAP password.
const int hidden = 0;         // If hidden is 1 probe request event handling does not work ?

struct tm timeinfo;
#define MY_TZ "EST5EDT,M3.2.0,M11.1.0" //(New York) https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

std::string sentTopic = "data";
std::string receivedTopic = "command";

const char* http_username = "admin";  // Web file editor interface Login.
const char* http_password = "admin";  // Web file editor interface password.

String dataFile = "/data.json";       // File to store sensor data.

//==================User configuration not required below this line ================================================

#define MYFS SPIFFS
#define FORMAT_SPIFFS_IF_FAILED true

MqttBroker broker(1883);
MqttClient myClient(&broker);

AsyncWebServer webserver(80);

char str [256], s [70];
String ssid,password,graphData, Hour, Minute;
int device, rssi, sleepTime, motionLevel, sensorValues[4], sensorTypes[4], commandSent;
float voltage;
uint8_t receivedCommand[6],showConfig[256];
const char* ntpServer = "pool.ntp.org";
unsigned long currentMillis, lastMillis;
unsigned long epoch; 
String Epoch = String(epoch);String Loc = String(device);String V = String(voltage, 2);String S = String(rssi);String T = String(sensorValues[0]);String H = String(sensorValues[1]);String P = String(sensorValues[2]);String L = String(sensorValues[3]); 

uint8_t Command[] =                                            // Maximum limit is 1500 bytes?
{
  0x80, 0x00,                                                 //  0- 1: Frame Control. Type 8 = Beacon.
  0x00, 0x00,                                                 //  2- 3: Duration
  0x06, 0x44, 0x44, 0x44, 0x44, 0x44,                         //  4- 9: Destination address
  0x06, 0x55, 0x55, 0x55, 0x55, 0x55,                         // 10-15: Source address
  0x66, 0x66, 0x66, 0x66, 0x66, 0x66,                         // 16-21: BSSID
  0x00, 0x00,                                                 // 22-23: Sequence / fragment number
}; 

unsigned long getTime() {time_t now;if (!getLocalTime(&timeinfo)) {Serial.println("Failed to obtain time");return(0);}time(&now);return now;}

void setup() {
  Serial.begin(115200);
  delay(500);
  SPIFFS.begin();
  EEPROM.begin(512);
#if FIRSTTIME  
  // Setup device IDs and wifi Channel for remote devices in EEPROM permanantly.
  for (int i = 6; i < 256; i = i+10) {EEPROM.writeByte(i, i);EEPROM.writeByte(i+1, 107);EEPROM.writeByte(i+2, apChannel);}}
  EEPROM.writeByte(0, WiFiChannel);EEPROM.commit();
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
   if (WiFi.waitForConnectResult() != WL_CONNECTED) {ssid = EEPROM.readString(270); password = EEPROM.readString(301);Serial.println("Wifi connection failed");WiFi.disconnect(false);delay(1000);WiFi.begin(ssid.c_str(), password.c_str());}
   
   delay(10);
   if (commandSent == 1)
   {  
      Serial.println();Serial.print("Data received from remote sensor -  ");Serial.print(device);Serial.println(" is below: ");for (int i = 0; i < 23; i++) {Serial.printf("%02X", sensorValues[i]);if (i < 22)Serial.print(":");}Serial.println();
      Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
      Serial.print("Epoch Time: ");Serial.println(epoch); Serial.print("Time Sent to remote device - ");Serial.print(device);Serial.print(" = ");Serial.print(Hour); Serial.print(":"); Serial.println(Minute);
      Serial.print("Connect at IP: ");Serial.print(WiFi.localIP()); Serial.print(" or 192.168.4.1");Serial.println(" to monitor and control whole network");      Serial.println("Following ## Sensor Values ## receiced from remote device  & published via MQTT: ");Serial.println(str);      
      for(int i=0;i<10;i++){ Serial.printf("%d ", showConfig[i+device]);} Serial.println();
      Serial.println("Received packet & sending command...... ");
      Serial.print("Sent Command to remote device: "); for (int i = 4; i < 9; i++) {Serial.print(Command[i]);} Serial.println();
      
      File f = SPIFFS.open(dataFile, "r+"); // See https://github.com/lorol/LITTLEFS/issues/33
      f.seek((f.size()-1), SeekSet);f.print(graphData);f.close(); Serial.println("Sensor data saved to flash.");
      commandSent = 0;
      }
}

void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) 
{ 
  // Avoid any Serial.print statements in this callback function to send command to remote devices as soon as possible.
  
  wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t*)buf;
  
  for (int i = 6; i < 256; i = i+10) 
   {
    if (p->payload[0] == 0x80 && p->payload[4] == i) // Hex value 80 for type - Beacon to filter out unwanted traffic.
    {
      
      device = p->payload[4];  // Device ID.
            
      EEPROM.readBytes(0, showConfig,256);
         
      for (int i = 0; i < 6; i++) Command[i+4] = showConfig[i+device];      // Prepare command to be sent to remote device.
      for (int j = 0; j < 4; j++) sensorTypes[j] = showConfig[j+device+6]; // Assign sensor types to the particular device.
                    
      timeSynch();
      if (Command[5] == 0 || Command[5] == 255) {Command[4] = device; Command[5] = 107; Command[6] = WiFiChannel; timeSynch();}
      esp_wifi_80211_tx(WIFI_IF_AP, Command, sizeof(Command), true);   // Avoid any Serial.print statements in this callback function to send command to remote devices as soon as possible.
      
      int remoteChannel = p->rx_ctrl.channel;
      int rssi = p->rx_ctrl.rssi;         
      voltage = p->payload[5];
      voltage = voltage * 2 / 100;
      motionLevel = p->payload[10];
      sensorValues[0] = p->payload[6];sensorValues[1] = p->payload[7];sensorValues[2] = p->payload[8];sensorValues[3] = p->payload[9];      

      sprintf (str, "{");sprintf (s, "\"%s\":\"%i\"", "Location", device);strcat(str, s);sprintf(s, ",\"%s\":\"%.2f\"", "Voltage", voltage);strcat(str, s);sprintf(s, ",\"%s\":\"%i\"", "RSSI", rssi);strcat(str, s);sprintf(s, ",\"%s\":\"%i\"", "Motion Level", motionLevel);strcat(str, s);
      sprintf(s, ",\"%s\":\"%i\"", "Father", p->payload[11]);strcat(str, s);
      sprintf(s, ",\"%s\":\"%i\"", "Mother", p->payload[12]);strcat(str, s);
      sprintf(s, ",\"%s\":\"%i\"", "Son", p->payload[13]);strcat(str, s);
      sprintf(s, ",\"%s\":\"%i\"", "Daughter", p->payload[14]);strcat(str, s);
      sprintf(s, ",\"%i\":\"%i\"", sensorTypes[0], sensorValues[0]);strcat(str, s);sprintf (s, ",\"%i\":\"%i\"", sensorTypes[1], sensorValues[1]); strcat (str, s);sprintf (s, ",\"%i\":\"%i\"", sensorTypes[2], sensorValues[2]); strcat (str, s);sprintf (s, ",\"%i\":\"%i\"", sensorTypes[3], sensorValues[3]); strcat (str, s);sprintf (s, "}"); strcat (str, s);
      myClient.publish("sensor", str);
       
      graphData = ",";graphData += epoch;graphData += ",";graphData += device;graphData += ",";graphData += voltage;graphData += ",";graphData += rssi;graphData += ",";graphData += motionLevel;graphData += ",";graphData += sensorTypes[0];graphData += ",";graphData += sensorValues[0];graphData += ",";graphData += sensorTypes[1];graphData += ",";graphData += sensorValues[1];graphData += ",";graphData += sensorTypes[2];graphData += ",";graphData += sensorValues[2];graphData += ",";graphData += sensorTypes[3];graphData += ",";graphData += sensorValues[3];graphData += "]";
                                      
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
    EEPROM.commit();Serial.println();Serial.print("Wifi Configuration saved to EEPROM: SSID="); Serial.print(ssid);Serial.print(" & Password="); Serial.println(password);Serial.println("Restarting Gateway now...");delay(1000);
    ESP.restart();
  }
}
    
void timeSynch(){ if (Command[1] == 105 || Command[1] == 106) {return;}else{epoch = getTime();Hour = timeinfo.tm_hour; Command[8] = Hour.toInt();Minute = timeinfo.tm_min; Command[9] = Minute.toInt();}}

void startWiFi()
{
  WiFi.mode(WIFI_AP_STA);
  
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
   
    Serial.print("Connect at IP: ");Serial.print(WiFi.localIP()); Serial.print(" or 192.168.4.1");Serial.println(" to monitor and control whole network");

}


void startAsyncwebserver()
{
  
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
