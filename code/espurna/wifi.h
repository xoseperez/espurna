/*

WIFI MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include "espurna.h"

#include <lwip/init.h>
#if LWIP_VERSION_MAJOR == 1
#include <netif/etharp.h>
#elif LWIP_VERSION_MAJOR >= 2
#include <lwip/etharp.h>
#endif

#define LWIP_INTERNAL
#include <JustWifi.h>
#include <Ticker.h>
#undef LWIP_INTERNAL

extern "C" {
  #include <lwip/opt.h>
  #include <lwip/ip.h>
  #include <lwip/tcp.h>
  #include <lwip/inet.h> // ip_addr_t
  #include <lwip/err.h> // ERR_x
  #include <lwip/dns.h> // dns_gethostbyname
  #include <lwip/ip_addr.h> // ip4/ip6 helpers
};

// ref: https://github.com/me-no-dev/ESPAsyncTCP/pull/115/files#diff-e2e636049095cc1ff920c1bfabf6dcacR8
// This is missing with Core 2.3.0 and is sometimes missing from the build flags. Assume HIGH_BANDWIDTH version.
#ifndef TCP_MSS
#define TCP_MSS (1460)
#endif

using wifi_callback_f = std::function<void(justwifi_messages_t code, char * parameter)>;

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
