# Supported Hardware

This is the official list of supported hardware for the ESPurna firmware. The hardware configuration for each of these boards can be selected by supplying the build flag (see [Firmware section](Firmware.h)).

**CAUTION: Never ever connect any of these devices to your computer and to mains at the same time. Never ever manipulate them while connected to mains. Seriously. I don't want you to die. I hold no responsibility for any damage to you, your family, your house,... for any action or results derived from flashing or using these devices.**

* [IteadStudio Sonoff](#iteadstudio-sonoff)
* [IteadStudio Sonoff RF](#iteadstudio-sonoff-rf)
* [IteadStudio Sonoff TH](#iteadstudio-sonoff-th)
* [IteadStudio Sonoff POW](#iteadstudio-sonoff-pow)
* [IteadStudio Sonoff DUAL](#iteadstudio-sonoff-dual)
* [IteadStudio Sonoff TOUCH](#iteadstudio-sonoff-touch)
* [IteadStudio Sonoff 4CH](#iteadstudio-sonoff-4ch)
* [IteadStudio Sonoff SV](#iteadstudio-sonoff-sv)
* [IteadStudio Slampher](#iteadstudio-slampher)
* [IteadStudio S20](#iteadstudio-s20)
* [Electrodragon ESP Relay Board](#electrodragon-esp-relay-board)
* [WorkChoice EcoPlug](#workchoice-ecoplug)

## IteadStudio Sonoff

|Property|Value|
|---|---|
|Manufacturer|Itead Studio|
|Web page|[https://www.itead.cc/sonoff-wifi-wireless-switch.html](https://www.itead.cc/sonoff-wifi-wireless-switch.html)|
|Build flag|SONOFF|

The [IteadStudio Sonoff][1] has an ESP8266 on board with a 8Mbit flash memory chip, a mains to 3V3 transformer and a relay (GPIO12). It also features a button (GPIO0), an LED (GPIO13) and an unpopulated header you can use to reprogram it.

### Flashing

![Sonoff - Inside front view](images/sonoff-flash.jpg)

The unpopulated header in the Sonoff has all the required pins. My board has a 5 pins header in-line with the button. They are (from the button outwards) 3V3, RX, TX, GND and GPIO14.

Last one is not necessary. Mind it's a **3V3 device**, if connected to 5V you will probably fry it. Button is connected to GPIO0 on the ESP8266 chip, so to enter flash mode you have to hold the button pressed while powering on the board, then you can release it again.

## IteadStudio Sonoff RF

|Property|Value|
|---|---|
|Manufacturer|Itead Studio|
|Web page|[https://www.itead.cc/sonoff-rf.html][2]|
|Build flag|SONOFF_RF|

### Flashing

![Sonoff POW - Inside back view](images/sonoff-rf-flash.jpg)

The Sonoff RF has the same unpopulated header as the Sonoff. It is a 5 pins header in-line with the button. They are (from the button outwards) 3V3, RX, TX, GND and GPIO14.

Solder a male or female header here and connect your USB-to-UART programmer (remember **it's a 3V3 device**). This time through **the button is not connected to GPIO0** but to a EFM8BB1 microcontroller that also monitors the RF module output.

There are a couple of ways to enter flash mode. Some recommend to move 0Ohm R9 resistor to R21 to connect the button directly to the ESP8266 GPIO0 and use it in the same way as for the Sonoff or Sonoff TH. The drawback is the by doing that you lose the RF capability.

My recommendation is to **temporary shortcut the bottom pad of the unpopulated R21 footprint** (see the image above) and connect your USB-to-UART board at the same time. You will have to do it just once (unless there is something really wrong in the firmware) and use OTA updates from there on.


## IteadStudio Sonoff TH

|Property|Value|
|---|---|
|Manufacturer|Itead Studio|
|Web page|[https://www.itead.cc/sonoff-th.html](https://www.itead.cc/sonoff-th.html)|
|Build flag|SONOFF_TH|

### Flashing

![Sonoff TH - Inside back view](images/sonoff-th-flash.jpg)

You have all the required pins in an unpopulated header in one of the corners of the board (see top left corner in the image above). Solder a 4 pins male or female header here and connect it to your favourite USB-to-UART module. Remember: **it's a 3V3 device**!!.

As in the Sonoff the button is connected to GPIO0, so to enter flash mode press and hold the button and connect the programmer to your computer to power the board.

## IteadStudio Sonoff POW

|Property|Value|
|---|---|
|Manufacturer|Itead Studio|
|Web page|[https://www.itead.cc/sonoff-pow.html](https://www.itead.cc/sonoff-pow.html)|
|Build flag|SONOFF_POW|

### Flashing

![Sonoff POW - Inside back view](images/sonoff-pow-flash.jpg)

Same as for the [Sonoff TH](#iteadstudio-sonoff-th) above.

## IteadStudio Sonoff DUAL

|Property|Value|
|---|---|
|Manufacturer|Itead Studio|
|Web page|[https://www.itead.cc/sonoff-dual.html](https://www.itead.cc/sonoff-dual.html)|
|Build flag|SONOFF_DUAL|

### Flashing

![Sonoff DUAL - Inside back view](images/sonoff-dual-flash.jpg)

The Sonoff Dual it's a bit tricky to flash since GPIO0 is not connected to the button as in the TH or POW, but to the pin 15 in the SIL F330 chip that manages the buttons and the relays. SO you have to locate a pad connected to GPIO and short it to ground while powering the device.

In the picture above you have a location of an available and easily accessible GPIO0 pad. The other required pins are brought out in the top header. Remember it's a *3V3* device.

Once flashed use OTA to update the firmware or the filesystem.

## IteadStudio Sonoff TOUCH

|Property|Value|
|---|---|
|Manufacturer|Itead Studio|
|Web page|[https://www.itead.cc/sonoff-touch.html](https://www.itead.cc/sonoff-touch.html)|
|Build flag|SONOFF_TOUCH|

### Flashing

*TODO*

## IteadStudio Sonoff 4CH

|Property|Value|
|---|---|
|Manufacturer|Itead Studio|
|Web page|[https://www.itead.cc/sonoff-4ch.html](https://www.itead.cc/sonoff-4ch.html)|
|Build flag|SONOFF_4CH|

### Flashing

*TODO*

## IteadStudio Sonoff SV

|Property|Value|
|---|---|
|Manufacturer|Itead Studio|
|Web page|[https://www.itead.cc/sonoff-sv.html](https://www.itead.cc/sonoff-sv.html)|
|Build flag|SONOFF_SV|

### Flashing

*TODO*

## IteadStudio Slampher

|Property|Value|
|---|---|
|Manufacturer|Itead Studio|
|Web page|[https://www.itead.cc/slampher.html](https://www.itead.cc/slampher.html)|
|Build flag|SLAMPHER|

### Flashing

![Slampher - Inside front view](images/slampher-flash1.jpg)
![Slampher - Flashing short](images/slampher-flash2.jpg)

There is a 4 pin unpopulated header in a border near the ESP8266 chip. Starting form the little white mark this header brings out 3V3, RX, TX and GND. Solder a male or female header here and connect your USB-to-UART programmer (remember it's a 3V3 device). This time through **the button is not connected to GPIO0** but to a EFM8BB1 microcontroller that also monitors the RF module output.

There are a couple of ways to enter flash mode. Some recommend to move R21 to R20 (at the top right of the first picture above) to connect the button directly to the ESP8266 GPIO0 and use it in the same way as for the Sonoff or Sonoff TH. The drawback is the by doing that you lose the RF capability.

My recommendation is to **temporary shortcut the right pad of the unpopulated R20 footprint** (see second image above) and connect your USB-to-UART board at the same time. You will have to do it just once (unless there is something really wrong in the firmware) and use OTA updates from there on.

## IteadStudio S20 Smart Socket

|Property|Value|
|---|---|
|Manufacturer|Itead Studio|
|Web page|[https://www.itead.cc/smart-socket.html](https://www.itead.cc/smart-socket.html)|
|Build flag|S20|

### Flashing

![S20 Smart Socket - Inside front view](images/s20-flash.jpg)

There is a labeled header in the front of the PCB and the button is connected to GPIO0, so no problems here.

Solder a 4 pin male or female header and connect it to your USB-to-UART bridge. Again, remember **it's a 3V3 device**. Then press and hold the button and connect the programmer to your computer. The microcontroller will boot into flash mode and you are ready to update the firmware.

## Electrodragon ESP Relay Board

|Property|Value|
|---|---|
|Manufacturer|Electrodragon|
|Web page|[http://www.electrodragon.com/product/wifi-iot-relay-board-based-esp8266/](http://www.electrodragon.com/product/wifi-iot-relay-board-based-esp8266/)|
|Build flag|ESP_RELAY_BOARD|

### Flashing

![Electrodragon ESP Relay Board - Front view](images/electrodragon-flash.jpg)

The Electrodragon ESP Relay Board is pretty easy to flash IF you do not follow their wiki, it's all wrong. Check the picture above and note that:

* Power the board from the 5V pin, GND to GND
* The RX pin in the header should go to your programmer TX pin and
* The TX pin in the header should go to your programmer RX pin
* The button labeled BTN2 is connected to GPIO0, so hold it down while powering the board, I've had better results keeping it down until the flashing starts

## WorkChoice EcoPlug

|Property|Value|
|---|---|
|Manufacturer|WorkChoice|
|Web page (non-official)|[http://thegreatgeekery.blogspot.com.es/2016/02/ecoplug-wifi-switch-hacking.html](http://thegreatgeekery.blogspot.com.es/2016/02/ecoplug-wifi-switch-hacking.html)|
|Build flag|ECOPLUG|

### Flashing

*TODO*



[1]: https://www.itead.cc/sonoff-wifi-wireless-switch.html
[2]: https://www.itead.cc/sonoff-rf.html
[2]: https://www.itead.cc/sonoff-th.html
[4]: https://www.itead.cc/sonoff-pow.html
[5]: https://www.itead.cc/slampher.html
[6]: https://www.itead.cc/smart-socket.html
