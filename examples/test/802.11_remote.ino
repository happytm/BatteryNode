// 35 ms transmit & receive time for 57 bytes of data and 68 ms total uptime required in two way mode.Confirm and try to reduce this time.
#include <WiFi.h>
#include <esp_wifi.h>

int WiFiChannel = 7;     // This must be same for all devices on network.

uint8_t header[] =       // Looks like 1500 bytes is maximum limit.
 {
  
  0x80, 0x00,                                                 //  0- 1: Frame Control. Type 8 = Beacon.
  0x05, 0x06,                                                 //  2- 3: Duration
  0x11, 0x11, 0x11, 0x11, 0x11, 0x11,                         //  4- 9: Destination address
  0x06, 0x22, 0x22, 0x22, 0x22, 0x22,                         // 10-15: Source address
  0x33, 0x33, 0x33, 0x33, 0x33, 0x33,                         // 16-21: BSSID
  0x00, 0x00,                                                 // 22-23: Sequence / fragment number
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,             // 24-31: Timestamp (GETS OVERWRITTEN TO 0 BY HARDWARE)
  0x64, 0x00,                                                 // 32-33: Beacon interval
  0x31, 0x04,                                                 // 34-35: Capability info
  0x00, 0x00, /* FILL CONTENT HERE(Data  body?) */                       // 36-38: SSID parameter set, 0x00:length:content
  0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24, // 39-48: Supported rates
  0x03, 0x01, 0x01,                                           // 49-51: DS Parameter set, current channel 1 (= 0x01),
  0x05, 0x04, 0x01, 0x02, 0x00, 0x00,                         // 52-57: Traffic Indication Map
  
};
 
  
void setup(){
//adc_power_off();     // ADC work finished so turn it off to save power.
WiFi.mode(WIFI_OFF); // WiFi transmit & receive work finished so turn it off to save power.
esp_wifi_stop();     // WiFi transmit & receive work finished so turn it off to save power.

Serial.begin(115200);

Serial.println("Initiating 802.11 receiver.....");
}

void loop(){ 
  header[4] = 6;
 
  Serial.println("sending packet....."); 
  long lastmillis = millis();
  WiFi.mode(WIFI_AP_STA); 
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);
  esp_wifi_set_channel(WiFiChannel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_80211_tx(WIFI_IF_AP, header, sizeof(header), true);
  Serial.println(WiFi.macAddress());
  long currentmillis = millis() - lastmillis;
  Serial.print("Transmit & receive (Milliseconds) : ");Serial.println(currentmillis);
  
  int upTime = millis();Serial.print("Total up time (Milliseconds) : "); Serial.println(upTime);
  esp_sleep_enable_timer_wakeup(1 * 6000000);            // 60000000 for 1 minute.
  esp_deep_sleep_start();
  }

void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) 
{ 
  
 wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t*)buf;
  
 if (p->payload[4] == 6)
  {
   Serial.print("Received packet: ");
  
   for(int i=0;i<=21;i++){
   Serial.print(p->payload[i], HEX);
   }
   Serial.println();
  
  }
}

