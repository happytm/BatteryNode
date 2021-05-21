// Based on https://github.com/sascha432/ESPAsyncWebServer
// To do : 
#define PROBEREQUESTS     true
#define MQTT              true
#define APPENDTOSPIFFS    true // If false stops appending to file but still shows file size.
#define STANDALONE        true // To do RTC manual time input 

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <FS.h>
#include <SPIFFS.h>
#include <SPIFFSEditor.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include "time.h"

const char* ssid = "";
const char* password = "";
const char* apSSID = "ESP";
const char* apPassword = "";
const int apChannel = 7;
const int hidden = 0; // If hidden is 1 probe request event handling does not work ?

const char* http_username = "admin";
const char* http_password = "admin";

#if PROBEREQUESTS
#include <esp_wifi.h>
#endif

#if MQTT
#include <TinyMqtt.h>   // Thanks to https://github.com/hsaturn/TinyMqtt
std::string sentTopic = "data";
std::string receivedTopic = "command";
MqttBroker broker(1883);
MqttClient myClient(&broker);
#endif

//#define MYFS SPIFFS

const char* ntpServer = "pool.ntp.org";
unsigned long epoch; 

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
 
char str [256];
char s [70];
String deviceData;
String sensorData;
int device;
float voltage;
uint8_t mac[6];
int sensorValues[4];
int sensorTypes[4];
int deviceStatus[4];

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

#if PROBEREQUESTS
#include <esp_wifi.h>
#endif

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(type == WS_EVT_CONNECT){
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->printf("You are connected Client %u :)", client->id());
    client->ping();
  } else if(type == WS_EVT_DISCONNECT){
    Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
  } else if(type == WS_EVT_ERROR){
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if(type == WS_EVT_PONG){
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if(type == WS_EVT_DATA){
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    String msg = "";
    if(info->final && info->index == 0 && info->len == len){
      //the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      Serial.printf("%s\n",msg.c_str());

      if(info->opcode == WS_TEXT)
        
        client->text(msg);
      else
        client->binary("I got your binary message");
    } else {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if(info->index == 0){
        if(info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT)?"text":"binary", info->index, info->index + len);

      if(info->opcode == WS_TEXT){
        for(size_t i=0; i < len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for(size_t i=0; i < len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      Serial.printf("%s\n",msg.c_str());

      if((info->index + len) == info->len){
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if(info->final){
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT)?"text":"binary");
          if(info->message_opcode == WS_TEXT)
            client->text(msg);
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}

void receivedMessage(const MqttClient* source, const Topic& topic, const char* payload, size_t length)
{
  Serial << endl << "Received message on topic " << receivedTopic.c_str() << " with payload " << payload << endl;
  
  if (receivedTopic == "command")
  {
    mac[0] = atoi(&payload[0]);
    mac[1] = atoi(&payload[3]);
    mac[2] = atoi(&payload[6]);
    mac[3] = atoi(&payload[9]);
    mac[4] = atoi(&payload[12]);
    mac[5] = atoi(&payload[15]);
    
  }
}

void sendCommand()  {

  esp_wifi_set_mac(ESP_IF_WIFI_AP, mac);
  Serial << "Command sent to remote device :  " << mac[0] << "/" << mac[1] << "/" << mac[2] << "/" << mac[3] << "/" << mac[4] << "/" << mac[5] << "/" << endl;
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
  
  Serial.print(F("*CONNECTED* IP:"));
  Serial.println(WiFi.localIP());

  configTime(0, 0, ntpServer);
  epoch = getTime();
  Serial.print("Epoch Time: ");
  Serial.println(epoch);
  
  //Send OTA events to the browser
  ArduinoOTA.onStart([]() { events.send("Update Start", "ota"); });
  ArduinoOTA.onEnd([]() { events.send("Update End", "ota"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    char p[32];
    sprintf(p, "Progress: %u%%\n", (progress/(total/100)));
    events.send(p, "ota");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if(error == OTA_AUTH_ERROR) events.send("Auth Failed", "ota");
    else if(error == OTA_BEGIN_ERROR) events.send("Begin Failed", "ota");
    else if(error == OTA_CONNECT_ERROR) events.send("Connect Failed", "ota");
    else if(error == OTA_RECEIVE_ERROR) events.send("Recieve Failed", "ota");
    else if(error == OTA_END_ERROR) events.send("End Failed", "ota");
  });
  ArduinoOTA.setHostname(apSSID);
  ArduinoOTA.begin();

  MDNS.addService("http","tcp",80);


  if (SPIFFS.begin()) {

    Serial.print(F("FS mounted\n"));
  } else {
    Serial.print(F("FS mount failed\n"));  
  }

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  events.onConnect([](AsyncEventSourceClient *client){
    client->send("hello!",NULL,millis(),1000);
  });
  server.addHandler(&events);

  server.addHandler(new SPIFFSEditor(SPIFFS, http_username,http_password));

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");

  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      Serial.printf("GET");
    else if(request->method() == HTTP_POST)
      Serial.printf("POST");
    else if(request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if(request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if(request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if(request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if(request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
      Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

      int headers = request->headers();
      int i;
      for(i=0;i<headers;i++){
        AsyncWebHeader* h = request->getHeader(i);
        Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

      int params = request->params();
      for(i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isFile()){
          Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
        } else if(p->isPost()){
          Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        } else {
          Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });
  server.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index)
      Serial.printf("UploadStart: %s\n", filename.c_str());
      Serial.printf("%s", (const char*)data);
      if(final)
      Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if(!index)
      Serial.printf("BodyStart: %u\n", total);
      Serial.printf("%s", (const char*)data);
      if(index + len == total)
      Serial.printf("BodyEnd: %u\n", total);
  });
  
  //Followin line must be added before server.begin() to allow local lan request see : https://github.com/me-no-dev/ESPAsyncWebServer/issues/726
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  server.begin();

#if MQTT
    broker.begin();

  // ============= Client Subscribe ================
    myClient.setCallback(receivedMessage);
    myClient.subscribe(receivedTopic);
    myClient.subscribe(sentTopic);
#endif

#if PROBEREQUESTS
    WiFi.onEvent(probeRequest, SYSTEM_EVENT_AP_PROBEREQRECVED);
//    Serial << "Waiting for probe requests ... " << endl;
#endif

#if APPENDTOSPIFFS
  /*
  // --------Only for first time to create first line of file for column headings-----------
  // Comment out following whole block after first use.
  //--------- Write to file
  File fileToWrite = SPIFFS.open("/sensors.csv", FILE_WRITE);

  if (!fileToWrite) {
    Serial.println("There was an error opening the file for writing");
    return;
  }
  String columns = "epoch,location,voltage,rssi,temperature,humidity,pressure,light";
  if (fileToWrite.println(columns)) {
    Serial.println("File was written");;
  } else {
    Serial.println("File write failed");
  }

  fileToWrite.close();
  //----------------------------------------------------------------------------------------------
*/
#endif

} // End of setup

void loop(){
  ArduinoOTA.handle();
  ws.cleanupClients();
  
}  // End of loop

#if PROBEREQUESTS

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

      if (info.ap_probereqrecved.mac[1] == 06) 
      { // only accept data from device with voltage as a sensor type at byte 1.
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
               deviceStatus[0] = info.ap_probereqrecved.mac[2];
               deviceStatus[1] = info.ap_probereqrecved.mac[3];
               deviceStatus[2] = info.ap_probereqrecved.mac[4];
               deviceStatus[3] = info.ap_probereqrecved.mac[5];
      } else {

      device = info.ap_probereqrecved.mac[0];
               
      voltage = info.ap_probereqrecved.mac[1];
      voltage = voltage * 2 / 100;
                  
      sensorValues[0] = info.ap_probereqrecved.mac[2];
      sensorValues[1] = info.ap_probereqrecved.mac[3];
      sensorValues[2] = info.ap_probereqrecved.mac[4];
      sensorValues[3] = info.ap_probereqrecved.mac[5];
              
      }

      if (voltage > 2.50 && voltage < 3.60) 
      {
      
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
      /*
      sensorData = (",[" + Date + "," + Time + "," + String(info.ap_probereqrecved.mac[0], DEC) + "," + String(info.ap_probereqrecved.mac[1], DEC) + "," + String(info.ap_probereqrecved.mac[2], DEC) + "," + String(info.ap_probereqrecved.mac[3], DEC) + "," + String(info.ap_probereqrecved.mac[4], DEC) + "," + String(info.ap_probereqrecved.mac[5], DEC) + "]");
      Serial.println();
      Serial.print("Received Sensor data: "); 
      Serial.println(sensorData);
      Serial.println();
      
     // myClient.publish("Sensor", sensorData);
     // myClient.publish("Sensor", str);
      */
      #if APPENDTOSPIFFS
      File fileToAppend = SPIFFS.open("/sensors.csv", FILE_APPEND);

      if (!fileToAppend) {
        Serial.println("There was an error opening the file for appending");
        return;
      }
      epoch = getTime();
      String a = (String(epoch) + "," + String(info.ap_probereqrecved.mac[0], DEC) + "," + String(info.ap_probereqrecved.mac[1], DEC) + "," + String(info.ap_probereqrecved.rssi) + "," + String(sensorValues[0], DEC) + "," + String(sensorValues[1], DEC) + "," + String(sensorValues[2], DEC) + "," + String(sensorValues[3], DEC));
      
      if (fileToAppend.println(a)) {
        Serial.println("File content was appended");
      } else {
        Serial.println("File append failed");
      }

      fileToAppend.close();
      #endif
      
      File fileToTest = SPIFFS.open("/sensors.csv");

      if (!fileToTest) {
        Serial.println("Failed to open file for reading");
        return;
      }

      Serial.print("Total File size: ");
      Serial.println(fileToTest.size());

      fileToTest.close();
      
      
      if (voltage < 2.50) {      // if voltage of battery gets to low, print the warning below.
         //myClient.publish("Warning/Battery Low", location);
       }
     }

   if (info.ap_probereqrecved.mac[3] == apChannel) {
     
     sprintf (str, "{");
     sprintf (s, "\"%s\":\"%i\"", "Location", device);    strcat (str, s);
     sprintf (s, ",\"%s\":\"%i\"", "RSSI", info.ap_probereqrecved.rssi); strcat (str, s);
     sprintf (s, ",\"%s\":\"%i\"", "MODE", deviceStatus[0]); strcat (str, s);
     sprintf (s, ",\"%s\":\"%i\"", "CHANNEL", deviceStatus[1]); strcat (str, s);
     sprintf (s, ",\"%s\":\"%i\"", "IP", deviceStatus[2]); strcat (str, s);
     sprintf (s, ",\"%s\":\"%i\"", "Sleeptime", deviceStatus[3]); strcat (str, s);
     sprintf (s, "}"); strcat (str, s);
                           
     Serial.println();
     Serial.println("Following ## Device Status ## receiced from remote device & published via MQTT: ");
     Serial.println(str);
     Serial.println();
     /*
     deviceData = (",[" + Date + "," + Time + ","  + String(info.ap_probereqrecved.mac[0], DEC) + "," + String(info.ap_probereqrecved.mac[1], DEC) + "," + String(info.ap_probereqrecved.mac[2], DEC) + "," + String(info.ap_probereqrecved.mac[3], DEC) + "," + String(info.ap_probereqrecved.mac[4], DEC) + "," + String(info.ap_probereqrecved.mac[5], DEC) + "]");
     Serial.print("Received Status data : "); 
     Serial.println(deviceData);
     Serial.println();
                      
     //myClient.publish("Device", deviceData);
     //myClient.publish("Device", str);
      
      #if APPENDTOSPIFFS
      File fileToAppend = SPIFFS.open("/sensors.csv", FILE_APPEND);

      if (!fileToAppend) {
        Serial.println("There was an error opening the file for appending");
        return;
      }

      if (fileToAppend.println(str)) {
        Serial.println("File content was appended");
      } else {
        Serial.println("File append failed");
      }

      fileToAppend.close();
      #endif
      */
      File fileToTest = SPIFFS.open("/sensors.csv");

      if (!fileToTest) {
        Serial.println("Failed to open file for reading");
        return;
      }

      Serial.print("Total File size: ");
      Serial.println(fileToTest.size());

      fileToTest.close();
      
   }
  }
}
#endif     
