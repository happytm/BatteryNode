
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

 
 
#include <ESP8266WiFi.h>

#include <Arduino.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>


int device = 1;
int temperature = 10;
int humidity = 20;
int pressure = 30;
int Voltage = 40;
int light = 50; 
int VOLT_LIMIT = 3;
uint8_t fakeMac[] = {temperature,humidity,pressure,Voltage,light,device}; 

// ==================== start of TUNEABLE PARAMETERS ====================

// The GPIO an IR detector/demodulator is connected to. Recommended: 14 (D5)
const uint16_t kRecvPin = 2;

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
IRsend irsend(kIrLedPin);
// The IR receiver.
IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, false);

// Somewhere to store the captured message.
decode_results results;


WiFiEventHandler probeRequestPrintHandler;
volatile boolean buttonPressed;

void setup() {

  irrecv.enableIRIn();  // Start up the IR receiver.
  irsend.begin();       // Start up the IR sender.

  Serial.begin(kBaudRate, SERIAL_8N1);
  while (!Serial)  // Wait for the serial connection to be establised.
  delay(50);
  Serial.println();

  Serial.print("SmartIRRepeater is now running and waiting for IR input "
               "on Pin ");
  Serial.println(kRecvPin);
  Serial.print("and will retransmit it on Pin ");
  Serial.println(kIrLedPin);
  
  WiFi.mode(WIFI_STA);   // declare mode as station to set fake MAC id
  wifi_set_macaddr(STATION_IF, fakeMac);
  WiFi.mode(WIFI_AP);    // Change to AP once fake MAC is set
  WiFi.softAP("Gateway", "<notused>", 6, 0, 0);
  Serial.println();
  delay(500);
  Serial.println("Serial Gateway");
  Serial.println();
  Serial.print("This Device MAC ID is: ");
  Serial.println(WiFi.macAddress());
  Serial.print("This Device Name is: ");
  Serial.println(WiFi.SSID());
  Serial.print("This Device BSSID is: ");
  Serial.println(WiFi.BSSIDstr());
  Serial.println();
   
    probeRequestPrintHandler = WiFi.onSoftAPModeProbeRequestReceived(&onProbeRequest);
}

void onProbeRequest(const WiFiEventSoftAPModeProbeRequestReceived& dataReceived) {
  //if (evt.mac[0] != 0x36 || evt.mac[1] != SONOFF_ID) return;

  
  if (dataReceived.mac[5] == 0x01) {
    Serial.print("Probe Request:- ");
    Serial.print(" Device ID:  ");
    Serial.print(dataReceived.mac[5],DEC);
    Serial.print(" Temperature:  ");
    Serial.print(dataReceived.mac[0],DEC);
    Serial.print(" Humidity:  ");
    Serial.print(dataReceived.mac[1],DEC);
    Serial.print(" Pressure:  ");
    Serial.print(dataReceived.mac[2],DEC);
    Serial.print(" Battery:  ");
    Serial.print(dataReceived.mac[3],DEC);
    Serial.print(" Light:  ");
    Serial.println(dataReceived.mac[4],DEC);
    delay(10);
  } else {
    //Serial.println("Waiting for Data............");
    
  }
}

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

     
     
    // Resume capturing IR messages. It was not restarted until after we sent
    // the message so we didn't capture our own message.
    irrecv.resume();

    // Display a crude timestamp & notification.
    uint32_t now = millis();
    Serial.printf(
        "%06u.%03u: A %d-bit %s message was %ssuccessfully retransmitted.\n",
        now / 1000, now % 1000, size, typeToString(protocol).c_str(),
        success ? "" : "un");
  }
  yield();  // Or delay(milliseconds); This ensures the ESP doesn't WDT reset.
//============================IR Repeater Loop ends=================================
  
}
  
 
