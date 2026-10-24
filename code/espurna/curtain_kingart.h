/*

KingArt Cover/Shutter/Blind/Curtain support for ESPURNA

Based on xdrv_19_ps16dz.dimmer.ino, PS_16_DZ dimmer support for Tasmota
Copyright (C) 2019 by Albert Weterings

*/

#include <cstddef>

void curtainSetup();

size_t curtainCount();
void curtainUpdate(size_t id, int value);

