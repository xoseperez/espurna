/*

UTILS MODULE

Copyright (C) 2017-2018 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include <Ticker.h>
Ticker _defer_reset;

String getIdentifier() {
    char buffer[20];
    snprintf_P(buffer, sizeof(buffer), PSTR("%s_%06X"), APP_NAME, ESP.getChipId());
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

// WTF
// Calling ESP.getFreeHeap() is making the system crash on a specific
// AiLight bulb, but anywhere else...
unsigned int getFreeHeap() {
    if (getSetting("wtfHeap", 0).toInt() == 1) return 9999;
    return ESP.getFreeHeap();
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

#if HEARTBEAT_ENABLED

void heartbeat() {

    unsigned long uptime_seconds = getUptime();
    unsigned int free_heap = getFreeHeap();

    #if MQTT_SUPPORT
        bool serial = !mqttConnected();
    #else
        bool serial = true;
    #endif

    // -------------------------------------------------------------------------
    // Serial
    // -------------------------------------------------------------------------

    if (serial) {
        DEBUG_MSG_P(PSTR("[MAIN] Uptime: %lu seconds\n"), uptime_seconds);
        DEBUG_MSG_P(PSTR("[MAIN] Free heap: %lu bytes\n"), free_heap);
        #if ADC_VCC_ENABLED
            DEBUG_MSG_P(PSTR("[MAIN] Power: %lu mV\n"), ESP.getVcc());
        #endif
        #if NTP_SUPPORT
            if (ntpSynced()) DEBUG_MSG_P(PSTR("[MAIN] Time: %s\n"), (char *) ntpDateTime().c_str());
        #endif
    }

    // -------------------------------------------------------------------------
    // MQTT
    // -------------------------------------------------------------------------

    #if MQTT_SUPPORT
        if (!serial) {
            #if (HEARTBEAT_REPORT_INTERVAL)
                mqttSend(MQTT_TOPIC_INTERVAL, HEARTBEAT_INTERVAL / 1000);
            #endif
            #if (HEARTBEAT_REPORT_APP)
                mqttSend(MQTT_TOPIC_APP, APP_NAME);
            #endif
            #if (HEARTBEAT_REPORT_VERSION)
                mqttSend(MQTT_TOPIC_VERSION, APP_VERSION);
            #endif
            #if (HEARTBEAT_REPORT_BOARD)
                mqttSend(MQTT_TOPIC_BOARD, getBoardName().c_str());
            #endif
            #if (HEARTBEAT_REPORT_HOSTNAME)
                mqttSend(MQTT_TOPIC_HOSTNAME, getSetting("hostname").c_str());
            #endif
            #if (HEARTBEAT_REPORT_IP)
                mqttSend(MQTT_TOPIC_IP, getIP().c_str());
            #endif
            #if (HEARTBEAT_REPORT_MAC)
                mqttSend(MQTT_TOPIC_MAC, WiFi.macAddress().c_str());
            #endif
            #if (HEARTBEAT_REPORT_RSSI)
                mqttSend(MQTT_TOPIC_RSSI, String(WiFi.RSSI()).c_str());
            #endif
            #if (HEARTBEAT_REPORT_UPTIME)
                mqttSend(MQTT_TOPIC_UPTIME, String(uptime_seconds).c_str());
            #endif
            #if (HEARTBEAT_REPORT_DATETIME) && (NTP_SUPPORT)
                if (ntpSynced())  mqttSend(MQTT_TOPIC_DATETIME, ntpDateTime().c_str());
            #endif
            #if (HEARTBEAT_REPORT_FREEHEAP)
                mqttSend(MQTT_TOPIC_FREEHEAP, String(free_heap).c_str());
            #endif
            #if (HEARTBEAT_REPORT_RELAY)
                relayMQTT();
            #endif
            #if (LIGHT_PROVIDER != LIGHT_PROVIDER_NONE) & (HEARTBEAT_REPORT_LIGHT)
                lightMQTT();
            #endif
            #if (HEARTBEAT_REPORT_VCC)
            #if ADC_VCC_ENABLED
                mqttSend(MQTT_TOPIC_VCC, String(ESP.getVcc()).c_str());
            #endif
            #endif
            #if (HEARTBEAT_REPORT_STATUS)
                mqttSend(MQTT_TOPIC_STATUS, MQTT_STATUS_ONLINE, true);
            #endif
            #if (LOADAVG_REPORT)
                mqttSend(MQTT_TOPIC_LOADAVG, String(systemLoadAverage()).c_str());
            #endif
        }
    #endif

    // -------------------------------------------------------------------------
    // InfluxDB
    // -------------------------------------------------------------------------

    #if INFLUXDB_SUPPORT
        #if (HEARTBEAT_REPORT_UPTIME)
            idbSend(MQTT_TOPIC_UPTIME, String(uptime_seconds).c_str());
        #endif
        #if (HEARTBEAT_REPORT_FREEHEAP)
            idbSend(MQTT_TOPIC_FREEHEAP, String(free_heap).c_str());
        #endif
    #endif

}

#endif /// HEARTBEAT_ENABLED

unsigned int sectors(size_t size) {
    return (int) (size + SPI_FLASH_SEC_SIZE - 1) / SPI_FLASH_SEC_SIZE;
}

void info() {

    DEBUG_MSG_P(PSTR("\n\n"));
    DEBUG_MSG_P(PSTR("[INIT] %s %s\n"), (char *) APP_NAME, (char *) APP_VERSION);
    DEBUG_MSG_P(PSTR("[INIT] %s\n"), (char *) APP_AUTHOR);
    DEBUG_MSG_P(PSTR("[INIT] %s\n\n"), (char *) APP_WEBSITE);
    DEBUG_MSG_P(PSTR("[INIT] CPU chip ID: 0x%06X\n"), ESP.getChipId());
    DEBUG_MSG_P(PSTR("[INIT] CPU frequency: %u MHz\n"), ESP.getCpuFreqMHz());
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
    DEBUG_MSG_P(PSTR("[INIT] EEPROM size:       %8u bytes / %4d sectors\n"), settingsMaxSize(), sectors(settingsMaxSize()));
    DEBUG_MSG_P(PSTR("[INIT] Empty space:       %8u bytes /   4 sectors\n"), 4 * SPI_FLASH_SEC_SIZE);
    DEBUG_MSG_P(PSTR("\n"));

    // -------------------------------------------------------------------------

    #if SPIFFS_SUPPORT
        FSInfo fs_info;
        bool fs = SPIFFS.info(fs_info);
        if (fs) {
            DEBUG_MSG_P(PSTR("[INIT] SPIFFS total size: %8u bytes / %4d sectors\n"), fs_info.totalBytes, sectors(fs_info.totalBytes));
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

    DEBUG_MSG_P(PSTR("[INIT] BOARD: %s\n"), getBoardName().c_str());
    DEBUG_MSG_P(PSTR("[INIT] SUPPORT:"));

    #if ALEXA_SUPPORT
        DEBUG_MSG_P(PSTR(" ALEXA"));
    #endif
    #if BROKER_SUPPORT
        DEBUG_MSG_P(PSTR(" BROKER"));
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
    #if DOMOTICZ_SUPPORT
        DEBUG_MSG_P(PSTR(" DOMOTICZ"));
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
    #if MDNS_SERVER_SUPPORT
        DEBUG_MSG_P(PSTR(" MDNS_SERVER"));
    #endif
    #if MDNS_CLIENT_SUPPORT
        DEBUG_MSG_P(PSTR(" MDNS_CLIENT"));
    #endif
    #if MQTT_SUPPORT
        DEBUG_MSG_P(PSTR(" MQTT"));
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
    #if SCHEDULER_SUPPORT
        DEBUG_MSG_P(PSTR(" SCHEDULER"));
    #endif
    #if SENSOR_SUPPORT
        DEBUG_MSG_P(PSTR(" SENSOR"));
    #endif
    #if SPIFFS_SUPPORT
        DEBUG_MSG_P(PSTR(" SPIFFS"));
    #endif
    #if SSDP_SUPPORT
        DEBUG_MSG_P(PSTR(" SSDP"));
    #endif
    #if TELNET_SUPPORT
        DEBUG_MSG_P(PSTR(" TELNET"));
    #endif
    #if TERMINAL_SUPPORT
        DEBUG_MSG_P(PSTR(" TERMINAL"));
    #endif
    #if THINGSPEAK_SUPPORT
        DEBUG_MSG_P(PSTR(" THINGSPEAK"));
    #endif
    #if UART_MQTT_SUPPORT
        DEBUG_MSG_P(PSTR(" UART_MQTT"));
    #endif
    #if WEB_SUPPORT
        DEBUG_MSG_P(PSTR(" WEB"));
    #endif

    #if SENSOR_SUPPORT

        DEBUG_MSG_P(PSTR("\n"));
        DEBUG_MSG_P(PSTR("[INIT] SENSORS:"));

        #if AM2320_SUPPORT
            DEBUG_MSG_P(PSTR(" AM2320_I2C"));
        #endif
        #if ANALOG_SUPPORT
            DEBUG_MSG_P(PSTR(" ANALOG"));
        #endif
        #if BMX280_SUPPORT
            DEBUG_MSG_P(PSTR(" BMX280"));
        #endif
        #if DALLAS_SUPPORT
            DEBUG_MSG_P(PSTR(" DALLAS"));
        #endif
        #if DHT_SUPPORT
            DEBUG_MSG_P(PSTR(" DHTXX"));
        #endif
        #if DIGITAL_SUPPORT
            DEBUG_MSG_P(PSTR(" DIGITAL"));
        #endif
        #if ECH1560_SUPPORT
            DEBUG_MSG_P(PSTR(" ECH1560"));
        #endif
        #if EMON_ADC121_SUPPORT
            DEBUG_MSG_P(PSTR(" EMON_ADC121"));
        #endif
        #if EMON_ADS1X15_SUPPORT
            DEBUG_MSG_P(PSTR(" EMON_ADX1X15"));
        #endif
        #if EMON_ANALOG_SUPPORT
            DEBUG_MSG_P(PSTR(" EMON_ANALOG"));
        #endif
        #if EVENTS_SUPPORT
            DEBUG_MSG_P(PSTR(" EVENTS"));
        #endif
        #if GUVAS12SD_SUPPORT
            DEBUG_MSG_P(PSTR(" GUVAS12SD"));
        #endif
        #if HLW8012_SUPPORT
            DEBUG_MSG_P(PSTR(" HLW8012"));
        #endif
        #if MHZ19_SUPPORT
            DEBUG_MSG_P(PSTR(" MHZ19"));
        #endif
        #if PMSX003_SUPPORT
            DEBUG_MSG_P(PSTR(" PMSX003"));
        #endif
        #if PZEM004T_SUPPORT
            DEBUG_MSG_P(PSTR(" PZEM004T"));
        #endif
        #if SHT3X_I2C_SUPPORT
            DEBUG_MSG_P(PSTR(" SHT3X_I2C"));
        #endif
        #if SI7021_SUPPORT
            DEBUG_MSG_P(PSTR(" SI7021"));
        #endif
        #if V9261F_SUPPORT
            DEBUG_MSG_P(PSTR(" V9261F"));
        #endif

    #endif // SENSOR_SUPPORT

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

    DEBUG_MSG_P(PSTR("[INIT] Settings size: %u bytes\n"), settingsSize());
    DEBUG_MSG_P(PSTR("[INIT] Free heap: %u bytes\n"), getFreeHeap());
    #if ADC_VCC_ENABLED
        DEBUG_MSG_P(PSTR("[INIT] Power: %u mV\n"), ESP.getVcc());
    #endif

    DEBUG_MSG_P(PSTR("[INIT] Power saving delay value: %lu ms\n"), systemLoopDelay());

    #if SYSTEM_CHECK_ENABLED
        if (!systemCheck()) DEBUG_MSG_P(PSTR("\n[INIT] Device is in SAFE MODE\n"));
    #endif

    DEBUG_MSG_P(PSTR("\n"));

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
        status = EEPROM.read(EEPROM_CUSTOM_RESET);
        if (status > 0) resetReason(0);
        if (status > CUSTOM_RESET_MAX) status = 0;
    }
    return status;
}

void resetReason(unsigned char reason) {
    EEPROM.write(EEPROM_CUSTOM_RESET, reason);
    EEPROM.commit();
}

void reset(unsigned char reason) {
    resetReason(reason);
    ESP.restart();
}

void deferredReset(unsigned long delay, unsigned char reason) {
    _defer_reset.once_ms(delay, reset, reason);
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
