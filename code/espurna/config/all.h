/*

    If you want to modify the stock configuration but you don't want to touch
    the repo files you can define USE_CUSTOM_H in your build settings.

    Arduino IDE:
    define it in your boards.txt for the board of your choice.
    For instance, for the "Generic ESP8266 Module" with prefix "generic" just add:

        generic.build.extra_flags=-DESP8266 -DUSE_CUSTOM_H

    PlatformIO:
    add the setting to your environment or just define global PLATFORMIO_BUILD_FLAGS

        export PLATFORMIO_BUILD_FLAGS="'-DUSE_CUSTOM_H'"

    Check https://github.com/xoseperez/espurna/issues/104
    for an example on how to use this file.

*/

#ifdef USE_CUSTOM_H
#include "custom.h"
#endif

#include "version.h"
#include "types.h"
#include "arduino.h"
#include "hardware.h"
#include "defaults.h"
#include "buildtime.h"
#include "deprecated.h"
#include "general.h"
#include "dependencies.h"
#include "debug.h"
#include "prototypes.h"
#include "sensors.h"
#include "webui.h"
#include "progmem.h"
