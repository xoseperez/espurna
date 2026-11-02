#pragma once

#if defined(ESP8266)
    #include "arch/esp8266/system_time_esp8266.h"
#elif defined(ESP32)
    #include "arch/esp32/system_time_esp32.h"
#elif defined(LIBRETINY)
    #include "arch/beken/system_time_beken.h"
#endif

