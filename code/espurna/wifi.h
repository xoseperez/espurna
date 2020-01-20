/*

WIFI MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

uint8_t wifiState();
void wifiReconnectCheck();
bool wifiConnected();

String getNetwork();
String getIP();

void wifiDebug();
void wifiDebug(WiFiMode_t modes);

void wifiStartAP();
void wifiStartSTA();
void wifiDisconnect();

void wifiStartWPS();
void wifiStartSmartConfig();

void wifiRegister(wifi_callback_f callback);

void wifiSetup();
void wifiLoop();
