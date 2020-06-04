//#######################################################################################################
//#################################### Plugin 110: HTTP #################################################
//#######################################################################################################

/*
 * Commands: sendToHTTP <host>,<port>,<path>
*/

#ifdef USES_P110
#define P110_BUILD            6
#define PLUGIN_110
#define PLUGIN_ID_110         110

#if defined(ESP8266)
  #include <ESP8266HTTPClient.h>
#else
  #include <HTTPClient.h>
#endif

boolean Plugin_110(byte function, String& cmd, String& params)
{
  boolean success = false;
  switch (function)
  {
    case PLUGIN_INFO:
      {
        printWebTools += F("<TR><TD>P110 - HTTP<TD>");
        printWebTools += P110_BUILD;        
        break;
      }
      
    case PLUGIN_WRITE:
      {
        if (cmd.equalsIgnoreCase(F("sendToHTTP")))
        {
          success = true;
          String host = parseString(params,1);
          int port = parseString(params,2).toInt();
          int pathpos = getParamStartPos(params,3);
          String path = params.substring(pathpos);

          WiFiClient client;
          if (client.connect(host.c_str(), port))
          {
            client.print(String("GET ") + path + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");

            unsigned long timer = millis() + 200;
            while (!client.available() && millis() < timer)
            delay(1);

            while (client.available()) {
              String line = client.readStringUntil('\n');
              if (line.substring(0, 15) == "HTTP/1.1 200 OK")
                logger->println(line);
              delay(1);
            }
            client.flush();
            client.stop();
          }
        }
        break;
      }
  }
  return success;
}
#endif

