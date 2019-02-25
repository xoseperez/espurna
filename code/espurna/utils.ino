/*

UTILS MODULE

Copyright (C) 2017-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <Ticker.h>
Ticker _defer_reset;

uint8_t _reset_reason = 0;

String getIdentifier() {
    char buffer[20];
    snprintf_P(buffer, sizeof(buffer), PSTR("%s-%06X"), APP_NAME, ESP.getChipId());
    return String(buffer);
}

void setDefaultHostname() {
    if (strlen(HOSTNAME) > 0) {
        setSetting("hostname", HOSTNAME);
    } else {
        setSetting("hostname", getIdentifier());
    }
}

void setBoardName() {
    #ifndef ESPURNA_CORE
        setSetting("boardName", DEVICE_NAME);
    #endif
}

String getBoardName() {
    return getSetting("boardName", DEVICE_NAME);
}

String getAdminPass() {
    return getSetting("adminPass", ADMIN_PASS);
}

String getCoreVersion() {
    String version = ESP.getCoreVersion();
    #ifdef ARDUINO_ESP8266_RELEASE
        if (version.equals("00000000")) {
            version = String(ARDUINO_ESP8266_RELEASE);
        }
    #endif
    version.replace("_", ".");
    return version;
}

String getCoreRevision() {
    #ifdef ARDUINO_ESP8266_GIT_VER
        return String(ARDUINO_ESP8266_GIT_VER);
    #else
        return String("");
    #endif
}

unsigned char getHeartbeatMode() {
    return getSetting("hbMode", HEARTBEAT_MODE).toInt();
}

unsigned char getHeartbeatInterval() {
    return getSetting("hbInterval", HEARTBEAT_INTERVAL).toInt();
}

// WTF
// Calling ESP.getFreeHeap() is making the system crash on a specific
// AiLight bulb, but anywhere else...
unsigned int getFreeHeap() {
    if (getSetting("wtfHeap", 0).toInt() == 1) return 9999;
    return ESP.getFreeHeap();
}

unsigned int getInitialFreeHeap() {
    static unsigned int _heap = 0;
    if (0 == _heap) {
        _heap = getFreeHeap();
    }
    return _heap;
}

unsigned int getUsedHeap() {
    return getInitialFreeHeap() - getFreeHeap();
}

String getEspurnaModules() {
    return FPSTR(espurna_modules);
}

#if SENSOR_SUPPORT
String getEspurnaSensors() {
    return FPSTR(espurna_sensors);
}
#endif

String getEspurnaWebUI() {
    return FPSTR(espurna_webui);
}

String buildTime() {

    const char time_now[] = __TIME__;   // hh:mm:ss
    unsigned int hour = atoi(&time_now[0]);
    unsigned int minute = atoi(&time_now[3]);
    unsigned int second = atoi(&time_now[6]);

    const char date_now[] = __DATE__;   // Mmm dd yyyy
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    unsigned int month = 0;
    for ( int i = 0; i < 12; i++ ) {
        if (strncmp(date_now, months[i], 3) == 0 ) {
            month = i + 1;
            break;
        }
    }
    unsigned int day = atoi(&date_now[3]);
    unsigned int year = atoi(&date_now[7]);

    char buffer[20];
    snprintf_P(
        buffer, sizeof(buffer), PSTR("%04d-%02d-%02d %02d:%02d:%02d"),
        year, month, day, hour, minute, second
    );

    return String(buffer);

}


unsigned long getUptime() {

    static unsigned long last_uptime = 0;
    static unsigned char uptime_overflows = 0;

    if (millis() < last_uptime) ++uptime_overflows;
    last_uptime = millis();
    unsigned long uptime_seconds = uptime_overflows * (UPTIME_OVERFLOW / 1000) + (last_uptime / 1000);

    return uptime_seconds;

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
        Description = 1 << 18
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
            (Interval * (HEARTBEAT_REPORT_INTERVAL));
    }

    uint32_t currentValue() {
        const String cfg = getSetting("hbReport");
        if (!cfg.length()) return defaultValue();

        return strtoul(cfg.c_str(), NULL, 10);
    }

}

void heartbeat() {

    unsigned long uptime_seconds = getUptime();
    unsigned int free_heap = getFreeHeap();

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
        DEBUG_MSG_P(PSTR("[MAIN] Uptime: %lu seconds\n"), uptime_seconds);
        infoMemory("Heap", getInitialFreeHeap(), getFreeHeap());
        #if ADC_MODE_VALUE == ADC_VCC
            DEBUG_MSG_P(PSTR("[MAIN] Power: %lu mV\n"), ESP.getVcc());
        #endif
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
                mqttSend(MQTT_TOPIC_INTERVAL, String(getHeartbeatInterval() / 1000).c_str());

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

            if (hb_cfg & Heartbeat::Ip)
                mqttSend(MQTT_TOPIC_IP, getIP().c_str());

            if (hb_cfg & Heartbeat::Mac)
                mqttSend(MQTT_TOPIC_MAC, WiFi.macAddress().c_str());

            if (hb_cfg & Heartbeat::Rssi)
                mqttSend(MQTT_TOPIC_RSSI, String(WiFi.RSSI()).c_str());

            if (hb_cfg & Heartbeat::Uptime)
                mqttSend(MQTT_TOPIC_UPTIME, String(uptime_seconds).c_str());

            #if NTP_SUPPORT
                if ((hb_cfg & Heartbeat::Datetime) && (ntpSynced()))
                    mqttSend(MQTT_TOPIC_DATETIME, ntpDateTime().c_str());
            #endif

            if (hb_cfg & Heartbeat::Freeheap)
                mqttSend(MQTT_TOPIC_FREEHEAP, String(free_heap).c_str());

            if (hb_cfg & Heartbeat::Relay)
                relayMQTT();

            #if (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE)
                if (hb_cfg & Heartbeat::Light)
                    lightMQTT();
            #endif

            if ((hb_cfg & Heartbeat::Vcc) && (ADC_MODE_VALUE == ADC_VCC))
                mqttSend(MQTT_TOPIC_VCC, String(ESP.getVcc()).c_str());

            if (hb_cfg & Heartbeat::Status)
                mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, true);

            if (hb_cfg & Heartbeat::Loadavg)
                mqttSend(MQTT_TOPIC_LOADAVG, String(systemLoadAverage()).c_str());

        } else if (!serial && _heartbeat_mode == HEARTBEAT_REPEAT_STATUS) {
            mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, true);
        }

    #endif

    // -------------------------------------------------------------------------
    // InfluxDB
    // -------------------------------------------------------------------------

    #if INFLUXDB_SUPPORT
        if (hb_cfg & Heartbeat::Uptime)
            idbSend(MQTT_TOPIC_UPTIME, String(uptime_seconds).c_str());

        if (hb_cfg & Heartbeat::Freeheap)
            idbSend(MQTT_TOPIC_FREEHEAP, String(free_heap).c_str());

        if (hb_cfg & Heartbeat::Rssi)
            idbSend(MQTT_TOPIC_RSSI, String(WiFi.RSSI()).c_str());
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

const char* _info_wifi_sleep_mode(WiFiSleepType_t type) {
    switch (type) {
        case WIFI_NONE_SLEEP: return "NONE";
        case WIFI_LIGHT_SLEEP: return "LIGHT";
        case WIFI_MODEM_SLEEP: return "MODEM";
        default: return "UNKNOWN";
    }
}


void info() {

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
    DEBUG_MSG_P(PSTR("\n"));

    // -------------------------------------------------------------------------

    FlashMode_t mode = ESP.getFlashChipMode();
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
    infoMemory("Heap", getInitialFreeHeap(), getFreeHeap());
    infoMemory("Stack", 4096, getFreeStack());
    DEBUG_MSG_P(PSTR("\n"));

    // -------------------------------------------------------------------------

    DEBUG_MSG_P(PSTR("[MAIN] Boot version: %d\n"), ESP.getBootVersion());
    DEBUG_MSG_P(PSTR("[MAIN] Boot mode: %d\n"), ESP.getBootMode());
    unsigned char reason = resetReason();
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
    #if SENSOR_SUPPORT
        DEBUG_MSG_P(PSTR("[MAIN] Sensors: %s\n"), getEspurnaSensors().c_str());
    #endif // SENSOR_SUPPORT
    DEBUG_MSG_P(PSTR("[MAIN] WebUI image: %s\n"), getEspurnaWebUI().c_str());
    DEBUG_MSG_P(PSTR("\n"));

    // -------------------------------------------------------------------------

    DEBUG_MSG_P(PSTR("[MAIN] Firmware MD5: %s\n"), (char *) ESP.getSketchMD5().c_str());
    #if ADC_MODE_VALUE == ADC_VCC
        DEBUG_MSG_P(PSTR("[MAIN] Power: %u mV\n"), ESP.getVcc());
    #endif
    if (systemLoopDelay()) {
        DEBUG_MSG_P(PSTR("[MAIN] Power saving delay value: %lu ms\n"), systemLoopDelay());
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

}

// -----------------------------------------------------------------------------
// SSL
// -----------------------------------------------------------------------------

#if ASYNC_TCP_SSL_ENABLED

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

#endif

// -----------------------------------------------------------------------------
// Reset
// -----------------------------------------------------------------------------

unsigned char resetReason() {
    static unsigned char status = 255;
    if (status == 255) {
        status = EEPROMr.read(EEPROM_CUSTOM_RESET);
        if (status > 0) resetReason(0);
        if (status > CUSTOM_RESET_MAX) status = 0;
    }
    return status;
}

void resetReason(unsigned char reason) {
    _reset_reason = reason;
    EEPROMr.write(EEPROM_CUSTOM_RESET, reason);
    eepromCommit();
}

void reset() {
    ESP.restart();
}

void deferredReset(unsigned long delay, unsigned char reason) {
    _defer_reset.once_ms(delay, resetReason, reason);
}

bool checkNeedsReset() {
    return _reset_reason > 0;
}

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
