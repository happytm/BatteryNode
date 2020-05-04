// This is a Core edition of ESP Easy, initially based on R120 and some Mega code
// Copyright notice on tab __Copyright!
// Command reference on tab __Doc and the plugins each have their own command reference

// This tab does not contain code other than one call to setup2
// Maintain your own version when required

#define FACTORY_APKEY                    "configesp"
#define BUILD_NOTES                      ""

// Select features to include into the Core:
#define FEATURE_RULES                    true
#define FEATURE_PLUGINS                  true
#define FEATURE_TIME                     true
#define FEATURE_I2C                      true

// Some extra features, disabled by default
#define FEATURE_ADC_VCC                  true
#define SERIALDEBUG                      true


// Select a custom plugin set
//#define PLUGIN_SET_BASIC
#define PLUGIN_SET_ALL

#ifdef PLUGIN_SET_BASIC
  #define USES_P001 // Switch
  #define USES_P002 // ADC
#endif

#ifdef PLUGIN_SET_ALL
  #define USES_P001 // Switch
  #define USES_P002 // ADC
  #define USES_P004 // Dallas
  #define USES_P011 // PME
  #define USES_P012 // LCD
  #define USES_P014 // SI7021
  #define USES_P100 // MSGBUS
  //#define USES_P101 // MQTT
  #if defined(ESP8266)
   // #define USES_P102 // ESPNOW
  #endif
  #define USES_P110 // HTTP
  //#define USES_P111 // HTTPS
  #define USES_P200 // Nano Serial
  #define USES_P201 // Tuya LSC
  #if defined(ESP32)
    #define USES_P203 // M5
  #endif
  #define USES_P254 // Local Log
  #define USES_P255 // Debugging stuff
#endif


// End of config section, do not remove this:
// Could not move setup() to base tab, preprocessor gets confused (?)
// So we just call setup2() from here
#include "Globals.h"
void setup() {
  setup2();
}
