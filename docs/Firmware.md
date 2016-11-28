# Firmware

## Build the firmware

The project is ready to be build using [PlatformIO][1].
Please refer to their web page for instructions on how to install the builder.

If you are using PlatformIO /strongly recommended) it will take care of the library dependencies. Otherwise you will have to install them manually:

* Benoit Blanchon's [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
* Hristo Gochkov's [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP)
* Hristo Gochkov's [ESPAsyncUDP](https://github.com/me-no-dev/ESPAsyncUDP)
* Hristo Gochkov's [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
* Marvin Roger's [AyncMqttClient](https://github.com/marvinroger/async-mqtt-client)
* Adafruit's [DHT Sensor Library](https://github.com/adafruit/DHT-sensor-library) (required if compiling with DHT support: -DENABLE_DHT)
* Adafruit's [Unified Sensor Library](https://github.com/adafruit/Adafruit_Sensor) (required if compiling with DHT support: -DENABLE_DHT)
* The PatternAgents (et al.) [Embedis](https://github.com/thingSoC/embedis)
* German Martin's [NtpCLientLib](https://github.com/gmag11/NtpClient)
*  Michael Maregolis & Paul Stoffregen's [Time](https://github.com/PaulStoffregen/Time)
* Randy Simons' [RemoteSwitch](https://github.com/jccprj/RemoteSwitch-arduino-library) (required if using custom RF module: -DENABLE_RF)

And my own libraries:

* [JustWifi](https://bitbucket.org/xoseperez/justwifi.git)
* [FauxmoESP](https://bitbucket.org/xoseperez/fauxmoesp.git) (required if compiling with WeMo emulation support: -DENABLE_FAUXMO)
* [HLW8012](https://bitbucket.org/xoseperez/hlw8012.git) (required if compiling for Sonoff POW: -DENABLE_POW)
* [EmonLiteESP](https://bitbucket.org/xoseperez/emonliteesp.git) (required if compiling with Energy Monitoring support: -DENABLE_EMON)
* [NoFUSS](https://bitbucket.org/xoseperez/nofuss.git) (required if compiling with NoFUSS Automatic OTA support: -DENABLE_NOFUSS)

Once you have all the code, you can check if it's working by:

```bash
> pio run -e node-debug
```

If it compiles you are ready to flash the firmware.

## Flash your board

Wire your board (check the [Hardware page](Hardware.md)) and flash the firmware (with ```upload```):

```bash
> pio run -t upload -e sonoff
```

(or any other environment, depending on the board you are woring with).

Library dependencies are automatically managed via PlatformIO Library Manager or included via submodules and linked from the "lib" folder.

Once the firmware is uploaded next step is to upload the web interface. Check how to [build and flash the filesystem](Filesystem.md).

[1]: http://www.platformio.org
