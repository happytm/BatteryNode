#include <WiFi.h>
#include <esp_wifi.h>
#include <PicoMQTT.h>
#include <PicoWebsocket.h>
#include <WebServer.h>

WebServer server(80);WiFiServer websocket_underlying_server(81);WiFiServer tcp_server(1883);

PicoWebsocket::Server<::WiFiServer> websocket_server(websocket_underlying_server);PicoMQTT::Server mqtt(tcp_server, websocket_server);

int rssi;

int apChannel = 7;        // WiFi Channel for this device.
const int hidden = 0;     // If hidden is 1 probe request event handling does not work ?
const char* apSSID = "ESP";
const char* apPassword = "";


const char webpage[] PROGMEM = R"raw(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>MQTT.js Example</title>
    <script src="https://unpkg.com/mqtt/dist/mqtt.min.js" type="text/javascript"></script>
</head>

<body>
  
    <h1>MQTT Example</h1>
    <script type="text/javascript">
        const clientId = 'mqttjs_' + Math.random().toString(16).substr(2, 8)

        //const host = 'ws://' + location.host + ':81';
        const host = 'ws://10.1.10.241:81';
        const options = {keepalive: 60,clientId: clientId,protocolId: 'MQTT',protocolVersion: 4,clean: true,reconnectPeriod: 1000,connectTimeout: 30 * 1000,}
        console.log('Connecting mqtt client');
        const client = mqtt.connect(host, options)
        client.on('error', (err) => {console.log('Connection error: ', err) 
        client.end()})
        client.on('reconnect', () => {console.log('Reconnecting...')})
        client.on('connect', () => {console.log(`Client connected: ${clientId}`) 
        client.subscribe('#', { qos: 0 })})
        client.on('message', (topic, message, packet) => {
        console.log(`Received Message: ${message.toString()} On topic: ${topic}`) })
        
        // Publish a message
        function publishMessage() {client.publish('fromWebpage', 'Published with button press on webpage: ')
        console.log(`publishMessage`);}
    </script>
    <button onclick="publishMessage()">Publish Message</button>
    
</body>

</html>
)raw";

void handleNotFound() {String message = "File Not Found\n\n";message += "URI: ";message += server.uri();message += "\nMethod: ";message += (server.method() == HTTP_GET) ? "GET" : "POST";message += "\nArguments: ";message += server.args();message += "\n";for (uint8_t i = 0; i < server.args(); i++) {message += " " + server.argName(i) + ": " + server.arg(i) + "\n";}server.send(404, "text/plain", message);log_i("reply: %s", message.c_str());}


// Had to move this function above setup function to compile the sketch successfully.
// See: https://forum.arduino.cc/t/exit-status-1-was-not-declared-in-this-scope-error-message/632717
void probeRequest(WiFiEvent_t event, WiFiEventInfo_t info) 
{       
  int macID[6];
  Serial.print("Probe received from MAC ID :  ");for (int i = 0; i < 6; i++) {macID[i] = info.wifi_ap_probereqrecved.mac[i]; Serial.printf("%02X", info.wifi_ap_probereqrecved.mac[i]);if (i < 5)Serial.print(":");}Serial.println();
  for (int i = 0; i < 6; i++) {Serial.println(macID[i], HEX);} 
  
  // This helps reduce interference from unknown devices from far away with weak signals.
  if (info.wifi_ap_probereqrecved.rssi > -90)  
  {
    rssi = info.wifi_ap_probereqrecved.rssi;
    Serial.print("RSSI: "); Serial.println(rssi);  
    Serial.print("Connect at IP: ");Serial.print(WiFi.localIP()); Serial.print(" or 192.168.4.1 with connection to ESP AP");Serial.println(" to access the website");
    
    char str [256], s [70];
    
    sprintf (str, "{");
    sprintf (s, "%i, ", rssi);    
    strcat (str, s);sprintf (s, "%i:%i:%i:%i:%i:%i", macID[0], macID[1], macID[2], macID[3], macID[4], macID[5]); 
    strcat (str, s);sprintf (s, "}"); 
    strcat (str, s);
    mqtt.publish("fromServer", str);
  
  }
}      

void setup() {
  Serial.begin(115200); delay(100);
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin("HTM", "");
  WiFi.softAP(apSSID, apPassword, apChannel, hidden);
  esp_wifi_set_event_mask(WIFI_EVENT_MASK_NONE); // This line is must to activate probe request received event handler.
  
  Serial.begin(115200);

  mqtt.begin();
    
  server.on("/", []() {server.send(200, "text/html", webpage);});
  server.onNotFound(handleNotFound);
  server.begin();

  WiFi.onEvent(probeRequest,WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED);
  Serial.print("Waiting for probe requests ... ");
}


void loop() {

        mqtt.subscribe("#", [](const char * payload) {if (payload && strlen(payload)) {Serial.println(payload);Serial.printf("Received message in topic '#' & message is:- %s\n", payload);}});

        // Publish message with fixed interval
        static unsigned long lastPublish = 0;if (millis() - lastPublish >= 6000) {
        mqtt.publish("fromServer", "Data from server");lastPublish = millis();}
        
        mqtt.loop();
        server.handleClient();
        yield();
}
  
