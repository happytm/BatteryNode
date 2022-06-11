// Transmit time only 1 millisecond.

#include <WiFi.h>
#include <esp_wifi.h>

uint8_t header[] =                                   // This is defined globally
{
  0x80, 0x00,                                                 //  0- 1: Frame Control. Type 8 = Beacon.
  0x05, 0x06,                                                 //  2- 3: Duration
  0x11, 0x11, 0x11, 0x11, 0x11, 0x11,                         //  4- 9: Destination address
  0x22, 0x22, 0x22, 0x22, 0x22, 0x22,                         // 10-15: Source address
  0x33, 0x33, 0x33, 0x33, 0x33, 0x33,                         // 16-21: BSSID
  0x00, 0x00,                                                 // 22-23: Sequence / fragment number
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,             // 24-31: Timestamp (GETS OVERWRITTEN TO 0 BY HARDWARE)
  0x64, 0x00,                                                 // 32-33: Beacon interval
  0x31, 0x04,                                                 // 34-35: Capability info
  0x00, 0x00, /* FILL CONTENT HERE */                         // 36-38: SSID parameter set, 0x00:length:content
  0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24, // 39-48: Supported rates
  0x03, 0x01, 0x01,                                           // 49-51: DS Parameter set, current channel 1 (= 0x01),
  0x05, 0x04, 0x01, 0x02, 0x00, 0x00,                         // 52-57: Traffic Indication Map
};


void setup(){
Serial.begin(115200);

WiFi.mode(WIFI_AP_STA); 

}

void loop(){ 
  
  //header[4] = 2;
  Serial.println("sending packet....."); 
  long lastmillis = millis(); Serial.println(lastmillis);
  esp_wifi_80211_tx(WIFI_IF_STA, header, sizeof(header), true);
  Serial.println(millis());
  long currentmillis = millis() - lastmillis;
  Serial.print("Milliseconds : ");Serial.println(currentmillis);
  Serial.println(WiFi.macAddress());
  //delay(5000);
  int upTime = millis();Serial.print("Uptime : "); Serial.println(upTime);
  esp_sleep_enable_timer_wakeup(1 * 6000000);            // 60000000 for 1 minute.
  esp_deep_sleep_start();
  }
