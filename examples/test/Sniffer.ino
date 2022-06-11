// Reference: https://www.hackster.io/p99will/esp32-wifi-mac-scanner-sniffer-promiscuous-4c12f4

#include <WiFi.h>
#include "esp_wifi.h"

String deviceList[64][3]; 
int deviceCount = 0;

String knownDevices[10][2] = {  // Put devices you want to be reconized
  {"ESP1","222222222222"},
  {"ESP2","333333333333"},
  {"NAME","MACADDRESS"},
  {"NAME","MACADDRESS"},
  {"NAME","MACADDRESS"},
  {"NAME","MACADDRESS"},
  {"NAME","MACADDRESS"},
  {"NAME","MACADDRESS"},
  {"NAME","MACADDRESS"}
};

String timeElapsed = "60"; // Maximum time (Apx seconds) elapsed before device is consirded offline

typedef struct { // or this
  uint8_t mac[6];
} __attribute__((packed)) MacAddr;

typedef struct { // still dont know much about this
  int16_t frameControl;
  int16_t duration;
  MacAddr da;
  MacAddr sa;
  MacAddr bssid;
  int16_t seqctl;
  unsigned char payload[];
} __attribute__((packed)) managementHeader;
  
#define maximumChannels 13 //max Channel -> US = 11, EU = 13, Japan = 14
int currentChannel = 1;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
 
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&sniffer);
  esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
  
  Serial.println("starting!");
}


void loop() {
    //Serial.println("Changed channel:" + String(currentChannel));
    if(currentChannel > maximumChannels){ 
      currentChannel = 1;
    }
    esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
    delay(1000);
    updatetime();
    purge();
    showpeople();
    currentChannel++;
}

void sniffer(void* buf, wifi_promiscuous_pkt_type_t type) 
{ 
  
  wifi_promiscuous_pkt_t *p = (wifi_promiscuous_pkt_t*)buf; // Dont know headerat these 3 lines do
  int len = p->rx_ctrl.sig_len;
  managementHeader *header = (managementHeader*)p->payload;
  
  String packet;
  String mac;
  
  for(int i=0;i<=57;i++){
  Serial.print(p->payload[i], HEX);
  }
  
  int frameControl = ntohs(header->frameControl);
  
  for(int i=8;i<=8+6+1;i++){ // This reads the first couple of bytes of the packet. This is headerere you can read the headerole packet replaceing the "8+6+1" with "p->rx_ctrl.sig_len"
     packet += String(p->payload[i],HEX);
  }
  for(int i=4;i<=15;i++){ // This removes the 'nibble' bits from the stat and end of the data we want. So we only get the mac address.
    mac += packet[i];
  }
  mac.toUpperCase();

  
  int added = 0;
  for(int i=0;i<=63;i++){ // checks if the MAC address has been added before
    if(mac == deviceList[i][0]){
      deviceList[i][1] = timeElapsed;
      if(deviceList[i][2] == "AWAY"){
        deviceList[i][2] = "0";
      }
      added = 1;
    }
  }
  
  if(added == 0){ // If its new. add it to the array.
    deviceList[deviceCount][0] = mac;
    deviceList[deviceCount][1] = timeElapsed;
    //Serial.println(mac);
    deviceCount ++;
    if(deviceCount >= 64){
      Serial.println("Too many addresses");
      deviceCount = 0;
    }
  }
}

void purge(){ // This maanges the TTL
  for(int i=0;i<=63;i++){
    if(!(deviceList[i][0] == "")){
      int ttl = (deviceList[i][1].toInt());
      ttl --;
      if(ttl <= 0){
        //Serial.println("OFFLINE: " + deviceList[i][0]);
        deviceList[i][2] = "AWAY";
        deviceList[i][1] = timeElapsed;
      }else{
        deviceList[i][1] = String(ttl);
      }
    }
  }
}

void updatetime(){ // This updates the time the device has been online for
  for(int i=0;i<=63;i++){
    if(!(deviceList[i][0] == "")){
      if(deviceList[i][2] == "")deviceList[i][2] = "0";
      if(!(deviceList[i][2] == "OFFLINE")){
          int timehere = (deviceList[i][2].toInt());
          timehere ++;
          deviceList[i][2] = String(timehere);
      }
      
      //Serial.println(deviceList[i][0] + " : " + deviceList[i][2]);
      
    }
  }
}

void showpeople(){ // This checks if the MAC is in the reckonized list and then displays it on the OLED and/or prints it to serial.
  
  for(int i=0;i<=63;i++){
    String tmp1 = deviceList[i][0];
    if(!(tmp1 == "")){
      for(int j=0;j<=9;j++){
        String tmp2 = knownDevices[j][1];
        if(tmp1 == tmp2){
        Serial.print(knownDevices[j][0] + " : " + tmp1 + " : " + deviceList[i][2] + "\n -- \n");
        }
      }
    }
  }
}

