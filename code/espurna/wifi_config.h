/*

WIFI MODULE CONFIG

Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include "espurna.h"

namespace wifi {
namespace build {

constexpr size_t NetworksMax { WIFI_MAX_NETWORKS };

constexpr unsigned long staReconnectionInterval() {
    return WIFI_RECONNECT_INTERVAL;
}

constexpr unsigned long staConnectionInterval() {
    return WIFI_CONNECT_INTERVAL;
}

constexpr int staConnectionRetries() {
    return WIFI_CONNECT_RETRIES;
}

constexpr wifi::StaMode staMode() {
    return WIFI_STA_MODE;
}

constexpr bool softApCaptive() {
    return 1 == WIFI_AP_CAPTIVE_ENABLED;
}

constexpr wifi::ApMode softApMode() {
    return WIFI_AP_MODE;
}

constexpr uint8_t softApChannel() {
    return WIFI_AP_CHANNEL;
}

constexpr bool hasSoftApSsid() {
    return strlen(WIFI_AP_SSID);
}

const __FlashStringHelper* softApSsid() {
    return F(WIFI_AP_SSID);
}

constexpr bool hasSoftApPassphrase() {
    return strlen(WIFI_AP_PASS);
}

const __FlashStringHelper* softApPassphrase() {
    return F(WIFI_AP_PASS);
}

constexpr unsigned long softApFallbackTimeout() {
    return WIFI_FALLBACK_TIMEOUT;
}

constexpr bool scanNetworks() {
    return 1 == WIFI_SCAN_NETWORKS;
}

constexpr int8_t scanRssiThreshold() {
    return WIFI_SCAN_RSSI_THRESHOLD;
}

constexpr unsigned long scanRssiCheckInterval() {
    return WIFI_SCAN_RSSI_CHECK_INTERVAL;
}

constexpr int8_t scanRssiChecks() {
    return WIFI_SCAN_RSSI_CHECKS;
}

constexpr unsigned long garpIntervalMin() {
    return WIFI_GRATUITOUS_ARP_INTERVAL_MIN;
}

constexpr unsigned long garpIntervalMax() {
    return WIFI_GRATUITOUS_ARP_INTERVAL_MAX;
}

constexpr WiFiSleepType_t sleep() {
    return WIFI_SLEEP_MODE;
}

constexpr float outputDbm() {
    return WIFI_OUTPUT_POWER_DBM;
}

constexpr bool hasSsid(size_t index) {
    return (
        (index == 0) ? (strlen(WIFI1_SSID) > 0) :
        (index == 1) ? (strlen(WIFI2_SSID) > 0) :
        (index == 2) ? (strlen(WIFI3_SSID) > 0) :
        (index == 3) ? (strlen(WIFI4_SSID) > 0) :
        (index == 4) ? (strlen(WIFI5_SSID) > 0) : false
    );
}

constexpr bool hasIp(size_t index) {
    return (
        (index == 0) ? (strlen(WIFI1_IP) > 0) :
        (index == 1) ? (strlen(WIFI2_IP) > 0) :
        (index == 2) ? (strlen(WIFI3_IP) > 0) :
        (index == 3) ? (strlen(WIFI4_IP) > 0) :
        (index == 4) ? (strlen(WIFI5_IP) > 0) : false
    );
}

const __FlashStringHelper* ssid(size_t index) {
    return (
        (index == 0) ? F(WIFI1_SSID) :
        (index == 1) ? F(WIFI2_SSID) :
        (index == 2) ? F(WIFI3_SSID) :
        (index == 3) ? F(WIFI4_SSID) :
        (index == 4) ? F(WIFI5_SSID) : nullptr
    );
}

const __FlashStringHelper* passphrase(size_t index) {
    return (
        (index == 0) ? F(WIFI1_PASS) :
        (index == 1) ? F(WIFI2_PASS) :
        (index == 2) ? F(WIFI3_PASS) :
        (index == 3) ? F(WIFI4_PASS) :
        (index == 4) ? F(WIFI5_PASS) : nullptr
    );
}

const __FlashStringHelper* ip(size_t index) {
    return (
        (index == 0) ? F(WIFI1_IP) :
        (index == 1) ? F(WIFI2_IP) :
        (index == 2) ? F(WIFI3_IP) :
        (index == 3) ? F(WIFI4_IP) :
        (index == 4) ? F(WIFI5_IP) : nullptr
    );
}

const __FlashStringHelper* gateway(size_t index) {
    return (
        (index == 0) ? F(WIFI1_GW) :
        (index == 1) ? F(WIFI2_GW) :
        (index == 2) ? F(WIFI3_GW) :
        (index == 3) ? F(WIFI4_GW) :
        (index == 4) ? F(WIFI5_GW) : nullptr
    );
}

const __FlashStringHelper* mask(size_t index) {
    return (
        (index == 0) ? F(WIFI1_MASK) :
        (index == 1) ? F(WIFI2_MASK) :
        (index == 2) ? F(WIFI3_MASK) :
        (index == 3) ? F(WIFI4_MASK) :
        (index == 4) ? F(WIFI5_MASK) : nullptr
    );
}

const __FlashStringHelper* dns(size_t index) {
    return (
        (index == 0) ? F(WIFI1_DNS) :
        (index == 1) ? F(WIFI2_DNS) :
        (index == 2) ? F(WIFI3_DNS) :
        (index == 3) ? F(WIFI4_DNS) :
        (index == 4) ? F(WIFI5_DNS) : nullptr
    );
}

} // namespace build
} // namespace wifi
