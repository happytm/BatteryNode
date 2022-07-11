// 35 ms transmit & receive time for 24 bytes of data and 68 ms total uptime required in two way mode.Confirm and try to reduce this time.
// Reference : http://nomartini-noparty.blogspot.com/2016/07/esp8266-and-beacon-frames.html
#include <WiFi.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <ESP32httpUpdate.h>  // Install from arduino library manager
#include <EEPROM.h>
#include "driver/adc.h"

int WiFiChannel = 7;           // This must be same for all devices on network.
const char* ssid = "ESP";      // Required for OTA update.
const char* password = "";     // Required for OTA update.

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
     
int warnVolt = 130;     // Start warning when battery level goes below 2.60 volts (260/2 = 130).

uint8_t sensorValues[] =       // Looks like 24 bytes is minimum (sending as WIFI_IF_AP) and 1500 bytes is maximum limit.
 {
  0x80, 0x00,                                                 //  0- 1: Frame Control. Type 80 = Beacon.
  0x00, 0x00,                                                 //  2- 3: Duration
  0x11, 0x11, 0x11, 0x11, 0x11, 0x11,                         //  4- 9: Destination address.Fill with custom data.
  0x06, 0x22, 0x22, 0x22, 0x22, 0x22,                         // 10-15: Source address.Fill with custom data.
  0x33, 0x33, 0x33, 0x33, 0x33, 0x33,                         // 16-21: BSSID.Fill with custom data.
  0x00, 0x00,                                                 // 22-23: Sequence / fragment number
 };
 
void setup(){
  //adc_power_off();     // ADC work finished so turn it off to save power.
  WiFi.mode(WIFI_OFF);   // WiFi transmit & receive work finished so turn it off to save power.
  esp_wifi_stop();       // WiFi transmit & receive work finished so turn it off to save power.

  EEPROM.begin(20);
  
  if (EEPROM.readByte(0) == 0 || EEPROM.readByte(0) == 255)  {EEPROM.writeByte(0, 246);} 
  if (EEPROM.readByte(15) < 1 || EEPROM.readByte(15) > 14) {EEPROM.writeByte(15, WiFiChannel);}
  if (EEPROM.readByte(16) == 0 || EEPROM.readByte(16) == 255) {EEPROM.writeByte(16, 1);}
  EEPROM.commit();

  Serial.begin(115200);
  Serial.println();
}

//===================== End of Setup ====================================================

void loop(){ 
  
  sensorValues[4] = EEPROM.readByte(0);   // Device ID.
  sensorValues[5] = 165;                  // Voltage must be between 130 and 180 here in whole integer.
  sensorValues[6] = random(70,74);        // Temperature F;
  sensorValues[7] = random(40,100);       // Humidity %;
  sensorValues[8] = random(850,1024) / 4; // Pressure mb;
  sensorValues[9] = random(0,100);        // Light %;
  
  // Values received from all sensors used on this device and should replace random values of sensorValues array.
  
  Serial.println("sending sensor values....."); 
  long lastmillis = millis();
  WiFi.mode(WIFI_AP_STA); 
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);
  esp_wifi_set_channel(WiFiChannel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_80211_tx(WIFI_IF_AP, sensorValues, sizeof(sensorValues), true);
  
  long currentmillis = millis() - lastmillis;
  Serial.print("Transmit & receive time (Milliseconds) : ");Serial.println(currentmillis);
  
  int upTime = millis();Serial.print("Total up time (Milliseconds) : "); Serial.println(upTime);
  esp_sleep_enable_timer_wakeup(1 * 6000000);  // 60000000 for 1 minute.
  esp_deep_sleep_start();
  }

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
         Serial.println("Contents of EEPROM for this device below: ");
         EEPROM.readBytes(0, showConfig,19);for(int i=0;i<19;i++){ 
         Serial.printf("%d ", showConfig[i]);}Serial.println();
         delay(1);
    } else {
    
    Serial.println("Resending sensor values..."); 
    ESP.restart();   // Seems like gateway did not receive sensor values let's try again.
    }
  }
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

    WiFi.begin(ssid, password);
    
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
     Serial.printf("STA: Failed!\n");
     WiFi.disconnect(false);
     delay(1000);
     WiFi.begin(ssid, password);
     }
     delay(500);

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
       break;            
   }
} 
