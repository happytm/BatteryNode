#include <WiFi.h>
#include <esp_wifi.h>
#include <EEPROM.h>
#include "FS.h"
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PicoMQTT.h>
#include <PicoWebsocket.h>
//#include <motionDetector.h> // Manually install this library from https://github.com/happytm/MotionDetector/tree/main/motionDetector
#include "time.h"

AsyncWebServer server(80);WiFiServer websocket_underlying_server(81);WiFiServer tcp_server(1883);
PicoWebsocket::Server<::WiFiServer> websocket_server(websocket_underlying_server);PicoMQTT::Server mqtt(tcp_server, websocket_server);

//========================================================================================================================//
//              USER-SPECIFIED VARIABLES                                                                                  //
//========================================================================================================================//

int sensitivity = 30;  // Adujust sensitivity of motion sensor.
int sampleBufSize = 64, AvgSize = 32, varThreshold = 3, varIntegratorLimit = 3; // Tweak according to requirement.

//==================User configuration not required below this line =============================================

struct tm timeinfo;
#define MY_TZ "EST5EDT,M3.2.0,M11.1.0" //(New York) https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

const char* apSSID = "ESP"; const char* apPassword = ""; const int apChannel = 6; const int hidden = 0;                   // If hidden is 0 change show hidden to true in remote code.

String dataFile = "/data.json";  // File to store sensor data.

char str [256], s [70];
String ssid,password, graphData, Hour, Minute;
int device, rssi, sleepTime, lastLevel, sensorValues[4], sensorTypes[4]; 
float voltage;
uint8_t mac[6],receivedCommand[6],showConfig[256];
const char* ntpServer = "pool.ntp.org";
unsigned long epoch, currentMillis, lastMillis, lastDetected;

unsigned long getTime() {time_t now;if (!getLocalTime(&timeinfo)) {Serial.println("Failed to obtain time");return(0);}Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");time(&now);return now;}

// ==== MOTION DETECTOR SETTINGS ====
#define MAX_sampleSize 256
#define MAX_AVERAGEBUFFERSIZE 64
#define MAX_VARIANCE 65535
#define ABSOLUTE_RSSI_LIMIT -100

int *sampleBuffer = nullptr, *averageBuffer = nullptr, *varianceBuffer = nullptr;

int sampleSize = MAX_sampleSize;
int averageFilterSize = MAX_sampleSize;
int averageBufferSize = MAX_AVERAGEBUFFERSIZE;
int average = 0;
int averageTemp = 0;
int sampleBufferIndex = 0;
int sampleBufferValid = 0;
int averageBufferIndex = 0;
int averageBufferValid = 0;
int variance = 0;
int variancePrev = 0;
int varianceSample = 0;
int varianceAR = 0;
int varianceIntegral = 0;
int varianceThreshold = 3;
int varianceIntegratorLimitMax = MAX_sampleSize;
int varianceIntegratorLimit = 3;
int varianceBufferSize = MAX_sampleSize;
int detectionLevel = 0;
int varianceBufferIndex = 0;
int varianceBufferValid = 0;
int enableCSVout = 0;
int minimumRSSI = ABSOLUTE_RSSI_LIMIT;

//========================================================================================================================//
//                FUNCTIONS                                                                                               //
//========================================================================================================================//


// Had to move all functions above setup function to compile the sketch successfully.
// See: https://forum.arduino.cc/t/exit-status-1-was-not-declared-in-this-scope-error-message/632717

void timeSynch(){ if (mac[1] == 105 || mac[1] == 106) {return;}else{epoch = getTime();Serial.print("Epoch Time: ");Serial.println(epoch); Hour = timeinfo.tm_hour; mac[4] = Hour.toInt();Minute = timeinfo.tm_min; mac[5] = Minute.toInt();Serial.print("Time Sent to remote device ");Serial.print(device);Serial.print("  ");Serial.print(Hour); Serial.print(":"); Serial.println(Minute);}}

void probeRequest(WiFiEvent_t event, WiFiEventInfo_t info) 
{ 
  Serial.println();
  Serial.print("Probe Received :  ");for (int i = 0; i < 6; i++) {Serial.printf("%02X", info.wifi_ap_probereqrecved.mac[i]);if (i < 5)Serial.print(":");}Serial.println();
  Serial.print("Connect at IP: ");Serial.print(WiFi.localIP()); Serial.println(" to monitor and control whole network");
  
  // Allow data from device ID ending in number 6, voltage value between 2.4V and 3.6V. and signal is stonger than -70.
  for (int i = 6; i < 256; i = i+10) if (info.wifi_ap_probereqrecved.mac[0] == i )
  {
    if (info.wifi_ap_probereqrecved.mac[1] > 120 && info.wifi_ap_probereqrecved.mac[1] < 180)
    {
      // This helps reduce interference from unknown devices from far away with weak signals.
      if (info.wifi_ap_probereqrecved.rssi > -70)  
      {
      
      device = info.wifi_ap_probereqrecved.mac[0];
            
      
      for (int i = 0; i < 6; i++) mac[i] = showConfig[i+device];           // Prepare command to be sent to remote device.
      for (int j = 0; j < 4; j++) sensorTypes[j] = showConfig[j+device+6]; // Assign sensor types to the particular device.
      Serial.println("Contents of command data saved in EEPROM for this device: "); EEPROM.readBytes(0, showConfig,256);for(int i=0;i<10;i++){Serial.printf("%d ", showConfig[i+device]);} Serial.println();
             
      timeSynch();
      if (mac[1] == 0 || mac[1] == 255) {mac[0] = device; mac[1] = 107; mac[2] = apChannel; timeSynch();}
                     
      esp_err_t err = esp_wifi_set_mac(WIFI_IF_AP, &mac[0]);  //https://randomnerdtutorials.com/get-change-esp32-esp8266-mac-address-arduino/ https://github.com/justcallmekoko/ESP32Marauder/issues/418
      Serial.print("Command sent to remote device :  "); for (int i = 0; i < 6; i++) { Serial.print(mac[i]);} Serial.println();        
                
      rssi = info.wifi_ap_probereqrecved.rssi;         
      voltage = info.wifi_ap_probereqrecved.mac[1];
      voltage = voltage * 2 / 100;
      sensorValues[0] = info.wifi_ap_probereqrecved.mac[2];sensorValues[1] = info.wifi_ap_probereqrecved.mac[3];sensorValues[2] = info.wifi_ap_probereqrecved.mac[4];sensorValues[3] = info.wifi_ap_probereqrecved.mac[5];
            
      sprintf (str, "{");sprintf (s, "\"%s\":\"%i\"", "Location", device);strcat (str, s);sprintf (s, ",\"%s\":\"%.2f\"", "Voltage", voltage);strcat (str, s);sprintf (s, ",\"%s\":\"%i\"", "RSSI", rssi);strcat (str, s);sprintf (s, ",\"%i\":\"%i\"", sensorTypes[0], sensorValues[0]); strcat (str, s);sprintf (s, ",\"%i\":\"%i\"", sensorTypes[1], sensorValues[1]); strcat (str, s);sprintf (s, ",\"%i\":\"%i\"", sensorTypes[2], sensorValues[2]); strcat (str, s);sprintf (s, ",\"%i\":\"%i\"", sensorTypes[3], sensorValues[3]); strcat (str, s);sprintf (s, "}"); strcat (str, s);
                    
      Serial.println("Following ## Sensor Values ## receiced from remote device  & published via MQTT: ");Serial.println(str);
      
      mqtt.publish("Sensor Values", str); 
      
      graphData = ",";graphData += epoch;graphData += ",";graphData += device;graphData += ",";graphData += voltage;graphData += ",";graphData += rssi;graphData += ",";graphData += sensorTypes[0];graphData += ",";graphData += sensorValues[0];graphData += ",";graphData += sensorTypes[1];graphData += ",";graphData += sensorValues[1];graphData += ",";graphData += sensorTypes[2];graphData += ",";graphData += sensorValues[2];graphData += ",";graphData += sensorTypes[3];graphData += ",";graphData += sensorValues[3];graphData += "]";
      
      File f = SPIFFS.open(dataFile, "r+"); // See https://github.com/lorol/LITTLEFS/issues/33
      Serial.print("File size: "); Serial.println(f.size());
      f.seek((f.size()-1), SeekSet); Serial.print("Position: "); Serial.println(f.position());
      f.print(graphData); Serial.println(); Serial.print("Appended to file: "); Serial.println(graphData); Serial.print("File size: "); Serial.println(f.size());
      f.close(); 
                    
      if (voltage < 2.50) { mqtt.publish("Low battery for device: ", String(device)); }      // if voltage of battery gets to low, print the warning below.
      }           
    }
  }     
       // In some cases send command only once.
       if (mac[1] == 105 || mac[1] == 106 || mac[1] == 108 || mac[1] == 110){for (int i = 6; i < 256; i = i+10) {EEPROM.writeByte(i+1, 107);EEPROM.writeByte(i+2, apChannel);EEPROM.commit();}}
} // End of Proberequest function.


int motionSensor(int sample) {sampleBuffer[sampleBufferIndex++] = sample; if (sampleBufferIndex >= sampleSize) {sampleBufferIndex = 0; sampleBufferValid = 1;} if (sampleBufferValid) {averageTemp = 0; for (int i = 0; i < averageFilterSize; i++) {int idx = sampleBufferIndex - i; if (idx < 0) idx += sampleSize; averageTemp += sampleBuffer[idx];} average = averageTemp / averageFilterSize; averageBuffer[averageBufferIndex++] = average; if (averageBufferIndex >= averageBufferSize) {averageBufferIndex = 0; averageBufferValid = 1;} varianceSample = (sample - average)*(sample - average); varianceBuffer[varianceBufferIndex++] = varianceSample; if (varianceBufferIndex >= sampleSize) { varianceBufferIndex = 0; varianceBufferValid = 1;} varianceIntegral = 0; for (int i = 0; i < varianceIntegratorLimit; i++) {int idx = varianceBufferIndex - i; if (idx < 0) idx += sampleSize; varianceIntegral += varianceBuffer[idx];} variance = varianceIntegral;}

  return variance;
} // End of motion sensor function.


//========================================================================================================================//
//                 SETUP                                                                                                  //
//========================================================================================================================//

void setup(){
  Serial.begin(115200);
  delay(100);
  
  EEPROM.begin(512);
  SPIFFS.begin();
  
  EEPROM.readBytes(0, showConfig,256);for(int i=0;i<256;i++){Serial.printf("%d ", showConfig[i]);}Serial.println();
  
  if (showConfig[0] == 0 || showConfig[0] == 255){
  // Setup device numbers and wifi Channel for remote devices in EEPROM permanantly.
  for (int i = 6; i < 256; i = i+10) {EEPROM.writeByte(i, i);EEPROM.writeByte(i+1, 107);EEPROM.writeByte(i+2, apChannel);}
  EEPROM.writeByte(0, apChannel);EEPROM.commit();
  }
  
  //=============Setup motion sensor===================================================================================================// 
  if (sampleBufSize > MAX_sampleSize) { sampleBufSize = MAX_sampleSize; } sampleSize = sampleBufSize; varianceBufferSize = sampleBufSize;
  if (AvgSize > MAX_sampleSize) { AvgSize = MAX_sampleSize; } averageFilterSize = AvgSize;
  if (varThreshold > MAX_VARIANCE) { varThreshold = MAX_VARIANCE; } varianceThreshold = varThreshold;
  if (varIntegratorLimit > varianceIntegratorLimitMax) { varIntegratorLimit = varianceIntegratorLimitMax; } varianceIntegratorLimit = varIntegratorLimit;
  
  // Allocate memory based on config
  sampleBuffer = (int*)malloc(sizeof(int) * sampleSize); for (int i = 0; i < sampleSize; i++) sampleBuffer[i] = 0;
  averageBuffer = (int*)malloc(sizeof(int) * averageBufferSize); for (int i = 0; i < averageBufferSize; i++) averageBuffer[i] = 0;
  varianceBuffer = (int*)malloc(sizeof(int) * varianceBufferSize); for (int i = 0; i < varianceBufferSize; i++) varianceBuffer[i] = 0;
 //=====================================================================================================================================//

  Serial.setTimeout(100);
  WiFi.mode(WIFI_AP_STA);
  
  WiFi.softAP(apSSID, apPassword, apChannel, hidden);
  esp_wifi_set_event_mask(WIFI_EVENT_MASK_NONE); // This line is must to activate probe request received event handler.
  Serial.print("AP started with SSID: ");Serial.println(apSSID);

  int n = WiFi.scanNetworks();
  int strongestRSSI = -80;
  
  for (int i = 0; i < n; i++) {if (WiFi.RSSI(i) > strongestRSSI) {strongestRSSI = WiFi.RSSI(i); ssid = WiFi.SSID(i);}}

  if (WiFi.SSID(0) == ssid && WiFi.encryptionType(0) == WIFI_AUTH_OPEN) {WiFi.begin(ssid.c_str(), password.c_str());}
  
  mqtt.begin();
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
/*  
  server.on("/data.json", HTTP_GET, [](AsyncWebServerRequest *request){
    int rssi = WiFi.RSSI();
    int val = motionDetector_process(rssi);
    String json = "{";
    json += "\"rssi\":" + String(rssi) + ", ";
    json += "\"variance\":" + String(val);
    json += "}";
    request->send(200, "application/json", json);
  });

  server.on("/config.json", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"threshold\":" + String(varianceThreshold) ;
    json += "}";
    request->send(200, "application/json", json);
  });
*/
  server.begin();
  
  configTime(0, 0, ntpServer); setenv("TZ", MY_TZ, 1); tzset(); // Set environment variable with your time zone
  epoch = getTime(); Serial.print("Epoch Time: "); Serial.println(epoch); delay(500);


  WiFi.onEvent(probeRequest,WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED);
  Serial.print("Waiting for probe requests ... ");
 
 }  // End of Setup function

//========================================================================================================================//
//                     LOOP                                                                                               //
//========================================================================================================================//

void loop()
{
   
   if (WiFi.waitForConnectResult() != WL_CONNECTED) { ESP.restart(); }
   
   int rssi = WiFi.RSSI();
   int motion = motionSensor(rssi); 
  //Serial.print("RSSI: "); Serial.print(rssi); Serial.print(" -> Detection: "); Serial.println(motion);
  
   if (motion > sensitivity) {Serial.print(" Motion level detected: "); Serial.println(motion);}
   delay(100); // Adjust as needed
        
   mqtt.subscribe("command", [](const char * payload) {if (payload && strlen(payload)) {Serial.println(payload);Serial.printf("Received message in topic 'command' & message is:- %s\n", payload); 
     
   auto result = sscanf(payload, R"(["%u","%u","%u","%u","%u","%u",%10[^"]","%10[^"]"])", &receivedCommand[0], &receivedCommand[1], &receivedCommand[2], &receivedCommand[3], &receivedCommand[4], &receivedCommand[5], &ssid, &password);
  
   Serial.println(receivedCommand[0]);Serial.println(receivedCommand[1]);Serial.println(receivedCommand[2]);Serial.println(receivedCommand[3]);Serial.println(receivedCommand[4]);Serial.println(receivedCommand[5]);Serial.println(ssid.c_str());Serial.println(password.c_str()); 
    }
      if (receivedCommand[1] == 121) // Set sensor types on devices based on command received from website or mqtt client.
      { for (int i = 0; i < 4; i++) {uint8_t tempSensortypes[4]; tempSensortypes[i] = receivedCommand[i+2]; EEPROM.writeBytes(receivedCommand[0]+6, tempSensortypes,4);}
      } else if (receivedCommand[1] >= 101 && receivedCommand[1] <= 120) // Set everything else on devices based on command received from website or mqtt client.
      {for (int i = 0; i < 6; i++){ EEPROM.writeBytes(receivedCommand[0], receivedCommand,6);}}

      EEPROM.commit();Serial.println();Serial.println("Command or sensor types saved to EEPROM.");
      EEPROM.readBytes(0, showConfig,256);for(int i=0;i<256;i++){Serial.printf("%d ", showConfig[i]);}Serial.println();
      });
  
      mqtt.loop();
   
      yield();
       
}  // End of loop Function 
