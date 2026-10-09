#pragma once

#if defined(ESP8266)
    #include <pgmspace.h>
#elif defined(ESP32)
    #include <pgmspace.h>
#else
    #include <sys/pgmspace.h>
#endif
