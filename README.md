# ESPurna Firmware

ESPurna ("spark" in Catalan) is a custom firmware for ESP8266 based smart switches.
It was originally developed with the **[IteadStudio Sonoff][1]** in mind but now it supports a growing number of ESP8266-based boards.
It uses the Arduino Core for ESP8266 framework and a number of 3rd party libraries.

**Current Release Version is 1.2.0**, read the [changelog](CHANGELOG.md).

## Features

* Support for **multiple ESP8266-based boards** ([check list](#supported-hardware))
* Wifi **AP Mode** or **STA mode** with **multiple network definitions**
* **MQTT** enabled
    * Switch on/off and toggle relays
    * LED notifications
* Support for different **sensors**
    * DHT11 / DHT22 / DHT21 / AM2301
    * DS18B20
    * HLW8012 (Sonoff POW)
    * Non-invasive current sensor using the [EmonLiteESP Library][3] (requires some hacking)
* Fast asynchronous **HTTP Server**
    * Basic authentication
    * Web-based configuration
    * Relay switching from the web
    * Websockets-based communication between the device and the browser
* **REST API**
    * GET and PUT relay status
* **Command line configuration**
* **Over-The-Air** (OTA) updates even for 1Mb boards
    * Manually from PlatformIO or Arduino Inside
    * Automatic updates through the [NoFUSS Library][2]
* **Alexa** integration (requires staging version of Arduino Core for ESP8266)

## Documentation

For more information please refer to the [ESPurna Wiki](https://bitbucket.org/xoseperez/espurna/wiki/Home).


## Supported hardware

|![Sonoff](images/devices/s20.jpg) **IteadStudio S20**|![Sonoff](images/devices/slampher.jpg) **IteadStudio Slampher**|![Sonoff](images/devices/sonoff-4ch.jpg) **IteadStudio Sonoff 4CH**|
|![Sonoff](images/devices/sonoff-basic.jpg) **IteadStudio Sonoff Basic**|![Sonoff](images/devices/motor-switch.jpg) **IteadStudio Motor Switch**|![Sonoff](images/devices/1ch-inching.jpg) **IteadStudio 1CH Inching**|
|![Sonoff](images/devices/sonoff-dual.jpg) **IteadStudio Sonoff Dual**|![Sonoff](images/devices/sonoff-pow.jpg) **IteadStudio Sonoff POW**|![Sonoff](images/devices/sonoff-th10-th16.jpg) **IteadStudio Sonoff TH10/TH16**|
|![Sonoff](images/devices/sonoff-rf.jpg) **IteadStudio Sonoff RF**|![Sonoff](images/devices/sonoff-sv.jpg) **IteadStudio Sonoff SV**|![Sonoff](images/devices/sonoff-led.jpg) **IteadStudio Sonoff LED**|
|![Sonoff](images/devices/sonoff-touch.jpg) **IteadStudio Sonoff Touch**|![Sonoff](images/devices/electrodragon-relay-board.jpg) **Electrodragon Relay Board**|![Sonoff](images/devices/workchoice-ecoplug.jpg) **WorkChoice EcoPlug**|


## License

Copyright (C) 2016 by Xose Pérez (@xoseperez)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.


[1]: https://www.itead.cc/sonoff-wifi-wireless-switch.html
[2]: https://bitbucket.org/xoseperez/nofuss
[3]: https://bitbucket.org/xoseperez/emonliteesp