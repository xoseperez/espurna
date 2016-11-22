# Firmware

## Build the firmware

The project is ready to be build using [PlatformIO][1].
Please refer to their web page for instructions on how to install the builder.

PlatformIO will take care of some of the library dependencies, but not all the required libraries are available in the platform library manager. Some dependencies are thus checked out as submodules in GIT. So the normal initial checkout should be:

```
git clone https://bitbucket.org/xoseperez/espurna.git
git submodule init
git submodule update
```

On linux/max systems the libraries are soft linked to the code/lib folder and you are ready to go. Windows systems do not have this feature so you will have to copy them manually like this (ONLY WINDOWS):

```
cd espurna/code
copy vendor/emonliteesp/code/lib/EmonLiteESP lib/EmonLiteESP
copy vendor/nofuss/client/lib/NoFUSSClient lib/NoFUSSClient
copy vendor/RemoteSwitch-arduino-library lib/RemoteSwitch
```

This libraries are optional at the moment and will only be linked if you set ENABLE_NOFUSS, ENABLE_EMON or ENABLE_RF from your build flag.

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
