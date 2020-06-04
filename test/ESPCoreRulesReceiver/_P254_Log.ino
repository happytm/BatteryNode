//#######################################################################################################
//#################################### Plugin 254: Log ##################################################
//#######################################################################################################

/*
 * Commands:
 * 
 * loginit <max entries>                         Init the log system with a max nr of entries
 * log <msg>                                     Add entry to local log
*/

#ifdef USES_P254
#define P254_BUILD            1
#define PLUGIN_254
#define PLUGIN_ID_254         254

struct P254_LogStruct
{
  unsigned long timeStamp;
  unsigned long delta;
  String *Message;
};

P254_LogStruct *P254_Logging;
boolean P254_Init = false;
int P254_logcount = -1;
int P254_logMax = 10;
unsigned long P254_lastLogTime = 0;

boolean Plugin_254(byte function, String& cmd, String& params)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_INFO:
      {
        printWebTools += F("<TR><TD>P254 - Log<TD>");
        printWebTools += P254_BUILD;
        break;
      }
          
    case PLUGIN_WRITE:
      {

        if (cmd.equalsIgnoreCase(F("LogInit")))
        {
          success = true;
          if(!P254_Init){
            byte logmax = parseString(params,1).toInt();
            if(logmax)
              P254_logMax = logmax;
            P254_Logging = (P254_LogStruct*)malloc(sizeof(P254_LogStruct) * P254_logMax);
            if (P254_Logging != NULL) {
              for(byte x=0; x < P254_logMax; x++){
                 P254_Logging[x].timeStamp = 0;
                 P254_Logging[x].delta = 0;
                 P254_Logging[x].Message = new String();
              }
              P254_Init = true;
              WebServer.on("/log", P254_handle_log);
            }
          }
        }

        if (cmd.equalsIgnoreCase(F("Log")))
        {
          success = true;
          if(P254_Init){
            P254_logcount++;
             if (P254_logcount >= P254_logMax)
              P254_logcount = 0;
            #if FEATURE_TIME  
              P254_Logging[P254_logcount].timeStamp = sysTime;
            #else
              P254_Logging[P254_logcount].timeStamp = millis();
            #endif
            P254_Logging[P254_logcount].delta = millis()-P254_lastLogTime;
            P254_lastLogTime = millis();
            *P254_Logging[P254_logcount].Message = params;
          }
        }

        if (cmd.equalsIgnoreCase(F("LogList")))
        {
          success = true;
          if(P254_Init){
            for(byte x=0; x < P254_logMax; x++){
              logger->println(*P254_Logging[x].Message);
            }
          }
        }
        
        break;
      }
  }
  return success;
}

//********************************************************************************
// Web Interface log page
//********************************************************************************
void P254_handle_log() {

  if(!P254_Init){
    WebServer.send(200, "text/html", "No init");
    return;
  }
    
  #if FEATURE_TIME
    struct  timeStruct logTime;
  #endif
  
  String reply = "";
  addHeader(true, reply);

  reply += printWebString;

  reply += F("<script language='JavaScript'>function RefreshMe(){window.location = window.location}setTimeout('RefreshMe()', 3000);</script>");
  reply += F("<table><TR><TD>");

  if (P254_logcount != -1)
  {
    byte counter = P254_logcount;
    do
    {
      if (P254_Logging[counter].timeStamp > 0)
      {
        #if FEATURE_TIME
          breakTime(P254_Logging[counter].timeStamp,logTime);
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
          reply += P254_Logging[counter].timeStamp;
        #endif
        reply += " +";
        reply += P254_Logging[counter].delta;
        reply += " : ";
        reply += *P254_Logging[counter].Message;
        reply += F("<BR>");
      }
      counter--;
      if (counter == 255)
        counter = P254_logMax-1;
    }  while (counter != P254_logcount);
  }
  reply += F("</table>");
  WebServer.send(200, "text/html", reply);
}

#endif

