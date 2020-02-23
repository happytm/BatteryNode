
/*
  Data exchange time in simulated sensors mode (no sensors physically connected).
  Duplex mode =  125 milliseconds, One way = 52 milliseonds.
************************************************************
Command structure:  (commands are issued via MQTT payload with topic name "command/"

Command1 = Device ID Number -       device ID must be 2 digits end with 2,6,A or E to avoid conflict with other devices.
                                            See https://serverfault.com/questions/40712/what-range-of-mac-addresses-can-i-safely-use-for-my-virtual-machines.
                                            use any of following for devie ID ending with 6.
                                            06,16,26,36,46,56,66,76,86,96,106,116,126,136,146,156,166,176,186,196,206,216,226,236,246 etc.
                                            Device ID and last part of fixed IP are same.
                                            
Command2 = Command type     -        value 01 to 09 is reserved for following commands(must have 0 as first digit):

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
                                       09 = Activate alternative function for OTA,Wifimanager ETC.
                                            Example command payload 36/09/00 or 01 or 02/ (01 to activate Auto firmware update & 02 to activate AutoConnect.).

                                            value 10 to 20 is reserved for following commands:
                                        

Command3 = Command  pinNumber  -            pinNumber in case of command type 01 to 04 above. 
                                            Neopixel LED number in case of command type 05.
                                            Value in case of command type 06,07,08 & 09 commandtype.
                                            sensorType4 value in case of command 06.
                                            

Command4 = Command value1      -            00 or 255 in case of command type 01 (digitalWrite & analogWrite)  
                                            or RED neopixel value in case of command type 05 
                                            or sensorType4 value in case of command 06.

Command5 = Command value2      -            00 to 255 for GREEN neopixel in case of command type 05 
                                            or sensorType5 value in case of command 06.
        
Command6 = Command value2      -            00 to 255 for BLUE neopixel in case of command type 05 
                                            or sensorType6 value in case of command 06.
*/


#define DUPLEX            true    // true if two way communication required with controller (around 130 milliseconds of uptime as opposed to 60 milliseonds if false).

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
  uint32_t crc32;          // 4 bytes stored at begining of RTC memory to cross check data integrity.
  byte data[10];           // 10 bytes of data stored in RTC memory.  
} rtcData;
#endif

#include <ESP8266WiFi.h>


#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#define binFile "https://raw.githubusercontent.com/happytm/BatteryNode/master/sender.bin"
#define BOOT_AFTER_UPDATE    false
HTTPClient http;

const char* ssid = "HTM1";
const char* password = "kb1henna";

// use any of following for devie ID ending with 6.
// 6,16,26,36,46,56,66,76,86,96,106,116,126,136,146,156,166,176,186,196,206,216,226,236,246 etc.

int device = 36;         // Unique device ID must end with 2,6,A or E. See https://serverfault.com/questions/40712/what-range-of-mac-addresses-can-i-safely-use-for-my-virtual-machines.
int wifiChannel = 7;     // WiFi Channel for this device. 
char* gateway = "ESP";   // This name has to be same as main controller's ssid.
int sleepTime = 1;       // Sleep time in minutes.
int upTime;              // Device uptime in milliseconds.
int deviceMode;      // 0 for regular, 1 for autupdate and 2 for AutoConnect.
int deviceIP = device;   // last part of this device's fixed IP
//int requestStatus;
//int duplexStatus;


// Status values to be sent to Gateway

uint8_t deviceStatus[6];  // {device, deviceMode, deviceIP, wifiChannel, sleepTime, random(255)}
/*
int sensorType1 = 36;// Predefined sensor type table is below:
int sensorType2 = 06;// Predefined sensor type table is below:
int sensorType3 = 16;// volatage = 6, temperature = 16, humidity= 26,
int sensorType4 = 26;// pressure= 36, light= 46, OpenClose = 56, level = 66,
int sensorType5 = 36;// presence = 76, motion = 86, rain = 96 etc.
int sensorType6 = 46;// volatage = 6, temperature = 16, humidity= 26,
*/

// Sensor types to be sent to Gateway

uint8_t sensorType[6];  // = {sensorType1, sensorType2, sensorType3, sensorType4, sensorType5, sensorType6}; // Change last 4 bytes according to sensot type used.

// Sensor values to be sent to Gateway

uint8_t sensorData[6];  // = {device, voltage, temperature, humidity, pressure, light};

int receivedDevice;
int receivedCommand;  // digitalwrite, analogwrite, digitalRead, analogRead, neopixel, pin setup etc.
int pinNumber;        // gpio pin number 1 to 5 & 12 to 16 or value for sensorType 3.
int value1;           // 0 or 1 in case of digitalwrte, 0 to 255 in case of analogwrite or value for RED neopixel or value for sensorType 4.
int value2;           // 0 to 255 - value for GREEN neopixel or value for sensorType 5.
int value3;           // 0 to 255 - value for BLUE neopixel or value for sensorType 6.

int warnVolt = 130;   // Start warning when battery level goes below 2.60 volts (260/2 = 130).


//============Do not need user configuration from here on============================



#define FACTORY_APKEY                    "configesp"
#define BUILD_NOTES                      ""

// Select features to include into the Core:
#define FEATURE_RULES                    true
#define FEATURE_PLUGINS                  true
#define FEATURE_TIME                     true
#define FEATURE_I2C                      true

// Some extra features, disabled by default
#define FEATURE_ADC_VCC                  false
#define SERIALDEBUG                      true


// Select a custom plugin set
//#define PLUGIN_SET_BASIC
#define PLUGIN_SET_ALL

#ifdef PLUGIN_SET_BASIC
  #define USES_P001 // Switch
  #define USES_P002 // ADC
#endif

#ifdef PLUGIN_SET_ALL
  #define USES_P001 // Switch
  #define USES_P002 // ADC
  #define USES_P004 // Dallas
  #define USES_P011 // PME
  #define USES_P012 // LCD
  #define USES_P014 // SI7021
  #define USES_P100 // MSGBUS
  #define USES_P101 // MQTT
  #if defined(ESP8266)
    #define USES_P102 // ESPNOW
  #endif
  #define USES_P110 // HTTP
  //#define USES_P111 // HTTPS
  #define USES_P200 // Nano Serial
  #define USES_P201 // Tuya LSC
  #if defined(ESP32)
    #define USES_P203 // M5
  #endif
  #define USES_P254 // Local Log
  #define USES_P255 // Debugging stuff
#endif


// End of config section, do not remove this:
// Could not move setup() to base tab, preprocessor gets confused (?)
// So we just call setup2() from here
#include "Globals.h"
void setup() {
  
  // Read struct from RTC memory
if (ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcData, sizeof(rtcData))) 
  {
   // Serial.print("RTC memory: ");
    printMemory();
    //Serial.println();
    uint32_t crcOfData = calculateCRC32((uint8_t*) &rtcData.data[0], sizeof(rtcData.data));
    //Serial.print("CRC32 Data: ");
    //Serial.println(crcOfData, HEX);
    //Serial.print("CRC32 from RTC: ");
    //Serial.println(rtcData.crc32, HEX);
    if (crcOfData != rtcData.crc32) {
      //Serial.println("CRC32 in RTC memory doesn't match CRC32 of data. Data is probably invalid!");
    
    } else {
      //Serial.println("CRC32 check ok, data is valid.");
     }
  } 
    
     deviceMode =rtcData.data[8];
     
  if (deviceMode == 2) {
        rtcData.data[8] = 0;
        setup2(); // Start alternative firmware EspCoreRules Code for WiFi setup & Webserver.
  
  } else {
    WiFi.scanDelete();  //remove previous scan data from memory
    Serial.begin(115200);
    
    rtcData.data[8] = 0;
    sensorType[0] = rtcData.data[0];   // Device ID
    sensorType[1] = rtcData.data[1];   // Voltage
    sensorType[2] = rtcData.data[2];   // Sensor Tytp 1
    sensorType[3] = rtcData.data[3];   // Sensor Tytp 2
    sensorType[4] = rtcData.data[4];   // Sensor Tytp 3
    sensorType[5] = rtcData.data[5];   // Sensor Tytp 4
    
  wifi_set_macaddr(STATION_IF, sensorType);
  probeRequest();
  Serial.print("Sensor Type values sent to controller: ");
  Serial.println(WiFi.macAddress());
  
  sensorValues();
  probeRequest();
  Serial.print("Sensors values sent to controller: ");
  Serial.println(WiFi.macAddress());

 
    delay(60);  // Minimum 60 milliseonds delay required to receive message from controller reliably.

    receivedDevice = WiFi.BSSID(0)[0];
  
  if (receivedDevice == device)  {   //match first byte of gateway's mac id with this devices's ID here.
    
    Serial.print("Message received from Controller: ");
    Serial.println(&WiFi.BSSIDstr(0)[0]);
    Serial.println();
    //Serial.print("This Device MAC ID is: ");
    //Serial.println(WiFi.macAddress());
    //Serial.print("This Device Name is: ");
    //Serial.println(WiFi.hostname());
    Serial.print("Gateway Name is: ");
    Serial.println(WiFi.SSID(0));
    Serial.print("Gateway Channel is: ");
    Serial.println(WiFi.channel(0));
    
    uint8_t* receivedData[6]=  {WiFi.BSSID(0)};
    
    receivedDevice = WiFi.BSSID(0)[0];
    receivedCommand = WiFi.BSSID(0)[1];
    pinNumber = WiFi.BSSID(0)[2];
    value1 = WiFi.BSSID(0)[3];
    value2 = WiFi.BSSID(0)[4];
    value3 = WiFi.BSSID(0)[5];
   
     
   if (receivedCommand == 6)      
             {
              rtcData.data[0] = receivedDevice;
              rtcData.data[1] = receivedCommand;
              rtcData.data[2] = pinNumber;
              if (pinNumber == 16)
              {  
              rtcData.data[3] = 26;
              rtcData.data[4] = 36; 
              } else {
              rtcData.data[3] = value1;
              rtcData.data[4] = value2;
              }
              rtcData.data[5] = value3;
    
    } else if (receivedCommand == 7) 
             {
              rtcData.data[6] = pinNumber;
              wifiChannel = rtcData.data[6];
              
     } else if (receivedCommand == 8 && pinNumber != 0) 
             {
              rtcData.data[7] = pinNumber;    // Save sleep time in minutes.
              sleepTime = rtcData.data[7];
               
     } else if (receivedCommand == 9) 
             {
              rtcData.data[8] = pinNumber;      // Save device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
              deviceMode = rtcData.data[8];
              }
      
      
  }  else 
    {
    Serial.println("Message from controller did not arrive, let me try again to get message data........................................");
    //ESP.restart();
    }

    // Update CRC32 of data
     rtcData.crc32 = calculateCRC32((uint8_t*) &rtcData.data[0], sizeof(rtcData.data));
     // Write struct to RTC memory
    if (ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData))) 
     {
   // Serial.println("Write: ");
      Serial.println();
     }

     delay(1);

      gpioControl();

if (receivedDevice == device && receivedCommand == 9)  {
   
      otaControl();
      
    }  

   rtcData.data[8] = deviceMode;
   rtcData.data[9] = (millis() + 8);  // Estimated 8 milliseconds added to account for next process in loop.
   upTime = rtcData.data[9];
   sendStatus();
   Serial.print("Total time I spent before going to sleep: ");
   Serial.println(upTime);
   Serial.print("I will wakeup in: ");
   Serial.print(sleepTime);
   Serial.println(" Minutes");
   delay(5000); ESP.restart();   // For testing only.
   //ESP.deepSleepInstant(sleepTime * 60000000, WAKE_NO_RFCAL); //If last digit of MAC ID matches to device ID go to deep sleep else loop through again.
    
  
  }   
}

//=========================Probe request function starts===========


void probeRequest()  {
 
  //int8_t scanNetworks(bool async = true, bool show_hidden = false, uint8 channel = 0, uint8* ssid = NULL);
    int n = WiFi.scanNetworks(true, false, wifiChannel, (uint8*) gateway);
  //int n = WiFi.scanNetworks(true, false);

  yield();

  Serial.println();
  WiFi.scanDelete();
 
}

//=========================Probe request function ends===========


void sensorValues() {

  float voltage = ESP.getVcc() / (float)1023 * 50; // * (float)1.07;
  /*
  Serial.print("Voltage: "); Serial.print(voltage); Serial.print("  Minimum Voltage Required: "); Serial.println(warnVolt);
  if (voltage < warnVolt)      // if voltage of battery gets to low, print the warning below.
  {
    Serial.println("Warning :- Battery Voltage low please change batteries" );
    Serial.println();
  }
  */

 sensorData[0] = device;
 sensorData[1] = voltage;
/*  sensorData[2] = random(90);         //temperature;
  sensorData[3] = random(100);        //humidity;
  sensorData[4] = random(1024) / 4;   //pressure;
  sensorData[5] = random(100);        //light;
  */
  
 //Functions for all sensors used on this device goes here and activated by command received from controller.
 //Values received from sensors replaces 4 random values of sensorData array.
    
    if (sensorType[2] == 16)    // If 16 (BME280 sensor) is chosen here the sensor value is Temperature.
    {

      sensorType[3] = 26;   // If sensorType 16 (BME280 sensor) is chosen for sensorType[2] above sensor type must be 26 (Humidity) here.
      sensorType[4] = 36;   // If sensorType 16 (BME280 sensor) is chosen for sensorType[2] above sensor type must be 36 (Pressure) here.
      Serial.println();
      Serial.println("Activate function for BME280");
      sensorData[2] = 1;         //temperature;
      sensorData[3] = 2;        //humidity;
      sensorData[4] = random(1024) / 4;   //pressure;
    
    } else if (sensorType[2] == 46) {

      sensorData[2] = 60;  
      Serial.println("Activate function for APDS9960 Light Sensor");
      Serial.println();
    
    } else if (sensorType[2] == 56) {

      sensorData[2] = 1; 
      Serial.println("Activate function for OpenClose sensor");
      Serial.println();
    } else if (sensorType[2] == 66) {

      sensorData[2] = 8;  // 0 to 256 cm.
      Serial.println("Activate function for HCSR04 distance sensor");
      Serial.println();
    
    } else if (sensorType[2] == 76) {

      sensorData[2] = 0;  
      Serial.println("Activate function for motion (rcwl-0516 or hc-sr505) sensor");
      Serial.println();
      
    } else if (sensorType[2] == 86) {

      sensorData[2] = random(90);  
      Serial.println("Activate function for some sensor");
      Serial.println();

    } else if (sensorType[2] == 96) {

      sensorData[2] = random(90);  
      Serial.println("Activate function for some sensor");
      Serial.println();
    }

    if (sensorType[3] == 46) {

      sensorData[3] = 60;  
      Serial.println("Activate function for APDS9960 Light Sensor");
      Serial.println();
    
    } else if (sensorType[3] == 56) {

      sensorData[3] = 1;  // 0 or 1
      Serial.println("Activate function for OpenClose sensor");
      Serial.println();
    } else if (sensorType[3] == 66) {

      sensorData[3] = 8;  // 0 to 256 cm.
      Serial.println("Activate function for HCSR04 distance sensor");
      Serial.println();
    
    } else if (sensorType[3] == 76) {

      sensorData[3] = random(1);  // 0 or 1
      Serial.println("Activate function for motion (rcwl-0516 or hc-sr505) sensor");
      Serial.println();
      
    } else if (sensorType[3] == 86) {

      sensorData[3] = random(90);  
      Serial.println("Activate function for some sensor");
      Serial.println();

    } else if (sensorType[3] == 96) {

      sensorData[3] = random(90);  
      Serial.println("Activate function for some sensor");
      Serial.println();
    }

    if (sensorType[4] == 46) {

      sensorData[4] = random(100);  
      Serial.println("Activate function for APDS9960 Light Sensor");
      Serial.println();
    
    } else if (sensorType[4] == 56) {

      sensorData[4] = random(1);  // 0 or 1
      Serial.println("Activate function for OpenClose sensor");
      Serial.println();
    } else if (sensorType[4] == 66) {

      sensorData[4] = 8;  // 0 to 256 cm.
      Serial.println("Activate function for HCSR04 distance sensor");
      Serial.println();
    
    } else if (sensorType[4] == 76) {

      sensorData[4] = random(1);  // 0 or 1
      Serial.println("Activate function for motion (rcwl-0516 or hc-sr505) sensor");
      Serial.println();
      
    } else if (sensorType[4] == 86) {

      sensorData[4] = random(90);  
      Serial.println("Activate function for some sensor");
      Serial.println();

    } else if (sensorType[4] == 96) {

      sensorData[4] = random(90);  
      Serial.println("Activate function for some sensor");
      Serial.println();
    }

    if (sensorType[5] == 46) {

      sensorData[5] = random(100);  
      Serial.println("Activate function for APDS9960 Light Sensor");
      Serial.println();
    
    } else if (sensorType[5] == 56) {

      sensorData[5] = random(2);  // 0 or 1
      Serial.println("Activate function for OpenClose sensor");
      Serial.println();
    } else if (sensorType[5] == 66) {

      sensorData[5] = 8;  // 0 to 256 cm.
      Serial.println("Activate function for HCSR04 distance sensor");
      Serial.println();
    
    } else if (sensorType[5] == 76) {

      sensorData[5] = random(1);  // 0 or 1
      Serial.println("Activate function for motion (rcwl-0516 or hc-sr505) sensor");
      Serial.println();
      
    } else if (sensorType[5] == 86) {

      sensorData[5] = random(90);  
      Serial.println("Activate function for some sensor");
      Serial.println();

    } else if (sensorType[5] == 96) {

      sensorData[5] = random(90);  
      Serial.println("Activate function for some sensor");
      Serial.println();
    }

 
  wifi_set_macaddr(STATION_IF, sensorData);

}


#if DUPLEX
void gpioControl()   {

if (receivedDevice = device)   {
  
   if ((pinNumber >= 1 && pinNumber <= 5) || (pinNumber >= 12 && pinNumber <= 16))   { 
    if (receivedCommand == 1)    {

      if (value1 == 1) 
      { 
      digitalWrite(pinNumber, HIGH);
      Serial.print("digitalWrite");
      Serial.print("(");
      Serial.print(pinNumber);
      Serial.print(",");
      Serial.print(value1);
      Serial.println(");");
      Serial.println();
      Serial.println();
      } else if (value1 == 0) 
      {
      digitalWrite(pinNumber, LOW);
      Serial.print("digitalWrite");
      Serial.print("(");
      Serial.print(pinNumber);
      Serial.print(",");
      Serial.print(value1);
      Serial.println(");");
      Serial.println();
      Serial.println();
      } else 
      {
      analogWrite(pinNumber, value1);
      Serial.print("analogWrite");
      Serial.print("(");
      Serial.print(pinNumber);
      Serial.print(",");
      Serial.print(value1);
      Serial.println(");");
      Serial.println();
      Serial.println(); 
      }
    }  
  /*
   } else if (receivedCommand == 5)    {
      // TO DO - write function for neopixel
      analogWrite(pinNumber, value1);
      Serial.print("analogWrite");
      Serial.print("(");
      Serial.print(pinNumber);
      Serial.print(",");
      Serial.print(value1);
      Serial.println(");");
      Serial.println();
      Serial.println();
      } 
      */
     }     
    }
   }
  
void otaControl() 
{


  rtcData.data[8] = pinNumber;      // Save device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
  
  Serial.print("Device Mode set to: ");
  Serial.println(rtcData.data[8]); 
  if (rtcData.data[8] == 1) 
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
  rtcData.data[8] = 0;     // Set device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).

  Serial.print("Device Mode set to: ");
  Serial.println(rtcData.data[8]); 
  
  http.end();

    switch(ret) {
         case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
           // rtcData.data[8] = 2;            // Save device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
            rtcData.data[8] = 2;   // Set device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
            Serial.print("Device Mode set to: ");
            Serial.println(rtcData.data[8]);  
            break;

         case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            rtcData.data[8] = 2;     // Set device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
            Serial.print("Device Mode set to: ");
            Serial.println(rtcData.data[8]);  
            break;

         case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            delay(5000);                    // wait for few seconds issue command with payload 36/09/00/.
            Serial.print("Device Mode set to: ");
            Serial.println(rtcData.data[8]); 
            rtcData.data[8] = 0;
            ESP.restart();
            break;

         default:
            Serial.printf("Undefined HTTP_UPDATE function: ");Serial.println(ret);
              }

    } else if (rtcData.data[8] == 2)
    
    { 
            
      
    } else {
     
      
     }
  }

void sendStatus() {
  
   deviceStatus[0] = device;  
   deviceStatus[1] = deviceMode;     // 0 for regular, 1 for autupdate and 2 for AutoConnect.
   deviceStatus[2] = deviceIP;              // Last part of this device's fixed IP (same as device ID).
   deviceStatus[3] = wifiChannel;         // WiFi Channel for this device. 
   deviceStatus[4] = sleepTime;           // Sleep time in minutes for this device.
   deviceStatus[5] = upTime;              // Device upTime in milliseconds.
   wifi_set_macaddr(STATION_IF, deviceStatus);
   probeRequest();
   Serial.print("Device status values sent to controller: ");
   Serial.println(WiFi.macAddress());           
  }

void printMemory() {
  char buf[3];
  uint8_t *ptr = (uint8_t *)&rtcData;
  for (size_t i = 0; i < sizeof(rtcData); i++) {
    sprintf(buf, "%02X", ptr[i]);
    Serial.print(buf);
    if ((i + 1) % 32 == 0) {
      Serial.println();
    } else {
      Serial.print(" ");
    }
  }
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
