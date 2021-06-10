## Very simple DIY Low Power, low cost, Local standalone (no cloud service required) or web connected IOT Network.



# Features:

## Very low power consumption.

This is the main goal of this project. Proberequest protocol is used for data transfer to conserve power.

## No local linux server or cloud service required.

Whole network can be accesed using any android or ios MQTT front end dashboard client app like MQTT Dash thereby making it very simple to implement. 


## Build it, position towards maximum sun exposure and forget about it. 

No need to charge small 100 mAh LIR2450 coin cell battery manually for life of battery or solar cell if LIR2450 coin cell battery and 53mmx30mm small solar cell is used.

## Very low cost DIY sensors.

Average sensor node could be below $5.

## OTA update using github. 

Simply placing .bin file to github and publishing MQTT command from MQTT client app all sensor devices on the network can be updated with latest firmware. Simply publish "command/devicenumber/09/01" from any MQTT client. 

# Concept in detail:

This code create small standalone network (maximum of 100) of battery powered esp8266 devices connecting to one ESP32 or ESP8266  gateway device in star network topology.
Each device can be controlled simply by MQTT app like MQTT Dash over local network or over internet if used with DynDNS service like DuckDNS.
There is no need for other home automation software locally or on cloud.

My testing shows 18 bytes of data (4 different sensor's values with their respective sensor types + 5 different device status data + battery voltage using 1 byte + device indentifier using 1 byte)  is moved within 70 milliseconds on average thereby saving significant battery power.

If two way communication required between gateway and remote sensor then data is communiated both ways within 140 milliseconds (more efficient than ESPNow ?). With two way communication activated even control of actuator is possible but not justifiable for battery powered devices.

The device can auto update firmware via Github if specific .bin file is available.Use payload <deviceid>/09/01 under topic "command/".

### Installation:

- To test the code at minimum 1 ESP8266 as a slave and 1 ESP32 device (recommended) as a gateway or both ESP8266 devices (as gateway and slave) are required.
- One device (always on and mains powered) use gateway sketch and another device (sleeping most of the time and battery powered) use remote code.
- There can be as many as 100 battery powered devices which can send data to one gateway device. 
### Important: The apChannel and apSSID variables of all devices on the network needs to be same in order to have proper sensor network.
- In gateway sketch ssid and password of your home access point required for access to whole sensor network via MQTT Dash app to read sensor data and to issue commands to remote devices.
- Web interface shows current and historical graphs of sensor data from whole network, allows to issue commands (via websockets) to any slave device on network.

### Commands to control any remote devices on network by publishing MQTT messages via any MQTT client app (if #define DUPLEX is true in remote code)

### Command structure:  

commands are issued via MQTT payload with topic name "command/". At least first three commands are necessary.
      
#### Command1 = Device ID Number (required for all commands) -               
```c
- Device ID must be 2 digits end with 2,6,A or E to avoid conflict with other devices.
- See https://serverfault.com/questions/40712/what-range-of-mac-addresses-can-i-safely-use-for-my-virtual-machines.
- use any of following for devie ID ending with 6.
- 06,16,26,36,46,56,66,76,86,96,106,116,126,136,146,156,166,176,186,196,206,216,226,236,246.
- Device ID and last part of fixed IP are same.
```                                            
#### Command2 = Command type  (required for all commands)   -         
```c
- value 91 to 99 is reserved for following commands:

 - 91 = digitalWright or analogWrite.
           - Example command payload 36/91/<01 to 05 or 12 to 16>/<00 0r 01>/ for digitalWrite.
           - Example command payload 36/91/<01 to 05 or 12 to 160>/<2 to 256>/ for analogWrite(pwm).
 - 92 = digitalRead.
           - Example command payload 36/92/<01 to 05 or 12 to 16>/
 - 93 = analogRead,
 - 94 = Reserved,
 - 95 = Neopixel etc.
           - Example command payload 36/95/<01 to 05 or 12 to 16>/<00 to 256>/<00 to 256>/<00 to 256>/
 

  - 97 =     change wifiChannel.
  - 98 =     change sleepTime.
           - Example command payload 36/98/<00 to 255>/ (Sleep Time in minutes).
  - 99 =     Activate alternative code for OTA,alternative sketch to be run on the device ETC.
           - Example command payload 36/99/<00 or 255>/  (01 to activate Auto over the air firmware update (OTA)).
```
#### Command3 = Command  pinNumber  -    (required for all commands)        
```c
- pinNumber in case of command type 91 to 94 above. 
- Neopixel LED number in case of command type 95.
- Value in case of command type 96,97,98 & 99 commandtype.

```                                            
#### Command4 = Command value1      -            
```c
- 00 or 255 in case of command type 91 (digitalWrite & analogWrite)  
- or RED neopixel value in case of command type 95 

```
#### Command5 = Command value2      -            
```c
- 00 to 255 for GREEN neopixel in case of command type 95 

```        
#### Command6 = Command value3      -            
```c
- 00 to 255 for BLUE neopixel in case of command type 95 

```

Most suitable use cases around typical home - Weather Station, Door/Window sensor, Water/Oil tank level sensor, Presence Detection sensor, Soil moisture sensor for garden etc. 


## Tested with following MQTT front end GUI client software:

- MQTT Dash : https://play.google.com/store/apps/details?id=net.routix.mqttdash&hl=en_US (preffered).
            basic javascript automation possible with this App. 
            See https://github.com/ByTE1974/byte1974.github.io/tree/master/mqttdash/js

- MQTT Explorer : https://github.com/thomasnordquist/MQTT-Explorer

- Node-Red :
It is also possible to run Node-Red on android smartphones or tablets. 
https://nodered.org/docs/getting-started/android

## Useful tool to estimate power consumption and solar battery charging:
https://github.com/G6EJD/Processor-Solar-Power-Sleep-Calc

http://www.of-things.de/battery-life-calculator.php
      
## To do:
 - Web interface for ESP32 Gateway. 

## Special Thanks to creators of following libraries used for this project:
 - https://github.com/happytm/LittleDB
 - https://github.com/siara-cc/esp32_arduino_sqlite3_lib
 - https://github.com/lorol/LITTLEFS 
 - https://github.com/martin-ger/uMQTTBroker
 - https://github.com/hsaturn/TinyMqtt
 - https://github.com/gilmaimon/ArduinoWebsockets
 - https://github.com/leeoniya/uPlot   
 - https://github.com/me-no-dev/ESPAsyncWebServer
