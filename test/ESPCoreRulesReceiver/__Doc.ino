/* List of core commands
Config,AutoConnect,<0|1>                            Enable/Disable Auto connect as Wifi client during boot
Config,AutoAPOnFailure,<0|1>                        Enable/Disable Auto launch AP on Wifi client connect failure
Config,DST,<0|1>                                    Enable/Disable DST
Config,Baudrate,<speed>                             Enable/Disable Serial and speed (speed 0=disable)
Config,LogEvents,<0|1>                              Enable/Disable logging msbus events on telnet session
Config,I2C,<sda>,<scl>                              Enable I2c and optionally set custom pins for scl/sda
Config,Name                                         Set the device name
Config,Network,<ip>,<mask>,<gateway>,<DNS>          Set static IP config
Config,NodeListMax                                  Max number of nodes supported in the Nodelist, 0 = disabled
Config,Plugins,<nr>,<nr>,<nr>,...                   Only enable selected list of plugins
Config,Rules,Clock,<0|1>                            Enable/Disable rules processing on clock events (each minute)
Config,Rules,Serial,<0|1>                           Enable/Disable rules processing on serial input
Config,SendARP,<0|1>                                Enable/Disable grat ARP broadcast
Config,Time,<0|1>                                   Enable/Disable NTP time service client
Config,Timezone,<offset>                            Timezone offset in minutes (can also be negative)
Config,TimerMax                                     Max number of timers supported, 0 = disabled
Config,UserVarNumericMax                            Max number of numeric user variables, 0 = disabled
Config,UserVarStringMax                             Max number of string user variables, 0 = disabled
Config,WifiSSID,<ssid>                              Set the Wifi SSID
Config,WifiKey,<key>                                Set the Wifi WPA key
Config,WifiAPKey,<key>                              Set the Wifi AP key
DeepSleep <seconds)                                 Turn off to deepsleep mode for <seconds> (max 4294 seconds). Only RTC will be active. Connect GPIO16 to Reset pin
Delay <milliseconds>                                Stop processing for <milliseconds> (blocking command!, use only for small delays)
Event <event>                                       Generate event
ParseFromJSON <varname>,<name>,<jsonString>         Retrieve a name/value pair from a basic JSON object, store into <varname>
Reboot                                              Reboot device
SerialFloat                                         Makes the serial pins float
Serial                                              Enables serial (after using float)
SerialSend <string>                                 Sends a text to serial port
SerialTelnet <0|1>                                  Enable/Disable Serial/Telnet bridge
SendToUDP <ip>,<port>,<msg>                         Sends a text string to give IP and port on UDP protocol
Settings                                            Outputs some basic settings to serial port
StringSet <name>,<text>                             Set a variable by name to string value
StringLength <name>,<source var>                    Gets the string length, store into numberic var by <name>
StringReplace <name>,<replace>,<with>               Replace a string pattern within a string var with another string pattern
StringSubstring <name>,<source var>,<start>,<end>   Get a substring from another string var. Start can be numeric of a string to match
Syslog <string>                                     Sends a text string to syslog using broadcast
TimerSet <name>,<seconds>                           Set a timer by name to x seconds countdown
ValueSet <name>,<value>                             Set a variable by name to float value
VCCRead <variable>                                  Read VCC voltage (when feature for ADC to read VCC is enabled)
WebPrint <html>                                     Print html to the main webpage (no pararameter clears the buffer)
WebButton <class>;<href>;<caption>                  Adds a button to the main webpage

Plugins commands are explained in the plugin sections


Variables, to be used in rules:
%event%                   Full eventstring
%eventvalue%              Value from an event, everything that comes after the = token
%eventtopic%              In case of json event, the topic part (before the = token)
%eventpayload%            In case of json event, the payload itself (after the = token)
%group%                   Group name that has been configured
%mac%                     Wifi Station MAC address
%millis%                  Uptime in milliseconds, returns internal millis() counter, overflows in 49 days
%sysname%                 System name that has been configured
%systime%                 Time in HH:MM
*/

