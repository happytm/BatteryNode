// 120ms uptime (cpu frequency 160 mhz and core 2.51).
ADC_MODE(ADC_VCC); //vcc read-mode

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#define binFile "https://raw.githubusercontent.com/happytm/BatteryNode/master/sender.bin"
#define BOOT_AFTER_UPDATE    false
HTTPClient http;

const char* ssid = "";
const char* password = "";

// use any of following for devie ID ending with 6.
// 6,16,26,36,46,56,66,76,86,96,106,116,126,136,146,156,166,176,186,196,206,216,226,236,246 etc.

int device = 26;         // Unique device ID must end with 2,6,A or E. See https://serverfault.com/questions/40712/what-range-of-mac-addresses-can-i-safely-use-for-my-virtual-machines.
int battery = 6;         // Battery voltage sensor Type.
int apChannel = 7;       // WiFi Channel for this device.
char* gateway = "ESP";   // This name has to be same as main controller's ssid.
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
  int n = WiFi.scanNetworks(true, false, apChannel, (uint8*) gateway);

  Serial.begin(115200);
  Serial.print("Sensors values data sent to controller: ");
  Serial.println(WiFi.macAddress());

  delay(60);  // Minimum 60 milliseonds delay required to receive message from gateway reliably.

  receivedDevice = WiFi.BSSID(0)[0];
  receivedCommand = WiFi.BSSID(0)[1];
  
if (receivedDevice == device)  
 {   //match first byte of gateway's mac id with this devices's ID here.
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

  if (receivedDevice == device && receivedCommand == 99)  
  {
    otaControl();
  }

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
  float voltage = ESP.getVcc() / (float)1023 * 50; // * (float)1.07;
  sensorData[0] = device;
  sensorData[1] = voltage;
  sensorData[2] = random(45,55);        //temperature;
  sensorData[3] = random(40,100);     //humidity;
  sensorData[4] = random(850,1024) / 4;   //pressure;
  sensorData[5] = random(30,100);        //light;
  
  wifi_set_macaddr(STATION_IF, sensorData);
  
  //Functions for all sensors used on this device goes here.
  //Values received from sensors replaces 4 random values of sensorData array.
}

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
   } else if (deviceMode == 2){
  } else {
 }
}
