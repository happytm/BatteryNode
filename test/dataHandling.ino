
float voltage = 2.62;
const char* room = "livingroom";
const char* sensorType1 = "temperature";
const char* sensorType2 = "humidity";
const char* sensorType3 = "pressure";
const char* sensorType4 = "light";

int sensorValue1 = 25;
int sensorValue2 = 50;
int sensorValue3 = 1000;
int sensorValue4 = 80;

const char* statusType1 = "rssi";

const char *statusType2 = "mode";
const char *statusType3 = "ip";
const char *statusType4 = "channel";
const char *statusType5 = "sleeptime";
const char *statusType6 = "uptime";

int deviceStatus1 = -67;
int deviceStatus2 = 1;
int deviceStatus3 = 8;
int deviceStatus4 = 7;
int deviceStatus5 = 10;
int deviceStatus6 = 12;

// -----------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();


//char *
//void fmtStr() 
//{
    static char str [400] = {};
    char s [40];

    sprintf (s,  "\"%s\":\"%s\"", "room", room);    strcat (str, s);
    sprintf (s, ",\"%s\":\"%.2f\"", "voltage", voltage);    strcat (str, s);

    sprintf (s, ",\"%s\":\"%d\"", sensorType1, sensorValue1); strcat (str, s);
    sprintf (s, ",\"%s\":\"%d\"", sensorType2, sensorValue2); strcat (str, s);
    sprintf (s, ",\"%s\":\"%d\"", sensorType3, sensorValue3); strcat (str, s);
    sprintf (s, ",\"%s\":\"%d\"", sensorType4, sensorValue4); strcat (str, s);

    sprintf (s, ",\"%s\":\"%d\"", statusType1, deviceStatus1); strcat (str, s);
    sprintf (s, ",\"%s\":\"%d\"", statusType2, deviceStatus2); strcat (str, s);
    sprintf (s, ",\"%s\":\"%d\"", statusType3, deviceStatus3); strcat (str, s);
    sprintf (s, ",\"%s\":\"%d\"", statusType4, deviceStatus4); strcat (str, s);
    sprintf (s, ",\"%s\":\"%d\"", statusType5, deviceStatus5); strcat (str, s);
    sprintf (s, ",\"%s\":\"%d\"", statusType6, deviceStatus6); strcat (str, s);
    
    Serial.println(str);
    
    delay(100);

}  // end Setup.

void loop()
{

}
