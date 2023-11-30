/*

MDNS MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <Arduino.h>

#include <cstdint>

using MdnsServerQueryCallback = bool(*)(String&& server, uint16_t port);
bool mdnsServiceQuery(const String& service, const String& protocol, MdnsServerQueryCallback callback);

bool mdnsRunning();
void mdnsServerSetup();
