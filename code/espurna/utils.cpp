/*

UTILS MODULE

Copyright (C) 2017-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#include "board.h"
#include "ntp.h"

bool tryParseId(const char* p, TryParseIdFunc limit, size_t& out) {
    static_assert(std::numeric_limits<size_t>::max() >= std::numeric_limits<unsigned long>::max(), "");

    char* endp { nullptr };
    out = strtoul(p, &endp, 10);
    if ((endp == p) || (*endp != '\0') || (out >= limit())) {
        return false;
    }

    return true;
}

void setDefaultHostname() {
    if (strlen(HOSTNAME) > 0) {
        setSetting("hostname", F(HOSTNAME));
    } else {
        setSetting("hostname", getIdentifier());
    }
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

const char* getVersion() {
    static const char version[] {
#if defined(APP_REVISION)
        APP_VERSION APP_REVISION
#else
        APP_VERSION
#endif
    };

    return version;
}

const char* getAppName() {
    static const char app[] = APP_NAME;
    return app;
}

const char* getAppAuthor() {
    static const char author[] = APP_AUTHOR;
    return author;
}

const char* getAppWebsite() {
    static const char website[] = APP_WEBSITE;
    return website;
}

const char* getDevice() {
    static const char device[] = DEVICE;
    return device;
}

const char* getManufacturer() {
    static const char manufacturer[] = MANUFACTURER;
    return manufacturer;
}

String buildTime() {
#if NTP_SUPPORT
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

bool isNumber(const String& value) {
    if (value.length()) {
        const char* begin { value.c_str() };
        const char* end { value.c_str() + value.length() };

        bool dot { false };
        bool digit { false };
        const char* ptr { begin };

        while (ptr != end) {
            switch (*ptr) {
            case '\0':
                break;
            case '-':
            case '+':
                if (ptr != begin) {
                    return false;
                }
                break;
            case '.':
                if (dot) {
                    return false;
                }
                dot = true;
                break;
            case '0' ... '9':
                digit = true;
                break;
            case 'a' ... 'z':
            case 'A' ... 'Z':
                return false;
            }

            ++ptr;
        }

        return digit;
    }

    return false;
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

const char* getFlashChipMode() {
    const char* mode { nullptr };
    if (!mode) {
        switch (ESP.getFlashChipMode()) {
        case FM_QIO:
            mode = "QIO";
            break;
        case FM_QOUT:
            mode = "QOUT";
            break;
        case FM_DIO:
            mode = "DIO";
            break;
        case FM_DOUT:
            mode = "DOUT";
            break;
        case FM_UNKNOWN:
        default:
            mode = "UNKNOWN";
            break;
        }
    }

    return mode;
}
