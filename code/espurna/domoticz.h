/*

DOMOTICZ MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include "sensor.h"

void domoticzSendMagnitude(unsigned char, const sensor::Value&);
void domoticzSetup();
bool domoticzEnabled();
