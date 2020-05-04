/*
   uMQTTBroker demo for Arduino (C++-style)
   The program defines a custom broker class with callbacks,
   starts it, subscribes locally to anything, and publishs a topic every second.
   Try to connect from a remote client and publish something - the console will show this as well.
*/

#define  MQTTBROKER           true


#include <ESP8266WiFi.h>
#include "uMQTTBroker.h"  // https://github.com/martin-ger/uMQTTBroker
#include "JsonLogger.h"   // https://github.com/ravelab/JsonLogger

char sensorTypes[256], sensorValues[256], deviceStatus[256];

//   Your WiFi config here

int apChannel = 7;
char* room = "Livingroom";  // Needed for person locator.Each location must run probeReceiver sketch to implement person locator.
int rssiThreshold = -50; // Adjust according to signal strength by trial & error.
char apssid[] = "ESP";
char ssid[] = "ssid";     // your network SSID (name)
char pass[] = "password"; // your network password
//bool WiFiAP = false;      // Do yo want the ESP as AP?

int device;
float voltage;
uint8_t data[12];

int sensorValue1; int sensorValue2; int sensorValue3; int sensorValue4; int sensorValue5;
int deviceStatus1; int deviceStatus2; int deviceStatus3; int deviceStatus4; int deviceStatus5;
//int sensorType1; int sensorType2; int sensorType3; int sensorType4; int sensorType5;
int command1 = 36; int command2;  int command3;  int command4;  int command5; int command6;
uint8_t mac[6] = {command1, command2, command3, command4, command5, command6};
char topic1[50]; char topic2[50]; char topic3[50]; char topic4[50]; char topic5[50]; char topic6[50]; char topic7[50]; char topic8[50]; char topic9[50]; char topic10[50]; char topic11[50]; char topic12[50];

const char* sensorType1; const char* sensorType2; const char* sensorType3; const char* sensorType4;
const char* location;



//uint8_t securityCode[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Security code must be same at remote sensors to compare.
uint8_t PresencePerson1[6] = {0x36, 0x33, 0x33, 0x33, 0xC0, 0x8A}; // Mac ID of Cell phone #1.
uint8_t PresencePerson2[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Mac ID of Cell phone #2.
uint8_t PresencePerson3[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Mac ID of Cell phone #3.
uint8_t PresencePerson4[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Mac ID of Cell phone #4.


// ==================== end of TUNEABLE PARAMETERS ====================

extern "C" void preinit() {
  wifi_set_opmode(STATIONAP_MODE);
  wifi_set_macaddr(SOFTAP_IF, mac);
}



//   Custom broker class with overwritten callback functions

class myMQTTBroker: public uMQTTBroker
{
  public:
    virtual bool onConnect(IPAddress addr, uint16_t client_count) {
      Serial.println(addr.toString() + " connected");
      return true;
    }

    virtual bool onAuth(String username, String password) {
      Serial.println("Username/Password: " + username + "/" + password);
      return true;
    }

    virtual void onData(String topic, const char *data, uint32_t length) {

      char data_str[length + 1];
      os_memcpy(data_str, data, length);
      data_str[length] = '\0';
      Serial.println("received topic '" + topic + (String)data_str);
      
      if (topic == "command/")   {

       /************************************************************
        Command structure:  (commands are issued via MQTT payload with topic name "command/"

        Command1 = Device ID Number -       device ID must be 2 digits end with 2,6,A or E. See https://serverfault.com/questions/40712/what-range-of-mac-addresses-can-i-safely-use-for-my-virtual-machines.
                                            use any of following for devie ID ending with 6.
                                            06,16,26,36,46,56,66,76,86,96,106,116,126,136,146,156,166,176,186,196,206,216,226,236,246 etc.

        Command2 = Command type  -     value 1 to 9 is reserved for following commands(must have 0 as first digit):
                                       
                                       01 = digitalWright or analogWrite.
                                            Example command payload 36/01/00 0r 01/ for digitalWrite.
                                            Example command payload 36/01/02 to 256/ for analogWrite.
                                       02 = digitalRead.
                                            Example command payload 36/02/01 to 05 or 12 to 16/ 
                                       03 = analogRead, 
                                       04=  Reserved, 
                                       05 = Neopixel etc.
                                            Example command payload 36/05/01 to 05 or 12 to 16/00 to 256/00 to 256/00 to 256/
                                       06 = change sensoor types.First byte must be target device id and 
                                            second byte must be 06 (sensor type voltage). Rest of 4 bytes (each ending with 6) can be changed according to hardware setup.
                                            Example command payload 36/06/16/26/36/46/.
                                      
                                       07 = change apChannel, 
                                       08 = change sleeptime. 
                                            Example command payload 36/08/00 to 255/ (Sleep Time in minutes).  
                                       09 = Activate alternative code for OTA,Wifimanager ETC.
                                            Example command payload 36/09/00 or 01/(01 to activate alternative Code).
                                           
                                            value 10 to 20 is reserved for following commands:
                                       10 = change define DUPLEX, 11 = change define SEURITY, 12 = change define OTA, 13 = change define uMQTTBROKER etc.
                                   
        Command3 = Command  pinNumber  -    pinNumber in case of command type 01 to 04 above. Neopixel LED number in case of command type 05.
                                            Predefined number to represent value of command type 11 to 20 above.
                                            00 or 01 to represent false/or true for defines in case of command type 21 to 30. 
                                                        
        Command4 = Command pinValue  -      00 or 255 in case of command type 01 (digitalWrite & analogWrite)  or RED neopixel value in case of command type 05.
        
        Command5 = Command extraValue1  -   00 to 255 for GREEN neopixel in case of command type 05                        
                                            or sensorType value in case of command 06.
        Command6 = Command extraValue1  -   00 to 255 for BLUE neopixel in case of command type 05 
                                            or sensorType value in case of command 06. 
                                   
*************************************************************/
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


//   WiFi init stuff

void startWiFiClient()
{
  Serial.println("Connecting to " + (String)ssid);
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

  WiFi.softAP(apssid, "<notused>", apChannel, 0, 0);   //(gateway, "<notused>", 7, 1, 0) for hidden SSID.
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
     Subscribe to anything
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

   if (voltage > 2) {
      int len = json(sensorValues, "s|Location", location, "f3|Volts", voltage, "s|Sensor1", sensorType1, "i|SensorValue1", sensorValue1, "s|Sensor2", sensorType2, "i|SensorValue2", sensorValue2, "s|Sensor3", sensorType3, "i|SensorValue3", sensorValue3, "s|Sensor4", sensorType4, "i|SensorValue4", sensorValue4);
     // int len = json(sensorValues, "s|location", location, "f3|Volts", voltage, sensorType1, sensorValue1, "i|SensorValue2", sensorValue2, "i|SensorValue3", sensorValue3, "i|SensorValue4", sensorValue4);
     // Serial.println(String(sensorValues));
      myBroker.publish("SensorValues/", String(sensorValues));
      } 
      
  delay(5000);
}
  /*
  sprintf(topic1, "%s%i%s%i%s", "Sensordata/", device, "/", device, "/");
  sprintf(topic2, "%s%i%s%i%s", "Sensordata/", device, "/", sensorType1, "/");
  sprintf(topic3, "%s%i%s%i%s", "Sensordata/", device, "/", sensorType2, "/");
  sprintf(topic4, "%s%i%s%i%s", "Sensordata/", device, "/", sensorType3, "/");
  sprintf(topic5, "%s%i%s%i%s", "Sensordata/", device, "/", sensorType4, "/");
  sprintf(topic6, "%s%i%s%i%s", "Sensordata/", device, "/", sensorType5, "/");
  


  // myBroker.publish(topic1, (String)device);
  // myBroker.publish("SensorData/device/", (String)device);
  myBroker.publish(topic2, (String)voltage);
  myBroker.publish(topic3, (String)sensorValue1);
  myBroker.publish(topic4, (String)sensorValue2);
  myBroker.publish(topic5, (String)sensorValue3);
  myBroker.publish(topic6, (String)sensorValue4);
  
  

  // wait a second

  delay(5000);
}
*/
void onProbeRequest(const WiFiEventSoftAPModeProbeRequestReceived& dataReceived) {
     
      
     if (dataReceived.mac[0] == PresencePerson1[0] && dataReceived.mac[1] == PresencePerson1[1] && dataReceived.mac[2] == PresencePerson1[2])
  { // write code to match MAC ID of cell phone to predefined variable and store presence/absense in new variable.
    Serial.println("################ Person 1 arrived ###################### ");
    myBroker.publish("Sensordata/Person1/", "Home");
    Serial.print("Signal Strength of remote sensor: ");
    Serial.println(dataReceived.rssi);
    myBroker.publish("Sensordata/Signal/", (String)dataReceived.rssi);

    if (dataReceived.rssi > rssiThreshold) // Adjust according to signal strength by trial & error.
    { // write code to match MAC ID of cell phone to predefined variable and store presence/absense in new variable.
    myBroker.publish("Sensordata/Person1/in/", room);

    }
  }
 
     if (dataReceived.mac[0] == 6 || dataReceived.mac[0] == 16 || dataReceived.mac[0] == 26 || dataReceived.mac[0] == 36 || dataReceived.mac[0] == 46 || dataReceived.mac[0] == 56 || dataReceived.mac[0] == 66 || dataReceived.mac[0] == 76 || dataReceived.mac[0] == 86 || dataReceived.mac[0] == 96 || dataReceived.mac[0] == 106 || dataReceived.mac[0] == 116 || dataReceived.mac[0] == 126 || dataReceived.mac[0] == 136 || dataReceived.mac[0] == 146 || dataReceived.mac[0] == 156 || dataReceived.mac[0] == 166 || dataReceived.mac[0] == 176 || dataReceived.mac[0] == 186 || dataReceived.mac[0] == 196 || dataReceived.mac[0] == 206 || dataReceived.mac[0] == 216 || dataReceived.mac[0] == 226 || dataReceived.mac[0] == 236 || dataReceived.mac[0] == 246) // only accept data from certain devices.
      {
     
    sendCommand();

      if (dataReceived.mac[1] == 6) // only accept data from device with voltage as a sensor type at byte 1.
  {

      //  Predefined sensor type table is below:
     //  volatage = 06, temperature = 16, humidity= 26, pressure= 36, light= 46, OpenClose = 66,
     //  level = 66, presence = 76, motion = 86 custom = 96 etc.
 
      if (dataReceived.mac[2] == 16) sensorType1 = "Temperature";
      if (dataReceived.mac[2] == 26) sensorType1 = "Humidity";
      if (dataReceived.mac[2] == 36) sensorType1 = "Pressure";
      if (dataReceived.mac[2] == 46) sensorType1 = "Light";
      if (dataReceived.mac[2] == 56) sensorType1 = "OpenClose";
      if (dataReceived.mac[2] == 66) sensorType1 = "Level";
      if (dataReceived.mac[2] == 76) sensorType1 = "Presence";
      if (dataReceived.mac[2] == 86) sensorType1 = "Motion";
      if (dataReceived.mac[2] == 96) sensorType1 = "Custom";

      if (dataReceived.mac[3] == 16) sensorType2 = "Temperature";
      if (dataReceived.mac[3] == 26) sensorType2 = "Humidity";
      if (dataReceived.mac[3] == 36) sensorType2 = "Pressure";
      if (dataReceived.mac[3] == 46) sensorType2 = "Light";
      if (dataReceived.mac[3] == 56) sensorType2 = "OpenClose";
      if (dataReceived.mac[3] == 66) sensorType2 = "Level";
      if (dataReceived.mac[3] == 76) sensorType2 = "Presence";
      if (dataReceived.mac[3] == 86) sensorType2 = "Motion";
      if (dataReceived.mac[3] == 96) sensorType2 = "Custom";

      if (dataReceived.mac[4] == 16) sensorType3 = "Temperature";
      if (dataReceived.mac[4] == 26) sensorType3 = "Humidity";
      if (dataReceived.mac[4] == 36) sensorType3 = "Pressure";
      if (dataReceived.mac[4] == 46) sensorType3 = "Light";
      if (dataReceived.mac[4] == 56) sensorType3 = "OpenClose";
      if (dataReceived.mac[4] == 66) sensorType3 = "Level";
      if (dataReceived.mac[4] == 76) sensorType3 = "Presence";
      if (dataReceived.mac[4] == 86) sensorType3 = "Motion";
      if (dataReceived.mac[4] == 96) sensorType3 = "Custom";


      if (dataReceived.mac[5] == 16) sensorType4 = "Temperature";
      if (dataReceived.mac[5] == 26) sensorType4 = "Humidity";
      if (dataReceived.mac[5] == 36) sensorType4 = "Pressure";
      if (dataReceived.mac[5] == 46) sensorType4 = "Light";
      if (dataReceived.mac[5] == 56) sensorType4 = "OpenClose";
      if (dataReceived.mac[5] == 66) sensorType4 = "Level";
      if (dataReceived.mac[5] == 76) sensorType4 = "Presence";
      if (dataReceived.mac[5] == 86) sensorType4 = "Motion";
      if (dataReceived.mac[5] == 96) sensorType4 = "Custom";

    } else if (dataReceived.mac[3] == apChannel)

    {  
       
      int len = json(deviceStatus, "s|Location", location, "i|SingnalStrength", dataReceived.rssi, "i|DeviceMode", dataReceived.mac[1], "i|DeviceIP", dataReceived.mac[2], "i|WiFiChannel", dataReceived.mac[3], "i|SleepTime", dataReceived.mac[4], "i|UpTime", dataReceived.mac[5]);
   //   Serial.println(String(deviceStatus));
      myBroker.publish("DeviceStatus/", String(deviceStatus));
      delay(100);
      
      deviceStatus1 = (dataReceived.mac[1]);
      deviceStatus2 = (dataReceived.mac[2]);
      deviceStatus3 = (dataReceived.mac[3]);
      deviceStatus4 = (dataReceived.mac[4]);
      deviceStatus5 = (dataReceived.mac[5]);

      mqttPublish();
    }
    
    device = dataReceived.mac[0];
    
    voltage = dataReceived.mac[1];
    voltage = voltage * 2 / 100;

    sensorValue1 = dataReceived.mac[2];
    sensorValue2 = dataReceived.mac[3];
   
    if (sensorType4 == "Pressure")
   {  
    sensorValue3 = dataReceived.mac[4];
    sensorValue3 = sensorValue3 * 4;
   } else {    
    sensorValue3 = dataReceived.mac[4];
   }
    
    sensorValue4 = dataReceived.mac[5];
   
    if (voltage < 295)      // if voltage of battery gets to low, print the warning below.
    {
           delay(1);
           myBroker.publish("Warning/Battery Low/", location);
     }
     
    
} else {

 Serial.println("Waiting for Data............");
  }
 }

void sendCommand()  {
  
  mac[0] = command1;
  mac[1] = command2;
  mac[2] = command3;
  mac[3] = command4;
  mac[4] = command5;
  mac[5] = command6;

  wifi_set_macaddr(SOFTAP_IF, mac);
}
