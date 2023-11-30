/*

KingArt Cover/Shutter/Blind/Curtain support for ESPURNA

Based on xdrv_19_ps16dz.dimmer.ino, PS_16_DZ dimmer support for Tasmota
Copyright (C) 2019 by Albert Weterings

*/

void kingartCurtainSetup();
void curtainSetPosition(unsigned char id, int value);
unsigned char curtainCount();
