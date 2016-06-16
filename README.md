# ESPurna

ESPurna ("spark" in Catalan) is a custom C firmware for [ITead Sonoff][1] Smart WiFi Switch.
This device has an ESP8266 on board with a 8Mbit flash memory chip, a mains to 3V3 transformer
and a relay (GPIO12). It also features a button (GPIO0), an LED (GPIO13) and an unpopulated header you can use to reprogram it.
You can read about this board and firmware in [my blog][2].

## Features

* WebServer for configuration and simple relay toggle
* Flashing firmware Over-The-Air (OTA)
* Up to 3 configurable WIFI networks
* MQTT support with configurable host and topic
* Manual switch ON/OFF with button
* Support for custom RF module (check blog post)
* Visual status of the connection via the LED

## Flashing

The unpopulated header has all the required pins. My board has a 5 pins header
in-line with the button. They are (from the button outwards):

* 3V3
* RX
* TX
* GND
* GPIO14

Last one is not necessary. Mind it's a **3V3 device**, if connected to 5V you will
probably fry it. Button is connected to GPIO0 on the ESP8266 chip, so to enter
flash mode you have to hold the button pressed while powering on the board, then
you can realease it again.

The project is ready to be build using [PlatformIO][3].
Please refer to their web page for instructions on how to install the builder.
Once installed:

```bash
> platformio run --target upload -e wire
> platformio run --target uploadfs -e wire
```

Once you have flashed it you can flash it again over-the-air using the ```ota``` environment:

```bash
> platformio run --target upload -e ota
> platformio run --target uploadfs -e ota
```

Library dependencies are automatically managed via PlatformIO Library Manager.

## Usage

On normal boot (i.e. button not pressed) it will execute the firmware.
It configures the hardware (button, LED, relay), the SPIFFS memory access, the
WIFI, the WebServer and MQTT connection.

Obviously the default values for WIFI network and MQTT will probably not match
your requirements. Either it connects to a WiFi or not, it will set up a Soft AP
named "SONOFF_XXXX", where XXXX are the las 2 bytes of the radio MAC. Connect with
phone, PC, laptop, whatever to that network, password is "fibonacci". Once connected
browse to 192.168.4.1 and you will be presented a configuration page where you will
be able to define up to 3 possible WIFI networks and the MQTT configuration parameters.

It will then try to connect to the first WIFI network. If fail it will try the second
in 30 seconds, and so on. Once connected it will try to connect the MQTT server.

The device will publish the relay state to the given topic and it will subscribe to
the same topic plus "/set" for remote switching. So if your topic is "/home/living/switch"
you will be able to switch it on/off sending "1"/"0" to "/home/living/switch/set".

You can also use "{identifier}" as place holder in the topic. It will be translated to
your device ID (same as the soft AP network it creates).


[1]: https://www.itead.cc/sonoff-wifi-wireless-switch.html
[2]: http://tinkerman.cat/adding-rf-to-a-non-rf-itead-sonoff
[3]: http://www.platformio.org
