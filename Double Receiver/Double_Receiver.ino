/*
 * IRremoteESP8266: SmartIRRepeater.ino - Record and playback IR codes.
 * Copyright 2019 David Conran (crankyoldgit)
 *
 * This program will try to capture incoming IR messages and tries to
 * intelligently replay them back.
 * It uses the advanced detection features of the library, and the custom
 * sending routines. Thus it will try to use the correct frequencies,
 * duty cycles, and repeats as it thinks is required.
 * Anything it doesn't understand, it will try to replay back as best it can,
 * but at 38kHz.
 * Note:
 *   That might NOT be the frequency of the incoming message, so some not
 *   recogised messages that are replayed may not work. The frequency & duty
 *   cycle of unknown incoming messages is lost at the point of the Hardware IR
 *   demodulator. The ESP can't see it.
 *
 *                               W A R N I N G
 *   This code is just for educational/example use only. No help will be given
 *   to you to make it do something else, or to make it work with some
 *   weird device or circuit, or to make it more usable or practical.
 *   If it works for you. Great. If not, Congratulations on changing/fixing it.
 *
 * An IR detector/demodulator must be connected to the input, kRecvPin.
 * An IR LED circuit must be connected to the output, kIrLedPin.
 *
 * Example circuit diagrams (both are needed):
 *  https://github.com/crankyoldgit/IRremoteESP8266/wiki#ir-receiving
 *  https://github.com/crankyoldgit/IRremoteESP8266/wiki#ir-sending
 *
 * Common mistakes & tips:
 *   * Don't just connect the IR LED directly to the pin, it won't
 *     have enough current to drive the IR LED effectively.
 *   * Make sure you have the IR LED polarity correct.
 *     See: https://learn.sparkfun.com/tutorials/polarity/diode-and-led-polarity
 *   * Some digital camera/phones can be used to see if the IR LED is flashed.
 *     Replace the IR LED with a normal LED if you don't have a digital camera
 *     when debugging.
 *   * Avoid using the following pins unless you really know what you are doing:
 *     * Pin 0/D3: Can interfere with the boot/program mode & support circuits.
 *     * Pin 1/TX/TXD0: Any serial transmissions from the ESP will interfere.
 *     * Pin 3/RX/RXD0: Any serial transmissions to the ESP will interfere.
 *   * ESP-01 modules are tricky. We suggest you use a module with more GPIOs
 *     for your first time. e.g. ESP-12 etc.
 *
 * Changes:
 *   Version 1.0: June, 2019
 *     - Initial version.
 */

#define  MQTTBROKER           true
 
#include <ESP8266WiFi.h>
#if MQTTBROKER
#include "uMQTTBroker.h"

/*
 * Your WiFi config here
 */
char ssid[] = "";     // your network SSID (name)
char pass[] = ""; // your network password
char gateway[] = "ESP"; // your gateway name
//bool WiFiAP = false;      // Do yo want the ESP as AP?
#endif
 
#include <Arduino.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>

// ==================== start of TUNEABLE PARAMETERS ====================


int device = 1;
int msg1 = 50;
int msg2 = 10;
int msg3 = 10;
int msg4 = 10;
int msg5 = 10; 
int VOLT_LIMIT = 3;
uint8_t mac[6] = {50, 10, 20, 30, 60, 01};
//uint8_t mac[6] = {msg1, msg2, msg3, msg4, msg5, device}; 

extern "C" void preinit() {
wifi_set_opmode(STATIONAP_MODE);
wifi_set_macaddr(SOFTAP_IF, mac);
}


/*/////////////////////////////////////////////////////////////////
 * Custom broker class with overwritten callback functions
 */


#if MQTTBROKER
 
class myMQTTBroker: public uMQTTBroker
{
public:
    virtual bool onConnect(IPAddress addr, uint16_t client_count) {
      Serial.println(addr.toString()+" connected");
      return true;
    }
    
    virtual bool onAuth(String username, String password) {
      Serial.println("Username/Password: "+username+"/"+password);
      return true;
    }
    
    virtual void onData(String topic, const char *data, uint32_t length) {
      char data_str[length+1];
      os_memcpy(data_str, data, length);
      data_str[length] = '\0';
      
      Serial.println("received topic '"+topic+"' with data '"+(String)data_str+"'");
    }
};

myMQTTBroker myBroker;

#endif
/////////////////////////////////////////////////////////////////////



int unit = 1;

int temperature;
int humidity;
int pressure;
int voltage;
int light; 

uint8_t probeData[] = {temperature,humidity,pressure,voltage,light,unit}; 

// The GPIO an IR detector/demodulator is connected to. Recommended: 14 (D5)
const uint16_t kRecvPin = 2;

// Example Samsung A/C state captured from IRrecvDumpV2.ino
//uint8_t myMac[kSamsungAcStateLength] = {temperature, humidity, pressure, Voltage, light, device}; //{0xb4, 0xe6, 0x52, 0x44, 0x86, 0xad, 0xb4, 0xe6, 0x52, 0x44, 0x86, 0xad };
//uint64_t myMac = 0x1234567890AB;//{temperature, humidity, pressure, battery, lux, device};  // 48bits 
uint64_t irData = 
    (uint64_t)(uint8_t (probeData[5])) | 
    (uint64_t)(uint8_t (probeData[4])) << 8 |          
    (uint64_t)(uint8_t (probeData[3])) << 16 |          
    (uint64_t)(uint8_t (probeData[2])) << 24 |          
    (uint64_t)(uint8_t (probeData[1])) << 32 |          
    (uint64_t)(uint8_t (probeData[0])) << 40;    

// GPIO to use to control the IR LED circuit. Recommended: 4 (D2).
const uint16_t kIrLedPin = 16;

// The Serial connection baud rate.
// NOTE: Make sure you set your Serial Monitor to the same speed.
const uint32_t kBaudRate = 115200;

// As this program is a special purpose capture/resender, let's use a larger
// than expected buffer so we can handle very large IR messages.
const uint16_t kCaptureBufferSize = 1024;  // 1024 == ~511 bits

// kTimeout is the Nr. of milli-Seconds of no-more-data before we consider a
// message ended.
const uint8_t kTimeout = 50;  // Milli-Seconds

// kFrequency is the modulation frequency all UNKNOWN messages will be sent at.
const uint16_t kFrequency = 38000;  // in Hz. e.g. 38kHz.

// ==================== end of TUNEABLE PARAMETERS ====================

// The IR transmitter.
IRsend irsend(kIrLedPin);  // Set the GPIO to be used to sending the message.

// The IR receiver.
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, false);

// Somewhere to store the captured message.
decode_results results;

WiFiEventHandler probeRequestPrintHandler;
volatile boolean buttonPressed;



void setup() {

  


  irrecv.enableIRIn();  // Start up the IR receiver.
  irsend.begin();       // Start up the IR sender.
  //probeToirSender();

  Serial.begin(kBaudRate, SERIAL_8N1);
  while (!Serial)  // Wait for the serial connection to be establised.
  delay(50);
  Serial.println();

  
  
  Serial.print("SmartIRRepeater is now running and waiting for IR input "
               "on Pin ");
  Serial.println(kRecvPin);
  Serial.print("and will retransmit it on Pin ");
  Serial.println(kIrLedPin);
  
  //WiFi.mode(WIFI_STA);   // declare mode as station to set fake MAC id
  WiFi.persistent(false);
  //wifi_set_macaddr(0, const_cast<uint8*>(mac));
//  wifi_set_macaddr(STATION_IF, mac);
  //WiFi.mode(WIFI_AP);    // Change to AP once fake MAC is set
 // WiFi.softAP(messageTostations, "<notused>", 6, 1, 0);  // Set channel 6 and hide SSID with 1.
  
  Serial.println();
  delay(500);

  Serial.println("Serial Gateway");
  Serial.println();
  Serial.print("This Device MAC ID is: ");
  Serial.println(WiFi.macAddress());
  Serial.print("This Device's message is: ");
  Serial.println(WiFi.SSID());
  WiFi.hostname("Controller");
  Serial.print("This Device's Host Name is: ");
  Serial.println(WiFi.hostname());
  Serial.print("This Device is connected to  AP MAC: ");
  Serial.println(WiFi.BSSIDstr());
  Serial.print("This Device is connected to: ");
  //Serial.println(WiFi.SSID());
  Serial.println();
 
    WiFi.persistent(false);
    probeRequestPrintHandler = WiFi.onSoftAPModeProbeRequestReceived(&onProbeRequest);

//////////////////////////////////////////////////////////////////
#if MQTTBROKER
 // Connect to a WiFi network
  startWiFiClient();

    // Start the broker
  Serial.println("Starting MQTT broker");
  myBroker.init();

/*
 * Subscribe to anything
 */
  myBroker.subscribe("#");
#endif
  
////////////////////////////////////////////////////////////////////

 
}  //END OF SETUP


void loop() {

//============================IR Repeater Loop starts=================================
  
  
   
  // Check if an IR message has been received.
  if (irrecv.decode(&results)) {  // We have captured something.
    // The capture has stopped at this point.
    decode_type_t protocol = results.decode_type;
    uint16_t size = results.bits;
    bool success = true;
    // Is it a protocol we don't understand?
    if (protocol == decode_type_t::UNKNOWN) {  // Yes.
      // Convert the results into an array suitable for sendRaw().
      // resultToRawArray() allocates the memory we need for the array.
      uint16_t *raw_array = resultToRawArray(&results);
      // Find out how many elements are in the array.
      size = getCorrectedRawLength(&results);
      // Send it out via the IR LED circuit.
      irsend.sendRaw(raw_array, size, kFrequency);
      // Deallocate the memory allocated by resultToRawArray().
      delete [] raw_array;
    } else if (hasACState(protocol)) {  // Does the message require a state[]?
      // It does, so send with bytes instead.
      success = irsend.send(protocol, results.state, size / 8);
    
    } 
    
    else {  // Anything else must be a simple message protocol. ie. <= 64 bits
      success = irsend.send(protocol, results.value, size);
    } 
      delay(200);    // Minimum delay of 200 works OK.
      probeToirSender(); 
    
     
    // Resume capturing IR messages. It was not restarted until after we sent
    // the message so we didn't capture our own message.
     irrecv.resume();
      
      Serial.print("Repeater retransmitted direct IR signal received:  ");
      serialPrintUint64(results.value, HEX);  // Should be your 48bit MAC Address in Decimal (Base 10).
      Serial.println();
    
}
  
  
   
  yield();  // Or delay(milliseconds); This ensures the ESP doesn't WDT reset.

   
//============================IR Repeater Loop ends=================================

   
  
}    //END OF MAIN LOOP


///////////////////////////////////////////////////////////////////////////////////////////////////

#if MQTTBROKER
void startWiFiClient()
{
  Serial.println("Connecting to "+(String)ssid);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  
  Serial.println("WiFi connected");
  Serial.println("IP address: " + WiFi.localIP().toString());
  WiFi.softAP(gateway, "<notused>", 7, 0, 0);
}
#endif

////////////////////////////////////////////////////////////////////////////

//========================IR sender function starts=========================================

void probeToirSender()   {
  //Send IR signal received from probe request.
    
   Serial.println("Starting IR data sender");
   Serial.println("Transmitting IR signal for sensor data received by probe request");
   
   irsend.sendMidea(irData); 
   Serial.print("Probe request data trasmitted over IR protocol:    ");
   serialPrintUint64(irData, HEX);  
   Serial.println(); 
   delay(100);
   
 }      

 void onProbeRequest(const WiFiEventSoftAPModeProbeRequestReceived& dataReceived) {
  //if (evt.mac[0] != 0x36 || evt.mac[1] != SONOFF_ID) return;
  
  if (dataReceived.mac[5] == 0x01) {
    
    Serial.print("Probe Request:- ");
    Serial.print(" Device ID:  ");
    Serial.print(dataReceived.mac[5],DEC);
    unit = dataReceived.mac[5];
    Serial.print(" Temperature:  ");
    Serial.print(dataReceived.mac[0],DEC);
    temperature = dataReceived.mac[0];
    Serial.print(" Humidity:  ");
    Serial.print(dataReceived.mac[1],DEC);
    humidity = dataReceived.mac[1];
    Serial.print(" Pressure:  ");
    Serial.print(dataReceived.mac[2],DEC);
    pressure = dataReceived.mac[2];
    Serial.print(" Battery:  ");
    Serial.print(dataReceived.mac[3],DEC);
    voltage = dataReceived.mac[3];
    Serial.print(" Light:  ");
    Serial.println(dataReceived.mac[4],DEC);
    light = dataReceived.mac[4];
    mqttPublish();
   
 } else {
    
    //Serial.println("Waiting for Data............");
    
  }
}

///////////////////////////////////////////////////////////////////////////
#if MQTTBROKER
void mqttPublish()    {
   
 myBroker.publish("SensorData/unit/", (String)unit);
 myBroker.publish("SensorData/temperature/", (String)temperature);
 myBroker.publish("SensorData/humidity/", (String)humidity);
 myBroker.publish("SensorData/pressure/", (String)pressure);
 myBroker.publish("SensorData/voltage/", (String)voltage);
 myBroker.publish("SensorData/light/", (String)light);
  
 // wait a second

//delay(1000);
}
#endif

//////////////////////////////////////////////////////////////////////////////
     

