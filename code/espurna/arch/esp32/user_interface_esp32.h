#pragma once

#include <Arduino.h>
#include <esp_system.h>
#include <esp_wifi.h>

#ifdef __cplusplus
#include <Ticker.h>

extern "C" {
#endif

// --- Basic types (removed redundant and conflicting typedefs) ---

// --- System constants ---
#ifndef REASON_DEFAULT_RST
#define REASON_DEFAULT_RST 0
#define REASON_WDT_RST 1
#define REASON_EXCEPTION_RST 2
#define REASON_SOFT_WDT_RST 3
#define REASON_SOFT_RESTART 4
#define REASON_DEEP_SLEEP_AWAKE 5
#define REASON_EXT_SYS_RST 6
#endif

struct rst_info {
    uint32_t reason;
    uint32_t exccause;
    uint32_t epc1;
    uint32_t epc2;
    uint32_t epc3;
    uint32_t excvaddr;
    uint32_t depc;
};

extern struct rst_info resetInfo;
void system_init_reset_info();

// --- System functions ---
#define system_get_free_heap_size() esp_get_free_heap_size()
#define system_restart() esp_restart()
#define system_get_sdk_version() esp_get_idf_version()
#define system_get_cpu_freq() ((uint8_t)getCpuFrequencyMhz())
#define system_get_chip_id() ((uint32_t)ESP.getEfuseMac())

// --- Timer functions (removed redundant and conflicting definitions) ---

// --- WiFi functions ---
#ifndef STATION_IF
#define STATION_IF WIFI_IF_STA
#endif
#ifndef SOFTAP_IF
#define SOFTAP_IF WIFI_IF_AP
#endif

#define wifi_get_opmode() (uint8_t)WiFi.getMode()
#define wifi_station_get_rssi() WiFi.RSSI()

// Sleep types
#ifndef NONE_SLEEP_T
enum {
    NONE_SLEEP_T = 0,
    LIGHT_SLEEP_T,
    MODEM_SLEEP_T
};
#endif

#define wifi_set_sleep_type(type) esp_wifi_set_ps((type == NONE_SLEEP_T) ? WIFI_PS_NONE : WIFI_PS_MIN_MODEM)
#define wifi_get_sleep_type() (WiFi.getSleep() ? MODEM_SLEEP_T : NONE_SLEEP_T)

#ifdef __cplusplus
}
#endif
