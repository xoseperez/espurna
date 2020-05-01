/*

BOARD MODULE

*/

#include "board.h"
#include "relay.h"
#include "sensor.h"

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

int getBoardId() {
    #if defined(ESPURNA_CORE) || defined(ESPURNA_CORE_WEBUI)
        return 0;
    #elif defined(NODEMCU_LOLIN)
        return 2;
    #elif defined(NODEMCU_BASIC)
        return 3;
    #elif defined(WEMOS_D1_MINI)
        return 4;
    #elif defined(WEMOS_D1_MINI_RELAYSHIELD)
        return 5;
    #elif defined(WEMOS_D1_TARPUNA_SHIELD)
        return 6;
    #elif defined(TINKERMAN_ESPURNA_H06)
        return 7;
    #elif defined(TINKERMAN_ESPURNA_H08)
        return 8;
    #elif defined(TINKERMAN_ESPURNA_SWITCH)
        return 9;
    #elif defined(TINKERMAN_RFM69GW)
        return 10;
    #elif defined(ITEAD_SONOFF_BASIC)
        return 11;
    #elif defined(ITEAD_SONOFF_RF)
        return 12;
    #elif defined(ITEAD_SONOFF_MINI)
        return 13;
    #elif defined(ITEAD_SONOFF_TH)
        return 14;
    #elif defined(ITEAD_SONOFF_SV)
        return 15;
    #elif defined(ITEAD_SLAMPHER)
        return 16;
    #elif defined(ITEAD_S20)
        return 17;
    #elif defined(ITEAD_SONOFF_TOUCH)
        return 18;
    #elif defined(ITEAD_SONOFF_POW)
        return 19;
    #elif defined(ITEAD_SONOFF_POW_R2)
        return 20;
    #elif defined(ITEAD_SONOFF_DUAL)
        return 21;
    #elif defined(ITEAD_SONOFF_DUAL_R2)
        return 22;
    #elif defined(ITEAD_SONOFF_4CH)
        return 23;
    #elif defined(ITEAD_SONOFF_4CH_PRO)
        return 24;
    #elif defined(ITEAD_1CH_INCHING)
        return 25;
    #elif defined(ITEAD_MOTOR)
        return 26;
    #elif defined(ITEAD_BNSZ01)
        return 27;
    #elif defined(ITEAD_SONOFF_RFBRIDGE)
        return 28;
    #elif defined(ITEAD_SONOFF_B1)
        return 29;
    #elif defined(ITEAD_SONOFF_LED)
        return 30;
    #elif defined(ITEAD_SONOFF_T1_1CH)
        return 31;
    #elif defined(ITEAD_SONOFF_T1_2CH)
        return 32;
    #elif defined(ITEAD_SONOFF_T1_3CH)
        return 33;
    #elif defined(ITEAD_SONOFF_S31)
        return 34;
    #elif defined(ITEAD_SONOFF_S31_LITE)
        return 35;
    #elif defined(ITEAD_SONOFF_IFAN02)
        return 36;
    #elif defined(ORVIBO_B25)
        return 37;
    #elif defined(YJZK_SWITCH_1CH)
        return 38;
    #elif defined(YJZK_SWITCH_2CH)
        return 39;
    #elif defined(YJZK_SWITCH_3CH)
        return 40;
    #elif defined(ELECTRODRAGON_WIFI_IOT)
        return 41;
    #elif defined(WORKCHOICE_ECOPLUG)
        return 42;
    #elif defined(AITHINKER_AI_LIGHT)
        return 43;
    #elif defined(LYASI_LIGHT)
        return 44;
    #elif defined(MAGICHOME_LED_CONTROLLER)
        return 45;
    #elif defined(MAGICHOME_LED_CONTROLLER_20)
        return 46;
    #elif defined(MAGICHOME_ZJ_WFMN_A_11)
        return 47;
    #elif defined(MAGICHOME_ZJ_WFMN_B_11)
        return 48;
    #elif defined(MAGICHOME_ZJ_WFMN_C_11)
        return 49;
    #elif defined(MAGICHOME_ZJ_ESPM_5CH_B_13)
        return 50;
    #elif defined(MAGICHOME_ZJ_LB_RGBWW_L)
        return 51;
    #elif defined(HUACANXING_H801)
        return 52;
    #elif defined(HUACANXING_H802)
        return 53;
    #elif defined(JANGOE_WIFI_RELAY_NC)
        return 54;
    #elif defined(JANGOE_WIFI_RELAY_NO)
        return 55;
    #elif defined(JORGEGARCIA_WIFI_RELAYS)
        return 56;
    #elif defined(OPENENERGYMONITOR_MQTT_RELAY)
        return 57;
    #elif defined(WION_50055)
        return 58;
    #elif defined(EXS_WIFI_RELAY_V31)
        return 59;
    #elif defined(EXS_WIFI_RELAY_V50)
        return 60;
    #elif defined(GENERIC_V9261F)
        return 61;
    #elif defined(GENERIC_ECH1560)
        return 62;
    #elif defined(MANCAVEMADE_ESPLIVE)
        return 63;
    #elif defined(INTERMITTECH_QUINLED)
        return 64;
    #elif defined(ARILUX_AL_LC01)
        return 65;
    #elif defined(ARILUX_AL_LC02)
        return 66;
    #elif defined(ARILUX_AL_LC02_V14)
        return 67;
    #elif defined(ARILUX_AL_LC06)
        return 68;
    #elif defined(ARILUX_AL_LC11)
        return 69;
    #elif defined(ARILUX_E27)
        return 70;
    #elif defined(XENON_SM_PW702U)
        return 71;
    #elif defined(ISELECTOR_SM_PW702)
        return 72;
    #elif defined(AUTHOMETION_LYT8266)
        return 73;
    #elif defined(GIZWITS_WITTY_CLOUD)
        return 74;
    #elif defined(KMC_70011)
        return 75;
    #elif defined(EUROMATE_WIFI_STECKER_SCHUKO)
        return 76;
    #elif defined(EUROMATE_WIFI_STECKER_SCHUKO_V2)
        return 77;
    #elif defined(GENERIC_8CH)
        return 78;
    #elif defined(STM_RELAY)
        return 79;
    #elif defined(TONBUX_POWERSTRIP02)
        return 80;
    #elif defined(LINGAN_SWA1)
        return 81;
    #elif defined(HEYGO_HY02)
        return 82;
    #elif defined(MAXCIO_WUS002S)
        return 83;
    #elif defined(MAXCIO_WDE004)
        return 84;
    #elif defined(OUKITEL_P1)
        return 85;
    #elif defined(YIDIAN_XSSSA05)
        return 86;
    #elif defined(TONBUX_XSSSA01)
        return 87;
    #elif defined(TONBUX_XSSSA06)
        return 88;
    #elif defined(GREEN_ESP8266RELAY)
        return 89;
    #elif defined(IKE_ESPIKE)
        return 90;
    #elif defined(ARNIEX_SWIFITCH)
        return 91;
    #elif defined(GENERIC_ESP01S_RELAY_V40)
        return 92;
    #elif defined(GENERIC_ESP01S_RGBLED_V10)
        return 93;
    #elif defined(GENERIC_ESP01S_DHT11_V10)
        return 94;
    #elif defined(GENERIC_ESP01S_DS18B20_V10)
        return 95;
    #elif defined(PILOTAK_ESP_DIN_V1)
        return 96;
    #elif defined(HELTEC_TOUCHRELAY)
        return 97;
    #elif defined(ZHILDE_EU44_W)
        return 98;
    #elif defined(ALLNET_4DUINO_IOT_WLAN_RELAIS)
        return 99;
    #elif defined(LUANI_HVIO)
        return 100;
    #elif defined(TONBUX_MOSQUITO_KILLER)
        return 101;
    #elif defined(NEO_COOLCAM_NAS_WR01W)
        return 102;
    #elif defined(DELTACO_SH_P01)
        return 103;
    #elif defined(DELTACO_SH_P03USB)
        return 104;
    #elif defined(FORNORM_ZLD_34EU)
        return 105;
    #elif defined(BH_ONOFRE)
        return 106;
    #elif defined(BLITZWOLF_BWSHPX)
        return 107;
    #elif defined(BLITZWOLF_BWSHPX_V23)
        return 108;
    #elif defined(BLITZWOLF_BWSHP5)
        return 109;
    #elif defined(TECKIN_SP21)
        return 110;
    #elif defined(TECKIN_SP22_V14)
        return 111;
    #elif defined(GOSUND_WS1)
        return 112;
    #elif defined(HOMECUBE_16A)
        return 113;
    #elif defined(VANZAVANZU_SMART_WIFI_PLUG_MINI)
        return 114;
    #elif defined(GENERIC_AG_L4)
        return 115;
    #elif defined(ALLTERCO_SHELLY1)
        return 116;
    #elif defined(ALLTERCO_SHELLY2)
        return 117;
    #elif defined(ALLTERCO_SHELLY1PM)
        return 118;
    #elif defined(ALLTERCO_SHELLY25)
        return 119;
    #elif defined(LOHAS_E27_9W)
        return 120;
    #elif defined(LOHAS_E26_A19)
        return 121;
    #elif defined(TECKIN_SB53)
        return 122;
    #elif defined(XIAOMI_SMART_DESK_LAMP)
        return 123;
    #elif defined(PHYX_ESP12_RGB)
        return 124;
    #elif defined(IWOOLE_LED_TABLE_LAMP)
        return 125;
    #elif defined(GENERIC_GU10)
        return 126;
    #elif defined(GENERIC_E14)
        return 127;
    #elif defined(DELTACO_SH_LEXXW)
        return 128;
    #elif defined(DELTACO_SH_LEXXRGB)
        return 129;
    #elif defined(NEXETE_A19)
        return 130;
    #elif defined(LOMBEX_LUX_NOVA2_TUNABLE_WHITE)
        return 131;
    #elif defined(LOMBEX_LUX_NOVA2_WHITE_COLOR)
        return 132;
    #elif defined(BESTEK_MRJ1011)
        return 133;
    #elif defined(GBLIFE_RGBW_SOCKET)
        return 134;
    #elif defined(SMARTLIFE_MINI_SMART_SOCKET)
        return 135;
    #elif defined(HAMA_WIFI_STECKDOSE_00176533)
        return 136;
    #elif defined(DIGOO_NX_SP202)
        return 137;
    #elif defined(FOXEL_LIGHTFOX_DUAL)
        return 138;
    #elif defined(TECKIN_SP20)
        return 139;
    #elif defined(LITESUN_LA_WF3)
        return 140;
    #elif defined(PSH_WIFI_PLUG)
        return 141;
    #elif defined(PSH_RGBW_CONTROLLER)
        return 142;
    #elif defined(PSH_WIFI_SENSOR)
        return 143;
    #elif defined(JINVOO_VALVE_SM_AW713)
        return 144;
    #elif defined(TUYA_GENERIC_DIMMER)
        return 145;
    #elif defined(ETEKCITY_ESW01_USA)
        return 146;
    #elif defined(FS_UAP1)
        return 147;
    #elif defined(TFLAG_NX_SMX00)
        return 148;
    #elif defined(MUVIT_IO_MIOBULB001)
        return 149;
    #elif defined(HYKKER_SMART_HOME_POWER_PLUG)
        return 150;
    #elif defined(KOGAN_SMARTER_HOME_PLUG_W_POW)
        return 151;
    #elif defined(LSC_SMART_LED_LIGHT_STRIP)
        return 152;
    #elif defined(EHOMEDIY_WT02)
        return 153;
    #elif defined(EHOMEDIY_WT03)
        return 154;
    #elif defined(LINKSPRITE_LINKNODE_R4)
        return 155;
    #elif defined(GENERIC_ESP01_512KB)
        return 156;
    #elif defined(GOSUND_WP3)
        return 157;
    #else
        return -1; // CUSTOM
    #endif
}

