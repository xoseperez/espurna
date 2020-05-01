/*

ENCODER MODULE

Copyright (C) 2018-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#if ENCODER_SUPPORT && (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE)
#include "libs/Encoder.h"
#include <vector>
#endif

void encoderSetup();
