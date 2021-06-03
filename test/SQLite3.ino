#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h> // Thanks to 
#include <SPI.h>
#include <FS.h>
#include <LITTLEFS.h>

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
   Serial.print(F("Time taken in microseconds: "));
   Serial.println(micros()-start);
   return rc;
}

void setup() {

   Serial.begin(115200);
   
   

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

   // Create the db file before trying to open it.
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
   String timestamp = "1601211530";
   String date = "060121";
   
   String insert = "insert into test1 values (";
  
               insert += timestamp;
               insert += ", ";
               insert += date;
               insert += ", ";
               insert += 1530;
               insert += ", ";
               insert += 15;
               insert += ", ";
               insert += 26;
               insert += ", ";
               insert += 2.80;
               insert += ", ";
               insert += 45;
               insert += ", ";
               insert += 55;
               insert += ", ";
               insert += 65;
               insert += ", ";
               insert += 230;
               insert += ", ";
               insert += 70;
               insert += ")";
               Serial.println(insert);
              
   
   rc = db_exec(db1, insert.c_str());
   if (rc != SQLITE_OK) {
       sqlite3_close(db1);
       return;
   }
   insert = "insert into test1 values (";
  
               insert += timestamp;
               insert += ", ";
               insert += date;
               insert += ", ";
               insert += 1530;
               insert += ", ";
               insert += 15;
               insert += ", ";
               insert += 27;
               insert += ", ";
               insert += 2.80;
               insert += ", ";
               insert += 45;
               insert += ", ";
               insert += 50;
               insert += ", ";
               insert += 65;
               insert += ", ";
               insert += 230;
               insert += ", ";
               insert += 70;
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
   rc = db_exec(db1, "Select * from test1 where not Location = '27'");
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

}

void loop() {
   
}