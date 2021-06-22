// 165 ms uptime in two way mode. // try to reduce this time.
// To do : implement OTA updater code like ESP8266.
#include <WiFi.h>
#include <esp_wifi.h>

const char* ssid = "";
const char* password = "";

// use any of following for devie ID ending with 6.
// 6,16,26,36,46,56,66,76,86,96,106,116,126,136,146,156,166,176,186,196,206,216,226,236,246 etc.

int device = 26;         // Unique device ID must end with 2,6,A or E. See https://serverfault.com/questions/40712/what-range-of-mac-addresses-can-i-safely-use-for-my-virtual-machines.
int battery = 6;         // Battery voltage sensor Type.
int apChannel = 7;       // WiFi Channel for this device.
int sleepTime = 1;       // Sleep time in minutes.
int upTime;              // Device uptime in milliseconds.
int deviceMode;

// Sensor values to be sent to Gateway
uint8_t sensorData[6];  // = {device, voltage, temperature, humidity, pressure, light};

int receivedDevice;
int receivedCommand;  // digitalwrite, analogwrite, digitalRead, analogRead, neopixel, pin setup etc.
int pinNumber;        // gpio pin number 1 to 5 & 12 to 16 or value for sensorType 3.
int value1;           // 0 or 1 in case of digitalwrte, 0 to 255 in case of analogwrite or value for RED neopixel or value for sensorType 4.
int value2;           // 0 to 255 - value for GREEN neopixel or value for sensorType 5.
int value3;           // 0 to 255 - value for BLUE neopixel or value for sensorType 6.

uint8_t Date[3], Time[2];

int warnVolt = 130;   // Start warning when battery level goes below 2.60 volts (260/2 = 130).


//============Do not need user configuration from here on============================

void setup() {
  //WiFi.scanDelete();  //remove previous scan data from memory
  sensorValues();
  int n = WiFi.scanNetworks(true, false, false, 5, apChannel);
  
  Serial.begin(115200);
  Serial.print("Sensors values data sent to controller: ");
  Serial.println(WiFi.macAddress());

  delay(10);  // Minimum 10 milliseonds delay required to receive message from gateway reliably.

  receivedDevice = WiFi.BSSID(0)[0];
  receivedCommand = WiFi.BSSID(0)[1];
  
if ( receivedDevice == device )  {   //match first byte of gateway's mac id with this devices's ID here.
    if (receivedCommand > 31)
     { 
      Serial.print("Command received from Gateway: ");
      Serial.println(&WiFi.BSSIDstr(0)[0]);
      Serial.println();
      Serial.print("Gateway Name is: ");
      Serial.println(WiFi.SSID(0));
      Serial.print("Gateway Channel is: ");
      Serial.println(WiFi.channel(0));

      uint8_t* receivedData[6] =  {WiFi.BSSID(0)};

     // receivedDevice = WiFi.BSSID(0)[0];
     // receivedCommand = WiFi.BSSID(0)[1];
      pinNumber = WiFi.BSSID(0)[2];
      value1 = WiFi.BSSID(0)[3];
      value2 = WiFi.BSSID(0)[4];
      value3 = WiFi.BSSID(0)[5];
      
       if (receivedCommand == 97)
       {
         apChannel = pinNumber;
         
       } else if (receivedCommand == 98 && pinNumber != 0)
       {
         sleepTime = pinNumber;    // Save sleep time in minutes.
      
       } else if (receivedCommand == 99)
       {
         deviceMode = pinNumber;      // Save device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
       }

 }  else       //if receivedCommand is between 0 & 31.
 {
    
    Date[0] = WiFi.BSSID(0)[1];
    Date[1] = WiFi.BSSID(0)[2];
    Date[2] = WiFi.BSSID(0)[3];
    Time[0] = WiFi.BSSID(0)[4];
    Time[1] = WiFi.BSSID(0)[5];
    Serial.print("Time received from Gateway is: "); Serial.print(Date[0]); Serial.print("/"); Serial.print(Date[1]); Serial.print("/"); Serial.print(Date[2]); Serial.print(" "); Serial.print(Time[0]); Serial.print(":"); Serial.print(Time[1]);
 }
 
    delay(1);
  }
}  // Setup ends here

//========================Main Loop================================

void loop() {

  gpioControl();

  upTime = (millis() + 8);  // Estimated 8 milliseconds added to account for next process in loop.
  
  Serial.print("Total time I spent before going to sleep: ");
  Serial.println(upTime);
  Serial.print("I will wakeup in: ");
  Serial.print(sleepTime);
  Serial.println(" Minutes");
  delay(sleepTime * 6000); // 60000 for 1 minute
  ESP.restart();   // For testing only.
  //ESP.deepSleepInstant(sleepTime * 60000000, WAKE_NO_RFCAL);
}     
//=========================Main Loop ends==========================

void sensorValues() 
{
  
  sensorData[0] = device;
  sensorData[1] = 2.93;
  sensorData[2] = random(45,55);        //temperature;
  sensorData[3] = random(40,100);     //humidity;
  sensorData[4] = random(850,1024) / 4;   //pressure;
  sensorData[5] = random(30,100);        //light;
  
  esp_base_mac_addr_set(sensorData);
  
  //Functions for all sensors used on this device goes here.
  //Values received from sensors replaces 4 random values of sensorData array.
}

void gpioControl()   {
 if ( receivedDevice == device )   {
    if ((pinNumber >= 1 && pinNumber <= 5) || (pinNumber >= 12 && pinNumber <= 39))   {
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
        /*  AnalogWrite(pinNumber, value1);
          Serial.print("AnalogWrite");
          Serial.print("(");
          Serial.print(pinNumber);
          Serial.print(",");
          Serial.print(value1);
          Serial.println(");");
          Serial.println();
          Serial.println(); */
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
