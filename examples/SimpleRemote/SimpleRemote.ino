// 80 ms uptime in two way mode.Confirm and try to reduce this time.

#include <WiFi.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <ESP32httpUpdate.h>  // Install from arduino library manager

const char* ssid = "ESP";
const char* password = "";

//==================User configuration generally not required below this line ================================================

String binFile = "http://192.168.4.1/device_246.bin";

RTC_DATA_ATTR int Hour;          // Hour received from Gateway. More reliable source than internal RTC of local device. 
RTC_DATA_ATTR int Minute;        // Minute received from Gateway. More reliable source than internal RTC of local device.
RTC_DATA_ATTR int device;        // Unique device ID must end with 2,6,A or E. See https://serverfault.com/questions/40712/what-range-of-mac-addresses-can-i-safely-use-for-my-virtual-machines.
RTC_DATA_ATTR int apChannel;     // WiFi Channel for this device.
RTC_DATA_ATTR int sleepTime;     // Sleep time in minutes.
RTC_DATA_ATTR int upTime;        // Device uptime in milliseconds.
RTC_DATA_ATTR int deviceMode;    // Device Mode. Default is 0.
RTC_DATA_ATTR int target1, target2, target3, target4; // Targets set via web interface which can be used for automations locally.
RTC_DATA_ATTR int neopixelValue1,neopixelValue2,neopixelValue3,neopixelValue4;
RTC_DATA_ATTR int digitalWriteValue1,digitalWriteValue2;
RTC_DATA_ATTR int analogWriteValue1, analogWriteValue2;

// Sensor values to be sent to Gateway
uint8_t sensorData[6];  // = {device, voltage, Sensorvalue4, Sensorvalue4, Sensorvalue4, Sensorvalue4};

//int receivedDevice;   // Device ID.
int commandType;      // digitalwrite, analogwrite, digitalRead, analogRead, neopixel, pin setup etc.
int value1;           // gpio pin number or other values like device ID, sleeptime, Ap Channel, Device mode etc.
int value2;           // 0 or 1 in case of digitalwrte, 0 to 255 in case of analogwrite or value for RED neopixel or value for sensorType 4.
int value3;           // 0 to 255 - value for GREEN neopixel or value for sensorType 5.
int value4;           // 0 to 255 - value for BLUE neopixel or value for sensorType 6.
     
int warnVolt = 130;   // Start warning when battery level goes below 2.60 volts (260/2 = 130).


//============Do not need user configuration from here on============================

void setup() {
  
  if (device == 0) {device = 246;}
  if (apChannel == 0) {apChannel = 7;}
  if (sleepTime == 0) {sleepTime = 1;}
  
  //WiFi.scanDelete();  //remove previous scan data from memory
  sensorValues();
  int n = WiFi.scanNetworks(true, false, false, 5, apChannel);
     
  Serial.begin(115200);
  Serial.print("Sensors values data sent to controller: ");Serial.println(WiFi.macAddress());
  
  delay(10);  // Minimum 10 milliseonds delay required to reliably receive message from Gateway.
    
  device = WiFi.BSSID(0)[0];
  commandType = WiFi.BSSID(0)[1];
  
  if ( commandType > 100 && commandType < 121)  {   // If commandType is 101 to 120.
    
      Serial.print("Gateway Name is: ");Serial.print(WiFi.SSID(0)); Serial.print(" & Gateway Channel is: ");Serial.println(WiFi.channel(0));
         
      value1 = WiFi.BSSID(0)[2];
      value2 = WiFi.BSSID(0)[3];
      value3 = WiFi.BSSID(0)[4];
      value4 = WiFi.BSSID(0)[5];
      
      Serial.print("Command received from Gateway: ");Serial.println(&WiFi.BSSIDstr(0)[0]);
      synchTime();
       
       if (commandType == 101)        // Digital Write
       {
         gpioControl();
         
       } else if (commandType == 102) // Analog Write
       {
         gpioControl();
       } else if (commandType == 103)  // Digital Read
       {
         
       } else if (commandType == 104)  // Analog Read
       { 
          
       } else if (commandType == 105)  // Neopixel
       {
         
         gpioControl();
       } else if (commandType == 107)  // Set Targets
       {
          target1 = value1;
          target2 = value2;
          target3 = value3;
          target4 = value4;
       } else if (commandType == 108)  // Set AP Channel
       {
         apChannel = value1;  
       } else if (commandType == 109)  // Set Sleep Time
       {
         sleepTime = value1;
       } else if (commandType == 110)  // Set Mode
       {
         
         deviceMode = value1;          // Save device mode (0 for regular, 1 for autupdate and 2 for any other option).
         if (deviceMode == 1) {OTAupdate();}
        
       } else if (commandType == 111)  // Set Device ID
       {
         
         device = value1;              // Save device ID.
        }
    delay(1);
  } else {
    Serial.println("Resending sensor values..."); 
    esp_sleep_enable_timer_wakeup(sleepTime * 100);
    esp_deep_sleep_start();
    //ESP.restart();   // Seems like gateway did not receive sensor values let's try again.
    }
      
  }  // Setup ends here

//========================Main Loop================================

void loop() {

  

  upTime = (millis() + 8);  // Estimated 8 milliseconds added to account for next process in loop.
  
  Serial.print("Total time I spent before going to sleep: ");
  Serial.println(upTime);
  Serial.print("I will wakeup in: ");
  Serial.print(sleepTime);
  Serial.println(" Minutes");
  esp_sleep_enable_timer_wakeup(sleepTime * 6000000); // 60000000 for 1 minute.
  esp_deep_sleep_start();
  //delay(sleepTime * 60000); // 60000 for 1 minute
  //ESP.restart();   // For testing only replace with proper sleep mode command.  
}     
//=========================Main Loop ends==========================

void sensorValues() 
{
  
  sensorData[0] = device;
  sensorData[1] = 2.93;
  sensorData[2] = random(45,55);        //temperature;
  sensorData[3] = random(40,100);       //humidity;
  sensorData[4] = random(850,1024) / 4; //pressure;
  sensorData[5] = random(30,100);       //light;
  
  esp_base_mac_addr_set(sensorData);
  
  //Functions for all sensors used on this device goes here.
  //Values received from sensors replaces 4 random values of sensorData array.
}

void synchTime(){
  if (commandType == 105 || commandType == 107)
  {
    Hour = Hour;              // Hour value from local RTC memory.
    Minute = Minute + sleepTime;  // Minute from local RTC memory + Sleep Time.
  } else {
    Hour = value3;     // New hour value received from Gateway.
    Minute = value4;  //  New minute value received from Gateway.
  }
  Serial.print("Time received from Gateway: ");Serial.print(Hour); Serial.print(":"); Serial.println(Minute);
}


void gpioControl()   {
 
    if ((value1 >= 1 && value1 <= 5) || (value1 >= 12 && value1 <= 39))   
    {if (value2 == 1){digitalWrite(value1, HIGH);} else if (value1 == 0){digitalWrite(value1, LOW);}
      /*    
        } else if (commandType == 102){
           analogWrite(value1, value2);
         
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
                deviceMode = 0; 
                break;
        }
    } 
