// -----------------------------------------------------------------------------
// Devices
// -----------------------------------------------------------------------------

// New boards have to be added just before the BOARD_LAST line,
// then add configuration in the hardware.ino
enum boards {

    BOARD_UNKNOWN,
    BOARD_CUSTOM,

    BOARD_NODEMCU_LOLIN,
    BOARD_WEMOS_D1_MINI_RELAYSHIELD,
    BOARD_ITEAD_SONOFF_BASIC,
    BOARD_ITEAD_SONOFF_TH,
    BOARD_ITEAD_SONOFF_SV,
    BOARD_ITEAD_SONOFF_TOUCH,
    BOARD_ITEAD_SONOFF_POW,
    BOARD_ITEAD_SONOFF_DUAL,
    BOARD_ITEAD_1CH_INCHING,
    BOARD_ITEAD_SONOFF_4CH,
    BOARD_ITEAD_SLAMPHER,
    BOARD_ITEAD_S20,
    BOARD_ELECTRODRAGON_WIFI_IOT,
    BOARD_WORKCHOICE_ECOPLUG,
    BOARD_JANGOE_WIFI_RELAY_NC,
    BOARD_JANGOE_WIFI_RELAY_NO,
    BOARD_OPENENERGYMONITOR_MQTT_RELAY,
    BOARD_JORGEGARCIA_WIFI_RELAYS,
    BOARD_AITHINKER_AI_LIGHT,
    BOARD_MAGICHOME_LED_CONTROLLER,
    BOARD_ITEAD_MOTOR,
    BOARD_TINKERMAN_ESPURNA_H06,
    BOARD_HUACANXING_H801,
    BOARD_ITEAD_BNSZ01,
    BOARD_ITEAD_SONOFF_RFBRIDGE,
    BOARD_ITEAD_SONOFF_4CH_PRO,
    BOARD_ITEAD_SONOFF_B1,
    BOARD_ITEAD_SONOFF_LED,
    BOARD_ITEAD_SONOFF_T1_1CH,
    BOARD_ITEAD_SONOFF_T1_2CH,
    BOARD_ITEAD_SONOFF_T1_3CH,
    BOARD_ITEAD_SONOFF_RF,
    BOARD_WION_50055,
    BOARD_EXS_WIFI_RELAY_V31,
    BOARD_HUACANXING_H802,
    BOARD_GENERIC_V9261F,
    BOARD_GENERIC_ECH1560,
    BOARD_TINKERMAN_ESPURNA_H08,
    BOARD_MANCAVEMADE_ESPLIVE,
    BOARD_INTERMITTECH_QUINLED,
    BOARD_MAGICHOME_LED_CONTROLLER_20,
    BOARD_ARILUX_AL_LC06,
    BOARD_XENON_SM_PW702U,
    BOARD_AUTHOMETION_LYT8266,
    BOARD_ARILUX_E27,
    BOARD_YJZK_SWITCH_2CH,
    BOARD_ITEAD_SONOFF_DUAL_R2,
    BOARD_GENERIC_8CH,
    BOARD_ARILUX_AL_LC01,
    BOARD_ARILUX_AL_LC11,
    BOARD_ARILUX_AL_LC02,
    BOARD_KMC_70011,
    BOARD_GIZWITS_WITTY_CLOUD,
    BOARD_EUROMATE_WIFI_STECKER_SCHUKO,
    BOARD_TONBUX_POWERSTRIP02,
    BOARD_LINGAN_SWA1,
    BOARD_HEYGO_HY02,
    BOARD_MAXCIO_WUS002S,
    BOARD_YIDIAN_XSSSA05,
    BOARD_TONBUX_XSSSA06,
    BOARD_GREEN_ESP8266RELAY,
    BOARD_IKE_ESPIKE,
    BOARD_ARNIEX_SWIFITCH,
    BOARD_GENERIC_ESP01S_RELAY_V40,
    BOARD_GENERIC_ESP01S_RGBLED_V10,
    BOARD_HELTEC_TOUCHRELAY,
    BOARD_GENERIC_ESP01S_DHT11_V10,
    BOARD_GENERIC_ESP01S_DS18B20_V10,
    BOARD_ZHILDE_EU44_W,
    BOARD_ITEAD_SONOFF_POW_R2,
    BOARD_LUANI_HVIO,
    BOARD_ALLNET_4DUINO_IOT_WLAN_RELAIS,
    BOARD_TONBUX_MOSQUITO_KILLER,
    BOARD_NEO_COOLCAM_NAS_WR01W,
    BOARD_PILOTAK_ESP_DIN_V1,
    BOARD_ESTINK_WIFI_POWER_STRIP,
    BOARD_BH_ONOFRE,
    BOARD_BLITZWOLF_BWSHP2,
    BOARD_TINKERMAN_ESPURNA_SWITCH,
    BOARD_ITEAD_SONOFF_S31,
    BOARD_STM_RELAY,
    BOARD_VANZAVANZU_SMART_WIFI_PLUG_MINI,
    BOARD_GENERIC_GEIGER_COUNTER,
    BOARD_TINKERMAN_RFM69GW,
    BOARD_ITEAD_SONOFF_IFAN02,
    BOARD_GENERIC_AG_L4,
    BOARD_HOMECUBE_16A,

    BOARD_LAST

};

// -----------------------------------------------------------------------------
// Board => Image type
// -----------------------------------------------------------------------------

#if \
    defined(ALLNET_4DUINO_IOT_WLAN_RELAIS) || \
    defined(ARNIEX_SWIFITCH) || \
    defined(BH_ONOFRE) || \
    defined(ELECTRODRAGON_WIFI_IOT) || \
    defined(ESTINK_WIFI_POWER_STRIP) || \
    defined(EUROMATE_WIFI_STECKER_SCHUKO) || \
    defined(EXS_WIFI_RELAY_V31) || \
    defined(GENERIC_8CH) || \
    defined(GENERIC_ESP01S_RELAY_V40) || \
    defined(GENERIC_ESP01S_RGBLED_V10) || \
    defined(GREEN_ESP8266RELAY) || \
    defined(HELTEC_TOUCHRELAY) || \
    defined(HEYGO_HY02) || \
    defined(IKE_ESPIKE) || \
    defined(ITEAD_1CH_INCHING) || \
    defined(ITEAD_MOTOR) || \
    defined(ITEAD_S20) || \
    defined(ITEAD_SLAMPHER) || \
    defined(ITEAD_SONOFF_4CH) || \
    defined(ITEAD_SONOFF_4CH_PRO) || \
    defined(ITEAD_SONOFF_BASIC) || \
    defined(ITEAD_SONOFF_DUAL_R2) || \
    defined(ITEAD_SONOFF_IFAN02) || \
    defined(ITEAD_SONOFF_RF) || \
    defined(ITEAD_SONOFF_SV) || \
    defined(ITEAD_SONOFF_T1_1CH) || \
    defined(ITEAD_SONOFF_T1_2CH) || \
    defined(ITEAD_SONOFF_T1_3CH) || \
    defined(ITEAD_SONOFF_TOUCH) || \
    defined(JANGOE_WIFI_RELAY_NC) || \
    defined(JANGOE_WIFI_RELAY_NO) || \
    defined(JORGEGARCIA_WIFI_RELAYS) || \
    defined(LINGAN_SWA1) || \
    defined(LUANI_HVIO) || \
    defined(NEO_COOLCAM_NAS_WR01W) || \
    defined(NODEMCU_LOLIN) || \
    defined(OPENENERGYMONITOR_MQTT_RELAY) || \
    defined(TINKERMAN_ESPURNA_SWITCH) || \
    defined(TONBUX_MOSQUITO_KILLER) || \
    defined(TONBUX_POWERSTRIP02) || \
    defined(TONBUX_XSSSA06) || \
    defined(WEMOS_D1_MINI_RELAYSHIELD) || \
    defined(WION_50055) || \
    defined(WORKCHOICE_ECOPLUG) || \
    defined(XENON_SM_PW702U) || \
    defined(YJZK_SWITCH_2CH) || \
    defined(ZHILDE_EU44_W)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------

#elif \
    defined(BLITZWOLF_BWSHP2) || \
    defined(HOMECUBE_16A) || \
    defined(ITEAD_SONOFF_POW) || \
    defined(KMC_70011) || \
    defined(MAXCIO_WUS002S) || \
    defined(TINKERMAN_ESPURNA_H06) || \
    defined(TINKERMAN_ESPURNA_H08) || \
    defined(VANZAVANZU_SMART_WIFI_PLUG_MINI) || \
    defined(YIDIAN_XSSSA05)

    #define ESPURNA_IMAGE               ESPURNA_HLW8012


// -----------------------------------------------------------------------------

#elif \
    defined(ITEAD_SONOFF_POW_R2) || \
    defined(ITEAD_SONOFF_S31)

    #define ESPURNA_IMAGE               ESPURNA_CSE77XX

// -----------------------------------------------------------------------------

#elif defined(GENERIC_V9261F)

    #define ESPURNA_IMAGE               ESPURNA_V9261F

// -----------------------------------------------------------------------------

#elif defined(GENERIC_ECH1560)

    #define ESPURNA_IMAGE               ESPURNA_ECH1560

// -----------------------------------------------------------------------------

#elif \
    defined(ARILUX_AL_LC01) || \
    defined(ARILUX_AL_LC02) || \
    defined(ARILUX_AL_LC06) || \
    defined(ARILUX_AL_LC11) || \
    defined(AUTHOMETION_LYT8266) || \
    defined(GENERIC_AG_L4) || \
    defined(HUACANXING_H801) || \
    defined(HUACANXING_H802) || \
    defined(INTERMITTECH_QUINLED) || \
    defined(ITEAD_BNSZ01) || \
    defined(ITEAD_SONOFF_LED)

    #define ESPURNA_IMAGE               ESPURNA_DIMMER

// -----------------------------------------------------------------------------

#elif \
    defined(AITHINKER_AI_LIGHT) || \
    defined(ARILUX_E27) || \
    defined(ITEAD_SONOFF_B1)

    #define ESPURNA_IMAGE               ESPURNA_MY92XX

// -----------------------------------------------------------------------------

#elif \
    defined(GENERIC_ESP01S_DHT11_V10) || \
    defined(GENERIC_ESP01S_DS18B20_V10) || \
    defined(ITEAD_SONOFF_TH) || \
    defined(MANCAVEMADE_ESPLIVE)

    #define ESPURNA_IMAGE               ESPURNA_SENSOR


// -----------------------------------------------------------------------------

#elif defined(TINKERMAN_RFM69GW)

    #define ESPURNA_IMAGE               ESPURNA_RFM69

// -----------------------------------------------------------------------------

#elif defined(ITEAD_SONOFF_DUAL)

    #define ESPURNA_IMAGE               ESPURNA_SONOFF_DUAL

// -----------------------------------------------------------------------------

#elif defined(STM_RELAY)

    #define ESPURNA_IMAGE               ESPURNA_STM

// -----------------------------------------------------------------------------

#elif defined(GENERIC_GEIGER_COUNTER)

    #define ESPURNA_IMAGE               ESPURNA_GEIGER

// -----------------------------------------------------------------------------
// PENDING
// -----------------------------------------------------------------------------

#elif defined(ITEAD_SONOFF_RFBRIDGE)

    #define ESPURNA_IMAGE               ESPURNA_SONOFF_RFBRIDGE

    // Info
    #define RELAY_PROVIDER              RELAY_PROVIDER_RFBRIDGE

    // RFB Direct hack thanks to @wildwiz
    // https://github.com/xoseperez/espurna/wiki/Hardware-Itead-Sonoff-RF-Bridge---Direct-Hack
    #ifndef RFB_DIRECT
    #define RFB_DIRECT                  0
    #endif

    #ifndef RFB_RX_PIN
    #define RFB_RX_PIN                  4   // GPIO for RX when RFB_DIRECT
    #endif

    #ifndef RFB_TX_PIN
    #define RFB_TX_PIN                  5   // GPIO for TX when RFB_DIRECT
    #endif

    // When using un-modified harware, ESPurna communicates with the secondary
    // MCU EFM8BB1 via UART at 19200 bps so we need to change the speed of
    // the port and remove UART noise on serial line
    #if not RFB_DIRECT
    #define SERIAL_BAUDRATE             19200
    #define DEBUG_SERIAL_SUPPORT        0
    #endif

// -----------------------------------------------------------------------------

#elif defined(PILOTAK_ESP_DIN_V1)

    #define ESPURNA_IMAGE               ESPURNA_SENSOR

    #ifndef RF_SUPPORT
    #define RF_SUPPORT                  1       // Does this need the RFBRIDGE image?
    #endif
    #define RF_PIN                      14

// -----------------------------------------------------------------------------

#elif defined(MAGICHOME_LED_CONTROLLER)

    #define ESPURNA_IMAGE               ESPURNA_DIMMER

    // IR
    #define IR_SUPPORT                  1
    #define IR_RECEIVER_PIN             4
    #define IR_BUTTON_SET               1

#elif defined(MAGICHOME_LED_CONTROLLER_20)

    #define ESPURNA_IMAGE               ESPURNA_DIMMER

    // IR
    #define IR_SUPPORT                  1
    #define IR_RECEIVER_PIN             4
    #define IR_BUTTON_SET               1

#elif defined(GIZWITS_WITTY_CLOUD)

    #define ESPURNA_IMAGE               ESPURNA_DIMMER

    #define ANALOG_SUPPORT              1   // TODO: specific or generic?

// -----------------------------------------------------------------------------
// TEST boards (do not use!!)
// -----------------------------------------------------------------------------

#elif defined(TRAVIS01)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

    // A bit of I2C - pins 3,4
    #define I2C_SDA_PIN                 3
    #define I2C_SCL_PIN                 4

    // And, as they say in "From Dusk till Dawn":
    // This is a sensor blow out!
    // Alright, we got white sensor, black sensor, spanish sensor, yellow sensor. We got hot sensor, cold sensor.
    // We got wet sensor. We got smelly sensor. We got hairy sensor, bloody sensor. We got snapping sensor.
    // We got silk sensor, velvet sensor, naugahyde sensor. We even got horse sensor, dog sensor, chicken sensor.
    // C'mon, you want sensor, come on in sensor lovers!
    // If we don’t got it, you don't want it!
    #define AM2320_SUPPORT              1
    #define BH1750_SUPPORT              1
    #define BMX280_SUPPORT              1
    #define SHT3X_I2C_SUPPORT           1
    #define EMON_ADC121_SUPPORT         1
    #define EMON_ADS1X15_SUPPORT        1
    #define SHT3X_I2C_SUPPORT           1
    #define SI7021_SUPPORT              1
    #define PMSX003_SUPPORT             1
    #define SENSEAIR_SUPPORT            1


    // A bit of lights - pin 5
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT           1
    #define LIGHT_CHANNELS              1
    #define LIGHT_CH1_PIN               5
    #define LIGHT_CH1_INVERSE           0

    // A bit of HLW8012 - pins 6,7,8
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             6
    #define HLW8012_CF1_PIN             7
    #define HLW8012_CF_PIN              8

    // A bit of Dallas - pin 9
    #ifndef DALLAS_SUPPORT
    #define DALLAS_SUPPORT              1
    #endif
    #define DALLAS_PIN                  9

    // A bit of ECH1560 - pins 10,11, 12
    #ifndef ECH1560_SUPPORT
    #define ECH1560_SUPPORT             1
    #endif
    #define ECH1560_CLK_PIN             10
    #define ECH1560_MISO_PIN            11
    #define ECH1560_INVERTED            12

#elif defined(TRAVIS02)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

    // A bit of CSE7766 - pin 1
    #ifndef CSE7766_SUPPORT
    #define CSE7766_SUPPORT             1
    #endif
    #define CSE7766_PIN                 1

    // Relay type dual  - pins 2,3
    #define RELAY_PROVIDER              RELAY_PROVIDER_DUAL
    #define RELAY1_PIN                  2
    #define RELAY2_PIN                  3
    #define RELAY1_TYPE                 RELAY_TYPE_NORMAL
    #define RELAY2_TYPE                 RELAY_TYPE_NORMAL

    // IR - pin 4
    #define IR_SUPPORT                  1
    #define IR_RECEIVER_PIN             4
    #define IR_BUTTON_SET               1

    // A bit of DHT - pin 5
    #ifndef DHT_SUPPORT
    #define DHT_SUPPORT                 1
    #endif
    #define DHT_PIN                     5

    // A bit of TMP3X (analog)
    #define TMP3X_SUPPORT               1

    // A bit of EVENTS - pin 10
    #define EVENTS_SUPPORT              1
    #define EVENTS_PIN                  6

    // Sonar
    #define SONAR_SUPPORT               1
    #define SONAR_TRIGGER               7
    #define SONAR_ECHO                  8

    // MHZ19
    #define MHZ19_SUPPORT               1
    #define MHZ19_RX_PIN                9
    #define MHZ19_TX_PIN                10

    // PZEM004T
    #define PZEM004T_SUPPORT            1
    #define PZEM004T_RX_PIN             11
    #define PZEM004T_TX_PIN             12

    // V9261F
    #define V9261F_SUPPORT              1
    #define V9261F_PIN                  13

    // GUVAS12SD
    #define GUVAS12SD_SUPPORT           1
    #define GUVAS12SD_PIN               14

    // Test non-default modules
    #define MDNS_CLIENT_SUPPORT         1
    #define NOFUSS_SUPPORT              1
    #define UART_MQTT_SUPPORT           1
    #define INFLUXDB_SUPPORT            1
    #define IR_SUPPORT                  1

#elif defined(TRAVIS03)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

    // MY9231 Light - pins 1,2
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_MY92XX
    #define DUMMY_RELAY_COUNT           1
    #define LIGHT_CHANNELS              5
    #define MY92XX_MODEL                MY92XX_MODEL_MY9231
    #define MY92XX_CHIPS                2
    #define MY92XX_DI_PIN               1
    #define MY92XX_DCKI_PIN             2
    #define MY92XX_COMMAND              MY92XX_COMMAND_DEFAULT
    #define MY92XX_MAPPING              4, 3, 5, 0, 1

    // A bit of Analog EMON (analog)
    #ifndef EMON_ANALOG_SUPPORT
    #define EMON_ANALOG_SUPPORT         1
    #endif

    // Test non-default modules
    #define LLMNR_SUPPORT               1
    #define NETBIOS_SUPPORT             1
    #define SSDP_SUPPORT                1

// -----------------------------------------------------------------------------
// ESPurna Core
// -----------------------------------------------------------------------------

#else

    // This is a special device targeted to generate a light-weight binary image
    // meant to be able to do two-step-updates:
    // https://github.com/xoseperez/espurna/wiki/TwoStepUpdates

    #define ESPURNA_IMAGE               ESPURNA_CORE

#endif

// -----------------------------------------------------------------------------
// Image definitions
// -----------------------------------------------------------------------------

#if ESPURNA_IMAGE == ESPURNA_CORE

    // Disable non-core modules
    #define ALEXA_SUPPORT               0
    #define BROKER_SUPPORT              0
    #define BUTTON_SUPPORT              0
    #define DOMOTICZ_SUPPORT            0
    #define HOMEASSISTANT_SUPPORT       0
    #define I2C_SUPPORT                 0
    #define MDNS_SERVER_SUPPORT         0
    #define MQTT_SUPPORT                0
    #define NTP_SUPPORT                 0
    #define SCHEDULER_SUPPORT           0
    #define SENSOR_SUPPORT              0
    #define THINGSPEAK_SUPPORT          0
    #define WEB_SUPPORT                 0

    // Extra light-weight image
    //#define DEBUG_SERIAL_SUPPORT       0
    //#define DEBUG_TELNET_SUPPORT       0
    //#define DEBUG_WEB_SUPPORT          0
    //#define LED_SUPPORT                0
    //#define TELNET_SUPPORT             0
    //#define TERMINAL_SUPPORT           0

#elif ESPURNA_IMAGE == ESPURNA_BASIC

#elif ESPURNA_IMAGE == ESPURNA_DIMMER

    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER

#elif ESPURNA_IMAGE == ESPURNA_MY92XX

    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_MY92XX
    #define MY92XX_COMMAND              MY92XX_COMMAND_DEFAULT

#elif ESPURNA_IMAGE == ESPURNA_EMON

    #ifndef EMON_ANALOG_SUPPORT
    #define EMON_ANALOG_SUPPORT         1
    #endif

    #ifndef EMON_ADC121_SUPPORT
    #define EMON_ADC121_SUPPORT         1
    #endif

    #ifndef EMON_ADS1X15_SUPPORT
    #define EMON_ADS1X15_SUPPORT        1
    #endif

#elif ESPURNA_IMAGE == ESPURNA_HLW8012

    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif

#elif ESPURNA_IMAGE == ESPURNA_CSE77XX

    #ifndef CSE7766_SUPPORT
    #define CSE7766_SUPPORT             1
    #endif

#elif ESPURNA_IMAGE == ESPURNA_V9261F

    #ifndef V9261F_SUPPORT
    #define V9261F_SUPPORT              1
    #endif

    #ifndef ALEXA_SUPPORT
    #define ALEXA_SUPPORT               0
    #endif

#elif ESPURNA_IMAGE == ESPURNA_ECH1560

    #ifndef ECH1560_SUPPORT
    #define ECH1560_SUPPORT             1
    #endif

    #ifndef ALEXA_SUPPORT
    #define ALEXA_SUPPORT               0
    #endif

#elif ESPURNA_IMAGE == ESPURNA_SENSOR

    #ifndef ANALOG_SUPPORT
    #define ANALOG_SUPPORT              1
    #endif

    #ifndef DALLAS_SUPPORT
    #define DALLAS_SUPPORT              1
    #endif

    #ifndef DHT_SUPPORT
    #define DHT_SUPPORT                 1
    #endif

    #ifndef DIGITAL_SUPPORT
    #define DIGITAL_SUPPORT             1
    #endif

#elif ESPURNA_IMAGE == ESPURNA_SONOFF_DUAL

    #define SERIAL_BAUDRATE             19230
    #define RELAY_PROVIDER              RELAY_PROVIDER_DUAL
    #define DEBUG_SERIAL_SUPPORT        0

#elif ESPURNA_IMAGE == ESPURNA_SONOFF_RFBRIDGE

#elif ESPURNA_IMAGE == ESPURNA_RFM69

    // RFM69GW
    #define RFM69_SUPPORT               1

    // Disable non-core modules
    #define ALEXA_SUPPORT               0
    #define DOMOTICZ_SUPPORT            0
    #define HOMEASSISTANT_SUPPORT       0
    #define I2C_SUPPORT                 0
    #define SCHEDULER_SUPPORT           0
    #define SENSOR_SUPPORT              0
    #define THINGSPEAK_SUPPORT          0

#elif ESPURNA_IMAGE == ESPURNA_STM

    #define RELAY_PROVIDER              RELAY_PROVIDER_STM
    #define DEBUG_SERIAL_SUPPORT        0

#elif ESPURNA_IMAGE == ESPURNA_GEIGER

    // Enable Geiger Counter
    #define GEIGER_SUPPORT              1

    // Disable uneeded modules
    #define ALEXA_SUPPORT               0
    #define SCHEDULER_SUPPORT           0

#endif
