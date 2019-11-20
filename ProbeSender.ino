ADC_MODE(ADC_VCC); //vcc read-mode
//#define SLEEP_SECS 15 * 60 // 15 minutes

#define DUPLEX    true   // true if two way communication required with controller (around 150 milliseconds as opposed to 60 milliseonds if false).

uint8 gateway[32] = "ESP";   // This name has to be same as main controller's ssid.
int apChannel = 7;           // This channel has to be same as main controller's channel & AP where main controller is connected for internet.
int sleepTime = 100;         // Sleep time in seconds 
// use any of following for devie ID ending with 6.
// 6,16,26,36,46,56,66,76,86,96,106,116,126,136,146,156,166,176,186,196,206,216,226,236,246 etc.

int device = 36;    // device ID must end with 2,6,A or E. See https://serverfault.com/questions/40712/what-range-of-mac-addresses-can-i-safely-use-for-my-virtual-machines.

// Values to be sent to Gateway
int voltage;
int temperature;
int humidity;
int pressure;
int light;
//int openClose = 0;    // 0 or 1
//int level;            // 0 to 255
//int presence = 0;     // 0 or 1
//int motion = 0;       // 0 to 6 (0=no motion, 1=up, 2=down, 3=right, 4=left, 5=on, 6=off).

uint8_t sensorData[6] = {device, voltage, temperature, humidity, pressure, light};
uint8_t sensorType[6] = {device, 6, 16, 26, 36, 46}; // Change last 4 bytes according to sensot type used.
// Predefined sensor types table is below:
// volatage = 6, temperature = 16, humidity= 26, pressure= 36, light= 46, OpenClose = 56,
// level = 66, presence = 76, motion = 86 etc.

//uint8_t securityCode[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Security code must be same at controller to compare.
int receivedDevice;
int receivedCommand;  // digitalwrite, analogwrite, pin setup etc.
int pinNumber;              // gpio pin number 1 to 16
int pinValue;         // 0 or 1 in case of digitalwrte or 0 to 255 in case of analogwrite.
int setSleeptime;  // 0 to 255 command to set sleep time variable in minutes for this device.
int setDefines;    // Set Defines (whether to run ertain code to run WiFi & OTA options etc.).

int ledPin = 12;
//int lowVolt = 0;
//int highVolt = 330;   // highest voltage of battery used multiply by 100.
int warnVolt = 130;    // Start warning when battery level goes below 2.60 volts (260/2 = 130).
unsigned long lastMillis;
unsigned long passedMillis;


#include <ESP8266WiFi.h>


//============Do not need user configuration from here on============================

void setup() {
  
  // WiFi.disconnect();
 // WiFi.scanDelete();
  Serial.begin(115200);
 /*
  wifi_set_macaddr(STATION_IF, securityCode);
  probeRequest();
  Serial.print("Security Code sent to controller: ");
  Serial.println(WiFi.macAddress());
  */
  wifi_set_macaddr(STATION_IF, sensorType);
   probeRequest();
  Serial.print("sensorType values sent to controller: ");
  Serial.println(WiFi.macAddress());
  
  sensorValues();
  probeRequest();
  //Serial.print("Sensor values sent to controller: ");
  //Serial.println(WiFi.macAddress());

#if DUPLEX
  delay(60);  // Minimum 60 milliseonds delay required to receive message from controller reliably.

  receivedDevice = WiFi.BSSID(0)[0];
  
//  if (WiFi.BSSIDstr(0)[16] == '1')  {   //match last digit of gateway's mac id with this devices's ID here.
  if (receivedDevice == device)  {   //match first byte of gateway's mac id with this devices's ID here.

    Serial.println();
    Serial.print("This Device MAC ID is: ");
    Serial.println(WiFi.macAddress());
    Serial.print("This Device Name is: ");
    Serial.println(WiFi.hostname());
    Serial.print("Gateway Name is: ");
    Serial.println(WiFi.SSID(0));
    Serial.print("Gateway Channel is: ");
    Serial.println(WiFi.channel(0));
    Serial.print("Message sent to Controller is: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Message received from Controller is: ");
    Serial.println(&WiFi.BSSIDstr(0)[0]);
    
    uint8_t* receivedData[6]=  {WiFi.BSSID(0)};
   
    receivedDevice = WiFi.BSSID(0)[0];
    Serial.println(receivedDevice);
    receivedCommand = WiFi.BSSID(0)[1];
    Serial.println(receivedCommand);
    pinNumber = WiFi.BSSID(0)[2];
    Serial.println(pinNumber);
    pinValue = WiFi.BSSID(0)[3];
    Serial.println(pinValue);
    setSleeptime = WiFi.BSSID(0)[4];
    Serial.println(setSleeptime);
    setDefines = WiFi.BSSID(0)[5];
    Serial.println(setDefines);
  
   
      
     
  } else {
    Serial.println("Message from controller did not arrive, let me try again to get message data........................................");
    ESP.restart();

   }

#endif

  delay(1);
  
  WiFi.hostname("Livingroom");
  Serial.println();
}

// Setup ends here

//========================Main Loop================================

void loop() {

  //  probeRequest();
  //  yield();
   #if DUPLEX
 
   if (pinNumber >= 0 && pinNumber <= 5)   {
     
     if (pinValue >= 2 && pinValue <= 256) { 
     
      Serial.print("analogWrite");
      Serial.print("(");
      Serial.print(pinNumber);
      Serial.print(",");
      Serial.print(pinValue);
      Serial.println(");");
      Serial.println();
      Serial.println();
      
        } else if (pinValue == 0 || pinValue == 1)  {
   
      Serial.print("digitalWrite");
      Serial.print("(");
      Serial.print(pinNumber);
      Serial.print(",");
      Serial.print(pinValue);
      Serial.println(");");
      Serial.println();
      Serial.println();
        }
    } else if (pinNumber >= 11 && pinNumber <= 16) {
        
      if (pinValue >= 2 && pinValue <= 256) { 
     
      Serial.print("analogWrite");
      Serial.print("(");
      Serial.print(pinNumber);
      Serial.print(",");
      Serial.print(pinValue);
      Serial.println(");");
      Serial.println();
      Serial.println();
      
        } else if (pinValue == 0 || pinValue == 1)  {
    
      Serial.print("digitalWrite");
      Serial.print("(");
      Serial.print(pinNumber);
      Serial.print(",");
      Serial.print(pinValue);
      Serial.println(");");
      Serial.println();
      Serial.println();
        }
    }
    #endif

  Serial.print("Total time I spent before going to sleep: ");
  Serial.println(millis());
  Serial.print("I will wakeup in: ");
  Serial.print(sleepTime);
  Serial.println(" Seconds");

  delay(5000);         //disable delay when deep sleep activated.
  Serial.println("Going to Deep Sleep..........................");
  ESP.restart();
  //ESP.deepSleep(0); //If last digit of MAC ID matches to device ID go to deep sleep else loop through again.
}

//=========================Main Loop ends==========================


void sensorValues()     {


  // voltage = random(100);
  float voltage = ESP.getVcc() / (float)1023 * 50; // * (float)1.07;
  //voltage = map(voltage, lowVolt, highVolt, 0, 100);

  Serial.print("Voltage: "); Serial.print(voltage); Serial.print("  Minimum Voltage Required: "); Serial.println(warnVolt);
  if (voltage < warnVolt)      // if voltage of battery gets to low, print the warning below.
  {
    Serial.println("Warning :- Battery Voltage low please change batteries" );
    Serial.println();
  }

  temperature = random(90);
  humidity = random(100);
  pressure = random(1024);
  pressure = pressure / 4;
  light = random(100);
  //sensorData[0] = device;
  sensorData[1] = voltage;
  sensorData[2] = temperature;
  sensorData[3] = humidity;
  sensorData[4] = pressure;
  sensorData[5] = light;
  wifi_set_macaddr(STATION_IF, sensorData);

}


void gotoSleep() {      //need connection between GPIO16 and reset pin of ESP8266
  // add some randomness to avoid collisions with multiple devices
  int sleepSecs = sleepTime;// + ((uint8_t)RANDOM_REG32/2);
  Serial.printf("Up for %i ms, going to sleep for %i secs...\n", millis(), sleepSecs);
  ESP.deepSleep(sleepSecs * 1000000, RF_NO_CAL);
  ////ESP.deepSleep( SLEEPTIME, WAKE_RF_DISABLED );
}


//=========================Probe request function starts===========


void probeRequest()  {
  /*
    Serial.println("Starting Probe sender");
    Serial.println("Sending sensor data over Probe request protocol to Master Node");
    Serial.println();
  */
  //int8_t scanNetworks(bool async = true, bool show_hidden = false, uint8 channel = 0, uint8* ssid = NULL);
 int n = WiFi.scanNetworks(true, false, apChannel, (uint8*) gateway);

  yield();

  Serial.println();
  WiFi.scanDelete();
  passedMillis = millis() - lastMillis;
  Serial.print("Time spent on Probe Request: ");
  Serial.println(passedMillis);
  lastMillis = millis();
  Serial.println();
}
