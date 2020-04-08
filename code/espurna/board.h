/*

BOARD MODULE

*/

#pragma once

const String& getChipId();
const String& getIdentifier();

String getEspurnaModules();
String getEspurnaOTAModules();
String getEspurnaSensors();

String getEspurnaWebUI();

bool isEspurnaCore();

int getBoardId();
