// Average 152 milliseconds uptime in one way mode.

#include <WiFi.h>
#include <esp_wifi.h>

// use any of following for devie ID ending with 6.
// 6,16,26,36,46,56,66,76,86,96,106,116,126,136,146,156,166,176,186,196,206,216,226,236,246 etc.

int device = 6;           // Unique device ID must end with 2,6,A or E. See https://serverfault.com/questions/40712/what-range-of-mac-addresses-can-i-safely-use-for-my-virtual-machines.
int apChannel = 7;        // WiFi Channel for this device.
int sleepTime = 1;        // Sleep time in minutes.
int upTime;               // Device uptime in milliseconds.

uint8_t sensorData[6];  // Sensor values to be sent to Gateway = {device, voltage, temperature, humidity, pressure, light};

//============Do not need user configuration from here on============================

void setup() {
  //WiFi.scanDelete();  //remove previous scan data from memory
  sensorValues();
  //int16_t scanNetworks(bool async = false, bool show_hidden = false, bool passive = false, uint32_t max_ms_per_chan = 300, uint8_t channel = 0);
  int n = WiFi.scanNetworks(true, false, false, 200, apChannel);
  Serial.begin(115200);
  Serial.print("Sensors values data sent to controller: ");
  Serial.println(WiFi.macAddress());

}
//========================Main Loop================================

void loop() {

  upTime = (millis() + 8);  // Estimated 8 milliseconds added to account for next process in loop.
  
  Serial.print("Total time I spent before going to sleep: ");
  Serial.println(upTime);
  Serial.print("I will wakeup in: ");
  Serial.print(sleepTime);
  Serial.println(" Minutes");
  delay(5000); ESP.restart();   // For testing only.
  //ESP.deepSleepInstant(sleepTime * 60000000, WAKE_NO_RFCAL); //If last digit of MAC ID matches to device ID go to deep sleep else loop through again.
}
//=========================Main Loop ends==========================

void sensorValues() 
{

  sensorData[0] = device;
  sensorData[1] = 2.93;
  sensorData[2] = random(45,55);          // Temperature;
  sensorData[3] = random(40,100);         // Humidity;
  sensorData[4] = random(850,1024) / 4;   // Pressure;
  sensorData[5] = random(30,100);         // Light;
  
  esp_base_mac_addr_set(sensorData);
  
  //Functions for all sensors used on this device goes here.
  //Values received from sensors replaces 4 random values of sensorData array.
}
