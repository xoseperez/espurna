/*

BOARD MODULE

*/

#pragma once

#include <Arduino.h>

const String& getChipId();
const String& getFullChipId();
const String& getIdentifier();

const char* getEspurnaModules();
const char* getEspurnaSensors();
const char* getEspurnaWebUI();

void boardSetup();
