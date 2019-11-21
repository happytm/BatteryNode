/*
 * uMQTTBroker demo for Arduino (C++-style)
 * 
 * The program defines a custom broker class with callbacks, 
 * starts it, subscribes locally to anything, and publishs a topic every second.
 * Try to connect from a remote client and publish something - the console will show this as well.
 */

#define  MQTTBROKER           true


#include <ESP8266WiFi.h>
#include "uMQTTBroker.h"

/*
 * Your WiFi config here
 */
char apssid[] = "ESP";   
char ssid[] = "HTM1";     // your network SSID (name)
char pass[] = "kb1henna"; // your network password
//bool WiFiAP = false;      // Do yo want the ESP as AP?

int device;
int voltage;
int temperature;
int humidity;
int pressure;
int light;

/*
 Predefined sensor type table is below:
 volatage = 16, temperature = 26, humidity= 36, pressure= 46, light= 56, OpenClose = 66,
 level = 76, presence = 86, motion = 96 etc.
 */ 

int sensorType1;
int sensorType2;
int sensorType3;
int sensorType4;
int sensorType5;

char topic1[50];
char topic2[50];
char topic3[50];
char topic4[50];
char topic5[50];
char topic6[50];

int command1 = 36;
int command2 = 0;
int command3 = 0;
int command4 = 0;
int command5 = 0;
int command6 = 0;

uint8_t mac[6] = {command1, command2, command3, command4, command5, command6};

// ==================== end of TUNEABLE PARAMETERS ====================

extern "C" void preinit() {
  wifi_set_opmode(STATIONAP_MODE);
  wifi_set_macaddr(SOFTAP_IF, mac);
}


/*
 * Custom broker class with overwritten callback functions
 */
class myMQTTBroker: public uMQTTBroker
{
public:
    virtual bool onConnect(IPAddress addr, uint16_t client_count) {
      Serial.println(addr.toString()+" connected");
      return true;
    }
    
    virtual bool onAuth(String username, String password) {
      Serial.println("Username/Password: "+username+"/"+password);
      return true;
    }
    
    virtual void onData(String topic, const char *data, uint32_t length) {
    
      char data_str[length+1];
      os_memcpy(data_str, data, length);
      data_str[length] = '\0';
      Serial.println("received topic '" + topic + (String)data_str);
   /*  
      char* sendCommand; 
      char* rest = data_str; 
      while ((sendCommand = strtok_r(rest, "/", &rest))) 
   //   printf("%s\n", sendCommand); 
 */
 
 

      
 if (topic == "send/")   {
 
 command1 = atoi(&data[0]);  
 Serial.println(command1);  
 command2 = atoi(&data[3]); 
 Serial.println(command2); 
 command3 = atoi(&data[6]);  
 Serial.println(command3);  
 command4 = atoi(&data[9]);  
 Serial.println(command4); 
 command5 = atoi(&data[12]);  
 Serial.println(command5);  
 command6 = atoi(&data[15]);  
 Serial.println(command6);

}
    }
        
};

myMQTTBroker myBroker;

WiFiEventHandler probeRequestPrintHandler;

/*
 * WiFi init stuff
 */
void startWiFiClient()
{
  Serial.println("Connecting to "+(String)ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  
  Serial.println("WiFi connected");
  Serial.println("IP address: " + WiFi.localIP().toString());
}

void startWiFiAP()
{
 
  WiFi.softAP(apssid, "<notused>", 7, 0, 0);   //(gateway, "<notused>", 7, 1, 0) for hidden SSID.
  Serial.println("AP started");
  Serial.println("IP address: " + WiFi.softAPIP().toString());
}

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();

    //startWiFiAP();
    startWiFiClient();
    startWiFiAP();

  probeRequestPrintHandler = WiFi.onSoftAPModeProbeRequestReceived(&onProbeRequest);
  delay(1);
//  wifi_set_macaddr(SOFTAP_IF, mac); 
  


  // Start the broker
  Serial.println("Starting MQTT broker");
  myBroker.init();

/*
 * Subscribe to anything
 */
  myBroker.subscribe("#");
}


void loop()
{
     
}


void mqttPublish()    {

/*
 Predefined sensor type table is below:
 volatage = 16, temperature = 26, humidity= 36, pressure= 46, light= 56, OpenClose = 66,
 level = 76, presence = 86, motion = 96 etc.
 */ 


 sprintf(topic1,"%s%i%s%i%s", "Sensordata/", device, "/", device, "/");
 sprintf(topic2,"%s%i%s%i%s", "Sensordata/", device, "/", sensorType1, "/");
 sprintf(topic3,"%s%i%s%i%s", "Sensordata/", device, "/", sensorType2, "/");
 sprintf(topic4,"%s%i%s%i%s", "Sensordata/", device, "/", sensorType3, "/");
 sprintf(topic5,"%s%i%s%i%s", "Sensordata/", device, "/", sensorType4, "/");
 sprintf(topic6,"%s%i%s%i%s", "Sensordata/", device, "/", sensorType5, "/");

 

 
 // myBroker.publish(topic1, (String)device);
 // myBroker.publish("SensorData/device/", (String)device);
  myBroker.publish(topic2, (String)voltage);
  myBroker.publish(topic3, (String)temperature);
  myBroker.publish(topic4, (String)humidity);
  myBroker.publish(topic5, (String)pressure);
  myBroker.publish(topic6, (String)light);

  // wait a second

  delay(5000);
}

void onProbeRequest(const WiFiEventSoftAPModeProbeRequestReceived& dataReceived) {
   
   sendCommand(); 
  /*
    for(int i = 0; i < 6; i++ )
{
     Serial.println(dataReceived.mac[i]);
      }
   */

  if (dataReceived.mac[1] == 6) // only accept data from device with voltage as a sensor type at byte 1.
  {
    sensorType1 = (dataReceived.mac[1]);
    sensorType2 = (dataReceived.mac[2]);
    sensorType3 = (dataReceived.mac[3]);
    sensorType4 = (dataReceived.mac[4]);
    sensorType5 = (dataReceived.mac[5]);
   }
  
  if (dataReceived.mac[0] == 6 || dataReceived.mac[0] == 16 || dataReceived.mac[0] == 26 || dataReceived.mac[0] == 36 || dataReceived.mac[0] == 46 || dataReceived.mac[0] == 56 || dataReceived.mac[0] == 66 || dataReceived.mac[0] == 76 || dataReceived.mac[0] == 86 || dataReceived.mac[0] == 96 || dataReceived.mac[0] == 106 || dataReceived.mac[0] == 116 || dataReceived.mac[0] == 126 || dataReceived.mac[0] == 136 || dataReceived.mac[0] == 146 || dataReceived.mac[0] == 156 || dataReceived.mac[0] == 166 || dataReceived.mac[0] == 176 || dataReceived.mac[0] == 186 || dataReceived.mac[0] == 196 || dataReceived.mac[0] == 206 || dataReceived.mac[0] == 216 || dataReceived.mac[0] == 226 || dataReceived.mac[0] == 236 || dataReceived.mac[0] == 246) // only accept data from certain devices.
  {
    
    Serial.print("Probe Request:- ");
    Serial.print(" Device ID:  ");
    Serial.print(dataReceived.mac[0], DEC);
    device = dataReceived.mac[0];
    Serial.print(" Voltage:  ");
    Serial.print(dataReceived.mac[1], DEC);
    voltage = dataReceived.mac[1];
    voltage = voltage * 2;
    Serial.print(" Temperature:  ");
    Serial.print(dataReceived.mac[2], DEC);
    temperature = dataReceived.mac[2];
    Serial.print(" Humidity:  ");
    Serial.print(dataReceived.mac[3], DEC);
    humidity = dataReceived.mac[3];
    Serial.print(" Pressure:  ");
    Serial.print(dataReceived.mac[4], DEC);
    pressure = dataReceived.mac[4];
    pressure = pressure * 4;
    Serial.print(" Light:  ");
    Serial.println(dataReceived.mac[5], DEC);
    light = dataReceived.mac[5];

      if (voltage < 295)      // if voltage of battery gets to low, print the warning below.
  {
    #if MQTTBROKER
    myBroker.publish("SensorData/warning", "Battery Low");
    #endif
  
    Serial.println("**************Warning :- Battery Voltage low please change batteries********************" );
    Serial.println();
 
}  
   if (dataReceived.mac[1] > 115 && dataReceived.mac[1] < 180)  {
    #if MQTTBROKER
    mqttPublish();
    #endif
    
   }

  } else {

    //Serial.println("Waiting for Data............");

  }
   
}

void sendCommand()  {
  /*
  command1 = 36; //random(25);
  command2 = random(16);
  command3 = random(16);
  command4 = random(4);
  command5 = random(256);
  command6 = random(25); */
  mac[0] = command1;
  mac[1] = command2;
  mac[2] = command3;
  mac[3] = command4;
  mac[4] = command5;
  mac[5] = command6;

  
  
  wifi_set_macaddr(SOFTAP_IF, mac);

}
