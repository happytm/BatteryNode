// 35 ms transmit & receive time for 24 bytes of data and 68 ms total uptime required in two way mode.Confirm and try to reduce this time.
#include <WiFi.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <ESP32httpUpdate.h>                // Install from arduino library manager
#include <EEPROM.h>
#include "driver/adc.h"                     // required to turn off ADC.
#include <esp_bt.h>                         // required to turn off BT.
#include "motionDetector.h"                 // Thanks to https://github.com/paoloinverse/motionDetector_esp
#include <ESP32Ping.h>                      // Thanks to https://github.com/marian-craciunescu/ESP32Ping

const char* routerSSID = "routerssid";       // Main router's SSID.
const char* routerPassword = "routerpassword";    // Main router's password.

const char* room = "Livingroom";            // Needed for person locator.Each location must run probeReceiver sketch to implement person locator.
const char* apPassword = "";
const int apChannel = 0;
const int hidden = 0;                       // If hidden is 1 probe request event handling does not work ?
int rssiThreshold = -50;                    // Adjust according to signal strength by trial & error.
int motionThreshold = 70;                   // Adjust the sensitivity of motion sensor.Higher the number means less sensetive motion sensor is.

int WiFiChannel = 7;                        // This must be same for all devices on network.
const char* ssid = "ESP";                   // Required for OTA update & motion detection.
const char* password = "";                  // Required for OTA update & motion detection.

int enableCSVgraphOutput = 1;               // 0 disable, 1 enable.If enabled, you may use Tools-> Serial Plotter to plot the variance output for each transmitter. 
long dataInterval;                          // Interval to send data.
int motionLevel = -1;         

IPAddress deviceIP(192, 168, 0, 2);         // Fixed IP address assigned to family member's devices to be checked for their presence at home.
//IPAddress deviceIP = WiFi.localIP();
int device1IP = 2, device2IP = 3, device3IP = 4, device4IP = 5;
uint8_t device1ID[3] = {0xD0, 0xC0, 0x8A};  // First and last 2 bytes of Mac ID of Cell phone #1.
uint8_t device2ID[3] = {0x36, 0x33, 0x33};  // First and last 2 bytes of Mac ID of Cell phone #2.
uint8_t device3ID[3] = {0x36, 0x33, 0x33};  // First and last 2 bytes of Mac ID of Cell phone #3.
uint8_t device4ID[3] = {0x36, 0x33, 0x33};  // First and last 2 bytes of Mac ID of Cell phone #4.



//==================User configuration generally not required below this line ============================

String binFile = "http://192.168.4.1/device_246.bin";

int Hour;               // Hour received from Gateway. More reliable source than internal RTC of local device
int Minute;             // Minute received from Gateway. More reliable source than internal RTC of local device.

uint8_t showConfig[20]; // Content of EEPROM is saved here.

int commandType;        // digitalwrite, analogwrite, digitalRead, analogRead, neopixel, pin setup etc.
int value1;             // gpio pin number or other values like device ID, sleeptime, Ap Channel, Device mode etc.
int value2;             // 0 or 1 in case of digitalwrte, 0 to 255 in case of analogwrite or value for RED neopixel or value for sensorType 4.
int value3;             // 0 to 255 - value for GREEN neopixel or value for sensorType 5.
int value4;             // 0 to 255 - value for BLUE neopixel or value for sensorType 6.

uint8_t sensorValues[] =                // Looks like 24 bytes is minimum (sending as WIFI_IF_AP) and 1500 bytes is maximum limit.
 {
  0x80, 0x00,                           //  0- 1: Frame Control. Type 80 = Beacon.
  0x00, 0x00,                           //  2- 3: Duration
  0x11, 0x11, 0x11, 0x11, 0x11, 0x11,   //  4- 9: Destination address.Fill with custom data.
  0x06, 0x22, 0x22, 0x22, 0x22, 0x22,   // 10-15: Source address.Fill with custom data.
  0x33, 0x33, 0x33, 0x33, 0x33, 0x33,   // 16-21: BSSID.Fill with custom data.
  0x00, 0x00,                           // 22-23: Sequence / fragment number
 };
 
void setup(){
  
  EEPROM.begin(20);
  
  if (EEPROM.readByte(0) == 0 || EEPROM.readByte(0) == 255)  {EEPROM.writeByte(0, 246);} 
  if (EEPROM.readByte(15) < 1 || EEPROM.readByte(15) > 14) {EEPROM.writeByte(15, WiFiChannel);}
  if (EEPROM.readByte(16) == 0 || EEPROM.readByte(16) == 255) {EEPROM.writeByte(16, 1);}
  EEPROM.commit();

  motionDetector_init();  // initializes the storage arrays in internal RAM
  motionDetector_config(64, 16, 3, 3, false); 
  Serial.setTimeout(1000);

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Wifi connection failed");
    WiFi.disconnect(false);
    delay(1000);
    WiFi.begin(ssid, password);
 }
  
  Serial.begin(115200);
  Serial.println("Wait about 1 minute for motion sensor to be ready to detect motion.....");

  WiFi.onEvent(probeRequest, SYSTEM_EVENT_AP_PROBEREQRECVED); 
}

//===================== End of Setup ====================================================

void loop(){ 
 dataInterval++; 
 
 motionDetector_set_minimum_RSSI(-80);                // Minimum RSSI value to be considered reliable. Default value is 80 * -1 = -80.
 motionLevel = 0;  // Reset motionLevel to 0 to resume motion tracking.
 motionLevel = motionDetector_esp();                  // if the connection fails, the radar will automatically try to switch to different operating modes by using ESP32 specific calls. 
  //Serial.print("Motion detected & motion level is: ");Serial.println(motionLevel);
 if (dataInterval > (EEPROM.readByte(16) * 100))      // 100 for 1 minute & 10 for 6 seconds.
 {
  sendSensorvalues();
 } else if (motionLevel > motionThreshold)  // Adjust the sensitivity of motion sensor.Higher the number means less sensetive motion sensor is.
 {
  Serial.print("Motion detected & motion level is: ");Serial.println(motionLevel);
  sendSensorvalues();
  WiFi.softAP(room, apPassword, apChannel, hidden);
  esp_wifi_set_event_mask(WIFI_EVENT_MASK_NONE); // This line is must to activate probe request received event handler.
  Serial.print("AP started with name: ");Serial.println(room);
  
  
  }
  delay(600);   // Do not change this. Data interval is calculated based on this value.
} // End of loop.

//============= End of main loop and all functions below ====================================

void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) 
{ 
  
 wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t*)buf;
 
 if (p->payload[0] == 0x80 && p->payload[4] == sensorValues[4])   // HEX 80 for type - Beacon to filter out unwanted traffic. 
  {
   Serial.print("Command received from Gateway : ");
  
   for(int i=0;i<=21;i++){
   Serial.print(p->payload[i], HEX);
   }
   
   Serial.println();
   EEPROM.writeByte(1, p->payload[5]);  // Command type at address 1. 
   EEPROM.commit();

   commandType = EEPROM.readByte(1);

   Serial.println("Contents of EEPROM for this device below: ");
   EEPROM.readBytes(0, showConfig,19);for(int i=0;i<19;i++){ 
   Serial.printf("%d ", showConfig[i]);}
      
   if ( commandType > 100 && commandType < 121)  {   // If commandType is 101 to 120.
      
      Serial.println();
      Serial.print("This device's Wifi Channel is: ");Serial.println(EEPROM.readByte(15));  
      Serial.print("This device's MAC ID is: ");Serial.println(WiFi.macAddress());
      value1 = p->payload[6];
      value2 = p->payload[7];
      value3 = p->payload[8];
      value4 = p->payload[9];
      
       synchTime();

       if (commandType == 101)        // Digital Write
       {
         EEPROM.writeByte(2,value1);
         EEPROM.writeByte(3,value2);EEPROM.commit();
         Serial.print("Received Command Digital Write: ");Serial.print(value1);  Serial.println(value2);
       
         gpioControl();
         
       } else if (commandType == 102) // Analog Write
       {
         EEPROM.writeByte(4,value1);
         EEPROM.writeByte(5,value2);EEPROM.commit();
         Serial.print("Received Command Analog Write:  ");Serial.print(value1);  Serial.println(value2);

         gpioControl();
         
       } else if (commandType == 103)  // Digital Read
       {
          Serial.println("Received Command Digital Read pin:  ");Serial.println(value1);
       } else if (commandType == 104)  // Analog Read
       { 
           Serial.println("Received Command Digital Read pin: ");Serial.println(value1);
       } else if (commandType == 105)  // Neopixel
       {
         EEPROM.writeByte(6,value1);
         EEPROM.writeByte(7,value2);
         EEPROM.writeByte(8,value3);
         EEPROM.writeByte(9,value4);EEPROM.commit();
         Serial.print("Received Command Neopixel: ");Serial.print(value1);  Serial.println(value2);Serial.print(value3);  Serial.println(value4);

         gpioControl();
         
       } else if (commandType == 106)  // Set Targets
       {
         EEPROM.writeByte(10,value1);
         EEPROM.writeByte(11,value2);
         EEPROM.writeByte(12,value3);
         EEPROM.writeByte(13,value4);EEPROM.commit();
         Serial.print("Received Command Set Target Values to: ");Serial.println(value1);  Serial.println(value2);Serial.print(value3);  Serial.println(value4);        
         
       } else if (commandType == 107)  // Set AP Channel
       {
         EEPROM.writeByte(14,value1);EEPROM.commit();
         Serial.print("Received Command Set AP Channel to: ");Serial.println(value1);
               
       } else if (commandType == 108 && value1 == 1)  // Set Mode
       {
         Serial.print("Received Command Set Device Mode to: ");Serial.println(value1);
         EEPROM.writeByte(15,0);
         EEPROM.writeByte(0,246);EEPROM.commit();
         OTAupdate();
         
       } else if (commandType == 109)  // Set Sleep Time
       {
         EEPROM.writeByte(16,value1);EEPROM.commit();
         Serial.print("Received Command Set Sleep Time to:   ");Serial.print(value1);Serial.println(" minutes.");
          
       } else if (commandType == 110)  // Set Device ID
       {
         
         EEPROM.writeByte(0,value1);EEPROM.commit();
         Serial.print("Received Command Set Device ID to: ");Serial.println(value1);
         
        }
         
         Serial.println("Command from Gateway saved to EEPROM");
         Serial.println("Contents of EEPROM for this device below: ");Serial.println();
         EEPROM.readBytes(0, showConfig,19);for(int i=0;i<19;i++){ 
         Serial.printf("%d ", showConfig[i]);}Serial.println();
         delay(1);
    } else {
    
    Serial.println("Resending sensor values..."); 
    ESP.restart();   // Seems like gateway did not receive sensor values let's try again.
    }
  }
}

void probeRequest(WiFiEvent_t event, WiFiEventInfo_t info) 
{ 
  Serial.println();
    
  if (info.ap_probereqrecved.mac[0] != device1ID[0] && info.ap_probereqrecved.mac[4] != device1ID[1] && info.ap_probereqrecved.mac[5] != device1ID[2]) 
  { // write code to match MAC ID of cell phone to predefined variable and store presence/absense in new variable.
    if (info.ap_probereqrecved.rssi > -70)
    {
      Serial.print("Guest MAC ID is :  ");for (int i = 16; i < 21; i++) {sensorValues[i] = info.ap_probereqrecved.mac[i - 16];Serial.printf("%02X", sensorValues[i]);if (i < 20)Serial.print(":");}Serial.println();
      Serial.println("################ Guest arrived ###################### ");    
      Serial.print("Signal Strength of guest device: "); Serial.println(info.ap_probereqrecved.rssi);
      sensorValues[15] = info.ap_probereqrecved.rssi;
      
    if (info.ap_probereqrecved.rssi > rssiThreshold) // Adjust according to signal strength by trial & error.
     { // write code to match MAC ID of cell phone to predefined variable and store presence/absense in new variable.
       Serial.print("Guest is near device: ");Serial.println(EEPROM.readByte(0));
         
     }
    }          
  }
       WiFi.softAPdisconnect(true);
       WiFi.enableAP(false);
       WiFi.disconnect(true);
       
       // Connect to main router to ping known devices.
       if (WiFi.waitForConnectResult() != WL_CONNECTED) {Serial.println("Wifi connection failed");WiFi.disconnect(false);delay(500);WiFi.begin(routerSSID, routerPassword);}
       Serial.println("Checking to see who is at home.... ");
        
       int pingTime;
         
         deviceIP[3] = device1IP;Serial.println("Pinging IP address 2... ");if(Ping.ping(deviceIP)) {pingTime = Ping.averageTime();Serial.print("Ping time in milliseconds: ");Serial.println(pingTime);sensorValues[11] = (pingTime);} else {sensorValues[11] = 0;}
         deviceIP[3] = device2IP;Serial.println("Pinging IP address 3... ");if(Ping.ping(deviceIP)) {pingTime = Ping.averageTime();Serial.print("Ping time in milliseconds: ");Serial.println(pingTime);sensorValues[12] = (pingTime);} else {sensorValues[12] = 0;}
         deviceIP[3] = device3IP;Serial.println("Pinging IP address 4... ");if(Ping.ping(deviceIP)) {pingTime = Ping.averageTime();Serial.print("Ping time in milliseconds: ");Serial.println(pingTime);sensorValues[13] = (pingTime);} else {sensorValues[13] = 0;}
         deviceIP[3] = device4IP;Serial.println("Pinging IP address 5... ");if(Ping.ping(deviceIP)) {pingTime = Ping.averageTime();Serial.print("Ping time in milliseconds: ");Serial.println(pingTime);sensorValues[14] = (pingTime);} else {sensorValues[14] = 0;}

       sendSensorvalues();
 
 } // End of Proberequest function.

 
void sendSensorvalues()
{
  sensorValues[4] = EEPROM.readByte(0);   // Device ID.
  sensorValues[5] = 165;                  // Voltage must be between 130 and 180 here in whole integer.
  sensorValues[6] = random(70,74);        // Sensor 1 value.
  sensorValues[7] = random(40,100);       // Sensor 2 value.
  sensorValues[8] = random(900,1024) / 4; // Sensor 3 value.
  sensorValues[9] = random(0,100);        // Sensor 4 value.
  sensorValues[10] = motionLevel;         // Motion Level.
  // Values received from all sensors used on this device and should replace random values of sensorValues array.
  
  Serial.println("Sending sensor values....."); 
  long lastmillis = millis();
  WiFi.mode(WIFI_AP_STA); 
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);
  esp_wifi_set_channel(WiFiChannel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_80211_tx(WIFI_IF_AP, sensorValues, sizeof(sensorValues), true);
  
  long currentmillis = millis() - lastmillis;
  Serial.print("Transmit & receive time (Milliseconds) : ");Serial.println(currentmillis);
  dataInterval = 0;   // Data sent. Reset the data interval counter.
}

void synchTime() {
  if (commandType == 105 || commandType == 106)
  {
    Hour = EEPROM.readByte(17);                           // Hour value from EEPROM.
    Minute = EEPROM.readByte(18) + EEPROM.readByte(16);   // Minute from EEPROM + Sleep Time from EEPROM.
  } else {
    EEPROM.writeByte(17,value3);                   // Save hour to EEPROM.
    EEPROM.writeByte(18,value4); EEPROM.commit();  // Save minute to EEPROM.
    Hour = value3;     // New hour value received from Gateway.
    Minute = value4;   //  New minute value received from Gateway.
  }
  Serial.print("Time received from Gateway: ");Serial.print(Hour); Serial.print(":"); Serial.println(Minute);
  }


void gpioControl() {
 
    if ((EEPROM.readByte(2) >= 1 && EEPROM.readByte(2) <= 5) || (EEPROM.readByte(2) >= 12 && EEPROM.readByte(2) <= 39))   
    {if (EEPROM.readByte(3) == 1){digitalWrite(EEPROM.readByte(2), HIGH);} else if (EEPROM.readByte(2) == 0){digitalWrite(EEPROM.readByte(2), LOW);}
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

    switch(ret) {
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
