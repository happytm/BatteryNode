// SQLite reference at https://www.w3schools.com/sql/exercise.asp
#define FIRSTTIME    false 
#include <WiFi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h> // Thanks to https://github.com/siara-cc/esp32_arduino_sqlite3_lib
#include <SPI.h>
#include <FS.h>
#include <LITTLEFS.h>
#include <ezTime.h>   // Arduino built in
String Timestamp, Date, Day, Time, Hour;
String epoch, dateReceived, dayReceived, timeReceived, hourReceived, rooms, V, S, T, H, P, L;

   
const char* ssid = ""; // Your WiFi SSID
const char* password = ""; // Your WiFi Password

String deviceData;
String sensorData;
int device;

#define FORMAT_LITTLEFS_IF_FAILED true
int rc;
sqlite3 *db1;

const char* data = "Data requested below:";
static int callback(void *data, int argc, char **argv, char **azColName) {
   int i;
   
   Serial.printf("%s: ", (const char*)data);
   Serial.println();
      
       for (i = 0; i<argc; i++){
       Serial.printf("%s\n", argv[i] ? argv[i] : "NULL");
       
      
      
   }
   Serial.printf("\n");
   return 0;
}

int db_open(const char *filename, sqlite3 **db) {
   int rc = sqlite3_open(filename, db);
   if (rc) {
       Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
       return rc;
   } else {
       Serial.printf("Opened database successfully\n");
   }
   return rc;
}

char *zErrMsg = 0;
int db_exec(sqlite3 *db, const char *sql) {
   Serial.println(sql);
   long start = micros();
   int rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
   if (rc != SQLITE_OK) {
       Serial.printf("SQL error: %s\n", zErrMsg);
       sqlite3_free(zErrMsg);
   } else {
       Serial.printf("Operation done successfully\n");
   }
   Serial.print(F("Time taken in milliseconds: "));
   Serial.println((micros()-start)/1000);
   return rc;
}

void setup() {

  Serial.begin(115200);
  delay(500);

//  Serial << "Starting WiFi connection......." << endl;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
  //  Serial << '-';
    delay(500);
  }

  //Serial << "Connected to " << ssid << " IP address: " << WiFi.localIP() << endl;
  processData(); 
   
  waitForSync();
  Serial.println("UTC: " + UTC.dateTime());
  
  Timezone America;
  America.setLocation("America/New_York");
  //America.setPosix("EST--5EDT,M3.2.0,M11.1.0/2");
  Serial.println("EST time: " + America.dateTime());
  Serial.println("Time now is:" + America.dateTime("l ~t~h~e jS ~o~f F Y, g:i A") );
                                          //Saturday the 25th of August 2018, 2:23 PM
  
  //Serial.println(String((now()))); // + buffer); //for milliseconds precision                                      
  Timestamp = String((now())); // + buffer;     //for milliseconds precision
  //Serial.println(" " + America.dateTime("mdy") );                                       
  Date = " " + America.dateTime("mdy");
  Day  = " " + America.dateTime("l");
  //Serial.println(" " + America.dateTime("Hi") );                                       
  Time = " " + America.dateTime("Hi");
  Hour = " " + America.dateTime("H");

#if FIRSTTIME   
   if (!LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
       Serial.println("Failed to mount file system");
       return;
   }

   // list LITTLEFS contents
   File root = LITTLEFS.open("/");
   if (!root) {
       Serial.println("- failed to open directory");
       return;
   }
   if (!root.isDirectory()) {
       Serial.println(" - not a directory");
       return;
   }
   File file = root.openNextFile();
   while (file) {
       if (file.isDirectory()) {
           Serial.print("  DIR : ");
           Serial.println(file.name());
       } else {
           Serial.print("  FILE: ");
           Serial.print(file.name());
           Serial.print("\tSIZE: ");
           Serial.println(file.size());
       }
       file = root.openNextFile();
   }

   // remove existing file so we start clean for this test.
   LITTLEFS.remove("/test1.db");
   LITTLEFS.remove("/test1.db-journal");

//    Create the db file before trying to open it.
   if (!LITTLEFS.exists("/test1.db")){
      File file = LITTLEFS.open("/test1.db", FILE_WRITE);   //  /littlefs is automatically added to the front 
      file.close();
   }

   // We also need to create the journal file.
   if (!LITTLEFS.exists("/test1.db-journal")){
      File file = LITTLEFS.open("/test1.db-journal", FILE_WRITE);   //  /littlefs is automatically added to the front 
      file.close();
   }

   sqlite3_initialize();

   if (db_open("/littlefs/test1.db", &db1))
       return;

   rc = db_exec(db1, "CREATE TABLE test1 (id INTEGER, Date, Time, Hour, Location, Voltage, RSSI, Temperature, Humidity, Pressure, Light);");
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       return;
   }
#endif  
   
}

void loop() {
   processData();
   delay(10000);
}

void processData()
{
              
               device = random(1,10);
               int voltage = random (26, 33);
               int temperature = random (65,70);
               int humidity = random (55,60);
               int pressure = random (80,90);
               int light = random (70,80);
               
               sensorData = ("{" + String(device) + "," + String(voltage) + "," + String(temperature) + "," + String(humidity) + "," + String(pressure) + "," + String(light) + "}");
               //Serial.println();
               //Serial.print("Received Sensor data: "); 
               //Serial.println(sensorData);
               //Serial.println();
 
               //myClient.publish("Sensor", sensorData);

               device = random(1,10);
               int rssi = random (40, 45);
               int deviceMode = random (1,3);
               int WiFiChannel = random (1,11);
               int deviceIP = device;
               int sleepTime = random (1,100);
               
               deviceData = ("{" + String(device) + "," + String(rssi) + "," + String(deviceMode) + "," + String(WiFiChannel) + "," + String(deviceIP) + "," + String(sleepTime) + "}");
               //Serial.print("Received Status data : "); 
               //Serial.println(deviceData);
               //Serial.println();
                      
               //myClient.publish("Device", deviceData);

  Timezone America;
  America.setLocation("America/New_York");
  //America.setPosix("EST--5EDT,M3.2.0,M11.1.0/2");
  Serial.println("EST time: " + America.dateTime());
  Serial.println("Time now is:" + America.dateTime("l ~t~h~e jS ~o~f F Y, g:i A") );
                                          //Saturday the 25th of August 2018, 2:23 PM
  
  //Serial.println(String((now()))); // + buffer); //for milliseconds precision                                      
  Timestamp = String((now())); // + buffer;     //for milliseconds precision
  //Serial.println(" " + America.dateTime("mdy") );                                       
  Date = " " + America.dateTime("mdy");
  Day  = " " + America.dateTime("l");
  //Serial.println(" " + America.dateTime("Hi") );                                       
  Time = " " + America.dateTime("Hi");
  Hour = " " + America.dateTime("H");
   
   if (!LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
       Serial.println("Failed to mount file system");
       return;
   }

   //Create the db file if it does not exist before trying to open it.
   if (!LITTLEFS.exists("/test1.db")){
      File file = LITTLEFS.open("/test1.db", FILE_WRITE);   //  /littlefs is automatically added to the front 
      file.close();
   }

   // We also need to create the journal file if it does not exist.
   if (!LITTLEFS.exists("/test1.db-journal")){
      File file = LITTLEFS.open("/test1.db-journal", FILE_WRITE);   //  /littlefs is automatically added to the front 
      file.close();
   }

   sqlite3_initialize();

   if (db_open("/littlefs/test1.db", &db1))
       return;

   String insert = "insert into test1 values (";
  
               insert += Timestamp;
               insert += ", ";
               insert += Date;
               insert += ", ";
               insert += Time;
               insert += ", ";
               insert += Hour;
               insert += ", ";
               insert += device;
               insert += ", ";
               insert += voltage;
               insert += ", ";
               insert += rssi;
               insert += ", ";
               insert += temperature;
               insert += ", ";
               insert += humidity;
               insert += ", ";
               insert += pressure;
               insert += ", ";
               insert += light;
               insert += ")";
               Serial.println(insert);
              
   
   rc = db_exec(db1, insert.c_str());
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       return;
   }
   
   
   rc = db_exec(db1, "Select Temperature, Humidity FROM test1");
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       return;
   }
   rc = db_exec(db1, "Select * from test1 where  Location = '26' and id = '0601211530'");
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       return;
   }
   rc = db_exec(db1, "Select * from test1 where Location in ( '26', '27')");
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       return;
   }
   rc = db_exec(db1, "Select * from test1 where not Location = '27'");
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       return;
   }
   rc = db_exec(db1, "Select min(Temperature) from test1");
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       return;
   }
   rc = db_exec(db1, "Select max(Temperature) from test1");
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       return;
   }
   rc = db_exec(db1, "Select avg(Temperature) from test1");
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       return;
   }
   rc = db_exec(db1, "Select * from test1 where Location between '20' and '30'");
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       return;
   }
   sqlite3_close(db1);
   // list LITTLEFS contents
   File root = LITTLEFS.open("/");
   if (!root) {
       Serial.println("- failed to open directory");
       return;
   }
   if (!root.isDirectory()) {
       Serial.println(" - not a directory");
       return;
   }
   File file = root.openNextFile();
   while (file) {
       if (file.isDirectory()) {
           Serial.print("  DIR : ");
           Serial.println(file.name());
       } else {
           Serial.print("  FILE: ");
           Serial.print(file.name());
           Serial.print("\tSIZE: ");
           Serial.println(file.size());
       }
       file = root.openNextFile();
   }
 }
