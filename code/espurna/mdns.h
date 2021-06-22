/*

MDNS MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

using MdnsServerQueryCallback = bool(*)(String&& server, uint16_t port);

bool mdnsRunning();
bool mdnsServiceQuery(const String& service, const String& protocol, MdnsServerQueryCallback callback);
void mdnsServerSetup();
