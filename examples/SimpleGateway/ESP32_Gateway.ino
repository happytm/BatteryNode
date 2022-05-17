#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <EEPROM.h>
#include <FS.h>
#include "LITTLEFS.h"
#include <SPIFFSEditor.h>
#include <TinyMqtt.h>   // Thanks to https://github.com/hsaturn/TinyMqtt
#include "time.h"

struct tm timeinfo;
#define MY_TZ "EST5EDT,M3.2.0,M11.1.0" //(New York) https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

#define MYFS LITTLEFS
#define FORMAT_LITTLEFS_IF_FAILED true

const char* ssid = "";
const char* password = "";
const char* apSSID = "ESP";
const char* apPassword = "";
const int apChannel = 7;
const int hidden = 0; // If hidden is 1 probe request event handling does not work ?

const char* http_username = "admin";  // Web file editor interface Login.
const char* http_password = "admin";  // Web file editor interface password.

String dataFile = "/data.json";  // File to store sensor data.
//String configFile = "/config.json";  // File to store configuration data.

int livingroom[4] = {16,26,36,46};
int kitchen[4] =    {46,36,26,16};
int bedroom1[4] =   {46,36,26,16};
int bedroom2[4] =   {16,26,36,36};
int bedroom3[4] =   {16,26,36,36};
int bedroom4[4] =   {16,26,36,36};
int bathroom1[4] =  {16,26,36,36};
int bathroom2[4] =  {16,26,36,36};
int bathroom3[4] =  {16,26,36,36};
int bathroom4[4] =  {16,26,36,36};
int laundry[4] =    {16,26,36,36};
int boiler[4] =     {16,26,36,36};
int workshop[4] =   {16,26,36,36};
int garage[4] =     {16,26,36,36};
int office[4] =     {16,26,36,36};
int tank[4] =       {16,26,36,36};
int solar[4] =      {16,26,36,36};
//==================User configuration not required below this line ================================================

char str [256], s [70];
String graphData, configData, Hour, Minute;
int device, rssi, sensorValues[4], sensorTypes[4];
float voltage;
int arraySize = 10;   // Data integers for each device.
uint8_t Config[266],savedConfig[266],mac[6],receivedCommand[10],livingroomCommand[10],kitchenCommand[10],bedroom1Command[10],bedroom2Command[10],bedroom3Command[10],bedroom4Command[10],bathroom1Command[10],bathroom2Command[10],bathroom3Command[10],bathroom4Command[10],laundryCommand[10],boilerCommand[10],workshopCommand[10],garageCommand[10],officeCommand[10],tankCommand[10],solarCommand[10];
const char* ntpServer = "pool.ntp.org";
unsigned long epoch; 
String Epoch = String(epoch);String Loc = String(device);String V = String(voltage, 2);String S = String(rssi);String T = String(sensorValues[0]);String H = String(sensorValues[1]);String P = String(sensorValues[2]);String L = String(sensorValues[3]); 

std::string sentTopic = "data";
std::string receivedTopic = "command";
MqttBroker broker(1883);
MqttClient myClient(&broker);

AsyncWebServer webserver(80);

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
    if (receivedCommand[0] == 6 && receivedCommand[1] == 106) { for (int i = 3; i < 5; i++) livingroom[i] = receivedCommand[i];} else {for (int i = device; i < (device+6); i++) savedConfig[i] = receivedCommand[i];}    
    if (receivedCommand[0] == 6 && receivedCommand[1] == 106) { for (int i = 2; i < 5; i++) kitchen[i] = receivedCommand[i];} else {for (int i = 0; i < 6; i++) kitchenCommand[i] = receivedCommand[i];}
    if (receivedCommand[0] == 26 && receivedCommand[1] == 106) { for (int i = 2; i < 5; i++) bedroom1[i] = receivedCommand[i];} else {for (int i = 0; i < 6; i++) bedroom1Command[i] = receivedCommand[i];}
    if (receivedCommand[0] == 36 && receivedCommand[1] == 106) { for (int i = 2; i < 5; i++) bedroom2[i] = receivedCommand[i];} else {for (int i = 0; i < 6; i++) bedroom2Command[i] = receivedCommand[i];}
    if (receivedCommand[0] == 46 && receivedCommand[1] == 106) { for (int i = 2; i < 5; i++) bedroom3[i] = receivedCommand[i];} else {for (int i = 0; i < 6; i++) bedroom3Command[i] = receivedCommand[i];}
    if (receivedCommand[0] == 56 && receivedCommand[1] == 106) { for (int i = 2; i < 5; i++) bedroom4[i] = receivedCommand[i];} else {for (int i = 0; i < 6; i++) bedroom4Command[i] = receivedCommand[i];}
    if (receivedCommand[0] == 66 && receivedCommand[1] == 106) { for (int i = 2; i < 5; i++) bathroom1[i] = receivedCommand[i];} else {for (int i = 0; i < 6; i++) bathroom1Command[i] = receivedCommand[i];}
    if (receivedCommand[0] == 76 && receivedCommand[1] == 106) { for (int i = 2; i < 5; i++) bathroom2[i] = receivedCommand[i];} else {for (int i = 0; i < 6; i++) bathroom2Command[i] = receivedCommand[i];}
    if (receivedCommand[0] == 86 && receivedCommand[1] == 106) { for (int i = 2; i < 5; i++) bathroom3[i] = receivedCommand[i];} else {for (int i = 0; i < 6; i++) bathroom3Command[i] = receivedCommand[i];}
    if (receivedCommand[0] == 96 && receivedCommand[1] == 106) { for (int i = 2; i < 5; i++) bathroom4[i] = receivedCommand[i];} else {for (int i = 0; i < 6; i++) bathroom4Command[i] = receivedCommand[i];}
    if (receivedCommand[0] == 106 && receivedCommand[1] == 106) { for (int i = 2; i < 5; i++) laundry[i] = receivedCommand[i];} else {for (int i = 0; i < 6; i++) laundryCommand[i] = receivedCommand[i];}
    if (receivedCommand[0] == 116 && receivedCommand[1] == 106) { for (int i = 2; i < 5; i++) boiler[i] = receivedCommand[i];} else {for (int i = 0; i < 6; i++) boilerCommand[i] = receivedCommand[i];}
    if (receivedCommand[0] == 126 && receivedCommand[1] == 106) { for (int i = 2; i < 5; i++) workshop[i] = receivedCommand[i];} else {for (int i = 0; i < 6; i++) workshopCommand[i] = receivedCommand[i];}
    if (receivedCommand[0] == 136 && receivedCommand[1] == 106) { for (int i = 2; i < 5; i++) garage[i] = receivedCommand[i];} else {for (int i = 0; i < 6; i++) garageCommand[i] = receivedCommand[i];}
    if (receivedCommand[0] == 146 && receivedCommand[1] == 106) { for (int i = 2; i < 5; i++) office[i] = receivedCommand[i];} else {for (int i = 0; i < 6; i++) officeCommand[i] = receivedCommand[i];}
    if (receivedCommand[0] == 156 && receivedCommand[1] == 106) { for (int i = 2; i < 5; i++) tank[i] = receivedCommand[i];} else {for (int i = 0; i < 6; i++) tankCommand[i] = receivedCommand[i];}
    if (receivedCommand[0] == 166 && receivedCommand[1] == 106) { for (int i = 2; i < 5; i++) solar[i] = receivedCommand[i];} else {for (int i = 0; i < 6; i++) solarCommand[i] = receivedCommand[i];}
    
    EEPROM.readBytes(0, Config,266);for(int k=0;k<266;k++){Serial.printf("%d ", Config[k]);}
 }                                               
 
void timeSynch(){ if (receivedCommand[1] != 105) {Hour = timeinfo.tm_hour; mac[4] = Hour.toInt();Minute = timeinfo.tm_min; mac[5] = Minute.toInt();Serial.print("Time Sent to remote device ");Serial.print(device);Serial.print("  ");Serial.print(Hour); Serial.print(":"); Serial.println(Minute);}}

unsigned long getTime() {
time_t now;
if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return(0);
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  time(&now);
  return now;
}

void setup(){
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apSSID, apPassword, apChannel, hidden);
  esp_wifi_set_event_mask(WIFI_EVENT_MASK_NONE); // This line is must to activate probe request received event handler.
 
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("STA: Failed!\n");
    WiFi.disconnect(false);
    delay(1000);
    WiFi.begin(ssid, password);
  }
  
  Serial.print("CONNECTED IP: ");Serial.println(WiFi.localIP());
  
  configTime(0, 0, ntpServer); setenv("TZ", MY_TZ, 1); tzset(); // Set environment variable with your time zone
 
  epoch = getTime();
  Serial.print("Epoch Time: ");Serial.println(epoch);
  delay(500);
  Epoch = String(epoch);

  LITTLEFS.begin();
  EEPROM.begin(512);  
  
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
    String input6 =request->getParam(6)->value();receivedCommand[6] =(atoi(input6.c_str()));
    String input7 =request->getParam(7)->value();receivedCommand[7] =(atoi(input7.c_str()));  
    String input8 =request->getParam(8)->value();receivedCommand[8] =(atoi(input8.c_str()));
    String input9 =request->getParam(9)->value();receivedCommand[9] =(atoi(input9.c_str())); 

    ssid = request->getParam(10)->value().c_str();                  
    password =request->getParam(11)->value().c_str();
 /* 
  if(p->isPost()){
    Serial.printf("Command[%s]: %s\n", p->name().c_str(), p->value());
    }
 */
 } 
request -> send(200, "text/plain", "Command received by server successfully, please click browser's back button to get back to main page.");
Serial.print("Command received from Browser: ");Serial.print(receivedCommand[0]);Serial.print(receivedCommand[1]);Serial.print(receivedCommand[2]);Serial.print(receivedCommand[3]);Serial.print(receivedCommand[4]);Serial.println(receivedCommand[5]);
saveCommand();
//Serial.println(ssid);
//Serial.println(password);
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

    broker.begin();

    myClient.setCallback(receivedMessage);
    myClient.subscribe(receivedTopic);
    myClient.subscribe(sentTopic);
        
    WiFi.onEvent(probeRequest, SYSTEM_EVENT_AP_PROBEREQRECVED);
    Serial.print("Waiting for probe requests ... ");

} // End of setup

void loop(){
 if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("STA: Failed!\n");
    WiFi.disconnect(false);
    delay(1000);
    WiFi.begin(ssid, password);
  }
 }  // End of loop


void probeRequest(WiFiEvent_t event, WiFiEventInfo_t info) 
{
    Serial.print("Probe Received :  ");
    for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", info.ap_probereqrecved.mac[i]);
    if (i < 5)Serial.print(":");
    }
    Serial.println();

    if (info.ap_probereqrecved.mac[0] == 6 || info.ap_probereqrecved.mac[0] == 16 || info.ap_probereqrecved.mac[0] == 26 || info.ap_probereqrecved.mac[0] == 36 || info.ap_probereqrecved.mac[0] == 46 || info.ap_probereqrecved.mac[0] == 56 || info.ap_probereqrecved.mac[0] == 66 || info.ap_probereqrecved.mac[0] == 76 || info.ap_probereqrecved.mac[0] == 86 || info.ap_probereqrecved.mac[0] == 96 || info.ap_probereqrecved.mac[0] == 106 || info.ap_probereqrecved.mac[0] == 116 || info.ap_probereqrecved.mac[0] == 126 || info.ap_probereqrecved.mac[0] == 136 || info.ap_probereqrecved.mac[0] == 146 || info.ap_probereqrecved.mac[0] == 156 || info.ap_probereqrecved.mac[0] == 166 || info.ap_probereqrecved.mac[0] == 176 || info.ap_probereqrecved.mac[0] == 186 || info.ap_probereqrecved.mac[0] == 196 || info.ap_probereqrecved.mac[0] == 206 || info.ap_probereqrecved.mac[0] == 216 || info.ap_probereqrecved.mac[0] == 226 || info.ap_probereqrecved.mac[0] == 236 || info.ap_probereqrecved.mac[0] == 246) // only accept data from certain devices.
    {
      device = info.ap_probereqrecved.mac[0];
      epoch = getTime();
      Serial.print("Epoch Time: ");Serial.println(epoch); 
             
      if (device == 6)  { EEPROM.writeBytes(device, savedConfig,arraySize); for (int j = 0; j < 6; j++) mac[j] = savedConfig[j]; for (int k = 0; k < arraySize; k++) sensorTypes[k] = livingroomCommand[k]; timeSynch();}
      if (device == 16) { EEPROM.writeBytes(device, Config,arraySize); for (int j = 0; j < 6; j++) mac[j] = kitchenCommand[j]; for (int k = 7; k < arraySize; k++) sensorTypes[k] = kitchenCommand[k]; timeSynch();}
      if (device == 26) { EEPROM.writeBytes(device, Config,arraySize); for (int j = 0; j < 6; j++) mac[j] = bedroom1Command[j]; for (int k = 7; k < arraySize; k++) sensorTypes[k] = bedroom1Command[k]; timeSynch();}
      if (device == 36) { EEPROM.writeBytes(device, Config,arraySize); for (int j = 0; j < 6; j++) mac[j] = bedroom2Command[j]; for (int k = 7; k < arraySize; k++) sensorTypes[k] = bedroom2Command[k]; timeSynch();}
      if (device == 46) { EEPROM.writeBytes(device, Config,arraySize); for (int j = 0; j < 6; j++) mac[j] = bedroom3Command[j]; for (int k = 7; k < arraySize; k++) sensorTypes[k] = bedroom3Command[k]; timeSynch();}
      if (device == 56) { EEPROM.writeBytes(device, Config,arraySize); for (int j = 0; j < 6; j++) mac[j] = bedroom4Command[j]; for (int k = 7; k < arraySize; k++) sensorTypes[k] = bedroom4Command[k]; timeSynch();}
      if (device == 66) { EEPROM.writeBytes(device, Config,arraySize); for (int j = 0; j < 6; j++) mac[j] = bathroom1Command[j]; for (int k = 7; k < arraySize; k++) sensorTypes[k] = bathroom1Command[k]; timeSynch();}
      if (device == 76) { EEPROM.writeBytes(device, Config,arraySize); for (int j = 0; j < 6; j++) mac[j] = bathroom2Command[j]; for (int k = 7; k < arraySize; k++) sensorTypes[k] = bathroom2Command[k]; timeSynch();}
      if (device == 86) { EEPROM.writeBytes(device, Config,arraySize); for (int j = 0; j < 6; j++) mac[j] = bathroom3Command[j]; for (int k = 7; k < arraySize; k++) sensorTypes[k] = bathroom3Command[k]; timeSynch();}
      if (device == 96) { EEPROM.writeBytes(device, Config,arraySize); for (int j = 0; j < 6; j++) mac[j] = bathroom4Command[j]; for (int k = 7; k < arraySize; k++) sensorTypes[k] = bathroom4Command[k]; timeSynch();}
      if (device == 106) { EEPROM.writeBytes(device, Config,arraySize); for (int j = 0; j < 6; j++) mac[j] = laundryCommand[j]; for (int k = 7; k < arraySize; k++) sensorTypes[k] = laundryCommand[k]; timeSynch();}
      if (device == 116) { EEPROM.writeBytes(device, Config,arraySize); for (int j = 0; j < 6; j++) mac[j] = boilerCommand[j]; for (int k = 7; k < arraySize; k++) sensorTypes[k] = boilerCommand[k]; timeSynch();}
      if (device == 126) { EEPROM.writeBytes(device, Config,arraySize); for (int j = 0; j < 6; j++) mac[j] = workshopCommand[j]; for (int k = 7; k < arraySize; k++) sensorTypes[k] = workshopCommand[k]; timeSynch();}
      if (device == 136) { EEPROM.writeBytes(device, Config,arraySize); for (int j = 0; j < 6; j++) mac[j] = garageCommand[j]; for (int k = 7; k < arraySize; k++) sensorTypes[k] = garageCommand[k]; timeSynch();}
      if (device == 146) { EEPROM.writeBytes(device, Config,arraySize); for (int j = 0; j < 6; j++) mac[j] = officeCommand[j]; for (int k = 7; k < arraySize; k++) sensorTypes[k] = officeCommand[k]; timeSynch();}
      if (device == 156) { EEPROM.writeBytes(device, Config,arraySize); for (int j = 0; j < 6; j++) mac[j] = tankCommand[j]; for (int k = 7; k < arraySize; k++) sensorTypes[k] = tankCommand[k]; timeSynch();}
      if (device == 166) { EEPROM.writeBytes(device, Config,arraySize); for (int j = 0; j < 6; j++) mac[j] = solarCommand[j]; for (int k = 7; k < arraySize; k++) sensorTypes[k] = solarCommand[k]; timeSynch();}
      
      if (mac[1] == 0) {mac[0] = device; mac[1] = 101; timeSynch();}
        
      
             
      esp_wifi_set_mac(ESP_IF_WIFI_AP, mac);
      Serial.print("Command sent to remote device :  ");Serial.print(mac[0]);Serial.print("/");Serial.print(mac[1]);Serial.print("/");Serial.print(mac[2]);Serial.print("/");Serial.print(mac[3]);Serial.print("/");Serial.print(mac[4]);Serial.print("/");Serial.print(mac[5]);Serial.println("/");        
               
      rssi = info.ap_probereqrecved.rssi;         
      voltage = info.ap_probereqrecved.mac[1];
      voltage = voltage * 2 / 100;
      sensorValues[0] = info.ap_probereqrecved.mac[2];
      sensorValues[1] = info.ap_probereqrecved.mac[3];
      sensorValues[2] = info.ap_probereqrecved.mac[4];
      sensorValues[3] = info.ap_probereqrecved.mac[5];
           
      sprintf (str, "{");
      sprintf (s, "\"%s\":\"%i\"", "Location", device);    strcat (str, s);
      sprintf (s, ",\"%s\":\"%.2f\"", "Voltage", voltage);    strcat (str, s);
      sprintf (s, ",\"%i\":\"%i\"", sensorTypes[0], sensorValues[0]); strcat (str, s);
      sprintf (s, ",\"%i\":\"%i\"", sensorTypes[1], sensorValues[1]); strcat (str, s);
      sprintf (s, ",\"%i\":\"%i\"", sensorTypes[2], sensorValues[2]); strcat (str, s);
      sprintf (s, ",\"%i\":\"%i\"", sensorTypes[3], sensorValues[3]); strcat (str, s);
      sprintf (s, "}"); strcat (str, s);
                    
      Serial.println("Following ## Sensor Values ## receiced from remote device  & published via MQTT: ");
      Serial.println(str);
      
      myClient.publish("sensor", str);
    
      graphData = ",";graphData += epoch;graphData += ",";graphData += device;graphData += ",";graphData += voltage;graphData += ",";graphData += rssi;graphData += ",";graphData += sensorValues[0];graphData += ",";graphData += sensorValues[1];graphData += ",";graphData += sensorValues[2];graphData += ",";graphData += sensorValues[3];graphData += "]";
     
      File f = LITTLEFS.open(dataFile, "r+"); // See https://github.com/lorol/LITTLEFS/issues/33
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
