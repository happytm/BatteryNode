# Low Power coin cell sensors

Low battery powered ESP8266 devices using Adhoc Network protocol described by Anthony Elder at https://github.com/HarringayMakerSpace/sonoff-adhoc and excellent uMQTTBroker Library by Martin Ger at https://github.com/martin-ger/uMQTTBroker.

My testing shows 6 bytes (5 different sensor's data + device indentifier using 1 byte)  of sensor data is moved within 55 milliseconds on average.If two way communication required between gateway and remote sensor then data is communiated both ways within 150 milliseconds.

Most suitable use cases - Weather Station, Door/Window sensor, Water/Oil tank level sensor, Presence Detection sensor, Soil moisture sensor for garden etc. 

Required Hardware Links:

ESP8266:

https://www.ebay.com/itm/ESP8266-ESP-12E-Wifi-Serial-Wireless-Transceiver-Remote-Port-Network-Development/323838950930?hash=item4b664e3e12:g:BZcAAOSw72JdCyNR

DC-DC buck regulator:

https://www.ebay.com/itm/10PCS-HT7333-A-7333-A-HT7333-HT7333A-1-TO92-Low-Power-Consumption-v-K7T/323963188059?hash=item4b6db5f35b:g:mIYAAOSwLWhbTbMp

Coin Cell battery:

https://www.ebay.com/itm/16-pcs-LIR2450-Li-ion-3-6V-Volt-Rechargeable-Button-Cell-Coin-Battery-US-Stock/153292477462?hash=item23b0f19416:g:P6cAAOSwAuNW6njZ

Battery holder:

https://www.ebay.com/itm/PCB-Hole-Plugging-Type-CR-LIR2450-Button-Cell-Battery-Holder-5-Pcs-Black-J7J1/253626265024?epid=1549407330&hash=item3b0d4dadc0:g:mW4AAOSw-W9a-9m~

Solar Cell:

https://www.ebay.com/itm/10pcs-5V-30mA-Micro-Mini-Power-Solar-Cells-Panel-Board-Set-For-DIY-Toy-53-30mm/352673192324?_trkparms=aid%3D555018%26algo%3DPL.SIM%26ao%3D1%26asc%3D61112%26meid%3D200eec1edfcf4fbcbb5b66d5e3fa5118%26pid%3D100005%26rk%3D3%26rkt%3D12%26mehot%3Dpf%26sd%3D333302663283%26itm%3D352673192324%26pmt%3D1%26noa%3D0%26pg%3D2047675&_trksid=p2047675.c100005.m1851

Battery charger:

https://www.ebay.com/itm/2X-1A-5V-TP4056-Lithium-Battery-Charging-Module-USB-Board-Electronic-Componen-LL/372528382545?hash=item56bc6c0251:g:UHIAAOSwmRNauwg0

USB connectors:

https://www.ebay.com/itm/5-10-20Pcs-USB2-0-Type-A-Plug-4-pin-Male-Female-Adapter-Connector-Plastic-Cover/183819336730?ssPageName=STRK%3AMEBIDX%3AIT&_trksid=p2057872.m2749.l2649

https://www.ebay.com/itm/10PCS-Mini-Portable-Bright-3-LEDs-Night-Light-USB-Lamp-for-PC-Laptop-Reading/233145266453?var=532628409387&hash=item36488a7115:g:Cl8AAOSwnDRcctLK

https://www.ebay.com/itm/10PCS-Mini-Portable-Bright-3-LED-Night-Light-USB-Lamp-for-PC-Laptop-Reading-YDF/283574068369?hash=item420654e491&var&_trkparms=ispr%3D1&enc=AQAEAAADQKvsXIZtBqdkfsZsMtzFbFsbX3WcW5fmB%2Fx7ZbaZTyext08DotvCxRVjaZ6YwURjfcf59trbU5xcCNlTsKXviwRVaYBpDsthwh6vfd%2FLBouQzW46TvMRdyocf5bQ2%2F2rFJ8ux4CNElpH6dfmSdZDX0GfPKI09XyV6j2O7QsgRrTTsXkHKg2balQEebFXcWbQzc0fz%2BevUeTKhTBvboIt8OflVYDVO74G9XZnEd4i6GMo9sHa2gG0Y31ISoo2w0AMc0NpcpZDwXENd2%2BwMn2UIcsv%2FRgWcDToFjPntxfCPqpQL4eOAkDM2Vlup3liKhLNwc2XBWyHeEeqHWXNgocpA0ZTzkNo11%2Fj2ERjj%2FJbdD2meaDbiKnf0oydXOkLPnHbW3hAMMfqhGlojNhpuMxy%2Bx2REEOlfyEpeIElp4CygbpbUOANi4pWQAHDe5LX2p6TF0jmpRimytt48Mi%2FKQZSx2KeNFrPIb31RdLXfxb7MajCUBqm8%2BZwVnQu9LrywIB6C7jf7Wa7PESjCPiY7Bo6cUy3Wdmiry0z7a40VLYdjoAEgSse5TIKxZH%2FcV%2BbMOviW4WPCswEW5BySiMUNXrgN4KOpPmYhoBkyzCFMOK%2B3a3nk8WBbLS4F0dAbdlv2nDyLmuK05L39ky3v2yvmxfxMAX4JSvx0w1P4IABXl6zg30EiPg85Wm6GZjxtZDnlMuWuvOvGWEQQRmJ7Sams0nsEt5PwiXtA%2B5oqF0jWrumzvuYa%2F80aEp9b5HAQc5ugYJiBP7efX%2BdrMVOhu41r8tZXKDlX1IrsWmEJvU2ng7FJLwM%2FXiiUqZ14fvm04tmaJNOnv42aydTFsvDihWKMRx9JMxb4S%2BXIknbjvSebdeqBBq2WieSt%2F8OwcwfdiyoSQ6DvbDKZL4W7%2FAgrBnHHm%2FygJ9hT1h%2BE2uoPAYnkzNYvtuG60qzoSUxcFQIQQPxfc5Wukhvw2%2F5m5%2FdDtyttNqfm6iZlYCJTzTHDXSERO81PV6VaXgpGIEiPamQtG2%2FlIcRjehaux7VZ0Jma0Hdnfu2PeCbC%2F2XV1XOIBRBJFMzicQ55BYjQ5TdGyz3QxxZin3QIyWKzOyY0JR7UbthiLm%2Bwhk%3D&checksum=283574068369a6003252f5bd4b51818e8b067c33d9f6

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
