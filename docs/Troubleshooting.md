# Troubleshooting

After flashing the firmware via serial do a hard reset of the device (unplug & plug). There is an issue with the ESP.reset() method. Check [https://github.com/esp8266/Arduino/issues/1017][1] for more info.

[1]: https://github.com/esp8266/Arduino/issues/1017
