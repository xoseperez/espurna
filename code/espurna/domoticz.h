/*

DOMOTICZ MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

void domoticzSendMagnitude(unsigned char type, unsigned char index, double value, const char* buffer);
void domoticzSetup();
bool domoticzEnabled();
