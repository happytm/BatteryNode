/*
  This sketch uses IRremoteESP8266 library from https://github.com/crankyoldgit/IRremoteESP8266.
  In order to send MIDEA protocol IR message there is change needed in library
  as documented here : https://github.com/crankyoldgit/IRremoteESP8266/issues/887#issuecomment-531314986

  Change this line: https://github.com/crankyoldgit/IRremoteESP8266/blob/master/src/IRrecv.h#L402

  to:

                   bool strict = false);
  e.g.

  #if DECODE_MIDEA
  bool decodeMidea(decode_results *results, uint16_t nbits = kMideaBits,
                   bool strict = true);
  #endif

  On receiver side example can be used from here: https://github.com/happytm/BatteryNode/blob/master/Double%20Receiver/Double_Receiver.ino
*/
#define IRSENDER              false
#define PROBEREQUESTER        true
#define SLEEP_SECS 15 * 60 // 15 minutes
int device = 1;

#if PROBEREQUESTER
#include <ESP8266WiFi.h>
#endif

//============Do not need user configuration from here on============================


#if IRSENDER
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
//#include <IRrecv.h>
//#include <IRutils.h>
#endif

ADC_MODE(ADC_VCC); //vcc read-mode
uint8 ssid[32] = "ESP_Controller";
int apChannel = 7;
int temperature = 10;
int humidity = 20;
int pressure = 90;
int voltage = 80;
int light = 70;

uint8_t irMac[] = {temperature, humidity, pressure, voltage, light, device};

int VOLT_LIMIT = 3;
unsigned long lastMillis;
unsigned long passedMillis;
int ledPin = 12;


//====================IR defines starts============================

#if IRSENDER
const uint16_t kIrLed = 16;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.


//uint64_t myMac = 0x1234567890AB;//{temperature, humidity, pressure, battery, lux, device};  // 48bits
uint64_t myMac =
  (uint64_t)(uint8_t (irMac[5])) |
  (uint64_t)(uint8_t (irMac[4])) << 8 |
  (uint64_t)(uint8_t (irMac[3])) << 16 |
  (uint64_t)(uint8_t (irMac[2])) << 24 |
  (uint64_t)(uint8_t (irMac[1])) << 32 |
  (uint64_t)(uint8_t (irMac[0])) << 40;
#endif


//====================IR defines ends==============================


void setup() {

  sensorValues();
  Serial.begin(115200);
  Serial.println();
  Serial.print("Milliseconds passed before setup: ");
  Serial.println(millis());
  lastMillis = millis();
  Serial.println();
  delay(1);
  

#if PROBEREQUESTER
//wifi_set_macaddr(STATION_IF, irMac);
  WiFi.disconnect();
  WiFi.hostname("Livingroom");
  Serial.println();
#endif

  float voltage = ESP.getVcc() / (float)1023; // * (float)1.07;
  Serial.print("Voltage: "); Serial.print(voltage); Serial.print("  Voltage Expected: "); Serial.println(VOLT_LIMIT);
  if (voltage < VOLT_LIMIT)      // if voltage of battery gets to low, the LED wil blink fast.
  {
    Serial.println("Warning :- Battery Voltage low please change batteries" );
    Serial.println();
  }

  Serial.println("Setup finished");

  //#if PROBEREQUESTER
  probeRequest();
  //#endif

#if IRSENDER
  irsend.begin();
  irSender();
#endif
  
  
  passedMillis = millis() - lastMillis;
  Serial.print("Time spent on setup: ");
  Serial.println(passedMillis);
  Serial.println();
  lastMillis = millis();
  Serial.println();

}   // Setup ends here

//========================Main Loop================================

void loop() {



  //===========================IR loop===============================
#if IRSENDER
  //irSender();
 #endif
  //=====================Probe request Loop=========================

  // put your code here to run in loop or move to setup to run oncce.

#if PROBEREQUESTER
  probeRequest();
#endif

  Serial.print("Total time I spent before going to sleep: ");
  Serial.println(millis());
  Serial.print("I will wakeup in: ");
  Serial.print(SLEEP_SECS / 60);
  Serial.println(" Minutes");
  delay(5000);
  ESP.restart();
  //ESP.deepSleep(0);

}

//=========================Main Loop ends==========================


void gotoSleep() {                            //need connection between GPIO16 and reset pin on ESP8266
  // add some randomness to avoid collisions with multiple devices
  int sleepSecs = SLEEP_SECS;// + ((uint8_t)RANDOM_REG32/2);
  Serial.printf("Up for %i ms, going to sleep for %i secs...\n", millis(), sleepSecs);
  ESP.deepSleep(sleepSecs * 1000000, RF_NO_CAL);
  ////ESP.deepSleep( SLEEPTIME, WAKE_RF_DISABLED );
}

#if PROBEREQUESTER
//=========================Probe request function starts===========

void sensorValues()     {

  temperature = random(90);
  irMac[0] = temperature;
  wifi_set_macaddr(STATION_IF, irMac);
  
}

void probeRequest()  {
  Serial.println("Starting Probe sender");
  Serial.println("Sending sensor data over Probe request protocol to Master Node");
  Serial.println();
 
  //int8_t scanNetworks(bool async = true, bool show_hidden = false, uint8 channel = 0, uint8* ssid = NULL);
  int n = WiFi.scanNetworks(true, false, apChannel, (uint8*) ssid);

#if !IRSENDER
  delay(25); // Minimum delay of 25 required to scan networks in async mode if not using IRSENDER
#endif

#if IRSENDER
  delay(10);     // Minimum delay of 10 required to scan networks in async mode if using IRSENDER
#endif

  if (WiFi.BSSIDstr(0)[16] == '1')  {     //use device number here
    String ssid;
    Serial.println();
    Serial.print("This Device MAC ID is: ");
    Serial.println(WiFi.macAddress());
    Serial.print("This Device Name is: ");
    Serial.println(WiFi.hostname());
    Serial.print("Message received from Controller is: ");
    Serial.println(&WiFi.BSSIDstr(0)[0]);
    /* Serial.print(WiFi.BSSIDstr(0)[0]);
      Serial.print(WiFi.BSSIDstr(0)[1]);
      Serial.println(WiFi.BSSIDstr(0)[2]);
      Serial.print(WiFi.BSSIDstr(0)[3]);
      Serial.print(WiFi.BSSIDstr(0)[4]);
      Serial.println(WiFi.BSSIDstr(0)[5]);
      Serial.print(WiFi.BSSIDstr(0)[6]);
      Serial.print(WiFi.BSSIDstr(0)[7]);
      Serial.println(WiFi.BSSIDstr(0)[8]);
      Serial.print(WiFi.BSSIDstr(0)[9]);
      Serial.print(WiFi.BSSIDstr(0)[10]);
      Serial.println(WiFi.BSSIDstr(0)[11]);
      Serial.print(WiFi.BSSIDstr(0)[12]);
      Serial.print(WiFi.BSSIDstr(0)[13]);
      Serial.println(WiFi.BSSIDstr(0)[14]);
      Serial.print(WiFi.BSSIDstr(0)[15]);
      Serial.print(WiFi.BSSIDstr(0)[16]);
    */
    Serial.println();

    Serial.print("Gateway Name is: ");
    Serial.println(WiFi.SSID(0));
    Serial.print("Gateway Channel is: ");
    Serial.println(WiFi.channel(0));

    Serial.print("Message received from Controller: ");
    if (WiFi.BSSIDstr(0)[0] == '3')  {
      Serial.print("GPIO,");
      Serial.print(ledPin);
      Serial.print(",");
      Serial.print(WiFi.BSSIDstr(0)[15]);
    }

    if (WiFi.BSSIDstr(0)[1] == '1')  {
      Serial.print("I will wake up next time with WiFi & OTA and deepsleep disabled");
      // Define some features here to activate wifi & set 5 seconds deep sleep time & disable deepsleep when unit wakes up next time.
    }
  }

  Serial.println();
  WiFi.scanDelete();
  passedMillis = millis() - lastMillis;
  Serial.print("Time spent on Probe Request: ");
  Serial.println(passedMillis);
  lastMillis = millis();
  Serial.println();

  delay(10);    // minimum delay of 10 required here.

}

#endif

#if IRSENDER
//========================IR function starts=========================================
void irSender()   {

  Serial.println("Starting IR data sender");
  Serial.println("Sending sensor data over IR protocol to Master Node");
  Serial.println();

  irsend.sendMidea(myMac);
  Serial.print("Message sent : ");
  //   serialPrintUint64(myMac, HEX); //
  Serial.println();


  passedMillis = millis() - lastMillis;
  Serial.print("Time spent on IR Sender: ");
  Serial.println(passedMillis);
  lastMillis = millis();
  Serial.println();
  //delay(10);
}
#endif
