# ESPurna Firmware

ESPurna ("spark" in Catalan) is a custom firmware for ESP8266 based smart switches.
It was originally developed with the **[IteadStudio Sonoff](https://www.itead.cc/sonoff-wifi-wireless-switch.html)** in mind but now it supports a growing number of ESP8266-based boards.
It uses the Arduino Core for ESP8266 framework and a number of 3rd party libraries.

> **Current Release Version is 1.11.0**, read the [changelog](https://bitbucket.org/xoseperez/espurna/src/master/CHANGELOG.md).

> **NOTICE**: Default flash layout changed in 1.8.3, as an unpredicted consequence devices will not be able to persist/retrieve configuration if flashed with 1.8.3 via **OTA** from **PlatformIO**. Please check issue #187.

> **NOTICE**: since version 1.9.0 the default **MQTT topics for commands have changed**. They all now end with "/set". This means you will have to change your controller software (Node-RED or alike) to send messages to -for instance- "/home/living/light/relay/0/set". The device will publish its state in "/home/living/light/relay/0" like before.

## Features

* *KRACK* vulnerability free (when built against Arduino Core 2.4.0 RC2)
* Support for **multiple ESP8266-based boards** ([check list](https://bitbucket.org/xoseperez/espurna/wiki/Hardware))
* Power saving options
* Wifi **AP Mode** or **STA mode**
    * Up to 5 different networks can be defined
    * Supports static IP
    * Scans for strongest network if more than one defined
    * Defaults to AP mode (also available after double clicking the main button)
* Network visibility
    * Supports mDNS (service reporting and metadata)
    * Supports NetBIOS, LLMNR and Netbios (when built against Arduino Core 2.4.0 RC2) and SSDP (experimental)
* Switch management
    * Support for **push buttons** and **toggle switches**
    * Configurable **status on boot** per switch (always ON, always OFF, same as before or toggle)
    * Support for **pulse mode** per switch (normally ON or normally OFF) with configurable time
    * Support for **relay synchronization** (all equal, only one ON, one and only on ON)
    * Support for **MQTT groups** to sync switches between devices
    * Support for **delayed ON/OFF**
* **MQTT** enabled
    * **SSL/TLS support** (not on regular builds, see #64)
    * Switch on/off and toggle relays, group topics (sync relays between different devices)
    * Report button event notifications
    * Enable/disable pulse mode
    * Change LED notification mode
    * Remote reset the board
* **Alexa** integration using the [FauxmoESP Library](https://bitbucket.org/xoseperez/fauxmoesp)
* [**Google Assistant**](http://tinkerman.cat/using-google-assistant-control-your-esp8266-devices/) integration using IFTTT and Webhooks (Google Home, Allo)
* [**Domoticz**](https://domoticz.com/) integration via MQTT
* [**Home Assistant**](https://home-assistant.io/) integration via MQTT
    * Support for switches (on/off)
    * Support for lights (color, brightness, on/off state)
    * Supports MQTT auto-discover feature (both switches and lights)
* [**InfluxDB**](https://www.influxdata.com/) integration via HTTP API
* Support for different **sensors**
    * Environment
        * **DHT11 / DHT22 / DHT21 / AM2301 / Itead's SI7021** (supports celsius & fahrenheit reporting)
        * **BMP280** and **BME280** temperature, humidity (BME280) and pressure sensor by Bosch
        * **SI7021** temperature and humidity sensor
        * **SHT2X** temperature and humidity sensor over I2C (Wemos shield)
        * **Dallas OneWire sensors** like the DS18B20 (supports celsius & fahrenheit reporting)
        * **MHZ19** CO2 sensor
        * **PMSX003** dust sensor
    * Power monitoring
        * **HLW8012** using the [HLW8012 Library](https://bitbucket.org/xoseperez/hlw8012) (Sonoff POW)
        * Non-invasive **current sensor** using **internal ADC** or **ADC121** or **ADS1115**
        * **ECH1560** power monitor chip
        * **V9261F** power monitor chip
    * Raw analog and digital sensors
    * Simple pulse counter
* Support for LED lights
    * MY92XX-based light bulbs and PWM LED strips (dimmers) up to 5 channels (RGB, cold white and warm white, for instance)
    * RGB and HSV color codes supported
    * Manage channels individually
    * Temperature color supported (in mired and kelvin) via MQTT / REST API
    * Flicker-free PWM management
    * Soft color transitions
* Fast asynchronous **HTTP Server**
    * Configurable port
    * Basic authentication
    * Web-based configuration
    * Relay switching and sensor data from the web interface
    * Handle color, brightness, and white/warm channels for lights
    * Websockets-based communication between the device and the browser
    * Backup and restore settings option
    * Upgrade firmware from the web interface
    * Works great behind a [secured reverse proxy](http://tinkerman.cat/secure-remote-access-to-your-iot-devices/)
* **REST API** (enable/disable from web interface)
    * GET and PUT relay status
    * Change light color (for supported hardware)
    * GET sensor data (power, current, voltage, temperature and humidity) depending on the available hardware
    * Works great behind a secured reverse proxy
* **RPC API** (enable/disable from web interface)
    * Remote reset the board
* **Over-The-Air** (OTA) updates even for 1Mb boards
    * Manually from PlatformIO or Arduino IDE
    * Automatic updates through the [NoFUSS Library](https://bitbucket.org/xoseperez/nofuss)
    * Update from web interface using pre-built images
* **Command line configuration**
    * Change configuration
    * Run special commands
* **Telnet support**
    * Enable/disable via the web UI
    * Show debug info and allows to run terminal commands
* **Unstable system check**
    * Detects unstable system (crashes on boot continuously) and defaults to a stable system
    * Only WiFi AP, OTA and Telnet available if system is flagged as unstable
* Button interface
    * Click to toggle relays
    * Double click to enter AP mode (only main button)
    * Long click (>1 second) to reboot device (only main button)
    * Extra long click (>10 seconds) to go back to factory settings (only main button)

## Documentation

For more information please refer to the [ESPurna Wiki](https://bitbucket.org/xoseperez/espurna/wiki/Home).

## Supported hardware

Here is the list of supported hardware. For more information please refer to the [ESPurna Wiki Hardware page](https://bitbucket.org/xoseperez/espurna/wiki/Hardware).

||||
|---|---|---|
|![Tinkerman Espurna H](images/devices/tinkerman-espurna-h.jpg)|![IteadStudio Sonoff RF Bridge](images/devices/itead-sonoff-rfbridge.jpg)||
|**Tinkerman ESPurna H**|**IteadStudio Sonoff RF Bridge**||
|![IteadStudio Sonoff Basic](images/devices/itead-sonoff-basic.jpg)|![IteadStudio Sonoff RF](images/devices/itead-sonoff-rf.jpg)|![Electrodragon WiFi IOT](images/devices/electrodragon-wifi-iot.jpg)|
|**IteadStudio Sonoff Basic**|**IteadStudio Sonoff RF**|**Electrodragon WiFi IOT**|
|![IteadStudio Sonoff Dual](images/devices/itead-sonoff-dual.jpg)|![IteadStudio Sonoff POW](images/devices/itead-sonoff-pow.jpg)|![IteadStudio Sonoff TH10/TH16](images/devices/itead-sonoff-th.jpg)|
|**IteadStudio Sonoff Dual**|**IteadStudio Sonoff POW**|**IteadStudio Sonoff TH10/TH16**|
|![IteadStudio Sonoff 4CH](images/devices/itead-sonoff-4ch.jpg)|![IteadStudio Sonoff 4CH Pro](images/devices/itead-sonoff-4ch-pro.jpg)|![OpenEnergyMonitor WiFi MQTT Relay / Thermostat](images/devices/openenergymonitor-mqtt-relay.jpg)|
|**IteadStudio Sonoff 4CH**|**IteadStudio Sonoff 4CH Pro**|**OpenEnergyMonitor WiFi MQTT Relay / Thermostat**|
|![IteadStudio S20](images/devices/itead-s20.jpg)|![WorkChoice EcoPlug](images/devices/workchoice-ecoplug.jpg)|![Power meters based on V9261F and ECH1560](images/devices/generic-v9261f.jpg)|
|**IteadStudio S20**|**WorkChoice EcoPlug**|**Power meters based on V9261F and ECH1560**|
|![IteadStudio Sonoff Touch](images/devices/itead-sonoff-touch.jpg)|![IteadStudio Sonoff T1](images/devices/itead-sonoff-t1.jpg)||
|**IteadStudio Sonoff Touch**|**IteadStudio Sonoff T1**||
|![IteadStudio Slampher](images/devices/itead-slampher.jpg)|![AI-Thinker Wifi Light / Noduino OpenLight](images/devices/aithinker-ai-light.jpg)|![Itead Sonoff B1](images/devices/itead-sonoff-b1.jpg)|
|**IteadStudio Slampher**|**AI-Thinker Wifi Light / Noduino OpenLight**|**IteadStudio Sonoff B1**|
|![MagicHome LED Controller (1.0 and 2.0)](images/devices/magichome-led-controller.jpg)|![Huacanxing H801](images/devices/huacanxing-h801.jpg)|![Itead BN-SZ01](images/devices/itead-bn-sz01.jpg)|
|**MagicHome LED Controller (1.0 and 2.0)**|**Huacanxing H801**|**Itead BN-SZ01**|
|![IteadStudio Sonoff SV](images/devices/itead-sonoff-sv.jpg)|![IteadStudio 1CH Inching](images/devices/itead-1ch-inching.jpg)|![IteadStudio Motor Clockwise/Anticlockwise](images/devices/itead-motor.jpg)|
|**IteadStudio Sonoff SV**|**IteadStudio 1CH Inching**|**IteadStudio Motor Clockwise/Anticlockwise**|
|![Wemos D1 Mini Relay Shield](images/devices/wemos-d1-mini-relayshield.jpg)|![Jan Goedeke Wifi Relay (NO/NC)](images/devices/jangoe-wifi-relay.png)|![Jorge García Wifi + Relays Board Kit](images/devices/jorgegarcia-wifi-relays.jpg)|
|**Wemos D1 Mini Relay Shield**|**Jan Goedeke Wifi Relay (NO/NC)**|**Jorge García Wifi + Relays Board Kit**|
|![EXS Wifi Relay v3.1](images/devices/exs-wifi-relay-v31.jpg)|||
|**EXS Wifi Relay v3.1**|||

**Other supported boards:** Itead Sonoff LED, Itead Sonoff Dual R2, Huacanxing H802, WiOn 50055, ManCaveMade ESP-Live, InterMitTech QuinLED 2.6, Arilux AL-LC06, Arilux E27 light bulb, Xenon SM-PW702U, Authometion LYT8266, YJZK 2-gang switch.

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
