//#######################################################################################################
//#################################### Plugin 001: Input Switch #########################################
//#######################################################################################################

/*
 * Commands:
 * gpio <pin>,<value>               Set gpio output
 * gpioRead <variable>,<pin>        Turn into input and read value
 * gpioDebug <pin>                  Show pinstate every second on serial
 * gpioState <variable>,<pin>       Read pin state without changing to input/ouput
 * gpioMonitor <pin>		            Monitor pin and create an event on change
*/

#ifdef USES_P001
#define P001_BUILD            6
#define PLUGIN_001
#define PLUGIN_ID_001         1

boolean Plugin_001(byte function, String& cmd, String& params)
{
  boolean success = false;

  static int8_t PinMonitor[PIN_D_MAX];
  static int8_t PinMonitorState[PIN_D_MAX];
  static int8_t PinDebugStart = -1;
  static int8_t PinDebugEnd = -1;

  switch (function)
  {
    case PLUGIN_INFO:
      {
        printWebTools += F("<TR><TD>P001 - Switch<TD>");
        printWebTools += P001_BUILD;
        break;
      }
      
    case PLUGIN_WRITE:
      {
        if (cmd.equalsIgnoreCase(F("gpio")))
        {
          success = true;
          byte pin = parseString(params,1).toInt();
          byte state = parseString(params,2).toInt();
          if (pin >= 0 && pin <= PIN_D_MAX)
          {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, state);
          }
        }
        
        if (cmd.equalsIgnoreCase(F("gpioRead")))
        {
          success = true;
          String varName = parseString(params,1);
          byte pin = parseString(params,2).toInt();
          pinMode(pin, INPUT);
          byte state = digitalRead(pin);
          setNvar(varName, state);
        }

        if (cmd.equalsIgnoreCase(F("gpioDebug")))
        {
          success = true;
          PinDebugStart = parseString(params,1).toInt();
          PinDebugEnd = parseString(params,2).toInt();
        }

        if (cmd.equalsIgnoreCase(F("gpioState")))
        {
          success = true;
          String varName = parseString(params,1);
          byte pin = parseString(params,2).toInt();
          byte state = digitalRead(pin);
          setNvar(varName, state);
        }
        
        if (cmd.equalsIgnoreCase(F("gpioMonitor")))
        {
          String param1 = parseString(params, 1);
          byte pin = parseString(params,1).toInt();
          pinMode(pin,INPUT_PULLUP);
          PinMonitor[pin] = 1;
          success = true;
        }
        break;
      }
      
    case PLUGIN_ONCE_A_SECOND:
      {
        if (PinDebugStart != -1){
          String debug = "";
          for(byte pin = PinDebugStart; pin <= PinDebugEnd; pin++){
            debug += (String)pin;
            debug += ":";
            debug += (String)digitalRead(pin);
            debug += " ";
          }
          logger->println(debug);
        }
        break;
      }
      
    case PLUGIN_TEN_PER_SECOND:
      {
        // port monitoring, on request by rule command
        for (byte x=0; x < PIN_D_MAX; x++)
           if (PinMonitor[x] != 0){
             byte state = digitalRead(x);
             if (PinMonitorState[x] != state){
               #if FEATURE_RULES
                 String eventString = F("GPIO#");
                 eventString += x;
                 eventString += F("=");
                 eventString += state;
                 rulesProcessing(FILE_RULES, eventString);
               #endif
               PinMonitorState[x] = state;
             }
           }
        break;
      }
     
  }
  return success;
}
#endif

