# ESPurna

ESPurna ("spark" in Catalan) is a custom C firmware for ESP8266 based smart switches.
It was originally developed with the **[ITead Sonoff][1]** in mind but now it supports a growing number of ESP8266-based boards.

## Features

* **Asynchronous WebServer for configuration** and simple relay toggle with **basic authentication**
* Communication between webserver and webclient via **websockets** with secure ticket check
* **Flashing firmware Over-The-Air** (OTA)
* Up to **3 configurable WIFI networks**, connects to the strongest signal
* **MQTT support** with configurable host and topic
* **REST API** to query and set relay statuses
* Support for **multi-relay boards** (Sonoff Dual, Electrodragon ESP Relay Board,...)
* Manual switch ON/OFF with button (single click the button)
* AP mode backup (double click the button)
* Manual reset the board (long click the button)
* Visual status of the connection via the LED
* **Alexa** integration (Amazon Echo or Dot) by emulating a Belkin WeMo switch
* Support for **automatic over-the-air updates** through the [NoFUSS Library][2]
* Support for **DHT22** and **DS18B20** sensors
* Support for the **HLW8012** power sensor present in the Sonoff POW
* Support for **current monitoring** through the [EmonLiteESP Library][3] using a non-intrusive current sensor ([requires some hacking][4])
* Command line configuration

## Index

* [Supported hardware](docs/Hardware.md)
* [Build and flash the firmware](docs/Firmware.md)
* [Build and flash the filesystem](docs/Filesystem.md)
* [Configuration](docs/Configuration.md)
* [Over-the-air updates](docs/OTA.md)
* [Sensors](docs/Sensors.md)
* [Troubleshooting](docs/Troubleshooting.md)

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
[4]: http://tinkerman.cat/your-laundry-is-done/
