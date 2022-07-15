#include <WiFi.h>                           // Built in arduino librrary.
#include <esp_wifi.h>                       // Built in arduino librrary.
#include <HTTPClient.h>                     // Built in arduino librrary.
#include <ESP32httpUpdate.h>                // Install from arduino library manager
#include <EEPROM.h>                         // Built in arduino librrary.
#include "driver/adc.h"                     // required to turn off ADC.
#include <esp_bt.h>                         // required to turn off BT.
#include "motionDetector.h"                 // Thanks to https://github.com/paoloinverse/motionDetector_esp
//#include <ESP32Ping.h>                      // Thanks to https://github.com/marian-craciunescu/ESP32Ping
//#include <SimpleKalmanFilter.h>             // Built in arduino library. Reference : https://github.com/denyssene/SimpleKalmanFilter

#define PINGABLE      false                  // If true use ESPPing library to detect presence of known devices.

#if PINGABLE
#include <ESP32Ping.h>                      // Thanks to https://github.com/marian-craciunescu/ESP32Ping
const char* routerSSID = "";       // Main router's SSID.
const char* routerPassword = "";    // Main router's password.
#endif


const char* room = "Livingroom";            // Needed for devices locator.Each location must run probeReceiver sketch to implement devices locator.
int rssiThreshold = -50;                    // Adjust according to signal strength by trial & error.
int motionThreshold = 40;                   // Adjust the sensitivity of motion sensor.Higher the number means less sensetive motion sensor is.

int WiFiChannel = 7;                        // This must be same for all devices on network.
const char* ssid = "ESP";                   // Required for OTA update & motion detection.
const char* password = "";                  // Required for OTA update & motion detection.

int enableCSVgraphOutput = 1;               // 0 disable, 1 enable.If enabled, you may use Tools-> Serial Plotter to plot the variance output for each transmitter.
int dataInterval;                           // Interval in minutes to send data.
int pingInterval;                           // interval in minutes to ping known devices.
int motionLevel = -1;
//int kalmanMotion = 0;
float receivedRSSI = 0;

/*
#if PINGABLE
IPAddress deviceIP(192, 168, 0, 2);         // Fixed IP address assigned to family member's devices to be checked for their presence at home.
//IPAddress deviceIP = WiFi.localIP();
int device1IP = 2, device2IP = 3, device3IP = 4, device4IP = 5;
#endif   //#if PINGABLE


uint8_t device1[3] = {0xD0, 0xC0, 0x8A};  // Device1. First and last 2 bytes of Mac ID of Device 1.
uint8_t device2[3] = {0x3C, 0x1C, 0x20};  // Device2. First and last 2 bytes of Mac ID of Device 2.
uint8_t device3[3] = {0x36, 0x33, 0x33};  // Device3. First and last 2 bytes of Mac ID of Device 3.
uint8_t device4[3] = {0x36, 0x33, 0x33};  // Device4. First and last 2 bytes of Mac ID of Device 4.
*/
//==================User configuration generally not required below this line ============================

String binFile = "http://192.168.4.1/device_246.bin";

int Month, Date, Hour, Minute, Second;      // Time synch received from Gateway stored here for further time based automation. More reliable source than internal RTC of local device

uint8_t showConfig[20]; // Content of EEPROM is saved here.

int commandType;        // digitalwrite, analogwrite, digitalRead, analogRead, neopixel, pin setup etc.
int value1;             // gpio pin number or other values like device ID, sleeptime, Ap Channel, Device mode etc.
int value2;             // 0 or 1 in case of digitalwrte, 0 to 255 in case of analogwrite or value for RED neopixel or value for sensorType 4.
int value3;             // 0 to 255 - value for GREEN neopixel or value for sensorType 5.
int value4;             // 0 to 255 - value for BLUE neopixel or value for sensorType 6.

uint8_t sensorValues[] =                // Looks like 24 bytes is minimum (sending as WIFI_IF_AP) and 1500 bytes is maximum limit.
{
  0x80, 0x00,                           //  0- 1:  First byte here must be 80 for Type = Beacon.
  0x00, 0x00,                           //  2- 3:  Can it be used to send more data to gateway?
  0xF6, 0x11, 0x11, 0x11, 0x11, 0x11,   //  4- 9:  First byte here must be device ID (default F6 for device ID 246).Second byte is voltage value.Fill rest with any 4 types of sensor data.
  0x06, 0x22, 0x22, 0x22, 0x22, 0x22,   //  10-15: Unknown device's MAC.
  0x33, 0x33, 0x33, 0x33, 0x33, 0x33,   //  16-21: Motion level, unknown device's RSSI, device1 ping time, device2 ping type, device3 ping time, device4 ping time.
  0x00, 0x00,                           //  22-23: Can it be used to send more data to remote device?
};

void setup() {

   
  if (EEPROM.readByte(0) == 0 || EEPROM.readByte(0) == 255)  {EEPROM.writeByte(0, 246);} 
  if (EEPROM.readByte(15) < 1 || EEPROM.readByte(15) > 14) {EEPROM.writeByte(15, WiFiChannel);} 
  EEPROM.writeByte(16, 1);
  EEPROM.commit(); 
  Serial.println("Contents of EEPROM for this device below: "); EEPROM.readBytes(0, showConfig, 19); for (int i = 0; i < 19; i++) {Serial.printf("%d ", showConfig[i]);}


  motionDetector_init(); motionDetector_config(64, 16, 3, 3, false); Serial.setTimeout(1000); // Initializes the storage arrays in internal RAM and start motion detector with custom configuration.

  WiFi.mode(WIFI_AP_STA); if (WiFi.waitForConnectResult() != WL_CONNECTED) { Serial.println("Wifi connection failed"); WiFi.disconnect(false); delay(500); WiFi.begin(ssid, password);}

  esp_wifi_set_promiscuous(true); esp_wifi_set_promiscuous_rx_cb(&sniffer); esp_wifi_set_channel(WiFiChannel, WIFI_SECOND_CHAN_NONE);

  Serial.begin(115200); Serial.println("Wait about 1 minute for motion sensor to be ready to detect motion.....");
}

//===================== End of Setup ====================================================

void loop() {
  dataInterval++; pingInterval++;
  
  motionDetector_set_minimum_RSSI(-80);                // Minimum RSSI value to be considered reliable. Default value is 80 * -1 = -80.
  motionLevel = motionDetector_esp();                  // if the connection fails, the radar will automatically try to switch to different operating modes by using ESP32 specific calls.
  
  Serial.print("Motion detected & motion level is: ");Serial.println(motionLevel);
 // Serial.print("Motion detected & motion level is: ");Serial.println(kalmanMotion);
  
  if (pingInterval > (EEPROM.readByte(16) * 500))      // 500 for 5 minutes.
  {
   /*
   #if PINGABLE
    // Connect to main router to ping known devices.
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {Serial.println("Wifi connection failed");WiFi.disconnect(false);delay(500);WiFi.begin(routerSSID, routerPassword);}
    Serial.println("Checking to see who is at home.... ");

    int pingTime;

    deviceIP[3] = device1IP; Serial.println("Pinging IP address 2... "); if (Ping.ping(deviceIP, 5)) {pingTime = Ping.averageTime();Serial.print("Ping time in milliseconds: ");Serial.println(pingTime);sensorValues[18] = (pingTime);} else {sensorValues[18] = 0;}
    deviceIP[3] = device2IP; Serial.println("Pinging IP address 3... "); if (Ping.ping(deviceIP, 5)) {pingTime = Ping.averageTime();Serial.print("Ping time in milliseconds: ");Serial.println(pingTime);sensorValues[19] = (pingTime);} else {sensorValues[19] = 0;}
    deviceIP[3] = device3IP; Serial.println("Pinging IP address 4... "); if (Ping.ping(deviceIP, 5)) {pingTime = Ping.averageTime();Serial.print("Ping time in milliseconds: ");Serial.println(pingTime);sensorValues[20] = (pingTime);} else {sensorValues[20] = 0;}
    deviceIP[3] = device4IP; Serial.println("Pinging IP address 5... "); if (Ping.ping(deviceIP, 5)) {pingTime = Ping.averageTime();Serial.print("Ping time in milliseconds: ");Serial.println(pingTime);sensorValues[21] = (pingTime);} else {sensorValues[21] = 0;}

    WiFi.disconnect(true); delay(500);         // Pinging is done. Disconnect from main router.
    WiFi.begin(ssid, password);                // Connect to motion detector link AP.
  #endif   // #if PINGABLE 
  */
 }
  
  if (dataInterval > (EEPROM.readByte(16) * 100))  // 100 for 1 minute.
  {
    sendSensorvalues();                            // Send sensor values to gateway at predefined interval (EEPROM.readByte(16)).
  } else if (motionLevel > motionThreshold)        // Adjust the sensitivity of motion sensor. Higher the number means less sensetive motion sensor is.
  {
    Serial.print("Motion detected & motion level is: "); Serial.println(motionLevel);
    sendSensorvalues();
  }

  delay(600);   // Do not change this. Data interval and ping interval is calculated based on this value.
} // End of loop.

//============= End of main loop and all functions below ====================================

void sniffer(void* buf, wifi_promiscuous_pkt_type_t type)
{
  
  wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t*)buf;

  if (p->payload[0] == 0x40 )   // HEX 40 for type = Proberequest to filter out unwanted traffic.
  {
    /*
    //Check if any family member (known devices predefined above) came home.
    if (p->payload[10] == device1[0] && p->payload[14] == device1[1] && p->payload[15] == device1[2]) {
      Serial.print("Device 1 is Home with RSSI : "); Serial.println(p->rx_ctrl.rssi);
    } else if (p->payload[10] == device2[0] && p->payload[14] == device2[1] && p->payload[16] == device2[2]) {
      Serial.print("Device 2 is Home with RSSI : "); Serial.println(p->rx_ctrl.rssi);
    } else if (p->payload[10] == device3[0] && p->payload[14] == device3[1] && p->payload[17] == device3[2]) {
      Serial.print("Device 3 is Home with RSSI : "); Serial.println(p->rx_ctrl.rssi);
    } else if (p->payload[10] == device4[0] && p->payload[14] == device4[1] && p->payload[15] == device4[2]) {
      Serial.print("Device 4 is Home with RSSI : "); Serial.println(p->rx_ctrl.rssi);
    
    } else {
    */    
        
      if (p->rx_ctrl.rssi > -70) {        // Limit filter to nearest devices. Adjust according to area to be monitored.
      
        Serial.print("Unknown device detected with MAC ID : "); for (int i = 10; i <= 15; i++) { sensorValues[i] = p->payload[i]; Serial.print(sensorValues[i], HEX); }
        receivedRSSI = p->rx_ctrl.rssi;
        Serial.print(" & RSSI : "); Serial.println(receivedRSSI);
        //kalmanFilterRSSI();                     // Calculate the estimated value after applying Kalman Filter
      }
       
      // RSSI = -10nlog10(d/d0)+A0 // https://www.wouterbulten.nl/blog/tech/kalman-filters-explained-removing-noise-from-rssi-signals/#fn:2
      // https://create.arduino.cc/projecthub/deodates/rssi-based-social-distancing-af0e26
      // Following three variables must be float type.

      float RSSI_1meter = -50; // RSSI at 1 meter distance. Adjust according to your environment.Use WiFi Analyser android app from VREM Software & take average of RSSI @ 1 meter. .
      float Noise = 2;         // Try between 2 to 4. 2 is acceptable number but Adjust according to your environment.
      float Distance = pow(10, (RSSI_1meter -  receivedRSSI) / (10 * Noise)); Serial.print("Distance:  "); Serial.println(Distance);    
     }

  if (p->payload[0] == 0x80 && p->payload[4] == EEPROM.readByte(0))   // HEX 80 for type = Beacon to filter out unwanted traffic and match device number.
  {
    Serial.print("Command received from Gateway : ");

    for (int i = 0; i <= 21; i++) {
      Serial.print(p->payload[i], HEX);
    }

    Serial.println();
    EEPROM.writeByte(1, p->payload[5]);  // Command type at EEPROM address 1.
    EEPROM.commit();

    commandType = EEPROM.readByte(1);

    Serial.println("Contents of EEPROM for this device below: "); EEPROM.readBytes(0, showConfig, 19); for (int i = 0; i < 19; i++) {Serial.printf("%d ", showConfig[i]);}

    if ( commandType > 100 && commandType < 121)  {   // If commandType is 101 to 120.

      Serial.println();
      Serial.print("This device's Wifi Channel is: "); Serial.println(EEPROM.readByte(15));
      Serial.print("This device's MAC ID is: "); Serial.println(WiFi.macAddress());

      value1 = p->payload[6];
      value2 = p->payload[7];
      value3 = p->payload[8];
      value4 = p->payload[9];
      //  New time synch received from Gateway.
      Month  = p->payload[10];  // January is 0.
      Date   = p->payload[11];
      Hour   = p->payload[12];
      Minute = p->payload[13];
      Second = p->payload[14];
      Serial.print("Time synch received from Gateway: "); Serial.print(Month); Serial.print("/"); Serial.print(Date); Serial.print("  "); Serial.print(Hour); Serial.print(":"); Serial.print(Minute); Serial.print(":"); Serial.println(Second);

      if (commandType == 101)        // Digital Write
      {
        EEPROM.writeByte(2, value1);
        EEPROM.writeByte(3, value2); EEPROM.commit();
        Serial.print("Received Command Digital Write: "); Serial.print(value1);  Serial.println(value2);

        gpioControl();

      } else if (commandType == 102) // Analog Write
      {
        EEPROM.writeByte(4, value1);
        EEPROM.writeByte(5, value2); EEPROM.commit();
        Serial.print("Received Command Analog Write:  "); Serial.print(value1);  Serial.println(value2);

        gpioControl();

      } else if (commandType == 103)  // Digital Read
      {
        Serial.println("Received Command Digital Read pin:  "); Serial.println(value1);
      } else if (commandType == 104)  // Analog Read
      {
        Serial.println("Received Command Digital Read pin: "); Serial.println(value1);
      } else if (commandType == 105)  // Neopixel
      {
        EEPROM.writeByte(6, value1);
        EEPROM.writeByte(7, value2);
        EEPROM.writeByte(8, value3);
        EEPROM.writeByte(9, value4); EEPROM.commit();
        Serial.print("Received Command Neopixel: "); Serial.print(value1);  Serial.println(value2); Serial.print(value3);  Serial.println(value4);

        gpioControl();

      } else if (commandType == 106)  // Set Targets
      {
        EEPROM.writeByte(10, value1);
        EEPROM.writeByte(11, value2);
        EEPROM.writeByte(12, value3);
        EEPROM.writeByte(13, value4); EEPROM.commit();
        Serial.print("Received Command Set Target Values to: "); Serial.println(value1);  Serial.println(value2); Serial.print(value3);  Serial.println(value4);

      } else if (commandType == 107)  // Set AP Channel
      {
        EEPROM.writeByte(14, value1); EEPROM.commit();
        Serial.print("Received Command Set AP Channel to: "); Serial.println(value1);

      } else if (commandType == 108 && value1 == 1)  // Set Mode
      {
        Serial.print("Received Command Set Device Mode to: "); Serial.println(value1);
        EEPROM.writeByte(15, 0);
        EEPROM.writeByte(0, 246); EEPROM.commit();
        OTAupdate();

      } else if (commandType == 109)  // Set Sleep Time
      {
        EEPROM.writeByte(16, value1); EEPROM.commit();
        Serial.print("Received Command Set Sleep Time to:   "); Serial.print(value1); Serial.println(" minutes.");

      } else if (commandType == 110)  // Set Device ID
      {

        EEPROM.writeByte(0, value1); EEPROM.commit();
        Serial.print("Received Command Set Device ID to: "); Serial.println(value1);

      }

      Serial.println("Command from Gateway saved to EEPROM");
      Serial.println("Contents of EEPROM for this device below: "); Serial.println();
      EEPROM.readBytes(0, showConfig, 19); for (int i = 0; i < 19; i++) {
        Serial.printf("%d ", showConfig[i]);
      } Serial.println();
      delay(1);
    } else {

      Serial.println("Resending sensor values...");
      ESP.restart();   // Seems like gateway did not receive sensor values let's try again.
    }
  }
}
    
void sendSensorvalues()
{
  sensorValues[4] = EEPROM.readByte(0);    // Device ID.
  sensorValues[5] = 165;                   // Voltage must be between 130 and 180 here in whole integer.
  sensorValues[6] = random(70, 74);        // Sensor 1 value.
  sensorValues[7] = random(40, 100);       // Sensor 2 value.
  sensorValues[8] = random(900, 1024) / 4; // Sensor 3 value.
  sensorValues[9] = random(0, 100);        // Sensor 4 value.
  sensorValues[16] = motionLevel / 10;     // Motion Level.
  // Values received from all sensors used on this device and should replace random values of sensorValues array.

  Serial.println("Sending sensor values.....");
  long lastmillis = millis();
  esp_wifi_80211_tx(WIFI_IF_STA, sensorValues, sizeof(sensorValues), true);
  long currentmillis = millis() - lastmillis;
  Serial.print("Transmit & receive time (Milliseconds) : "); Serial.println(currentmillis);

  dataInterval = 0;   // Data sent. Reset the data interval counter.
}


void gpioControl() {

  if ((EEPROM.readByte(2) >= 1 && EEPROM.readByte(2) <= 5) || (EEPROM.readByte(2) >= 12 && EEPROM.readByte(2) <= 39))
  { if (EEPROM.readByte(3) == 1) {
      digitalWrite(EEPROM.readByte(2), HIGH);
    } else if (EEPROM.readByte(2) == 0) {
      digitalWrite(EEPROM.readByte(2), LOW);
    }
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
 
  t_httpUpdate_return ret = ESPhttpUpdate.update(binFile);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      ESP.restart();
      break;
  }
}
