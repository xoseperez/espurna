# Troubleshooting

## Problems resetting the board

After flashing the firmware via serial do a hard reset of the device (unplug & plug). There is an issue with the ESP.reset() method. Check [https://github.com/esp8266/Arduino/issues/1017][1] for more info.

## Can't find espresiff8266_stage platform

The fauxmoESP library requires the staging version of Arduino Core for ESP8266. Either you are using Arduino IDE or PlatformIO you will have to manually install this. Check the [documentation for the fauxmoESP library][2] for more info.


[1]: https://github.com/esp8266/Arduino/issues/1017
[2]: https://bitbucket.org/xoseperez/fauxmoesp
