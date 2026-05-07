#pragma once

#if defined(ESP8266)
    #include <user_interface.h>
#elif defined(ESP32)
    #include "arch/esp32/user_interface_esp32.h"
#endif

