/*
  Data exchange time in simulated sensors mode (no sensors physically connected).
  Duplex mode =  134 milliseconds, One way = 52 milliseonds.
************************************************************
        Command structure:  (commands are issued via MQTT payload with topic name "command/"

        Command1 = Device ID Number -       device ID must be 2 digits end with 2,6,A or E to avoid conflict with other devices.
                                            See https://serverfault.com/questions/40712/what-range-of-mac-addresses-can-i-safely-use-for-my-virtual-machines.
                                            use any of following for devie ID ending with 6.
                                            06,16,26,36,46,56,66,76,86,96,106,116,126,136,146,156,166,176,186,196,206,216,226,236,246 etc.
                                            Device ID and last part of fixed IP are same.
                                            
        Command2 = Command type  -     value 1 to 9 is reserved for following commands(must have 0 as first digit):

                                       01 = digitalWright or analogWrite.
                                            Example command payload 36/01/00 0r 01/ for digitalWrite.
                                            Example command payload 36/01/02 to 256/ for analogWrite.
                                       02 = digitalRead.
                                            Example command payload 36/02/01 to 05 or 12 to 16/
                                       03 = analogRead,
                                       04=  Reserved,
                                       05 = Neopixel etc.
                                            Example command payload 36/05/01 to 05 or 12 to 16/00 to 256/00 to 256/00 to 256/
                                       06 = change sensoor types.First byte must be target device id and
                                            second byte must be 06 (sensor type voltage). Rest of 4 bytes (each ending with 6) can be changed according to hardware setup.
                                            Example command payload 36/06/16/26/36/46/.

                                       07 = change wifiChannel.
                                       08 = change sleepTime.
                                            Example command payload 36/08/00 to 255/ (Sleep Time in minutes).
                                       09 = Activate alternative code for OTA,Wifimanager ETC.
                                            Example command payload 36/09/00 or 01 or 02/ (01 to activate Auto firmware update & 02 to activate AutoConnect.).

                                            value 10 to 20 is reserved for following commands:
                                        
                                        10 = change device ID (existing deviceID 16 to 256 ending with 6)/10/new device ID 16 to 256 ending with 6/). 
                                             Example command payload  36/10/46/
                                             Device ID and last part of fixed IP are same.
                                        11 = request for device status (01 for true and 00 for false).
                                             Example command payload 36/11/00 or 01/  
                                        12 = change define DUPLEX (01 for true and 00 for false).
                                             Example command payload 36/12/00 or 01/ 

        Command3 = Command  pinNumber  -    pinNumber in case of command type 01 to 04 above. 
                                            Neopixel LED number in case of command type 05.
                                            Value in case of command type 09 above (00 for normal deep sleep operation, 01 to activate Auto firmware update & 02 to activate AutoConnect).
                                            Predefined number to represent value of command type 11 to 20 above.
                                            00 or 01 to represent false/or true for defines in case of command type 21 to 30.

        Command4 = Command pinValue     -   00 or 255 in case of command type 01 (digitalWrite & analogWrite)  or RED neopixel value in case of command type 05.

        Command5 = Command extraValue1  -   00 to 255 for GREEN neopixel in case of command type 05
                                            or sensorType value in case of command 06.
        Command6 = Command extraValue1  -   00 to 255 for BLUE neopixel in case of command type 05
                                            or sensorType value in case of command 06.

**********************************************************************/
/*
  WebUpdate.ino, Example for the AutoConnect library.
  Copyright (c) 2018, Hieromon Ikasamo
  https://github.com/Hieromon/AutoConnect
  This example is an implementation of a lightweight update feature
  that updates the ESP8266's firmware from your web browser. It embeds
  ESP8266HTTPUpdateServer into the AutoConnect menu and can invoke the
  firmware update UI via a Web browser.
  You need a compiled sketch binary file to the actual update and can
  retrieve it using Arduino-IDE menu: [Sketck] -> [Export compiled binary].
  Then you will find the .bin file in your sketch folder. Select the.bin
  file on the update UI page to update the firmware.
  Notes:
  1. To experience this example, your client OS needs to be running a
  service that can respond to multicast DNS.
  For Mac OSX support is built in through Bonjour already.
  For Linux, install Avahi.
  For Windows10, available since Windows10 1803(April 2018 Update/RS4).
  2. If you receive an error as follows:
  Update error: ERROR[11]: Invalid bootstrapping state, reset ESP8266 before updating.
  You need reset the module before sketch running.
  Refer to https://hieromon.github.io/AutoConnect/faq.html#hang-up-after-reset for details.
  This software is released under the MIT License.
  https://opensource.org/licenses/MIT
*/

/*  To Do:
 *  - Send status of running mode (regular, autoupdate or Autoconnect),last part of IP, wifi Channel & sleeptime to controller.
 *  - Assign fixed IP - last part of IP could be same as device id of the unit. 
 */
#if defined(ARDUINO_ARCH_ESP8266)
ADC_MODE(ADC_VCC); //vcc read-mode
#endif

#define DUPLEX            true    // true if two way communication required with controller (around 130 milliseconds of uptime as opposed to 60 milliseonds if false).
#define AUTOOTA           true    // true if auto OTA update required through Github.

#if AUTOOTA
#define AUTOCONNECT       true    //deactivate AutoConnect if Auto OTA activated above.
#endif

#if DUPLEX
/*
  #define BME280SENSOR      false    // Temperature, Humidity & pressure.
  #define APDS9960SENSOR    false    // Light or Gesture.
  #define PHOTOSENSOR       false    // Light.
  #define OPENCLOSESENSOR   true     // Hall sensor,reed switch,ball switch or mercury switch.
  #define HCSR04SENSOR      false    // Distance.
  #define PIRSENSOR         true     // Presence detection alarm.
  #define RCWL0516SENSOR    true     // Presence detection alarm.
*/
/*
// CRC function used to ensure data validity
uint32_t calculateCRC32(const uint8_t *data, size_t length);
*/
// Structure which will be stored in RTC memory.
// First field is CRC32, which is calculated based on the
// rest of structure contents.
// Any fields can go after CRC32.
// We use byte array as an example.

struct {
  uint32_t crc32;
  byte data[4];           // 4 bytes of data for 4 sensor types.
  
  int deviceMode;         // 0 for regular, 1 for autupdate and 2 for AutoConnect.
  int devceIP;            // last part of this device's fixed IP
  int wifiChannel;        // WiFi Channel for this device. 
  int sleepTime;          // Sleep time in minutes.
  int requestStatus;
  int duplexStatus;
  
} rtcData;
#endif

#if AUTOOTA
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#define binFile "https://raw.githubusercontent.com/happytm/BatteryNode/master/senderFirmware.bin"
#define BOOT_AFTER_UPDATE    false
HTTPClient http;

const char* ssid = "";
const char* password = "";
#endif  //AUTOOTA

char* gateway = "ESP";   // This name has to be same as main controller's ssid.

// use any of following for devie ID ending with 6.
// 6,16,26,36,46,56,66,76,86,96,106,116,126,136,146,156,166,176,186,196,206,216,226,236,246 etc.

int device = 36;    // Unique device ID must end with 2,6,A or E. See https://serverfault.com/questions/40712/what-range-of-mac-addresses-can-i-safely-use-for-my-virtual-machines.
//uint8_t securityCode[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Security code must be same at controller to compare.

int deviceMode = 0;         // 0 for regular, 1 for autupdate and 2 for AutoConnect.
int devceIP = device;            // last part of this device's fixed IP
int wifiChannel = 7;        // WiFi Channel for this device. 
int sleepTime = 1;          // Sleep time in minutes.
int requestStatus;
int duplexStatus;

// Status values to be sent to Gateway

uint8_t deviceStatus[6];  // {device, deviceMode, deviceIP, wifiChannel, sleepTime, random(255)}


int sensorType1 = 16;// Predefined sensor type table is below:
int sensorType2 = 26;// volatage = 6, temperature = 16, humidity= 26,
int sensorType3 = 36;// pressure= 36, light= 46, OpenClose = 56, level = 66,
int sensorType4 = 46;// presence = 76, motion = 86, rain = 96 etc.


// Sensor types to be sent to Gateway

uint8_t sensorType[6] = {device, 6, sensorType1, sensorType2, sensorType3, sensorType4}; // Change last 4 bytes according to sensot type used.

// Sensor values to be sent to Gateway

uint8_t sensorData[6];  // = {device, voltage, temperature, humidity, pressure, light};

#define USERNAME "user"   // For web OTA update.
#define PASSWORD "pass"   // For web OTA update.

int receivedDevice;
int receivedCommand;  // digitalwrite, analogwrite, digitalRead, analogRead, neopixel, pin setup etc.
int pinNumber;        // gpio pin number 1 to 5 & 12 to 16 or value for sensorType 3.
int pinValue;         // 0 or 1 in case of digitalwrte, 0 to 255 in case of analogwrite or value for RED neopixel or value for sensorType 4.
int extraValue1;      // 0 to 255 - value for GREEN neopixel or value for sensorType 5.
int extraValue2;      // 0 to 255 - value for BLUE neopixel or value for sensorType 6.

int warnVolt = 130;    // Start warning when battery level goes below 2.60 volts (260/2 = 130).
unsigned long lastMillis;
unsigned long passedMillis;

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>

#if DUPLEX

#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#define HOSTIDENTIFY  "esp8266"
#define mDNSUpdate(c)  do { c.update(); } while(0)
using WebServerClass = ESP8266WebServer;
using HTTPUpdateServerClass = ESP8266HTTPUpdateServer;

#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "HTTPUpdateServer.h"
#define HOSTIDENTIFY  "esp32"
#define mDNSUpdate(c)  do {} while(0)
using WebServerClass = WebServer;
using HTTPUpdateServerClass = HTTPUpdateServer;
#endif

#include <WiFiClient.h>
#include <AutoConnect.h>

// This page for an example only, you can prepare the other for your application.
static const char AUX_AppPage[] PROGMEM = R"(
{
  "title": "Hello world",
  "uri": "/",
  "menu": true,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "value": "<h2>Hello, world</h2>",
      "style": "text-align:center;color:#2f4f4f;padding:10px;"
    },
    {
      "name": "content",
      "type": "ACText",
      "value": "In this page, place the custom web page handled by the sketch application."
    }
  ]
}
)";

// Fix hostname for mDNS. It is a requirement for the lightweight update feature.
static const char* host = HOSTIDENTIFY "-webupdate";
#define HTTP_PORT 80

// ESP8266WebServer instance will be shared both AutoConnect and UpdateServer.
WebServerClass  httpServer(HTTP_PORT);


// Declare AutoConnectAux to bind the HTTPWebUpdateServer via /update url
// and call it from the menu.
// The custom web page is an empty page that does not contain AutoConnectElements.
// Its content will be emitted by ESP8266HTTPUpdateServer.
HTTPUpdateServerClass httpUpdater;
AutoConnectAux  update("/update", "Update");

// Declare AutoConnect and the custom web pages for an application sketch.
AutoConnect     portal(httpServer);
AutoConnectAux  hello;
#endif

//============Do not need user configuration from here on============================

void setup() {
  
  // WiFi.disconnect();
  WiFi.scanDelete();  //remove previous scan data from memory
  Serial.begin(115200);
  //Serial.println(&WiFi.BSSIDstr(0)[0]);

/* 
  wifi_set_macaddr(STATION_IF, securityCode);
  probeRequest();
  Serial.print("Security Code sent to controller: ");
  Serial.println(WiFi.macAddress());
 */ 
#if DUPLEX
// Read struct from RTC memory
if (ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcData, sizeof(rtcData))) 
  {
   //Serial.println("Read: ");
    //printMemory();
    Serial.println();
    uint32_t crcOfData = calculateCRC32((uint8_t*) &rtcData.data[0], sizeof(rtcData.data));
    Serial.print("CRC32 of data: ");
    Serial.println(crcOfData, HEX);
    Serial.print("CRC32 read from RTC: ");
    Serial.println(rtcData.crc32, HEX);
    if (crcOfData != rtcData.crc32) {
      Serial.println("CRC32 in RTC memory doesn't match CRC32 of data. Data is probably invalid!");
    
    } else {
      Serial.println("CRC32 check ok, data is probably valid.");
     }
  } 

   
     // Update CRC32 of data
     rtcData.crc32 = calculateCRC32((uint8_t*) &rtcData.data[0], sizeof(rtcData.data));
     // Write struct to RTC memory
    if (ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData))) 
     {
   // Serial.println("Write: ");
      Serial.println();
     }

    sensorType[2] = rtcData.data[1] ;
    sensorType[3] = rtcData.data[2] ;
    sensorType[4] = rtcData.data[3] ;
    sensorType[5] = rtcData.data[4] ;
  
 
 #endif   
 
  wifi_set_macaddr(STATION_IF, sensorType);
  probeRequest();
  Serial.print("Sensor Type values sent to controller: ");
  Serial.println(WiFi.macAddress());
  
  sensorValues();
  probeRequest();
  Serial.print("Sensors values sent to controller: ");
  Serial.println(WiFi.macAddress());

/*
  deviceStatus[0] = device;  
  deviceStatus[1] = deviceMode;         // 0 for regular, 1 for autupdate and 2 for AutoConnect.
  deviceStatus[2] = device;            // last part of this device's fixed IP
  deviceStatus[3] = wifiChannel;        // WiFi Channel for this device. 
  deviceStatus[4] = sleepTime;          // Sleep tme nminutes for this device.
  deviceStatus[5] = random(100);        // To do.
  wifi_set_macaddr(STATION_IF, deviceStatus);
  probeRequest();

  Serial.print("Device status values sent to controller: ");
  Serial.println(WiFi.macAddress());
  */
  
#if DUPLEX
  delay(60);  // Minimum 60 milliseonds delay required to receive message from controller reliably.

  receivedDevice = WiFi.BSSID(0)[0];
  
  if (receivedDevice == device)  {   //match first byte of gateway's mac id with this devices's ID here.
    
    Serial.println();
    //Serial.print("This Device MAC ID is: ");
    //Serial.println(WiFi.macAddress());
    //Serial.print("This Device Name is: ");
    //Serial.println(WiFi.hostname());
    Serial.print("Gateway Name is: ");
    Serial.println(WiFi.SSID(0));
    Serial.print("Gateway Channel is: ");
    Serial.println(WiFi.channel(0));
    
    Serial.print("Message received from Controller (HEX format)is: ");
    Serial.println(&WiFi.BSSIDstr(0)[0]);
    
    uint8_t* receivedData[6]=  {WiFi.BSSID(0)};
    Serial.println("Message (DEC format): ");
    
    receivedDevice = WiFi.BSSID(0)[0];
    Serial.print("Received Device: ");
    Serial.print(receivedDevice);
    receivedCommand = WiFi.BSSID(0)[1];
    Serial.print(" receivedCommand: ");
    Serial.print(receivedCommand);
    pinNumber = WiFi.BSSID(0)[2];
    Serial.print(" pinNumber: ");
    Serial.print(pinNumber);
    pinValue = WiFi.BSSID(0)[3];
    Serial.print(" pinValue: ");
    Serial.print(pinValue);
    extraValue1 = WiFi.BSSID(0)[4];
    Serial.print(" extraValue1: ");
    Serial.print(extraValue1);
    extraValue2 = WiFi.BSSID(0)[5];
    Serial.print(" extraValue2: ");
    Serial.println(extraValue2);
     
   if (receivedCommand == 6)      
   {
              rtcData.data[1] = pinNumber;
              rtcData.data[2] = pinValue;
              rtcData.data[3] = extraValue1;
              rtcData.data[4] = extraValue2;
    
    } else if (receivedCommand == 7) 
           {
              rtcData.data[12] = pinNumber;
              wifiChannel = rtcData.data[12];
              Serial.print("WiFi Channel set to: ");
              Serial.println(wifiChannel);
     
     } else if (receivedCommand == 8 && pinNumber != 0) 
            {
               rtcData.data[20] = pinNumber;    // Save sleep time in minutes.
               sleepTime = rtcData.data[20];    // Set sleep tme in minutes. 
               Serial.print("Sleep Time in minutes: ");
               Serial.println(sleepTime);
      
      } else if (receivedCommand == 9) 
             {
               rtcData.data[28] = pinNumber;      // Save device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
               deviceMode = rtcData.data[28];     // Set device mode.
               Serial.print("Device Mode set to: ");
               Serial.println(deviceMode);    
              }
      
  }  else 
    {
    Serial.println("Message from controller did not arrive, let me try again to get message data........................................");
    ESP.restart();
    }

   

    if (sensorType1 == 16 || sensorType1 == 26 || sensorType1 == 36 || 
        sensorType2 == 16 || sensorType2 == 26 || sensorType2 == 36 || 
        sensorType3 == 16 || sensorType3 == 26 || sensorType3 == 36 || 
        sensorType4 == 16 || sensorType4 == 26 || sensorType4 == 36) 
    {           
     #define BME280SENSOR   true
     Serial.println();
     Serial.println("Activate code for BME280");
    }  
    
    if (sensorType1 == 46 || sensorType2 == 46 || sensorType3 == 46 || sensorType4 == 46)  
    {
    #define APDS9960SENSOR   true
    Serial.println("Activate code for APDS9960");
    } 
    
    if (sensorType1 == 56 || sensorType2 == 56 || sensorType3 == 56 || sensorType4 == 56)  
    {
    #define OPENCLOSESENSOR   true  
    Serial.println("Activate code for OpenClose sensor");
    } 
    
    if (sensorType1 == 66 || sensorType2 == 66 || sensorType3 == 66 || sensorType4 == 66)  
    {
    #define HCSR04SENSOR   true
    Serial.println("Activate code for distance sensor");
    } 
     
    if (sensorType1 == 76 || sensorType2 == 76 || sensorType3 == 76 || sensorType4 == 76)  
    {
    #define PIRSENSOR   true  
    Serial.println("Activate code for presene detection sensor");
    } 

    if (sensorType1 == 86 || sensorType2 == 86 || sensorType3 == 86 || sensorType4 == 86)  
    {
    #define RCWL0516SENSOR   true
    Serial.println("Activate code for presence detection sensor");
    }    

    if (sensorType1 == 96 || sensorType2 == 96 || sensorType3 == 96 || sensorType4 == 96)  
    {
    #define PHOTOSENSOR   true
    Serial.println("Activate code for light sensor");
    Serial.println();
    }  
    
  
   
#endif

  delay(1);
  
}      // Setup ends here

//========================Main Loop================================

void loop() {

#if DUPLEX
    
 gpioControl();


if (receivedDevice == device)  {
   if (receivedCommand == 9) 
    {
      otaControl();
      
    } else if (receivedCommand == 8 && pinNumber != 0) 
            {
               rtcData.data[20] = pinNumber;    // Save sleep time in minutes.
               sleepTime = rtcData.data[20];    // Set sleep tme in minutes. 
               Serial.print("Sleep Time in minutes: ");
               Serial.println(sleepTime);
      
    } else if (receivedCommand == 10 && pinNumber == 1) 
             {
               rtcData.data[36] = pinNumber;      // Save device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
               device = rtcData.data[36];     // Set device mode.
               Serial.print("Device ID set to: ");
               Serial.println(device);    
              

    } else if (receivedCommand == 11 && pinNumber == 1) 
             {
               sendStatus();
              
    } else if (receivedCommand == 12 && pinNumber == 1) 
             {
               rtcData.data[52] = pinNumber;      // Save device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
               duplexStatus = rtcData.data[52];     // Set device mode.
               Serial.print("Device Mode set to: ");
               Serial.println(duplexStatus);    
              } 
 }

#endif
            Serial.print("Total time I spent before going to sleep: ");
            Serial.println(millis());
            Serial.print("I will wakeup in: ");
            Serial.print(sleepTime);
            Serial.println(" Miinutes");

  
         //   delay(5000);         //disable delay when deep sleep activated.
         //   Serial.println("Going to Deep Sleep..........................");

    delay(5000); ESP.restart();   // For testing only.
   // gotoSleep();
   //ESP.deepSleep(sleepTime * 60000000, RF_NO_CAL); //If last digit of MAC ID matches to device ID go to deep sleep else loop through again.
    
}     // end of main loop.
//=========================Main Loop ends==========================


//=========================Probe request function starts===========


void probeRequest()  {
  /*
    Serial.println("Starting Probe sender");
    Serial.println("Sending sensor data over Probe request protocol to Master Node");
    Serial.println();
  */
  //int8_t scanNetworks(bool async = true, bool show_hidden = false, uint8 channel = 0, uint8* ssid = NULL);
 int n = WiFi.scanNetworks(true, false, wifiChannel, (uint8*) gateway);

  yield();

  Serial.println();
  WiFi.scanDelete();
 /*
  passedMillis = millis() - lastMillis;
  Serial.print("Time spent on Probe Request: ");
  Serial.println(passedMillis);
  lastMillis = millis();
  Serial.println();
  */
}

//=========================Probe request function ends===========


void sensorValues()     {


  // voltage = random(100);
  float voltage = ESP.getVcc() / (float)1023 * 50; // * (float)1.07;
  //voltage = map(voltage, lowVolt, highVolt, 0, 100);

  Serial.print("Voltage: "); Serial.print(voltage); Serial.print("  Minimum Voltage Required: "); Serial.println(warnVolt);
  if (voltage < warnVolt)      // if voltage of battery gets to low, print the warning below.
  {
    Serial.println("Warning :- Battery Voltage low please change batteries" );
    Serial.println();
  }
  
 //Functions for all sensors used on this device goes here and activated by command received from controller.
 //Values received from sensors replaces 4 random values of sensorData array.

  sensorData[0] = device;
  sensorData[1] = voltage;
  sensorData[2] = random(90);         //temperature;
  sensorData[3] = random(100);        //humidity;
  sensorData[4] = random(1024) / 4;   //pressure;
  sensorData[5] = random(100);        //light;
  wifi_set_macaddr(STATION_IF, sensorData);

}


#if DUPLEX
void gpioControl()   {

if (receivedDevice = device)   {
  
   if ((pinNumber >= 1 && pinNumber <= 5) || (pinNumber >= 12 && pinNumber <= 16))   { 
    if (receivedCommand == 1)    {

      if (pinValue == 1) 
      { 
      digitalWrite(pinNumber, HIGH);
      Serial.print("digitalWrite");
      Serial.print("(");
      Serial.print(pinNumber);
      Serial.print(",");
      Serial.print(pinValue);
      Serial.println(");");
      Serial.println();
      Serial.println();
      } else if (pinValue == 0) 
      {
      digitalWrite(pinNumber, LOW);
      Serial.print("digitalWrite");
      Serial.print("(");
      Serial.print(pinNumber);
      Serial.print(",");
      Serial.print(pinValue);
      Serial.println(");");
      Serial.println();
      Serial.println();
      } else 
      {
      analogWrite(pinNumber, pinValue);
      Serial.print("analogWrite");
      Serial.print("(");
      Serial.print(pinNumber);
      Serial.print(",");
      Serial.print(pinValue);
      Serial.println(");");
      Serial.println();
      Serial.println(); 
      }
    }  
  /*
   } else if (receivedCommand == 5)    {
      // TO DO - write code for neopixel
      analogWrite(pinNumber, pinValue);
      Serial.print("analogWrite");
      Serial.print("(");
      Serial.print(pinNumber);
      Serial.print(",");
      Serial.print(pinValue);
      Serial.println(");");
      Serial.println();
      Serial.println();
      } 
      */
     }     
    }
   }
  

  void ActivateAutoConnect()
 {
  Serial.print("Running code to acctivate AutoConnect");
  #define AUTOCONNECT       true
  //sleepTime = 1;
             
   
  // Prepare the ESP8266HTTPUpdateServer
  // The /update handler will be registered during this function.
     httpUpdater.setup(&httpServer, USERNAME, PASSWORD);

  // Load a custom web page for a sketch and a dummy page for the updater.
     hello.load(AUX_AppPage);
     portal.join({ hello, update });

  if (portal.begin()) {
    if (MDNS.begin(host)) {
        MDNS.addService("http", "tcp", HTTP_PORT);
        Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host);
            
    }
     else
        Serial.println("Error setting up MDNS responder");
      } 
            // Sketches the application here.

            // Invokes mDNS::update and AutoConnect::handleClient() for the menu processing.
            mDNSUpdate(MDNS);
            portal.handleClient();
            delay(1);
     
            Serial.println("waiting for OTA update..........................");
            Serial.print("Log in at this IP within 5 minutes to load new firmware: ");
            Serial.println(WiFi.localIP().toString());
            delay(60000 * 5);

      }


void otaControl() 
{

#if AUTOOTA
  deviceMode = pinNumber;
  rtcData.data[28] = pinNumber;      // Save device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
  
  Serial.print("Device Mode set to: ");
  Serial.println(deviceMode); 
  if (deviceMode == 1) 
  {  
  Serial.println("Start WiFi Connection......");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println(".");
  }                                   
  Serial.println("Connected to WiFi");
  pinMode(LED_BUILTIN, OUTPUT);
  WiFiClient client;
  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);


  ESPhttpUpdate.rebootOnUpdate(BOOT_AFTER_UPDATE);
  Serial.println("New firmware will be loaded now..............");
  t_httpUpdate_return ret = ESPhttpUpdate.update(binFile,"","CC AA 48 48 66 46 0E 91 53 2C 9C 7C 23 2A B1 74 4D 29 9D 33");
  deviceMode = 0;     // Set device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).

  Serial.print("Device Mode set to: ");
  Serial.println(deviceMode); 
  
  http.end();

    switch(ret) {
         case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
           // rtcData.data[28] = 2;            // Save device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
            deviceMode = 2;   // Set device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
            Serial.print("Device Mode set to: ");
            Serial.println(deviceMode);  
            break;

         case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            //rtcData.data[28] = 2;              // Save device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
            deviceMode = 2;     // Set device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
            Serial.print("Device Mode set to: ");
            Serial.println(deviceMode);  
            break;

         case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            delay(5000);                    // wait for few seconds issue command with payload 36/09/00/.
            //rtcData.data[28] = 0;            // Save device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
            
            Serial.print("Device Mode set to: ");
            Serial.println(deviceMode); 
            deviceMode = 0;   // Set device mode (0 for regular, 1 for autupdate and 2 for AutoConnect). 
            rtcData.data[28] = 0;
            ESP.restart();
            break;

         default:
            Serial.printf("Undefined HTTP_UPDATE Code: ");Serial.println(ret);
              }

     #endif  //AUTOOTA
    

    } else if (deviceMode == 2)
    
    { 
            ActivateAutoConnect();
      
    } else {
     
      sleepTime = rtcData.data[22];   // Set sleeptme.
      Serial.print("Sleep Time in minutes: ");
      Serial.println(sleepTime);
     }
  }

void sendStatus() {
  
               rtcData.data[44] = pinNumber;      // Save device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
               requestStatus = rtcData.data[44];     // Set device mode.
               Serial.print("Request status: ");
               Serial.println(requestStatus);  
               deviceStatus[0] = device;  
               deviceStatus[1] = deviceMode;         // 0 for regular, 1 for autupdate and 2 for AutoConnect.
               deviceStatus[2] = devceIP;            // last part of this device's fixed IP
               deviceStatus[3] = wifiChannel;        // WiFi Channel for this device. 
               deviceStatus[4] = sleepTime;          // Sleep tme nminutes for this device.
               deviceStatus[5] = random(100);        // To do.
               wifi_set_macaddr(STATION_IF, deviceStatus);
               probeRequest();
               Serial.print("Device status values sent to controller: ");
               Serial.println(WiFi.macAddress());
               requestStatus = 0;
               rtcData.data[44] = 0; 
         }


 uint32_t calculateCRC32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}
#endif
