Project: Recognize the postman with ProbeRequests
 
If you have configured several WiFi networks on your smartphone (home, work, friends, cafes, ...) and leave the house, the device constantly asks whether these networks can be reached in the area. If so, it connects. Most of the time, however, you are on the road and far away from home - then you continue to ask cheerfully. And for all entries. So if you were in the hotel WiFi during your last vacation, then it also asks for this network in the pedestrian zone in Germany. These requests are called sample requests. The same thing happens when the DHL delivery man is at your front door with his handheld. These devices also have WiFi and are constantly asking for this. This is how we can recognize them. Cool, isn't it?

So what if the DHL employee was on the way to the front door and we already knew. Before he rings! For this we listen with a Raspberry Pi for sample requests from nearby devices. So if someone calls for their DHL network, it is most likely not their neighbor. So we know that the postman is approaching and can act.

With this data you can do a lot of nonsense. Track people, create motion profiles and much more. Please adhere to the local laws for storing and evaluating personal data. I am not a lawyer and cannot tell you whether the procedure is legal. After all, the data is simply sent around unencrypted. In the United States, it is common practice for companies to use and analyze this data (for example, to create motion profiles in retail stores).

What you need?

Reference Links : 

https://www.youtube.com/watch?v=eLZb8VvX9aE&list=FLgxA0uNGRdFmcM-iASEkiHg&index=2&t=0s
https://github.com/HarringayMakerSpace/sonoff-adhoc/issues/1



commands
Before you do anything, the system should be up to date:

- sudo apt-get update
- sudo apt-get upgrade
- sudo rpi-update

Since there are several packages on GitHub, we need git on the system to load them.

- sudo apt-get install git

Then we start with a few dependencies, which we will need later.

- git clone https://github.com/drkjam/netaddr
- cd netaddr
- sudo python setup.py install
- cd ..

- git clone https://github.com/secdev/scapy.git
- cd scapy
- sudo python setup.py install
- cd ..

Now it goes on with Probemon and Aircrack-NG (the second one is not necessary, but is a cool tool).

- git clone https://github.com/nikharris0/probemon.git
- sudo apt-get install aircrack-ng
- ifconfig
- sudo ifconfig wlan0 down
- sudo airmon-ng start wlan0

Build new drivers for the interface
Since I wanted to use the internal WiFi module of the Raspberry Pi, I had to build a new driver with nexmon , since the standard driver does not support monitoring mode. Sounds easy at first, but requires a whole bunch of individual steps.

Here I have documented the commands from the video. The best thing to do is to follow the instructions in the wiki.

###These commands are for kernel 4.14 and a Raspberry Pi Zero W

- sudo su
- cd /usr/local/src

- wget  -O re4son-kernel_current.tar.xz https://re4son-kernel.com/download/re4son-kernel-current/

- tar -xJf re4son-kernel_current.tar.xz

- cd re4son-kernel_4*

- ./install.sh

(answer Y to a few prompts)

(reboots)

- sudo iw phy phy0 interface add mon0 type monitor

- sudo ifconfig mon0 up

- sudo apt-get install tcpdump

(now tracking my device 6c:96:cf:db:2f:77)

- sudo tcpdump -i mon0 -e -s 0 type mgt subtype probe-req and ether host 6c:96:cf:db:2f:77

These commands are for kernel 4.14 and a Raspberry Pi 3 (NOT 3B +)

- sudo su
- apt install raspberrypi-kernel-headers git libgmp3-dev gawk qpdf bison flex make
- git clone https://github.com/seemoo-lab/nexmon.git
- cd nexmon
- file /usr/lib/arm-linux-gnueabihf/libisl.so.10
- cd buildtools/isl-0.10
- ./configure
- make
- make install
- ln -s /usr/local/lib/libisl.so /usr/lib/arm-linux-gnueabihf/libisl.so.10
- cd ../..
- source setup_env.sh
- make
- cd patches/bcm43430a1/7_45_41_46/nexmon/
- wget https://raw.githubusercontent.com/notro/rpi-source/master/rpi-source -O /usr/bin/rpi-source && chmod +x /usr/bin/rpi-source && /usr/bin/rpi-source -q --tag-update
- apt-get install bc libncurses5-dev
rpi-source


make
make backup-firmware
make install-firmware

cd utilities/nexutil/
make
make install

modinfo brcmfmac
mv /lib/modules/4.14.92-v7+/kernel/drivers/net/wireless/broadcom/brcm80211/brcmfmac/brcmfmac.ko /lib/modules/4.14.92-v7+/kernel/drivers/net/wireless/broadcom/brcm80211/brcmfmac/brcmfmac.ko.orig
cp /home/pi/python/nexmon/patches/bcm43430a1/7_45_41_46/nexmon/brcmfmac_4.14.y-nexmon/brcmfmac.ko /lib/modules/4.14.92-v7+/kernel/drivers/net/wireless/broadcom/brcm80211/brcmfmac/brcmfmac.ko
depmod -a

reboot
sudo su

Next, a monitoring device is created, which can then be used to receive sample requests. Supposedly you can even use the Monitoring Mode and the normal WiFi interface in parallel. But I didn't test it because my Raspberry Pi is hanging from the cable in the basement.

iw phy `iw dev wlan0 info | gawk '/wiphy/ {printf "phy" $2}'` interface add mon0 type monitor
ifconfig mon0 up

exit
cd ~/python

Now install tcpdump and test whether everything works.

sudo apt-get install tcpdump

ifconfig
sudo tcpdump -i mon0
sudo tcpdump -i mon0 -e -s 256 type mgt subtype probe-req

sudo python probemon.py -i mon0 --mac-info --ssid --log
Now install the dependencies for my variant (via MQTT) and exchange the Probemon:

sudo pip install paho-mqtt

cd ..
rm -rf probemon
git clone https://github.com/klein0r/probemon.git
Then the whole thing can be tested. Please adjust MQTT broker, user and password.

sudo python probemon.py -i mon0 --mac-info --ssid --log --mqtt-broker 192.168.44.11 --mqtt-user raspberry --mqtt-password xxx --mqtt-topic /SmartHome/Interface/WiFi/ProbeRequest

If everything goes well, I'll take care of the auto start. A new file is created for this.

chmod +x probemon.py
sudo vi /lib/systemd/system/probemon.service

This gets this content (theoretically it would be nicer to move the whole thing from the home directory of the pi user to another place beforehand. But that's how it works.)

Please remove –log as a parameter. We do not need to spend on stdout

[Unit]
Description=Probemon MQTT Service

[Service]
ExecStart=/home/pi/python/probemon/probemon.py -i mon0 --mac-info --ssid --mqtt-broker 192.168.44.11 --mqtt-user raspberry --mqtt-password xxx --mqtt-topic /SmartHome/Interface/WiFi/ProbeRequest
StandardOutput=null

[Install]
WantedBy=multi-user.target
Alias=probemon.service
Then activate and start the new service.

sudo systemctl enable probemon.service
sudo systemctl start probemon.service
Finished. Now you should receive regular messages via MQTT with all data.

NodeRed
In NodeRed I built a flow that looks at the configured topic to see if new messages are coming in. Then it is checked which SSID is asked for. If it contains DHL, DPD, GLS or UPS, I send myself a push notification.

Here is my sample flow:

[{"id":"fee8da92.ffe228","type":"subflow","name":"An FHEM","info":"","in":[{"x":320,"y":140,"wires":[{"id":"7d4eb671.f86aa8"}]}],"out":[]},{"id":"7d4eb671.f86aa8","type":"mqtt out","z":"fee8da92.ffe228","name":"An FHEM","topic":"/Service/fhem/cmnd","qos":"","retain":"","broker":"afa97030.18184","x":460,"y":140,"wires":[]},{"id":"9ac0b213.4b60d","type":"tab","label":"WiFiProbes","disabled":false,"info":""},{"id":"af7ec61a.250718","type":"mqtt in","z":"9ac0b213.4b60d","name":"","topic":"/SmartHome/Interface/WiFi/ProbeRequest","qos":"2","broker":"afa97030.18184","x":180,"y":140,"wires":[["e20cd860.836888"]]},{"id":"e20cd860.836888","type":"json","z":"9ac0b213.4b60d","name":"","property":"payload","action":"","pretty":false,"x":440,"y":140,"wires":[["8df8bce5.be216"]]},{"id":"8df8bce5.be216","type":"switch","z":"9ac0b213.4b60d","name":"Welcher Dienst","property":"payload.ssid","propertyType":"msg","rules":[{"t":"cont","v":"DHL","vt":"str"},{"t":"cont","v":"UPS","vt":"str"},{"t":"cont","v":"DPD","vt":"str"},{"t":"cont","v":"GLS","vt":"str"},{"t":"else"}],"checkall":"false","repair":false,"outputs":5,"x":610,"y":140,"wires":[["275d4978.731266"],["2a31156a.f23f5a"],["86fc14c5.edc9c8"],["77d835b5.45b92c"],[]]},{"id":"275d4978.731266","type":"trigger","z":"9ac0b213.4b60d","op1":"DHL","op2":"false","op1type":"str","op2type":"bool","duration":"60","extend":true,"units":"s","reset":"","bytopic":"all","name":"DHL","x":800,"y":80,"wires":[["2ef92fab.3ea27"]]},{"id":"2a31156a.f23f5a","type":"trigger","z":"9ac0b213.4b60d","op1":"UPS","op2":"false","op1type":"str","op2type":"bool","duration":"60","extend":true,"units":"s","reset":"","bytopic":"all","name":"UPS","x":800,"y":120,"wires":[["2ef92fab.3ea27"]]},{"id":"86fc14c5.edc9c8","type":"trigger","z":"9ac0b213.4b60d","op1":"DPD","op2":"false","op1type":"str","op2type":"bool","duration":"60","extend":true,"units":"s","reset":"","bytopic":"all","name":"DPD","x":800,"y":160,"wires":[["2ef92fab.3ea27"]]},{"id":"77d835b5.45b92c","type":"trigger","z":"9ac0b213.4b60d","op1":"GLS","op2":"false","op1type":"str","op2type":"bool","duration":"60","extend":true,"units":"s","reset":"","bytopic":"all","name":"GLS","x":800,"y":200,"wires":[["2ef92fab.3ea27"]]},{"id":"2ef92fab.3ea27","type":"switch","z":"9ac0b213.4b60d","name":"Neuer Dienst","property":"payload","propertyType":"msg","rules":[{"t":"false"},{"t":"else"}],"checkall":"false","repair":false,"outputs":2,"x":980,"y":140,"wires":[[],["8b5e750b.842698"]]},{"id":"bf1000dc.99902","type":"subflow:fee8da92.ffe228","z":"9ac0b213.4b60d","x":1320,"y":160,"wires":[]},{"id":"8b5e750b.842698","type":"function","z":"9ac0b213.4b60d","name":"FHEM-CMD","func":"msg.payload = \"msg push @rr_Matthias |Paketdienst| \" + msg.payload + \" ist jetzt da!\"\nreturn msg;","outputs":1,"noerr":0,"x":1170,"y":160,"wires":[["bf1000dc.99902"]]},{"id":"afa97030.18184","type":"mqtt-broker","z":"","name":"","broker":"mqtt","port":"1883","clientid":"nodered","usetls":false,"compatmode":true,"keepalive":"60","cleansession":true,"willTopic":"/Service/nodered/status","willQos":"0","willPayload":"crashed","birthTopic":"/Service/nodered/status","birthQos":"0","birthPayload":"started"}]
