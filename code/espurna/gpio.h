/*

GPIO MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

bool gpioValid(unsigned char gpio);
bool gpioGetLock(unsigned char gpio);
bool gpioReleaseLock(unsigned char gpio);

void gpioSetup();
