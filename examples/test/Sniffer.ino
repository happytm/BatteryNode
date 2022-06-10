// Reference: https://github.com/maddog986/arduino_sniffer

#include <WiFi.h>
#include "esp_wifi.h"

#define HEADER_LENGTH 24
#define PACKET_LENGTH 1500

#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

int curChannel = 1;
long previousMillis = 0;
int startup = 0;

String maclist[64][3];
String aps[64][2];
int listcount = 0;

const wifi_promiscuous_filter_t filt={
    .filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT|WIFI_PROMIS_FILTER_MASK_DATA
};

typedef struct {
    wifi_pkt_rx_ctrl_t rx_ctrl;
    uint8_t buf[112];
    uint16_t cnt;
    uint16_t len; //length of packet
} wifi_pkt_mgmt_t;

typedef struct {
  uint16_t length;
  uint16_t seq;
  uint8_t address3[6];
} wifi_pkt_lenseq_t;

typedef struct {
    wifi_pkt_rx_ctrl_t rx_ctrl;
    uint8_t buf[36];
    uint16_t cnt;
    wifi_pkt_lenseq_t lenseq[1];
} wifi_pkt_data_t;


typedef enum
{
    ASSOCIATION_REQ,
    ASSOCIATION_RES,
    REASSOCIATION_REQ,
    REASSOCIATION_RES,
    PROBE_REQ,
    PROBE_RES,
    NU1,  /* ......................*/
    NU2,  /* 0110, 0111 not used */
    BEACON,
    ATIM,
    DISASSOCIATION,
    AUTHENTICATION,
    DEAUTHENTICATION,
    ACTION,
    ACTION_NACK,
} wifi_mgmt_subtypes_t;

typedef struct
{
  unsigned interval:16;
  unsigned capability:16;
  unsigned tag_number:8;
  unsigned tag_length:8;
  char ssid[0];
  uint8_t rates[1];
} wifi_mgmt_beacon_t;

typedef struct
{
    unsigned protocol:2;
    unsigned type:2;
    unsigned subtype:4;
    unsigned to_ds:1;
    unsigned from_ds:1;
    unsigned more_frag:1;
    unsigned retry:1;
    unsigned pwr_mgmt:1;
    unsigned more_data:1;
    unsigned wep:1;
    unsigned strict:1;
} wifi_header_frame_control_t;

/**
 * Ref: https://github.com/lpodkalicki/blog/blob/master/esp32/016_wifi_sniffer/main/main.c
 */
typedef struct
{
    wifi_header_frame_control_t frame_ctrl;
    //unsigned duration_id:16; /* !!!! ugly hack */
    uint8_t addr1[6]; /* receiver address */
    uint8_t addr2[6]; /* sender address */
    uint8_t addr3[6]; /* filtering address */
    unsigned sequence_ctrl:16;
    uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct
{
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t payload[2]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

// According to the SDK documentation, the packet type can be inferred from the
// size of the buffer. We are ignoring this information and parsing the type-subtype
// from the packet header itself. Still, this is here for reference.
wifi_promiscuous_pkt_type_t packet_type_parser(uint16_t len)
{
    switch(len)
    {
      // If only rx_ctrl is returned, this is an unsupported packet
      case sizeof(wifi_pkt_rx_ctrl_t):
      return WIFI_PKT_MISC;

      // Management packet
      case sizeof(wifi_pkt_mgmt_t):
      return WIFI_PKT_MGMT;

      // Data packet
      default:
      return WIFI_PKT_DATA;
    }
}

// Uncomment to enable MAC address masking
//#define MASKED

//Returns a human-readable string from a binary MAC address.
//If MASKED is defined, it masks the output with XX
void mac2str(const uint8_t* ptr, char* string)
{
  #ifdef MASKED
  sprintf(string, "XX:XX:XX:%02x:%02x:XX", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
  #else
  sprintf(string, "%02x:%02x:%02x:%02x:%02x:%02x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
  #endif
  return;
}

//Parses 802.11 packet type-subtype pair into a human-readable string
const char* wifi_pkt_type2str(wifi_promiscuous_pkt_type_t type, wifi_mgmt_subtypes_t subtype)
{
  switch(type)
  {
    case WIFI_PKT_MGMT:
      switch(subtype)
      {
         case ASSOCIATION_REQ:
         return "Mgmt: Association request";
         case ASSOCIATION_RES:
         return "Mgmt: Association response";
         case REASSOCIATION_REQ:
         return "Mgmt: Reassociation request";
         case REASSOCIATION_RES:
         return "Mgmt: Reassociation response";
         case PROBE_REQ:
         return "Mgmt: Probe request";
         case PROBE_RES:
         return "Mgmt: Probe response";
         case BEACON:
         return "Mgmt: Beacon frame";
         case ATIM:
         return "Mgmt: ATIM";
         case DISASSOCIATION:
         return "Mgmt: Dissasociation";
         case AUTHENTICATION:
         return "Mgmt: Authentication";
         case DEAUTHENTICATION:
         return "Mgmt: Deauthentication";
         case ACTION:
         return "Mgmt: Action";
         case ACTION_NACK:
         return "Mgmt: Action no ack";
      default:
        return "Mgmt: Unsupported/error";
      }

    case WIFI_PKT_MISC:
    return "Misc";

    case WIFI_PKT_DATA:
    return "Data";

    default:
      return "Unsupported/error";
  }
}

bool working = false;

// In this example, the packet handler function does all the parsing and output work.
// This is NOT ideal.
//void wifi_sniffer_packet_handler(uint8_t *buff, uint16_t len)
void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {
  while(working) {
    delay(10);
  }

  working = true

  // First layer: type cast the received buffer into our generic SDK structure
  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
  // Second layer: define pointer to where the actual 802.11 packet is within the structure
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  // Third layer: define pointers to the 802.11 packet header and payload
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
  const uint8_t *data = ipkt->payload;

  // Pointer to the frame control section within the packet header
  const wifi_header_frame_control_t *frame_ctrl = (wifi_header_frame_control_t *)&hdr->frame_ctrl;

  int row = 0;
 // int now = ntpClient.getUnixTime();

  // Parse MAC addresses contained in packet header into human-readable strings
  char addr1[] = "00:00:00:00:00:00\0";
  char addr2[] = "00:00:00:00:00:00\0";
  char addr3[] = "00:00:00:00:00:00\0";

  mac2str(hdr->addr1, addr1);
  mac2str(hdr->addr2, addr2);
  mac2str(hdr->addr3, addr3);

  if (addr2 == "") return;

  char ssid[32] = {0};

  //maclist[64][3]
  //0 = mac
  //1 = signal strengh
  //2 = time last seen
  //3 = assoicated ap
  for(int i=0;i<=63;i++){
    //found, or next empty slot
    if (maclist[i][0] == "" || maclist[i][0] == addr2) {
      maclist[i][0] = addr2; //mac address
      maclist[i][1] = ppkt->rx_ctrl.rssi; //signal
      //maclist[i][2] = now; //timestamp

      break;
    }/* else {
      int ttl = (maclist[i][2]).toInt();
      if (now > ttl + 30) { //expired
        maclist[i][0] = "";
      }
    }*/

    if (i == 63) {
      Serial.print("\nOut of mac slots!\n");
    }
  }

  //AP letting others know its here
  if (frame_ctrl->type == WIFI_PKT_MGMT && frame_ctrl->subtype == BEACON) {
    const wifi_mgmt_beacon_t *beacon_frame = (wifi_mgmt_beacon_t*) ipkt->payload;

    if (beacon_frame->tag_length >= 32) {
      strncpy(ssid, beacon_frame->ssid, 31);
    } else {
      strncpy(ssid, beacon_frame->ssid, beacon_frame->tag_length);
    }

    
    for(int i=0;i<=63;i++){
      if (addr2 == "") break;

      if (aps[i][0] == "" || aps[i][0] == addr2) {
        //Serial.printf("\nAP Probe: %s", ssid);

        aps[i][0] = addr2;
        aps[i][1] = ssid;
        //aps[i][2] = now; //timestamp
        break;
      }/* else {
        int ttl = (aps[i][2].toInt());
        if (now > ttl + 30) { //expired
          aps[i][0] = "";
        }
      }*/

      if (i == 63) {
        Serial.print("\nOut of ap slots!\n");
      }
    }
  }

  //building device info to send to Hass
  String request;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["source_type"] = "router";

  //device looking for APs
  if (frame_ctrl->type == WIFI_PKT_MGMT && frame_ctrl->subtype == PROBE_REQ) {
    //addr2 = device

    //return;

    root["host_name"] = addr2;
    root["mac"] = addr2;
  }

  //APs responding to device
  if (frame_ctrl->type == WIFI_PKT_MGMT && frame_ctrl->subtype == PROBE_RES) {
    //addr1 = device
    //addr2 = ap

    root["host_name"] = addr1;
    root["mac"] = addr1;

    //return;

    //aps[64][2]
    //0 = mac
    //1 = ssid
    //3 = timestamp
    for(int i=0;i<=63;i++){
      if (addr2 == "") break;

      if (aps[i][0] == "" || aps[i][0] == addr2) {
        //Serial.printf("\nAP Probe: %s", ssid);

        aps[i][0] = addr2;
        //aps[i][1] = ssid;
        //aps[i][2] = now; //timestamp
        break;
      }/* else {
        int ttl = (aps[i][2].toInt());
        if (now > ttl + 30) { //expired
          aps[i][0] = "";
        }
      }*/

      if (i == 63) {
        Serial.print("\nOut of ap slots!\n");
      }
    }

    if (root["mac"]) {
      root.printTo(request);
    }

    for(int i=0;i<=63;i++){
      //maclist[64][3]
      //0 = mac
      //1 = signal strengh
      //2 = time last seen
      //3 = assoicated ap
      if (maclist[i][0] == addr1 && addr1 != addr2) { //found device match
        maclist[i][3] = addr2; //now device assoicated with this ap
      }
    }
  }


  // Output info to serial
  Serial.printf("\n%s | %s | %s | %u | %02d | %u | %u(%-2u) | %-28s | %u | %u | %u | %u | %u | %u | %u | %u | ",
    addr1,
    addr2,
    addr3,
   //  now,
   // ppkt->rx_ctrl.channel,
    ppkt->rx_ctrl.rssi,
    frame_ctrl->protocol,
    frame_ctrl->type,
    frame_ctrl->subtype,
    wifi_pkt_type2str((wifi_promiscuous_pkt_type_t)frame_ctrl->type, (wifi_mgmt_subtypes_t)frame_ctrl->subtype),
    frame_ctrl->to_ds,
    frame_ctrl->from_ds,
    frame_ctrl->more_frag,
    frame_ctrl->retry,
    frame_ctrl->pwr_mgmt,
    frame_ctrl->more_data,
    frame_ctrl->wep,
    frame_ctrl->strict);


  // Print ESSID if beacon
  if (frame_ctrl->type == WIFI_PKT_MGMT && frame_ctrl->subtype == BEACON) {
    Serial.printf("%s", ssid);
  }

  working = false;
}


void setup()
{
  // Serial setup
  Serial.begin(115200);
  delay(10);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_filter(&filt);
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
  esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);
  
  esp_wifi_set_event_mask(WIFI_EVENT_MASK_NONE); // This line is must to activate probe request received event handler.
  WiFi.onEvent(probeRequest, SYSTEM_EVENT_AP_PROBEREQRECVED); Serial.print("Waiting for probe requests ... ");

  // Print header
  Serial.printf("\n\nMAC Address 1     | MAC Address 2     | MAC Address 3     | TS      | Ch| RSSI| Pr| T(S) | Frame type                   |TDS|FDS| MF|RTR|PWR| MD|ENC|STR|   SSID");
}

void loop() {
  //delay(10);

  if (working) return;
  working = true;

  if(curChannel > 11){
    curChannel = 1;
  }

  //esp_wifi_set_channel(curChannel, WIFI_SECOND_CHAN_NONE);

  unsigned long currentMillis = millis();

  if ((currentMillis - previousMillis) > 1000) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    curChannel++;

    if (curChannel == 2) {
      curChannel = 11;
    }

    Serial.println("\n\n===========================");

    for(int i=0;i<=63;i++){
      if (aps[i][0] == "") { //dead
        continue;
      }

      Serial.println(String("AP: ") + aps[i][1] + String(" (") + aps[i][0] + String(")"));

      for(int i2=0;i2<=63;i2++){
        if (maclist[i2][0] == "") { //dead
          continue;
        }

        if (maclist[i2][3] == aps[i][0]) {
          Serial.print(String("- ") + maclist[i2][0] + String(" (") + maclist[i2][1] + String(")\n"));

          
        }
      }
    }

    Serial.println("===========================");
  }

  working = false;
}

void probeRequest(WiFiEvent_t event, WiFiEventInfo_t info) 
{ 
  Serial.println();
  Serial.print("Probe Received :  ");for (int i = 0; i < 6; i++) {Serial.printf("%02X", info.ap_probereqrecved.mac[i]);if (i < 5)Serial.print(":");}Serial.println();
} // End of Proberequest function.
