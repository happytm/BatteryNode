//#######################################################################################################
//#################################### Plugin 111: SSL ##################################################
//#######################################################################################################

/*
 * Commands:
 * fingerpring <fingerprint>              20 byte fingerprint
 * sendToHTTPS <host>,<port>,<path>
*/

#ifdef USES_P111
#define P111_BUILD            6
#define PLUGIN_111
#define PLUGIN_ID_111         111

#include <WiFiClientSecureBearSSL.h>
#include <ESP8266HTTPClient.h>

uint8_t P111_fingerprint[20];

boolean Plugin_111(byte function, String& cmd, String& params)
{
  boolean success = false;
  switch (function)
  {
    case PLUGIN_INFO:
      {
        printWebTools += F("<TR><TD>P111 - HTTPS<TD>");
        printWebTools += P111_BUILD;        
        break;
      }
     
    case PLUGIN_WRITE:
      {
        if (cmd.equalsIgnoreCase(F("fingerprint")))
        {
          success = true;
          parseBytes(params.c_str(), ',', P111_fingerprint, 20, 16);
        } 
        if (cmd.equalsIgnoreCase(F("sendToHTTPS")))
        {
          success = true;
          String host = F("https://");
          host += parseString(params,1);
          int port = parseString(params,2).toInt();
          int pathpos = getParamStartPos(params,3);
          String path = params.substring(pathpos);

          std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
          client->setFingerprint(P111_fingerprint);
          HTTPClient https;
logger->println("[HTTPS] begin...\n");
          if (https.begin(*client, host.c_str())) {  // HTTPS
            // start connection and send HTTP header
            int httpCode = https.GET();

            // httpCode will be negative on error
            if (httpCode > 0) {
              // HTTP header has been send and Server response header has been handled
logger->printf("[HTTPS] GET... code: %d\n", httpCode);
              // file found at server
              if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                String payload = https.getString();
logger->println(payload);
              }
            }
            else
              {
logger->printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
              }
            https.end();
          } else {
logger->printf("[HTTPS] Unable to connect\n");
          }
        }

        break;
      }
  }
  return success;
}
#endif

