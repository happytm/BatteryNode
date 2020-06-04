//#######################################################################################################
//#################################### Plugin 002: Analog ###############################################
//#######################################################################################################

/*
 * Commands:
 * analogRead <variable>,<pin>                   Read analog value into variable
 * analogDebug <pin>                             Show analog value every second on telnet session
*/

#ifdef USES_P002
#define P002_BUILD            6
#define PLUGIN_002
#define PLUGIN_ID_002         2

boolean Plugin_002(byte function, String& cmd, String& params)
{
  boolean success = false;
  static int8_t  PinDebug = -1;

  switch (function)
  {
    case PLUGIN_INFO:
      {
        printWebTools += F("<TR><TD>P002 - Analog<TD>");
        printWebTools += P002_BUILD;
        break;
      }
          
    case PLUGIN_WRITE:
      {
        if (cmd.equalsIgnoreCase(F("AnalogRead")))
        {
          success = true;
          String varName = parseString(params,1);

          #if defined(ESP8266)
            int value = analogRead(A0);
          #endif
          #if defined(ESP32)
            byte pin = parseString(params,2).toInt();
            int value = analogRead(pin);
          #endif
          
          setNvar(varName, value);
        }

        if (cmd.equalsIgnoreCase(F("AnalogDebug")))
        {
          success = true;
          PinDebug = parseString(params,1).toInt();
        }

        break;
      }
      
    case PLUGIN_ONCE_A_SECOND:
      {
        if (PinDebug != -1){
          #if defined(ESP8266)
            String debug = (String)analogRead(A0);
          #endif
          #if defined(ESP32)
            byte pin = parseString(params,2).toInt();
            String debug = (String)analogRead(pin);
          #endif 
          logger->println(debug);
        }
        break;
      }

  }
  return success;
}
#endif

