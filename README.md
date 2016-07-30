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
* Tinkerman ESPurna board

## Features

* **WebServer for configuration** and simple relay toggle
* **Flashing firmware Over-The-Air** (OTA)
* Up to **3 configurable WIFI networks**
* **MQTT support** with configurable host and topic
* Manual switch ON/OFF with button (single click the button)
* AP mode backup (double click the button)
* Visual status of the connection via the LED
* Support for custom **[RF module][2]**
* Support for **automatic over-the-air updates** through the [NoFUSS Library][6]
* Support for **current monitoring** through then [EmonLiteESP Library][7]
* Support for **DHT22** sensors


## Flashing

The unpopulated header has all the required pins. My board has a 5 pins header in-line with the button. They are (from the button outwards):

* 3V3
* RX
* TX
* GND
* GPIO14

Last one is not necessary. Mind it's a **3V3 device**, if connected to 5V you will probably fry it. Button is connected to GPIO0 on the ESP8266 chip, so to enter flash mode you have to hold the button pressed while powering on the board, then you can realease it again.

The project is ready to be build using [PlatformIO][3].
Please refer to their web page for instructions on how to install the builder. Once installed:

```bash
> platformio run --target upload -e wire
> platformio run --target uploadfs -e wire
```

Once you have flashed it you can flash it again over-the-air using the ```ota``` environment:

```bash
> platformio run --target upload -e ota
> platformio run --target uploadfs -e ota
```

When using OTA environment it defaults to the IP address of the device in SoftAP mode. If you want to flash it when connected to your home network best way is to supply the IP of the device:

```bash
> platformio run --target upload -e ota --upload-port 192.168.1.151
> platformio run --target uploadfs -e ota --upload-port 192.168.1.151
```

You can also use the automatic OTA update feature. Check the [NoFUSS library][6] for more info.

Library dependencies are automatically managed via PlatformIO Library Manager.

## Usage

On normal boot (i.e. button not pressed) it will execute the firmware. It configures the hardware (button, LED, relay), the SPIFFS memory access, the WIFI, the WebServer and MQTT connection.

Obviously the default values for WIFI network and MQTT will probably not match your requirements. The device will start in Soft AP creating a WIFI SSID named "SONOFF_XXXXXX", where XXXXXX are the last 3 bytes of the radio MAC. Connect with phone, PC, laptop, whatever to that network, password is "fibonacci". Once connected
browse to 192.168.4.1 and you will be presented a configuration page where you will be able to define up to 3 possible WIFI networks and the MQTT configuration parameters.

It will then try to connect to the configure WIFI networks one after the other. If none of the 3 attempts succeed it will default to SoftAP mode again. Once connected it will try to connect the MQTT server. You can also switch to SoftAP mode by long clicking the on board button or reset the board double clicking the it.

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
