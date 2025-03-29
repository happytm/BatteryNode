// 45 ms transmit & receive time and 82 ms total uptime required in two way mode.Confirm and try to reduce this time.
#include <WiFi.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <ESP32httpUpdate.h>  // Install from arduino library manager
#include <EEPROM.h>
#include "driver/adc.h"

const char* ssid = "ESP";
const char* password = "";

//==================User configuration generally not required below this line ================================================

String binFile = "http://192.168.4.1/device_246.bin";

int Hour;          // Hour received from Gateway. More reliable source than internal RTC of local device
int Minute;        // Minute received from Gateway. More reliable source than internal RTC of local device.

// Sensor values to be sent to Gateway
uint8_t sensorData[6];  // = {device, voltage, Sensorvalue4, Sensorvalue4, Sensorvalue4, Sensorvalue4};
uint8_t showConfig[20]; // Content of EEPROM is saved here.

int commandType;      // digitalwrite, analogwrite, digitalRead, analogRead, neopixel, pin setup etc.
int value1;           // gpio pin number or other values like device ID, sleeptime, Ap Channel, Device mode etc.
int value2;           // 0 or 1 in case of digitalwrte, 0 to 255 in case of analogwrite or value for RED neopixel or value for sensorType 4.
int value3;           // 0 to 255 - value for GREEN neopixel or value for sensorType 5.
int value4;           // 0 to 255 - value for BLUE neopixel or value for sensorType 6.
     
int warnVolt = 130;   // Start warning when battery level goes below 2.60 volts (260/2 = 130).

void setup() {
  EEPROM.begin(20);
  
  int lastmillis = millis();
  
  if (EEPROM.readByte(0) == 255)  {EEPROM.writeByte(0, 246);} 
  if (EEPROM.readByte(15) == 255) {EEPROM.writeByte(15, 7);}
  if (EEPROM.readByte(16) == 255) {EEPROM.writeByte(16, 1);}
  EEPROM.commit();
  
  sensorValues();
  //int16_t scanNetworks(bool async = false, bool show_hidden = false, bool passive = false, uint32_t max_ms_per_chan = 300, uint8_t channel = 0, const char * ssid=nullptr, const uint8_t * bssid=nullptr);
  int n = WiFi.scanNetworks(true, false, false, 5, EEPROM.readByte(15));
    
  Serial.begin(115200);
  Serial.print("Sensors values data sent to controller: ");Serial.println(WiFi.macAddress());
  
  delay(10);  // Minimum 10 milliseonds delay required to reliably receive message from Gateway.
     
  lastmillis = millis()-lastmillis;
  Serial.println();Serial.print("Transmit & receive Time (Milliseconds):     ");Serial.println(lastmillis);    
  
  // If WiFi channels of Gateway and thid device do not match. 
  // if (WiFi.BSSID(0)[0] < 6) {sensorValues();Serial.println("Scanning multiple channels...");int n = WiFi.scanNetworks(true, false, false, 5, 0);delay(10);}

  EEPROM.writeByte(1, WiFi.BSSID(0)[1]);  // Command type at address 1. 
  EEPROM.commit();
  
  adc_power_off();     // ADC work finished so turn it off to save power.
  WiFi.mode(WIFI_OFF); // WiFi transmit & receive work finished so turn it off to save power.
  esp_wifi_stop();     // WiFi transmit & receive work finished so turn it off to save power.
  
  commandType = EEPROM.readByte(1);

  Serial.println("Contents of EEPROM for this device below: ");
  EEPROM.readBytes(0, showConfig,19);for(int i=0;i<19;i++){ 
  Serial.printf("%d ", showConfig[i]);}
      
  if ( commandType > 100 && commandType < 121)  {   // If commandType is 101 to 120.
      
      Serial.println();
      Serial.print("Gateway Name is: ");Serial.print(WiFi.SSID(0));Serial.print(" & Gateway's Wifi Channel is: ");Serial.println(WiFi.channel(0));
      Serial.print("This device's Wifi Channel is: ");Serial.println(EEPROM.readByte(15));  
      
      value1 = WiFi.BSSID(0)[2];
      value2 = WiFi.BSSID(0)[3];
      value3 = WiFi.BSSID(0)[4];
      value4 = WiFi.BSSID(0)[5];
      
      Serial.print("Command received from Gateway: ");Serial.println(&WiFi.BSSIDstr(0)[0]);
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
    //esp_sleep_enable_timer_wakeup(EEPROM.readByte(16) * 100);
    //esp_deep_sleep_start();
    ESP.restart();   // Seems like gateway did not receive sensor values let's try again.
    }
      
  }  // Setup ends here

//========================Main Loop================================

void loop() {
  
  Serial.print("I will wakeup in: ");
  Serial.print(EEPROM.readByte(16));   // Sleeptime in minutes.
  Serial.println(" Minutes");
  int upTime = (millis());  
  Serial.print("Total time I spent before going to sleep: ");
  Serial.println(upTime);
   //esp_bluedroid_disable();
   //esp_bt_controller_disable(); 
   //esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF); 
   //esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);             // see https://esp32.com/viewtopic.php?t=9681
   //esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);    // see https://esp32.com/viewtopic.php?t=9681
   //esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);       // see https://esp32.com/viewtopic.php?t=9681
  esp_sleep_enable_timer_wakeup(EEPROM.readByte(16) * 6000000);            // 60000000 for 1 minute.
  esp_deep_sleep_start();
}     
//=========================Main Loop ends==========================

void sensorValues() 
{
  sensorData[0] = EEPROM.readByte(0);
  sensorData[1] = 165;                  //voltage must be between 130 and 180 here in whole integer.
  sensorData[2] = random(70,74);        //temperature F;
  sensorData[3] = random(40,100);       //humidity %;
  sensorData[4] = random(850,1024) / 4;  //pressure mb;
  sensorData[5] = random(0,100);        //light %;
  
   esp_err_t esp_base_mac_addr_set(uint8_t *sensorData);  // https://github.com/justcallmekoko/ESP32Marauder/issues/418
   Serial.print("Probe request sent: ");for (int i = 0; i < 6; i++) {Serial.print(sensorData[i], HEX);if (i < 5)Serial.print(":");} Serial.println();

  
  //Functions for all sensors used on this device goes here.
  //Values received from sensors replaces 4 random values of sensorData array.
}

void synchTime(){
  if (commandType == 105 || commandType == 106)
  {
    Hour = EEPROM.readByte(17);                           // Hour value from local RTC memory.
    Minute = EEPROM.readByte(18) + EEPROM.readByte(16);   // Minute from local RTC memory + Sleep Time.
  } else {
    EEPROM.writeByte(17,value3);
    EEPROM.writeByte(18,value4);
    Hour = value3;     // New hour value received from Gateway.
    Minute = value4;   //  New minute value received from Gateway.
  }
  Serial.print("Time received from Gateway: ");Serial.print(Hour); Serial.print(":"); Serial.println(Minute);
}


void gpioControl()   {
 
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

void OTAupdate(){  // Receive  OTA update from bin file on Gateway's LittleFS data folder.
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
