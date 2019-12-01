/*
   uMQTTBroker demo for Arduino (C++-style)

   The program defines a custom broker class with callbacks,
   starts it, subscribes locally to anything, and publishs a topic every second.
   Try to connect from a remote client and publish something - the console will show this as well.
*/

#define  MQTTBROKER           true


#include <ESP8266WiFi.h>
#include "uMQTTBroker.h"

/*
   Your WiFi config here
*/
int apChannel = 7;
char gateway[] = "ESP";
char ssid[] = "your_ssid";     // your network SSID (name)
char pass[] = "your_password"; // your network password

int receivedDevice;
float voltage;

int sensorValue1;     
int sensorValue2;     
int sensorValue3;     
int sensorValue4;     

uint8_t securityCode[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Security code must be same at remote sensors to compare.

uint8_t PresencePerson1[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Mac ID of Cell phone #1.
uint8_t PresencePerson2[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Mac ID of Cell phone #2.
uint8_t PresencePerson3[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Mac ID of Cell phone #3.
uint8_t PresencePerson4[6] = {0x36, 0x33, 0x33, 0x33, 0x33, 0x33}; // Mac ID of Cell phone #4.


/*  Predefined sensor type table is below:
  volatage = 6, temperature = 16, humidity= 26, pressure= 36, light= 46, 
  OpenClose = 56, level = 66, presence = 76, motion = 86, rain = 96 etc.
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
char topic7[50];
char topic8[50];
char topic9[50];
char topic10[50];
char topic11[50];
char topic12[50];

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
   Custom broker class with overwritten callback functions
*/
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
      /*
         char* sendCommand;
         char* rest = data_str;
         while ((sendCommand = strtok_r(rest, "/", &rest)))
        //   printf("%s\n", sendCommand);
      */




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

/*
   WiFi init stuff
*/
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

  WiFi.softAP(gateway, "<notused>", apChannel, 1, 0);   //(gateway, "<notused>", 7, 1, 0) for hidden SSID.
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


  sprintf(topic1, "%s%i%s%i%s", "Sensordata/", receivedDevice, "/", receivedDevice, "/");
  sprintf(topic2, "%s%i%s%i%s", "Sensordata/", receivedDevice, "/", sensorType1, "/");
  sprintf(topic3, "%s%i%s%i%s", "Sensordata/", receivedDevice, "/", sensorType2, "/");
  sprintf(topic4, "%s%i%s%i%s", "Sensordata/", receivedDevice, "/", sensorType3, "/");
  sprintf(topic5, "%s%i%s%i%s", "Sensordata/", receivedDevice, "/", sensorType4, "/");
  sprintf(topic6, "%s%i%s%i%s", "Sensordata/", receivedDevice, "/", sensorType5, "/");


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

void onProbeRequest(const WiFiEventSoftAPModeProbeRequestReceived& dataReceived) {
     
      receivedDevice = dataReceived.mac[0];

      if (dataReceived.mac[0] == PresencePerson1[0] && dataReceived.mac[1] == PresencePerson1[1] && dataReceived.mac[2] == PresencePerson1[2]) 
      {  // write code to match MAC ID of cell phone to predefined variable and store presence/absense in new variable.
         Serial.println("################ Person 1 arrived ###################### ");
         myBroker.publish("Sensordata/Person1/", "Home");
         
         } 
      
      if (dataReceived.mac[0] == PresencePerson2[0] && dataReceived.mac[1] == PresencePerson2[1] && dataReceived.mac[2] == PresencePerson2[2]) 
      {  
         Serial.println("################ Person 2 arrived ###################### ");
         myBroker.publish("Sensordata/Person2/", "Home");
      } 

      if (dataReceived.mac[0] == PresencePerson3[0] && dataReceived.mac[1] == PresencePerson3[1] && dataReceived.mac[2] == PresencePerson3[2]) 
      {  
         Serial.println("################ Person 3 arrived ###################### ");
         myBroker.publish("Sensordata/Person3/", "Home");
      } 

      if (dataReceived.mac[0] == PresencePerson4[0] && dataReceived.mac[1] == PresencePerson4[1] && dataReceived.mac[2] == PresencePerson4[2]) 
      {  
         Serial.println("################ Person 4 arrived ###################### ");
         myBroker.publish("Sensordata/Person4/", "Home");
      } 
           
    
        
     if (dataReceived.mac[0] == 6 || dataReceived.mac[0] == 16 || dataReceived.mac[0] == 26 || dataReceived.mac[0] == 36 || dataReceived.mac[0] == 46 || dataReceived.mac[0] == 56 || dataReceived.mac[0] == 66 || dataReceived.mac[0] == 76 || dataReceived.mac[0] == 86 || dataReceived.mac[0] == 96 || dataReceived.mac[0] == 106 || dataReceived.mac[0] == 116 || dataReceived.mac[0] == 126 || dataReceived.mac[0] == 136 || dataReceived.mac[0] == 146 || dataReceived.mac[0] == 156 || dataReceived.mac[0] == 166 || dataReceived.mac[0] == 176 || dataReceived.mac[0] == 186 || dataReceived.mac[0] == 196 || dataReceived.mac[0] == 206 || dataReceived.mac[0] == 216 || dataReceived.mac[0] == 226 || dataReceived.mac[0] == 236 || dataReceived.mac[0] == 246) // only accept data from certain devices.
      {
        
        sendCommand();

      if (dataReceived.mac[1] == 6) // only accept data from device with voltage as a sensor type at byte 1.
  {
      sensorType1 = (dataReceived.mac[1]);
      sensorType2 = (dataReceived.mac[2]);
      sensorType3 = (dataReceived.mac[3]);
      sensorType4 = (dataReceived.mac[4]);
      sensorType5 = (dataReceived.mac[5]); 
      
    }

    Serial.print("Signal Strength of remote sensor: ");
    Serial.println(dataReceived.rssi); 
    myBroker.publish("Sensordata/Signal/", (String)dataReceived.rssi);

   
        
    Serial.print("Probe Request:- ");
    Serial.print(" Device ID:  ");
    Serial.print(dataReceived.mac[0], DEC);
    receivedDevice = dataReceived.mac[0];
    
    Serial.print(" Voltage:  ");
    Serial.print(dataReceived.mac[1], DEC);
    voltage = dataReceived.mac[1];
    voltage = voltage * 2 / 100;

    Serial.print(" Sensor 1:  ");
    Serial.print(dataReceived.mac[2], DEC);
    sensorValue1 = dataReceived.mac[2];
    
    Serial.print(" Sensor 2:  ");
    Serial.print(dataReceived.mac[3], DEC);
    sensorValue2 = dataReceived.mac[3];
   
    Serial.print(" Sensor 3:  ");
    Serial.print(dataReceived.mac[4], DEC);
    sensorValue3 = dataReceived.mac[4];
    sensorValue3 = sensorValue3 * 4;
      
    Serial.print(" Sensor 4:  ");
    Serial.println(dataReceived.mac[5], DEC);
    sensorValue4 = dataReceived.mac[5];
       
  
    if (voltage < 295)      // if voltage of battery gets to low, print the warning below.
    {
        #if MQTTBROKER
           delay(1);
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
      //}
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
