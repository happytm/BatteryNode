//#include <WiFi.h>
#include <esp_wifi.h>
#include <TinyMqtt.h>   // Thanks to https://github.com/hsaturn/TinyMqtt

const char* ssid = "HAPPYHOME"; // Your WiFi SSID
const char* password = "kb1henna"; // Your WiFi Password
const char* apSSID = "ESP";
const char* apPassword = "";
const int apChannel = 7;
const int hidden = 0; // If hidden is 1 probe request event handling does not work ?

int device;
float voltage;

const char* location;

char str [256];
char s [256];

uint8_t mac[6];

char topic1[50]; char topic2[50]; char topic3[50]; char topic4[50]; char topic5[50]; char topic6[50]; // char topic7[50]; char topic8[50]; char topic9[50]; char topic10[50]; char topic11[50]; char topic12[50];

int sensorValues[4];
int sensorTypes[4];

int Livingroom[4] = {16,26,36,46};
int Kitchen[4] =    {46,36,26,16};
int Bedroom1[4] =   {46,36,26,16};
int Bedroom2[4] =   {16,26,36,36};
int Bedroom3[4] =   {16,26,36,36};
int Bedroom4[4] =   {16,26,36,36};
int Bathroom1[4] =  {16,26,36,36};
int Bathroom2[4] =  {16,26,36,36};
int Bathroom3[4] =  {16,26,36,36};
int Bathroom4[4] =  {16,26,36,36};
int Laundry[4] =    {16,26,36,36};
int Boiler[4] =     {16,26,36,36};
int Workshop[4] =   {16,26,36,36};
int Garage[4] =     {16,26,36,36};
int Office[4] =     {16,26,36,36};
int Tank[4] =       {16,26,36,36};
int Solar[4] =      {16,26,36,36};

//uint8_t securityCode[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Security code must be same at remote sensors to compare.

std::string sentTopic = "data";
std::string receivedTopic = "command";

MqttBroker broker(1883);

MqttClient client(&broker);

void receivedMessage(const MqttClient* source, const Topic& topic, const char* payload, size_t length)
{
  //Serial << endl << "Received message on topic " << receivedTopic.c_str() << " with payload " << payload << endl;
  // Serial << endl << "Received message on topic " << sentTopic.c_str() << " with payload " << payload << endl;

  if (receivedTopic == "command")
  {
    mac[0] = atoi(&payload[0]);
    mac[1] = atoi(&payload[3]);
    mac[2] = atoi(&payload[6]);
    mac[3] = atoi(&payload[9]);
    mac[4] = atoi(&payload[12]);
    mac[5] = atoi(&payload[15]);
  }

}

void sendCommand()  {

  esp_wifi_set_mac(ESP_IF_WIFI_AP, mac);
  Serial << "Command sent to remote device :  " << mac[0] << "/" << mac[1] << "/" << mac[2] << "/" << mac[3] << "/" << mac[4] << "/" << mac[5] << "/" << endl;

}

void setup()
{
  Serial.begin(115200);
  delay(500);

  Serial << "Starting client and AP......." << endl;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial << '-';
    delay(500);
  }

  Serial << "Connected to " << ssid << " IP address: " << WiFi.localIP() << endl;

  WiFi.softAP(apSSID, apPassword, apChannel, hidden);
  esp_wifi_set_event_mask(WIFI_EVENT_MASK_NONE); // This line is must to activate probe request received event handler.
  Serial << "The AP mac address is " << WiFi.softAPmacAddress().c_str() << endl;
  Serial << "Access point started at " << apSSID << " with IP address: " << WiFi.localIP() << endl;
 
  broker.begin();

  // ============= Client Subscribe ================
  client.setCallback(receivedMessage);
  client.subscribe(receivedTopic);
  client.subscribe(sentTopic);

  WiFi.onEvent(probeRequest, SYSTEM_EVENT_AP_PROBEREQRECVED);
  Serial << "Waiting for probe requests ... " << endl;

}  // End of Setup


void loop()
{
  
  
  broker.loop();  // Don't forget to add loop for every broker and clients
  client.loop();

} // End of Loop


void probeRequest(WiFiEvent_t event, WiFiEventInfo_t info) 
{
  
    Serial.print("Probe Received :  ");
    for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", info.ap_probereqrecved.mac[i]);
    if (i < 5)Serial.print(":");
    }
    Serial.println();

    String probeHEX = ("HEX : " + String(info.ap_probereqrecved.mac[0], HEX) + ":" + String(info.ap_probereqrecved.mac[1], HEX) + ":" + String(info.ap_probereqrecved.mac[2], HEX) + ":" + String(info.ap_probereqrecved.mac[3], HEX) + ":" + String(info.ap_probereqrecved.mac[4], HEX) + ":" + String(info.ap_probereqrecved.mac[5], HEX));
    String probeDEC = ("{" + String(info.ap_probereqrecved.mac[0], DEC) + "," + String(info.ap_probereqrecved.mac[1], DEC) + "," + String(info.ap_probereqrecved.mac[2], DEC) + "," + String(info.ap_probereqrecved.mac[3], DEC) + "," + String(info.ap_probereqrecved.mac[4], DEC) + "," + String(info.ap_probereqrecved.mac[5], DEC) + "}");
    Serial.print("Received probe request packet with "); Serial.println("RSSI : " + String(info.ap_probereqrecved.rssi));
    //Serial.println(probeHEX);
    Serial.println(probeDEC);
    Serial.println();
 
      if (info.ap_probereqrecved.mac[0] == 6 || info.ap_probereqrecved.mac[0] == 16 || info.ap_probereqrecved.mac[0] == 26 || info.ap_probereqrecved.mac[0] == 36 || info.ap_probereqrecved.mac[0] == 46 || info.ap_probereqrecved.mac[0] == 56 || info.ap_probereqrecved.mac[0] == 66 || info.ap_probereqrecved.mac[0] == 76 || info.ap_probereqrecved.mac[0] == 86 || info.ap_probereqrecved.mac[0] == 96 || info.ap_probereqrecved.mac[0] == 106 || info.ap_probereqrecved.mac[0] == 116 || info.ap_probereqrecved.mac[0] == 126 || info.ap_probereqrecved.mac[0] == 136 || info.ap_probereqrecved.mac[0] == 146 || info.ap_probereqrecved.mac[0] == 156 || info.ap_probereqrecved.mac[0] == 166 || info.ap_probereqrecved.mac[0] == 176 || info.ap_probereqrecved.mac[0] == 186 || info.ap_probereqrecved.mac[0] == 196 || info.ap_probereqrecved.mac[0] == 206 || info.ap_probereqrecved.mac[0] == 216 || info.ap_probereqrecved.mac[0] == 226 || info.ap_probereqrecved.mac[0] == 236 || info.ap_probereqrecved.mac[0] == 246) // only accept data from certain devices.
       {

       sendCommand();

      if (info.ap_probereqrecved.mac[1] == 06) 
      { // only accept data from device with voltage as a sensor type at byte 1.

        if (device == 06) { for (int i = 0; i < 4; i++) sensorTypes[i] = Livingroom[i];} 
               if (device == 16) { for (int i = 0; i < 4; i++) sensorTypes[i] = Kitchen[i];}
               if (device == 26) { for (int i = 0; i < 4; i++) sensorTypes[i] = Bedroom1[i];} 
               if (device == 36) { for (int i = 0; i < 4; i++) sensorTypes[i] = Bedroom2[i];}
               if (device == 46) { for (int i = 0; i < 4; i++) sensorTypes[i] = Bedroom3[i];} 
               if (device == 56) { for (int i = 0; i < 4; i++) sensorTypes[i] = Bedroom4[i];}
               if (device == 66) { for (int i = 0; i < 4; i++) sensorTypes[i] = Bathroom1[i];} 
               if (device == 76) { for (int i = 0; i < 4; i++) sensorTypes[i] = Bathroom2[i];}
               if (device == 86) { for (int i = 0; i < 4; i++) sensorTypes[i] = Bathroom3[i];} 
               if (device == 96) { for (int i = 0; i < 4; i++) sensorTypes[i] = Bathroom4[i];}
               if (device == 106) { for (int i = 0; i < 4; i++) sensorTypes[i] = Laundry[i];} 
               if (device == 116) { for (int i = 0; i < 4; i++) sensorTypes[i] = Boiler[i];}
               if (device == 126) { for (int i = 0; i < 4; i++) sensorTypes[i] = Workshop[i];} 
               if (device == 136) { for (int i = 0; i < 4; i++) sensorTypes[i] = Garage[i];}
               if (device == 146) { for (int i = 0; i < 4; i++) sensorTypes[i] = Office[i];}
               if (device == 156) { for (int i = 0; i < 4; i++) sensorTypes[i] = Tank[i];} 
               if (device == 166) { for (int i = 0; i < 4; i++) sensorTypes[i] = Solar[i];}
               
       } else {

               device = info.ap_probereqrecved.mac[0];
               
               voltage = info.ap_probereqrecved.mac[1];
               voltage = voltage * 2 / 100;
                  
               sensorValues[0] = info.ap_probereqrecved.mac[2];
               sensorValues[1] = info.ap_probereqrecved.mac[3];
               sensorValues[2] = info.ap_probereqrecved.mac[4];
               sensorValues[3] = info.ap_probereqrecved.mac[5];
              
             }

               if (voltage > 2.50 && voltage < 3.60) 
               {

               sprintf (str, "{");
               sprintf (s, "\"%s\":\"%i\"", "Location", device);    strcat (str, s);
               sprintf (s, ",\"%s\":\"%.2f\"", "Voltage", voltage);    strcat (str, s);
               sprintf (s, ",\"%i\":\"%i\"", sensorTypes[0], sensorValues[0]); strcat (str, s);
               sprintf (s, ",\"%i\":\"%i\"", sensorTypes[1], sensorValues[1]); strcat (str, s);
               sprintf (s, ",\"%i\":\"%i\"", sensorTypes[2], sensorValues[2]); strcat (str, s);
               sprintf (s, ",\"%i\":\"%i\"", sensorTypes[3], sensorValues[3]); strcat (str, s);
               sprintf (s, "}"); strcat (str, s);
              
               Serial.println();
               Serial.println("Following ## Sensor Values ## receiced from remote device  & published via MQTT: ");
               Serial.println(str);
               Serial.println();
               static auto next=millis();               // The next line is an efficient delay() replacement
               if (millis() > next){next += 10000;}
               client.publish("sensorValues", String(str));

               if (voltage < 2.50) {      // if voltage of battery gets to low, print the warning below.
              //client.publish("Warning/Battery Low", location);
               }
              }

                  if (info.ap_probereqrecved.mac[3] == apChannel) {

                      sprintf (str, "{");
                      sprintf (s, "\"%s\":\"%i\"", "Location", device);    strcat (str, s);
                      sprintf (s, ",\"%s\":\"%i\"", "RSSI", info.ap_probereqrecved.rssi); strcat (str, s);
                      sprintf (s, ",\"%s\":\"%i\"", "MODE", info.ap_probereqrecved.mac[1]); strcat (str, s);
                      sprintf (s, ",\"%s\":\"%i\"", "IP", info.ap_probereqrecved.mac[2]); strcat (str, s);
                      sprintf (s, ",\"%s\":\"%i\"", "CHANNEL", info.ap_probereqrecved.mac[3]); strcat (str, s);
                      sprintf (s, ",\"%s\":\"%i\"", "Sleeptime", info.ap_probereqrecved.mac[4]); strcat (str, s);
                      sprintf (s, ",\"%s\":\"%i\"", "Uptime", info.ap_probereqrecved.mac[5]); strcat (str, s);
                      sprintf (s, "}"); strcat (str, s);
                           
                      Serial.println();
                      Serial.println("Following ## Device Status ## receiced from remote device & published via MQTT: ");
                      Serial.println(str);
                      Serial.println();
                      
                      static auto next=millis();               // The next line is an efficient delay() replacement
                      if (millis() > next){next += 10000;}
                      client.publish("deviceStatus", String(str));
                      }
                     }
                    }
