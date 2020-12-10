/*

IR MODULE

Copyright (C) 2018 by Alexander Kolesnikov (raw and MQTT implementation)
Copyright (C) 2017-2019 by François Déchery
Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#if IR_SUPPORT

#include "ir_button.h"

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRsend.h>

void irSetup();

#endif // IR_SUPPORT == 1
