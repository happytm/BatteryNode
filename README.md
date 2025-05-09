# BatteryNode

### Framework for simplest end to end complete DIY low power, low cost, local standalone or web connected IOT network (no cloud service or programming knowledge required).

## Features

### Ultra low power consumption 

This is the main goal of this project. Proberequest protocol is used for data transfer to conserve power. 
#### TODO : Implement BLE communication using CH582F for battery powered sensors for hardware/software simplicity, lower cost (less than $2) and power consumption (around 15 mA).
#### TODO : Use IR for in-room control of devices.Implement IR wakeup when required using IR phototransistor/photodiode to wakeup microcontroller with sleep interrupt.
#### TODO : https://github.com/mlesniew/PicoMQTT/blob/master/examples/websocket_server/websocket_server.ino
### KISS (Keep It Simple Stupid) design principle

No over engeeniering.
Use analog sensors as much as possible for simplicity in hardware and code with lower hardware cost.
Use ESP-CAM hardware for low cost video survailance or motion tracking. 
- See https://github.com/eloquentarduino/EloquentVision & https://github.com/eloquentarduino/EloquentEsp32cam

### True DIY for low cost system

The goal is to implement whole house automation under $100 for upto 20 sensor/actuators (or $5 or less per sensor) including survailance by couple of ESP-CAMS. In most use cases no need to buy expensive commercial products except ESP32 barebone modules and NTC thermistors for temperature sensor, photoresistors or phototransistors for light sensor,
some low cost sensor modules, LDO and/or battery charging IC, coin cell battery and/or small solar panels based on use cases. 

### Simple hardware structure using 3 types of technology - WiFi for motion/presence detection and data/graphs, BLE for remote battery powered sensors, IR for in-room control.

#### There are 3 types of ESP32/CH582F/CH570/CH572 devices.

- The mains powered single ESP32 Gateway device (may or may not be connected to internet) with SoftAP setting.This is the main brain and single point entry from a smartphone or a PC responsible for providing access to moniter and 
control whole home automation network.
- Maines powered room/zone sensors with WiFi station setting. There must be atleast 1 sensor for each room/zone. Ideally they should have IR transmitter/IR blaster to control any appliace with IR receiver within line of sight. Make any electric device IR controllable using https://www.youtube.com/watch?v=RdqzdMUO6QE
- Battery powered sensors mostly for locations (indoor/outdoor) where mains power is not available. Mostly used for monitoring purpose but can be used for control purposes also. TODO : Implement BLE communication using CH582F/CH570/CH572 for battery powered sensors for lower cost (less than $2) and power consumption (around 15 mA).
   
### Easily customisable with basic coding knowledge

There is no need to use overwhelming and heavy software like home assistance. Easily customise look and feel of front end in javascript or remote sensors code in C++  as you wish. 

### No local server or cloud service (no IOT) required except built in webserver on ESP32 Gateway device

Whole network can be monitored and controlled using any android or ios MQTT front end dashboard client app like MQTT Dash thereby making it very simple to implement.There is also very simple but powerful dashbord to monitor sensor data from whole network using awsome uPlot graphing library and built in webserver. Sensor data of whole network is stored on ESP32 gateway device using SPIFFS file system. The web interface is simple one page but powerful enough to monitor and control whole network of devices. Web interface could be customised any way you want live by changing html, css or javascript code using built in SPIFFSEditor library without recompiling or reuploading files. Changing data format in SPIFFS also possible live from web interface using SPIFFSEditor.

### Build it, position towards maximum sun exposure and forget about it

No need to charge small 100 mAh LIR2450 coin cell battery or any chargable battery manually for life of battery or solar cell. Small 53mmx30mm or even smaller 3V to 5V solar cell can be used.

### Very low cost DIY sensors (see ideas below)

Average sensor node could be made for $2 to $5 (less than $2 using CH582F microcontroller) each including battery and/or small solar panel for charging. Use of IR transmitters recommended as much as possible for in room automation. 
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
- DIY analog Soil moisture sensor for garden/greenhouse https://github.com/rbaron/b-parasite
- DIY solar tracker using either couple of motors (geared motors/DIY linear actuators/chinese windlass system) or miniature DC water pumps, inexpensive accelerometer like LIS2DH12 and 2 servos to lock down solar panel.
  - For details see https://github.com/happytm/SunTracker
- DIY wire antennas for long range sensors like basement, water/oil tank, garden, wether station, solar tracker if needed.
  - For details see https://github.com/happytm/EasyAntennas 
- Presence/location detection for family members without using any hardware sensors.
  - For details see https://github.com/happytm/MotionDetector
- Intruder alarm system for whole house without using any hardware sensors.
  - For details see https://github.com/happytm/MotionDetector
- Low cost video survailance with or without motion tracking Using ESP-CAM hardware for . See
  - For details see https://github.com/eloquentarduino/EloquentVision & https://github.com/eloquentarduino/EloquentEsp32cam  

## Concept in detail

This code can create small standalone network (maximum of 100) of battery powered WiFi devices connecting to one ESP32 gateway device in star network topology.
Each device can be controlled simply by MQTT app like MQTT Dash or built in web interface over local network or over internet if used with DynDNS service like DuckDNS.
There is no need for other home automation software locally or on cloud.

My testing shows data communication is achieved within 80 milliseconds total uptime for remote device out of which only 45 milliseconds of time used for more power hungry WiFi receive & transmit on average thereby saving significant battery power. I think this is better than ESPNow protocol.

### Installation & testing

- To test the code at least 1 remote ESP32 and 1 ESP32 Gateway device are required.
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

## Ideas worth exploring

- Digital potentiometer using built in LED and photoresister to control diac/triac dimmer or use FL5150 based dimmer.
- AC-DC power converter using BP2525F.
- ACS712 for energy measurement.
- IR wakeup using https://www.vishay.com/docs/80067/appoverview.pdf
- Battery as a switch for door window sensor.

## Acknowledgments

This project was possible thanks to creators of following libraries used for this project:
 
 - https://github.com/hsaturn/TinyMqtt
   - TO DO: Try https://github.com/mlesniew/PicoMQTT/blob/master/examples/websocket_server/websocket_server.ino
 - https://github.com/leeoniya/uPlot   
 - https://github.com/me-no-dev/ESPAsyncWebServer
 - https://github.com/eloquentarduino/EloquentVision & https://github.com/eloquentarduino/EloquentEsp32cam
