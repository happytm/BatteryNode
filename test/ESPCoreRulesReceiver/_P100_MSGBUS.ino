//#######################################################################################################
//#################################### Plugin 100: MSGBUS ###############################################
//#######################################################################################################

/*
 * Commands: 
 * MSGBUSinit <port>,<sendjson 0|1>,<sendOnCount>   Init MSGBUS on port <port> with optional JSON format announce
 *                                                    send announcement every <sendOnCount> minutes
 * MSGBUSRules <pattern>                            Set a filter for ruleprocessing from MSGBUS events, defaults to *
 *                                                    no pattern disables rule processing
 * MSGBUS <message>                                 Send a message on the MSGBUS
 * MSGBUSLog <0|1>                                  Enable/Disable UDP logging, default disabled
*/

#ifdef USES_P100
#define P100_BUILD            7
#define PLUGIN_100
#define PLUGIN_ID_100         100

#define P100_CONFIRM_QUEUE_MAX  8

WiFiUDP P100_portUDP;
int     P100_Port = 0;
boolean P100_Init = false;
boolean P100_log = false;
boolean P100_sendJSON = false;
byte    P100_sendOnCount = 1;
byte    P100_sendCounter = 0;
String  P100_rulesFilter = "*";

struct P100_confirmQueueStruct
{
  String Name;
  byte Attempts;
  byte State;
  byte TimerTicks;
} P100_confirmQueue[CONFIRM_QUEUE_MAX];

boolean Plugin_100(byte function, String& cmd, String& params)
{
  boolean success = false;
  switch (function)
  {
    case PLUGIN_INFO:
      {
        printWebTools += F("<TR><TD>P100 - MSGBUS<TD>");
        printWebTools += P100_BUILD;        
        break;
      }
      
    case PLUGIN_WRITE:
      {
        if (cmd.equalsIgnoreCase(F("MSGBUSinit")))
        {
          success = true;
          int port = parseString(params,1).toInt();

          int sendJSON = parseString(params,2).toInt();
          if(sendJSON == 1)
            P100_sendJSON = true;

          int sendOnCount = parseString(params,3).toInt();
          if(sendOnCount > 0)
            P100_sendOnCount = sendOnCount;
            
          P100_portUDP.begin(port);
          P100_Port = port;
          registerFastPluginCall(&P100_MSGBusReceive);

          // Announce me directly on init 
          P100_MSGBusAnnounceMe();
                      
          // Make other nodes announce themselves
          String event = F("MSGBUS/Refresh");
          P100_UDPSend(event);
          P100_Init = true;
        }

        if (cmd.equalsIgnoreCase(F("MSGBUSRules")))
        {
          success = true;
          P100_rulesFilter = parseString(params,1);
        }

        if (cmd.equalsIgnoreCase(F("MSGBUSLog")))
        {
          success = true;
          P100_log = parseString(params,1).toInt();
        }

        if (cmd.equalsIgnoreCase(F("MSGBUS")))
        {
          success = true;
          if(P100_Init){
            if (params[0] == '>') {
              for (byte x = 0; x < CONFIRM_QUEUE_MAX ; x++) {
                if (P100_confirmQueue[x].State == 0) {
                  P100_confirmQueue[x].Name = params;
                  P100_confirmQueue[x].Attempts = 9;
                  P100_confirmQueue[x].State = 1;
                  P100_confirmQueue[x].TimerTicks = 3;
                  P100_UDPSend(params);
                  break;
                }
              }
            }
            else{
              P100_UDPSend(params);
            }
          }
        }      
        break;
      }

    case PLUGIN_TEN_PER_SECOND:
      {
        if(P100_Init && WifiConnected()){
          P100_MSGBusQueue();
        }
        break;
      }
            
    case PLUGIN_ONCE_A_MINUTE:
      {
        if(P100_Init && WifiConnected()){
          P100_sendCounter++;
          if(P100_sendCounter >= P100_sendOnCount){
            P100_MSGBusAnnounceMe();
            P100_sendCounter=0;
          }
        }
        break;
      }
            
  }
  return success;
}


//********************************************************************************************
// UDP receive message bus packets
//********************************************************************************************
void P100_MSGBusReceive() {
  int packetSize = P100_portUDP.parsePacket();

  if (packetSize > 0)
  {
    IPAddress remoteIP = P100_portUDP.remoteIP();
    char packetBuffer[packetSize + 1];
    int len = P100_portUDP.read(packetBuffer, packetSize);

    // check if this is a plain text message, do not process other messages
    if (packetBuffer[0] > 127)
      return;
      
    packetBuffer[len] = 0;
    String msg = &packetBuffer[0];

    // First process messages that request confirmation
    // These messages start with '>' and must be addressed to my node name
    String mustConfirm = String(">") + Settings.Name + String("/");
    if (msg.startsWith(mustConfirm)) {
      String reply = "<" + msg.substring(1);
      P100_UDPSend(reply);
    }
    if (msg[0] == '>'){
     msg = msg.substring(1); // Strip the '>' request token from the message
    }

    // Process confirmation messages
    if (msg[0] == '<'){
      for (byte x = 0; x < P100_CONFIRM_QUEUE_MAX ; x++) {
        if (P100_confirmQueue[x].Name.substring(1) == msg.substring(1)) {
          P100_confirmQueue[x].State = 0;
          break;
        }
      }

      if(Settings.LogEvents)
        logger->println(String("UDP: ") + msg);
      return; // This message needs no further processing, so return.
    }

    // Special MSGBus system events
    if (msg.substring(0, 7) == F("MSGBUS/")) {
      String sysMSG = msg.substring(7);
      if (sysMSG.substring(0, 9) == F("Hostname=")) {
        String parameters = sysMSG.substring(9);
        String hostName = "";
        String group = "";
        String IP = "";
        int jsonPos = parameters.indexOf("{");
        if (jsonPos == -1)
          {
            hostName = parseString(parameters, 1);
            //IP = parseString(parameters, 2); we just take the remote ip here
            group = parseString(parameters, 3);
            nodelist(remoteIP, hostName, group);
          }
        else
          {
            hostName = parseJSON(parameters, "Hostname");
            hostName.replace("\"","");
            group = parseJSON(parameters, "Groupname");
            group.replace("\"","");
            IP = parseJSON(parameters, "IP");
            IP.replace("\"","");
            char tmpString[26];
            byte IPaddress[4];
            IP.toCharArray(tmpString, 26);
            str2ip(tmpString, IPaddress);
            IPAddress remoteHostIP = IPaddress;
            nodelist(remoteHostIP, hostName, group);
          }
      }
      if (sysMSG.substring(0, 7) == F("Refresh")) {
        P100_MSGBusAnnounceMe();
      }
    }

    #if FEATURE_RULES
      boolean procRules = false;
      if(P100_rulesFilter == "*"){
        procRules = true;
      }else{
        String tmpMsg = msg;
        int pos = P100_rulesFilter.indexOf('*');
        if (pos != -1) // a * sign in rule, so use a'wildcard' match on message
          tmpMsg = msg.substring(0, pos) + "*";
        if(P100_rulesFilter.equalsIgnoreCase(tmpMsg))
          procRules = true;
      }
      
      if(procRules)
        rulesProcessing(FILE_RULES, msg);

    #endif

    if(P100_log)
      logger->println(String("UDP: ") + msg);
  }
}


//********************************************************************************************
// UDP Send message bus packet
//********************************************************************************************
void P100_UDPSend(String message)
{
  IPAddress broadcastIP(255, 255, 255, 255);
  P100_portUDP.beginPacket(broadcastIP, P100_Port);
  P100_portUDP.print(message);
  P100_portUDP.endPacket();
  delay(0);
}


//********************************************************************************************
// UDP Send message bus hostname announcement
//********************************************************************************************
void P100_MSGBusAnnounceMe() {

  char strIP[20];
  IPAddress ip = WiFi.localIP();
  sprintf_P(strIP, PSTR("%u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);

  String msg = F("MSGBUS/Hostname=");
  if(P100_sendJSON){
    msg += "{\"Hostname\":\"";
    msg += Settings.Name;
    msg += "\",\"IP\":\"";
    msg += strIP;
    msg += "\",\"Groupname\":\"";
    msg += Settings.Group;
    msg += "\",\"Mac\":\"";
    msg +=  WiFi.macAddress();
    msg += "\"}";
  }
  else{
    msg += Settings.Name;
    msg += ",";
    msg += strIP;
    msg += ",";
    msg += Settings.Group;
    msg += ",";
    msg +=  WiFi.macAddress();
  }
  P100_UDPSend(msg);
}


//********************************************************************************************
// Check MessageBus queue
//********************************************************************************************
void P100_MSGBusQueue() {
  for (byte x = 0; x < P100_CONFIRM_QUEUE_MAX ; x++) {
    if (P100_confirmQueue[x].State == 1){
      if(P100_confirmQueue[x].Attempts !=0){
        P100_confirmQueue[x].TimerTicks--;
        if(P100_confirmQueue[x].TimerTicks == 0){
          P100_confirmQueue[x].TimerTicks = 3;
          P100_confirmQueue[x].Attempts--;
          P100_UDPSend(P100_confirmQueue[x].Name);
        }
      }
      else{
        logger->println(F("Confirmation Timeout"));
        P100_confirmQueue[x].State = 0;
      }
    }
  }
}

#endif

