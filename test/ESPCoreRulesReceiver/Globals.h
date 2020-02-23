#define BUILD                               7

#define CMD_REBOOT                         89
#define PLUGIN_MAX                         32
#define PLUGIN_FASTCALL_MAX                 8
#define RULES_MAX_SIZE                   2048
#define RULES_BUFFER_SIZE                  64
#define RULES_MAX_NESTING_LEVEL             3
#define CONFIRM_QUEUE_MAX                   8
#define INPUT_BUFFER_SIZE                 128

#define PLUGIN_INIT                         2
#define PLUGIN_ONCE_A_SECOND                4
#define PLUGIN_TEN_PER_SECOND               5
#define PLUGIN_WRITE                       13
#define PLUGIN_SERIAL_IN                   16
#define PLUGIN_UNCONDITIONAL_POLL          25
#define PLUGIN_ONCE_A_MINUTE              254
#define PLUGIN_INFO                       255

#if defined(ESP32)
  #define ARDUINO_OTA_PORT  3232
#else
  #define ARDUINO_OTA_PORT  8266
#endif

#if defined(ESP32)
#define FEATURE_ARDUINO_OTA
#endif

#if FEATURE_I2C
  #include <Wire.h>
#endif

#if defined(ESP32)
#define FILE_BOOT         "/boot.txt"
#define FILE_CONFIG       "/config.dat"
#define FILE_SECURITY     "/security.dat"
#define FILE_RULES        "/rules1.txt"
#include <WiFi.h>
#include <WebServer.h>
#include "SPIFFS.h"
WebServer WebServer(80);
#ifdef FEATURE_ARDUINO_OTA
  #include <ArduinoOTA.h>
  #include <ESPmDNS.h>
  bool ArduinoOTAtriggered = false;
#endif
#define PIN_D_MAX        40
#endif

#if defined(ESP8266)
  #define FILE_BOOT         "boot.txt"
  #define FILE_CONFIG       "config.dat"
  #define FILE_SECURITY     "security.dat"
  #define FILE_NOTIFICATION "notification.dat"
  #define FILE_RULES        "rules1.txt"
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <ESP8266HTTPUpdateServer.h>
  ESP8266HTTPUpdateServer httpUpdater(false);
  ESP8266WebServer WebServer(80);
  #include <lwip/netif.h>
  #include <lwip/etharp.h>
  extern "C" {
    #include "user_interface.h"
    #if FEATURE_ESPNOW
      #include <espnow.h>
    #endif
  }
  #define PIN_D_MAX        16
  #if FEATURE_ADC_VCC
    ADC_MODE(ADC_VCC);
  #endif

  // Info on SPIFFS area
  #define FLASH_EEPROM_SIZE 4096
  extern "C" {
    #include "spi_flash.h"
  }
  extern "C" uint32_t _SPIFFS_start;
  extern "C" uint32_t _SPIFFS_end;
  extern "C" uint32_t _SPIFFS_page;
  extern "C" uint32_t _SPIFFS_block;
#endif

WiFiServer *ser2netServer;
WiFiClient ser2netClient;
byte connectionState = 0;

#include <WiFiUdp.h>
#include <FS.h>
using namespace fs;

struct SecurityStruct
{
  char          WifiSSID[32];
  char          WifiKey[64];
  char          WifiSSID2[32];
  char          WifiKey2[64];
  char          WifiAPKey[64];
} SecuritySettings;

struct SettingsStruct
{
  boolean       Wifi = true;
  boolean       AutoConnect = true;
  boolean       AutoAPOnFailure = true;
  byte          BSSID[6];
  byte          WifiChannel=0;
  byte          IP[4];
  byte          Gateway[4];
  byte          Subnet[4];
  byte          DNS[4];
  unsigned int  Port;
  char          Name[26];
  char          Group[26];
  char          NTPHost[64];
  int8_t        Pin_i2c_sda;
  int8_t        Pin_i2c_scl;
  unsigned long BaudRate;
  byte          deepSleep;
  boolean       DST;
  boolean       RulesClock;
  boolean       RulesSerial;
  boolean       UseSerial = true;
  boolean       UseTime = true;
  boolean       UseNTP;
  int16_t       TimeZone;
  byte          SerialTelnet=0;
  boolean       UseGratuitousARP = false;
  boolean       LogEvents = false;
  boolean       ForceAPMode = false;
  byte          NodeListMax = 32;
  byte          nUserVarMax = 8;
  byte          sUserVarMax = 8;
  byte          TimerMax = 8;
} Settings;

struct NodeStruct
{
  byte IP[4];
  byte age;
  String *nodeName;
  String *group;
};
struct NodeStruct *Nodes;

struct nvarStruct
{
  String *Name;
  float Value;
  byte Decimals;
};
struct nvarStruct *nUserVar;

struct svarStruct
{
  String *Name;
  String *Value;
};
struct svarStruct *sUserVar;

#if FEATURE_RULES
struct timerStruct
{
  String *Name;
  unsigned long Value;
};
struct timerStruct *RulesTimer;
#endif

unsigned int NC_Count = 0;
unsigned int C_Count = 0;
boolean AP_Mode = false;
byte cmd_within_mainloop = 0;
boolean bootConfig = false;
unsigned long timer60 = 0;
unsigned long timer1 = 0;
unsigned long timer10ps = 0;
unsigned long uptime = 0;

// Serial support global vars
byte SerialInByte;
int SerialInByteCounter = 0;
char InputBuffer_Serial[INPUT_BUFFER_SIZE + 2];

// Telnet support global vars
int TelnetByteCounter = 0;
char TelnetBuffer[INPUT_BUFFER_SIZE + 2];

String printWebString = "";
String printWebTools = "";

void setNvar(String varName, float value, int decimals = -1);
String parseString(String& string, byte indexFind, char separator = ',');

unsigned long loopCounter = 0;
unsigned long loopCounterLast=0;

#if FEATURE_PLUGINS
  boolean (*Plugin_ptr[PLUGIN_MAX])(byte, String&, String&);
  byte Plugin_id[PLUGIN_MAX];
  byte Plugin_Enabled[PLUGIN_MAX];
  String dummyString = "";
#endif

#if SERIALDEBUG
  byte debugLevel = 0;
#endif

Print* logger;
void (*coreSerialCall_ptr)(void);
void (*corePluginCall_ptr[PLUGIN_FASTCALL_MAX])(void);

byte *sortedIndex;

boolean mallocOK = false;
