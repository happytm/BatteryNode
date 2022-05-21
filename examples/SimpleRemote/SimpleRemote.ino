// 80 ms uptime in two way mode.Confirm and try to reduce this time.

#include <WiFi.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <ESP32httpUpdate.h>  // Install from arduino library manager
#include <EEPROM.h>

const char* ssid = "ESP";
const char* password = "";

//==================User configuration generally not required below this line ================================================

String binFile = "http://192.168.4.1/device_246.bin";

int device;        // Unique device ID must end with 2,6,A or E. See https://serverfault.com/questions/40712/what-range-of-mac-addresses-can-i-safely-use-for-my-virtual-machines.
int apChannel;     // WiFi Channel for this device.
int sleepTime;     // Sleep time in minutes.
int upTime;        // Device uptime in milliseconds.
int deviceMode;    // Device Mode. Default is 0.
int target1, target2, target3, target4; // Targets set via web interface which can be used for automations locally.
int neopixelValue1,neopixelValue2,neopixelValue3,neopixelValue4;
int digitalWriteValue1,digitalWriteValue2;
int analogWriteValue1, analogWriteValue2;
int Hour;          // Hour received from Gateway. More reliable source than internal RTC of local device
int Minute;        // Minute received from Gateway. More reliable source than internal RTC of local device.

// Sensor values to be sent to Gateway
uint8_t sensorData[6];  // = {device, voltage, Sensorvalue4, Sensorvalue4, Sensorvalue4, Sensorvalue4};
uint8_t showConfig[20]; // Content of EEPROM is saved here.

//int receivedDevice;   // Device ID.
int commandType;      // digitalwrite, analogwrite, digitalRead, analogRead, neopixel, pin setup etc.
int value1;           // gpio pin number or other values like device ID, sleeptime, Ap Channel, Device mode etc.
int value2;           // 0 or 1 in case of digitalwrte, 0 to 255 in case of analogwrite or value for RED neopixel or value for sensorType 4.
int value3;           // 0 to 255 - value for GREEN neopixel or value for sensorType 5.
int value4;           // 0 to 255 - value for BLUE neopixel or value for sensorType 6.
     
int warnVolt = 130;   // Start warning when battery level goes below 2.60 volts (260/2 = 130).


//============Do not need user configuration from here on============================

void setup() {
  EEPROM.begin(30);
  
  if (EEPROM.readByte(0) == 255)  {device = 246;}  else {device = EEPROM.readByte(18);}
  if (EEPROM.readByte(15) == 255) {apChannel = 7;} else {apChannel = EEPROM.readByte(15);}
  if (EEPROM.readByte(16) == 255) {sleepTime = 1;} else {sleepTime = EEPROM.readByte(16);}
  
  //WiFi.scanDelete();  //remove previous scan data from memory
  sensorValues();
  int n = WiFi.scanNetworks(true, false, false, 5, apChannel);
     
  Serial.begin(115200);
  Serial.print("Sensors values data sent to controller: ");Serial.println(WiFi.macAddress());
  
  delay(10);  // Minimum 10 milliseonds delay required to reliably receive message from Gateway.
  
  EEPROM.writeByte(0,WiFi.BSSID(0)[0]);   // Device ID at address 1.
  EEPROM.writeByte(1, WiFi.BSSID(0)[1]);  // Command type  
  EEPROM.commit();
  //device = EEPROM.readByte(0);
  commandType = EEPROM.readByte(1);

  Serial.println("Contents of EEPROM for this device below: ");
  EEPROM.readBytes(0, showConfig,21);for(int i=0;i<21;i++){ 
  Serial.printf("%d ", showConfig[i]);
  }
      
  if ( commandType > 100 && commandType < 121)  {   // If commandType is 101 to 120.
      
      Serial.println();
      Serial.print("Gateway Name is: ");Serial.print(WiFi.SSID(0));Serial.print(" & Gateway's Wifi Channel is: ");Serial.println(WiFi.channel(0));
      Serial.print("This device's Wifi Channel is: ");Serial.println(apChannel);  
      
      value1 = WiFi.BSSID(0)[2];
      value2 = WiFi.BSSID(0)[3];
      value3 = WiFi.BSSID(0)[4];
      value4 = WiFi.BSSID(0)[5];
      
      Serial.print("Command received from Gateway: ");Serial.println(&WiFi.BSSIDstr(0)[0]);
      synchTime();
       
       if (commandType == 101)        // Digital Write
       {
         EEPROM.writeByte(3,value1);
         EEPROM.writeByte(4,value2);
         Serial.print("Received Digital Write Command  ");Serial.print(EEPROM.readByte(3));  Serial.println(EEPROM.readByte(4));

         gpioControl();
         
       } else if (commandType == 102) // Analog Write
       {
         EEPROM.writeByte(5,value1);
         EEPROM.writeByte(6,value2);
         Serial.print("Received Analog Write Command  ");Serial.print(EEPROM.readByte(5));  Serial.println(EEPROM.readByte(6));

         gpioControl();
       } else if (commandType == 103)  // Digital Read
       {
          Serial.println("Received Digital Read pin:  ");
       } else if (commandType == 104)  // Analog Read
       { 
           Serial.println("Received Digital Read pin: ");
       } else if (commandType == 105)  // Neopixel
       {
         EEPROM.writeByte(7,value1);
         EEPROM.writeByte(8,value2);
         EEPROM.writeByte(9,value3);
         EEPROM.writeByte(10,value4);
         Serial.print("Received Neopixel Command  ");Serial.print(EEPROM.readByte(7));  Serial.println(EEPROM.readByte(8));Serial.print(EEPROM.readByte(9));  Serial.println(EEPROM.readByte(10));

         gpioControl();
       } else if (commandType == 107)  // Set Targets
       {
         EEPROM.writeByte(11,value1);
         EEPROM.writeByte(12,value2);
         EEPROM.writeByte(13,value3);
         EEPROM.writeByte(14,value4);
         Serial.print("Received Set Target Values Command  ");Serial.println(EEPROM.readByte(11));  Serial.println(EEPROM.readByte(12));Serial.print(EEPROM.readByte(13));  Serial.println(EEPROM.readByte(14));        

       } else if (commandType == 108)  // Set AP Channel
       {
         EEPROM.writeByte(15,value1);
         Serial.print("Received Set AP Channel Command  ");Serial.println(EEPROM.readByte(15));
         apChannel = EEPROM.readByte(15);  
       } else if (commandType == 109)  // Set Sleep Time
       {
         EEPROM.writeByte(16,value1);
         Serial.print("Received Set Sleep Time Command  ");Serial.println(EEPROM.readByte(16));
         sleepTime = EEPROM.readByte(16);
       } else if (commandType == 110)  // Set Mode
       {
         EEPROM.writeByte(17,value1);
         Serial.print("Received Set Device Mode Command  ");Serial.println(EEPROM.readByte(17));

         deviceMode = EEPROM.readByte(17);         // Save device mode (0 for regular, 1 for autupdate and 2 for any other option).
         if (deviceMode == 1) {OTAupdate();}
        
       } else if (commandType == 111)  // Set Device ID
       {
         
         EEPROM.writeByte(18,value1);
         Serial.print("Received Set Device ID Command  ");Serial.println(EEPROM.readByte(0));
         device = EEPROM.readByte(18);
        }
         EEPROM.commit();Serial.println();Serial.println("Command from Gateway saved to EEPROM");
    delay(1);
  } else {
    
    Serial.println("Resending sensor values..."); 
    esp_sleep_enable_timer_wakeup(sleepTime * 100);
    esp_deep_sleep_start();
    ESP.restart();   // Seems like gateway did not receive sensor values let's try again.
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
    Hour = EEPROM.readByte(19);                           // Hour value from local RTC memory.
    Minute = EEPROM.readByte(20) + EEPROM.readByte(16);   // Minute from local RTC memory + Sleep Time.
  } else {
    EEPROM.writeByte(19,value3);
    EEPROM.writeByte(20,value4);
    Hour = value3;     // New hour value received from Gateway.
    Minute = value4;   //  New minute value received from Gateway.
  }
  Serial.print("Time received from Gateway: ");Serial.print(Hour); Serial.print(":"); Serial.println(Minute);
}


void gpioControl()   {
 
    if ((EEPROM.readByte(3) >= 1 && EEPROM.readByte(3) <= 5) || (EEPROM.readByte(3) >= 12 && EEPROM.readByte(3) <= 39))   
    {if (EEPROM.readByte(4) == 1){digitalWrite(EEPROM.readByte(3), HIGH);} else if (EEPROM.readByte(3) == 0){digitalWrite(EEPROM.readByte(3), LOW);}
      /*    
        } else if (commandType == 102){
           analogWrite(EEPROM.readByte(5), EEPROM.readByte(6));
         
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
