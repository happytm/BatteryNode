#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <FS.h>
#include "LITTLEFS.h"
#include <SPIFFSEditor.h>
#include "time.h"
#include <TinyMqtt.h>   // Thanks to https://github.com/hsaturn/TinyMqtt

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

int Livingroom[4] = {16,26,36,46};
int Kitchen[4] =    {46,36,26,16};
int Bedroom1[4] =   {46,36,26,16};
int Bedroom2[4] =   {16,26,36,36};
int Bedroom3[4] =   {16,26,36,36};
int Bedroom4[4] =   {16,26,36,36};
int Bathroom1[4] =  {16,26,36,36};
int Bathroom2[4] =  {16,26,36,36};
int Bathroom3[4] =  {16,26,36,36};
int Bathroom4[4] =  {16,26,36,36};
int Laundry[4] =    {16,26,36,36};
int Boiler[4] =     {16,26,36,36};
int Workshop[4] =   {16,26,36,36};
int Garage[4] =     {16,26,36,36};
int Office[4] =     {16,26,36,36};
int Tank[4] =       {16,26,36,36};
int Solar[4] =      {16,26,36,36};


//==================User configuration not required below this line ================================================

char str [256], s [70];
String graphData;
int device, rssi, sensorValues[4], sensorTypes[4];
float voltage;
uint8_t mac[6];
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
  if (receivedTopic == "command")
  {
    mac[0] = atoi(&payload[0]);mac[1] = atoi(&payload[3]);mac[2] = atoi(&payload[6]);mac[3] = atoi(&payload[9]);mac[4] = atoi(&payload[12]);mac[5] = atoi(&payload[15]);  
  }
}

void sendCommand()  {
  esp_wifi_set_mac(ESP_IF_WIFI_AP, mac);
  Serial.print("Command sent to remote device :  ");Serial.print(mac[0]);Serial.print("/");Serial.print(mac[1]);Serial.print("/");Serial.print(mac[2]);Serial.print("/");Serial.print(mac[3]);Serial.print("/");Serial.print(mac[4]);Serial.print("/");Serial.print(mac[5]);Serial.println("/");
 }

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
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
  
  Serial.print(F("*CONNECTED* IP: "));Serial.println(WiFi.localIP());
 
  configTime(0, 0, ntpServer);
  epoch = getTime();
  Serial.print("Epoch Time: ");Serial.println(epoch);
  delay(500);
  Epoch = String(epoch);

  LITTLEFS.begin();
  
  webserver.addHandler(new SPIFFSEditor(MYFS, http_username,http_password));
  webserver.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
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
              //sendCommand();

               if (device == 06) { for (int i = 0; i < 4; i++) sensorTypes[i] = Livingroom[i];} 
               if (device == 16) { for (int i = 0; i < 4; i++) sensorTypes[i] = Kitchen[i];}
               if (device == 26) { for (int i = 0; i < 4; i++) sensorTypes[i] = Bedroom1[i];} 
               if (device == 36) { for (int i = 0; i < 4; i++) sensorTypes[i] = Bedroom2[i];}
               if (device == 46) { for (int i = 0; i < 4; i++) sensorTypes[i] = Bedroom3[i];} 
               if (device == 56) { for (int i = 0; i < 4; i++) sensorTypes[i] = Bedroom4[i];}
               if (device == 66) { for (int i = 0; i < 4; i++) sensorTypes[i] = Bathroom1[i];} 
               if (device == 76) { for (int i = 0; i < 4; i++) sensorTypes[i] = Bathroom2[i];}
               if (device == 86) { for (int i = 0; i < 4; i++) sensorTypes[i] = Bathroom3[i];} 
               if (device == 96) { for (int i = 0; i < 4; i++) sensorTypes[i] = Bathroom4[i];}
               if (device == 106) { for (int i = 0; i < 4; i++) sensorTypes[i] = Laundry[i];} 
               if (device == 116) { for (int i = 0; i < 4; i++) sensorTypes[i] = Boiler[i];}
               if (device == 126) { for (int i = 0; i < 4; i++) sensorTypes[i] = Workshop[i];} 
               if (device == 136) { for (int i = 0; i < 4; i++) sensorTypes[i] = Garage[i];}
               if (device == 146) { for (int i = 0; i < 4; i++) sensorTypes[i] = Office[i];}
               if (device == 156) { for (int i = 0; i < 4; i++) sensorTypes[i] = Tank[i];} 
               if (device == 166) { for (int i = 0; i < 4; i++) sensorTypes[i] = Solar[i];}
               
      device = info.ap_probereqrecved.mac[0];
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
              
      Serial.println();
      Serial.println("Following ## Sensor Values ## receiced from remote device  & published via MQTT: ");
      Serial.println(str);
      Serial.println();
      
      myClient.publish("sensor", str);

      epoch = getTime();
      Serial.print("Epoch Time: ");Serial.println(epoch); 
     
      graphData = ",";graphData += epoch;graphData += ",";graphData += device;graphData += ",";graphData += voltage;graphData += ",";graphData += rssi;graphData += ",";graphData += sensorValues[0];graphData += ",";graphData += sensorValues[1];graphData += ",";graphData += sensorValues[2];graphData += ",";graphData += sensorValues[3];graphData += "]";
     
      File f = LITTLEFS.open(dataFile, "r+"); // See https://github.com/lorol/LITTLEFS/issues/33
      Serial.print("File size: "); Serial.println(f.size());
      f.seek((f.size()-1), SeekSet);
      Serial.print("Position: "); Serial.println(f.position());
      f.print(graphData);Serial.println();
      Serial.print("Appended to file: "); Serial.println(graphData);
      Serial.print("File size: "); Serial.println(f.size());
      f.close(); 
    
    }

      if (voltage < 2.50) {      // if voltage of battery gets to low, print the warning below.
         myClient.publish("Warning/Battery Low", String(device));
   }
}
