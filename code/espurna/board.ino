/*

BOARD MODULE

*/

#include "board.h"

//--------------------------------------------------------------------------------

PROGMEM const char espurna_modules[] =
    #if ALEXA_SUPPORT
        "ALEXA "
    #endif
    #if API_SUPPORT
        "API "
    #endif
    #if BROKER_SUPPORT
        "BROKER "
    #endif
    #if BUTTON_SUPPORT
    #if BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_GENERIC
        "BUTTON "
    #else
        "BUTTON_DUAL "
    #endif
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
    #if MDNS_CLIENT_SUPPORT
        "MDNS_CLIENT "
    #endif
    #if MDNS_SERVER_SUPPORT
        "MDNS_SERVER "
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
    #if RFM69_SUPPORT
        "RFM69 "
    #endif
    #if RF_SUPPORT
        "RF "
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

PROGMEM const char espurna_ota_modules[] =
    #if OTA_ARDUINOOTA_SUPPORT
        "ARDUINO "
    #endif
    #if (OTA_CLIENT == OTA_CLIENT_ASYNCTCP)
        "ASYNCTCP "
    #endif
    #if (OTA_CLIENT == OTA_CLIENT_HTTPUPDATE)
    #if (SECURE_CLIENT == SECURE_CLIENT_NONE)
        "*HTTPUPDATE "
    #endif
    #if (SECURE_CLIENT == SECURE_CLIENT_AXTLS)
        "*HTTPUPDATE_AXTLS "
    #endif
    #if (SECURE_CLIENT == SECURE_CLIENT_BEARSSL)
        "*HTTPUPDATE_BEARSSL "
    #endif
    #endif // OTA_CLIENT_HTTPUPDATE
    #if OTA_MQTT_SUPPORT
        "MQTT "
    #endif
    #if WEB_SUPPORT
        "WEB "
    #endif
    "";

PROGMEM const char espurna_webui[] =
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
    #if WEBUI_IMAGE == WEBUI_IMAGE_THERMOSTAT
        "THERMOSTAT"
    #endif
    #if WEBUI_IMAGE == WEBUI_IMAGE_FULL
        "FULL"
    #endif
    "";

#if SENSOR_SUPPORT

PROGMEM const char espurna_sensors[] =
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

#endif // SENSOR_SUPPORT == 1

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
        value += APP_NAME;
        value += '-';
        value += getChipId();
    }
    return value;
}

String getEspurnaModules() {
    return FPSTR(espurna_modules);
}

String getEspurnaOTAModules() {
    return FPSTR(espurna_ota_modules);
}

#if SENSOR_SUPPORT
String getEspurnaSensors() {
    return FPSTR(espurna_sensors);
}
#endif // SENSOR_SUPPORT == 1

String getEspurnaWebUI() {
    return FPSTR(espurna_webui);
}

bool isEspurnaCore() {
    #if defined(ESPURNA_CORE) || defined(ESPURNA_CORE_WEBUI)
        return true;
    #else
        return false;
    #endif
}

bool haveRelaysOrSensors() {
    bool result = false;
    result = (relayCount() > 0);
    #if SENSOR_SUPPORT
        result = result || (magnitudeCount() > 0);
    #endif
    return result;
}

