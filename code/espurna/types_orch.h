#pragma once

#if defined(ESP8266)
    #include "arch/esp8266/types_esp8266.h"
#elif defined(ESP32)
    #include "arch/esp32/types_esp32.h"
#endif

