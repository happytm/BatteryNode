//********************************************************************************
// Web Interface init
//********************************************************************************
void WebServerInit()
{
  WebServer.on("/", handle_root);
  WebServer.on("/edit", handle_edit);
  WebServer.on("/tools", handle_tools);
  WebServer.on("/filelist", handle_filelist);
  WebServer.on("/upload", HTTP_GET, handle_upload);
  WebServer.on("/upload", HTTP_POST, handle_upload_post, handleFileUpload);
  WebServer.onNotFound(handleNotFound);
  #if defined(ESP8266)
    httpUpdater.setup(&WebServer);
  #endif

  WebServer.begin();
}


//********************************************************************************
// Web page header
//********************************************************************************
void addHeader(boolean showMenu, String & reply) {
  reply += F("<meta name=\"viewport\" content=\"width=width=device-width, initial-scale=1\">");
  reply += F("<STYLE>* {font-family:sans-serif; font-size:12pt;}");
  reply += F("h1 {font-size: 16pt; color: #07D; margin: 8px 0; font-weight: bold;}");
  reply += F(".button {margin:4px; padding:5px 15px; background-color:#07D; color:#FFF; text-decoration:none; border-radius:4px}");
  reply += F(".button-link {padding:5px 15px; background-color:#07D; color:#FFF; border:solid 1px #FFF; text-decoration:none}");
  reply += F(".button-widelink {display: inline-block; width: 100%; text-align: center; padding:5px 15px; background-color:#07D; color:#FFF; border:solid 1px #FFF; text-decoration:none}");
  reply += F(".button-nodelink {display: inline-block; width: 100%; text-align: center; padding:5px 15px; background-color:#888; color:#FFF; border:solid 1px #FFF; text-decoration:none}");
  reply += F(".button-nodelinkA {display: inline-block; width: 100%; text-align: center; padding:5px 15px; background-color:#28C; color:#FFF; border:solid 1px #FFF; text-decoration:none}");
  reply += F("td {padding:7px;}");
  reply += F("</STYLE>");
  reply += F("<h1>");
  reply += Settings.Name;
  reply += F("</h1>");
  reply += F("<a class=\"button-link\" href=\"/\">Main</a>");
  reply += F("<a class=\"button-link\" href=\"/edit?file=");
  reply += F(FILE_BOOT);
  reply += F("\">Boot</a>");
  reply += F("<a class=\"button-link\" href=\"/edit?file=");
  reply += F(FILE_RULES);
  reply += F("\">Rules</a>");
  reply += F("<a class=\"button-link\" href=\"/tools\">Tools</a>");
  reply += F("<BR><BR>");
}


//********************************************************************************
// Web Interface root page
//********************************************************************************
void handle_root() {

  String sCommand = WebServer.arg(F("cmd"));
  String group = WebServer.arg("group");
  boolean groupList = true;

  if (group != "")
    groupList = false;

  if (strcasecmp_P(sCommand.c_str(), PSTR("reboot")) != 0)
  {
    String reply = "";
    addHeader(true, reply);

    if (sCommand.length() > 0)
      ExecuteCommand(sCommand.c_str());

    #if FEATURE_RULES
      String event = F("Web#Print");
      rulesProcessing(FILE_RULES, event);
      reply += printWebString;
    #endif
    
    reply += F("<form><table>");

    if(Settings.NodeListMax){
      // first get the list in alphabetic order
      for (byte x = 0; x < Settings.NodeListMax; x++)
        sortedIndex[x] = x;

      if (groupList == true) {
        // Show Group list
        sortDeviceArrayGroup(); // sort on groupname
        String prevGroup = "?";
        for (byte x = 0; x < Settings.NodeListMax; x++)
        {
          byte index = sortedIndex[x];
          if (Nodes[index].IP[0] != 0) {
            String group = *Nodes[index].group;
            if (group != prevGroup)
            {
              prevGroup = group;
              reply += F("<TR><TD><a class=\"");
              reply += F("button-nodelink");
              reply += F("\" ");
              reply += F("href='/?group=");
              reply += group;
              reply += "'>";
              reply += group;
              reply += F("</a>");
              reply += F("<TD>");
            }
          }
        }
        // All nodes group button
        reply += F("<TR><TD><a class=\"button-nodelink\" href='/?group=*'>_ALL_</a><TD>");
      }
      else {
        // Show Node list
        sortDeviceArray();  // sort on nodename
        for (byte x = 0; x < Settings.NodeListMax; x++)
        {
          byte index = sortedIndex[x];
          if (Nodes[index].IP[0] != 0 && (group == "*" || *Nodes[index].group == group))
          {
            String buttonclass = "";
            if ((String)Settings.Name == *Nodes[index].nodeName)
              buttonclass = F("button-nodelinkA");
            else
              buttonclass = F("button-nodelink");
            reply += F("<TR><TD><a class=\"");
            reply += buttonclass;
            reply += F("\" ");
            char url[40];
            sprintf_P(url, PSTR("href='http://%u.%u.%u.%u"), Nodes[index].IP[0], Nodes[index].IP[1], Nodes[index].IP[2], Nodes[index].IP[3]);
            reply += url;
            if (group != "") {
              reply += F("?group=");
              reply += *Nodes[index].group;
            }
            reply += "'>";
            reply += *Nodes[index].nodeName;
            reply += F("</a>");
            reply += F("<TD>");
          }
        }
      }
    }
    reply += "</table></form>";
    WebServer.send(200, "text/html", reply);
  }
  else
  {
    // have to disconnect or reboot from within the main loop
    // because the webconnection is still active at this point
    // disconnect here could result into a crash/reboot...
    if (strcasecmp_P(sCommand.c_str(), PSTR("reboot")) == 0)
    {
      cmd_within_mainloop = CMD_REBOOT;
    }
    WebServer.send(200, "text/html", "OK");
    delay(100);
  }
}


//********************************************************************************
// Web Tools page
//********************************************************************************
void handle_tools() {

  String webrequest = WebServer.arg("cmd");

  String reply = "";
  addHeader(true, reply);

  reply += F("<form><table>");

  reply += F("<TR><TD>Command<TD>");
  reply += F("<input type='text' name='cmd' value='");
  reply += webrequest;
  reply += F("'><TR><TD><TD><input class=\"button-widelink\" type='submit' value='Submit'>");
  if (webrequest.length() > 0)
    ExecuteCommand(webrequest.c_str());
  
  reply += F("<TR><TD>File System<TD><a class=\"button-widelink\" href=\"/filelist\">Files</a>");
  #if defined(ESP8266)
    reply += F("<TR><TD>Update Firmware<TD><a class=\"button-widelink\" href=\"/update\">Firmware</a>");
  #endif
  reply += F("<TR><TD>Reboot System<TD><a class=\"button-widelink\" href=\"/?cmd=reboot\">Reboot</a>");

  reply += F("<TR><TR><TD>RSSI:<TD>");
  reply += WiFi.RSSI();
  reply += F(" dB");

  reply += F("<TR><TD>Board:<TD>");
  reply += ARDUINO_BOARD;

  #if defined(ESP8266)
    reply += F("<TR><TD>FlashSize:<TD>");
    reply += ESP.getFlashChipRealSize() / 1024;
    reply += F(" kB");

    reply += F(" (Free:");
    reply += ESP.getFreeSketchSpace() / 1024;
    reply += F(" kB)");
  #endif

  #if defined(ESP8266)
    uint32_t _start = ((uint32_t)&_SPIFFS_start - 0x40200000)/1024;
    uint32_t _end = ((uint32_t)&_SPIFFS_end - 0x40200000)/1024;
    reply += F("<TR><TD>SPIFFS:<TD>");
    reply += _start;
    reply += " / ";
    reply += _end;
    reply += " (";
    reply += _end - _start;
    reply += ") kB";
  #endif
  #if defined(ESP32)
    reply += F("<TR><TD>Data partition:<TD>");
    esp_partition_type_t partitionType = static_cast<esp_partition_type_t>(ESP_PARTITION_TYPE_DATA);
    esp_partition_iterator_t _mypartiterator = esp_partition_find(partitionType, ESP_PARTITION_SUBTYPE_ANY, NULL);
    if (_mypartiterator) {
      do {
        const esp_partition_t *_mypart = esp_partition_get(_mypartiterator);
        reply += _mypart->label;
        reply += " : ";
        reply += _mypart->address/1024;
        reply += " (";
        reply += _mypart->size/1024;
        reply += ") kB<BR>";
      } while ((_mypartiterator = esp_partition_next(_mypartiterator)) != NULL);
    }
    esp_partition_iterator_release(_mypartiterator);
  #endif
   
  reply += F("<TR><TD>Core:<TD>");
  String core = getSystemLibraryString();
  core.replace(",","<BR>");
  reply += core;

  reply += F("<TR><TD>Free RAM:<TD>");
  reply += ESP.getFreeHeap();

  reply += F("<TR><TD>MAC:<TD>");
  reply += WiFi.macAddress();

  #if FEATURE_TIME
    reply += F("<TR><TD>System Time:<TD>");
    reply += getTimeString(':');
  #endif

  reply += F("<TR><TD>Uptime:<TD>");
  reply += uptime;

  reply += F("<TR><TD>Build:<TD>");
  reply += BUILD;
  reply += F(BUILD_NOTES);

  reply += F("<TR><TD>LoopCount:<TD>");
  reply += loopCounterLast/60;

  reply += F("<TR><TD>Features:<TD>");
  #if FEATURE_RULES
    reply += F("<TR><TD><TD>Rules");
  #endif
  #if FEATURE_TIME
    reply += F("<TR><TD><TD>Time");
  #endif
  #if FEATURE_I2C
    reply += F("<TR><TD><TD>I2C");
  #endif
  #if FEATURE_ESPNOW
    reply += F("<TR><TD><TD>ESPNOW");
  #endif

  // Start a new table for the plugin list
  reply += F("</table><table>");

  #if FEATURE_PLUGINS
    reply += F("<TR><TD>Plugins<TD>Build<TD>Enabled");
    printWebTools = "";
    PluginCall(PLUGIN_INFO, dummyString,  dummyString);
    reply += printWebTools;
    printWebTools = "";
  #endif

  reply += F("</table></form>");
  WebServer.send(200, "text/html", reply);
}


//********************************************************************************
// Web Interface handle other requests
//********************************************************************************
void handleNotFound() {

  if (loadFromFS(true, WebServer.uri())) return;
  #ifdef FEATURE_SD
    if (loadFromFS(false, WebServer.uri())) return;
  #endif
  String message = F("URI: ");
  message += WebServer.uri();
  message += "\nMethod: ";
  message += (WebServer.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += WebServer.args();
  message += "\n";
  for (uint8_t i = 0; i < WebServer.args(); i++) {
    message += " NAME:" + WebServer.argName(i) + "\n VALUE:" + WebServer.arg(i) + "\n";
  }
  WebServer.send(404, "text/plain", message);
}

//********************************************************************************
// Web Interface server web file from SPIFFS
//********************************************************************************
bool loadFromFS(boolean spiffs, String path) {

  String dataType = F("text/plain");
  if (path.endsWith("/")) path += F("index.htm");

  if (path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if (path.endsWith(".htm")) dataType = F("text/html");
  else if (path.endsWith(".css")) dataType = F("text/css");
  else if (path.endsWith(".js")) dataType = F("application/javascript");
  else if (path.endsWith(".png")) dataType = F("image/png");
  else if (path.endsWith(".gif")) dataType = F("image/gif");
  else if (path.endsWith(".jpg")) dataType = F("image/jpeg");
  else if (path.endsWith(".ico")) dataType = F("image/x-icon");
  else if (path.endsWith(".txt")) dataType = F("application/octet-stream");
  else if (path.endsWith(".dat")) dataType = F("application/octet-stream");
  else if (path.endsWith(".esp")) return handle_custom(path);

  #if defined(ESP8266)
    path = path.substring(1);
  #endif
  
  if (spiffs)
  {
    fs::File dataFile = SPIFFS.open(path.c_str(), "r");
    if (!dataFile)
      return false;

    //prevent reloading stuff on every click
    WebServer.sendHeader("Cache-Control", "max-age=3600, public");
    WebServer.sendHeader("Vary", "*");
    WebServer.sendHeader("ETag", "\"2.0.0\"");

    if (path.endsWith(".dat"))
      WebServer.sendHeader("Content-Disposition", "attachment;");
    WebServer.streamFile(dataFile, dataType);
    dataFile.close();
  }
  else
  {
  #ifdef FEATURE_SD
    File dataFile = SD.open(path.c_str());
    if (!dataFile)
      return false;
    if (path.endsWith(".DAT"))
      WebServer.sendHeader("Content-Disposition", "attachment;");
    WebServer.streamFile(dataFile, dataType);
    dataFile.close();
  #endif
  }
  return true;
}

//********************************************************************************
// Web Interface custom page handler
//********************************************************************************
boolean handle_custom(String path) {

  path = path.substring(1);
  String reply = "";

  // handle commands from a custom page
  String webrequest = WebServer.arg(F("cmd"));
  if (webrequest.length() > 0 ) {
    ExecuteCommand(webrequest.c_str());
  }

  // create a dynamic custom page
  fs::File dataFile = SPIFFS.open(path.c_str(), "r");
  if (dataFile)
  {
    String page = "";
    page.reserve(dataFile.size());
    while (dataFile.available())
      page += ((char)dataFile.read());

    reply += parseTemplate(page, 0);
    dataFile.close();
  }
  else
    return false; // unknown file that does not exist...

  WebServer.send(200, "text/html", reply);
  return true;
}

//********************************************************************************
// Device Sort routine, switch array entries
//********************************************************************************
void switchArray(byte value)
{
  byte temp;
  temp = sortedIndex[value - 1];
  sortedIndex[value - 1] = sortedIndex[value];
  sortedIndex[value] = temp;
}


//********************************************************************************
// Device Sort routine, compare two array entries
//********************************************************************************
boolean arrayLessThan(const String& ptr_1, const String& ptr_2)
{
  unsigned int i = 0;
  while (i < ptr_1.length())    // For each character in string 1, starting with the first:
  {
    if (ptr_2.length() < i)    // If string 2 is shorter, then switch them
    {
      return true;
    }
    else
    {
      const char check1 = (char)ptr_1[i];  // get the same char from string 1 and string 2
      const char check2 = (char)ptr_2[i];
      if (check1 == check2) {
        // they're equal so far; check the next char !!
        i++;
      } else {
        return (check2 > check1);
      }
    }
  }
  return false;
}


//********************************************************************************
// Device Sort routine, actual sorting
//********************************************************************************
void sortDeviceArray()
{
  int innerLoop ;
  int mainLoop ;
  for ( mainLoop = 1; mainLoop < Settings.NodeListMax; mainLoop++)
  {
    innerLoop = mainLoop;
    while (innerLoop  >= 1)
    {
      String one = *Nodes[sortedIndex[innerLoop]].nodeName;
      String two = *Nodes[sortedIndex[innerLoop - 1]].nodeName;
      if (arrayLessThan(one, two))
      {
        switchArray(innerLoop);
      }
      innerLoop--;
    }
  }
}


//********************************************************************************
// Device Sort routine, actual sorting
//********************************************************************************
void sortDeviceArrayGroup()
{
  int innerLoop ;
  int mainLoop ;
  for ( mainLoop = 1; mainLoop < Settings.NodeListMax; mainLoop++)
  {
    innerLoop = mainLoop;
    while (innerLoop  >= 1)
    {
      String one = *Nodes[sortedIndex[innerLoop]].group;
      if(one.length()==0) one = "_";
      String two = *Nodes[sortedIndex[innerLoop - 1]].group;
      if(two.length()==0) two = "_";
      if (arrayLessThan(one, two))
      {
        switchArray(innerLoop);
      }
      innerLoop--;
    }
  }
}


//********************************************************************************
// Web Interface file list
//********************************************************************************
void handle_filelist() {

  String fdelete = WebServer.arg(F("delete"));

  if (fdelete.length() > 0)
  {
    SPIFFS.remove(fdelete);
  }

  String reply = "";
  addHeader(true, reply);
  reply += F("<table border=1px frame='box' rules='all'><TH><TH>Filename:<TH>Size<TH>");

#if defined(ESP8266)
  Dir dir = SPIFFS.openDir("");
  while (dir.next())
  {
    reply += F("<TR><TD>");
    reply += F("<a class=\"button-link\" href=\"edit?file=");
    reply += dir.fileName();
    reply += F("\">Edit</a>");
    reply += F("<TD><a href=\"");
    reply += dir.fileName();
    reply += F("\">");
    reply += dir.fileName();
    reply += F("</a>");
    File f = dir.openFile("r");
    reply += F("<TD>");
    reply += f.size();
    reply += F("<TD>");
    if (dir.fileName() != FILE_BOOT)
    {
      reply += F("<a class=\"button-link\" href=\"filelist?delete=");
      reply += dir.fileName();
      reply += F("\">Del</a>");
    }
  }
#endif

#if defined(ESP32)
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file)
  {
    if (!file.isDirectory()) {
      reply += F("<TR><TD>");
      reply += F("<a class=\"button-link\" href=\"edit?file=");
      reply += file.name();
      reply += F("\">Edit</a>");
      reply += F("<TD><a href=\"");
      reply += file.name();
      reply += F("\">");
      reply += file.name();
      reply += F("</a>");
      reply += F("<TD>");
      reply += file.size();
      reply += F("<TD>");
      if (file.name() != FILE_BOOT)
      {
        reply += F("<a class='button link' href=\"filelist?delete=");
        reply += file.name();
        reply += F("\">Del</a>");
      }
      file = root.openNextFile();
    }
  }
#endif

  reply += F("</table></form>");
  reply += F("<BR><a class='button link' href=\"/upload\">Upload</a>");
  WebServer.send(200, "text/html", reply);
}


//********************************************************************************
// Web Interface upload page
//********************************************************************************
byte uploadResult = 0;
void handle_upload() {

  String reply = "";
  addHeader(true, reply);
  reply += F("<form enctype=\"multipart/form-data\" method=\"post\"><p>Upload file:<br><input type=\"file\" name=\"datafile\" size=\"40\"></p><div><input class=\"button-link\" type='submit' value='Upload'></div><input type='hidden' name='edit' value='1'></form>");
  WebServer.send(200, "text/html", reply);
}


//********************************************************************************
// Web Interface upload page
//********************************************************************************
void handle_upload_post() {

  String reply = "";
  if (uploadResult == 1)
    reply += F("Upload OK");
  if (uploadResult == 2)
    reply += F("<font color=\"red\">No filename!</font>");

  addHeader(true, reply);
  reply += F("Upload finished");
  WebServer.send(200, "text/html", reply);
}


//********************************************************************************
// Web Interface upload handler
//********************************************************************************
File uploadFile;
void handleFileUpload() {
  static boolean valid = false;

  HTTPUpload& upload = WebServer.upload();

  if (upload.filename.c_str()[0] == 0)
  {
    uploadResult = 2;
    return;
  }

  if (upload.status == UPLOAD_FILE_START)
  {
    valid = false;
    uploadResult = 0;
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    // first data block, if this is the config file, check PID/Version
    if (upload.totalSize == 0)
    {
      String filename = upload.filename;
      #if defined(ESP32)
        filename = "/" + filename;
      #endif
      SPIFFS.remove((char *)filename.c_str());
      uploadFile = SPIFFS.open(filename.c_str(), "w");
    }
    if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (uploadFile) uploadFile.close();
  }

  uploadResult = 1;
}


//********************************************************************************
// File editor
//********************************************************************************
void handle_edit() {

  String reply = "";
  addHeader(true, reply);

  String fileName = WebServer.arg(F("file"));
  String content = WebServer.arg(F("content"));

  if (WebServer.args() >= 2) {
    if (content.length() > RULES_MAX_SIZE)
      reply += F("<span style=\"color:red\">Data was not saved, exceeds web editor limit!</span>");
    else
    {
      fs::File f = SPIFFS.open(fileName, "w");
      if (f)
      {
        f.print(content);
        f.close();
        logger->println(F("DBG: Flash Save"));
      }
    }
  }

  int size = 0;
  fs::File f = SPIFFS.open(fileName, "r+");
  if (f)
  {
    size = f.size();
    if (size > RULES_MAX_SIZE)
      reply += F("<span style=\"color:red\">Filesize exceeds web editor limit!</span>");
    else
    {
      reply += F("<form method='post'>");
      reply += F("<textarea name='content' rows='15' cols='80' wrap='off'>");
      while (f.available())
      {
        String c((char)f.read());
        htmlEscape(c);
        reply += c;
      }
      reply += F("</textarea>");
    }
    f.close();
  }

  reply += F("<br>Current size: ");
  reply += size;
  reply += F(" characters (Max ");
  reply += RULES_MAX_SIZE;
  reply += F(")");

  reply += F("<br><input class=\"button-link\" type='submit' value='Submit'>");
  reply += F("</form>");
  WebServer.send(200, "text/html", reply);
}


void htmlEscape(String & html)
{
  html.replace("&",  F("&amp;"));
  html.replace("\"", F("&quot;"));
  html.replace("'",  F("&#039;"));
  html.replace("<",  F("&lt;"));
  html.replace(">",  F("&gt;"));
  html.replace("/", F("&#047;"));
}

