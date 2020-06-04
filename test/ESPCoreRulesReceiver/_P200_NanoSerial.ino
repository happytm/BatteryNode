//#######################################################################################################
//#################################### Plugin 200: Nano Serial ##########################################
//#######################################################################################################

/*
 * Commands:
 * NanoSerialInit                                Initialize communications with NanoCore device
 * NanoProg                                      Start OTA programming mode (run an AVRDUDE session within 10 seconds)
*/

#ifdef USES_P200
#define P200_BUILD            7
#define PLUGIN_200
#define PLUGIN_ID_200         200

#define NANO_RULES_MAX_SIZE   512

struct P200_LogStruct
{
  unsigned long timeStamp;
  unsigned long delta;
  String *Message;
};
P200_LogStruct *P200_Logging;

int P200_logcount = -1;
int P200_logMax = 10;
unsigned long P200_lastLogTime = 0;
int P200_bootSizeMax = 512;
int P200_rulesSizeMax = 512;

boolean Plugin_200(byte function, String& cmd, String& params)
{
  boolean success = false;
  static boolean init = false;

  switch (function)
  {
    case PLUGIN_INFO:
      {
        printWebTools += F("<TR><TD>P200 - NanoCore<TD>");
        printWebTools += P200_BUILD;
        break;
      }
          
    case PLUGIN_SERIAL_IN:
      {
        if(init){
          P200_logcount++;
          if (P200_logcount >= P200_logMax)
            P200_logcount = 0;
          #if FEATURE_TIME  
            P200_Logging[P200_logcount].timeStamp = sysTime;
          #else
            P200_Logging[P200_logcount].timeStamp = millis();
          #endif
          P200_Logging[P200_logcount].delta = millis()-P200_lastLogTime;
          P200_lastLogTime = millis();
          *P200_Logging[P200_logcount].Message = cmd;
        }
        break;
      }


    case PLUGIN_WRITE:
      {

        if (cmd.equalsIgnoreCase(F("NanoSerialInit")))
        {
          success = true;

          P200_Logging = (P200_LogStruct*)malloc(sizeof(P200_LogStruct) * P200_logMax);
          if (P200_Logging != NULL) {
            init = true;
            for(byte x=0; x < P200_logMax; x++){
             P200_Logging[x].timeStamp = 0;
             P200_Logging[x].delta = 0;
             P200_Logging[x].Message = new String();
            }
          }
          
          Serial.println("echo 0");
          while (Serial.available()) Serial.read();
          Serial.println("rm b");
          String reply = Serial.readStringUntil('\n');
          if (reply.length() != 0)
            P200_bootSizeMax = reply.toInt();
          Serial.println("rm r");
          reply = Serial.readStringUntil('\n');
          if (reply.length() != 0)
            P200_rulesSizeMax = reply.toInt();
          WebServer.on("/nanoedit", P200_handle_nanoedit);
          WebServer.on("/nanolog", P200_handle_log);
          WebServer.on("/nanocmd", P200_handle_cmd);
        }

        if (cmd.equalsIgnoreCase(F("NanoProg")))
        {
          success = true;
          boolean proxyInit = false;
          int TXdelay = 100; // serial send delay in uSec
          int RXwait  = 500; // serial receive wait time in mSec
          if(parseString(params,1).length() > 0){
            proxyInit = parseString(params,1).toInt();
            TXdelay = parseString(params,2).toInt();
            RXwait = parseString(params,3).toInt();
          }
          P200_ProgMode(proxyInit, TXdelay, RXwait);
        }
        break;
      }
  }
  return success;
}


//********************************************************************************
// File editor
//********************************************************************************
void P200_handle_nanoedit() {

  String reply = "";
  addHeader(true, reply);

  reply += printWebString;

  String fileName = WebServer.arg(F("file"));
  String content = WebServer.arg(F("content"));

  int fileSizeMax = NANO_RULES_MAX_SIZE;
  if (fileName == "b")
    fileSizeMax = P200_bootSizeMax;
  if (fileName == "r")
    fileSizeMax = P200_rulesSizeMax;

  if (WebServer.args() >= 2) {
    if (content.length() > fileSizeMax)
      reply += F("<span style=\"color:red\">Data was not saved, exceeds web editor limit!</span>");
    else
    {
      Serial.println("echo 0");
      Serial.print("rc ");
      Serial.println(fileName);
      while (Serial.available()) Serial.read();
      Serial.print("rl ");
      Serial.println(fileName);
      const char* data = content.c_str();
      for (int x = 0; x < content.length(); x++) {
        Serial.write(*data);
        if (*data == '\n') { // in case of newline, wait for the NanoCore to write the line to eeprom
          for (byte d = 0; d < 250; d++) { // wait max 250 ms, or stop waiting on 0x0D char
            if (Serial.available())
              if (Serial.read() == 0x0D) // got confirmation on line write to eeprom
                break;
            delay(1);
          }
        }
        delayMicroseconds(500);
        data++;
      }
      Serial.write(0);
    }
  }

  int size = 0;
  Serial.println("echo 0");
  delay(10);
  while (Serial.available()) Serial.read();
  Serial.print("rs ");
  Serial.println(fileName);
  reply += F("<form method='post'>");
  reply += F("<textarea name='content' rows='15' cols='80' wrap='off'>");
  unsigned long timer = millis() + 100;
  boolean msgEnd = false;
  while (timer > millis() && !msgEnd) {
    while (Serial.available())
    {
      timer = millis() + 50;
      size++;
      byte data = Serial.read();
      if (data != 0) {
        String c((char)data);
        htmlEscape(c);
        reply += c;
      }
      else {
        msgEnd = true;
        break;
      }
    }
  }
  reply += F("</textarea>");

  reply += F("<br>Current size: ");
  reply += size;
  reply += F(" characters (Max ");
  reply += fileSizeMax;
  reply += F(")");

  reply += F("<br><input class=\"button-link\" type='submit' value='Submit'>");
  reply += F("</form>");
  WebServer.send(200, "text/html", reply);
}


//********************************************************************************
// Web Interface log page
//********************************************************************************
void P200_handle_log() {

  #if FEATURE_TIME
    struct  timeStruct logTime;
  #endif
  
  String reply = "";
  addHeader(true, reply);

  reply += printWebString;

  reply += F("<script language='JavaScript'>function RefreshMe(){window.location = window.location}setTimeout('RefreshMe()', 3000);</script>");
  reply += F("<table><TR><TD>");

  if (P200_logcount != -1)
  {
    byte counter = P200_logcount;
    do
    {
      if (P200_Logging[counter].timeStamp > 0)
      {
        #if FEATURE_TIME
          breakTime(P200_Logging[counter].timeStamp,logTime);
          reply += logTime.Month;
          reply += "-";
          reply += logTime.Day;
          reply += " ";
          reply += logTime.Hour;
          reply += ":";
          if (logTime.Minute < 10)
            reply += F("0");
          reply += logTime.Minute;
          reply += ":";
          if (logTime.Second < 10)
            reply += F("0");
          reply += logTime.Second;
        #else
          reply += P200_Logging[counter].timeStamp;
        #endif
        reply += " +";
        reply += P200_Logging[counter].delta;
        reply += " : ";
        reply += *P200_Logging[counter].Message;
        reply += F("<BR>");
      }
      counter--;
      if (counter == 255)
        counter = P200_logMax-1;
    }  while (counter != P200_logcount);
  }
  reply += F("</table>");
  WebServer.send(200, "text/html", reply);
}


//********************************************************************************
// Web Interface debug page
//********************************************************************************
void P200_handle_cmd() {

  String webrequest = WebServer.arg("cmd");
  String reply = "";
  addHeader(true, reply);

  reply += printWebString;

  if (webrequest.length() > 0)
  {
    Serial.println(webrequest);
  }

  reply += F("<form>");
  reply += F("<table>");
  //  reply += F("<TR><TD>System<TD><a class=\"button-link\" href=\"/?cmd=reboot\">Reboot</a><br><br>");

  reply += F("<textarea name='content' rows='15' cols='80' wrap='off'>");
  unsigned long timer = millis() + 100;
  while (timer > millis()) {
    while (Serial.available())
    {
      timer = millis() + 50;
      byte data = Serial.read();
      if (data != 0) {
        String c((char)data);
        htmlEscape(c);
        reply += c;
      }
      else
        break;
    }
  }
  reply += F("</textarea>");

  reply += F("<TR><TD>Command<TD>");
  reply += F("<input type='text' name='cmd' value='");
  reply += webrequest;
  reply += F("'><TR><TD><TD><input class=\"button-link\" type='submit' value='Submit'><TR><TD>");

  reply += F("</table></form>");
  WebServer.send(200, "text/html", reply);
}


void P200_ProgMode(boolean proxyInit, int TXdelay, int RXwait) {

  // go into Nanocore programming mode, listen on tcp port 2323 for avrdude
  // sample avrdude commdand where the ESP ip address is 192.168.0.106 and the filename is "NanoCore.ino.standard.hex"
  
  //   avrdude -V -patmega328p -carduino -Pnet:192.168.0.106:2323 -D -Uflash:w:NanoCore.ino.standard.hex:i

  // wait for 10 seconds to get a tcp connection from avrdude
  // log progress to telnet on port 23

  String log = "";

  P200_telnetLog(F("Start programming mode"));
  log = F(" proxyInit:");
  log += proxyInit;
  P200_telnetLog(log);
  log = F(" TXdelay:");
  log += TXdelay;
  P200_telnetLog(log);
  log = F(" RXwait:");
  log += RXwait;
  P200_telnetLog(log);

  Serial.begin(115200);
  unsigned long end = millis() + 10000;

  WiFiServer progServer(2323);
  WiFiClient progClient;

  progServer.begin();
  progServer.setNoDelay(true);

  boolean initDone = false;
  boolean initReply = false;
  unsigned long start = 0;
  unsigned long duration = 0;

  while (millis() < end) {
    if (progServer.hasClient())
    {
      start = millis();
      if (progClient) progClient.stop();
      progClient = progServer.available();
      P200_telnetLog(F("Client connected"));
      delay(0);
      if(proxyInit){ // we do the init message instead of waiting for ARVdude to do it
        Serial.write(0x30);
        Serial.write(0x20);
      }
    }

    // Read command from avrdude
    if (progClient.connected()) {

      while (Serial.available()) Serial.read(); // clear remaining out of sync stuff

      if (progClient.available()) {
        end = millis() + 3000;
        byte count = 0;
        String log = String(millis() - start);
        log += F(" NET: ");
        while (progClient.available()) {
          count++;
          byte data = progClient.read();
          Serial.write(data);
          delayMicroseconds(TXdelay); // in case of large chunks, could overflow avr buffer, must be some xon/xoff ??
          log += String(data, HEX);
          log += ",";
        }
        log = log.substring(0, 40);
        log += " c=";
        log += count;
        P200_telnetLog(log);

        // wait max 100 ms for optiboot to reply
        unsigned long tr = micros();
        for (int x = 0; x < RXwait; x++) {
          if (Serial.available())
            break;
          delay(1);
        }

        duration = micros() - tr;
        if (!Serial.available()) {
          P200_telnetLog(F("No reply"));
        }

        if (Serial.available()) {
          char buffer[10];
          byte count = 0;
          String log = String(millis() - start);
          log += F(" SER:");
          while (Serial.available()) {
            byte data = Serial.read();
            buffer[count++] = data;
            log += String(data, HEX);
            log += ",";
            if (data == 0x10) {
              log = log.substring(0, 40);
              log += " c=";
              log += count;
              log += " d=";
              log += duration;
              if (count > 2) initDone = true; // we have a sync session
              if (!initDone) {
                if (!initReply) {
                  progClient.write(buffer, count);
                  initReply = true;
                }
              }
              else
              {
                progClient.write(buffer, count);
              }
              break;
            }
          }
          delay(0);
          progClient.flush();
          P200_telnetLog(log);
        }
      }
      delay(0);
    }
    delay(0);
  }
  if (progClient) progClient.stop();
  delay(0);
  Serial.begin(Settings.BaudRate);
  P200_telnetLog(F("End programming mode"));
}

/********************************************************************************************\
Telnet log
\*********************************************************************************************/
void P200_telnetLog(String s)
{
  if(Settings.SerialTelnet) return;
  
  if (ser2netClient.connected()) {
    ser2netClient.println(s);
    ser2netClient.flush();
  }
}


/********************************************************************************************\
Telnet log flashstring
\*********************************************************************************************/
void P200_telnetLog(const __FlashStringHelper* flashString)
{
  if(Settings.SerialTelnet) return;

  if (ser2netClient.connected()) {
    String s(flashString);
    ser2netClient.println(s);
    ser2netClient.flush();
  }
}

#endif

