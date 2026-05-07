#pragma once

#if defined(ESP8266)
    #include "pgmspace_orch.h"
#elif defined(ESP32)
    #include <pgmspace.h>
#endif

