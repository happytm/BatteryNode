
#define FIRSTTIME  false  // Define true if setting up Gateway for first time.

#include <WiFi.h>
#include <esp_wifi.h>
#include <EEPROM.h>
#include "FS.h"
#include <SPIFFS.h>
#include <PicoMQTT.h>
#include <PicoWebsocket.h>
#include <WebServer.h>
#include <motionDetector.h> // Manually install this library from https://github.com/happytm/MotionDetector/tree/main/motionDetector
#include "time.h"

WebServer server(80);WiFiServer websocket_underlying_server(81);WiFiServer tcp_server(1883);
PicoWebsocket::Server<::WiFiServer> websocket_server(websocket_underlying_server);PicoMQTT::Server mqtt(tcp_server, websocket_server);

struct tm timeinfo;
#define MY_TZ "EST5EDT,M3.2.0,M11.1.0" //(New York) https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv

//========================================================================================================================//
//              USER-SPECIFIED VARIABLES                                                                                  //
//========================================================================================================================//
const char* apSSID = "ESP"; const char* apPassword = ""; const int apChannel = 6; const int hidden = 0;                   // If hidden is 0 change show hidden to true in remote code.

String dataFile = "/data.json";       // File to store sensor data.

int enableAlarm = 1;             // Enable/Disable alarm.
int motionTrigger = 10;          // Adjust alarm trigger lower the number more sensetive alarm is
int enableCSVgraphOutput = 1;    // 0 disable, 1 enable // if enabled, you may use Tools-> Serial Plotter to plot the variance output for each transmitter. 
int scanInterval = 5000;          // in milliseconds
int motionLevel = RADAR_BOOTING; // initial value = -1, any values < 0 are errors, see motionDetector.h , ERROR LEVELS sections for details on how to intepret any errors.

//==================User configuration not required below this line =============================================

char str [256], s [70];
String ssid,password, graphData, Hour, Minute;
int device, rssi, sleepTime, lastLevel, sensorValues[4], sensorTypes[4]; 
float voltage;
uint8_t mac[6],receivedCommand[6],showConfig[256];
const char* ntpServer = "pool.ntp.org";
unsigned long currentMillis, lastMillis, lastDetected;
unsigned long epoch; 

unsigned long getTime() {time_t now;if (!getLocalTime(&timeinfo)) {Serial.println("Failed to obtain time");return(0);}Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");time(&now);return now;}

//========================================================================================================================//
//              WebPage                                                                                                   //
//========================================================================================================================//

const char webpage[] PROGMEM = R"raw(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0"><title>MQTT.js Websocket Example</title>
    <script src="https://unpkg.com/mqtt/dist/mqtt.min.js" type="text/javascript"></script>
</head>

<body>

<script type="text/javascript">
        
        const clientId = 'MQTTJS CONNECTED' 
        const host = 'ws://' + location.host + ':81';
        const options = {keepalive: 60,clientId: clientId,protocolId: 'MQTT',protocolVersion: 4,clean: true,reconnectPeriod: 1000,connectTimeout: 30 * 1000,}
        console.log('Connecting mqtt client');
        const client = mqtt.connect(host, options)
        client.on('error', (err) => {console.log('Connection error: ', err) 
        client.end()})
        client.on('reconnect', () => {console.log('Reconnecting...')})
        client.on('connect', () => {console.log(`${clientId}`) 
        client.subscribe('#', { qos: 0 })})
        client.on('message', (topic, message, packet) => {
        console.log(`Received Message:- ${message.toString()} in topic:- ${topic}`) })

        var attachEvent = function(node, event, listener, useCapture) {
       // Method for FF, Opera, Chrome, Safari
        if ( window.addEventListener ) {
        node.addEventListener(event, listener, useCapture || false);
        }
       // IE has its own method
       else {
       node.attachEvent('on'+event, listener);
       }
     };

      // Once the window loads and the DOM is ready, attach the event to the main
      attachEvent(window, "load", function() {
      var select_command = document.getElementById("command");

      var selectHandler = function() {
      option1 = document.getElementById("device"),
      option2 = document.getElementById("command1");
      option3 = document.getElementById("command2");
      option4 = document.getElementById("command3");
      option5 = document.getElementById("command4");
      
      // Show and hide the appropriate select's
      if (this.value == "105" || this.value == "106" || this.value == "121") {
       
      option1.style.display = "";
      option2.style.display = "";
      option3.style.display = "";
      option4.style.display = "";
      option5.style.display = "";
       
      } else if (this.value == "103" || this.value == "104" || this.value == "107" || this.value == "108" || this.value == "109"|| this.value == "110" ) {
       
      option1.style.display = "";
      option2.style.display = "";
      option3.style.display = "none";
      option4.style.display = "none";
      option5.style.display = "none";
       
      } else if (this.value == "101" || this.value == "102") {
       
      option1.style.display = "";
      option2.style.display = "";
      option3.style.display = "";
      option4.style.display = "none";
      option5.style.display = "none";
     } 
  };
  
       // Use the onchange and onkeypress events to detect when the 
       // value of select_command has changed
       attachEvent(select_command, "change", selectHandler);
       attachEvent(select_command, "keypress", selectHandler);
       });
 
</script>

<h3>Send Command to ESP32 Gateway Server</h3>

<section class="command">
<form id="formElem">
  
  
  <SELECT class="combine" id ="device" name = "Device">
    
    <option value="">Select Device</option>
    <option value="246">New Device(246)</option>
    <option value="256">Gateway(256)</option>
    <option value="6">Livingroom(6)</option>
    <option value="16">Kitchen(16)</option>
    <option value="26">Bedroom1(26)</option>
    <option value="36">Bedroom2(36)</option>
    <option value="46">Bedroom3(46)</option>
    <option value="56">Bedroom4(56)</option>
    <option value="66">Bathroom1(66)</option>
    <option value="76">Bathroom2(76)</option>
    <option value="86">Bathroom3(86)</option>
    <option value="96">Bathroom4(96)</option>
    <option value="106">Laundry(106)</option>
    <option value="116">Boiler(116)</option>
    <option value="126">Workshop(126)</option>
    <option value="136">Garage(136)</option>
    <option value="146">Office(146)</option>
    <option value="156">WaterTank(156)</option>
    <option value="166">SolarTracker(166)</option>
    <option value="176">WeatherStation(176)</option>
    <option value="186">Greenhouse(186)</option>
</SELECT>

<SELECT class="combine" id ="command" name = "Command">
    <option value="">Select Command Type</option>
    <option value="101">Digital Write(101)</option>
    <option value="102">Analog Write(102)</option>
    <option value="103">Digital Read(103)</option>
    <option value="104">Analog Read(104)</option>
    <option value="105">Neopixel(105)</option>
    <option value="106">Set Target Values(106)</option>
    <option value="107">Set AP Channel(107)</option>
    <option value="108">Set Device Mode(108)</option>
    <option value="109">Set Sleep Time(109)</option>
    <option value="110">Set Device ID(110)</option>
    <option value="121">Set Sensor Types(121)</option>
</SELECT>

<input id="command1" name="command1" type="number" size="4" min="0" max="250">   
<input id="command2" name="command2" type="number" size="4" min="0" max="250">
<input id="command3" name="command3" type="number" size="4" min="0" max="250">
<input id="command4" name="command4" type="number" size="4" min="0" max="250">  
<br><br>
<input name="SSID" type="text" size="12" placeholder="SSID"/>
  <input name="Password" type="password" size="12" placeholder="Password"/>
  <button type="submit">Send Command</button>
  <br>

</form>
</section>

<div class="results">
  <p>Command Sent via MQTT:</p>
  <pre></pre>
</div>

<div style="border: 1px solid #FFFFFF; overflow: hidden; margin-left: 5px; max-width: 820px;">  
<textarea rows="10" cols="75" id="Console" value="" spellcheck="false"  readonly="true" style="font-size:16px;line-height: 1em;">
</textarea>
</div>

<script>     
  // Process command via MQTT
  // https://codetv.dev/blog/get-form-values-as-json#get-multi-select-values-like-checkboxes-as-json-with-the-formdata-api
  function handleFormSubmit(event) {
  event.preventDefault();
  
  const data = new FormData(event.target);
  const formJSON = Object.fromEntries(data.entries());
  const results = document.querySelector('.results pre');
  results.innerText = JSON.stringify(formJSON, null, 2);
   
  // Publish a message
  var values = Object.keys(formJSON).map(function (key) { return formJSON[key]; });
  
  console.log(values);
  client.publish('command', JSON.stringify(values))
}

  const form = document.querySelector('.command');
  form.addEventListener('submit', handleFormSubmit);

</script> 

<script>
 const consoleOutput = document.getElementById("Console") 
 const originalConsoleLog = console.log;
 
      console.log = function() {
      let message = Array.from(arguments).join(' ');
      consoleOutput.innerHTML += `${message}`;
      originalConsoleLog.apply(console, arguments); // Keep the original console.log functionality
    };
</script>

</body>
</html>
)raw";

//========================================================================================================================//
//                FUNCTIONS                                                                                               //
//========================================================================================================================//

void handleNotFound() {String message = "File Not Found\n\n";message += "URI: ";message += server.uri();message += "\nMethod: ";message += (server.method() == HTTP_GET) ? "GET" : "POST";message += "\nArguments: ";message += server.args();message += "\n";for (uint8_t i = 0; i < server.args(); i++) {message += " " + server.argName(i) + ": " + server.arg(i) + "\n";}server.send(404, "text/plain", message);log_i("reply: %s", message.c_str());}

// Had to move all functions above setup function to compile the sketch successfully.
// See: https://forum.arduino.cc/t/exit-status-1-was-not-declared-in-this-scope-error-message/632717

void timeSynch(){ if (mac[1] == 105 || mac[1] == 106) {return;}else{epoch = getTime();Serial.print("Epoch Time: ");Serial.println(epoch); Hour = timeinfo.tm_hour; mac[4] = Hour.toInt();Minute = timeinfo.tm_min; mac[5] = Minute.toInt();Serial.print("Time Sent to remote device ");Serial.print(device);Serial.print("  ");Serial.print(Hour); Serial.print(":"); Serial.println(Minute);}}

void probeRequest(WiFiEvent_t event, WiFiEventInfo_t info) 
{ 
  Serial.println();
  Serial.print("Probe Received :  ");for (int i = 0; i < 6; i++) {Serial.printf("%02X", info.wifi_ap_probereqrecved.mac[i]);if (i < 5)Serial.print(":");}Serial.println();
  Serial.print("Connect at IP: ");Serial.print(WiFi.localIP()); Serial.println(" to monitor and control whole network");
  
  // Allow data from device ID ending in number 6, voltage value between 2.4V and 3.6V. and signal is stonger than -70.
  for (int i = 6; i < 256; i = i+10) if (info.wifi_ap_probereqrecved.mac[0] == i )
  {
    if (info.wifi_ap_probereqrecved.mac[1] > 120 && info.wifi_ap_probereqrecved.mac[1] < 180)
    {
      // This helps reduce interference from unknown devices from far away with weak signals.
      if (info.wifi_ap_probereqrecved.rssi > -70)  
      {
      
      device = info.wifi_ap_probereqrecved.mac[0];
      Serial.println("Contents of command data saved in EEPROM for this device: "); EEPROM.readBytes(0, showConfig,256);for(int i=0;i<10;i++){Serial.printf("%d ", showConfig[i+device]);}
      
      Serial.println();
      for (int i = 0; i < 6; i++) mac[i] = showConfig[i+device];   // Prepare command to be sent to remote device.
      for (int j = 0; j < 4; j++) sensorTypes[j] = showConfig[j+device+6]; // Assign sensor types to the particular device.
                    
      timeSynch();
      //if (mac[1] == 0 || mac[1] == 255) {mac[0] = device; mac[1] = 107; mac[2] = apChannel; timeSynch();}
                     
      esp_err_t err = esp_wifi_set_mac(WIFI_IF_AP, &mac[0]);  //https://randomnerdtutorials.com/get-change-esp32-esp8266-mac-address-arduino/ https://github.com/justcallmekoko/ESP32Marauder/issues/418
      Serial.print("Command sent to remote device: "); Serial.println(WiFi.macAddress());
      Serial.print("Command sent to remote device :  "); for (int i = 0; i < 6; i++) { Serial.print(mac[i]);} Serial.println();        
                
      rssi = info.wifi_ap_probereqrecved.rssi;         
      voltage = info.wifi_ap_probereqrecved.mac[1];
      voltage = voltage * 2 / 100;
      sensorValues[0] = info.wifi_ap_probereqrecved.mac[2];sensorValues[1] = info.wifi_ap_probereqrecved.mac[3];sensorValues[2] = info.wifi_ap_probereqrecved.mac[4];sensorValues[3] = info.wifi_ap_probereqrecved.mac[5];
           
      sprintf (str, "{");sprintf (s, "\"%s\":\"%i\"", "Location", device);    strcat (str, s);sprintf (s, ",\"%s\":\"%.2f\"", "Voltage", voltage);    strcat (str, s);sprintf (s, ",\"%i\":\"%i\"", sensorTypes[0], sensorValues[0]); strcat (str, s);sprintf (s, ",\"%i\":\"%i\"", sensorTypes[1], sensorValues[1]); strcat (str, s);sprintf (s, ",\"%i\":\"%i\"", sensorTypes[2], sensorValues[2]); strcat (str, s);sprintf (s, ",\"%i\":\"%i\"", sensorTypes[3], sensorValues[3]); strcat (str, s);sprintf (s, "}"); strcat (str, s);
                    
      Serial.println("Following ## Sensor Values ## receiced from remote device  & published via MQTT: ");Serial.println(str);
      
      mqtt.publish("Sensor Values", str); 
      
      graphData = ",";graphData += epoch;graphData += ",";graphData += device;graphData += ",";graphData += voltage;graphData += ",";graphData += rssi;graphData += ",";graphData += sensorTypes[0];graphData += ",";graphData += sensorValues[0];graphData += ",";graphData += sensorTypes[1];graphData += ",";graphData += sensorValues[1];graphData += ",";graphData += sensorTypes[2];graphData += ",";graphData += sensorValues[2];graphData += ",";graphData += sensorTypes[3];graphData += ",";graphData += sensorValues[3];graphData += "]";
      
      File f = SPIFFS.open(dataFile, "r+"); // See https://github.com/lorol/LITTLEFS/issues/33
      Serial.print("File size: "); Serial.println(f.size());
      f.seek((f.size()-1), SeekSet); Serial.print("Position: "); Serial.println(f.position());
      f.print(graphData); Serial.println(); Serial.print("Appended to file: "); Serial.println(graphData); Serial.print("File size: "); Serial.println(f.size());
      f.close(); 
                    
      if (voltage < 2.50) { mqtt.publish("Low battery for device: ", String(device)); }      // if voltage of battery gets to low, print the warning below.
      }           
    }
  }    
       // In some cases send command only once.
       //if (mac[1] == 105 || mac[1] == 106 || mac[1] == 108 || mac[1] == 110){for (int i = 6; i < 256; i = i+10) {EEPROM.writeByte(i+1, 107);EEPROM.writeByte(i+2, apChannel);}}
} // End of Proberequest function.

//========================================================================================================================//
//                 SETUP                                                                                                  //
//========================================================================================================================//

void setup(){
  Serial.begin(115200);
  delay(100);
  
  EEPROM.begin(512);
  SPIFFS.begin();
  
#if FIRSTTIME  
  // Setup device numbers and wifi Channel for remote devices in EEPROM permanantly.
  for (int i = 6; i < 256; i = i+10) {EEPROM.writeByte(i, i);EEPROM.writeByte(i+1, 107);EEPROM.writeByte(i+2, apChannel);}
  EEPROM.writeByte(0, apChannel);EEPROM.commit();
#endif  
  
  EEPROM.readBytes(0, showConfig,256);for(int i=0;i<256;i++){Serial.printf("%d ", showConfig[i]);}Serial.println();
  
  motionDetector_init();  // initializes the storage arrays in internal RAM
  motionDetector_config(64, 16, 3, 3, true);  // (samplesize,filter size,trigger threshold,variance samples,autoregressive filter) 
    
  if (enableCSVgraphOutput > 0) {      // USE THE Tools->Serial Plotter to graph the live data!
      motionDetector_enable_serial_CSV_graph_data(enableCSVgraphOutput); // output CSV data only
  }

  if (enableAlarm > 0) {      
      motionDetector_set_alarm_threshold(enableAlarm); // Enable/Disable alarm.
  }
  Serial.setTimeout(100);
  WiFi.mode(WIFI_AP_STA);
  
  WiFi.softAP(apSSID, apPassword, apChannel, hidden);
  esp_wifi_set_event_mask(WIFI_EVENT_MASK_NONE); // This line is must to activate probe request received event handler.
  Serial.print("AP started with SSID: ");Serial.println(apSSID);

  int n = WiFi.scanNetworks();
  int strongestRSSI = -100;
  
  for (int i = 0; i < n; i++) {if (WiFi.RSSI(i) > strongestRSSI) {strongestRSSI = WiFi.RSSI(i); ssid = WiFi.SSID(i);}}

  if (WiFi.SSID(0) == ssid && WiFi.encryptionType(0) == WIFI_AUTH_OPEN) {WiFi.begin(ssid.c_str(), password.c_str());}
  
  mqtt.begin();
    
  server.on("/", []() {server.send(200, "text/html", webpage);});
  server.onNotFound(handleNotFound);
  server.begin();
  
  configTime(0, 0, ntpServer); setenv("TZ", MY_TZ, 1); tzset(); // Set environment variable with your time zone
  epoch = getTime(); Serial.print("Epoch Time: "); Serial.println(epoch); delay(500);

  WiFi.onEvent(probeRequest,WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED);
  Serial.print("Waiting for probe requests ... ");
 
 } // End of Setup function

//========================================================================================================================//
//                     LOOP                                                                                               //
//========================================================================================================================//

void loop()
{
   delay(scanInterval);
   if (WiFi.waitForConnectResult() != WL_CONNECTED) { ESP.restart(); }
   
   if (motionLevel > motionTrigger) {lastDetected = getTime(); lastLevel = motionLevel; mqtt.publish("Motion", String(motionLevel)); }
   
    motionLevel = motionDetector_esp();  // if the connection fails, the radar will automatically try to switch to different operating modes by using ESP32 specific calls. 
    if (enableCSVgraphOutput == 0) {  // for the graph via serial, we just let the library do the job. 
      Serial.print("motionLevel: "); Serial.println(motionLevel);
    }
        
  mqtt.subscribe("command", [](const char * payload) {if (payload && strlen(payload)) {Serial.println(payload);Serial.printf("Received message in topic 'command' & message is:- %s\n", payload); 
     
  auto result = sscanf(payload, R"(["%u","%u","%u","%u","%u","%u",%10[^"]","%10[^"]"])", &receivedCommand[0], &receivedCommand[1], &receivedCommand[2], &receivedCommand[3], &receivedCommand[4], &receivedCommand[5], &ssid, &password);
  
  Serial.println(receivedCommand[0]);Serial.println(receivedCommand[1]);Serial.println(receivedCommand[2]);Serial.println(receivedCommand[3]);Serial.println(receivedCommand[4]);Serial.println(receivedCommand[5]);Serial.println(ssid.c_str());Serial.println(password.c_str()); 
}
      if (receivedCommand[1] == 121) // Set sensor types on devices based on command received from website or mqtt client.
      { for (int i = 0; i < 4; i++) {uint8_t tempSensortypes[4]; tempSensortypes[i] = receivedCommand[i+2]; EEPROM.writeBytes(receivedCommand[0]+6, tempSensortypes,4);}

      } else if (receivedCommand[1] >= 101 && receivedCommand[1] <= 120) // Set everything else on devices based on command received from website or mqtt client.
      {for (int i = 0; i < 6; i++){ uint8_t tempCommand[6]; tempCommand[i] = receivedCommand[i];EEPROM.writeBytes(receivedCommand[0], tempCommand,6);}}

      EEPROM.commit();Serial.println();Serial.println("Command or sensor types saved to EEPROM.");
      EEPROM.readBytes(0, showConfig,256);for(int i=0;i<256;i++){Serial.printf("%d ", showConfig[i]);}Serial.println();
        });
  
        mqtt.loop();
        server.handleClient();
        yield(); 
}//End of loop Function 
