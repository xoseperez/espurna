/*

RFM69 MODULE

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#if RFM69_SUPPORT

#include <RFM69.h>
#include <RFM69_ATC.h>
#include <SPI.h>

void rfm69Setup();

#endif // RFM69_SUPPORT == 1
