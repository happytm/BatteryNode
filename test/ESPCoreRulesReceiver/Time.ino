//********************************************************************************************
// Time (clock)
//********************************************************************************************
#if FEATURE_TIME

#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24UL)
#define DAYS_PER_WEEK (7UL)
#define SECS_PER_WEEK (SECS_PER_DAY * DAYS_PER_WEEK)
#define SECS_PER_YEAR (SECS_PER_WEEK * 52UL)
#define SECS_YR_2000  (946684800UL) // the time at the start of y2k
#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )

struct  timeStruct {
  uint8_t Second;
  uint8_t Minute;
  uint8_t Hour;
  uint8_t Wday;   // day of week, sunday is day 1
  uint8_t Day;
  uint8_t Month;
  uint8_t Year;   // offset from 1970;
} tm;

uint32_t syncInterval = 3600;  // time sync will be attempted after this many seconds
uint32_t sysTime = 0;
uint32_t prevMillis = 0;
uint32_t nextSyncTime = 0;

byte PrevMinutes = 0;

//********************************************************************************************
// get time directly from NTP request
//********************************************************************************************
unsigned long updateNtp(){
  unsigned long  t = getNtpTime();
  if (t != 0) {
    if (Settings.DST)
      t += SECS_PER_HOUR; // add one hour if DST active
    setTime(t);
    breakTime(sysTime, tm);
  }
  return t;
}

//********************************************************************************************
// get time as string like hh:mm
//********************************************************************************************
String getTimeString(char delimiter)
{
  String reply;
  if (hour() < 10)
    reply += F("0");
  reply += String(hour());
  if (delimiter != '\0')
    reply += delimiter;
  if (minute() < 10)
    reply += F("0");
  reply += minute();
  return reply;
}


//********************************************************************************************
// Convert epoch to time struct
//********************************************************************************************
void breakTime(unsigned long timeInput, struct timeStruct &tm) {
  uint8_t year;
  uint8_t month, monthLength;
  uint32_t time;
  unsigned long days;
  const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  time = (uint32_t)timeInput;
  tm.Second = time % 60;
  time /= 60; // now it is minutes
  tm.Minute = time % 60;
  time /= 60; // now it is hours
  tm.Hour = time % 24;
  time /= 24; // now it is days
  tm.Wday = ((time + 4) % 7) + 1;  // Sunday is day 1

  year = 0;
  days = 0;
  while ((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time) {
    year++;
  }
  tm.Year = year; // year is offset from 1970

  days -= LEAP_YEAR(year) ? 366 : 365;
  time  -= days; // now it is days in this year, starting at 0

  days = 0;
  month = 0;
  monthLength = 0;
  for (month = 0; month < 12; month++) {
    if (month == 1) { // february
      if (LEAP_YEAR(year)) {
        monthLength = 29;
      } else {
        monthLength = 28;
      }
    } else {
      monthLength = monthDays[month];
    }

    if (time >= monthLength) {
      time -= monthLength;
    } else {
      break;
    }
  }
  tm.Month = month + 1;  // jan is month 1
  tm.Day = time + 1;     // day of month
}


//********************************************************************************************
// set time
//********************************************************************************************
void setTime(unsigned long t) {
  sysTime = (uint32_t)t;
  nextSyncTime = (uint32_t)t + syncInterval;
  prevMillis = millis();  // restart counting from now (thanks to Korman for this fix)
}


//********************************************************************************************
// now
//********************************************************************************************
unsigned long now() {
  // calculate number of seconds passed since last call to now()
  while (millis() - prevMillis >= 1000) {
    // millis() and prevMillis are both unsigned ints thus the subtraction will always be the absolute value of the difference
    sysTime++;
    prevMillis += 1000;
  }
  if (nextSyncTime <= sysTime) {
    unsigned long  t = getNtpTime();
    if (t != 0) {
      if (Settings.DST)
        t += SECS_PER_HOUR; // add one hour if DST active
      setTime(t);
    } else {
      nextSyncTime = sysTime + syncInterval;
    }
  }
  breakTime(sysTime, tm);
  return (unsigned long)sysTime;
}


//********************************************************************************************
// return hour as int
//********************************************************************************************
int hour()
{
  return tm.Hour;
}


//********************************************************************************************
// return minute as int
//********************************************************************************************
int minute()
{
  return tm.Minute;
}


//********************************************************************************************
// return seconds as int
//********************************************************************************************
byte second()
{
  return tm.Second;
}


//********************************************************************************************
// return weekday as int
//********************************************************************************************
int weekday()
{
  return tm.Wday;
}


//********************************************************************************************
// start time handling
//********************************************************************************************
void initTime()
{
  nextSyncTime = 0;
  now();
}


//********************************************************************************************
// periodic time update, create clock events
//********************************************************************************************
void checkTime()
{
  now();
  if (tm.Minute != PrevMinutes)
  {
    PrevMinutes = tm.Minute;
    if (Settings.RulesClock)
    {
      String weekDays = F("AllSunMonTueWedThuFriSat");
      String event = F("Clock#Time=");
      event += weekDays.substring(weekday() * 3, weekday() * 3 + 3);
      event += ",";
      if (hour() < 10)
        event += "0";
      event += hour();
      event += ":";
      if (minute() < 10)
        event += "0";
      event += minute();
      #if FEATURE_RULES
        rulesProcessing(FILE_RULES, event);
      #endif
    }
  }
}


//********************************************************************************************
// Convert a string like "Sun,12:30" into a 32 bit integer
//********************************************************************************************
unsigned long string2TimeLong(const String &str)
{
  // format 0000WWWWAAAABBBBCCCCDDDD
  // WWWW=weekday, AAAA=hours tens digit, BBBB=hours, CCCC=minutes tens digit DDDD=minutes

  #define TmpStr1Length 10
  char command[20];
  char TmpStr1[TmpStr1Length];
  int w, x, y;
  unsigned long a;
  {
    // Within a scope so the tmpString is only used for copy.
    String tmpString(str);
    tmpString.toLowerCase();
    tmpString.toCharArray(command, 20);
  }
  unsigned long lngTime = 0;

  if (GetArgv(command, TmpStr1, 1))
  {
    String day = TmpStr1;
    String weekDays = F("allsunmontuewedthufrisatwrkwkd");
    y = weekDays.indexOf(TmpStr1) / 3;
    if (y == 0)
      y = 0xf; // wildcard is 0xf
    lngTime |= (unsigned long)y << 16;
  }

  if (GetArgv(command, TmpStr1, 2))
  {
    y = 0;
    for (x = strlen(TmpStr1) - 1; x >= 0; x--)
    {
      w = TmpStr1[x];
      if ( (w >= '0' && w <= '9') || w == '*')
      {
        a = 0xffffffff  ^ (0xfUL << y); // create mask to clean nibble position y
        lngTime &= a; // maak nibble leeg
        if (w == '*')
          lngTime |= (0xFUL << y); // fill nibble with wildcard value
        else
          lngTime |= (w - '0') << y; // fill nibble with token
        y += 4;
      }
      else
        if (w == ':');
      else
      {
        break;
      }
    }
  }
  #undef TmpStr1Length
  return lngTime;
}


//********************************************************************************************
// Try to get time from an NTP server
//********************************************************************************************
unsigned long getNtpTime()
{
  WiFiUDP udp;
  udp.begin(123);
  for (byte x = 1; x < 4; x++)
  {
    #if SERIALDEBUG
      Serial.print("Get NTP Time try:");
      Serial.println(x);
    #endif

    const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
    byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

    IPAddress timeServerIP;
    const char* ntpServerName = "pool.ntp.org";
    WiFi.hostByName(ntpServerName, timeServerIP);

    while (udp.parsePacket() > 0) ; // discard any previously received packets

    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    packetBuffer[12]  = 49;
    packetBuffer[13]  = 0x4E;
    packetBuffer[14]  = 49;
    packetBuffer[15]  = 52;
    udp.beginPacket(timeServerIP, 123); //NTP requests are to port 123
    udp.write(packetBuffer, NTP_PACKET_SIZE);
    udp.endPacket();

    uint32_t beginWait = millis();
    while (millis() - beginWait < 2000) {
      int size = udp.parsePacket();
      if (size >= NTP_PACKET_SIZE) {
        udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
        unsigned long secsSince1900;
        // convert four bytes starting at location 40 to a long integer
        secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
        secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
        secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
        secsSince1900 |= (unsigned long)packetBuffer[43];
        #if SERIALDEBUG
          Serial.print("Got NTP Time:");
          Serial.println(secsSince1900);
        #endif
        return secsSince1900 - 2208988800UL + Settings.TimeZone * SECS_PER_MIN;
      }
    }
  }
  return 0;
}
#endif

