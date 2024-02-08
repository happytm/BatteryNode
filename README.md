# BatteryNode

Simplest end to end complete DIY Low Power, low cost, Local standalone (non IOT) or web connected IOT Network (no third party software or cloud service or programming knowledge required) framework template.

## Features

### Ultra low power consumption 

This is the main goal of this project. Proberequest protocol is used for data transfer to conserve power.

### KISS (Keep It Simple Stupid) design principle

No over engeeniering. Voice commands to control home automation system using microphones and speakers are expensive, stupid and funny except used by blind people.
Use analog sensors as much as possible for simplicity in hardware and code and low cost.

### True DIY for low cost system

In most use cases no need to buy expensive commercial products except ESP32 barebone modules and NTC thermistors for temperature sensor, photoresistors or phototransistors for light sensor,
some low cost sensor modules, LDO and/or battery charging IC, coin cell battery and/or small solar panels based on use cases. 

### Easily customisable with basic coding knowledge

There is no need to use overwhelming and heavy software like home assistance. Easily customise look and feel of front end in javascript or remote sensors code in C++  as you wish. 

### No local server or cloud service (no IOT) required except built in webserver on ESP32 Gateway device

Whole network can be monitored and controlled using any android or ios MQTT front end dashboard client app like MQTT Dash thereby making it very simple to implement.There is also very simple but powerful dashbord to monitor sensor data from whole network using awsome uPlot graphing library and built in webserver. Sensor data of whole network is stored on ESP32 gateway device using SPIFFS file system. The web interface is simple one page but powerful enough to monitor and control whole network of devices. Web interface could be customised any way you want live by changing html, css or javascript code using built in SPIFFSEditor library without recompiling or reuploading files. Changing data format in SPIFFS also possible live from web interface using SPIFFSEditor.

### Build it, position towards maximum sun exposure and forget about it

No need to charge small 100 mAh LIR2450 coin cell battery or any chargable manually for life of battery or solar cell  53mmx30mm small or even smaller solar cell is used.

### Very low cost DIY sensors (see ideas below)

Average sensor node could be made for $2 to $5 each including battery and/or small solar panel for charging. Use of IR transmitters recommended as much as possible for in room automation. 
Average home can be automated with more than 10 remote sensors for below $100 total cost.

### Intruder alarm, presence detection and locator service for family members.

Built in whole house intruder motion detector, presence and location detector for family members all implemented in software saving cost for any motion or presence detector hardware sensors.

### OTA update of all remote devices using ESP32 Gateway to store single standard .bin file for all remote devices

Simply storing single standard .bin file for all remote devices to ESP32 Gateway SPIFFS file system and publishing MQTT command from MQTT client app or by issuing command from web interface each sensor device on the network can be updated with latest firmware. Again there is no need to physically access hard to reach remote devices.

### Current time sent to all remote devices from ESP32 Gateway for further time based automation in remote devices

### Most suitable use cases around typical home either fully automated or all manually controlled by using a smartphone

- TV or any appliance with IR receiver
- DIY light control (AC or DC) using WS2812B, photoresistor, triacs, diacs.
- DIY Fan/AC control either using WS2812B, photoresistor, triacs & diacs or built in IR receiver.
- DIY energy monitor for whole house using either current transformer or ACS712.
- DIY analog weather station using NTC/temperature, photoresistor/phototransistor and hall effect/IR proximity sensors for rain guage and wind sensors.
- DIY Door/Window sensors using battery as switch shutting down whole micro controller for 0 power consunption.
- DIY analog Water/Oil tank level sensors
- DIY analog leak detection sensor
- DIY analog Soil moisture sensor for garden/greenhouse
- DIY solar tracker. See https://github.com/happytm/SunTracker
- DIY wire antennas for long range sensors like basement, water/oil tank, garden, wether station, solar tracker if needed. see https://github.com/happytm/EasyAntennas 
- Presence/location detection for family members without using any hardware sensors. https://github.com/happytm/MotionDetector
- Intruder alarm system for whole house without using any hardware sensors. https://github.com/happytm/MotionDetector
  
## Concept in detail

This code can create small standalone network (maximum of 100) of battery powered WiFi devices connecting to one ESP32 gateway device in star network topology.
Each device can be controlled simply by MQTT app like MQTT Dash or built in web interface over local network or over internet if used with DynDNS service like DuckDNS.
There is no need for other home automation software locally or on cloud.

My testing shows data communication is achieved within 80 milliseconds total uptime for remote device out of which only 45 milliseconds of time used for more power hungry WiFi receive & transmit on average thereby saving significant battery power. I think this is better than ESPNow protocol.

### Installation

- To test the code at minimum 1 remote ESP32 and 1 ESP32 Gateway device are required.
- One device (always on and mains powered) use gateway sketch and another device (sleeping most of the time and battery powered) use remote code.
- There are .bin files for gateway and remote sketches in their respective folders if do not want to compile the code.
- Please format flash as SPIFFS file system and upload data folder within Simple Gateway folder.
- New remote device will show up with device ID 246 but the device ID can be changed to any other unused DEVICE ID on network from web interface or MQTT client.
- There can be as many as 100 battery powered devices which can send data to one gateway device. 
- Web interface is started at IP 192.168.4.1 if connected to AP named "ESP" and shows graphs of sensor data from whole network, It also allows to issue commands to any remote device on network.

### Command structure

commands are issued via MQTT payload with topic name "command/" or using dropdown menu at built-in web interface. At least first three commands are necessary.
      
#### Command1 = Device ID Number - (required for all commands)    

```c
- Device ID must be 2 or 3 digits and ending with 2,6,A or E to avoid conflict with other devices.
- See https://serverfault.com/questions/40712/what-range-of-mac-addresses-can-i-safely-use-for-my-virtual-machines.
- Any of following for device ID is valid.
- 06,16,26,36,46,56,66,76,86,96,106,116,126,136,146,156,166,176,186,196,206,216,226,236,246.
```                                            
#### Command2 = Command type - (required for all commands)

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
           
 - 106 =     Set target values.
             It is posiible to send and store 4 target values to remote device for further automation locally in remote device.
           - Example command payload 36/106/<00 to 256>/<00 to 256>/<00 to 256>/<00 to 256>/
 
 - 107 =     Set WiFi Channel.
           - Example command payload 36/107/<00 to 14>/
           
 - 108 =     Set device mode - device mode 0 = normal mode and device mode 1 = OTA update.
           Mainly used for OTA update but can be used to run any alternative Code block.
           - Example command payload 36/108/<00 to 105>/ 
 
 - 109 =     Set sleep time in minutes.
           - Example command payload 36/109/<00 or 255>/
 
 - 110 =     Set device ID.
           - Example command payload 36/110/<00 or 255 if number ends with 2, 6, A, or E>/
           
 - 111 to 120 reserved for future expansion.
 
 - 121 =     Set sensor types for each device.
           - Example command payload 36/121/<00 to 256>/<00 to 256>/<00 to 256>/<00 to 256>/
```

#### Command3 = Command  pinNumber or value1 - (required for all commands)        

```c
- pinNumber in case of command type 101 to 105 above. 
- Value1 in case of command type 106,107,108,109 & 110 commandtype.
```

#### Command4 = value2

#### Command5 = value3

#### Command6 = value4     

### Tested with following MQTT front end GUI client software

- MQTT Dash : https://play.google.com/store/apps/details?id=net.routix.mqttdash&hl=en_US (preffered).
            basic javascript automation possible with this App. 
            See https://github.com/ByTE1974/byte1974.github.io/tree/master/mqttdash/js
- MQTT Explorer : https://github.com/thomasnordquist/MQTT-Explorer
- Node-Red : It is also possible to run Node-Red on android smartphones or tablets. 
             https://nodered.org/docs/getting-started/android

### Useful tools to estimate power consumption and solar battery charging:

- https://github.com/G6EJD/Processor-Solar-Power-Sleep-Calc
- http://www.of-things.de/battery-life-calculator.php
- Very informative discussion on energy harvesting: https://forum.mysensors.org/topic/10812/the-harvester-ultimate-power-supply-for-the-raybeacon-dk/105?lang=en-US 
- Interesting paper describing Batteryless remote sensor device: https://arxiv.org/pdf/1505.06815.pdf
- In depth energy harvesting video at Dave Jones: https://www.youtube.com/watch?v=9aSPopIcKLQ

### Tools used to convert HTML/CSS/JAVASCRIPT TO HEX: 

- https://ayushsharma82.github.io/file2raw/      
- https://gchq.github.io/CyberChef/#recipe=Gzip('Dynamic%20Huffman%20Coding','index.html.gz','',false)To_Hex('0x',0)Split('0x',',0x')&input=PGh0bWw%2BC

## Acknowledgments

This project was possible thanks to creators of following libraries used for this project:
 
 - https://github.com/hsaturn/TinyMqtt
 - https://github.com/leeoniya/uPlot   
 - https://github.com/me-no-dev/ESPAsyncWebServer
