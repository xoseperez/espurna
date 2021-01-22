/*

UTILS MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#include "board.h"
#include "ntp.h"

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
    return getSetting("boardName", F(DEVICE_NAME));
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

const String& getVersion() {
    static const String value {
#if defined(APP_REVISION)
        F(APP_VERSION APP_REVISION)
#else
        F(APP_VERSION)
#endif
    };

    return value;
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

#if NTP_SUPPORT

String getUptime() {
    time_t uptime = systemUptime();
    tm spec;
    gmtime_r(&uptime, &spec);

    char buffer[64];
    sprintf_P(buffer, PSTR("%02dy %02dd %02dh %02dm %02ds"),
        (spec.tm_year - 70), spec.tm_yday, spec.tm_hour,
        spec.tm_min, spec.tm_sec
    );

    return String(buffer);
}

#else

String getUptime() {
    return String(systemUptime(), 10);
}

#endif // NTP_SUPPORT

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

void infoMemory(const char* name, const HeapStats& stats) {
    infoMemory(name, systemInitialFreeHeap(), stats.available);
}

void infoHeapStats(const char* name, const HeapStats& stats) {
    DEBUG_MSG_P(
        PSTR("[MAIN] %-6s: %5u contiguous bytes available (%u%% fragmentation)\n"),
        name,
        stats.usable,
        stats.frag_pct
    );
}

void infoHeapStats(bool show_frag_stats) {
    auto stats = systemHeapStats();
    infoMemory("Heap", stats);
    if (show_frag_stats) {
        infoHeapStats("Heap", stats);
    }
}

const char* _info_wifi_sleep_mode(WiFiSleepType_t type) {
    switch (type) {
    case WIFI_NONE_SLEEP:
        return "NONE";
    case WIFI_LIGHT_SLEEP:
        return "LIGHT";
    case WIFI_MODEM_SLEEP:
        return "MODEM";
    default:
        break;
    }

    return "UNKNOWN";
}

void info(bool first) {
#if DEBUG_SUPPORT
#if DEBUG_LOG_BUFFER_SUPPORT
    if (first && debugLogBuffer()) return;
#endif

    DEBUG_MSG_P(PSTR("\n\n---8<-------\n\n"));

    // -------------------------------------------------------------------------

    DEBUG_MSG_P(PSTR("[MAIN] " APP_NAME " %s\n"), getVersion().c_str());
    DEBUG_MSG_P(PSTR("[MAIN] " APP_AUTHOR "\n"));
    DEBUG_MSG_P(PSTR("[MAIN] " APP_WEBSITE "\n\n"));
    DEBUG_MSG_P(PSTR("[MAIN] CPU chip ID: 0x%06X\n"), ESP.getChipId());
    DEBUG_MSG_P(PSTR("[MAIN] CPU frequency: %u MHz\n"), ESP.getCpuFreqMHz());
    DEBUG_MSG_P(PSTR("[MAIN] SDK version: %s\n"), ESP.getSdkVersion());
    DEBUG_MSG_P(PSTR("[MAIN] Core version: %s\n"), getCoreVersion().c_str());
    DEBUG_MSG_P(PSTR("[MAIN] Core revision: %s\n"), getCoreRevision().c_str());
    DEBUG_MSG_P(PSTR("[MAIN] Built: %s\n"), buildTime().c_str());
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
    _info_print_memory_layout_line("EEPROM size", eepromSpace());
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
    infoMemory("Stack", CONT_STACKSIZE, systemFreeStack());
    DEBUG_MSG_P(PSTR("\n"));

    // -------------------------------------------------------------------------

    DEBUG_MSG_P(PSTR("[MAIN] Boot version: %d\n"), ESP.getBootVersion());
    DEBUG_MSG_P(PSTR("[MAIN] Boot mode: %d\n"), ESP.getBootMode());

    auto reason = customResetReason();
    if (CustomResetReason::None != reason) {
        DEBUG_MSG_P(PSTR("[MAIN] Last reset reason: %s\n"), customResetReasonToPayload(reason).c_str());
    } else {
        DEBUG_MSG_P(PSTR("[MAIN] Last reset reason: %s\n"), ESP.getResetReason().c_str());
        DEBUG_MSG_P(PSTR("[MAIN] Last reset info: %s\n"), ESP.getResetInfo().c_str());
    }
    DEBUG_MSG_P(PSTR("\n"));

    // -------------------------------------------------------------------------

    DEBUG_MSG_P(PSTR("[MAIN] Board: %s\n"), getBoardName().c_str());
    DEBUG_MSG_P(PSTR("[MAIN] Support: %s\n"), getEspurnaModules().c_str());
    DEBUG_MSG_P(PSTR("[MAIN] OTA: %s\n"), getEspurnaOTAModules().c_str());
#if SENSOR_SUPPORT
    DEBUG_MSG_P(PSTR("[MAIN] Sensors: %s\n"), getEspurnaSensors().c_str());
#endif
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
// Helper functions
// -----------------------------------------------------------------------------

char* ltrim(char * s) {
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

// From a byte array to an hexa char array ("A220EE...", double the size)
size_t hexEncode(const uint8_t * in, size_t in_size, char * out, size_t out_size) {
    if ((2 * in_size + 1) > (out_size)) return 0;

    static const char base16[] = "0123456789ABCDEF";
    size_t index = 0;

    while (index < in_size) {
        out[(index*2)]   = base16[(in[index] & 0xf0) >> 4];
        out[(index*2)+1] = base16[(in[index] & 0xf)];
        ++index;
    }

    out[2*index] = '\0';

    return index ? (1 + (2 * index)) : 0;
}


// From an hexa char array ("A220EE...") to a byte array (half the size)
size_t hexDecode(const char* in, size_t in_size, uint8_t* out, size_t out_size) {
    if ((in_size & 1) || (out_size < (in_size / 2))) {
        return 0;
    }

    // We can only return small values
    constexpr uint8_t InvalidByte { 255u };

    auto char2byte = [](char ch) -> uint8_t {
        if ((ch >= '0') && (ch <= '9')) {
            return (ch - '0');
        } else if ((ch >= 'a') && (ch <= 'f')) {
            return 10 + (ch - 'a');
        } else if ((ch >= 'A') && (ch <= 'F')) {
            return 10 + (ch - 'A');
        } else {
            return InvalidByte;
        }
    };

    size_t index = 0;
    size_t out_index = 0;

    while (index < in_size) {
        const uint8_t lhs = char2byte(in[index]) << 4;
        const uint8_t rhs = char2byte(in[index + 1]);
        if ((InvalidByte != lhs) && (InvalidByte != rhs)) {
            out[out_index++] = lhs | rhs;
            index += 2;
            continue;
        }
        out_index = 0;
        break;
    }

    return out_index;
}
