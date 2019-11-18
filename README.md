# Low Power low cost sensors, standalone IOT Network (could be used with any android or ios MQTT front end software).

Low battery powered ESP8266 devices using Adhoc Network protocol described by Anthony Elder at https://github.com/HarringayMakerSpace/sonoff-adhoc and excellent uMQTTBroker Library by Martin Ger at https://github.com/martin-ger/uMQTTBroker.

My testing shows 6 bytes (5 different sensor's data + device indentifier using 1 byte)  of sensor data is moved within 55 milliseconds on average.If two way communication required between gateway and remote sensor then data is communiated both ways within 150 milliseconds.

Most suitable use cases - Weather Station, Door/Window sensor, Water/Oil tank level sensor, Presence Detection sensor, Soil moisture sensor for garden etc. 

Tested with following MQTT front end software:

MQTT Dash : https://play.google.com/store/apps/details?id=net.routix.mqttdash&hl=en_US

MQTT Dashboard : 
https://play.google.com/store/apps/details?id=com.thn.iotmqttdashboard&hl=en_US

IOT MQTT Panel : 
https://play.google.com/store/apps/details?id=snr.lab.iotmqttpanel.prod&hl=en_US

Required Hardware Links:

ESP8266:

https://www.ebay.com/itm/ESP8266-ESP-12E-Wifi-Serial-Wireless-Transceiver-Remote-Port-Network-Development/323838950930?hash=item4b664e3e12:g:BZcAAOSw72JdCyNR

DC-DC buck regulator:

https://www.ebay.com/itm/10PCS-HT7333-A-7333-A-HT7333-HT7333A-1-TO92-Low-Power-Consumption-v-K7T/323963188059?hash=item4b6db5f35b:g:mIYAAOSwLWhbTbMp

Lifepo4 battery (Buck regulator above not needed, can not be charge from solar using TP-4056 directly):

https://www.ebay.com/itm/8Pcs-Soshine-3-2V-LiFePO4-10440-AAA-Battery-UM4-FR03-E92-280mAh-Rechargeable-37/163902644062?hash=item26295bdf5e:g:AksAAOSwed9doUB1

Coin Cell battery:

https://www.ebay.com/itm/16-pcs-LIR2450-Li-ion-3-6V-Volt-Rechargeable-Button-Cell-Coin-Battery-US-Stock/153292477462?hash=item23b0f19416:g:P6cAAOSwAuNW6njZ

Battery holder:

https://www.ebay.com/itm/PCB-Hole-Plugging-Type-CR-LIR2450-Button-Cell-Battery-Holder-5-Pcs-Black-J7J1/253626265024?epid=1549407330&hash=item3b0d4dadc0:g:mW4AAOSw-W9a-9m~

Solar Cell:

https://www.ebay.com/itm/10pcs-5V-30mA-Micro-Mini-Power-Solar-Cells-Panel-Board-Set-For-DIY-Toy-53-30mm/352673192324?_trkparms=aid%3D555018%26algo%3DPL.SIM%26ao%3D1%26asc%3D61112%26meid%3D200eec1edfcf4fbcbb5b66d5e3fa5118%26pid%3D100005%26rk%3D3%26rkt%3D12%26mehot%3Dpf%26sd%3D333302663283%26itm%3D352673192324%26pmt%3D1%26noa%3D0%26pg%3D2047675&_trksid=p2047675.c100005.m1851

Battery charger:

https://www.ebay.com/itm/2X-1A-5V-TP4056-Lithium-Battery-Charging-Module-USB-Board-Electronic-Componen-LL/372528382545?hash=item56bc6c0251:g:UHIAAOSwmRNauwg0

Environment sensor:

https://www.ebay.com/itm/Temperature-and-Pressure-BME280-Sensor-Module-Voltage1-71V-3-6V/323861991809?hash=item4b67add181:g:H7UAAOSwDYxbvcJ6

Light/Color/Gesture/Proximity sensor:

https://www.ebay.com/itm/1pcs-GY-9960-3-3-APDS-9960-RGB-Infrared-IR-Gesture-Sensor-Motion-Direction-Recog/183447396095?hash=item2ab6511eff:g:LQoAAOSwbQZbfmeH

Presene sensor:

https://www.ebay.com/itm/5Pcs-RCWL-0516-Microwave-Radar-Sensor-Human-Body-Induction-Switch-Module-Output/192292834493?epid=24018078027&hash=item2cc58bccbd:g:peAAAOSwGY1ZppCB

Distance sensor:

https://www.ebay.com/itm/1PC-Ultrasonic-Module-HC-SR04-Distance-Measuring-Transducer-Sensor-for-Arduino/192696689086?hash=item2cdd9e21be:g:dioAAOSw3IVdgFfz

Soil moisture sensor:

https://www.ebay.com/itm/Analog-Capacitive-Soil-Moisture-Sensor-V1-2-Corrosion-Resistant-With-Cable-Wire/202321853501?epid=14021158393&hash=item2f1b527c3d:g:3QIAAOSwiHRbrt5k

Door/Window sensor:

https://www.ebay.com/itm/5PCS-SW-460-Tilt-Sensor-Electronic-Vibration-Sensor-Switch-Precise-CF-NEW/141975960158?hash=item210e6d3a5e:g:eEQAAOSwj0NUkX7K

https://www.ebay.com/itm/10PCS-SW-100-Electronic-Vibration-Sensor-Switch-Tilt-Sensor-for-Arduin-NMCA/283596557033?_trkparms=aid%3D555018%26algo%3DPL.SIM%26ao%3D1%26asc%3D61112%26meid%3D5f4c53935c40422c92e36ef62365cbb3%26pid%3D100005%26rk%3D1%26rkt%3D12%26mehot%3Dsb%26sd%3D132947034808%26itm%3D283596557033%26pmt%3D1%26noa%3D0%26pg%3D2047675&_trksid=p2047675.c100005.m1851

https://www.ebay.com/itm/20Pcs-vibration-switch-shock-vibration-sensor-shaked-switch-5-5mm-5-5mm-18ODHVXI/392521864551?_trkparms=aid%3D555018%26algo%3DPL.SIM%26ao%3D1%26asc%3D61112%26meid%3D68cbba8061bc4e4fb0bc521bd469c8aa%26pid%3D100005%26rk%3D1%26rkt%3D12%26mehot%3Dco%26sd%3D362797773253%26itm%3D392521864551%26pmt%3D1%26noa%3D0%26pg%3D2047675&_trksid=p2047675.c100005.m1851

https://www.ebay.com/itm/5pcs-A3144-A3144E-OH3144E-Hall-Effect-Sensor-SWITCHES-TO-92UA-3pin-SIP-NEW-CF/141975967989?hash=item210e6d58f5:g:P4AAAOSw54xUYBLp

USB connectors:

https://www.ebay.com/itm/5-10-20Pcs-USB2-0-Type-A-Plug-4-pin-Male-Female-Adapter-Connector-Plastic-Cover/183819336730?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2057872.m2749.l2649

https://www.ebay.com/itm/10PCS-Mini-Portable-Bright-3-LEDs-Night-Light-USB-Lamp-for-PC-Laptop-Reading/233145266453?var=532628409387&hash=item36488a7115:g:Cl8AAOSwnDRcctLK

https://www.ebay.com/itm/10PCS-Mini-Portable-Bright-3-LED-Night-Light-USB-Lamp-for-PC-Laptop-Reading-KVD/254324352827?var=553856471722&hash=item3b36e9a73b:m:mu0g_K_W0-MeXBHUvm8kxKA


# uMQTTBroker
MQTT Broker library for ESP8266 Arduino

You can start an MQTT broker in any ESP Arduino project. Just clone (or download the zip-file and extract it) into the libraries directory of your Arduino ESP8266 installation.

**Important: Use the setting "lwip Variant: 1.4 High Bandwidth" in the "Tools" menu**

lwip 2.0 has some strange behaviour that causes the socket to block after 5 connections.

Thanks to Tuan PM for sharing his MQTT client library https://github.com/tuanpmt/esp_mqtt as a basis with us. The modified code still contains the complete client functionality from the original esp_mqtt lib, but it has been extended by the basic broker service.

The broker does support:
- MQTT protocoll versions v3.1 and v3.1.1 simultaniously
- a smaller number of clients (at least 8 have been tested, memory is the issue)
- retained messages
- LWT
- QoS level 0
- username/password authentication
 
The broker does not yet support:
- QoS levels other than 0
- many TCP(=MQTT) clients
- non-clear sessions
- TLS

If you are searching for a complete ready-to-run MQTT broker for the ESP8266 with additional features (persistent configuration, scripting support and much more) have a look at https://github.com/martin-ger/esp_mqtt .

## API MQTT Broker (C++-style)
The MQTT broker has a new C++ style API with a broker class:
```c
class uMQTTBroker
{
public:
    uMQTTBroker(uint16_t portno=1883, uint16_t max_subscriptions=30, uint16_t max_retained_topics=30);

    void init();

    virtual bool onConnect(IPAddress addr, uint16_t client_count);
    virtual bool onAuth(String username, String password);
    virtual void onData(String topic, const char *data, uint32_t length);

    virtual bool publish(String topic, uint8_t* data, uint16_t data_length, uint8_t qos=0, uint8_t retain=0);
    virtual bool publish(String topic, String data, uint8_t qos=0, uint8_t retain=0);
    virtual bool subscribe(String topic, uint8_t qos=0);
    virtual bool unsubscribe(String topic);

    void cleanupClientConnections();
};
```
Use the broker as shown in oo-examples found in https://github.com/martin-ger/uMQTTBroker/tree/master/examples .

## API MQTT Broker (C-style)
The MQTT broker is started by simply including:

```c
#include "uMQTTBroker.h"
```
and then calling
```c
bool MQTT_server_start(uint16_t portno, uint16_t max_subscriptions, uint16_t max_retained_topics);
```
in the "setup()" function. Now it is ready for MQTT connections on all activated interfaces (STA and/or AP). The MQTT server will run in the background and you can connect with any MQTT client. Your Arduino project might do other application logic in its loop.

Your code can locally interact with the broker using these functions:

```c
bool MQTT_local_publish(uint8_t* topic, uint8_t* data, uint16_t data_length, uint8_t qos, uint8_t retain);
bool MQTT_local_subscribe(uint8_t* topic, uint8_t qos);
bool MQTT_local_unsubscribe(uint8_t* topic);


void MQTT_server_onData(MqttDataCallback dataCb);
```

With these functions you can publish and subscribe topics as a local client like you would with any remote MQTT broker. The provided dataCb is called on each reception of a matching topic, no matter whether it was published from a remote client or the "MQTT_local_publish()" function.

Username/password authentication is provided with the following interface:

```c
typedef bool (*MqttAuthCallback)(const char* username, const char *password, struct espconn *pesp_conn);

void MQTT_server_onAuth(MqttAuthCallback authCb);

typedef bool (*MqttConnectCallback)(struct espconn *pesp_conn, uint16_t client_count);

void MQTT_server_onConnect(MqttConnectCallback connectCb);
```

If an *MqttAuthCallback* function is registered with MQTT_server_onAuth(), it is called on each connect request. Based on username, password, and optionally the connection info (e.g. the IP address) the function has to return *true* for authenticated or *false* for rejected. If a request provides no username and/or password these parameter strings are empty. If no *MqttAuthCallback* function is set, each request will be admitted.

The *MqttConnectCallback* function does a similar check for the connection, but it is called right after the connect request before the MQTT connect request is processed. This is done in order to reject requests from unautorized clients in an early stage. The number of currently connected clients (incl. the current one) is given in the *client_count* paramater. With this info you can reject too many concurrent connections.

If you want to force a cleanup when the broker as a WiFi client (WIFI_STA mode) has lost connectivity to the AP, call:
```c
void MQTT_server_cleanupClientCons();
```
This will remove all broken connections, publishing LWT if defined.

Sample: in the Arduino setup() initialize the WiFi connection (client or SoftAP, whatever you need) and somewhere at the end add these line:
```c
MQTT_server_start(1883, 30, 30);
```

You can find a sample sketch here https://github.com/martin-ger/uMQTTBroker/tree/master/examples .

## API MQTT Client

To use the MQTT client functionality include:

```c
#include "MQTT.h"
```

This code is taken from Ingo Randolf from esp-mqtt-arduino (https://github.com/i-n-g-o/esp-mqtt-arduino). It is a wrapper to tuanpmt's esp_mqtt client library. Look here https://github.com/i-n-g-o/esp-mqtt-arduino/tree/master/examples for code samples.

# Thanks
- tuanpmt for esp_mqtt (https://github.com/tuanpmt/esp_mqtt )
- Ingo Randolf for esp-mqtt-arduino (https://github.com/i-n-g-o/esp-mqtt-arduino)
- Ian Craggs for mqtt_topic
- many others contributing to open software (for the ESP8266)
