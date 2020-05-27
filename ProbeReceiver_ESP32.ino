#include <WiFi.h>
#include <esp_wifi.h>

char sensorTypes[256], sensorValues[512], deviceStatus[256];
char str [512];// = {};
char s [60];

//   Your WiFi config here


char* room = "Livingroom";  // Needed for person locator.Each location must run probeReceiver sketch to implement person locator.
int rssiThreshold = -50; // Adjust according to signal strength by trial & error.
const char* apSSID = "ESP";
const char* apPassword = "";
const int apChannel = 7;
const int hidden = 0; // If hidden is 1 probe request event handling does not work ?

char ssid[] = "yourssid";     // your network SSID (name)
char pass[] = "yourpassword"; // your network password

int device;
float voltage;
uint8_t data[12];
const char* location;

int sensorValue1; int sensorValue2; int sensorValue3; int sensorValue4; int sensorValue5;
int command1 = 36; int command2;  int command3;  int command4;  int command5; int command6;
//uint8_t mac[6] = {command1, command2, command3, command4, command5, command6};
char topic1[50]; char topic2[50]; char topic3[50]; char topic4[50]; char topic5[50]; char topic6[50]; // char topic7[50]; char topic8[50]; char topic9[50]; char topic10[50]; char topic11[50]; char topic12[50];
const char* sensorType1; const char* sensorType2; const char* sensorType3; const char* sensorType4;
int deviceStatus1; int deviceStatus2; int deviceStatus3;  int deviceStatus4; int deviceStatus5; 
const char* statusType1 = "rssi"; const char *statusType2 = "mode"; const char *statusType3 = "ip"; const char *statusType4 = "channel"; const char *statusType5 = "sleeptime"; const char *statusType6 = "uptime";

//uint8_t securityCode[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Security code must be same at remote sensors to compare.

uint8_t PresencePerson2[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Mac ID of Cell phone #1.
uint8_t PresencePerson2[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Mac ID of Cell phone #2.
uint8_t PresencePerson3[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Mac ID of Cell phone #3.
uint8_t PresencePerson4[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Mac ID of Cell phone #4.

// ==================== end of TUNEABLE PARAMETERS ====================


void setup()
{
    Serial.begin(115200);

    delay(1000);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPassword, apChannel, hidden);
    esp_wifi_set_event_mask(WIFI_EVENT_MASK_NONE); // This line is must to activate probe request received event handler.
    Serial.printf("The AP mac address is %s\n", WiFi.softAPmacAddress().c_str());

    Serial.println();
    Serial.println();
    Serial.println("Waiting for probe requests ... ");

    WiFi.onEvent(probeRequest, SYSTEM_EVENT_AP_PROBEREQRECVED);
}

void loop()
{
    
}


void probeRequest(WiFiEvent_t event, WiFiEventInfo_t info){
    
  Serial.print("Probe Received :  ");
  for(int i = 0; i< 6; i++){
    Serial.printf("%02X", info.ap_probereqrecved.mac[i]);
    if(i<5)Serial.print(":");
    
    }
  
  Serial.println();
  Serial.println();
  Serial.println("Received probe request packet:");
  Serial.println("RSSI : " + String(info.ap_probereqrecved.rssi));
  Serial.println("MAC (HEX) : " + String(info.ap_probereqrecved.mac[0], HEX) + ":" + String(info.ap_probereqrecved.mac[1], HEX) + ":" + String(info.ap_probereqrecved.mac[2], HEX) + ":" + String(info.ap_probereqrecved.mac[3], HEX) + ":" + String(info.ap_probereqrecved.mac[4], HEX) + ":" + String(info.ap_probereqrecved.mac[5], HEX));
  Serial.println("MAC (DEC) : " + String(info.ap_probereqrecved.mac[0], DEC) + ":" + String(info.ap_probereqrecved.mac[1], DEC) + ":" + String(info.ap_probereqrecved.mac[2], DEC) + ":" + String(info.ap_probereqrecved.mac[3], DEC) + ":" + String(info.ap_probereqrecved.mac[4], DEC) + ":" + String(info.ap_probereqrecved.mac[5], DEC));
  Serial.println();

   if (info.ap_probereqrecved.mac[0] == PresencePerson1[0] && info.ap_probereqrecved.mac[1] == PresencePerson1[1] && info.ap_probereqrecved.mac[2] == PresencePerson1[2]){ // write code to match MAC ID of cell phone to predefined variable and store presence/absense in new variable.
      Serial.println("################ Person 1 arrived ###################### ");
      //myBroker.publish("Sensordata/Person1/", "Home");
      Serial.print("Signal Strength of remote sensor: ");
      Serial.println(info.ap_probereqrecved.rssi);
      //myBroker.publish("Sensordata/Signal/", (String)dataReceived.rssi);

    if (info.ap_probereqrecved.rssi > rssiThreshold) // Adjust according to signal strength by trial & error.
    { // write code to match MAC ID of cell phone to predefined variable and store presence/absense in new variable.
      //myBroker.publish("Sensordata/Person1/in/", room);

    }
   }
  
      
     if (info.ap_probereqrecved.mac[0] == 6 || info.ap_probereqrecved.mac[0] == 16 || info.ap_probereqrecved.mac[0] == 26 || info.ap_probereqrecved.mac[0] == 36 || info.ap_probereqrecved.mac[0] == 46 || info.ap_probereqrecved.mac[0] == 56 || info.ap_probereqrecved.mac[0] == 66 || info.ap_probereqrecved.mac[0] == 76 || info.ap_probereqrecved.mac[0] == 86 || info.ap_probereqrecved.mac[0] == 96 || info.ap_probereqrecved.mac[0] == 106 || info.ap_probereqrecved.mac[0] == 116 || info.ap_probereqrecved.mac[0] == 126 || info.ap_probereqrecved.mac[0] == 136 || info.ap_probereqrecved.mac[0] == 146 || info.ap_probereqrecved.mac[0] == 156 || info.ap_probereqrecved.mac[0] == 166 || info.ap_probereqrecved.mac[0] == 176 || info.ap_probereqrecved.mac[0] == 186 || info.ap_probereqrecved.mac[0] == 196 || info.ap_probereqrecved.mac[0] == 206 || info.ap_probereqrecved.mac[0] == 216 || info.ap_probereqrecved.mac[0] == 226 || info.ap_probereqrecved.mac[0] == 236 || info.ap_probereqrecved.mac[0] == 246) // only accept data from certain devices.
      {
     
    //    sendCommand();
        
        if (info.ap_probereqrecved.mac[1] == 06) { // only accept data from device with voltage as a sensor type at byte 1.
         
          if (info.ap_probereqrecved.mac[2] == 16) sensorType1 = "temperature";
          if (info.ap_probereqrecved.mac[2] == 26) sensorType1 = "humidity";
          if (info.ap_probereqrecved.mac[2] == 36) sensorType1 = "pressure";
          if (info.ap_probereqrecved.mac[2] == 46) sensorType1 = "light";
          if (info.ap_probereqrecved.mac[2] == 56) sensorType1 = "openclose";
          if (info.ap_probereqrecved.mac[2] == 66) sensorType1 = "level";
          if (info.ap_probereqrecved.mac[2] == 76) sensorType1 = "presence";
          if (info.ap_probereqrecved.mac[2] == 86) sensorType1 = "motion";
          if (info.ap_probereqrecved.mac[2] == 96) sensorType1 = "custom";

          if (info.ap_probereqrecved.mac[3] == 16) sensorType2 = "temperature";
          if (info.ap_probereqrecved.mac[3] == 26) sensorType2 = "humidity";
          if (info.ap_probereqrecved.mac[3] == 36) sensorType2 = "pressure";
          if (info.ap_probereqrecved.mac[3] == 46) sensorType2 = "light";
          if (info.ap_probereqrecved.mac[3] == 56) sensorType2 = "openclose";
          if (info.ap_probereqrecved.mac[3] == 66) sensorType2 = "level";
          if (info.ap_probereqrecved.mac[3] == 76) sensorType2 = "presence";
          if (info.ap_probereqrecved.mac[3] == 86) sensorType2 = "motion";
          if (info.ap_probereqrecved.mac[3] == 96) sensorType2 = "custom";

          if (info.ap_probereqrecved.mac[4] == 16) sensorType3 = "temperature";
          if (info.ap_probereqrecved.mac[4] == 26) sensorType3 = "humidity";
          if (info.ap_probereqrecved.mac[4] == 36) sensorType3 = "pressure";
          if (info.ap_probereqrecved.mac[4] == 46) sensorType3 = "light";
          if (info.ap_probereqrecved.mac[4] == 56) sensorType3 = "openclose";
          if (info.ap_probereqrecved.mac[4] == 66) sensorType3 = "level";
          if (info.ap_probereqrecved.mac[4] == 76) sensorType3 = "presence";
          if (info.ap_probereqrecved.mac[4] == 86) sensorType3 = "motion";
          if (info.ap_probereqrecved.mac[4] == 96) sensorType3 = "custom";

          if (info.ap_probereqrecved.mac[5] == 16) sensorType4 = "temperature";
          if (info.ap_probereqrecved.mac[5] == 26) sensorType4 = "humidity";
          if (info.ap_probereqrecved.mac[5] == 36) sensorType4 = "pressure";
          if (info.ap_probereqrecved.mac[5] == 46) sensorType4 = "light";
          if (info.ap_probereqrecved.mac[5] == 56) sensorType4 = "openclose";
          if (info.ap_probereqrecved.mac[5] == 66) sensorType4 = "level";
          if (info.ap_probereqrecved.mac[5] == 76) sensorType4 = "presence";
          if (info.ap_probereqrecved.mac[5] == 86) sensorType4 = "motion";
          if (info.ap_probereqrecved.mac[5] == 96) sensorType4 = "custom";

        } else {

             device = info.ap_probereqrecved.mac[0];
             
             if (device == 06) location = "Livingroom";
             if (device == 16) location = "Kitchen";
             if (device == 26) location = "Bedroom1";
             if (device == 36) location = "Bedroom2";
             if (device == 46) location = "Bedroom3";
             if (device == 56) location = "Bedroom4";
             if (device == 66) location = "Bathroom1";
             if (device == 76) location = "Bathroom2";
             if (device == 86) location = "Bathroom3";
             if (device == 96) location = "Bathroom4";
             if (device == 106) location = "Laudry";
             if (device == 116) location = "Boiler room";
             if (device == 126) location = "Workshop";
             if (device == 136) location = "Garage";
             if (device == 146) location = "Water Tank";
             if (device == 156) location = "Solar Tracker";
    
           voltage = info.ap_probereqrecved.mac[1];
           voltage = voltage * 2 / 100;

           sensorValue1 = info.ap_probereqrecved.mac[2];
           sensorValue2 = info.ap_probereqrecved.mac[3];
   
      //  if (sensorType4 == "pressure"){  
      //      sensorValue3 = info.ap_probereqrecved.mac[4];
       //     sensorValue3 = sensorValue3 * 4;
       // } else {    
            sensorValue3 = info.ap_probereqrecved.mac[4];
        //}
            sensorValue4 = info.ap_probereqrecved.mac[5];
        }
        
        if (voltage > 2.50 && voltage < 3.60) {

        
       // int len = json(sensorValues, "s|location", location, "f3|Voltage", voltage, "s|Sensor1", sensorType1, "i|SensorValue1", sensorValue1, "s|Sensor2", sensorType2, "i|SensorValue2", sensorValue2, "s|Sensor3", sensorType3, "i|SensorValue3", sensorValue3, "s|Sensor4", sensorType4, "i|SensorValue4", sensorValue4);
       // myBroker.publish("SensorValues", String(sensorValues));
       
        //sprintf (s, "{"); strcat (str, s);
        sprintf (s, "\"%s\":\"%s\"", "location", location);    strcat (str, s);
        sprintf (s, ",\"%s\":\"%.2f\"", "voltage", voltage);    strcat (str, s);

        sprintf (s, ",\"%s\":\"%d\"", sensorType1, sensorValue1); strcat (str, s);
        sprintf (s, ",\"%s\":\"%d\"", sensorType2, sensorValue2); strcat (str, s);
        sprintf (s, ",\"%s\":\"%d\"", sensorType3, sensorValue3); strcat (str, s);
        sprintf (s, ",\"%s\":\"%d\"", sensorType4, sensorValue4); strcat (str, s);
        sprintf (s, "}"); strcat (str, s);
        
        Serial.println(str);
        Serial.println();
       // myBroker.publish("sensorValues", str);
        sprintf (str, "{");
        
        
      #ifdef SPLITMQTTMESSAGES
        sprintf(topic1, "%s%s%s%s", location, "/", "voltage", "");
        sprintf(topic2, "%s%s%s%s", location, "/", sensorType1, "");
        sprintf(topic3, "%s%s%s%s", location, "/", sensorType2, "");
        sprintf(topic4, "%s%s%s%s", location, "/", sensorType3, "");
        sprintf(topic5, "%s%s%s%s", location, "/", sensorType4, "");
  
       // myBroker.publish(topic1, (String)voltage);
       // myBroker.publish(topic2, (String)sensorValue1);
       // myBroker.publish(topic3, (String)sensorValue2);
       // myBroker.publish(topic4, (String)sensorValue3);
       // myBroker.publish(topic5, (String)sensorValue4);
       #endif
        
        if (voltage < 2.50) {      // if voltage of battery gets to low, print the warning below.
         //  myBroker.publish("Warning/Battery Low", location);
         }
        }       

       if (info.ap_probereqrecved.mac[3] == apChannel) {  

        
        deviceStatus1 = (info.ap_probereqrecved.mac[1]);
        deviceStatus2 = (info.ap_probereqrecved.mac[2]);
        deviceStatus3 = (info.ap_probereqrecved.mac[3]);
        deviceStatus4 = (info.ap_probereqrecved.mac[4]);
        deviceStatus5 = (info.ap_probereqrecved.mac[5]);
        
       
       // int len = json(deviceStatus, "s|location", location, "i|rssi", dataReceived.rssi, "i|mode", deviceStatus1, "i|ip", deviceStatus2, "i|channel", deviceStatus3, "i|sleeptime", deviceStatus4, "i|uptime", deviceStatus5);
       // myBroker.publish("DeviceStatus", String(deviceStatus));
       
       
        //sprintf (s, "{"); strcat (str, s);
        sprintf (s, "\"%s\":\"%s\"", "location", location);    strcat (str, s);
        sprintf (s, ",\"%s\":\"%i\"", statusType1, info.ap_probereqrecved.rssi); strcat (str, s);
        sprintf (s, ",\"%s\":\"%d\"", statusType2, deviceStatus1); strcat (str, s);
        sprintf (s, ",\"%s\":\"%d\"", statusType3, deviceStatus2); strcat (str, s);
        sprintf (s, ",\"%s\":\"%d\"", statusType4, deviceStatus3); strcat (str, s);
        sprintf (s, ",\"%s\":\"%d\"", statusType5, deviceStatus4); strcat (str, s);
        sprintf (s, ",\"%s\":\"%d\"", statusType6, deviceStatus5); strcat (str, s);
        sprintf (s, "}"); strcat (str, s);
        
        Serial.println(str);
        Serial.println();
        //myBroker.publish("deviceStatus", str);
        sprintf (str, "{"); 
        
        
       #ifdef SPLITMQTTMESSAGES
        sprintf(topic1, "%s%s%s%s", location, "/", "rssi", "");
        sprintf(topic2, "%s%s%s%s", location, "/", "mode", "");
        sprintf(topic3, "%s%s%s%s", location, "/", "ip", "");
        sprintf(topic4, "%s%s%s%s", location, "/", "channel", "");
        sprintf(topic5, "%s%s%s%s", location, "/", "sleeptime", "");
        sprintf(topic6, "%s%s%s%s", location, "/", "uptime", "");
        
        //myBroker.publish(topic1, (String)dataReceived.rssi);
        //myBroker.publish(topic2, (String)deviceStatus1);
        //myBroker.publish(topic3, (String)deviceStatus2);
        //myBroker.publish(topic4, (String)deviceStatus3);
        //myBroker.publish(topic5, (String)deviceStatus4);
        //myBroker.publish(topic6, (String)deviceStatus5);
       #endif
       
       }
      } 
     }
