#include <WiFi.h>                           // Built in arduino librrary.
#include <esp_wifi.h>                       // Built in arduino librrary.
#include <HTTPClient.h>                     // Built in arduino librrary.
#include <ESP32httpUpdate.h>                // Install from arduino library manager
#include <EEPROM.h>                         // Built in arduino librrary.

int WiFiChannel = 7;                        // This must be same for all devices on network.
const char* ssid = "ESP";                   // Required for OTA update.SSID of gateway's  softAP.
const char* password = "";                  // Required for OTA update.password for gateway's softAP.

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
  EEPROM.begin(20);
  
  if (EEPROM.readByte(0) == 0 || EEPROM.readByte(0) == 255)  {EEPROM.writeByte(0, 246);} 
  if (EEPROM.readByte(15) < 1 || EEPROM.readByte(15) > 14) {EEPROM.writeByte(15, WiFiChannel);} 
  EEPROM.writeByte(16, 1);
  EEPROM.commit(); 
  Serial.println("Contents of EEPROM for this device below: "); EEPROM.readBytes(0, showConfig, 19); for (int i = 0; i < 19; i++) {Serial.printf("%d ", showConfig[i]);}

  WiFi.mode(WIFI_AP_STA);
  esp_wifi_set_promiscuous(true); esp_wifi_set_promiscuous_rx_cb(&sniffer); esp_wifi_set_channel(WiFiChannel, WIFI_SECOND_CHAN_NONE);

  Serial.begin(115200); 
  sendSensorvalues();
}

//===================== End of Setup ====================================================

void loop() {
 
  sendSensorvalues();

  Serial.print("I will wakeup in: ");
  Serial.print(EEPROM.readByte(16));   // Sleeptime in minutes.
  Serial.println(" Minutes");
  int upTime = (millis());  
  Serial.print("Total time I spent before going to sleep: ");
  Serial.println(upTime);
  esp_sleep_enable_timer_wakeup(EEPROM.readByte(16) * 6000000);            // 60000000 for 1 minute.
  esp_deep_sleep_start();  
} // End of loop.

//============= End of main loop and all functions below ====================================

void sniffer(void* buf, wifi_promiscuous_pkt_type_t type)
{
  
  wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t*)buf;

  
  if (p->payload[0] == 0x80 /*&& p->payload[4] == EEPROM.readByte(0)*/)   // HEX 80 for type = Beacon to filter out unwanted traffic and match device number.
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
  sensorValues[4] = 6;//EEPROM.readByte(0);    // Device ID.
  sensorValues[5] = 165;                   // Voltage must be between 130 and 180 here in whole integer.
  sensorValues[6] = random(70, 74);        // Sensor 1 value.
  sensorValues[7] = random(40, 100);       // Sensor 2 value.
  sensorValues[8] = random(900, 1024) / 4; // Sensor 3 value.
  sensorValues[9] = random(0, 100);        // Sensor 4 value.
    // Values received from all sensors used on this device and should replace random values of sensorValues array.

  Serial.println("Sending sensor values.....");
  long lastmillis = millis();
  esp_wifi_80211_tx(WIFI_IF_STA, sensorValues, sizeof(sensorValues), true);
  long currentmillis = millis() - lastmillis;
  Serial.print("Transmit & receive time (Milliseconds) : "); Serial.println(currentmillis);
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

  WiFi.begin(ssid, password);
  delay(500);

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
