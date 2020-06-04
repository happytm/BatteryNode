//#######################################################################################################
//#################################### Plugin 101: MQTT #################################################
//#######################################################################################################

/*
 * Commands: 
 * MQTTinit                           Init MQTT broker, it will generate MQTT#Connect event when ready
 * MQTTconnect <ip>,<port>            Connect to MQTT broker on <ip> using port <port>
 * MQTTsubscribe <topic>              Subscribe to MQTT topic
 * MQTTpublish <topic>,<payload>      Publish an MQTT message
 * MQTTNodeListInit                   Use the Nodelist and host announcements on MQTT (topic MSGBUS/)
*/

#ifdef USES_P101
#define P101_BUILD            7
#define PLUGIN_101
#define PLUGIN_ID_101         101

#include <PubSubClient.h>
WiFiClient *P101_mqtt;
PubSubClient *P101_MQTTclient;

unsigned long P101_connectionFailures;
boolean P101_Init = false;
boolean P101_NodeListInit = false;

boolean Plugin_101(byte function, String& cmd, String& params)
{
  boolean success = false;
  switch (function)
  {
    case PLUGIN_INFO:
      {
        printWebTools += F("<TR><TD>P101 - MQTT<TD>");
        printWebTools += P101_BUILD;        
        break;
      }
      
    case PLUGIN_WRITE:
      {
        if (cmd.equalsIgnoreCase(F("MQTTinit")))
        {
          success = true;
          if(!P101_Init){
            P101_mqtt = new WiFiClient ;
            P101_MQTTclient = new PubSubClient(*P101_mqtt);
            registerFastPluginCall(&P101_MQTTReceive);
            P101_Init = true;
            #if FEATURE_RULES
              String event = F("MQTT#Connect");
              rulesProcessing(FILE_RULES, event);
            #endif
          }
        }
        if (cmd.equalsIgnoreCase(F("MQTTconnect")))
        {
          success = true;
          if(P101_Init){
            char tmpString[26];
            String strIP = parseString(params,1);
            int port = parseString(params,2).toInt();
            String user = parseString(params,3);
            String password = parseString(params,4);
            byte IP[4];
            strIP.toCharArray(tmpString, 26);
            str2ip(tmpString, IP);
            IPAddress mqttIP(IP);
            P101_MQTTConnect(mqttIP, port, user, password);
          }
        }        
        if (cmd.equalsIgnoreCase(F("MQTTpublish")))
        {
          success = true;
          if(P101_Init){
            String topic = parseString(params,1);
            int payloadpos = getParamStartPos(params,2);
            String payload = params.substring(payloadpos);
            P101_MQTTclient->publish(topic.c_str(), payload.c_str());
          }
        }
        if (cmd.equalsIgnoreCase(F("MQTTsubscribe")))
        {
          success = true;
          if(P101_Init){
            P101_MQTTclient->subscribe(params.c_str());
          }
        }
        
        if (cmd.equalsIgnoreCase(F("MQTTNodeListInit")))
        {
          success = true;
          if(P101_Init){
            P101_NodeListInit = true;
            String msgbus = F("MSGBUS/#");
            P101_MQTTclient->subscribe(msgbus.c_str());
          }
        }
       
        break;
      }

    case PLUGIN_ONCE_A_MINUTE:
      {
        if(P101_Init && WifiConnected()){
          P101_MQTTCheck();
          if(P101_NodeListInit)
            P101_MSGBusAnnounceMe();
        }
        break;
      }
      
  }
  return success;
}


//********************************************************************************************
// MQTT receive
//********************************************************************************************
void P101_MQTTReceive() {
  P101_MQTTclient->loop();
}

        
/*********************************************************************************************\
 * Handle incoming MQTT messages
\*********************************************************************************************/
void P101_callback(char* c_topic, byte* b_payload, unsigned int length) {

  char c_payload[length];
  strncpy(c_payload,(char*)b_payload,length);
  c_payload[length] = 0;

  String topic = c_topic;
  String msg = c_payload;
  msg.replace("\r","");
  msg.replace("\n","");

  // Process MSGBUS hostname events
  if(P101_NodeListInit){
    if (topic == F("MSGBUS/Hostname")) {
      String hostName = parseJSON(msg, "Hostname");
      hostName.replace("\"","");
      String group = parseJSON(msg, "Groupname");
      group.replace("\"","");
      String IP = parseJSON(msg, "IP");
      IP.replace("\"","");
      char tmpString[26];
      byte IPaddress[4];
      IP.toCharArray(tmpString, 26);
      str2ip(tmpString, IPaddress);
      IPAddress remoteHostIP = IPaddress;
      nodelist(remoteHostIP, hostName, group);
    }
  }

  #if FEATURE_RULES
    String event = topic;
    event += "=";
    event += msg; 
    rulesProcessing(FILE_RULES, event);
  #endif
}


/*********************************************************************************************\
 * Connect to MQTT message broker
\*********************************************************************************************/
void P101_MQTTConnect(IPAddress IP, int port, String user, String password)
{
  P101_MQTTclient->setServer(IP, port);
  P101_MQTTclient->setCallback(P101_callback);

  // MQTT needs a unique clientname to subscribe to broker
  String clientid = F("ESPClient");
  clientid += Settings.Name;

  String LWTTopic = F("LWT");
  String LWTMsg = F("Connection lost ");
  LWTMsg +=Settings.Name;
  
  for (byte x = 1; x < 3; x++)
  {
    String log = "";
    boolean MQTTresult = false;

    //boolean connect(const char* id);
    //boolean connect(const char* id, const char* user, const char* pass);
    //boolean connect(const char* id, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage);
    //boolean connect(const char* id, const char* user, const char* pass, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage);

    if ((user != "") && (password != ""))
      MQTTresult = P101_MQTTclient->connect(clientid.c_str(), user.c_str(), password.c_str(), LWTTopic.c_str(), 0, 0, LWTMsg.c_str());
    else
      MQTTresult = P101_MQTTclient->connect(clientid.c_str(), LWTTopic.c_str(), 0, 0, LWTMsg.c_str());

    if (MQTTresult)
    {
      log = F("MQTT : Connected to broker");
      logger->println(log);
      #if FEATURE_RULES
        String event = F("MQTT#Connected");
        rulesProcessing(FILE_RULES, event);
      #endif
      break; // end loop if succesfull
    }
    else
    {
      log = F("MQTT : Failed to connected to broker");
      logger->println(log);
    }

    delay(500);
  }
}


/*********************************************************************************************\
 * Check connection MQTT message broker
\*********************************************************************************************/
void P101_MQTTCheck()
{
  if (!P101_MQTTclient->connected())
  {
    String log = F("MQTT : Connection lost");
    logger->println(log);
    P101_connectionFailures += 2;
    P101_MQTTclient->disconnect();
    delay(1000);
    #if FEATURE_RULES
      String event = F("MQTT#Connect");
      rulesProcessing(FILE_RULES, event);
    #endif
  }
  else if (P101_connectionFailures)
    P101_connectionFailures--;
}

//********************************************************************************************
// Send message bus hostname announcement
//********************************************************************************************
void P101_MSGBusAnnounceMe() {

  char strIP[20];
  IPAddress ip = WiFi.localIP();
  sprintf_P(strIP, PSTR("%u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);

  String topic = F("MSGBUS/Hostname");
  String payload = F("{\"Hostname\":\"");
  payload += Settings.Name;
  payload += "\",\"IP\":\"";
  payload += strIP;
  payload += "\",\"Groupname\":\"";
  payload += Settings.Group;
  payload += "\",\"Mac\":\"";
  payload +=  WiFi.macAddress();
  payload += "\"}";
  P101_MQTTclient->publish(topic.c_str(), payload.c_str());
}

#endif

