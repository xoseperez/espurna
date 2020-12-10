#pragma once

#include "espurna.h"
#include <Arduino.h>

#if MDNS_SERVER_SUPPORT

#include <ESP8266mDNS.h>
void mdnsServerSetup();

#endif
