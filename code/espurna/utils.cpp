/*

UTILS MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <limits>

#include "utils.h"

#include "board.h"
#include "influxdb.h"
#include "light.h"
#include "mqtt.h"
#include "ntp.h"
#include "relay.h"
#include "thermostat.h"

#include "libs/TypeChecks.h"


//--------------------------------------------------------------------------------
// Reset reasons
//--------------------------------------------------------------------------------

PROGMEM const char custom_reset_hardware[] = "Hardware button";
PROGMEM const char custom_reset_web[] = "Reboot from web interface";
PROGMEM const char custom_reset_terminal[] = "Reboot from terminal";
PROGMEM const char custom_reset_mqtt[] = "Reboot from MQTT";
PROGMEM const char custom_reset_rpc[] = "Reboot from RPC";
PROGMEM const char custom_reset_ota[] = "Reboot after successful OTA update";
PROGMEM const char custom_reset_http[] = "Reboot from HTTP";
PROGMEM const char custom_reset_nofuss[] = "Reboot after successful NoFUSS update";
PROGMEM const char custom_reset_upgrade[] = "Reboot after successful web update";
PROGMEM const char custom_reset_factory[] = "Factory reset";
PROGMEM const char* const custom_reset_string[] = {
    custom_reset_hardware, custom_reset_web, custom_reset_terminal,
    custom_reset_mqtt, custom_reset_rpc, custom_reset_ota,
    custom_reset_http, custom_reset_nofuss, custom_reset_upgrade,
    custom_reset_factory
};

void setDefaultHostname() {
    if (strlen(HOSTNAME) > 0) {
        setSetting("hostname", F(HOSTNAME));
    } else {
        setSetting("hostname", getIdentifier());
    }
}

const String& getDevice() {
    static const String value(F(DEVICE));
    return value;
}

const String& getManufacturer() {
    static const String value(F(MANUFACTURER));
    return value;
}

String getBoardName() {
    static const String defaultValue(F(DEVICE_NAME));
    return getSetting("boardName", defaultValue);
}

void setBoardName() {
    if (!isEspurnaCore()) {
        setSetting("boardName", F(DEVICE_NAME));
    }
}

String getAdminPass() {
    static const String defaultValue(F(ADMIN_PASS));
    return getSetting("adminPass", defaultValue);
}

const String& getCoreVersion() {
    static String version;
    if (!version.length()) {
        #ifdef ARDUINO_ESP8266_RELEASE
            version = ESP.getCoreVersion();
            if (version.equals("00000000")) {
                version = String(ARDUINO_ESP8266_RELEASE);
            }
            version.replace("_", ".");
        #else
            #define _GET_COREVERSION_STR(X) #X
            #define GET_COREVERSION_STR(X) _GET_COREVERSION_STR(X)
            version = GET_COREVERSION_STR(ARDUINO_ESP8266_GIT_DESC);
            #undef _GET_COREVERSION_STR
            #undef GET_COREVERSION_STR
        #endif
    }
    return version;
}

const String& getCoreRevision() {
    static String revision;
    if (!revision.length()) {
        #ifdef ARDUINO_ESP8266_GIT_VER
            revision = String(ARDUINO_ESP8266_GIT_VER, 16);
        #else
            revision = "(unspecified)";
        #endif
    }
    return revision;
}

int getHeartbeatMode() {
    return getSetting("hbMode", HEARTBEAT_MODE);
}

unsigned long getHeartbeatInterval() {
    return getSetting("hbInterval", HEARTBEAT_INTERVAL);
}

String buildTime() {
    #if NTP_LEGACY_SUPPORT && NTP_SUPPORT
        return ntpDateTime(__UNIX_TIMESTAMP__);
    #elif NTP_SUPPORT
        constexpr const time_t ts = __UNIX_TIMESTAMP__;
        tm timestruct;
        gmtime_r(&ts, &timestruct);
        return ntpDateTime(&timestruct);
    #else
        char buffer[20];
        snprintf_P(
            buffer, sizeof(buffer), PSTR("%04d-%02d-%02d %02d:%02d:%02d"),
            __TIME_YEAR__, __TIME_MONTH__, __TIME_DAY__,
            __TIME_HOUR__, __TIME_MINUTE__, __TIME_SECOND__
        );
        return String(buffer);
    #endif
}

unsigned long getUptime() {

    static unsigned long last_uptime = 0;
    static unsigned char uptime_overflows = 0;

    if (millis() < last_uptime) ++uptime_overflows;
    last_uptime = millis();
    unsigned long uptime_seconds = uptime_overflows * (UPTIME_OVERFLOW / 1000) + (last_uptime / 1000);

    return uptime_seconds;

}

//--------------------------------------------------------------------------------
// Heap stats
//--------------------------------------------------------------------------------

namespace {

template <typename T>
using has_getHeapStats_t = decltype(std::declval<T>().getHeapStats(0,0,0));

template <typename T>
using has_getHeapStats = is_detected<has_getHeapStats_t, T>;

template <typename T>
void _getHeapStats(const std::true_type&, T& instance, heap_stats_t& stats) {
    instance.getHeapStats(&stats.available, &stats.usable, &stats.frag_pct);
}

template <typename T>
void _getHeapStats(const std::false_type&, T& instance, heap_stats_t& stats) {
    stats.available = instance.getFreeHeap();
    stats.usable = 0;
    stats.frag_pct = 0;
}

} // namespace anonymous

void getHeapStats(heap_stats_t& stats) {
    _getHeapStats(has_getHeapStats<decltype(ESP)>{}, ESP, stats);
}

// WTF
// Calling ESP.getFreeHeap() is making the system crash on a specific
// AiLight bulb, but anywhere else it should work as expected
static bool _heap_value_wtf = false;

heap_stats_t getHeapStats() {
    heap_stats_t stats;
    if (_heap_value_wtf) {
        stats.available = 9999;
        stats.usable = 9999;
        stats.frag_pct = 0;
        return stats;
    }
    getHeapStats(stats);
    return stats;
}

void wtfHeap(bool value) {
    _heap_value_wtf = value;
}

unsigned int getFreeHeap() {
    return ESP.getFreeHeap();
}

// TODO: place in struct ctor to run at the earliest opportunity
static unsigned int _initial_heap_value = 0;
void setInitialFreeHeap() {
    _initial_heap_value = getFreeHeap();
}

unsigned int getInitialFreeHeap() {
    if (0 == _initial_heap_value) {
        setInitialFreeHeap();
    }
    return _initial_heap_value;
}

// -----------------------------------------------------------------------------
// Heartbeat helper
// -----------------------------------------------------------------------------
namespace Heartbeat {

    enum Report : uint32_t { 
        Status = 1 << 1,
        Ssid = 1 << 2,
        Ip = 1 << 3,
        Mac = 1 << 4,
        Rssi = 1 << 5,
        Uptime = 1 << 6,
        Datetime = 1 << 7,
        Freeheap = 1 << 8,
        Vcc = 1 << 9,
        Relay = 1 << 10,
        Light = 1 << 11,
        Hostname = 1 << 12,
        App = 1 << 13,
        Version = 1 << 14,
        Board = 1 << 15,
        Loadavg = 1 << 16,
        Interval = 1 << 17,
        Description = 1 << 18,
        Range = 1 << 19,
        RemoteTemp = 1 << 20,
        Bssid = 1 << 21
    };

    constexpr uint32_t defaultValue() {
        return (Status * (HEARTBEAT_REPORT_STATUS)) | \
            (Ssid * (HEARTBEAT_REPORT_SSID)) | \
            (Ip * (HEARTBEAT_REPORT_IP)) | \
            (Mac * (HEARTBEAT_REPORT_MAC)) | \
            (Rssi * (HEARTBEAT_REPORT_RSSI)) | \
            (Uptime * (HEARTBEAT_REPORT_UPTIME)) | \
            (Datetime * (HEARTBEAT_REPORT_DATETIME)) | \
            (Freeheap * (HEARTBEAT_REPORT_FREEHEAP)) | \
            (Vcc * (HEARTBEAT_REPORT_VCC)) | \
            (Relay * (HEARTBEAT_REPORT_RELAY)) | \
            (Light * (HEARTBEAT_REPORT_LIGHT)) | \
            (Hostname * (HEARTBEAT_REPORT_HOSTNAME)) | \
            (Description * (HEARTBEAT_REPORT_DESCRIPTION)) | \
            (App * (HEARTBEAT_REPORT_APP)) | \
            (Version * (HEARTBEAT_REPORT_VERSION)) | \
            (Board * (HEARTBEAT_REPORT_BOARD)) | \
            (Loadavg * (HEARTBEAT_REPORT_LOADAVG)) | \
            (Interval * (HEARTBEAT_REPORT_INTERVAL)) | \
            (Range * (HEARTBEAT_REPORT_RANGE)) | \
            (RemoteTemp * (HEARTBEAT_REPORT_REMOTE_TEMP)) | \
            (Bssid * (HEARTBEAT_REPORT_BSSID));
    }

    uint32_t currentValue() {
        // use default without any setting / when it is empty
        const auto value = getSetting("hbReport", defaultValue());

        // because we start shifting from 1, we could use the
        // first bit as a flag to enable all of the messages
        if (value == 1) {
            return std::numeric_limits<uint32_t>::max();
        }

        return value;
    }

}

void infoUptime() {
    const auto uptime [[gnu::unused]] = getUptime();
    #if NTP_SUPPORT
        DEBUG_MSG_P(
            PSTR("[MAIN] Uptime: %02dd %02dh %02dm %02ds\n"),
            elapsedDays(uptime), numberOfHours(uptime),
            numberOfMinutes(uptime), numberOfSeconds(uptime)
        );
    #else
        DEBUG_MSG_P(PSTR("[MAIN] Uptime: %lu seconds\n"), uptime);
    #endif // NTP_SUPPORT
}

void heartbeat() {

    auto heap_stats [[gnu::unused]] = getHeapStats();

    #if MQTT_SUPPORT
        unsigned char _heartbeat_mode = getHeartbeatMode();
        bool serial = !mqttConnected();
    #else
        bool serial = true;
    #endif

    // -------------------------------------------------------------------------
    // Serial
    // -------------------------------------------------------------------------

    if (serial) {
        infoUptime();
        infoHeapStats();
        if (ADC_MODE_VALUE == ADC_VCC) {
            DEBUG_MSG_P(PSTR("[MAIN] Power: %lu mV\n"), ESP.getVcc());
        }
        #if NTP_SUPPORT
            if (ntpSynced()) DEBUG_MSG_P(PSTR("[MAIN] Time: %s\n"), (char *) ntpDateTime().c_str());
        #endif
    }

    const uint32_t hb_cfg = Heartbeat::currentValue();
    if (!hb_cfg) return;

    // -------------------------------------------------------------------------
    // MQTT
    // -------------------------------------------------------------------------

    #if MQTT_SUPPORT
        if (!serial && (_heartbeat_mode == HEARTBEAT_REPEAT || systemGetHeartbeat())) {
            if (hb_cfg & Heartbeat::Interval)
                mqttSend(MQTT_TOPIC_INTERVAL, String(getHeartbeatInterval()).c_str());

            if (hb_cfg & Heartbeat::App)
                mqttSend(MQTT_TOPIC_APP, APP_NAME);

            if (hb_cfg & Heartbeat::Version)
                mqttSend(MQTT_TOPIC_VERSION, APP_VERSION);

            if (hb_cfg & Heartbeat::Board)
                mqttSend(MQTT_TOPIC_BOARD, getBoardName().c_str());

            if (hb_cfg & Heartbeat::Hostname)
                mqttSend(MQTT_TOPIC_HOSTNAME, getSetting("hostname", getIdentifier()).c_str());

            if (hb_cfg & Heartbeat::Description) {
                if (hasSetting("desc")) {
                    mqttSend(MQTT_TOPIC_DESCRIPTION, getSetting("desc").c_str());
                }
            }

            if (hb_cfg & Heartbeat::Ssid)
                mqttSend(MQTT_TOPIC_SSID, WiFi.SSID().c_str());

            if (hb_cfg & Heartbeat::Bssid)
                mqttSend(MQTT_TOPIC_BSSID, WiFi.BSSIDstr().c_str());

            if (hb_cfg & Heartbeat::Ip)
                mqttSend(MQTT_TOPIC_IP, getIP().c_str());

            if (hb_cfg & Heartbeat::Mac)
                mqttSend(MQTT_TOPIC_MAC, WiFi.macAddress().c_str());

            if (hb_cfg & Heartbeat::Rssi)
                mqttSend(MQTT_TOPIC_RSSI, String(WiFi.RSSI()).c_str());

            if (hb_cfg & Heartbeat::Uptime)
                mqttSend(MQTT_TOPIC_UPTIME, String(getUptime()).c_str());

            #if NTP_SUPPORT
                if ((hb_cfg & Heartbeat::Datetime) && (ntpSynced()))
                    mqttSend(MQTT_TOPIC_DATETIME, ntpDateTime().c_str());
            #endif

            if (hb_cfg & Heartbeat::Freeheap)
                mqttSend(MQTT_TOPIC_FREEHEAP, String(heap_stats.available).c_str());

            if (hb_cfg & Heartbeat::Relay)
                relayMQTT();

            #if (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE)
                if (hb_cfg & Heartbeat::Light)
                    lightMQTT();
            #endif

            if ((hb_cfg & Heartbeat::Vcc) && (ADC_MODE_VALUE == ADC_VCC))
                mqttSend(MQTT_TOPIC_VCC, String(ESP.getVcc()).c_str());

            if (hb_cfg & Heartbeat::Status)
                mqttSendStatus();

            if (hb_cfg & Heartbeat::Loadavg)
                mqttSend(MQTT_TOPIC_LOADAVG, String(systemLoadAverage()).c_str());

            #if THERMOSTAT_SUPPORT
                if (hb_cfg & Heartbeat::Range) {
                    const auto& range = thermostatRange();
                    mqttSend(MQTT_TOPIC_HOLD_TEMP "_" MQTT_TOPIC_HOLD_TEMP_MIN, String(range.min).c_str());
                    mqttSend(MQTT_TOPIC_HOLD_TEMP "_" MQTT_TOPIC_HOLD_TEMP_MAX, String(range.max).c_str());
                }

                if (hb_cfg & Heartbeat::RemoteTemp) {
                    const auto& remote_temp = thermostatRemoteTemp();
                    char buffer[16];
                    dtostrf(remote_temp.temp, 1, 1, buffer);
                    mqttSend(MQTT_TOPIC_REMOTE_TEMP, buffer);
                }
            #endif

        } else if (!serial && _heartbeat_mode == HEARTBEAT_REPEAT_STATUS) {
            mqttSendStatus();
        }

    #endif

    // -------------------------------------------------------------------------
    // InfluxDB
    // -------------------------------------------------------------------------

    #if INFLUXDB_SUPPORT
        if (hb_cfg & Heartbeat::Uptime)
            idbSend(MQTT_TOPIC_UPTIME, String(getUptime()).c_str());

        if (hb_cfg & Heartbeat::Freeheap)
            idbSend(MQTT_TOPIC_FREEHEAP, String(heap_stats.available).c_str());

        if (hb_cfg & Heartbeat::Rssi)
            idbSend(MQTT_TOPIC_RSSI, String(WiFi.RSSI()).c_str());

        if ((hb_cfg & Heartbeat::Vcc) && (ADC_MODE_VALUE == ADC_VCC))
            idbSend(MQTT_TOPIC_VCC, String(ESP.getVcc()).c_str());
                    
        if (hb_cfg & Heartbeat::Loadavg)
            idbSend(MQTT_TOPIC_LOADAVG, String(systemLoadAverage()).c_str());

        if (hb_cfg & Heartbeat::Ssid)
            idbSend(MQTT_TOPIC_SSID, WiFi.SSID().c_str());

        if (hb_cfg & Heartbeat::Bssid)
            idbSend(MQTT_TOPIC_BSSID, WiFi.BSSIDstr().c_str());
    #endif

}

// -----------------------------------------------------------------------------
// INFO
// -----------------------------------------------------------------------------

extern "C" uint32_t _SPIFFS_start;
extern "C" uint32_t _SPIFFS_end;

unsigned int info_bytes2sectors(size_t size) {
    return (int) (size + SPI_FLASH_SEC_SIZE - 1) / SPI_FLASH_SEC_SIZE;
}

unsigned long info_ota_space() {
    return (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
}

unsigned long info_filesystem_space() {
    return ((uint32_t)&_SPIFFS_end - (uint32_t)&_SPIFFS_start);
}

unsigned long info_eeprom_space() {
    return EEPROMr.reserved() * SPI_FLASH_SEC_SIZE;
}

void _info_print_memory_layout_line(const char * name, unsigned long bytes, bool reset) {
    static unsigned long index = 0;
    if (reset) index = 0;
    if (0 == bytes) return;
    unsigned int _sectors = info_bytes2sectors(bytes);
    DEBUG_MSG_P(PSTR("[MAIN] %-20s: %8lu bytes / %4d sectors (%4d to %4d)\n"), name, bytes, _sectors, index, index + _sectors - 1);
    index += _sectors;
}

void _info_print_memory_layout_line(const char * name, unsigned long bytes) {
    _info_print_memory_layout_line(name, bytes, false);
}

void infoMemory(const char * name, unsigned int total_memory, unsigned int free_memory) {

    DEBUG_MSG_P(
        PSTR("[MAIN] %-6s: %5u bytes initially | %5u bytes used (%2u%%) | %5u bytes free (%2u%%)\n"),
        name,
        total_memory,
        total_memory - free_memory,
        100 * (total_memory - free_memory) / total_memory,
        free_memory,
        100 * free_memory / total_memory
    );

}

void infoMemory(const char* name, const heap_stats_t& stats) {
    infoMemory(name, getInitialFreeHeap(), stats.available);
}

void infoHeapStats(const char* name, const heap_stats_t& stats) {
    DEBUG_MSG_P(
        PSTR("[MAIN] %-6s: %5u contiguous bytes available (%u%% fragmentation)\n"),
        name,
        stats.usable,
        stats.frag_pct
    );
}

void infoHeapStats(bool show_frag_stats) {
    const auto stats = getHeapStats();
    infoMemory("Heap", stats);
    if (show_frag_stats && has_getHeapStats<decltype(ESP)>{}) {
        infoHeapStats("Heap", stats);
    }
}

const char* _info_wifi_sleep_mode(WiFiSleepType_t type) {
    switch (type) {
        case WIFI_NONE_SLEEP: return "NONE";
        case WIFI_LIGHT_SLEEP: return "LIGHT";
        case WIFI_MODEM_SLEEP: return "MODEM";
        default: return "UNKNOWN";
    }
}


void info(bool first) {

    // Avoid printing on early boot when buffering is enabled
    #if DEBUG_SUPPORT

    #if DEBUG_LOG_BUFFER_SUPPORT
        if (first && debugLogBuffer()) return;
    #endif

    DEBUG_MSG_P(PSTR("\n\n---8<-------\n\n"));

    // -------------------------------------------------------------------------

    #if defined(APP_REVISION)
        DEBUG_MSG_P(PSTR("[MAIN] " APP_NAME " " APP_VERSION " (" APP_REVISION ")\n"));
    #else
        DEBUG_MSG_P(PSTR("[MAIN] " APP_NAME " " APP_VERSION "\n"));
    #endif
    DEBUG_MSG_P(PSTR("[MAIN] " APP_AUTHOR "\n"));
    DEBUG_MSG_P(PSTR("[MAIN] " APP_WEBSITE "\n\n"));
    DEBUG_MSG_P(PSTR("[MAIN] CPU chip ID: 0x%06X\n"), ESP.getChipId());
    DEBUG_MSG_P(PSTR("[MAIN] CPU frequency: %u MHz\n"), ESP.getCpuFreqMHz());
    DEBUG_MSG_P(PSTR("[MAIN] SDK version: %s\n"), ESP.getSdkVersion());
    DEBUG_MSG_P(PSTR("[MAIN] Core version: %s\n"), getCoreVersion().c_str());
    DEBUG_MSG_P(PSTR("[MAIN] Core revision: %s\n"), getCoreRevision().c_str());
    DEBUG_MSG_P(PSTR("[MAIN] Build time: %lu\n"), __UNIX_TIMESTAMP__);
    DEBUG_MSG_P(PSTR("\n"));

    // -------------------------------------------------------------------------

    FlashMode_t mode [[gnu::unused]] = ESP.getFlashChipMode();

    DEBUG_MSG_P(PSTR("[MAIN] Flash chip ID: 0x%06X\n"), ESP.getFlashChipId());
    DEBUG_MSG_P(PSTR("[MAIN] Flash speed: %u Hz\n"), ESP.getFlashChipSpeed());
    DEBUG_MSG_P(PSTR("[MAIN] Flash mode: %s\n"), mode == FM_QIO ? "QIO" : mode == FM_QOUT ? "QOUT" : mode == FM_DIO ? "DIO" : mode == FM_DOUT ? "DOUT" : "UNKNOWN");
    DEBUG_MSG_P(PSTR("\n"));

    // -------------------------------------------------------------------------

    _info_print_memory_layout_line("Flash size (CHIP)", ESP.getFlashChipRealSize(), true);
    _info_print_memory_layout_line("Flash size (SDK)", ESP.getFlashChipSize(), true);
    _info_print_memory_layout_line("Reserved", 1 * SPI_FLASH_SEC_SIZE, true);
    _info_print_memory_layout_line("Firmware size", ESP.getSketchSize());
    _info_print_memory_layout_line("Max OTA size", info_ota_space());
    _info_print_memory_layout_line("SPIFFS size", info_filesystem_space());
    _info_print_memory_layout_line("EEPROM size", info_eeprom_space());
    _info_print_memory_layout_line("Reserved", 4 * SPI_FLASH_SEC_SIZE);
    DEBUG_MSG_P(PSTR("\n"));

    // -------------------------------------------------------------------------

    #if SPIFFS_SUPPORT
        FSInfo fs_info;
        bool fs = SPIFFS.info(fs_info);
        if (fs) {
            DEBUG_MSG_P(PSTR("[MAIN] SPIFFS total size   : %8u bytes / %4d sectors\n"), fs_info.totalBytes, info_bytes2sectors(fs_info.totalBytes));
            DEBUG_MSG_P(PSTR("[MAIN]        used size    : %8u bytes\n"), fs_info.usedBytes);
            DEBUG_MSG_P(PSTR("[MAIN]        block size   : %8u bytes\n"), fs_info.blockSize);
            DEBUG_MSG_P(PSTR("[MAIN]        page size    : %8u bytes\n"), fs_info.pageSize);
            DEBUG_MSG_P(PSTR("[MAIN]        max files    : %8u\n"), fs_info.maxOpenFiles);
            DEBUG_MSG_P(PSTR("[MAIN]        max length   : %8u\n"), fs_info.maxPathLength);
        } else {
            DEBUG_MSG_P(PSTR("[MAIN] No SPIFFS partition\n"));
        }
        DEBUG_MSG_P(PSTR("\n"));
    #endif

    // -------------------------------------------------------------------------

    eepromSectorsDebug();
    DEBUG_MSG_P(PSTR("\n"));

    // -------------------------------------------------------------------------

    infoMemory("EEPROM", SPI_FLASH_SEC_SIZE, SPI_FLASH_SEC_SIZE - settingsSize());
    infoHeapStats(!first);
    infoMemory("Stack", CONT_STACKSIZE, getFreeStack());
    DEBUG_MSG_P(PSTR("\n"));

    // -------------------------------------------------------------------------

    DEBUG_MSG_P(PSTR("[MAIN] Boot version: %d\n"), ESP.getBootVersion());
    DEBUG_MSG_P(PSTR("[MAIN] Boot mode: %d\n"), ESP.getBootMode());
    unsigned char reason = customResetReason();
    if (reason > 0) {
        char buffer[32];
        strcpy_P(buffer, custom_reset_string[reason-1]);
        DEBUG_MSG_P(PSTR("[MAIN] Last reset reason: %s\n"), buffer);
    } else {
        DEBUG_MSG_P(PSTR("[MAIN] Last reset reason: %s\n"), (char *) ESP.getResetReason().c_str());
        DEBUG_MSG_P(PSTR("[MAIN] Last reset info: %s\n"), (char *) ESP.getResetInfo().c_str());
    }
    DEBUG_MSG_P(PSTR("\n"));

    // -------------------------------------------------------------------------

    DEBUG_MSG_P(PSTR("[MAIN] Board: %s\n"), getBoardName().c_str());
    DEBUG_MSG_P(PSTR("[MAIN] Support: %s\n"), getEspurnaModules().c_str());
    DEBUG_MSG_P(PSTR("[MAIN] OTA: %s\n"), getEspurnaOTAModules().c_str());
    #if SENSOR_SUPPORT
        DEBUG_MSG_P(PSTR("[MAIN] Sensors: %s\n"), getEspurnaSensors().c_str());
    #endif // SENSOR_SUPPORT
    DEBUG_MSG_P(PSTR("[MAIN] WebUI image: %s\n"), getEspurnaWebUI().c_str());
    DEBUG_MSG_P(PSTR("\n"));

    // -------------------------------------------------------------------------

    if (!first) {
        DEBUG_MSG_P(PSTR("[MAIN] Firmware MD5: %s\n"), (char *) ESP.getSketchMD5().c_str());
    }

    if (ADC_MODE_VALUE == ADC_VCC) {
        DEBUG_MSG_P(PSTR("[MAIN] Power: %u mV\n"), ESP.getVcc());
    }
    if (espurnaLoopDelay()) {
        DEBUG_MSG_P(PSTR("[MAIN] Power saving delay value: %lu ms\n"), espurnaLoopDelay());
    }

    const WiFiSleepType_t sleep_mode = WiFi.getSleepMode();
    if (sleep_mode != WIFI_NONE_SLEEP) {
        DEBUG_MSG_P(PSTR("[MAIN] WiFi Sleep Mode: %s\n"), _info_wifi_sleep_mode(sleep_mode));
    }

    // -------------------------------------------------------------------------

    #if SYSTEM_CHECK_ENABLED
        if (!systemCheck()) {
            DEBUG_MSG_P(PSTR("\n"));
            DEBUG_MSG_P(PSTR("[MAIN] Device is in SAFE MODE\n"));
        }
    #endif

    // -------------------------------------------------------------------------

    DEBUG_MSG_P(PSTR("\n\n---8<-------\n\n"));

    #endif // DEBUG_SUPPORT == 1

}

// -----------------------------------------------------------------------------
// SSL
// -----------------------------------------------------------------------------

bool sslCheckFingerPrint(const char * fingerprint) {
    return (strlen(fingerprint) == 59);
}

bool sslFingerPrintArray(const char * fingerprint, unsigned char * bytearray) {

    // check length (20 2-character digits ':' or ' ' separated => 20*2+19 = 59)
    if (!sslCheckFingerPrint(fingerprint)) return false;

    // walk the fingerprint
    for (unsigned int i=0; i<20; i++) {
        bytearray[i] = strtol(fingerprint + 3*i, NULL, 16);
    }

    return true;

}

bool sslFingerPrintChar(const char * fingerprint, char * destination) {

    // check length (20 2-character digits ':' or ' ' separated => 20*2+19 = 59)
    if (!sslCheckFingerPrint(fingerprint)) return false;

    // copy it
    strncpy(destination, fingerprint, 59);

    // walk the fingerprint replacing ':' for ' '
    for (unsigned char i = 0; i<59; i++) {
        if (destination[i] == ':') destination[i] = ' ';
    }

    return true;

}

// -----------------------------------------------------------------------------
// Reset
// -----------------------------------------------------------------------------

// Use fixed method for Core 2.3.0, because it erases only 2 out of 4 SDK-reserved sectors
// Fixed since 2.4.0, see: esp8266/core/esp8266/Esp.cpp: ESP::eraseConfig()
bool eraseSDKConfig() {
    #if defined(ARDUINO_ESP8266_RELEASE_2_3_0)
        constexpr size_t cfgsize = 0x4000;
        size_t cfgaddr = ESP.getFlashChipSize() - cfgsize;

        for (size_t offset = 0; offset < cfgsize; offset += SPI_FLASH_SEC_SIZE) {
            if (!ESP.flashEraseSector((cfgaddr + offset) / SPI_FLASH_SEC_SIZE)) {
                return false;
            }
        }

        return true;
    #else
        return ESP.eraseConfig();
    #endif
}

// -----------------------------------------------------------------------------
// Helper functions
// -----------------------------------------------------------------------------

char * ltrim(char * s) {
    char *p = s;
    while ((unsigned char) *p == ' ') ++p;
    return p;
}

double roundTo(double num, unsigned char positions) {
    double multiplier = 1;
    while (positions-- > 0) multiplier *= 10;
    return round(num * multiplier) / multiplier;
}

void nice_delay(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) delay(1);
}

// This method is called by the SDK to know where to connect the ADC
int __get_adc_mode() {
    return (int) (ADC_MODE_VALUE);
}

bool isNumber(const char * s) {
    unsigned char len = strlen(s);
    if (0 == len) return false;
    bool decimal = false;
    bool digit = false;
    for (unsigned char i=0; i<len; i++) {
        if (('-' == s[i]) || ('+' == s[i])) {
            if (i>0) return false;
        } else if (s[i] == '.') {
            if (!digit) return false;
            if (decimal) return false;
            decimal = true;
        } else if (!isdigit(s[i])) {
            return false;
        } else {
            digit = true;
        }
    }
    return digit;
}

// ref: lwip2 lwip_strnstr with strnlen
char* strnstr(const char* buffer, const char* token, size_t n) {
  size_t token_len = strnlen(token, n);
  if (token_len == 0) {
    return const_cast<char*>(buffer);
  }

  for (const char* p = buffer; *p && (p + token_len <= buffer + n); p++) {
    if ((*p == *token) && (strncmp(p, token, token_len) == 0)) {
      return const_cast<char*>(p);
    }
  }

  return nullptr;
}
