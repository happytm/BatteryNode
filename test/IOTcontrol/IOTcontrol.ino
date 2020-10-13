// This example uses an ESP32 Development Board
// to connect to shiftr.io.
//
// You can check on your device after a successful
// connection here: broker.shiftr.io.
// Ports : MQTT  = 1883,  MQTTS = 8883, MQTTWS = 80,  MQTTWSS = 443.
// username = try, password = try. 
// by Joël Gähwiler
// https://github.com/256dpi/arduino-mqtt


#include <WiFi.h>
#include <esp_wifi.h>
#include <MQTT.h>      // https://github.com/256dpi/arduino-mqtt
//#include "SPIFFS.h"


#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#define FILESYSTEM SPIFFS
// You only need to format the filesystem once
#define FORMAT_FILESYSTEM false
#define DBG_OUTPUT_PORT Serial

#if FILESYSTEM == FFat
#include <FFat.h>
#endif
#if FILESYSTEM == SPIFFS
#include <SPIFFS.h>
#endif


char PublishToMQTT [256];
char SaveSensorValues[256];
char SaveDeviceStatus[256];
char s [60];

//   Your WiFi & MQTT config below:

char* room = "Livingroom";  // Needed for person locator.Each location must run probeReceiver sketch to implement person locator.
int rssiThreshold = -50; // Adjust according to signal strength by trial & error.
const char* apSSID = "ESP";
const char* apPassword = "";
const int apChannel = 7;
const int hidden = 0; // If hidden is 1 probe request event handling does not work ?

char ssid[] = "HAPPYHOME";     // your network SSID (name)
char password[] = "kb1henna"; // your network password

const char* host = "esp32fs";
WebServer server(80);
//holds the current upload
File fsUploadFile;

int device;
float voltage;
uint8_t data[12];
const char* location;

int sensorValue1; int sensorValue2; int sensorValue3; int sensorValue4; int sensorValue5;
int command1; int command2;  int command3;  int command4;  int command5; int command6;
uint8_t mac[6] = {static_cast<uint8_t>(command1), static_cast<uint8_t>(command2), static_cast<uint8_t>(command3), static_cast<uint8_t>(command4), static_cast<uint8_t>(command5), static_cast<uint8_t>(command6)};
char topic1[50]; char topic2[50]; char topic3[50]; char topic4[50]; char topic5[50]; char topic6[50]; // char topic7[50]; char topic8[50]; char topic9[50]; char topic10[50]; char topic11[50]; char topic12[50];
const char* sensorType1; const char* sensorType2; const char* sensorType3; const char* sensorType4;
int deviceStatus1; int deviceStatus2; int deviceStatus3;  int deviceStatus4; int deviceStatus5;
const char* statusType1 = "rssi"; const char *statusType2 = "mode"; const char *statusType3 = "ip"; const char *statusType4 = "channel"; const char *statusType5 = "sleeptime"; const char *statusType6 = "uptime";

//uint8_t securityCode[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Security code must be same at remote sensors to compare.
uint8_t PresencePerson1[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Mac ID of Cell phone #1.
uint8_t PresencePerson2[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Mac ID of Cell phone #2.
uint8_t PresencePerson3[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Mac ID of Cell phone #3.
uint8_t PresencePerson4[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Mac ID of Cell phone #4.

// ==================== end of TUNEABLE PARAMETERS ====================


//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

String getContentType(String filename) {
  if (server.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".scss")) {
    return "text/scss";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".json")) {
    return "text/json";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}

bool exists(String path) {
  bool yes = false;
  File file = FILESYSTEM.open(path, "r");
  if (!file.isDirectory()) {
    yes = true;
  }
  file.close();
  return yes;
}

bool handleFileRead(String path) {
  DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  if (path.endsWith("/")) {
    path += "index.htm";
  }
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (exists(pathWithGz) || exists(path)) {
    if (exists(pathWithGz)) {
      path += ".gz";
    }
    File file = FILESYSTEM.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload() {
  if (server.uri() != "/edit") {
    return;
  }
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    DBG_OUTPUT_PORT.print("handleFileUpload Name: "); DBG_OUTPUT_PORT.println(filename);
    fsUploadFile = FILESYSTEM.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    //DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
    if (fsUploadFile) {
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {
      fsUploadFile.close();
    }
    DBG_OUTPUT_PORT.print("handleFileUpload Size: "); DBG_OUTPUT_PORT.println(upload.totalSize);
  }
}

void handleFileDelete() {
  if (server.args() == 0) {
    return server.send(500, "text/plain", "BAD ARGS");
  }
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
  if (path == "/") {
    return server.send(500, "text/plain", "BAD PATH");
  }
  if (!exists(path)) {
    return server.send(404, "text/plain", "FileNotFound");
  }
  FILESYSTEM.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate() {
  if (server.args() == 0) {
    return server.send(500, "text/plain", "BAD ARGS");
  }
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileCreate: " + path);
  if (path == "/") {
    return server.send(500, "text/plain", "BAD PATH");
  }
  if (exists(path)) {
    return server.send(500, "text/plain", "FILE EXISTS");
  }
  File file = FILESYSTEM.open(path, "w");
  if (file) {
    file.close();
  } else {
    return server.send(500, "text/plain", "CREATE FAILED");
  }
  server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
  if (!server.hasArg("dir")) {
    server.send(500, "text/plain", "BAD ARGS");
    return;
  }

  String path = server.arg("dir");
  DBG_OUTPUT_PORT.println("handleFileList: " + path);


  File root = FILESYSTEM.open(path);
  path = String();

  String output = "[";
  if (root.isDirectory()) {
    File file = root.openNextFile();
    while (file) {
      if (output != "[") {
        output += ',';
      }
      output += "{\"type\":\"";
      output += (file.isDirectory()) ? "dir" : "file";
      output += "\",\"name\":\"";
      output += String(file.name()).substring(1);
      output += "\"}";
      file = root.openNextFile();
    }
  }
  output += "]";
  server.send(200, "text/json", output);
}

WiFiClient net;
MQTTClient client;

unsigned long lastMillis = 0;

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("arduino", "try", "try")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  client.subscribe("command");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("Message received with topic  = " + topic + " & payload = " + payload);

  if (topic == "command")   {
    command1 = atoi(&payload[0]);
    Serial.println(command1);
    command2 = atoi(&payload[3]);
    Serial.println(command2);
    command3 = atoi(&payload[6]);
    Serial.println(command3);
    command4 = atoi(&payload[9]);
    Serial.println(command4);
    command5 = atoi(&payload[12]);
    Serial.println(command5);
    command6 = atoi(&payload[15]);
    Serial.println(command6);

  }


  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  if (FORMAT_FILESYSTEM) FILESYSTEM.format();
  FILESYSTEM.begin();
  {
    File root = FILESYSTEM.open("/");
    File file = root.openNextFile();
    while (file) {
      String fileName = file.name();
      size_t fileSize = file.size();
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
      file = root.openNextFile();
    }
    Serial.printf("\n");
  }


  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address:  ");
  Serial.println(WiFi.localIP());
  
   //WiFi.mode(WIFI_AP);  // This causes problem for MQTT connection.
  WiFi.softAP(apSSID, apPassword, apChannel, hidden);
  esp_wifi_set_event_mask(WIFI_EVENT_MASK_NONE); // This line is must to activate probe request received event handler.
  Serial.printf("The AP mac address is %s\n", WiFi.softAPmacAddress().c_str());

  Serial.println("Connected to the WiFi network");

  WiFi.onEvent(probeRequest, SYSTEM_EVENT_AP_PROBEREQRECVED);

  Serial.println();
  Serial.println();
  Serial.println("Waiting for probe requests ... ");

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.

  client.begin("broker.shiftr.io", net); // Use username "try" and password "try" from Mqtt client to access this broker.
  //client.begin("192.168.0.3", net);
  client.onMessage(messageReceived);

  connect();

  MDNS.begin(host);
/*  
  Serial.print("Open http://");
  Serial.print(host);
  Serial.println(".local/edit to see the file browser");

  //SERVER INIT
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.htm")) {
      server.send(404, "text/plain", "FileNotFound");
    }
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);

  //called when the url is not defined here
  //use it to load content from FILESYSTEM
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "FileNotFound");
    }
  });

  //get heap status, analog input value and all GPIO statuses in one json call
  
  server.on("/all", HTTP_GET, []() {
    String json = "{";
    json += "\"heap\":" + String(ESP.getFreeHeap());
    json += ", \"analog\":" + String(analogRead(A0));
    json += ", \"gpio\":" + String((uint32_t)(0));
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });

  */
  server.begin();
  Serial.println("HTTP server started");



  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  /*
    //--------- Write to file
    File fileToWrite = SPIFFS.open("/sensorData.json", FILE_WRITE);

    if (!fileToWrite) {
      Serial.println("There was an error opening the file for writing");
      return;
    }

    if (fileToWrite.println(PublishToMQTT)) {
      Serial.println("File was written");;
    } else {
      Serial.println("File write failed");
    }

    fileToWrite.close();
  */

}  // End of setup


void loop()
{
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  server.handleClient();

  if (!client.connected()) {
    connect();
  }
}  // End of Loop

void sendCommand()  {

  mac[0] = command1;
  mac[1] = command2;
  mac[2] = command3;
  mac[3] = command4;
  mac[4] = command5;
  mac[5] = command6;

  Serial.print("Command sent to remote device :  ");
  Serial.print(mac[0]); Serial.print("/");
  Serial.print(mac[1]); Serial.print("/");
  Serial.print(mac[2]); Serial.print("/");
  Serial.print(mac[3]); Serial.print("/");
  Serial.print(mac[4]); Serial.print("/");
  Serial.print(mac[5]); Serial.println("/");
  Serial.println();
  Serial.println();
  
  esp_wifi_set_mac(ESP_IF_WIFI_AP, mac);  

}

void probeRequest(WiFiEvent_t event, WiFiEventInfo_t info) {

     
    
    Serial.print("Probe Received :  ");
    for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", info.ap_probereqrecved.mac[i]);
    if (i < 5)Serial.print(":");

  }

    Serial.println();
    Serial.println();
    Serial.println("Received probe request packet:");
    Serial.println("RSSI : " + String(info.ap_probereqrecved.rssi));
    Serial.println("MAC (HEX) : " + String(info.ap_probereqrecved.mac[0], HEX) + ":" + String(info.ap_probereqrecved.mac[1], HEX) + ":" + String(info.ap_probereqrecved.mac[2], HEX) + ":" + String(info.ap_probereqrecved.mac[3], HEX) + ":" + String(info.ap_probereqrecved.mac[4], HEX) + ":" + String(info.ap_probereqrecved.mac[5], HEX));
    Serial.println("MAC (DEC) : " + String(info.ap_probereqrecved.mac[0], DEC) + ":" + String(info.ap_probereqrecved.mac[1], DEC) + ":" + String(info.ap_probereqrecved.mac[2], DEC) + ":" + String(info.ap_probereqrecved.mac[3], DEC) + ":" + String(info.ap_probereqrecved.mac[4], DEC) + ":" + String(info.ap_probereqrecved.mac[5], DEC));
    Serial.println();
  
  
  if (info.ap_probereqrecved.mac[0] == PresencePerson1[0] && info.ap_probereqrecved.mac[1] == PresencePerson1[1] && info.ap_probereqrecved.mac[2] == PresencePerson1[2]) { // write code to match MAC ID of cell phone to predefined variable and store presence/absense in new variable.
    Serial.println("################ Person 1 arrived ###################### ");
    client.publish("Sensordata/Person1/", "Home");
    Serial.print("Signal Strength of remote sensor: ");
    Serial.println(info.ap_probereqrecved.rssi);
    client.publish("Sensordata/Signal/", (String)info.ap_probereqrecved.rssi);

    if (info.ap_probereqrecved.rssi > rssiThreshold) // Adjust according to signal strength by trial & error.
    { // write code to match MAC ID of cell phone to predefined variable and store presence/absense in new variable.
      client.publish("Sensordata/Person1/in/", room);

    }
  }

     sendCommand();  // Investigate frequent reboots if activated.

  if (info.ap_probereqrecved.mac[0] == 6 || info.ap_probereqrecved.mac[0] == 16 || info.ap_probereqrecved.mac[0] == 26 || info.ap_probereqrecved.mac[0] == 36 || info.ap_probereqrecved.mac[0] == 46 || info.ap_probereqrecved.mac[0] == 56 || info.ap_probereqrecved.mac[0] == 66 || info.ap_probereqrecved.mac[0] == 76 || info.ap_probereqrecved.mac[0] == 86 || info.ap_probereqrecved.mac[0] == 96 || info.ap_probereqrecved.mac[0] == 106 || info.ap_probereqrecved.mac[0] == 116 || info.ap_probereqrecved.mac[0] == 126 || info.ap_probereqrecved.mac[0] == 136 || info.ap_probereqrecved.mac[0] == 146 || info.ap_probereqrecved.mac[0] == 156 || info.ap_probereqrecved.mac[0] == 166 || info.ap_probereqrecved.mac[0] == 176 || info.ap_probereqrecved.mac[0] == 186 || info.ap_probereqrecved.mac[0] == 196 || info.ap_probereqrecved.mac[0] == 206 || info.ap_probereqrecved.mac[0] == 216 || info.ap_probereqrecved.mac[0] == 226 || info.ap_probereqrecved.mac[0] == 236 || info.ap_probereqrecved.mac[0] == 246) // only accept data from certain devices.
  {


    if (info.ap_probereqrecved.mac[1] == 06) { // only accept data from device with voltage as a sensor type at byte 1.
        
      
      if (info.ap_probereqrecved.mac[2] == 16) sensorType1 = "temperature";
      if (info.ap_probereqrecved.mac[2] == 26) sensorType1 = "humidity";
      if (info.ap_probereqrecved.mac[2] == 36) sensorType1 = "pressure";
      if (info.ap_probereqrecved.mac[2] == 46) sensorType1 = "light";
      if (info.ap_probereqrecved.mac[2] == 56) sensorType1 = "openclose";
      if (info.ap_probereqrecved.mac[2] == 66) sensorType1 = "level";
      if (info.ap_probereqrecved.mac[2] == 76) sensorType1 = "presence";
      if (info.ap_probereqrecved.mac[2] == 86) sensorType1 = "motion";
      if (info.ap_probereqrecved.mac[2] == 96) sensorType1 = "custom";

      if (info.ap_probereqrecved.mac[3] == 16) sensorType2 = "temperature";
      if (info.ap_probereqrecved.mac[3] == 26) sensorType2 = "humidity";
      if (info.ap_probereqrecved.mac[3] == 36) sensorType2 = "pressure";
      if (info.ap_probereqrecved.mac[3] == 46) sensorType2 = "light";
      if (info.ap_probereqrecved.mac[3] == 56) sensorType2 = "openclose";
      if (info.ap_probereqrecved.mac[3] == 66) sensorType2 = "level";
      if (info.ap_probereqrecved.mac[3] == 76) sensorType2 = "presence";
      if (info.ap_probereqrecved.mac[3] == 86) sensorType2 = "motion";
      if (info.ap_probereqrecved.mac[3] == 96) sensorType2 = "custom";

      if (info.ap_probereqrecved.mac[4] == 16) sensorType3 = "temperature";
      if (info.ap_probereqrecved.mac[4] == 26) sensorType3 = "humidity";
      if (info.ap_probereqrecved.mac[4] == 36) sensorType3 = "pressure";
      if (info.ap_probereqrecved.mac[4] == 46) sensorType3 = "light";
      if (info.ap_probereqrecved.mac[4] == 56) sensorType3 = "openclose";
      if (info.ap_probereqrecved.mac[4] == 66) sensorType3 = "level";
      if (info.ap_probereqrecved.mac[4] == 76) sensorType3 = "presence";
      if (info.ap_probereqrecved.mac[4] == 86) sensorType3 = "motion";
      if (info.ap_probereqrecved.mac[4] == 96) sensorType3 = "custom";

      if (info.ap_probereqrecved.mac[5] == 16) sensorType4 = "temperature";
      if (info.ap_probereqrecved.mac[5] == 26) sensorType4 = "humidity";
      if (info.ap_probereqrecved.mac[5] == 36) sensorType4 = "pressure";
      if (info.ap_probereqrecved.mac[5] == 46) sensorType4 = "light";
      if (info.ap_probereqrecved.mac[5] == 56) sensorType4 = "openclose";
      if (info.ap_probereqrecved.mac[5] == 66) sensorType4 = "level";
      if (info.ap_probereqrecved.mac[5] == 76) sensorType4 = "presence";
      if (info.ap_probereqrecved.mac[5] == 86) sensorType4 = "motion";
      if (info.ap_probereqrecved.mac[5] == 96) sensorType4 = "custom";

    } else {

      device = info.ap_probereqrecved.mac[0];

      if (device == 06) location = "Livingroom";
      if (device == 16) location = "Kitchen";
      if (device == 26) location = "Bedroom1";
      if (device == 36) location = "Bedroom2";
      if (device == 46) location = "Bedroom3";
      if (device == 56) location = "Bedroom4";
      if (device == 66) location = "Bathroom1";
      if (device == 76) location = "Bathroom2";
      if (device == 86) location = "Bathroom3";
      if (device == 96) location = "Bathroom4";
      if (device == 106) location = "Laudry";
      if (device == 116) location = "Boiler room";
      if (device == 126) location = "Workshop";
      if (device == 136) location = "Garage";
      if (device == 146) location = "Water Tank";
      if (device == 156) location = "Solar Tracker";

      voltage = info.ap_probereqrecved.mac[1];
      voltage = voltage * 2 / 100;

      sensorValue1 = info.ap_probereqrecved.mac[2];
      sensorValue2 = info.ap_probereqrecved.mac[3];

      //  if (sensorType4 == "pressure"){
      //      sensorValue3 = info.ap_probereqrecved.mac[4];
      //     sensorValue3 = sensorValue3 * 4;
      // } else {
      sensorValue3 = info.ap_probereqrecved.mac[4];
      //}
      sensorValue4 = info.ap_probereqrecved.mac[5];
    }

    if (voltage > 2.50 && voltage < 3.60) {

      

      sprintf (PublishToMQTT, "{");
      sprintf (s, "\"%s\":\"%s\"", "location", location);    strcat (PublishToMQTT, s);
      sprintf (s, ",\"%s\":\"%.2f\"", "voltage", voltage);    strcat (PublishToMQTT, s);
      sprintf (s, ",\"%s\":\"%d\"", sensorType1, sensorValue1); strcat (PublishToMQTT, s);
      sprintf (s, ",\"%s\":\"%d\"", sensorType2, sensorValue2); strcat (PublishToMQTT, s);
      sprintf (s, ",\"%s\":\"%d\"", sensorType3, sensorValue3); strcat (PublishToMQTT, s);
      sprintf (s, ",\"%s\":\"%d\"", sensorType4, sensorValue4); strcat (PublishToMQTT, s);
      sprintf (s, "}"); strcat (PublishToMQTT, s);

      Serial.println("Following ## Sensor Values ## receiced from remote device  & published via MQTT: ");
      Serial.println();
      Serial.println(PublishToMQTT);
      Serial.println();
      client.publish("SensorValues", String(PublishToMQTT));

      strcat (SaveSensorValues, PublishToMQTT);
      sprintf (SaveSensorValues, "%s%s", SaveSensorValues, ",");

      File fileToAppend = SPIFFS.open("/sensorData.json", FILE_APPEND);

      if (!fileToAppend) {
        Serial.println("There was an error opening the file for appending");
        return;
      }

      if (fileToAppend.println((SaveSensorValues))) {
        Serial.println("Sensor Values were Saved to Spiffs");

      } else {
        Serial.println("File append failed");
      }

      fileToAppend.close();

      if (voltage < 2.50) {      // if voltage of battery gets to low, print the warning below.
        //  myBroker.publish("Warning/Battery Low", location);
      }
    }

    if (info.ap_probereqrecved.mac[3] == apChannel) {

      deviceStatus1 = (info.ap_probereqrecved.mac[1]);
      deviceStatus2 = (info.ap_probereqrecved.mac[2]);
      deviceStatus3 = (info.ap_probereqrecved.mac[3]);
      deviceStatus4 = (info.ap_probereqrecved.mac[4]);
      deviceStatus5 = (info.ap_probereqrecved.mac[5]);

      sprintf (PublishToMQTT, "{");
      sprintf (s, "\"%s\":\"%s\"", "location", location);    strcat (PublishToMQTT, s);
      sprintf (s, ",\"%s\":\"%i\"", statusType1, info.ap_probereqrecved.rssi); strcat (PublishToMQTT, s);
      sprintf (s, ",\"%s\":\"%d\"", statusType2, deviceStatus1); strcat (PublishToMQTT, s);
      sprintf (s, ",\"%s\":\"%d\"", statusType3, deviceStatus2); strcat (PublishToMQTT, s);
      sprintf (s, ",\"%s\":\"%d\"", statusType4, deviceStatus3); strcat (PublishToMQTT, s);
      sprintf (s, ",\"%s\":\"%d\"", statusType5, deviceStatus4); strcat (PublishToMQTT, s);
      sprintf (s, ",\"%s\":\"%d\"", statusType6, deviceStatus5); strcat (PublishToMQTT, s);
      sprintf (s, "}"); strcat (PublishToMQTT, s);

      Serial.println("Following ## Device Status ## receiced from remote device & published via MQTT: ");
      Serial.println();
      Serial.println(PublishToMQTT);
      Serial.println();

      client.publish("deviceStatus", String(PublishToMQTT));

      strcat (SaveDeviceStatus, PublishToMQTT);
      sprintf (SaveDeviceStatus, "%s%s", SaveDeviceStatus, ",");
      File fileToAppend = SPIFFS.open("/sensorData.json", FILE_APPEND);

      if (!fileToAppend) {
        Serial.println("There was an error opening the file for appending");
        return;
      }

      if (fileToAppend.println((SaveDeviceStatus))) {
        Serial.println("Device Status was saved to Spiffs");

      } else {
        Serial.println("File append failed");
      }

      fileToAppend.close();
    }
  }
}
