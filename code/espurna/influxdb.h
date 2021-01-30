/*

INFLUXDB MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

bool idbSend(const char * topic, unsigned char id, const char * payload);
bool idbSend(const char * topic, const char * payload);
bool idbEnabled();
void idbSetup();
