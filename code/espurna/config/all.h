/*

    If you want to modify the stock configuration but you don't want to touch
    the repo files you can define USE_CUSTOM_H in your build settings.

    * Using Arduino IDE: **********************************************************
    Define it in your boards.txt for the board of your choice.
    For instance, for the "Generic ESP8266 Module" with prefix "generic" just add:

        generic.build.extra_flags=-DESP8266 -DUSE_CUSTOM_H

    * Using PlatformIO: ************************************************************
    Add the setting to your environment or just define global PLATFORMIO_BUILD_FLAGS

        export PLATFORMIO_BUILD_FLAGS="'-DUSE_CUSTOM_H'"


    * Then add all your settings into a custom.h file ******************************
	- rename the "custom-sample.h" file to "custom.h" 
	- Modify custom.h according to your needs
	- BTW the custom.h file is ignored by git (so it will survive any future update)

*/

#include "version.h"
#include "arduino.h"
#include "hardware.h"
#include "defaults.h"
#include "general.h"
#include "prototypes.h"
#include "sensors.h"

#ifdef USE_CORE_VERSION_H
#include "core_version.h"
#endif

#include "build.h"

#ifdef USE_CUSTOM_H
#include "custom.h"
#endif
