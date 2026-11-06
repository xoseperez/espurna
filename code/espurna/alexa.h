/*

ALEXA MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

bool alexaEnabled();
void alexaSetup();

// while paused (Matter commissioning) the fauxmo servers are off, settings stay as they are
void alexaPause(bool);

