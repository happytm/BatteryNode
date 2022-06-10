// Transmit time only 1 millisecond.

#include <WiFi.h>
#include <esp_wifi.h>

#define HEADER_LENGTH 24
#define PACKET_LENGTH 1500

char header[HEADER_LENGTH]=                // This is defined globally
{
  0x08, 0x00,                         //  0- 1: Frame Control
  0x05, 0x06,                         //  2- 3: Duration
  0x22, 0x22, 0x22, 0x26, 0x16, 0x06, //  4- 9: Destination address
  0x4e, 0x5e, 0x6e, 0x22, 0x32, 0x42, // 10-15: Source address
  0x36, 0x26, 0x16, 0x33, 0x33, 0x33, // 16-21: BSSID
  0x00, 0x00,                         // 22-23: Sequence / fragment number
};

char sentPacket[PACKET_LENGTH];
char receivedPacket[PACKET_LENGTH];

void setup(){
Serial.begin(115200);

WiFi.mode(WIFI_STA);

//memcpy(sentPacket,header,sizeof(header));

//esp_wifi_80211_tx(WIFI_IF_STA, header, sizeof(header), true);
//esp_wifi_set_promiscuous(true);
}

void loop(){ 
  
  header[4] = 2;
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
