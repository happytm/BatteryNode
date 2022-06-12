#include <WiFi.h>
#include "esp_wifi.h"

int WiFiChannel = 7;  // This must be same for all devices on network.

uint8_t header[] =                                            // Maximum limit is 1500 bytes?
{
  0x80, 0x00,                                                 //  0- 1: Frame Control. Type 8 = Beacon.
  0x00, 0x00,                                                 //  2- 3: Duration
  0x44, 0x44, 0x44, 0x44, 0x44, 0x44,                         //  4- 9: Destination address
  0x06, 0x54, 0x54, 0x54, 0x54, 0x54,                         // 10-15: Source address
  0x64, 0x64, 0x64, 0x64, 0x64, 0x64,                         // 16-21: BSSID
  0x00, 0x00,                                                 // 22-23: Sequence / fragment number
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,             // 24-31: Timestamp (GETS OVERWRITTEN TO 0 BY HARDWARE)
  0x64, 0x00,                                                 // 32-33: Beacon interval
  0x31, 0x04,                                                 // 34-35: Capability info
  0x00, 0x00, /* FILL CONTENT HERE(Data  body?) */                         // 36-38: SSID parameter set, 0x00:length:content
  0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24, // 39-48: Supported rates
  0x03, 0x01, 0x01,                                           // 49-51: DS Parameter set, current channel 1 (= 0x01),
  0x05, 0x04, 0x01, 0x02, 0x00, 0x00,                         // 52-57: Traffic Indication Map
};

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
 
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);
  esp_wifi_set_channel(WiFiChannel, WIFI_SECOND_CHAN_NONE);
  
  Serial.println("Initiating 802.11 receiver.....");
}

void loop() { }

void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) 
{ 
  
  wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t*)buf;
  
 if (p->payload[4] == 6)
  {
   header[4] = 6;
   Serial.println("sending packet.....");
   esp_wifi_80211_tx(WIFI_IF_AP, header, sizeof(header), true);
   Serial.print("Received packet: ");
   for(int i=0;i<=21;i++){
   Serial.print(p->payload[i], HEX);
   }
   Serial.println();
  
  }
}
