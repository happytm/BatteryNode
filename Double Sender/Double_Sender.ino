
#define IRSENDER              false
#define PROBEREQUESTER        true
#define SLEEP_SECS 15 * 60 // 15 minutes
int device = 1;

#if PROBEREQUESTER
#include <ESP8266WiFi.h>
#endif

//============Do not need user configuration from here on============================

 
#if IRSENDER
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
//#include <IRrecv.h>
//#include <IRutils.h>
#endif

#include "SparkFunBME280.h"

ADC_MODE(ADC_VCC); //vcc read-mode

BME280 bme280;





int temperature = 10;
int humidity = 20;
int pressure = 30;
int Voltage = 40;
int light = 50; 
int VOLT_LIMIT = 3;
uint8_t irMac[] = {temperature,humidity,pressure,Voltage,light,device}; 


unsigned long lastMillis;
unsigned long passedMillis;


  

/*
//==============================Probe Request defines starts=======================  
extern "C" void preinit() {
    
   // Change MAC 

    uint8_t mac[6];
    mac[0] = temperature;
    mac[1] = humidity;
    mac[2] = pressure;
    mac[3] = Voltage;
    mac[4] = light;
    mac[5] = device;

  

  
  wifi_set_opmode (STATION_MODE);
  wifi_set_macaddr(STATION_IF, irMac);
}
//==============================Probe Request defines ends=======================  
*/

//====================IR defines starts============================

#if IRSENDER
const uint16_t kIrLed = 16;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

// Example Samsung A/C state captured from IRrecvDumpV2.ino
//uint8_t myMac[kSamsungAcStateLength] = {temperature, humidity, pressure, Voltage, light, device}; //{0xb4, 0xe6, 0x52, 0x44, 0x86, 0xad, 0xb4, 0xe6, 0x52, 0x44, 0x86, 0xad };
//uint64_t myMac = 0x1234567890AB;//{temperature, humidity, pressure, battery, lux, device};  // 48bits 
uint64_t myMac = 
    (uint64_t)(uint8_t (irMac[5])) | 
    (uint64_t)(uint8_t (irMac[4])) << 8 |          
    (uint64_t)(uint8_t (irMac[3])) << 16 |          
    (uint64_t)(uint8_t (irMac[2])) << 24 |          
    (uint64_t)(uint8_t (irMac[1])) << 32 |          
    (uint64_t)(uint8_t (irMac[0])) << 40;    
#endif
    
   
//====================IR defines ends==============================


void setup() {
    Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
    Serial.println();
    Serial.print("Milliseconds passed before setup: ");
    Serial.println(millis());
    lastMillis = millis();
    Serial.println();
    
    
    #if PROBEREQUESTER
    WiFi.mode(WIFI_STA); // Station mode for esp-now controller
  //   wifi_set_opmode (STATION_MODE);
    wifi_set_macaddr(STATION_IF, irMac);
    WiFi.disconnect();
    WiFi.hostname("Livingroom");
    Serial.println();
    #endif
    
    float Voltage = ESP.getVcc() / (float)1023; // * (float)1.07;
    Serial.print("Voltage: "); Serial.print(Voltage); Serial.print("  Voltage Expected: "); Serial.println(VOLT_LIMIT);
    if (Voltage < VOLT_LIMIT)      // if voltage of battery gets to low, the LED wil blink fast.
  {
    Serial.println("Warning :- Battery Voltage low please change batteries" );
    Serial.println();
  }
    
    Serial.println("Setup finished");
    
    
    passedMillis = millis() - lastMillis;
    Serial.print("Time spent on setup: "); 
    Serial.println(passedMillis);
    Serial.println();
    lastMillis = millis();

    readBME280();
    Serial.println();
    
    #if PROBEREQUESTER
    probeRequest();
    #endif
    
    #if IRSENDER
    irsend.begin();
    irSender();
    #endif
    
  }

//========================Main Loop================================

void loop() {

//===========================IR loop===============================
  #if IRSENDER
  //irSender();
  #endif
 //=====================Probe request Loop=========================
  
// put your code here to run in loop or move to setup to run oncce.
 
 #if PROBEREQUESTER
 probeRequest();
 #endif
 
 //readBME280();
  


 Serial.print("Total time I spent before going to sleep: "); 
 Serial.println(millis());
 Serial.print("I will wakeup in: "); 
 Serial.print(SLEEP_SECS / 60); 
 Serial.println(" Minutes"); 
 delay(20000);
 ESP.restart(); 
//ESP.deepSleep(0);
 
    
}

//=========================Main Loop ends==========================






void readBME280() {
  bme280.settings.commInterface = I2C_MODE;
  bme280.settings.I2CAddress = 0x76;
  bme280.settings.runMode = 2; // Forced mode with deepSleep
  bme280.settings.tempOverSample = 1;
  bme280.settings.pressOverSample = 1;
  bme280.settings.humidOverSample = 1;
  Serial.print("bme280 init="); Serial.println(bme280.begin(), HEX);
  float temperature = 25;//bme280.readTempC();
  humidity = bme280.readFloatHumidity();
  pressure = bme280.readFloatPressure() / 100.0;
  delay(10);
  passedMillis = millis() - lastMillis;
  Serial.print("Time spent on BME280: "); 
  Serial.println(passedMillis);
  Serial.println();
}



void gotoSleep() {                            //need connection between GPIO16 and reset pin on ESP8266
  // add some randomness to avoid collisions with multiple devices
  int sleepSecs = SLEEP_SECS;// + ((uint8_t)RANDOM_REG32/2);
  Serial.printf("Up for %i ms, going to sleep for %i secs...\n", millis(), sleepSecs);
  ESP.deepSleep(sleepSecs * 1000000, RF_NO_CAL);
}

#if PROBEREQUESTER
//=========================Probe request function starts===========
  
  void probeRequest()  {
  Serial.println("Starting Probe sender");
  Serial.println("Sending sensor data over Probe request protocol to Master Node");
  Serial.println();
 

//WiFi.scanNetworksAsync(prinScanResult);

//int8_t scanNetworks(bool async = true, bool show_hidden = false, uint8 channel = 0, uint8* ssid = NULL);
int n = WiFi.scanNetworks(true, true, 6);

#if !IRSENDER
delay(25); // Minimum delay required to scan networks in async mode if not using IRSENDER 
#endif

#if IRSENDER
delay(10);     // Minimum delay required to scan networks in async mode if using IRSENDER              
#endif
/*
String ssid;
uint8_t encryptionType;
int32_t RSSI;
uint8_t* BSSIDstr;
int32_t channel;
bool isHidden;

//for (int i = 0; i < n; i++)
//{
 
  
 // WiFi.getNetworkInfo(i, ssid, encryptionType, RSSI, BSSIDstr, channel, isHidden);
  //Serial.printf("%d: %s, Ch:%d (%ddBm) %s %s\n", i + 1, ssid.c_str(), channel, RSSI, encryptionType == ENC_TYPE_NONE ? "open" : "", isHidden ? "hidden" : "");
  delay(10);
//}

*/

 String ssid;
 Serial.println();
 Serial.print("This Device MAC ID is: ");
 Serial.println(WiFi.macAddress());
 Serial.print("This Device Name is: ");
 Serial.println(WiFi.hostname());
 Serial.print("Gateway MAC ID is: ");
 Serial.println(WiFi.BSSIDstr(0));
 Serial.print("Gateway Name is: ");
 Serial.println(WiFi.SSID(0));
 Serial.print("Gateway Channel is: ");
 Serial.println(WiFi.channel(0));
    



 Serial.println();
 WiFi.scanDelete();
 passedMillis = millis() - lastMillis;
 Serial.print("Time spent on Probe Request: "); 
 Serial.println(passedMillis);
 lastMillis = millis();
 Serial.println();
 
 delay(10);
}
#endif

#if IRSENDER
//========================IR function starts=========================================
void irSender()   {
    
  Serial.println("Starting IR data sender");
  Serial.println("Sending sensor data over IR protocol to Master Node");
  Serial.println();
   //  irsend.sendSamsungAC(myMac);
   //irsend.sendMidea(hwaddr);
   irsend.sendMidea(myMac); 
   Serial.print("Message sent : ");
//   serialPrintUint64(myMac, HEX); //  
   Serial.println();
   
  
 /* for(uint8_t u=0; u<6; u++)
  {
      Serial.print(" Device :  ");
      Serial.print(myMac[5]);
      Serial.print(" Temprature :  ");
      Serial.print(myMac[0]);
      Serial.print(" Humidity :  ");
      Serial.print(myMac[1]);
      Serial.print(" Pressure :  ");
      Serial.print(myMac[2]);
      Serial.print(" Battery :  ");
      Serial.print(myMac[3]);
      Serial.print(" Lux :  ");
      Serial.println(myMac[4]);
      //Serial.println(hwaddr[u]);
   }
   */  
    passedMillis = millis() - lastMillis;
    Serial.print("Time spent on IR Sender: "); 
    Serial.println(passedMillis);
    lastMillis = millis();
    Serial.println();
    //delay(10);
}      
#endif

 
