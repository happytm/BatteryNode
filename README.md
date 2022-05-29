## Simplest DIY Low Power, low cost, Local standalone or web connected IOT Network (no third party software or cloud service or programming knowledge required) 

# Features:

## Very low power consumption.

This is the main goal of this project. Proberequest protocol is used for data transfer to conserve power.

## No local linux server or cloud service required.

Whole network can be monitored and controlled using any android or ios MQTT front end dashboard client app like MQTT Dash thereby making it very simple to implement.There is also very simple but powerful dashbord to monitor sensor data from whole network using uPlot graphing library and built in webserver. Sensor data of whole network is stored on gateway device using LittleFS file system. The web interface is simple one page but powerful enough to monitor and control whole network of devices. Web interface could be customised any way you want live by changing html, css or javascript code using built in SPIFFSEditor library without recompiling or reuploading files.Changing data format in LittleFS also also possible live from web interface using SPIFFSEditor.


## Build it, position towards maximum sun exposure and forget about it. 

No need to charge small 100 mAh LIR2450 coin cell battery manually for life of battery or solar cell if LIR2450 coin cell battery and 53mmx30mm small solar cell is used.

## Very low cost DIY sensors.

Average sensor node could be below $5 each.

## OTA update of all remote devices using Gateway to store single standard .bin file for all remote device. 

Simply storing single standard .bin file for all remote devices to Gateway LittleFS file system and publishing MQTT command from MQTT client app or by issuing command from web interface each sensor device on the network can be updated with latest firmware. Again there is no need to physically access hard to reach remote devices.

## Current time sent to all remote devices from Gateway for further time based automation in remote devices. 

# Concept in detail:

This code create small standalone network (maximum of 100) of battery powered WiFi devices connecting to one ESP32 gateway device in star network topology.
Each device can be controlled simply by MQTT app like MQTT Dash or built in web interface over local network or over internet if used with DynDNS service like DuckDNS.
There is no need for other home automation software locally or on cloud.

My testing shows data communication is achieved within 80 milliseconds total uptime for remote device out of which only 45 milliseconds of time used for more power hungry WiFi receive & transmit on average thereby saving significant battery power. I think this is better than ESPNow protocol.

### Installation:

- To test the code at minimum 1 remote ESP32 and 1 ESP32 Gateway device are required.
- One device (always on and mains powered) use gateway sketch and another device (sleeping most of the time and battery powered) use remote code.
- There can be as many as 100 battery powered devices which can send data to one gateway device. 
- Web interface is started at IP 192.168.4.1 and shows graphs of sensor data from whole network, It also allows to issue commands to any remote device on network.

### Command structure:  

commands are issued via MQTT payload with topic name "command/". At least first three commands are necessary.
      
#### Command1 = Device ID Number (required for all commands) -               
```c
- Device ID must be 2 digits and ending with 2,6,A or E to avoid conflict with other devices.
- See https://serverfault.com/questions/40712/what-range-of-mac-addresses-can-i-safely-use-for-my-virtual-machines.
- For example use any of following for devie ID ending with 6.
- 06,16,26,36,46,56,66,76,86,96,106,116,126,136,146,156,166,176,186,196,206,216,226,236,246.
```                                            
#### Command2 = Command type  (required for all commands)   -         
```c
- value 101 to 121 is reserved for following commands:

 - 101 =     Digital Write.
           - Example command payload 36/101/<01 to 05 or 12 to 39>/<00 0r 01>/ for digitalWrite.
           
 - 102 =     Analog Write.
           - Example command payload 36/102/<01 to 05 or 12 to 39>/<1 to 256>/ for analogWrite(pwm).
 
 - 103 =     Digital Read.
           - Example command payload 36/103/<01 to 05 or 12 to 39>/
 
 - 104 =     Analog Read.
           - Example command payload 36/104/<01 to 05 or 12 to 39>/
 
 - 105 =     Neopixel.
           - Example command payload 36/105/<00 to 256>/<00 to 256>/<00 to 256>/<00 to 256>/
           
 - 106 =     Set target values - It is posiible to send and store 4 target values to remote device for further automation locally in remote device.
           - Example command payload 36/106/<00 to 256>/<00 to 256>/<00 to 256>/<00 to 256>/
 
 - 107 =     Set WiFi Channel.
           - Example command payload 36/107/<00 to 14>/
           
 - 108 =     Set device mode - device mode 0 = normal mode and device mode 1 = OTA update.
           - Example command payload 36/108/<00 to 105>/ - Mainly used for OTA update but can be used to run any alternative Code block.
 
 - 109 =     Set sleep time in minutes.
           - Example command payload 36/109/<00 or 255>/
 - 110 =     Set device ID.
           - Example command payload 36/110/<00 or 255 if number ends with 2, 6, A, or E>/
           
 - 111 to 120 reserved for future exansion.
 
 - 121 =     Set sensor types for each device.
           - Example command payload 36/121/<00 to 256>/<00 to 256>/<00 to 256>/<00 to 256>/
```
#### Command3 = Command  pinNumber or value1  -    (required for all commands)        
```c
- pinNumber in case of command type 101 to 105 above. 
- Value1 in case of command type 106,107,108,109 & 110 commandtype.

```                                            
#### Command4 = value2.           

#### Command5 = value3.           

#### Command6 = value3.          

Most suitable use cases around typical home - Light control, fan/AC control, Weather Station, Door/Window sensor, Water/Oil tank level sensor, Presence Detection/motion sensor, Soil moisture sensor for garden/greenhouse, solar tracker etc. 


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
      
## Tool used to convert HTML/CSS/JAVASCRIPT TO HEX: 
      
https://gchq.github.io/CyberChef/#recipe=Gzip('Dynamic%20Huffman%20Coding','index.html.gz','',false)To_Hex('0x',0)Split('0x',',0x')&input=PGh0bWw%2BC


## This project was possible thanks to creators of following libraries used for this project:
 
 - https://github.com/lorol/LITTLEFS 
 - https://github.com/hsaturn/TinyMqtt
 - https://github.com/leeoniya/uPlot   
 - https://github.com/me-no-dev/ESPAsyncWebServer
