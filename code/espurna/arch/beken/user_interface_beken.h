#pragma once

#include <Arduino.h>

// esp8266 user_interface.h names used by the common code

#ifndef REASON_DEFAULT_RST
#define REASON_DEFAULT_RST 0
#define REASON_WDT_RST 1
#define REASON_EXCEPTION_RST 2
#define REASON_SOFT_WDT_RST 3
#define REASON_SOFT_RESTART 4
#define REASON_DEEP_SLEEP_AWAKE 5
#define REASON_EXT_SYS_RST 6
#endif

// Same layout as esp8266, crash.cpp stores it (nothing fills it on this chip)
struct rst_info {
    uint32_t reason;
    uint32_t exccause;
    uint32_t epc1;
    uint32_t epc2;
    uint32_t epc3;
    uint32_t excvaddr;
    uint32_t depc;
};

#define system_get_free_heap_size() lt_heap_get_free()
#define system_restart() lt_reboot()
#define system_get_sdk_version() lt_get_version()
#define system_get_cpu_freq() ((uint8_t)lt_cpu_get_freq_mhz())
#define system_get_chip_id() (ESP.getChipId())

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
