/*

BUILD INFO

*/

#include "espurna.h"
#include "utils.h"

#include <cstring>

//--------------------------------------------------------------------------------

namespace espurna {
namespace build {
namespace {

namespace sdk {

espurna::StringView base() {
    // aka `const char SDK_VERSION[]`
    return system_get_sdk_version();
}

espurna::StringView core_version() {
    static const String out = ([]() {
        String out;
#ifdef ARDUINO_ESP8266_RELEASE
        out = ESP.getCoreVersion();
        if (out.equals("00000000")) {
            out = String(ARDUINO_ESP8266_RELEASE);
        }
        out.replace('_', '.');
#else
#define _GET_COREVERSION_STR(X) #X
#define GET_COREVERSION_STR(X) _GET_COREVERSION_STR(X)
        out = GET_COREVERSION_STR(ARDUINO_ESP8266_GIT_DESC);
#undef _GET_COREVERSION_STR
#undef GET_COREVERSION_STR
#endif
        return out;
    })();

    return out;
}

espurna::StringView core_revision() {
    static const String out = ([]() {
#ifdef ARDUINO_ESP8266_GIT_VER
        return String(ARDUINO_ESP8266_GIT_VER, 16);
#else
        return PSTR("(unspecified)");
#endif
    })();

    return out;
}

Sdk get() {
    return Sdk{
        .base = base(),
        .version = core_version(),
        .revision = core_revision(),
    };
}

} // namespace sdk

namespace hardware {
namespace internal {

PROGMEM_STRING(Manufacturer, MANUFACTURER);
PROGMEM_STRING(Device, DEVICE);

} // namespace internal

constexpr StringView manufacturer() {
    return internal::Manufacturer;
}

constexpr StringView device() {
    return internal::Device;
}

constexpr Hardware get() {
    return Hardware{
        .manufacturer = manufacturer(),
        .device = device(),
    };
}

} // namespace device

namespace app {
namespace internal {

alignas(4) static constexpr char Modules[] PROGMEM_STRING_ATTR =
#if ALEXA_SUPPORT
    "ALEXA "
#endif
#if API_SUPPORT
    "API "
#endif
#if BUTTON_SUPPORT
    "BUTTON "
#endif
#if DEBUG_SERIAL_SUPPORT
    "DEBUG_SERIAL "
#endif
#if DEBUG_TELNET_SUPPORT
    "DEBUG_TELNET "
#endif
#if DEBUG_UDP_SUPPORT
    "DEBUG_UDP "
#endif
#if DEBUG_WEB_SUPPORT
    "DEBUG_WEB "
#endif
#if DOMOTICZ_SUPPORT
    "DOMOTICZ "
#endif
#if ENCODER_SUPPORT
    "ENCODER "
#endif
#if FAN_SUPPORT
    "FAN "
#endif
#if HOMEASSISTANT_SUPPORT
    "HOMEASSISTANT "
#endif
#if I2C_SUPPORT
    "I2C "
#endif
#if INFLUXDB_SUPPORT
    "INFLUXDB "
#endif
#if IR_SUPPORT
    "IR "
#endif
#if LED_SUPPORT
    "LED "
#endif
#if LLMNR_SUPPORT
    "LLMNR "
#endif
#if MDNS_SERVER_SUPPORT
    "MDNS "
#endif
#if MQTT_SUPPORT
    "MQTT "
#endif
#if NETBIOS_SUPPORT
    "NETBIOS "
#endif
#if NOFUSS_SUPPORT
    "NOFUSS "
#endif
#if NTP_SUPPORT
    "NTP "
#endif
#if OTA_ARDUINOOTA_SUPPORT
    "ARDUINO_OTA "
#endif
#if OTA_WEB_SUPPORT
    "OTA_WEB "
#endif
#if (OTA_CLIENT != OTA_CLIENT_NONE)
    "OTA_CLIENT "
#endif
#if PROMETHEUS_SUPPORT
    "METRICS "
#endif
#if RELAY_SUPPORT
    "RELAY "
#endif
#if RFM69_SUPPORT
    "RFM69 "
#endif
#if RFB_SUPPORT
    "RFB "
#endif
#if RPN_RULES_SUPPORT
    "RPN_RULES "
#endif
#if SCHEDULER_SUPPORT
    "SCHEDULER "
#endif
#if SENSOR_SUPPORT
    "SENSOR "
#endif
#if SPIFFS_SUPPORT
    "SPIFFS "
#endif
#if SSDP_SUPPORT
    "SSDP "
#endif
#if TELNET_SUPPORT
    "TELNET "
#endif
#if TERMINAL_SUPPORT
    "TERMINAL "
#endif
#if GARLAND_SUPPORT
    "GARLAND "
#endif
#if THERMOSTAT_SUPPORT
    "THERMOSTAT "
#endif
#if THERMOSTAT_DISPLAY_SUPPORT
    "THERMOSTAT_DISPLAY "
#endif
#if THINGSPEAK_SUPPORT
    "THINGSPEAK "
#endif
#if UART_MQTT_SUPPORT
    "UART_MQTT "
#endif
#if WEB_SUPPORT
#if WEBUI_IMAGE == WEBUI_IMAGE_SMALL
    "WEB_SMALL "
#elif WEBUI_IMAGE == WEBUI_IMAGE_LIGHT
    "WEB_LIGHT "
#elif WEBUI_IMAGE == WEBUI_IMAGE_SENSOR
    "WEB_SENSOR "
#elif WEBUI_IMAGE == WEBUI_IMAGE_RFBRIDGE
    "WEB_RFBRIDGE "
#elif WEBUI_IMAGE == WEBUI_IMAGE_RFM69
    "WEB_RFM69 "
#elif WEBUI_IMAGE == WEBUI_IMAGE_LIGHTFOX
    "WEB_LIGHTFOX "
#elif WEBUI_IMAGE == WEBUI_IMAGE_GARLAND
    "WEB_GARLAND "
#elif WEBUI_IMAGE == WEBUI_IMAGE_THERMOSTAT
    "WEB_THERMOSTAT "
#elif WEBUI_IMAGE == WEBUI_IMAGE_CURTAIN
    "WEB_CURTAIN "
#elif WEBUI_IMAGE == WEBUI_IMAGE_FULL
    "WEB_FULL "
#endif
#endif
    "";

PROGMEM_STRING(Name, APP_NAME);
PROGMEM_STRING(Version, APP_VERSION);
PROGMEM_STRING(Author, APP_AUTHOR);
PROGMEM_STRING(Website, APP_WEBSITE);

PROGMEM_STRING(BuildDate, __DATE__);
PROGMEM_STRING(BuildTime, __TIME__);

} // namespace internal

// ref. https://gcc.gnu.org/onlinedocs/cpp/Standard-Predefined-Macros.html
//
// __DATE__
// > This macro expands to a string constant that describes the date on which the preprocessor is being run.
// > The string constant contains eleven characters and looks like "Feb 12 1996". If the day of the month is less than 10,
// > it is padded with a space on the left.
// > 
// > If GCC cannot determine the current date, it will emit a warning message (once per compilation) and __DATE__ will expand to "??? ?? ????".
// 
// __TIME__
// > This macro expands to a string constant that describes the time at which the preprocessor is being run.
// > The string constant contains eight characters and looks like "23:59:01".
// >
// > If GCC cannot determine the current time, it will emit a warning message (once per compilation) and __TIME__ will expand to "??:??:??".
namespace time {

// "Jan  1 1970"
//  ^^^
constexpr StringView raw_month() {
    return StringView(&internal::BuildDate[0], &internal::BuildDate[3]);
}
static_assert(raw_month().length() == 3, "");

// "Jan  1 1970"
//      ^^ (with space, or without)
constexpr StringView raw_day() {
    return (internal::BuildDate[4] == ' ')
        ? StringView(&internal::BuildDate[5], &internal::BuildDate[6])
        : StringView(&internal::BuildDate[4], &internal::BuildDate[6]);
}
static_assert(raw_day().length() < 3, "");

// "Jan  1 1970"
//         ^^^^
constexpr StringView raw_year() {
    return StringView(&internal::BuildDate[7], &internal::BuildDate[11]);
}
static_assert(raw_year().length() == 4, "");

// "03:00:00"
//  ^^
constexpr StringView raw_hour() {
    return StringView(&internal::BuildTime[0], &internal::BuildTime[2]);
}
static_assert(raw_hour().length() == 2, "");

// "03:00:00"
//     ^^
constexpr StringView raw_minute() {
    return StringView(&internal::BuildTime[3], &internal::BuildTime[5]);
}
static_assert(raw_minute().length() == 2, "");

// "03:00:00"
//        ^^
constexpr StringView raw_second() {
    return StringView(&internal::BuildTime[6], &internal::BuildTime[8]);
}
static_assert(raw_second().length() == 2, "");

#define STRING_EQUALS(EXPECTED, ACTUAL)\
    (__builtin_memcmp((ACTUAL).c_str(), (EXPECTED), (ACTUAL).length()) == 0)

constexpr int from_raw_month(StringView month) {
    return STRING_EQUALS("Jan", month) ? 1 :
        STRING_EQUALS("Feb", month) ? 2 :
        STRING_EQUALS("Mar", month) ? 3 :
        STRING_EQUALS("Apr", month) ? 4 :
        STRING_EQUALS("May", month) ? 5 :
        STRING_EQUALS("Jun", month) ? 6 :
        STRING_EQUALS("Jul", month) ? 7 :
        STRING_EQUALS("Aug", month) ? 8 :
        STRING_EQUALS("Sep", month) ? 9 :
        STRING_EQUALS("Oct", month) ? 10 :
        STRING_EQUALS("Nov", month) ? 11 :
        STRING_EQUALS("Dec", month) ? 12 : 0;
}
#undef STRING_EQUALS

constexpr int month() {
    return from_raw_month(raw_month());
}

constexpr int from_one_digit(char value) {
    return ((value >= '0') && (value <= '9'))
        ? (value - '0')
        : 0;
}

constexpr int from_two_digits(StringView value) {
    return (from_one_digit(value.c_str()[0]) * 10)
         + from_one_digit(value.c_str()[1]);
}

constexpr int from_raw_day(StringView day) {
    return (day.length() == 2)
        ? from_two_digits(day)
        : from_one_digit(*day.c_str());
}

constexpr int day() {
    return from_raw_day(raw_day());
}

constexpr int hour() {
    return from_two_digits(raw_hour());
}

constexpr int minute() {
    return from_two_digits(raw_minute());
}

constexpr int second() {
    return from_two_digits(raw_second());
}

constexpr int from_raw_year(StringView year) {
    return (from_one_digit(year.c_str()[0]) * 1000)
         + (from_one_digit(year.c_str()[1]) * 100)
         + (from_one_digit(year.c_str()[2]) * 10)
         + from_one_digit(year.c_str()[3]);
}

constexpr int year() {
    return from_raw_year(raw_year());
}

} // namespace time

constexpr StringView modules() {
    return internal::Modules;
}

constexpr StringView name() {
    return internal::Name;
}

constexpr StringView version() {
    return internal::Version;
}

constexpr StringView author() {
    return internal::Author;
}

constexpr StringView website() {
    return internal::Website;
}

StringView build_time() {
    // 1234-56-78 01:02:03
    static char out[20] = {0};

    if (out[0] == '\0') {
        // workaround for gcc4.8, explicitly mark as constexpr
        // otherwise, we will read progmem'ed string at runtime
        // (double-check the asm when changing anything here)
        constexpr int year = time::year();
        constexpr int month = time::month();
        constexpr int day = time::day();
        constexpr int hour = time::hour();
        constexpr int minute = time::minute();
        constexpr int second = time::second();
        snprintf_P(out, sizeof(out),
            PSTR("%4d-%02d-%02d %02d:%02d:%02d"),
            year, month, day,
            hour, minute, second);
    }

    return StringView(out, sizeof(out) - 1);
}

App get() {
    return App{
        .name = name(),
        .version = version(),
        .build_time = build_time(),
        .author = author(),
        .website = website(),
    };
};

} // namespace app

Info info() {
    return Info{
        .sdk = sdk::get(),
        .hardware = hardware::get(),
        .app = app::get(),
    };
}

} // namespace
} // namespace build
} // namespace espurna

espurna::StringView buildTime() {
    return espurna::build::app::build_time();
}

espurna::build::Sdk buildSdk() {
    return espurna::build::sdk::get();
}

espurna::build::Hardware buildHardware() {
    return espurna::build::hardware::get();
}

espurna::build::App buildApp() {
    return espurna::build::app::get();
}

espurna::build::Info buildInfo() {
    return espurna::build::info();
}

espurna::StringView buildModules() {
    return espurna::build::app::modules();
}
