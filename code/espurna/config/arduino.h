//--------------------------------------------------------------------------------
// These settings are normally provided by PlatformIO
// Uncomment the appropiate line(s) to build from the Arduino IDE
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
// Hardware
//--------------------------------------------------------------------------------

//#define NODEMCU_LOLIN
//#define WEMOS_D1_MINI
//#define WEMOS_D1_MINI_RELAYSHIELD
//#define TINKERMAN_ESPURNA_H06
//#define TINKERMAN_ESPURNA_H08
//#define TINKERMAN_RFM69GW
//#define ITEAD_SONOFF_BASIC
//#define ITEAD_SONOFF_RF
//#define ITEAD_SONOFF_TH
//#define ITEAD_SONOFF_SV
//#define ITEAD_SLAMPHER
//#define ITEAD_S20
//#define ITEAD_SONOFF_TOUCH
//#define ITEAD_SONOFF_POW
//#define ITEAD_SONOFF_POW_R2
//#define ITEAD_SONOFF_DUAL
//#define ITEAD_SONOFF_DUAL_R2
//#define ITEAD_SONOFF_4CH
//#define ITEAD_SONOFF_4CH_PRO
//#define ITEAD_1CH_INCHING
//#define ITEAD_MOTOR
//#define ITEAD_SONOFF_BNSZ01
//#define ITEAD_SONOFF_RFBRIDGE
//#define ITEAD_SONOFF_B1
//#define ITEAD_SONOFF_LED
//#define ITEAD_SONOFF_T1_1CH
//#define ITEAD_SONOFF_T1_2CH
//#define ITEAD_SONOFF_T1_3CH
//#define ITEAD_SONOFF_S31
//#define YJZK_SWITCH_2CH
//#define ELECTRODRAGON_WIFI_IOT
//#define WORKCHOICE_ECOPLUG
//#define AITHINKER_AI_LIGHT
//#define MAGICHOME_LED_CONTROLLER
//#define MAGICHOME_LED_CONTROLLER_20
//#define HUACANXING_H801
//#define HUACANXING_H802
//#define JANGOE_WIFI_RELAY_NC
//#define JANGOE_WIFI_RELAY_NO
//#define JORGEGARCIA_WIFI_RELAYS
//#define OPENENERGYMONITOR_MQTT_RELAY
//#define WION_50055
//#define EXS_WIFI_RELAY_V31
//#define GENERIC_V9261F
//#define GENERIC_ECH1560
//#define MANCAVEMADE_ESPLIVE
//#define INTERMITTECH_QUINLED
//#define ARILUX_AL_LC06
//#define ARILUX_E27
//#define XENON_SM_PW702U
//#define AUTHOMETION_LYT8266
//#define KMC_70011
//#define GENERIC_8CH
//#define ARILUX_AL_LC01
//#define ARILUX_AL_LC11
//#define ARILUX_AL_LC02
//#define WEMOS_D1_TARPUNA_SHIELD
//#define GIZWITS_WITTY_CLOUD
//#define EUROMATE_WIFI_STECKER_SCHUKO
//#define TONBUX_POWERSTRIP02
//#define LINGAN_SWA1
//#define HEYGO_HY02
//#define MAXCIO_WUS002S
//#define YIDIAN_XSSSA05
//#define TONBUX_XSSSA06
//#define GREEN_ESP8266RELAY
//#define IKE_ESPIKE
//#define ARNIEX_SWIFITCH
//#define GENERIC_ESP01S_RELAY_V40
//#define GENERIC_ESP01S_RGBLED_V10
//#define GENERIC_ESP01S_DHT11_V10
//#define GENERIC_ESP01S_DS18B20_V10
//#define HELTEC_TOUCHRELAY
//#define ZHILDE_EU44_W
//#define LUANI_HVIO
//#define ALLNET_4DUINO_IOT_WLAN_RELAIS
//#define TONBUX_MOSQUITO_KILLER
//#define NEO_COOLCAM_NAS_WR01W
//#define ESTINK_WIFI_POWER_STRIP
//#define PILOTAK_ESP_DIN_V1
//#define BLITZWOLF_BWSHP2
//#define BH_ONOFRE
//#define ITEAD_SONOFF_IFAN02
//#define GENERIC_AG_L4
//#define ALLTERCO_SHELLY1
//#define LOHAS_9W
//#define YJZK_SWITCH_1CH
//#define YJZK_SWITCH_3CH
//#define XIAOMI_SMART_DESK_LAMP
//#define ALLTERCO_SHELLY2
//#define PHYX_ESP12_RGB
//#define IWOOLE_LED_TABLE_LAMP
//#define EXS_WIFI_RELAY_V50

//--------------------------------------------------------------------------------
// Features (values below are non-default values)
//--------------------------------------------------------------------------------

//#define ALEXA_SUPPORT          0
//#define API_SUPPORT            0
//#define BROKER_SUPPORT         0
//#define BUTTON_SUPPORT         0
//#define DEBUG_SERIAL_SUPPORT   0
//#define DEBUG_TELNET_SUPPORT   0
//#define DEBUG_UDP_SUPPORT      1
//#define DEBUG_WEB_SUPPORT      0
//#define DOMOTICZ_SUPPORT       0
//#define ENCODER_SUPPORT        1
//#define HOMEASSISTANT_SUPPORT  0
//#define I2C_SUPPORT            1
//#define INFLUXDB_SUPPORT       1
//#define IR_SUPPORT             1
//#define LED_SUPPORT            0
//#define LLMNR_SUPPORT          1  // Only with Arduino Core 2.4.0
//#define MDNS_CLIENT_SUPPORT    1
//#define MDNS_SERVER_SUPPORT    0
//#define MQTT_SUPPORT           0
//#define NETBIOS_SUPPORT        1  // Only with Arduino Core 2.4.0
//#define NOFUSS_SUPPORT         1
//#define NTP_SUPPORT            0
//#define RFM69_SUPPORT          1
//#define RF_SUPPORT             1
//#define SCHEDULER_SUPPORT      0
//#define SENSOR_SUPPORT         1
//#define SPIFFS_SUPPORT         1
//#define SSDP_SUPPORT           1
//#define TELNET_SUPPORT         0
//#define TERMINAL_SUPPORT       0
//#define THINGSPEAK_SUPPORT     0
//#define UART_MQTT_SUPPORT      1
//#define WEB_SUPPORT            0

//--------------------------------------------------------------------------------
// Sensors (values below are non-default values)
//--------------------------------------------------------------------------------

//#define AM2320_SUPPORT         1
//#define ANALOG_SUPPORT         1
//#define BH1750_SUPPORT         1
//#define BMX280_SUPPORT         1
//#define CSE7766_SUPPORT        1
//#define DALLAS_SUPPORT         1
//#define DHT_SUPPORT            1
//#define DIGITAL_SUPPORT        1
//#define ECH1560_SUPPORT        1
//#define EMON_ADC121_SUPPORT    1
//#define EMON_ADS1X15_SUPPORT   1
//#define EMON_ANALOG_SUPPORT    1
//#define EVENTS_SUPPORT         1
//#define GEIGER_SUPPORT         1
//#define GUVAS12SD_SUPPORT      1
//#define HLW8012_SUPPORT        1
//#define MHZ19_SUPPORT          1
//#define MICS2710_SUPPORT       1
//#define MICS5525_SUPPORT       1
//#define NTC_SUPPORT            1
//#define PMSX003_SUPPORT        1
//#define PZEM004T_SUPPORT       1
//#define SDS011_SUPPORT         1
//#define SENSEAIR_SUPPORT       1
//#define SHT3X_I2C_SUPPORT      1
//#define SI7021_SUPPORT         1
//#define SONAR_SUPPORT          1
//#define TMP3X_SUPPORT          1
//#define V9261F_SUPPORT         1
