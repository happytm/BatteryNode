# Very simple DIY Low Power low cost sensors, Local standalone (no cloud service required) IOT Network.



# Features:

## Very low power consumption.

Proberequest protocol is used for data transfer to conserve power.

## No local linux server or cloud service required.

Whole network can be accesed using any android or ios MQTT front end dashboard client app like MQTT Dash thereby making it very simple to implement. 


## Build it, position towards maximum sun exposure and forget about it. 

No need to charge small 100 mAh LIR2450 coin cell battery manually for life of battery or solar cell if LIR2450 coin cell battery and 53mmx30mm small solar cell linked below in hardware section is used.

## Very low cost DIY sensors.

Average sensor node could be below $5. All the low cost sensors are listed in hardware section below.

## Person Locator/presence detector.

User's presence in the particular room can be located using proberequest protocol thereby allowing further automation based on presence.

## OTA update using github. 

Simply placing .bin file to github and publishing MQTT command from MQTT client app all sensor devices on the network can be updated with latest firmware. Simply publish "command/devicenumber/09/01" from any MQTT client. 

# Concept in detail:

This code create small standalone network (maximum of 100) of battery powered esp8266 devices connecting to one esp8266 gateway device in star network topology.Each device can be controlled simply by MQTT app like MQTT Dash over local network or over internet if used with DynDNS service like DuckDNS.There is no need for other home automation software locally or on cloud.

My testing shows 18 bytes of data (4 different sensor's values with their respective sensor types + 5 different device status data + battery voltage using 1 byte + device indentifier using 1 byte)  is moved within 55 milliseconds on average thereby saving significant battery power.If two way communication required between gateway and remote sensor then data is communiated both ways within 150 milliseconds (more efficient than ESPNow ?). With two way communication activated even control of actuator is possible but not justifiable for battery powered devices.

The device can auto update firmware via Github if specific .bin file is available.Use payload <deviceid>/09/01 under topic "command/".

### Installation:

To use the code atminimum 1 ESP8266 as a slave and 1 ESP32 device as a gateway or both ESP8266 devices are required.One device (always on and mains powered) use ProbeReceiver sketch and another device (sleeping most of the time and battery powered) use ProbeSender code.There can be as many as 100 battery powered devices which can send data to one ProbeReceiver device. The wifi channel number of your devices needs to be same in order to have least amount of time consumed to exchange the sensor data.In gateway sketch ssid and password of your home access point required for access to whole sensor network via MQTT Dash app to read sensor data and to issue commands to remote devices.

### Commands to control any remote devices on network by publishing MQTT messages via any MQTT client app (if #define DUPLEX is true in ProbeSender code)

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

MQTT Dash : https://play.google.com/store/apps/details?id=net.routix.mqttdash&hl=en_US (preffered).
            basic javascript automation possible with this App. 
            See https://github.com/ByTE1974/byte1974.github.io/tree/master/mqttdash/js

MQTT Explorer : https://github.com/thomasnordquist/MQTT-Explorer

Node-Red :
It is also possible to run Node-Red on android smartphones or tablets. 
https://nodered.org/docs/getting-started/android

## Useful tool to estimate power consumption and solar battery charging:
https://github.com/G6EJD/Processor-Solar-Power-Sleep-Calc

http://www.of-things.de/battery-life-calculator.php

## Required Hardware Links:

### ESP8266:

https://www.ebay.com/itm/ESP8266-ESP-12E-Wifi-Serial-Wireless-Transceiver-Remote-Port-Network-Development/323838950930?hash=item4b664e3e12:g:BZcAAOSw72JdCyNR

### DC-DC buck regulator:

https://www.ebay.com/itm/10PCS-HT7333-A-7333-A-HT7333-HT7333A-1-TO92-Low-Power-Consumption-v-K7T/323963188059?hash=item4b6db5f35b:g:mIYAAOSwLWhbTbMp

### Lifepo4 battery (Buck regulator above not needed, can not be charged from solar using TP-4056 directly):

https://www.ebay.com/itm/8Pcs-Soshine-3-2V-LiFePO4-10440-AAA-Battery-UM4-FR03-E92-280mAh-Rechargeable-37/163902644062?hash=item26295bdf5e:g:AksAAOSwed9doUB1

### Coin Cell battery:

https://www.ebay.com/itm/16-pcs-LIR2450-Li-ion-3-6V-Volt-Rechargeable-Button-Cell-Coin-Battery-US-Stock/153292477462?hash=item23b0f19416:g:P6cAAOSwAuNW6njZ

### Battery holder:

https://www.ebay.com/itm/PCB-Hole-Plugging-Type-CR-LIR2450-Button-Cell-Battery-Holder-5-Pcs-Black-J7J1/253626265024?epid=1549407330&hash=item3b0d4dadc0:g:mW4AAOSw-W9a-9m~

### Solar Cell:

https://www.ebay.com/itm/10pcs-5V-30mA-Micro-Mini-Power-Solar-Cells-Panel-Board-Set-For-DIY-Toy-53-30mm/352673192324?_trkparms=aid%3D555018%26algo%3DPL.SIM%26ao%3D1%26asc%3D61112%26meid%3D200eec1edfcf4fbcbb5b66d5e3fa5118%26pid%3D100005%26rk%3D3%26rkt%3D12%26mehot%3Dpf%26sd%3D333302663283%26itm%3D352673192324%26pmt%3D1%26noa%3D0%26pg%3D2047675&_trksid=p2047675.c100005.m1851

### Battery charger:

https://www.ebay.com/itm/2X-1A-5V-TP4056-Lithium-Battery-Charging-Module-USB-Board-Electronic-Componen-LL/372528382545?hash=item56bc6c0251:g:UHIAAOSwmRNauwg0

### Batterymonitor:

https://www.ebay.com/itm/10pcs-DS2438-DS2438Z-TR-DS2438Z-SOP-8/292502146541

### Environment sensor:

https://www.ebay.com/itm/Temperature-and-Pressure-BME280-Sensor-Module-Voltage1-71V-3-6V/323861991809?hash=item4b67add181:g:H7UAAOSwDYxbvcJ6

### Light/Color/Gesture/Proximity sensor:

https://www.ebay.com/itm/1pcs-GY-9960-3-3-APDS-9960-RGB-Infrared-IR-Gesture-Sensor-Motion-Direction-Recog/183447396095?hash=item2ab6511eff:g:LQoAAOSwbQZbfmeH

### Presene sensor:

https://www.ebay.com/itm/5Pcs-RCWL-0516-Microwave-Radar-Sensor-Human-Body-Induction-Switch-Module-Output/192292834493?epid=24018078027&hash=item2cc58bccbd:g:peAAAOSwGY1ZppCB

### Distance sensor:

https://www.ebay.com/itm/1PC-Ultrasonic-Module-HC-SR04-Distance-Measuring-Transducer-Sensor-for-Arduino/192696689086?hash=item2cdd9e21be:g:dioAAOSw3IVdgFfz

### Soil moisture sensor:

https://www.ebay.com/itm/Analog-Capacitive-Soil-Moisture-Sensor-V1-2-Corrosion-Resistant-With-Cable-Wire/202321853501?epid=14021158393&hash=item2f1b527c3d:g:3QIAAOSwiHRbrt5k

### Door/Window sensor:

https://www.ebay.com/itm/5PCS-SW-460-Tilt-Sensor-Electronic-Vibration-Sensor-Switch-Precise-CF-NEW/141975960158?hash=item210e6d3a5e:g:eEQAAOSwj0NUkX7K

https://www.ebay.com/itm/10PCS-SW-100-Electronic-Vibration-Sensor-Switch-Tilt-Sensor-for-Arduin-NMCA/283596557033?_trkparms=aid%3D555018%26algo%3DPL.SIM%26ao%3D1%26asc%3D61112%26meid%3D5f4c53935c40422c92e36ef62365cbb3%26pid%3D100005%26rk%3D1%26rkt%3D12%26mehot%3Dsb%26sd%3D132947034808%26itm%3D283596557033%26pmt%3D1%26noa%3D0%26pg%3D2047675&_trksid=p2047675.c100005.m1851

https://www.ebay.com/itm/20Pcs-vibration-switch-shock-vibration-sensor-shaked-switch-5-5mm-5-5mm-18ODHVXI/392521864551?_trkparms=aid%3D555018%26algo%3DPL.SIM%26ao%3D1%26asc%3D61112%26meid%3D68cbba8061bc4e4fb0bc521bd469c8aa%26pid%3D100005%26rk%3D1%26rkt%3D12%26mehot%3Dco%26sd%3D362797773253%26itm%3D392521864551%26pmt%3D1%26noa%3D0%26pg%3D2047675&_trksid=p2047675.c100005.m1851

https://www.ebay.com/itm/5pcs-A3144-A3144E-OH3144E-Hall-Effect-Sensor-SWITCHES-TO-92UA-3pin-SIP-NEW-CF/141975967989?hash=item210e6d58f5:g:P4AAAOSw54xUYBLp

### USB connectors:

https://www.ebay.com/itm/5-10-20Pcs-USB2-0-Type-A-Plug-4-pin-Male-Female-Adapter-Connector-Plastic-Cover/183819336730?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2057872.m2749.l2649

https://www.ebay.com/itm/10PCS-Mini-Portable-Bright-3-LEDs-Night-Light-USB-Lamp-for-PC-Laptop-Reading/233145266453?var=532628409387&hash=item36488a7115:g:Cl8AAOSwnDRcctLK

https://www.ebay.com/itm/10PCS-Mini-Portable-Bright-3-LED-Night-Light-USB-Lamp-for-PC-Laptop-Reading-KVD/254324352827?var=553856471722&hash=item3b36e9a73b:m:mu0g_K_W0-MeXBHUvm8kxKA



