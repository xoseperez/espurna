/*

BOARD MODULE

*/

#include "espurna.h"
#include "relay.h"
#include "rtcmem.h"
#include "sensor.h"
#include "utils.h"

#include <cstring>

//--------------------------------------------------------------------------------

#include <cstring>

const String& getChipId() {
    static String value;
    if (!value.length()) {
        char buffer[7];
        value.reserve(sizeof(buffer));
        snprintf_P(buffer, sizeof(buffer), PSTR("%06X"), ESP.getChipId());
        value = buffer;
    }
    return value;
}

const String& getIdentifier() {
    static String value;
    if (!value.length()) {
        value += getAppName();
        value += '-';
        value += getChipId();
    }
    return value;
}

// Full Chip ID (aka MAC)
// based on the [esptool.py](https://github.com/espressif/esptool) implementation
// - register addresses: https://github.com/espressif/esptool/blob/737825ba8d7aa696e4a9213cad932bceafb79f51/esptool.py#L1140-L1143
// - chip id & mac: https://github.com/espressif/esptool/blob/737825ba8d7aa696e4a9213cad932bceafb79f51/esptool.py#L1235-L1254

const String& getFullChipId() {
    static String out;

    if (!out.length()) {
        uint32_t regs[3] {
            READ_PERI_REG(0x3ff00050),
            READ_PERI_REG(0x3ff00054),
            READ_PERI_REG(0x3ff0005c)};

        uint8_t mac[6] {
            0xff,
            0xff,
            0xff,
            static_cast<uint8_t>((regs[1] >> 8ul) & 0xfful),
            static_cast<uint8_t>(regs[1] & 0xffu),
            static_cast<uint8_t>((regs[0] >> 24ul) & 0xffu)};

        if (mac[2] != 0) {
            mac[0] = (regs[2] >> 16ul) & 0xffu;
            mac[1] = (regs[2] >> 8ul) & 0xffu;
            mac[2] = (regs[2] & 0xffu);
        } else if (0 == ((regs[1] >> 16ul) & 0xff)) {
            mac[0] = 0x18;
            mac[1] = 0xfe;
            mac[2] = 0x34;
        } else if (1 == ((regs[1] >> 16ul) & 0xff)) {
            mac[0] = 0xac;
            mac[1] = 0xd0;
            mac[2] = 0x74;
        }

        char buffer[(sizeof(mac) * 2) + 1];
        if (hexEncode(mac, sizeof(mac), buffer, sizeof(buffer))) {
            out = buffer;
        }
    }

    return out;
}

const char* getEspurnaModules() {
    static const char modules[] PROGMEM =
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

    return modules;
}

#if SENSOR_SUPPORT

const char* getEspurnaSensors() {
    static const char sensors[] PROGMEM =
#if ADE7953_SUPPORT
    "ADE7953 "
#endif
#if AM2320_SUPPORT
    "AM2320_I2C "
#endif
#if ANALOG_SUPPORT
    "ANALOG "
#endif
#if BH1750_SUPPORT
    "BH1750 "
#endif
#if BMP180_SUPPORT
    "BMP180 "
#endif
#if BMX280_SUPPORT
    "BMX280 "
#endif
#if BME680_SUPPORT
    "BME680 "
#endif
#if CSE7766_SUPPORT
    "CSE7766 "
#endif
#if DALLAS_SUPPORT
    "DALLAS "
#endif
#if DHT_SUPPORT
    "DHTXX "
#endif
#if DIGITAL_SUPPORT
    "DIGITAL "
#endif
#if ECH1560_SUPPORT
    "ECH1560 "
#endif
#if EMON_ADC121_SUPPORT
    "EMON_ADC121 "
#endif
#if EMON_ADS1X15_SUPPORT
    "EMON_ADX1X15 "
#endif
#if EMON_ANALOG_SUPPORT
    "EMON_ANALOG "
#endif
#if EVENTS_SUPPORT
    "EVENTS "
#endif
#if GEIGER_SUPPORT
    "GEIGER "
#endif
#if GUVAS12SD_SUPPORT
    "GUVAS12SD "
#endif
#if HDC1080_SUPPORT
    "HDC1080 "
#endif
#if HLW8012_SUPPORT
    "HLW8012 "
#endif
#if INA219_SUPPORT
    "INA219 "
#endif
#if LDR_SUPPORT
    "LDR "
#endif
#if MAX6675_SUPPORT
    "MAX6675 "
#endif
#if MHZ19_SUPPORT
    "MHZ19 "
#endif
#if MICS2710_SUPPORT
    "MICS2710 "
#endif
#if MICS5525_SUPPORT
    "MICS5525 "
#endif
#if NTC_SUPPORT
    "NTC "
#endif
#if PM1006_SUPPORT
    "PM1006 "
#endif
#if PMSX003_SUPPORT
    "PMSX003 "
#endif
#if PULSEMETER_SUPPORT
    "PULSEMETER "
#endif
#if PZEM004T_SUPPORT
    "PZEM004T "
#endif
#if PZEM004TV30_SUPPORT
    "PZEM004TV30 "
#endif
#if SDS011_SUPPORT
    "SDS011 "
#endif
#if SENSEAIR_SUPPORT
    "SENSEAIR "
#endif
#if SHT3X_I2C_SUPPORT
    "SHT3X_I2C "
#endif
#if SI7021_SUPPORT
    "SI7021 "
#endif
#if SM300D2_SUPPORT
    "SM300D2 "
#endif
#if SONAR_SUPPORT
    "SONAR "
#endif
#if T6613_SUPPORT
    "T6613 "
#endif
#if TMP3X_SUPPORT
    "TMP3X "
#endif
#if V9261F_SUPPORT
    "V9261F "
#endif
#if VEML6075_SUPPORT
    "VEML6075 "
#endif
#if VL53L1X_SUPPORT
    "VL53L1X "
#endif
#if EZOPH_SUPPORT
    "EZOPH "
#endif
#if DUMMY_SENSOR_SUPPORT
    "DUMMY "
#endif
#if SI1145_SUPPORT
    "SI1145 "
#endif
    "";

    return sensors;
}

#endif

bool haveRelaysOrSensors() {
    bool result = false;
    result = (relayCount() > 0);
#if SENSOR_SUPPORT
    result = result || (magnitudeCount() > 0);
#endif
    return result;
}

void boardSetup() {
    // Some magic to allow seamless Tasmota OTA upgrades
    // - inject dummy data sequence that is expected to hold current version info
    // - purge settings, since we don't want accidentaly reading something as a kv
    // - sometimes we cannot boot b/c of certain SDK params, purge last 16KiB
    {
        // ref. `SetOption78 1` in Tasmota
        // - https://tasmota.github.io/docs/Commands/#setoptions (> SetOption78   Version check on Tasmota upgrade)
        // - https://github.com/esphome/esphome/blob/0e59243b83913fc724d0229514a84b6ea14717cc/esphome/core/esphal.cpp#L275-L287 (the original idea from esphome)
        // - https://github.com/arendst/Tasmota/blob/217addc2bb2cf46e7633c93e87954b245cb96556/tasmota/settings.ino#L218-L262 (specific checks, which succeed when finding 0xffffffff as version)
        // - https://github.com/arendst/Tasmota/blob/0dfa38df89c8f2a1e582d53d79243881645be0b8/tasmota/i18n.h#L780-L782 (constants)
        volatile uint32_t magic[] __attribute__((unused)) {
            0x5aa55aa5,
            0xffffffff,
            0xa55aa55a,
        };

        // ref. https://github.com/arendst/Tasmota/blob/217addc2bb2cf46e7633c93e87954b245cb96556/tasmota/settings.ino#L24
        // We will certainly find these when rebooting from Tasmota. Purge SDK as well, since we may experience WDT after starting up the softAP
        auto* rtcmem = reinterpret_cast<volatile uint32_t*>(RTCMEM_ADDR);
        if ((0xA55A == rtcmem[64]) && (0xA55A == rtcmem[68])) {
            DEBUG_MSG_P(PSTR("[BOARD] TASMOTA OTA, resetting...\n"));
            rtcmem[64] = rtcmem[68] = 0;
            customResetReason(CustomResetReason::Factory);
            resetSettings();
            eraseSDKConfig();
            __builtin_trap();
            // can't return!
        }

        // TODO: also check for things throughout the flash sector, somehow?
    }

    // Workaround for SDK changes between 1.5.3 and 2.2.x or possible
    // flash corruption happening to the 'default' WiFi config
#if SYSTEM_CHECK_ENABLED
    if (!systemCheck()) {
        const uint32_t Address { ESP.getFlashChipSize() - (FLASH_SECTOR_SIZE * 3) };

        static constexpr size_t PageSize { 256 };
#ifdef FLASH_PAGE_SIZE
        static_assert(FLASH_PAGE_SIZE == PageSize, "");
#endif
        static constexpr auto Alignment = alignof(uint32_t);
        alignas(Alignment) std::array<uint8_t, PageSize> page;

        if (ESP.flashRead(Address, reinterpret_cast<uint32_t*>(page.data()), page.size())) {
            constexpr uint32_t ConfigOffset { 0xb0 };

            // In case flash was already erased at some point, but we are still here
            alignas(Alignment) const std::array<uint8_t, 8> Empty { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
            if (std::memcpy(&page[ConfigOffset], &Empty[0], Empty.size()) != 0) {
                return;
            }

            // 0x00B0:  0A 00 00 00 45 53 50 2D XX XX XX XX XX XX 00 00     ESP-XXXXXX
            alignas(Alignment) const std::array<uint8_t, 8> Reference { 0xa0, 0x00, 0x00, 0x00, 0x45, 0x53, 0x50, 0x2d };
            if (std::memcmp(&page[ConfigOffset], &Reference[0], Reference.size()) != 0) {
                DEBUG_MSG_P(PSTR("[BOARD] Invalid SDK config at 0x%08X, resetting...\n"), Address + ConfigOffset);
                customResetReason(CustomResetReason::Factory);
                systemForceStable();
                forceEraseSDKConfig();
                // can't return!
            }
        }
    }
#endif

#if DEBUG_SERIAL_SUPPORT
    if (debugLogBuffer()) {
        return;
    }

    DEBUG_MSG_P(PSTR("[MAIN] %s %s built %s\n"),
            getAppName(), getVersion(), buildTime().c_str());
    DEBUG_MSG_P(PSTR("[MAIN] %s\n"), getAppAuthor());
    DEBUG_MSG_P(PSTR("[MAIN] %s\n"), getAppWebsite());
    DEBUG_MSG_P(PSTR("[MAIN] CPU chip ID: %s frequency: %hhuMHz\n"),
            getFullChipId().c_str(), system_get_cpu_freq());
    DEBUG_MSG_P(PSTR("[MAIN] SDK: %s Arduino Core: %s\n"),
            ESP.getSdkVersion(), getCoreVersion().c_str());
    DEBUG_MSG_P(PSTR("[MAIN] Support: %s\n"), getEspurnaModules());
#if SENSOR_SUPPORT
    DEBUG_MSG_P(PSTR("[MAIN] Sensors: %s\n"), getEspurnaSensors());
#endif
#endif
}
