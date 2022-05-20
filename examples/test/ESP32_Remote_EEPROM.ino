// 80 ms uptime in two way mode. Confirm and try to reduce this time.

#include <WiFi.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <ESP32httpUpdate.h>  // Install from arduino library manager
#include <EEPROM.h>

const char* ssid = "ESP";
const char* password = "";

String binFile =  "http://192.168.4.1/device_246.bin";
 
// Use any of following for devie ID ending with 6.
// 6,16,26,36,46,56,66,76,86,96,106,116,126,136,146,156,166,176,186,196,206,216,226,236,246 etc.

int device = 6;          // Unique device ID must end with 2,6,A or E. See https://serverfault.com/questions/40712/what-range-of-mac-addresses-can-i-safely-use-for-my-virtual-machines.
int apChannel = 7;       // WiFi Channel for this device.
int sleepTime = 1;       // Sleep time in minutes.
int upTime;              // Device uptime in milliseconds.
int deviceMode = 0;      // Default 

// Sensor values to be sent to Gateway
uint8_t sensorData[6];  // = {device, voltage, Sensorvalue1, Sensorvalue2, Sensorvalue3, Sensorvalue4};
int Hour, Minute;      
int warnVolt = 130;   // Start warning when battery level goes below 2.60 volts (260/2 = 130).


//============Do not need user configuration from here on============================

void setup() {
  
}  // Setup ends here

//========================Main Loop================================

void loop() {
  EEPROM.begin(30);
  if (EEPROM.readByte(18) != 255){device = EEPROM.readByte(18);} 
 //WiFi.scanDelete();  //remove previous scan data from memory
  sensorValues();
  int n = WiFi.scanNetworks(true, false, false, 5, apChannel);
  
  Serial.begin(115200);
   
  
  Serial.print("Sensors values data sent to controller: ");Serial.println(WiFi.macAddress());
  
  delay(10);  // Minimum 10 milliseonds delay required to receive message from gateway reliably.
  
  EEPROM.writeByte(0,WiFi.BSSID(0)[0]);   // Device ID at adress 0.
  EEPROM.writeByte(1,WiFi.BSSID(0)[0]);   // Device ID at address 1.
  EEPROM.writeByte(2, WiFi.BSSID(0)[1]);  // Command type        
  Serial.print(EEPROM.readByte(0));  Serial.println(EEPROM.readByte(1));Serial.print(EEPROM.readByte(2)); 
  Serial.println();
  Serial.print("Gateway Name is: ");Serial.print(WiFi.SSID(0)); Serial.print(" & Gateway Channel is: ");Serial.println(WiFi.channel(0));
  Serial.print("Command received from Gateway: ");Serial.println(&WiFi.BSSIDstr(0)[0]);
    
       if (EEPROM.readByte(2) == 101)         // Digital Write
       {
         
         EEPROM.writeByte(3,WiFi.BSSID(0)[2]);
         EEPROM.writeByte(4,WiFi.BSSID(0)[3]);
         Serial.print("Received Digital Write Command  ");Serial.print(EEPROM.readByte(3));  Serial.println(EEPROM.readByte(4));
         timeSynch();                    // Update time received from Gateway.
       
       } else if (EEPROM.readByte(2) == 102)  // Analog Write
       {
         EEPROM.writeByte(5,WiFi.BSSID(0)[2]);
         EEPROM.writeByte(6,WiFi.BSSID(0)[3]);
         Serial.print("Received Digital Analog Command  ");Serial.print(EEPROM.readByte(5));  Serial.println(EEPROM.readByte(6));

       } else if (EEPROM.readByte(2) == 103)  // Digital Read
       {
         Serial.println("Received Digital Read Command  ");
         timeSynch();
       
       } else if (EEPROM.readByte(2) == 104)  // Analog Read
       { 
         Serial.println("Received Analog Read Command  ");
         timeSynch(); 
       
       } else if (EEPROM.readByte(2) == 105 && EEPROM.readByte(3) != 0)  // Neopixel
       {
         EEPROM.writeByte(7,WiFi.BSSID(0)[2]);
         EEPROM.writeByte(8,WiFi.BSSID(0)[3]);
         EEPROM.writeByte(9,WiFi.BSSID(0)[4]);
         EEPROM.writeByte(10,WiFi.BSSID(0)[5]);
         Serial.print("Received Neopixel Command  ");Serial.print(EEPROM.readByte(7));  Serial.println(EEPROM.readByte(8));Serial.print(EEPROM.readByte(9));  Serial.println(EEPROM.readByte(10));

       } else if (EEPROM.readByte(2) == 107 && EEPROM.readByte(3) != 0)  // Save target values.
       {
         EEPROM.writeByte(11,WiFi.BSSID(0)[2]);
         EEPROM.writeByte(12,WiFi.BSSID(0)[3]);
         EEPROM.writeByte(13,WiFi.BSSID(0)[4]);
         EEPROM.writeByte(14,WiFi.BSSID(0)[5]);
         Serial.print("Received Set Target Values Command  ");Serial.println(EEPROM.readByte(11));  Serial.println(EEPROM.readByte(12));Serial.print(EEPROM.readByte(13));  Serial.println(EEPROM.readByte(14));        
       
       } else if (EEPROM.readByte(2) == 108)  // Set AP Channel
       {
        EEPROM.writeByte(15,WiFi.BSSID(0)[2]);
        Serial.print("Received Set AP Channel Command  ");Serial.println(EEPROM.readByte(15));
        apChannel = EEPROM.readByte(15);
        timeSynch();   
       
       } else if (EEPROM.readByte(2) == 109)  // Set Sleep Time
       {
        EEPROM.writeByte(16,WiFi.BSSID(0)[2]);
        Serial.print("Received Set Sleep Time Command  ");Serial.println(EEPROM.readByte(16));
        sleepTime = EEPROM.readByte(16);
        timeSynch();
       
       } else if (EEPROM.readByte(2) == 110)  // Set Device Mode
       {
         EEPROM.writeByte(17,WiFi.BSSID(0)[2]);
         Serial.print("Received Set Device Mode Command  ");Serial.println(EEPROM.readByte(17));
         timeSynch();
         deviceMode = EEPROM.readByte(17);       // Save device mode (0 for regular, 1 for autupdate and 2 for any other option).
         if (EEPROM.readByte(17) == 1) {OTAupdate();}
       
       } else if (EEPROM.readByte(2) == 111)   // Set Device ID
       {
         EEPROM.writeByte(18,WiFi.BSSID(0)[0]);
         Serial.print("Received Set Device ID Command  ");Serial.println(EEPROM.readByte(0));
         device = EEPROM.readByte(18);
         timeSynch();
       
    } else {
    Serial.print("Resending sensor values...   Set Device ID = "); Serial.println(WiFi.BSSID(0)[0]); 
    EEPROM.writeByte(1,WiFi.BSSID(0)[0]);
    esp_sleep_enable_timer_wakeup(sleepTime * 100);
    esp_deep_sleep_start();
    //ESP.restart();   // Seems like gateway did not receive sensor values let's try again.
    }       
    delay(1);
  
  gpioControl();

  upTime = (millis() + 8);  // Estimated 8 milliseconds added to account for next process in loop.
  
  Serial.print("Total time I spent before going to sleep: ");
  Serial.println(upTime);
  Serial.print("I will wakeup in: ");
  Serial.print(sleepTime);
  Serial.println(" Minutes");
  esp_sleep_enable_timer_wakeup(sleepTime * 6000000); // 60000000 for 1 minute.
  esp_deep_sleep_start();
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

void timeSynch(){
  Hour = WiFi.BSSID(0)[4];    // Hour
  Minute = WiFi.BSSID(0)[5];  // Minute
  Serial.print("Time received from Gateway: ");Serial.print(Hour); Serial.print(":"); Serial.println(Minute);
}

void gpioControl()   {

    if ((EEPROM.readByte(3) >= 1 && EEPROM.readByte(3) <= 5) || (EEPROM.readByte(3) >= 12 && EEPROM.readByte(3) <= 39))   {
      if (EEPROM.readByte(2) == 1)    {
       if (EEPROM.readByte(4) == 1)
        {
          digitalWrite(EEPROM.readByte(3), HIGH);
          Serial.print("digitalWrite");
          Serial.print("(");
          Serial.print(EEPROM.readByte(3));
          Serial.print(",");
          Serial.print(EEPROM.readByte(4));
          Serial.println(");");
          Serial.println();
          Serial.println();
        } else if (EEPROM.readByte(4) == 0)
        {
          digitalWrite(EEPROM.readByte(3), LOW);
          Serial.print("digitalWrite");
          Serial.print("(");
          Serial.print(EEPROM.readByte(3));
          Serial.print(",");
          Serial.print(EEPROM.readByte(4));
          Serial.println(");");
          Serial.println();
          Serial.println();
        } else
        {
        /*  AnalogWrite(EEPROM.readByte(5), EEPROM.readByte(6));
          Serial.print("AnalogWrite");
          Serial.print("(");
          Serial.print(EEPROM.readByte(5));
          Serial.print(",");
          Serial.print(EEPROM.readByte(6));
          Serial.println(");");
          Serial.println();
          Serial.println(); */
        }
      }
      /*
        } else if (showConfig[1] == 105)    {
          // TO DO - write function for neopixel
          analogWrite(EEPROM.readByte(5), EEPROM.readByte(6));
          Serial.print("analogWrite");
          Serial.print("(");
          Serial.print(EEPROM.readByte(5));
          Serial.print(",");
          Serial.print(EEPROM.readByte(6));
          Serial.println(");");
          Serial.println();
          Serial.println();
          }
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
                EEPROM.writeByte(1,0); 
                break;
        }
    } 
