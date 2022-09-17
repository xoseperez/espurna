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

alignas(4) static constexpr char Manufacturer[] PROGMEM = MANUFACTURER;
alignas(4) static constexpr char Device[] PROGMEM = DEVICE;

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

alignas(4) static constexpr char Modules[] PROGMEM =
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
#if TELNET_SERVER == TELNET_SERVER_WIFISERVER
    "TELNET_SYNC "
#else
    "TELNET "
#endif // TELNET_SERVER == TELNET_SERVER_WIFISERVER
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

alignas(4) static constexpr char Name[] PROGMEM = APP_NAME;
alignas(4) static constexpr char Version[] PROGMEM = APP_VERSION;
alignas(4) static constexpr char Author[] PROGMEM = APP_AUTHOR;
alignas(4) static constexpr char Website[] PROGMEM = APP_WEBSITE;

} // namespace internal

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

constexpr time_t time() {
    return __UNIX_TIMESTAMP__;
}

StringView time_string() {
    static const String out = ([]() -> String {
        char buf[32];

#if NTP_SUPPORT
        const time_t ts = time();
        tm now;

        gmtime_r(&ts, &now);
        now.tm_year += 1900;
        now.tm_mon += 1;
#else
        constexpr tm now {
            .tm_year = __TIME_YEAR__,
            .tm_mon = __TIME_MONTH__,
            .tm_mday = __TIME_DAY__,
            .tm_hour = __TIME_HOUR__,
            .tm_min = __TIME_MINUTE__,
            .tm_sec = __TIME_SECOND__,
        };
#endif
        snprintf_P(buf, sizeof(buf),
            PSTR("%04d-%02d-%02d %02d:%02d:%02d"),
            now.tm_year, now.tm_mon, now.tm_mday,
            now.tm_hour, now.tm_min, now.tm_sec);

        return buf;
    })();
    
    return out;
}

App get() {
    return App{
        .name = name(),
        .version = version(),
        .build_time = time_string(),
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

time_t buildTime() {
    return espurna::build::app::time();
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
