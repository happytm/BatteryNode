/*
 * WiFiScanDetails
 * 
 * By: Mike Klepper
 * Date: 22 Feb 2017
 * 
 * This program checks for available networks and displays details about each in the Serial Monitor. 
 * It is based upon the WiFiScan example in the Arduino IDE for the ESP8266
 * 
 */
 
#include "ESP8266WiFi.h"

void setup() 
{
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  delay(100);

  Serial.println("Setup done");
}

void loop() 
{
  Serial.println("Scan start");

  int numNetworks = WiFi.scanNetworks(false, true);
  Serial.println("Scan done");
  
  if(numNetworks == 0)
  {
    Serial.println("No networks found");
  }
  else
  {
    WiFi.printDiag(Serial);
    Serial.print(numNetworks);
    Serial.println(" networks found");
    
    for(int i = 0; i < numNetworks; i++)
    {
      Serial.print("message received from Controller:  ");
      Serial.println(&WiFi.BSSIDstr(i)[0]);
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.println(WiFi.SSID(i));

      Serial.print("     BSSID = ");
      Serial.println(WiFi.BSSIDstr(i));

      Serial.print("     RSSI = ");
      Serial.println(WiFi.RSSI(i));

      Serial.print("     isHidden = ");
      Serial.println(WiFi.isHidden(i));

      Serial.print("     channel = ");
      Serial.println(WiFi.channel(i));

      if (WiFi.BSSIDstr(0)[6] == 01)  {
        Serial.println("message received from Controller");
      }
      
      delay(10);
      yield();
    }
  }
  
  Serial.println("");

  delay(4000);
  yield();
}
