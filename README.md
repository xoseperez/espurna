# ESPurna

ESPurna ("spark" in Catalan) is a custom C firmware for ESP8266 based smart switches. It was originally developed with the **[ITead Sonoff][1]** in mind. This device has an ESP8266 on board with a 8Mbit flash memory chip, a mains to 3V3 transformer and a relay (GPIO12). It also features a button (GPIO0), an LED (GPIO13) and an unpopulated header you can use to reprogram it.
You can read about this board and firmware in [my blog][2].

![Sonoff board - front view](/images/pinout_front.jpg)

![Sonoff board - back view](/images/pinout_back.jpg)

## Hardware support

* [ITead Sonoff TH][1]
* [ITead Sonoff RF][8]
* [ITead Slampher][9]
* [ITead S20 Smart Socket][10]
* [ITead Sonoff POW][15]
* Tinkerman ESPurna board

## Features

* **WebServer for configuration** and simple relay toggle
* **Flashing firmware Over-The-Air** (OTA)
* Up to **3 configurable WIFI networks**, connects to the strongest signal
* **MQTT support** with configurable host and topic
* Manual switch ON/OFF with button (single click the button)
* AP mode backup (double click the button)
* Visual status of the connection via the LED
* Support for **automatic over-the-air updates** through the [NoFUSS Library][6]
* Support for **DHT22** sensors
* Support for the **HLW8012** power sensors present in the Sonoff POW
* Support for **current monitoring** through then [EmonLiteESP Library][7] using a non-intrusive current sensor (requires some hacking)
* Command line configuration

## Installing

### Build the web config site

Normally when you flash an ESP8266 you only flash the firmware, like for any other microcontroller. But the ESP8266 has plenty of room and normally it is split into different partitions. One such partition is used to store web files like a normal webserver. In the "Flash your board" section below you'll know how to flash this special partition, but first we will have to build it.

The build process read the HTML files, looks for the stylesheet and script files linked there, grabs them, merges them, minifies them and compresses them. Thus, a single HTML with N linked scripts and M linked CSS files is transformed in just 1 file (index.html.gz). This way the ESP8266 webserver can serve them really fast. Mind the ESP8266 is just a microcontroller, the webserver has a very limited capacity to hold concurrent requests, so few and lighter files are a must.

To build these files we are using **[Gulp][11]**, a build system built in [node.js][13]. So you will need node (and [npm][14], its package manager) first. [Read the documentation][12] on how to install them.

Once you have node and npm installed, go to the 'code' folder and install all the dependencies with:

```
npm install
```

It will take a minute or two. Then you are ready to build the webserver files with:

```
gulp
```

It will create a populate a 'data' folder with all the required files.

### Build the firmware

The project is ready to be build using [PlatformIO][3].
Please refer to their web page for instructions on how to install the builder.

PlatformIO will take care of some of the library dependencies, but not all the required libraries are available in the platform library manager. Some dependencies are thus checked out as submodules in GIT. So the normal initial checkout should be:

```
git clone git@bitbucket.org:xoseperez/espurna.git
git submodule init
git submodule update
```

On linux/max systems the libraries are soft linked to the code/lib folder and you are ready to go. Windows systems do not have this feature so you will have to copy them manually like this (ONLY WINDOWS):

```
cd espurna/code
copy vendor/emonliteesp/code/lib/EmonLiteESP lib/EmonLiteESP
copy vendor/nofuss/client/lib/NoFUSSClient lib/NoFUSSClient
copy vendor/RemoteSwitch-arduino-library lib/RemoteSwitch
```

The Embedis library is the only one required in all cases. The other three are optional at the moment and will only be linked if you set ENABLE_NOFUSS, ENABLE_EMON or ENABLE_RF in the defaults.h file.

Once you have all the code, you can check if it's working by:

```bash
> platformio run -e node-debug
```

If it compiles you are ready to flash the firmware.

### Flash your board

*This section only applies to the Sonoff Basic, but pretty much every other ESP8266-based hardware will be similar.*

The unpopulated header in the Sonoff has all the required pins. My board has a 5 pins header in-line with the button. They are (from the button outwards):

* 3V3
* RX
* TX
* GND
* GPIO14

Last one is not necessary. Mind it's a **3V3 device**, if connected to 5V you will probably fry it. Button is connected to GPIO0 on the ESP8266 chip, so to enter flash mode you have to hold the button pressed while powering on the board, then you can realease it again.

Wire your board and flash the firmware (with ```upload```) and the file system (with ```uploadfs```):

```bash
> platformio run --target upload -e node-debug
> platformio run --target uploadfs -e node-debug
```

You might notice than when you call "uploadfs" target it automatically builds the files (does a "gulp buildfs"). This is done by the hook defined in the extra_script parameter of each environment.

Once you have flashed it you can flash it again over-the-air using the ```ota``` environment:

```bash
> platformio run --target upload -e node-debug-ota
> platformio run --target uploadfs -e node-debug-ota
```

When using OTA environment it defaults to the IP address of the device in SoftAP mode. If you want to flash it when connected to your home network best way is to supply the IP of the device:

```bash
> platformio run --target upload -e node-debug-ota --upload-port 192.168.1.151
> platformio run --target uploadfs -e node-debug-ota --upload-port 192.168.1.151
```

You can also use the automatic OTA update feature. Check the [NoFUSS library][6] for more info.

Library dependencies are automatically managed via PlatformIO Library Manager or included via submodules and linked from the "lib" folder.

## Usage

On normal boot (i.e. button not pressed) it will execute the firmware. It configures the hardware (button, LED, relay), the SPIFFS memory access, the WIFI, the WebServer and MQTT connection.

Obviously the default values for WIFI network and MQTT will probably not match your requirements. The device will start in Soft AP creating a WIFI SSID named "SONOFF_XXXXXX", where XXXXXX are the last 3 bytes of the radio MAC. Connect with phone, PC, laptop, whatever to that network, password is "fibonacci". Once connected
browse to 192.168.4.1. It will then present an authentication challenge. Default user and password are "admin" and "fibonacci" (again). Then you will be presented a configuration page where you will be able to define different configuration parameters, including changing the default password. The same password is used for the WIFI Access Point, for the web interface and for the OTA firmware upload.

You can configure up to three WIFI networks. It will then try to connect to the configure WIFI networks in order of signal strength. If none of the 3 attempts succeed it will default to SoftAP mode again. Once connected it will try to connect the MQTT server. You can also switch to SoftAP mode by double click the on board button or reset the board long clicking it.

The device will publish the relay state to the given topic and it will subscribe to the same topic for remote switching. Don't worry, it avoids infinite loops.

You can also use "{identifier}" as place holder in the topic. It will be translated to your device ID (same as the soft AP network it creates).

## Troubleshooting

After flashing the firmware via serial do a hard reset of the device (unplug & plug). There is an issue with the ESP.reset() method. Check [https://github.com/esp8266/Arduino/issues/1017][4] for more info.

[1]: https://www.itead.cc/sonoff-wifi-wireless-switch.html
[2]: http://tinkerman.cat/adding-rf-to-a-non-rf-itead-sonoff
[3]: http://www.platformio.org
[4]: https://github.com/esp8266/Arduino/issues/1017
[5]: https://github.com/esp8266/Arduino/pull/2251
[6]: https://bitbucket.org/xoseperez/nofuss
[7]: https://bitbucket.org/xoseperez
[8]: https://www.itead.cc/sonoff-rf.html
[9]: https://www.itead.cc/slampher-wifi-wireless-light-holder.html
[10]: https://www.itead.cc/smart-socket-eu.html
[11]: http://gulpjs.com/
[12]: https://docs.npmjs.com/getting-started/installing-node
[13]: https://nodejs.org/en/
[14]: https://www.npmjs.com/
[15]: https://www.itead.cc/sonoff-pow.html
