/*

WIFI MODULE CONFIG

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include "espurna.h"

constexpr bool _wifiHasSSID(unsigned char index) {
    return (
        (index == 0) ? (strlen(WIFI1_SSID) > 0) :
        (index == 1) ? (strlen(WIFI2_SSID) > 0) :
        (index == 2) ? (strlen(WIFI3_SSID) > 0) :
        (index == 3) ? (strlen(WIFI4_SSID) > 0) :
        (index == 4) ? (strlen(WIFI5_SSID) > 0) : false
    );
}

constexpr bool _wifiHasIP(unsigned char index) {
    return (
        (index == 0) ? (strlen(WIFI1_IP) > 0) :
        (index == 1) ? (strlen(WIFI2_IP) > 0) :
        (index == 2) ? (strlen(WIFI3_IP) > 0) :
        (index == 3) ? (strlen(WIFI4_IP) > 0) :
        (index == 4) ? (strlen(WIFI5_IP) > 0) : false
    );
}

const __FlashStringHelper* _wifiSSID(unsigned char index) {
    return (
        (index == 0) ? F(WIFI1_SSID) :
        (index == 1) ? F(WIFI2_SSID) :
        (index == 2) ? F(WIFI3_SSID) :
        (index == 3) ? F(WIFI4_SSID) :
        (index == 4) ? F(WIFI5_SSID) : nullptr
    );
}

const __FlashStringHelper* _wifiPass(unsigned char index) {
    return (
        (index == 0) ? F(WIFI1_PASS) :
        (index == 1) ? F(WIFI2_PASS) :
        (index == 2) ? F(WIFI3_PASS) :
        (index == 3) ? F(WIFI4_PASS) :
        (index == 4) ? F(WIFI5_PASS) : nullptr
    );
}

const __FlashStringHelper* _wifiIP(unsigned char index) {
    return (
        (index == 0) ? F(WIFI1_IP) :
        (index == 1) ? F(WIFI2_IP) :
        (index == 2) ? F(WIFI3_IP) :
        (index == 3) ? F(WIFI4_IP) :
        (index == 4) ? F(WIFI5_IP) : nullptr
    );
}

const __FlashStringHelper* _wifiGateway(unsigned char index) {
    return (
        (index == 0) ? F(WIFI1_GW) :
        (index == 1) ? F(WIFI2_GW) :
        (index == 2) ? F(WIFI3_GW) :
        (index == 3) ? F(WIFI4_GW) :
        (index == 4) ? F(WIFI5_GW) : nullptr
    );
}

const __FlashStringHelper* _wifiNetmask(unsigned char index) {
    return (
        (index == 0) ? F(WIFI1_MASK) :
        (index == 1) ? F(WIFI2_MASK) :
        (index == 2) ? F(WIFI3_MASK) :
        (index == 3) ? F(WIFI4_MASK) :
        (index == 4) ? F(WIFI5_MASK) : nullptr
    );
}

const __FlashStringHelper* _wifiDNS(unsigned char index) {
    return (
        (index == 0) ? F(WIFI1_DNS) :
        (index == 1) ? F(WIFI2_DNS) :
        (index == 2) ? F(WIFI3_DNS) :
        (index == 3) ? F(WIFI4_DNS) :
        (index == 4) ? F(WIFI5_DNS) : nullptr
    );
}
