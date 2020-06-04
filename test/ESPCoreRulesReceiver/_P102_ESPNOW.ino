//#######################################################################################################
//#################################### Plugin 102: ESPNOW ###############################################
//#######################################################################################################

#define SERIALDEBUG true

/*
 * ESPNowConfig <kok>,<key>,<mac>,<mode>       Start ESPNOW sender/receiver (do not use Wifi at the same time)
 * ESPNowAddPeer <key>,<mac>,<role>            Add an ESPNOW peer with encryption key and mac address
 * ESPNowSend <msg>                            Send a message using ESPNOW protocol to peered receiver
 * 
*/

#ifdef USES_P102
#define P102_BUILD            7
#define P102_MAXPACKETSIZE    192  // absolute max for total payload = 250 bytes
#define PLUGIN_102
#define PLUGIN_ID_102         102

#include <espnow.h>

boolean Plugin_102(byte function, String& cmd, String& params)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_INFO:
      {
        printWebTools += F("<TR><TD>P102 - ESPNOW<TD>");
        printWebTools += P102_BUILD;        
        break;
      }
      
    case PLUGIN_WRITE:
      {
        if (cmd.equalsIgnoreCase(F("espnowConfig")))
        {
          success = true;
          String kok = parseString(params, 1);
          String key = parseString(params, 2);
          String macStr = parseString(params, 3);
          String mode = parseString(params, 4);
          byte mac[6];
          parseBytes(macStr.c_str(), ':', mac, 6, 16);
          if (mode.equalsIgnoreCase(F("Sender"))){
            P102_espnowSender(kok.c_str(), key.c_str(), mac);
          }else{
            P102_espnowReceiver(kok.c_str(), key.c_str(), mac);
          }
        }

        if (cmd.equalsIgnoreCase(F("espnowAddPeer")))
        {
          success = true;
          String key = parseString(params, 1);
          String macStr = parseString(params, 2);
          byte role = parseString(params, 3).toInt();
          byte mac[6];
          parseBytes(macStr.c_str(), ':', mac, 6, 16);
          P102_espnowAddPeer(key.c_str(), mac, role);
        }  

        if (cmd.equalsIgnoreCase(F("espnowSend")))
        {
          success = true;
          P102_espnowSend(params);
        }

        break;
      }
  }
  return success;
}

void P102_espnowSender(const char* kok, const char* key, uint8_t* mac){
#if SERIALDEBUG
  Serial.println("ESP Sender");
  Serial.print("KOK: ");
  Serial.println(kok);
  Serial.print("Key: ");
  Serial.println(key);
  Serial.print("MAC: ");
  for(byte x=0; x <6;x++){
    Serial.print(mac[x],HEX);
    if(x != 5)
      Serial.print("-");
  }
  Serial.println();
#endif
  WiFi.forceSleepWake();
  delay(1);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  byte wifiChannel = 1; 
  wifi_set_macaddr(STATION_IF, mac);
  #if SERIALDEBUG
    Serial.print("This node STA mac: "); Serial.println(WiFi.macAddress());
  #endif
  if (esp_now_init() == 0) {
    #if SERIALDEBUG
      Serial.println("*** ESP_Now Sender init");
    #endif
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_set_kok((uint8_t*)kok,16);
    esp_now_register_send_cb([](uint8_t* mac, uint8_t sendStatus) {
      #if SERIALDEBUG
      Serial.printf("send_cb, send done, status = %i\n", sendStatus);
      #endif
    });
  }
}


void P102_espnowSend(String msg){
  struct __attribute__((packed)) SENSOR_DATA {
    char msg[P102_MAXPACKETSIZE];
  }sensorData;
  strcpy(sensorData.msg, msg.c_str());  
  uint8_t bs[sizeof(sensorData)];
  memcpy(bs, &sensorData, sizeof(sensorData));
  esp_now_send(NULL, bs, sizeof(sensorData)); // NULL means send to all peers
}


void P102_espnowAddPeer(const char* key, uint8_t* mac, byte role){
#if SERIALDEBUG
  Serial.println("Add Peer");
  Serial.print("Key: ");
  Serial.println(key);
  Serial.print("MAC: ");
  for(byte x=0; x <6;x++){
    Serial.print(mac[x],HEX);
    if(x != 5)
      Serial.print("-");
  }
  Serial.println();
#endif
  byte wifiChannel = 1;
  if(role == 0){ 
    #if SERIALDEBUG
      Serial.println("PEER SLAVE");
    #endif
    esp_now_add_peer(mac, ESP_NOW_ROLE_SLAVE, wifiChannel, (uint8_t*)key, 16);
  }
  else
  {
    #if SERIALDEBUG
      Serial.println("PEER CONTROLLER");
    #endif
    esp_now_add_peer(mac, ESP_NOW_ROLE_CONTROLLER, wifiChannel, (uint8_t*)key, 16);
  }
}


void P102_espnowReceiver(const char* kok, const char* key, uint8_t* mac){
#if SERIALDEBUG
  Serial.println("ESP Receiver");
  Serial.print("KOK: ");
  Serial.println(kok);
  Serial.print("Key: ");
  Serial.println(key);
  Serial.print("MAC: ");
  for(byte x=0; x <6;x++){
    Serial.print(mac[x],HEX);
    if(x != 5)
      Serial.print("-");
  }
  Serial.println();
#endif  

  WiFi.forceSleepWake();
  delay(1);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  byte wifiChannel = 1; 

  wifi_set_macaddr(STATION_IF, mac);
  #if SERIALDEBUG
    Serial.print("This node STA mac: "); Serial.println(WiFi.macAddress());
  #endif
    if (esp_now_init()==0) {
      #if SERIALDEBUG
        Serial.println("*** ESP_Now receiver init");
      #endif
      esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
      esp_now_set_kok((uint8_t*)kok,16);
      esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {
      struct __attribute__((packed)) SENSOR_DATA {
        char msg[P102_MAXPACKETSIZE];
      }sensorData;
        if(esp_now_is_peer_exist(mac)){
          String eventString = F("Event ESPNOW#");
          eventString += mac[5];
          eventString += "=";
          //Serial.write(mac, 6); // mac address of remote ESP-Now device
          //Serial.write(len);
          memcpy(&sensorData, data, sizeof(sensorData));
          //Serial.write(data, len);
          eventString += sensorData.msg;
          Serial.println(eventString);
        }
        else
        {
          #if SERIALDEBUG
            Serial.print("Unknown MAC: ");
            for(byte x=0; x <6;x++){
              Serial.print(mac[x],HEX);
              if(x != 5)
                Serial.print("-");
            }
            Serial.println();
          #endif
        }
        
      });
    }
}
#endif
