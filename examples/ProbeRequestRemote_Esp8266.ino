// 140ms uptime in duplex mode & 80ms in one way mode.
ADC_MODE(ADC_VCC); //vcc read-mode

#define DUPLEX            true    // true if two way communication required with controller (around 140 milliseconds of uptime as opposed to 80 milliseonds if false).

#include <ESP8266WiFi.h>

#if DUPLEX

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#define binFile "https://raw.githubusercontent.com/happytm/BatteryNode/master/sender.ino.d1_mini.bin"
#define BOOT_AFTER_UPDATE    false
HTTPClient http;

const char* ssid = "";
const char* password = "";
#endif

// use any of following for devie ID ending with 6.
// 6,16,26,36,46,56,66,76,86,96,106,116,126,136,146,156,166,176,186,196,206,216,226,236,246 etc.

int device = 6;         // Unique device ID must end with 2,6,A or E. See https://serverfault.com/questions/40712/what-range-of-mac-addresses-can-i-safely-use-for-my-virtual-machines.
int battery = 6;         // Battery voltage sensor Type.
int apChannel = 7;       // WiFi Channel for this device.It must be same as gateway apChannel.
char* gateway = "ESP";   // This name has to be same as main controller's ssid.
int sleepTime = 1;       // Sleep time in minutes.
int upTime;              // Device uptime in milliseconds.
int deviceMode = 0;      // 0 for regular, 1 for autupdate and 2 for AutoConnect.
int deviceIP = device;   // last part of this device's fixed IP

/*
  int sensorType1 = 36;// Predefined sensor type table is below:
  int sensorType2 = 06;// Predefined sensor type table is below:
  int sensorType3 = 16;// volatage = 6, temperature = 16, humidity= 26,
  int sensorType4 = 26;// pressure= 36, light= 46, OpenClose = 56, level = 66,
  int sensorType5 = 36;// presence = 76, motion = 86, rain = 96 etc.
  int sensorType6 = 46;// volatage = 6, temperature = 16, humidity= 26,
*/

// Sensor types to be sent to Gateway

//uint8_t sensorType[6] = {device, battery, 46, 36, 26, 16}; // Change last 4 bytes according to sensor type used.

// Sensor values to be sent to Gateway

uint8_t sensorData[6];  // = {device, voltage, temperature, humidity, pressure, light};


// Status values to be sent to Gateway

uint8_t deviceStatus[6];  // {device, deviceMode, deviceIP, wifiChannel, sleepTime, random(255)}


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

  WiFi.scanDelete();  //remove previous scan data from memory
  Serial.begin(115200);
  /*
  wifi_set_macaddr(STATION_IF, sensorType);
  probeRequest();
  Serial.print("Sensor Types sent to controller: ");
  Serial.println(WiFi.macAddress());
  */
  sensorValues();
  probeRequest();
  Serial.print("Sensors values sent to controller: ");
  Serial.println(WiFi.macAddress());


#if DUPLEX
  delay(60);  // Minimum 60 milliseonds delay required to receive message from controller reliably.

  receivedDevice = WiFi.BSSID(0)[0];
  receivedCommand = WiFi.BSSID(0)[1];
  
if (receivedDevice == device)  
 {   //match first byte of gateway's mac id with this devices's ID here.
    if (receivedCommand > 31)
     { 
      Serial.print("Command received from Gateway: ");
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
 
#endif

 delay(1);

  }
}      // Setup ends here

//========================Main Loop================================

void loop() {

#if DUPLEX

  gpioControl();

  if (receivedDevice == device && receivedCommand == 99)  
  {
    otaControl();
  }

#endif

  upTime = (millis() + 8);  // Estimated 8 milliseconds added to account for next process in loop.
  
  //sendStatus();
  
  Serial.print("Total time I spent before going to sleep: ");
  Serial.println(upTime);
  Serial.print("I will wakeup in: ");
  Serial.print(sleepTime);
  Serial.println(" Minutes");
  delay(15000); ESP.restart();   // For testing only.
  //ESP.deepSleepInstant(sleepTime * 60000000, WAKE_NO_RFCAL); //If last digit of MAC ID matches to device ID go to deep sleep else loop through again.

}     // end of main loop.
//=========================Main Loop ends==========================


//=========================Probe request function starts===========


void probeRequest()  
{

  int n = WiFi.scanNetworks(true, false, apChannel, (uint8*) gateway);

  yield();

  Serial.println();
  WiFi.scanDelete();

}

//=========================Probe request function ends===========


void sensorValues() 
{

  float voltage = ESP.getVcc() / (float)1023 * 50; // * (float)1.07;

  sensorData[0] = device;
  sensorData[1] = voltage;
  sensorData[2] = random(100);         //temperature;
  sensorData[3] = random(30,100);        //humidity;
  sensorData[4] = random(1024) / 4;   //pressure;
  sensorData[5] = random(100);        //light;
  
  wifi_set_macaddr(STATION_IF, sensorData);
  
  //Functions for all sensors used on this device goes here.
  //Values received from sensors replaces 4 random values of sensorData array.
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
  Serial.print("Device Mode set to: ");
  Serial.println(deviceMode);  // // Show device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
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
    t_httpUpdate_return ret = ESPhttpUpdate.update(binFile, "", "CC AA 48 48 66 46 0E 91 53 2C 9C 7C 23 2A B1 74 4D 29 9D 33");
    deviceMode = 0;     // Set device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).

    Serial.print("Device Mode set to: ");
    Serial.println(deviceMode);

    http.end();

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        deviceMode = 2;   // Set device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
        Serial.print("Device Mode set to: ");
        Serial.println(deviceMode);
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        deviceMode = 2;     // Set device mode (0 for regular, 1 for autupdate and 2 for AutoConnect).
        Serial.print("Device Mode set to: ");
        Serial.println(deviceMode);
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        delay(5000);                    // wait for few seconds issue command with payload 36/09/00/.
        Serial.print("Device Mode set to: ");
        Serial.println(deviceMode);
        deviceMode = 0;
        ESP.restart();
        break;

      default:
        Serial.printf("Undefined HTTP_UPDATE function: "); Serial.println(ret);
    }

  } else if (deviceMode == 2)

  {


  } else {


  }
}

#endif

void sendStatus() {

  deviceStatus[0] = device;
  deviceStatus[1] = deviceMode;     // 0 for regular, 1 for autupdate and 2 for AutoConnect.
  deviceStatus[2] = deviceIP;       // Last part of this device's fixed IP (same as device ID).
  deviceStatus[3] = apChannel;      // WiFi Channel for this device.
  deviceStatus[4] = sleepTime;      // Sleep time in minutes for this device.
  deviceStatus[5] = upTime;         // Device upTime in milliseconds.
  
  wifi_set_macaddr(STATION_IF, deviceStatus);
  probeRequest();
  
  Serial.print("Device status values sent to Gateway: ");
  Serial.println(WiFi.macAddress());
}
