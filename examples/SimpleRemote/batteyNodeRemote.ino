// 45 ms transmit & receive time and 82 ms total uptime required in two way mode.Confirm and try to reduce this time.
#include <WiFi.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <ESP32httpUpdate.h>  // Install from arduino library manager
#include <EEPROM.h>
//#include "driver/adc.h"

#define FIRSTTIME  false  // Define true if setting up remote device for first time.

const char* ssid = "ESP";
const char* password = "";

//==================User configuration generally not required below this line ================================================

String binFile = "http://192.168.4.1/device_246.bin";

int Hour;               // Hour received from Gateway. More reliable source than internal RTC of local device
int Minute;             // Minute received from Gateway. More reliable source than internal RTC of local device.
int apChannel;          // AP WiFi channel
uint8_t command[6];     // Command received from webpage or MQTT
uint8_t sensorData[6];  // = {device, voltage, Sensorvalue1, Sensorvalue, Sensorvalue4, Sensorvalue4};
uint8_t showConfig[20]; // Content of EEPROM is saved here.

int warnVolt = 130;   // Start warning when battery level goes below 2.60 volts (260/2 = 130).

void setup() {
  
  long lastmillis = millis();

  EEPROM.begin(20);
  
#if FIRSTTIME  
  // Setup device ID,  AP wifi Channel, device mode and sleep time in minutes for remote device in EEPROM permanantly.
  EEPROM.writeByte(0, 246); EEPROM.writeByte(14, 6); EEPROM.writeByte(15, 0); EEPROM.writeByte(16, 1);apChannel = EEPROM.writeByte(14, 7); EEPROM.commit();
#endif 
  
  WiFi.mode(WIFI_STA);
  WiFi.STA.begin();
  
  Serial.begin(115200);
  Serial.println();
  sensorValues();
  startWiFiScan();
  
  delay(10);  // ***Very Important***  Minimum 10 milliseonds delay required to reliably receive message from Gateway.
   
  Serial.print("Sensor values sent to gateway: ");Serial.println(WiFi.macAddress());

     //adc_power_off();     // ADC work finished so turn it off to save power.
  WiFi.mode(WIFI_OFF); // WiFi transmit & receive work finished so turn it off to save power.
  esp_wifi_stop();     // WiFi transmit & receive work finished so turn it off to save power.
   
  lastmillis = millis()-lastmillis;
  Serial.println();Serial.print("Setup function complted  (Milliseconds):     ");Serial.println(lastmillis);    
    
}  // Setup ends here

//========================Main Loop================================

void loop() {
  if ( WiFi.scanComplete() > 0) { printScannedNetworks(WiFi.scanComplete()); startWiFiScan(); } 
  
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
  esp_sleep_enable_timer_wakeup(EEPROM.readByte(16) * 10000000);            // 60000000 for 1 minute.
  esp_deep_sleep_start();
  
}     
//=========================Main Loop ends==========================

void startWiFiScan() { WiFi.scanNetworks(true, false, false, 5, 6, "ESP"); }

void printScannedNetworks(uint16_t networksFound) {
      
    Serial.print(networksFound);
    Serial.println(" probeResponse received. ");
    Serial.print("Command received from Gateway: ");Serial.println(&WiFi.BSSIDstr(0)[0]);
    Serial.println();
    for (int i = 0; i < networksFound ; ++i) { for (int j = 0; j < 6; j++) {
    command[j] = WiFi.BSSID(i)[j]; }}

    EEPROM.writeByte(1, command[1]);  // Save command type at EEPROM address 1. 
    EEPROM.commit(); 
    Serial.println("Contents of EEPROM for this device below: ");
    EEPROM.readBytes(0, showConfig,19);for(int i=0;i<19;i++){ 
    Serial.printf("%d ", showConfig[i]);}
      
      if ( EEPROM.readByte(1) > 100 && EEPROM.readByte(1) < 121)  {   // If commandType is 101 to 120.
      
      Serial.println();
      Serial.print("Gateway Name is: ");Serial.print(WiFi.SSID(0));Serial.print(" & Gateway's Wifi Channel is: ");Serial.println(WiFi.channel(0));
      Serial.print("This device's Wifi Channel is: ");Serial.println(EEPROM.readByte(15));  
      synchTime();

       if (EEPROM.readByte(1) == 101)        // Digital Write
       {
         EEPROM.writeByte(2,command[2]);
         EEPROM.writeByte(3,command[3]);EEPROM.commit();
         Serial.print("Received Command - Digital Write: ");Serial.print(command[2]);  Serial.println(command[3]);
       
         gpioControl();
         
       } else if (EEPROM.readByte(1) == 102) // Analog Write
       {
         EEPROM.writeByte(4,command[2]);
         EEPROM.writeByte(5,command[3]);EEPROM.commit();
         Serial.print("Received Command - Analog Write:  ");Serial.print(command[2]);  Serial.println(command[3]);

         gpioControl();
         
       } else if (EEPROM.readByte(1) == 103)  // Digital Read
       {
          Serial.println("Received Command - Digital Read pin:  ");Serial.println(command[2]);
       } else if (EEPROM.readByte(1) == 104)  // Analog Read
       { 
           Serial.println("Received Command - Digital Read pin: ");Serial.println(command[2]);
       } else if (EEPROM.readByte(1) == 105)  // Neopixel
       {
         EEPROM.writeByte(6,command[2]);
         EEPROM.writeByte(7,command[3]);
         EEPROM.writeByte(8,command[4]);
         EEPROM.writeByte(9,command[5]);EEPROM.commit();
         Serial.print("Received Command Neopixel: ");Serial.print(command[2]);  Serial.println(command[3]);Serial.print(command[4]);  Serial.println(command[5]);

         gpioControl();
         
       } else if (EEPROM.readByte(1) == 106)  // Set Targets
       {
         EEPROM.writeByte(10,command[2]);
         EEPROM.writeByte(11,command[3]);
         EEPROM.writeByte(12,command[4]);
         EEPROM.writeByte(13,command[5]);EEPROM.commit();
         Serial.print("Received Command - Set Target Values to: ");Serial.println(command[2]);  Serial.println(command[3]);Serial.print(command[4]);  Serial.println(command[5]);        
         
       } else if (EEPROM.readByte(1) == 107)  // Set AP Channel
       {
         EEPROM.writeByte(14,command[2]);EEPROM.commit();
         Serial.print("Received Command - Set AP Channel to: ");Serial.println(command[2]);
               
       } else if (EEPROM.readByte(1) == 108 && command[2] == 1)  // Set Mode
       {
         Serial.print("Received Command - Set Device Mode to: ");Serial.println(command[2]);
         EEPROM.writeByte(15,0);
         EEPROM.writeByte(0,246);EEPROM.commit();
         OTAupdate();
         
       } else if (EEPROM.readByte(1) == 109)  // Set Sleep Time
       {
         EEPROM.writeByte(16,command[2]);EEPROM.commit();
         Serial.print("Received Command - Set Sleep Time to:   ");Serial.print(command[2]);Serial.println(" minutes.");
          
       } else if (EEPROM.readByte(1) == 110)  // Set Device ID
       {
         
         EEPROM.writeByte(0,command[2]);EEPROM.commit();
         Serial.print("Received Command - Set Device ID to: ");Serial.println(command[2]);
         
        }
         
         Serial.println("Command from Gateway saved to EEPROM");
         Serial.println("Contents of EEPROM for this device below: ");
         EEPROM.readBytes(0, showConfig,19);for(int i=0;i<19;i++){ 
         Serial.printf("%d ", showConfig[i]);}Serial.println();
         delay(1);
    } else {
    
    Serial.println("Gateway did not respond - resending sensor values..."); 
    
    ESP.restart();   // Seems like gateway did not receive sensor values let's try again.
  }

    delay(10);
    WiFi.scanDelete();
  }

void sensorValues() 
{
  sensorData[0] = EEPROM.readByte(0);    // Device ID
  sensorData[1] = 165;                   // Voltage must be between 130 and 180 here in whole integer.
  sensorData[2] = random(70,74);         // Sensor 1
  sensorData[3] = random(40,100);        // Sensor 2
  sensorData[4] = random(850,1024) / 4;  // Sensor 3
  sensorData[5] = random(0,100);         // Sensor 4
  
  esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, &sensorData[0]);  //https://randomnerdtutorials.com/get-change-esp32-esp8266-mac-address-arduino/ https://github.com/justcallmekoko/ESP32Marauder/issues/418

  //Functions for all sensors used on this device goes here.
  //Values received from sensors replaces 4 random values of sensorData array.
}

void synchTime(){
  if (EEPROM.readByte(1) == 105 || EEPROM.readByte(1) == 106)
  {
    Hour = EEPROM.readByte(17);                           // Hour value from local RTC memory.
    Minute = EEPROM.readByte(18) + EEPROM.readByte(16);   // Minute from local RTC memory + Sleep Time.
  } else {
    EEPROM.writeByte(17,command[4]);
    EEPROM.writeByte(18,command[5]);
    Hour = command[4];     // New hour value received from Gateway.
    Minute = command[5];   // New minute value received from Gateway.
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

void OTAupdate(){  // Receive  OTA update from bin file on Gateway's SPIFFS/LittleFS data folder.
    WiFi.begin(ssid, password); if (WiFi.waitForConnectResult() != WL_CONNECTED) { Serial.printf("STA: Failed!\n"); WiFi.disconnect(false); delay(100); WiFi.begin(ssid, password); }
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
