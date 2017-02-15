# ESPurna Firmware

ESPurna ("spark" in Catalan) is a custom firmware for ESP8266 based smart switches.
It was originally developed with the **[IteadStudio Sonoff](https://www.itead.cc/sonoff-wifi-wireless-switch.html)** in mind but now it supports a growing number of ESP8266-based boards.
It uses the Arduino Core for ESP8266 framework and a number of 3rd party libraries.

**Current Release Version is 1.6.3**, read the [changelog](https://bitbucket.org/xoseperez/espurna/src/master/CHANGELOG.md).

## Features

* Support for **multiple ESP8266-based boards** ([check list](https://bitbucket.org/xoseperez/espurna/wiki/Hardware))
* Wifi **AP Mode** or **STA mode**
    * Up to 5 different networks can be defined
    * Supports static IP
    * Scans for strongest network if more than one defined
    * Defaults to AP mode (also available after double clicking the main button)
* Switch management
    * Support for **push buttons** and **toggle switches**
    * Configurable **status on boot** (always ON, always OFF, same as before or toggle)
    * Support for **pulse mode** (normally ON or normally OFF) with configurable time
    * Support for **relay synchronization** (all equal, only one ON, one and only on ON)
* **MQTT** enabled
    * Switch on/off and toggle relays
    * Report button event notifications
    * Enable/disable pulse mode
    * Change LED notification mode
    * Remote reset the board
* **Alexa** integration using the [FauxmoESP Library](https://bitbucket.org/xoseperez/fauxmoesp)
* [**Domoticz**](https://domoticz.com/) integration via MQTT
* [**Home Assistant**](https://home-assistant.io/) integration via MQTT
* Support for different **sensors**
    * DHT11 / DHT22 / DHT21 / AM2301 (supports celsius & fahrenheit reporting)
    * DS18B20 (supports celsius & fahrenheit reporting)
    * HLW8012 using the [HLW8012 Library](https://bitbucket.org/xoseperez/hlw8012) (Sonoff POW)
    * Non-invasive current sensor using the [EmonLiteESP Library](https://bitbucket.org/xoseperez/emonliteesp) (requires some hacking)
* Fast asynchronous **HTTP Server**
    * Configurable port
    * Basic authentication
    * Web-based configuration
    * Relay switching and sensor data from the web interface
    * Websockets-based communication between the device and the browser
* **REST API** (enable/disable from web interface)
    * GET and PUT relay status
    * GET sensor data (power, current, voltage, temperature and humidity) depending on the available hardware
* **RPC API** (enable/disable from web interface)
    * Remote reset the board
* **Over-The-Air** (OTA) updates even for 1Mb boards
    * Manually from PlatformIO or Arduino IDE
    * Automatic updates through the [NoFUSS Library](https://bitbucket.org/xoseperez/nofuss)
* **Command line configuration**

## Documentation

For more information please refer to the [ESPurna Wiki](https://bitbucket.org/xoseperez/espurna/wiki/Home).

## Supported hardware

Here is the list of supported hardware. For more information please refer to the [ESPurna Wiki Hardware page](https://bitbucket.org/xoseperez/espurna/wiki/Hardware).

||||
|-|-|-|
|![IteadStudio S20](images/devices/s20.jpg)|![IteadStudio Slampher](images/devices/slampher.jpg)|![IteadStudio Sonoff 4CH](images/devices/sonoff-4ch.jpg)|
|**IteadStudio S20**|**IteadStudio Slampher**|**IteadStudio Sonoff 4CH**|
|![IteadStudio Sonoff Basic](images/devices/sonoff-basic.jpg)|![IteadStudio 1CH Inching](images/devices/1ch-inching.jpg)|![IteadStudio Motor Clockwise/Anticlockwise](images/devices/motor-switch.jpg)|
|**IteadStudio Sonoff Basic**|**IteadStudio 1CH Inching**|**IteadStudio Motor Clockwise/Anticlockwise**|
|![IteadStudio Sonoff Dual](images/devices/sonoff-dual.jpg)|![IteadStudio Sonoff POW](images/devices/sonoff-pow.jpg)|![IteadStudio Sonoff TH10/TH16](images/devices/sonoff-th10-th16.jpg)|
|**IteadStudio Sonoff Dual**|**IteadStudio Sonoff POW**|**IteadStudio Sonoff TH10/TH16**|
|![IteadStudio Sonoff RF](images/devices/sonoff-rf.jpg)|![IteadStudio Sonoff SV](images/devices/sonoff-sv.jpg)|![IteadStudio Sonoff Touch](images/devices/sonoff-touch.jpg)|
|**IteadStudio Sonoff RF**|**IteadStudio Sonoff SV**|**IteadStudio Sonoff Touch**|
|![Wemos D1 Mini Relay Shield](images/devices/d1mini.jpg)|![Electrodragon Relay Board](images/devices/electrodragon-relay-board.jpg)|![WorkChoice EcoPlug](images/devices/workchoice-ecoplug.jpg)|
|**Wemos D1 Mini Relay Shield**|**Electrodragon Relay Board**|**WorkChoice EcoPlug**|
|![Jan Goedeke Wifi Relay (NO/NC)](images/devices/jangoe-wifi-relay.png)|![OpenEnergyMonitor WiFi MQTT Relay / Thermostat](images/devices/mqtt-relay.jpg)|![Jorge García Wifi + Relays Board Kit](images/devices/jorgegarcia-wifi-relays-board-kit.jpg)|
|**Jan Goedeke Wifi Relay (NO/NC)**|**OpenEnergyMonitor WiFi MQTT Relay / Thermostat**|**Jorge García Wifi + Relays Board Kit**|

## License

Copyright (C) 2016-2017 by Xose Pérez (@xoseperez)

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
