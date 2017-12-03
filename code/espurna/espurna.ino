/*

ESPurna

Copyright (C) 2016-2017 by Xose Pérez <xose dot perez at gmail dot com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "config/all.h"
#include <EEPROM.h>

// -----------------------------------------------------------------------------
// METHODS
// -----------------------------------------------------------------------------

unsigned long _loopDelay = 0;

void hardwareSetup() {

    EEPROM.begin(EEPROM_SIZE);

    #if DEBUG_SERIAL_SUPPORT
        DEBUG_PORT.begin(SERIAL_BAUDRATE);
        #if DEBUG_ESP_WIFI
            DEBUG_PORT.setDebugOutput(true);
        #endif
    #elif defined(SERIAL_BAUDRATE)
        Serial.begin(SERIAL_BAUDRATE);
    #endif

    #if SPIFFS_SUPPORT
        SPIFFS.begin();
    #endif

    #if defined(ESPLIVE)
        //The ESPLive has an ADC MUX which needs to be configured.
        pinMode(16, OUTPUT);
        digitalWrite(16, HIGH); //Defualt CT input (pin B, solder jumper B)
    #endif

}

void hardwareLoop() {

    // Heartbeat
    static unsigned long last_uptime = 0;
    if ((millis() - last_uptime > HEARTBEAT_INTERVAL) || (last_uptime == 0)) {
        last_uptime = millis();
        heartbeat();
    }

}

// -----------------------------------------------------------------------------
// BOOTING
// -----------------------------------------------------------------------------

unsigned int sectors(size_t size) {
    return (int) (size + SPI_FLASH_SEC_SIZE - 1) / SPI_FLASH_SEC_SIZE;
}

void welcome() {

    DEBUG_MSG_P(PSTR("\n\n"));
    DEBUG_MSG_P(PSTR("[INIT] %s %s\n"), (char *) APP_NAME, (char *) APP_VERSION);
    DEBUG_MSG_P(PSTR("[INIT] %s\n"), (char *) APP_AUTHOR);
    DEBUG_MSG_P(PSTR("[INIT] %s\n\n"), (char *) APP_WEBSITE);
    DEBUG_MSG_P(PSTR("[INIT] CPU chip ID: 0x%06X\n"), ESP.getChipId());
    DEBUG_MSG_P(PSTR("[INIT] CPU frequency: %d MHz\n"), ESP.getCpuFreqMHz());
    DEBUG_MSG_P(PSTR("[INIT] SDK version: %s\n"), ESP.getSdkVersion());
    DEBUG_MSG_P(PSTR("[INIT] Core version: %s\n"), getCoreVersion().c_str());
    DEBUG_MSG_P(PSTR("[INIT] Core revision: %s\n"), getCoreRevision().c_str());
    DEBUG_MSG_P(PSTR("\n"));

    // -------------------------------------------------------------------------

    FlashMode_t mode = ESP.getFlashChipMode();
    DEBUG_MSG_P(PSTR("[INIT] Flash chip ID: 0x%06X\n"), ESP.getFlashChipId());
    DEBUG_MSG_P(PSTR("[INIT] Flash speed: %u Hz\n"), ESP.getFlashChipSpeed());
    DEBUG_MSG_P(PSTR("[INIT] Flash mode: %s\n"), mode == FM_QIO ? "QIO" : mode == FM_QOUT ? "QOUT" : mode == FM_DIO ? "DIO" : mode == FM_DOUT ? "DOUT" : "UNKNOWN");
    DEBUG_MSG_P(PSTR("\n"));
    DEBUG_MSG_P(PSTR("[INIT] Flash sector size: %8u bytes\n"), SPI_FLASH_SEC_SIZE);
    DEBUG_MSG_P(PSTR("[INIT] Flash size (CHIP): %8u bytes\n"), ESP.getFlashChipRealSize());
    DEBUG_MSG_P(PSTR("[INIT] Flash size (SDK):  %8u bytes / %4d sectors\n"), ESP.getFlashChipSize(), sectors(ESP.getFlashChipSize()));
    DEBUG_MSG_P(PSTR("[INIT] Firmware size:     %8u bytes / %4d sectors\n"), ESP.getSketchSize(), sectors(ESP.getSketchSize()));
    DEBUG_MSG_P(PSTR("[INIT] OTA size:          %8u bytes / %4d sectors\n"), ESP.getFreeSketchSpace(), sectors(ESP.getFreeSketchSpace()));
    #if SPIFFS_SUPPORT
        FSInfo fs_info;
        bool fs = SPIFFS.info(fs_info);
        if (fs) {
            DEBUG_MSG_P(PSTR("[INIT] SPIFFS size:       %8u bytes / %4d sectors\n"), fs_info.totalBytes, sectors(fs_info.totalBytes));
        }
    #else
        DEBUG_MSG_P(PSTR("[INIT] SPIFFS size:       %8u bytes / %4d sectors\n"), 0, 0);
    #endif
    DEBUG_MSG_P(PSTR("[INIT] EEPROM size:       %8u bytes / %4d sectors\n"), settingsMaxSize(), sectors(settingsMaxSize()));
    DEBUG_MSG_P(PSTR("[INIT] Empty space:       %8u bytes /    4 sectors\n"), 4 * SPI_FLASH_SEC_SIZE);
    DEBUG_MSG_P(PSTR("\n"));

    // -------------------------------------------------------------------------

    #if SPIFFS_SUPPORT
        if (fs) {
            DEBUG_MSG_P(PSTR("[INIT] SPIFFS total size: %8u bytes\n"), fs_info.totalBytes);
            DEBUG_MSG_P(PSTR("[INIT]        used size:  %8u bytes\n"), fs_info.usedBytes);
            DEBUG_MSG_P(PSTR("[INIT]        block size: %8u bytes\n"), fs_info.blockSize);
            DEBUG_MSG_P(PSTR("[INIT]        page size:  %8u bytes\n"), fs_info.pageSize);
            DEBUG_MSG_P(PSTR("[INIT]        max files:  %8u\n"), fs_info.maxOpenFiles);
            DEBUG_MSG_P(PSTR("[INIT]        max length: %8u\n"), fs_info.maxPathLength);
        } else {
            DEBUG_MSG_P(PSTR("[INIT] No SPIFFS partition\n"));
        }
        DEBUG_MSG_P(PSTR("\n"));
    #endif

    // -------------------------------------------------------------------------

    DEBUG_MSG_P(PSTR("[INIT] MANUFACTURER: %s\n"), MANUFACTURER);
    DEBUG_MSG_P(PSTR("[INIT] DEVICE: %s\n"), DEVICE);
    DEBUG_MSG_P(PSTR("[INIT] SUPPORT:"));

    #if ALEXA_SUPPORT
        DEBUG_MSG_P(PSTR(" ALEXA"));
    #endif
    #if ANALOG_SUPPORT
        DEBUG_MSG_P(PSTR(" ANALOG"));
    #endif
    #if COUNTER_SUPPORT
        DEBUG_MSG_P(PSTR(" COUNTER"));
    #endif
    #if DEBUG_SERIAL_SUPPORT
        DEBUG_MSG_P(PSTR(" DEBUG_SERIAL"));
    #endif
    #if DEBUG_TELNET_SUPPORT
        DEBUG_MSG_P(PSTR(" DEBUG_TELNET"));
    #endif
    #if DEBUG_UDP_SUPPORT
        DEBUG_MSG_P(PSTR(" DEBUG_UDP"));
    #endif
    #if DHT_SUPPORT
        DEBUG_MSG_P(PSTR(" DHT"));
    #endif
    #if DOMOTICZ_SUPPORT
        DEBUG_MSG_P(PSTR(" DOMOTICZ"));
    #endif
    #if DS18B20_SUPPORT
        DEBUG_MSG_P(PSTR(" DS18B20"));
    #endif
    #if HOMEASSISTANT_SUPPORT
        DEBUG_MSG_P(PSTR(" HOMEASSISTANT"));
    #endif
    #if I2C_SUPPORT
        DEBUG_MSG_P(PSTR(" I2C"));
    #endif
    #if INFLUXDB_SUPPORT
        DEBUG_MSG_P(PSTR(" INFLUXDB"));
    #endif
    #if LLMNR_SUPPORT
        DEBUG_MSG_P(PSTR(" LLMNR"));
    #endif
    #if MDNS_SUPPORT
        DEBUG_MSG_P(PSTR(" MDNS"));
    #endif
    #if NETBIOS_SUPPORT
        DEBUG_MSG_P(PSTR(" NETBIOS"));
    #endif
    #if NOFUSS_SUPPORT
        DEBUG_MSG_P(PSTR(" NOFUSS"));
    #endif
    #if NTP_SUPPORT
        DEBUG_MSG_P(PSTR(" NTP"));
    #endif
    #if RF_SUPPORT
        DEBUG_MSG_P(PSTR(" RF"));
    #endif
    #if SPIFFS_SUPPORT
        DEBUG_MSG_P(PSTR(" SPIFFS"));
    #endif
    #if TELNET_SUPPORT
        DEBUG_MSG_P(PSTR(" TELNET"));
    #endif
    #if TERMINAL_SUPPORT
        DEBUG_MSG_P(PSTR(" TERMINAL"));
    #endif
    #if WEB_SUPPORT
        DEBUG_MSG_P(PSTR(" WEB"));
    #endif

    DEBUG_MSG_P(PSTR("\n\n"));

    // -------------------------------------------------------------------------

    unsigned char reason = resetReason();
    if (reason > 0) {
        char buffer[32];
        strcpy_P(buffer, custom_reset_string[reason-1]);
        DEBUG_MSG_P(PSTR("[INIT] Last reset reason: %s\n"), buffer);
    } else {
        DEBUG_MSG_P(PSTR("[INIT] Last reset reason: %s\n"), (char *) ESP.getResetReason().c_str());
    }

    DEBUG_MSG_P(PSTR("[INIT] Free heap: %u bytes\n"), getFreeHeap());
    #if ADC_VCC_ENABLED
        DEBUG_MSG_P(PSTR("[INIT] Power: %d mV\n"), ESP.getVcc());
    #endif

    DEBUG_MSG_P(PSTR("[INIT] Power saving delay value: %lu ms\n"), _loopDelay);
    DEBUG_MSG_P(PSTR("\n"));

}

void setup() {

    // Init EEPROM, Serial and SPIFFS
    hardwareSetup();

    // Question system stability
    #if SYSTEM_CHECK_ENABLED
        systemCheck(false);
    #endif

    // Init persistance and terminal features
    settingsSetup();
    if (getSetting("hostname").length() == 0) {
        setSetting("hostname", getIdentifier());
    }

    // Cache loop delay value to speed things (recommended max 250ms)
    _loopDelay = atol(getSetting("loopDelay", LOOP_DELAY_TIME).c_str());

    // Show welcome message and system configuration
    welcome();

    // Basic modules, will always run
    wifiSetup();
    otaSetup();
    #if TELNET_SUPPORT
        telnetSetup();
    #endif

    // Do not run the next services if system is flagged stable
    #if SYSTEM_CHECK_ENABLED
        if (!systemCheck()) return;
    #endif

    // Init webserver required before any module that uses API
    #if WEB_SUPPORT
        webSetup();
        wsSetup();
        apiSetup();
    #endif

    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        lightSetup();
    #endif
    relaySetup();
    buttonSetup();
    ledSetup();
    #if MQTT_SUPPORT
        mqttSetup();
    #endif

    #if MDNS_SUPPORT
        mdnsSetup();
    #endif
    #if LLMNR_SUPPORT
        llmnrSetup();
    #endif
    #if NETBIOS_SUPPORT
        netbiosSetup();
    #endif
    #if NTP_SUPPORT
        ntpSetup();
    #endif
    #if I2C_SUPPORT
        i2cSetup();
    #endif
    #ifdef ITEAD_SONOFF_RFBRIDGE
        rfbSetup();
    #endif
    #if POWER_PROVIDER != POWER_PROVIDER_NONE
        powerSetup();
    #endif
    #if ALEXA_SUPPORT
        alexaSetup();
    #endif
    #if NOFUSS_SUPPORT
        nofussSetup();
    #endif
    #if INFLUXDB_SUPPORT
        idbSetup();
    #endif
    #if DS18B20_SUPPORT
        dsSetup();
    #endif
    #if ANALOG_SUPPORT
        analogSetup();
    #endif
    #if COUNTER_SUPPORT
        counterSetup();
    #endif
    #if DHT_SUPPORT
        dhtSetup();
    #endif
    #if RF_SUPPORT
        rfSetup();
    #endif
    #if IR_SUPPORT
        irSetup();
    #endif
    #if DOMOTICZ_SUPPORT
        domoticzSetup();
    #endif
    #if HOMEASSISTANT_SUPPORT
        haSetup();
    #endif

    // Prepare configuration for version 2.0
    hwUpwardsCompatibility();

    saveSettings();

}

void loop() {

    hardwareLoop();
    settingsLoop();
    wifiLoop();
    otaLoop();

    #if SYSTEM_CHECK_ENABLED
        systemCheckLoop();
        // Do not run the next services if system is flagged stable
        if (!systemCheck()) return;
    #endif

    #if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
        lightLoop();
    #endif
    relayLoop();
    buttonLoop();
    ledLoop();
    #if MQTT_SUPPORT
        mqttLoop();
    #endif

    #ifdef ITEAD_SONOFF_RFBRIDGE
        rfbLoop();
    #endif
    #if POWER_PROVIDER != POWER_PROVIDER_NONE
        powerLoop();
    #endif
    #if NTP_SUPPORT
        ntpLoop();
    #endif
    #if ALEXA_SUPPORT
        alexaLoop();
    #endif
    #if NOFUSS_SUPPORT
        nofussLoop();
    #endif
    #if DS18B20_SUPPORT
        dsLoop();
    #endif
    #if ANALOG_SUPPORT
        analogLoop();
    #endif
    #if COUNTER_SUPPORT
        counterLoop();
    #endif
    #if DHT_SUPPORT
        dhtLoop();
    #endif
    #if RF_SUPPORT
        rfLoop();
    #endif
    #if IR_SUPPORT
        irLoop();
    #endif

    // Power saving delay
    delay(_loopDelay);

}
