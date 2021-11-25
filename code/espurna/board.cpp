/*

BOARD MODULE

*/

#include "espurna.h"
#include "relay.h"
#include "sensor.h"
#include "utils.h"

//--------------------------------------------------------------------------------

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
    "WEB "
#endif
    "";

    return modules;
}

const char* getEspurnaWebUI() {
    static const char webui[] PROGMEM =
#if WEBUI_IMAGE == WEBUI_IMAGE_SMALL
    "SMALL"
#endif
#if WEBUI_IMAGE == WEBUI_IMAGE_LIGHT
    "LIGHT"
#endif
#if WEBUI_IMAGE == WEBUI_IMAGE_SENSOR
    "SENSOR"
#endif
#if WEBUI_IMAGE == WEBUI_IMAGE_RFBRIDGE
    "RFBRIDGE"
#endif
#if WEBUI_IMAGE == WEBUI_IMAGE_RFM69
    "RFM69"
#endif
#if WEBUI_IMAGE == WEBUI_IMAGE_LIGHTFOX
    "LIGHTFOX"
#endif
#if WEBUI_IMAGE == WEBUI_IMAGE_GARLAND
    "GARLAND"
#endif
#if WEBUI_IMAGE == WEBUI_IMAGE_THERMOSTAT
    "THERMOSTAT"
#endif
#if WEBUI_IMAGE == WEBUI_IMAGE_CURTAIN
    "CURTAIN"
#endif
#if WEBUI_IMAGE == WEBUI_IMAGE_FULL
    "FULL"
#endif
    "";

    return webui;
}

#if SENSOR_SUPPORT

const char* getEspurnaSensors() {
    static const char sensors[] PROGMEM =
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
#if HLW8012_SUPPORT
    "HLW8012 "
#endif
#if LDR_SUPPORT
    "LDR "
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
#if PMSX003_SUPPORT
    "PMSX003 "
#endif
#if PULSEMETER_SUPPORT
    "PULSEMETER "
#endif
#if PZEM004T_SUPPORT
    "PZEM004T "
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
#if ADE7953_SUPPORT
    "ADE7953 "
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
