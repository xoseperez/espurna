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

String getDescription() {
    return getSetting("desc");
}

String getHostname() {
    if (strlen(HOSTNAME) > 0) {
        return getSetting("hostname", F(HOSTNAME));
    }

    return getSetting("hostname", getIdentifier());
}

void setDefaultHostname() {
    if (!getSetting("hostname").length()) {
        if (strlen(HOSTNAME) > 0) {
            setSetting("hostname", F(HOSTNAME));
        } else {
            setSetting("hostname", getIdentifier());
        }
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
    static const char version[] = APP_VERSION;
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

String prettyDuration(espurna::duration::Seconds seconds) {
    time_t timestamp = static_cast<time_t>(seconds.count());
    tm spec;
    gmtime_r(&timestamp, &spec);

    char buffer[64];
    sprintf_P(buffer, PSTR("%02dy %02dd %02dh %02dm %02ds"),
        (spec.tm_year - 70), spec.tm_yday, spec.tm_hour,
        spec.tm_min, spec.tm_sec);

    return String(buffer);
}

String getUptime() {
#if NTP_SUPPORT
    return prettyDuration(systemUptime());
#else
    return String(systemUptime().count(), 10);
#endif
}

String buildTime() {
#if NTP_SUPPORT
    constexpr const time_t ts = __UNIX_TIMESTAMP__;
    tm timestruct;
    gmtime_r(&ts, &timestruct);
    return ntpDateTime(&timestruct);
#else
    char buffer[32];
    snprintf_P(
        buffer, sizeof(buffer), PSTR("%04d-%02d-%02d %02d:%02d:%02d"),
        __TIME_YEAR__, __TIME_MONTH__, __TIME_DAY__,
        __TIME_HOUR__, __TIME_MINUTE__, __TIME_SECOND__
    );
    return String(buffer);
#endif
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

double roundTo(double num, unsigned char positions) {
    double multiplier = 1;
    while (positions-- > 0) multiplier *= 10;
    return round(num * multiplier) / multiplier;
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

namespace {

// From a byte array to an hexa char array ("A220EE...", double the size)

template <typename T>
const uint8_t* hexEncodeImpl(const uint8_t* in_begin, const uint8_t* in_end, T&& callback) {
    static const char base16[] = "0123456789ABCDEF";

    constexpr uint8_t Left { 0xf0 };
    constexpr uint8_t Right { 0xf };
    constexpr uint8_t Shift { 4 };

    auto* in_ptr = in_begin;
    for (; in_ptr != in_end; ++in_ptr) {
        char buf[2] {
            base16[((*in_ptr) & Left) >> Shift],
            base16[(*in_ptr) & Right]};
        if (!callback(buf)) {
            break;
        }
    }

    return in_ptr;
}

} // namespace

char* hexEncode(const uint8_t* in_begin, const uint8_t* in_end, char* out_begin, char* out_end) {
    char* out_ptr { out_begin };

    hexEncodeImpl(in_begin, in_end, [&](const char (&byte)[2]) {
        *(out_ptr) = byte[0];
        ++out_ptr;

        *(out_ptr) = byte[1];
        ++out_ptr;

        return out_ptr != out_end;
    });

    return out_ptr;
}

String hexEncode(const uint8_t* in_begin, const uint8_t* in_end) {
    String out;
    out.reserve(in_end - in_begin);

    hexEncodeImpl(in_begin, in_end, [&](const char (&byte)[2]) {
        out.concat(byte, 2);
        return true;
    });

    return out;
}

size_t hexEncode(const uint8_t* in, size_t in_size, char* out, size_t out_size) {
    if (out_size >= ((in_size * 2) + 1)) {
        char* out_ptr = hexEncode(in, in + in_size, out, out + out_size);
        *out_ptr = '\0';
        ++out_ptr;
        return out_ptr - out;
    }

    return 0;
}

// From an hexa char array ("A220EE...") to a byte array (half the size)

uint8_t* hexDecode(const char* in_begin, const char* in_end, uint8_t* out_begin, uint8_t* out_end) {
    // We can only return small values (max 'z' aka 122)
    constexpr uint8_t InvalidByte { 255u };

    auto char2byte = [](char ch) -> uint8_t {
        switch (ch) {
        case '0'...'9':
            return (ch - '0');
        case 'a'...'f':
            return 10 + (ch - 'a');
        case 'A'...'F':
            return 10 + (ch - 'A');
        }

        return InvalidByte;
    };

    constexpr uint8_t Shift { 4 };

    const char* in_ptr { in_begin };
    uint8_t* out_ptr { out_begin };
    while ((in_ptr != in_end) && (out_ptr != out_end)) {
        uint8_t lhs = char2byte(*in_ptr);
        if (lhs == InvalidByte) {
            break;
        }
        ++in_ptr;

        uint8_t rhs = char2byte(*in_ptr);
        if (rhs == InvalidByte) {
            break;
        }
        ++in_ptr;

        (*out_ptr) = (lhs << Shift) | rhs;
        ++out_ptr;
    }

    return out_ptr;
}

size_t hexDecode(const char* in, size_t in_size, uint8_t* out, size_t out_size) {
    if ((in_size & 1) || (out_size < (in_size / 2))) {
        return 0;
    }

    uint8_t* out_ptr { hexDecode(in, in + in_size, out, out + out_size) };
    return out_ptr - out;
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
