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

    BOARD_LAST

};

// -----------------------------------------------------------------------------
// Development boards
// -----------------------------------------------------------------------------

#if defined(NODEMCU_LOLIN)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

#elif defined(WEMOS_D1_MINI_RELAYSHIELD)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// ESPurna
// -----------------------------------------------------------------------------

#elif defined(TINKERMAN_ESPURNA_H06)

    #define ESPURNA_IMAGE               ESPURNA_POWER

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             2
    #define HLW8012_CF1_PIN             13
    #define HLW8012_CF_PIN              14

#elif defined(TINKERMAN_ESPURNA_H08)

    #define ESPURNA_IMAGE               ESPURNA_POWER

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             5
    #define HLW8012_CF1_PIN             13
    #define HLW8012_CF_PIN              14

#elif defined(TINKERMAN_ESPURNA_SWITCH)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// Check http://tinkerman.cat/rfm69-wifi-gateway/
#elif defined(TINKERMAN_RFM69GW)

    #define ESPURNA_IMAGE               ESPURNA_RFM69

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

// -----------------------------------------------------------------------------
// Itead Studio boards
// -----------------------------------------------------------------------------

#elif defined(ITEAD_SONOFF_BASIC)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

#elif defined(ITEAD_SONOFF_RF)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

#elif defined(ITEAD_SONOFF_TH)

    #define ESPURNA_IMAGE               ESPURNA_SENSOR

    // Jack is connected to GPIO14 (and with a small hack to GPIO4)
    #ifndef DALLAS_SUPPORT
    #define DALLAS_SUPPORT              1
    #endif
    #define DALLAS_PIN                  14

    #ifndef DHT_SUPPORT
    #define DHT_SUPPORT                 1
    #endif
    #define DHT_PIN                     14

#elif defined(ITEAD_SONOFF_SV)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

#elif defined(ITEAD_SLAMPHER)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

#elif defined(ITEAD_S20)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

#elif defined(ITEAD_SONOFF_TOUCH)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

#elif defined(ITEAD_SONOFF_POW)

    #define ESPURNA_IMAGE               ESPURNA_POWER

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             5
    #define HLW8012_CF1_PIN             13
    #define HLW8012_CF_PIN              14

#elif defined(ITEAD_SONOFF_POW_R2)

    #define ESPURNA_IMAGE               ESPURNA_POWER

    // Disable UART noise
    #define DEBUG_SERIAL_SUPPORT        0

    // CSE7766
    #ifndef CSE7766_SUPPORT
    #define CSE7766_SUPPORT             1
    #endif
    #define CSE7766_PIN                 1

#elif defined(ITEAD_SONOFF_DUAL)

    #define ESPURNA_IMAGE               ESPURNA_SONOFF_DUAL

    #define SERIAL_BAUDRATE             19230
    #define RELAY_PROVIDER              RELAY_PROVIDER_DUAL
    #define DEBUG_SERIAL_SUPPORT        0

#elif defined(ITEAD_SONOFF_DUAL_R2)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

#elif defined(ITEAD_SONOFF_4CH)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

#elif defined(ITEAD_SONOFF_4CH_PRO)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

#elif defined(ITEAD_1CH_INCHING)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

#elif defined(ITEAD_MOTOR)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

#elif defined(ITEAD_BNSZ01)

    #define ESPURNA_IMAGE               ESPURNA_LIGHT

    // Info
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER
    #define LIGHT_CHANNELS              1
    #define LIGHT_CH1_PIN               12
    #define LIGHT_CH1_INVERSE           0

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

#elif defined(ITEAD_SONOFF_B1)

    #define ESPURNA_IMAGE               ESPURNA_LIGHT

    // Info
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_MY92XX
    #define LIGHT_CHANNELS              5
    #define MY92XX_MODEL                MY92XX_MODEL_MY9231
    #define MY92XX_CHIPS                2
    #define MY92XX_DI_PIN               12
    #define MY92XX_DCKI_PIN             14
    #define MY92XX_COMMAND              MY92XX_COMMAND_DEFAULT
    #define MY92XX_MAPPING              4, 3, 5, 0, 1
    #define LIGHT_WHITE_FACTOR          (0.1)                    // White LEDs are way more bright in the B1

#elif defined(ITEAD_SONOFF_LED)

    #define ESPURNA_IMAGE               ESPURNA_LIGHT

    // Info
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER
    #define LIGHT_CHANNELS              2
    #define LIGHT_CH1_PIN               12  // Cold white
    #define LIGHT_CH2_PIN               14  // Warm white
    #define LIGHT_CH1_INVERSE           0
    #define LIGHT_CH2_INVERSE           0

#elif defined(ITEAD_SONOFF_T1_1CH)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

#elif defined(ITEAD_SONOFF_T1_2CH)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

#elif defined(ITEAD_SONOFF_T1_3CH)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

#elif defined(ITEAD_SONOFF_S31)

    #define ESPURNA_IMAGE               ESPURNA_POWER

    // Disable UART noise
    #define DEBUG_SERIAL_SUPPORT        0

    // CSE7766
    #define CSE7766_SUPPORT             1
    #define CSE7766_PIN                 1

#elif defined(ITEAD_SONOFF_IFAN02)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// YJZK
// -----------------------------------------------------------------------------

#elif defined(YJZK_SWITCH_2CH)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// Electrodragon boards
// -----------------------------------------------------------------------------

#elif defined(ELECTRODRAGON_WIFI_IOT)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// WorkChoice ecoPlug
// -----------------------------------------------------------------------------

#elif defined(WORKCHOICE_ECOPLUG)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// AI Thinker
// -----------------------------------------------------------------------------

#elif defined(AITHINKER_AI_LIGHT)

    #define ESPURNA_IMAGE               ESPURNA_LIGHT

    // Info
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_MY92XX
    #define LIGHT_CHANNELS              4
    #define MY92XX_MODEL                MY92XX_MODEL_MY9291
    #define MY92XX_CHIPS                1
    #define MY92XX_DI_PIN               13
    #define MY92XX_DCKI_PIN             15
    #define MY92XX_COMMAND              MY92XX_COMMAND_DEFAULT
    #define MY92XX_MAPPING              0, 1, 2, 3

// -----------------------------------------------------------------------------
// LED Controller
// -----------------------------------------------------------------------------

#elif defined(MAGICHOME_LED_CONTROLLER)

    #define ESPURNA_IMAGE               ESPURNA_LIGHT

    // Info
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT           1
    #define LIGHT_CHANNELS              4
    #define LIGHT_CH1_PIN               14      // RED
    #define LIGHT_CH2_PIN               5       // GREEN
    #define LIGHT_CH3_PIN               12      // BLUE
    #define LIGHT_CH4_PIN               13      // WHITE
    #define LIGHT_CH1_INVERSE           0
    #define LIGHT_CH2_INVERSE           0
    #define LIGHT_CH3_INVERSE           0
    #define LIGHT_CH4_INVERSE           0

    // IR
    #define IR_SUPPORT                  1
    #define IR_RECEIVER_PIN             4
    #define IR_BUTTON_SET               1

#elif defined(MAGICHOME_LED_CONTROLLER_20)

    #define ESPURNA_IMAGE               ESPURNA_LIGHT

    // Info
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER
    #define LIGHT_CHANNELS              4
    #define LIGHT_CH1_PIN               5       // RED
    #define LIGHT_CH2_PIN               12      // GREEN
    #define LIGHT_CH3_PIN               13      // BLUE
    #define LIGHT_CH4_PIN               15      // WHITE
    #define LIGHT_CH1_INVERSE           0
    #define LIGHT_CH2_INVERSE           0
    #define LIGHT_CH3_INVERSE           0
    #define LIGHT_CH4_INVERSE           0

    // IR
    #define IR_SUPPORT                  1
    #define IR_RECEIVER_PIN             4
    #define IR_BUTTON_SET               1

// -----------------------------------------------------------------------------
// HUACANXING H801 & H802
// -----------------------------------------------------------------------------

#elif defined(HUACANXING_H801)

    #define ESPURNA_IMAGE               ESPURNA_LIGHT

    // Info
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER
    #define DEBUG_PORT                  Serial1
    #define SERIAL_RX_ENABLED           1
    #define LIGHT_CHANNELS              5
    #define LIGHT_CH1_PIN               15      // RED
    #define LIGHT_CH2_PIN               13      // GREEN
    #define LIGHT_CH3_PIN               12      // BLUE
    #define LIGHT_CH4_PIN               14      // WHITE1
    #define LIGHT_CH5_PIN               4       // WHITE2
    #define LIGHT_CH1_INVERSE           0
    #define LIGHT_CH2_INVERSE           0
    #define LIGHT_CH3_INVERSE           0
    #define LIGHT_CH4_INVERSE           0
    #define LIGHT_CH5_INVERSE           0

#elif defined(HUACANXING_H802)

    #define ESPURNA_IMAGE               ESPURNA_LIGHT

    // Info
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER
    #define DEBUG_PORT                  Serial1
    #define SERIAL_RX_ENABLED           1
    #define LIGHT_CHANNELS              4
    #define LIGHT_CH1_PIN               12      // RED
    #define LIGHT_CH2_PIN               14      // GREEN
    #define LIGHT_CH3_PIN               13      // BLUE
    #define LIGHT_CH4_PIN               15      // WHITE
    #define LIGHT_CH1_INVERSE           0
    #define LIGHT_CH2_INVERSE           0
    #define LIGHT_CH3_INVERSE           0
    #define LIGHT_CH4_INVERSE           0

// -----------------------------------------------------------------------------
// Jan Goedeke Wifi Relay
// https://github.com/JanGoe/esp8266-wifi-relay
// -----------------------------------------------------------------------------

#elif defined(JANGOE_WIFI_RELAY_NC)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

#elif defined(JANGOE_WIFI_RELAY_NO)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// Jorge García Wifi+Relays Board Kit
// https://www.tindie.com/products/jorgegarciadev/wifi--relays-board-kit
// https://github.com/jorgegarciadev/wifikit
// -----------------------------------------------------------------------------

#elif defined(JORGEGARCIA_WIFI_RELAYS)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// WiFi MQTT Relay / Thermostat
// -----------------------------------------------------------------------------

#elif defined(OPENENERGYMONITOR_MQTT_RELAY)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// WiOn 50055 Indoor Wi-Fi Wall Outlet & Tap
// https://rover.ebay.com/rover/1/711-53200-19255-0/1?icep_id=114&ipn=icep&toolid=20004&campid=5338044841&mpre=http%3A%2F%2Fwww.ebay.com%2Fitm%2FWiOn-50050-Indoor-Wi-Fi-Outlet-Wireless-Switch-Programmable-Timer-%2F263112281551
// https://rover.ebay.com/rover/1/711-53200-19255-0/1?icep_id=114&ipn=icep&toolid=20004&campid=5338044841&mpre=http%3A%2F%2Fwww.ebay.com%2Fitm%2FWiOn-50055-Indoor-Wi-Fi-Wall-Tap-Monitor-Energy-Usage-Wireless-Smart-Switch-%2F263020837777
// -----------------------------------------------------------------------------

#elif defined(WION_50055)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// EX-Store Wifi Relay v3.1
// https://ex-store.de/ESP8266-WiFi-Relay-V31
// -----------------------------------------------------------------------------

#elif defined(EXS_WIFI_RELAY_V31)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// V9261F
// -----------------------------------------------------------------------------

#elif defined(GENERIC_V9261F)

    #define ESPURNA_IMAGE               ESPURNA_POWER

    // Info
    #define ALEXA_SUPPORT               0

    // V9261F
    #define V9261F_SUPPORT              1
    #define V9261F_PIN                  2
    #define V9261F_PIN_INVERSE          1

// -----------------------------------------------------------------------------
// ECH1560
// -----------------------------------------------------------------------------

#elif defined(GENERIC_ECH1560)

    #define ESPURNA_IMAGE               ESPURNA_POWER

    // Info
    #define ALEXA_SUPPORT               0

    // ECH1560
    #define ECH1560_SUPPORT             1
    #define ECH1560_CLK_PIN             4
    #define ECH1560_MISO_PIN            5
    #define ECH1560_INVERTED            0

// -----------------------------------------------------------------------------
// ESPLive
// https://github.com/ManCaveMade/ESP-Live
// -----------------------------------------------------------------------------

#elif defined(MANCAVEMADE_ESPLIVE)

    #define ESPURNA_IMAGE               ESPURNA_SENSOR

    // DS18B20
    #ifndef DALLAS_SUPPORT
    #define DALLAS_SUPPORT             	1
    #endif
    #define DALLAS_PIN                 	2
    #define DALLAS_UPDATE_INTERVAL     	5000
    #define TEMPERATURE_MIN_CHANGE      1.0

// -----------------------------------------------------------------------------
// QuinLED
// http://blog.quindorian.org/2017/02/esp8266-led-lighting-quinled-v2-6-pcb.html
// -----------------------------------------------------------------------------

#elif defined(INTERMITTECH_QUINLED)

    #define ESPURNA_IMAGE               ESPURNA_LIGHT

    // Info
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER
    #define LIGHT_CHANNELS              2
    #define LIGHT_CH1_PIN               0
    #define LIGHT_CH2_PIN               2
    #define LIGHT_CH1_INVERSE           0
    #define LIGHT_CH2_INVERSE           0

// -----------------------------------------------------------------------------
// Arilux AL-LC06
// -----------------------------------------------------------------------------

#elif defined(ARILUX_AL_LC01)

    #define ESPURNA_IMAGE               ESPURNA_LIGHT

    // Info
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER
    #define LIGHT_CHANNELS              3
    #define LIGHT_CH1_PIN               5       // RED
    #define LIGHT_CH2_PIN               12      // GREEN
    #define LIGHT_CH3_PIN               13      // BLUE
    #define LIGHT_CH1_INVERSE           0
    #define LIGHT_CH2_INVERSE           0
    #define LIGHT_CH3_INVERSE           0

#elif defined(ARILUX_AL_LC02)

    #define ESPURNA_IMAGE               ESPURNA_LIGHT

    // Info
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER
    #define LIGHT_CHANNELS              4
    #define LIGHT_CH1_PIN               12      // RED
    #define LIGHT_CH2_PIN               5       // GREEN
    #define LIGHT_CH3_PIN               13      // BLUE
    #define LIGHT_CH4_PIN               15      // WHITE1
    #define LIGHT_CH1_INVERSE           0
    #define LIGHT_CH2_INVERSE           0
    #define LIGHT_CH3_INVERSE           0
    #define LIGHT_CH4_INVERSE           0

#elif defined(ARILUX_AL_LC06)

    #define ESPURNA_IMAGE               ESPURNA_LIGHT

    // Info
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER
    #define LIGHT_CHANNELS              5
    #define LIGHT_CH1_PIN               14      // RED
    #define LIGHT_CH2_PIN               12      // GREEN
    #define LIGHT_CH3_PIN               13      // BLUE
    #define LIGHT_CH4_PIN               15      // WHITE1
    #define LIGHT_CH5_PIN               5       // WHITE2
    #define LIGHT_CH1_INVERSE           0
    #define LIGHT_CH2_INVERSE           0
    #define LIGHT_CH3_INVERSE           0
    #define LIGHT_CH4_INVERSE           0
    #define LIGHT_CH5_INVERSE           0

#elif defined(ARILUX_AL_LC11)

    #define ESPURNA_IMAGE               ESPURNA_LIGHT

    // Info
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER
    #define LIGHT_CHANNELS              5
    #define LIGHT_CH1_PIN               5       // RED
    #define LIGHT_CH2_PIN               4       // GREEN
    #define LIGHT_CH3_PIN               14      // BLUE
    #define LIGHT_CH4_PIN               13      // WHITE1
    #define LIGHT_CH5_PIN               12      // WHITE1
    #define LIGHT_CH1_INVERSE           0
    #define LIGHT_CH2_INVERSE           0
    #define LIGHT_CH3_INVERSE           0
    #define LIGHT_CH4_INVERSE           0
    #define LIGHT_CH5_INVERSE           0

#elif defined(ARILUX_E27)

    #define ESPURNA_IMAGE               ESPURNA_LIGHT

    // Info
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_MY92XX
    #define LIGHT_CHANNELS              4
    #define MY92XX_MODEL                MY92XX_MODEL_MY9291
    #define MY92XX_CHIPS                1
    #define MY92XX_DI_PIN               13
    #define MY92XX_DCKI_PIN             15
    #define MY92XX_COMMAND              MY92XX_COMMAND_DEFAULT
    #define MY92XX_MAPPING              0, 1, 2, 3

// -----------------------------------------------------------------------------
// XENON SM-PW701U
// -----------------------------------------------------------------------------

#elif defined(XENON_SM_PW702U)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// AUTHOMETION LYT8266
// https://authometion.com/shop/en/home/13-lyt8266.html
// -----------------------------------------------------------------------------

#elif defined(AUTHOMETION_LYT8266)

    #define ESPURNA_IMAGE               ESPURNA_LIGHT

    // Info
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER
    #define LIGHT_CHANNELS              4
    #define LIGHT_CH1_PIN               13      // RED
    #define LIGHT_CH2_PIN               12      // GREEN
    #define LIGHT_CH3_PIN               14      // BLUE
    #define LIGHT_CH4_PIN               2       // WHITE
    #define LIGHT_CH1_INVERSE           0
    #define LIGHT_CH2_INVERSE           0
    #define LIGHT_CH3_INVERSE           0
    #define LIGHT_CH4_INVERSE           0
    #define LIGHT_ENABLE_PIN            15

#elif defined(GIZWITS_WITTY_CLOUD)

    #define ESPURNA_IMAGE               ESPURNA_LIGHT

    // Info
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER
    #define LIGHT_CHANNELS              3
    #define LIGHT_CH1_PIN               15       // RED
    #define LIGHT_CH2_PIN               12       // GREEN
    #define LIGHT_CH3_PIN               13      // BLUE
    #define LIGHT_CH1_INVERSE           0
    #define LIGHT_CH2_INVERSE           0
    #define LIGHT_CH3_INVERSE           0

    #define ANALOG_SUPPORT              1   // TODO: specific or generic?

// -----------------------------------------------------------------------------
// KMC 70011
// https://www.amazon.com/KMC-Monitoring-Required-Control-Compatible/dp/B07313TH7B
// -----------------------------------------------------------------------------

#elif defined(KMC_70011)

    #define ESPURNA_IMAGE               ESPURNA_POWER

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             5
    #define HLW8012_CF_PIN              4
    #define HLW8012_VOLTAGE_R_UP        ( 2 * 1000000 )  // Upstream voltage resistor

// -----------------------------------------------------------------------------
// Euromate (?) Wifi Stecker Shuko
// https://www.obi.de/hausfunksteuerung/wifi-stecker-schuko/p/2291706
// Thanks to @Geitde
// -----------------------------------------------------------------------------

#elif defined(EUROMATE_WIFI_STECKER_SCHUKO)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// Generic 8CH
// -----------------------------------------------------------------------------

#elif defined(GENERIC_8CH)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// STM RELAY
// -----------------------------------------------------------------------------

#elif defined(STM_RELAY)

    #define ESPURNA_IMAGE               ESPURNA_STM

    #define RELAY_PROVIDER              RELAY_PROVIDER_STM
    #define DEBUG_SERIAL_SUPPORT        0

// -----------------------------------------------------------------------------
// Tonbux Powerstrip02
// -----------------------------------------------------------------------------

#elif defined(TONBUX_POWERSTRIP02)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// Lingan SWA1
// -----------------------------------------------------------------------------

#elif defined(LINGAN_SWA1)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// HEYGO HY02
// -----------------------------------------------------------------------------

#elif defined(HEYGO_HY02)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// Maxcio W-US002S
// -----------------------------------------------------------------------------

#elif defined(MAXCIO_WUS002S)

    #define ESPURNA_IMAGE               ESPURNA_POWER

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             5
    #define HLW8012_CF_PIN		        4
    #define HLW8012_CURRENT_R           0.002            // Current resistor
    #define HLW8012_VOLTAGE_R_UP        ( 2 * 1000000 )  // Upstream voltage resistor

// -----------------------------------------------------------------------------
// YiDian XS-SSA05
// -----------------------------------------------------------------------------

#elif defined(YIDIAN_XSSSA05)

    #define ESPURNA_IMAGE               ESPURNA_POWER

    // HLW8012
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             3
    #define HLW8012_CF1_PIN             14
    #define HLW8012_CF_PIN              5
    #define HLW8012_CURRENT_R           0.001            // Current resistor
    #define HLW8012_VOLTAGE_R_UP        ( 2 * 1200000 )  // Upstream voltage resistor

// -----------------------------------------------------------------------------
// TONBUX XS-SSA06
// -----------------------------------------------------------------------------

#elif defined(TONBUX_XSSSA06)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// GREEN ESP8266 RELAY MODULE
// https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180323113846&SearchText=Green+ESP8266
// -----------------------------------------------------------------------------

#elif defined(GREEN_ESP8266RELAY)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// Henrique Gravina ESPIKE
// https://github.com/Henriquegravina/Espike
// -----------------------------------------------------------------------------

#elif defined(IKE_ESPIKE)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// SWIFITCH
// https://github.com/ArnieX/swifitch
// -----------------------------------------------------------------------------

#elif defined(ARNIEX_SWIFITCH)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// ESP-01S RELAY v4.0
// https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180404024035&SearchText=esp-01s+relay
// -----------------------------------------------------------------------------

#elif defined(GENERIC_ESP01S_RELAY_V40)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// ESP-01S RGB LED v1.0 (some sold with ws2818)
// https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180404023816&SearchText=esp-01s+led+controller
// -----------------------------------------------------------------------------

#elif defined(GENERIC_ESP01S_RGBLED_V10)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// ESP-01S DHT11 v1.0
// https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180410105907&SearchText=esp-01s+dht11
// -----------------------------------------------------------------------------

#elif defined(GENERIC_ESP01S_DHT11_V10)

    #define ESPURNA_IMAGE               ESPURNA_SENSOR

    // DHT11
    #ifndef DHT_SUPPORT
    #define DHT_SUPPORT                 1
    #endif
    #define DHT_PIN                     2
    #define DHT_TYPE                    DHT_CHIP_DHT11

// -----------------------------------------------------------------------------
// ESP-01S DS18B20 v1.0
// https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180410105933&SearchText=esp-01s+ds18b20
// -----------------------------------------------------------------------------

#elif defined(GENERIC_ESP01S_DS18B20_V10)

    #define ESPURNA_IMAGE               ESPURNA_SENSOR

    // DB18B20
    #ifndef DALLAS_SUPPORT
    #define DALLAS_SUPPORT              1
    #endif
    #define DALLAS_PIN                  2

// -----------------------------------------------------------------------------
// ESP-DIN relay board V1
// https://github.com/pilotak/esp_din
// -----------------------------------------------------------------------------

#elif defined(PILOTAK_ESP_DIN_V1)

    #define ESPURNA_IMAGE               ESPURNA_SENSOR

    #define I2C_SDA_PIN                 12  // TODO:what is this for?
    #define I2C_SCL_PIN                 13

    #ifndef DALLAS_SUPPORT
    #define DALLAS_SUPPORT              1
    #endif
    #define DALLAS_PIN                  2

    #ifndef RF_SUPPORT
    #define RF_SUPPORT                  1
    #endif
    #define RF_PIN                      14

    #ifndef DIGITAL_SUPPORT
    #define DIGITAL_SUPPORT             1
    #endif
    #define DIGITAL_PIN                 16
    #define DIGITAL_PIN_MODE            INPUT

// -----------------------------------------------------------------------------
// Heltec Touch Relay
// https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20180408043114&SearchText=esp8266+touch+relay
// -----------------------------------------------------------------------------

#elif defined(HELTEC_TOUCHRELAY)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// Zhilde ZLD-EU44-W
// http://www.zhilde.com/product/60705150109-805652505/EU_WiFi_Surge_Protector_Extension_Socket_4_Outlets_works_with_Amazon_Echo_Smart_Power_Strip.html
// -----------------------------------------------------------------------------

#elif defined(ZHILDE_EU44_W)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

    // Based on the reporter, this product uses GPIO1 and 3 for the button
    // and onboard LED, so hardware serial should be disabled...
    #define DEBUG_SERIAL_SUPPORT        0

// -----------------------------------------------------------------------------
// Allnet 4duino ESP8266-UP-Relais
// http://www.allnet.de/de/allnet-brand/produkte/neuheiten/p/allnet-4duino-iot-wlan-relais-unterputz-esp8266-up-relais/
// https://shop.allnet.de/fileadmin/transfer/products/148814.pdf
// -----------------------------------------------------------------------------

#elif defined(ALLNET_4DUINO_IOT_WLAN_RELAIS)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// Luani HVIO
// https://luani.de/projekte/esp8266-hvio/
// https://luani.de/blog/esp8266-230v-io-modul/
// -----------------------------------------------------------------------------

#elif defined(LUANI_HVIO)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// Tonbux 50-100M Smart Mosquito Killer USB
// https://www.aliexpress.com/item/Original-Tonbux-50-100M-Smart-Mosquito-Killer-USB-Plug-No-Noise-Repellent-App-Smart-Module/32859330820.html
// -----------------------------------------------------------------------------

#elif defined(TONBUX_MOSQUITO_KILLER)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// NEO Coolcam NAS-WR01W Wifi Smart Power Plug
// https://es.aliexpress.com/item/-/32854589733.html?spm=a219c.12010608.0.0.6d084e68xX0y5N
// https://www.fasttech.com/product/9649426-neo-coolcam-nas-wr01w-wifi-smart-power-plug-eu
// -----------------------------------------------------------------------------

#elif defined(NEO_COOLCAM_NAS_WR01W)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// ------------------------------------------------------------------------------
// Estink Wifi Power Strip
// https://www.amazon.de/Steckdosenleiste-Ladeger%C3%A4t-Sprachsteuerung-SmartphonesTablets-Android/dp/B0796W5FZY
// Fornorm Wi-Fi USB Extension Socket (ZLD-34EU)
// https://www.aliexpress.com/item/Fornorm-WiFi-Extension-Socket-with-Surge-Protector-Smart-Power-Strip-3-Outlets-and-4-USB-Charging/32849743948.html
// -----------------------------------------------------------------------------

#elif defined(ESTINK_WIFI_POWER_STRIP)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

    // Disable UART noise since this board uses GPIO3
    #define DEBUG_SERIAL_SUPPORT        0

// -----------------------------------------------------------------------------
// Bruno Horta's OnOfre
// https://www.bhonofre.pt/
// https://github.com/brunohorta82/BH_OnOfre/
// -----------------------------------------------------------------------------

#elif defined(BH_ONOFRE)

    #define ESPURNA_IMAGE               ESPURNA_BASIC

// -----------------------------------------------------------------------------
// Several boards under different names uing a power chip labelled BL0937 or HJL-01
// * Blitzwolf (https://www.amazon.es/Inteligente-Temporización-Dispositivos-Cualquier-BlitzWolf/dp/B07BMQP142)
// * HomeCube (https://www.amazon.de/Steckdose-Homecube-intelligente-Verbrauchsanzeige-funktioniert/dp/B076Q2LKHG)
// * Coosa (https://www.amazon.com/COOSA-Monitoring-Function-Campatible-Assiatant/dp/B0788W9TDR)
// * Goosund (http://www.gosund.com/?m=content&c=index&a=show&catid=6&id=5)
// * Ablue (https://www.amazon.de/Intelligente-Steckdose-Ablue-Funktioniert-Assistant/dp/B076DRFRZC)
// -----------------------------------------------------------------------------

#elif defined(BLITZWOLF_BWSHP2)

    #define ESPURNA_IMAGE               ESPURNA_POWER

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             12
    #define HLW8012_CF1_PIN             14
    #define HLW8012_CF_PIN              5
    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

// ----------------------------------------------------------------------------------------
//  Homecube 16A is similar but some pins differ and it also has RGB LEDs
//  https://www.amazon.de/gp/product/B07D7RVF56/ref=oh_aui_detailpage_o00_s01?ie=UTF8&psc=1
// ----------------------------------------------------------------------------------------
#elif defined(HOMECUBE_16A)

    #define ESPURNA_IMAGE               ESPURNA_POWER

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             16
    #define HLW8012_CF1_PIN             14
    #define HLW8012_CF_PIN              5
    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING


// -----------------------------------------------------------------------------
// VANZAVANZU Smart Outlet Socket (based on BL0937 or HJL-01)
// https://www.amazon.com/Smart-Plug-Wifi-Mini-VANZAVANZU/dp/B078PHD6S5
// -----------------------------------------------------------------------------

#elif defined(VANZAVANZU_SMART_WIFI_PLUG_MINI)

    #define ESPURNA_IMAGE               ESPURNA_POWER

    // Disable UART noise
    #define DEBUG_SERIAL_SUPPORT        0

    // HJL01 / BL0937
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT             1
    #endif
    #define HLW8012_SEL_PIN             3
    #define HLW8012_CF1_PIN             14
    #define HLW8012_CF_PIN              5
    #define HLW8012_SEL_CURRENT         LOW
    #define HLW8012_CURRENT_RATIO       25740
    #define HLW8012_VOLTAGE_RATIO       313400
    #define HLW8012_POWER_RATIO         3414290
    #define HLW8012_INTERRUPT_ON        FALLING

#elif defined(GENERIC_AG_L4)

    #define ESPURNA_IMAGE               ESPURNA_LIGHT

    // Info
    #define RELAY_PROVIDER              RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER              LIGHT_PROVIDER_DIMMER
    #define LIGHT_CHANNELS              3
    #define LIGHT_CH1_PIN               14       // RED
    #define LIGHT_CH2_PIN               13       // GREEN
    #define LIGHT_CH3_PIN               12      // BLUE
    #define LIGHT_CH1_INVERSE           0
    #define LIGHT_CH2_INVERSE           0
    #define LIGHT_CH3_INVERSE           0
// -----------------------------------------------------------------------------
// Generic board with Geiger Counter
// -----------------------------------------------------------------------------

#elif defined(GENERIC_GEIGER_COUNTER)

    #define ESPURNA_IMAGE               ESPURNA_GEIGER

    // Enable Geiger Counter
    #define GEIGER_SUPPORT              1

    // Disable uneeded modules
    #define ALEXA_SUPPORT               0
    #define SCHEDULER_SUPPORT           0

// -----------------------------------------------------------------------------
// TEST boards (do not use!!)
// -----------------------------------------------------------------------------

#elif defined(TRAVIS01)

    // A bit of I2C - pins 3,4
    #define I2C_SDA_PIN         3
    #define I2C_SCL_PIN         4

    // And, as they say in "From Dusk till Dawn":
    // This is a sensor blow out!
    // Alright, we got white sensor, black sensor, spanish sensor, yellow sensor. We got hot sensor, cold sensor.
    // We got wet sensor. We got smelly sensor. We got hairy sensor, bloody sensor. We got snapping sensor.
    // We got silk sensor, velvet sensor, naugahyde sensor. We even got horse sensor, dog sensor, chicken sensor.
    // C'mon, you want sensor, come on in sensor lovers!
    // If we don’t got it, you don't want it!
    #define AM2320_SUPPORT        1
    #define BH1750_SUPPORT        1
    #define BMX280_SUPPORT        1
    #define SHT3X_I2C_SUPPORT     1
    #define EMON_ADC121_SUPPORT   1
    #define EMON_ADS1X15_SUPPORT  1
    #define SHT3X_I2C_SUPPORT     1
    #define SI7021_SUPPORT        1
    #define PMSX003_SUPPORT       1
    #define SENSEAIR_SUPPORT      1


    // A bit of lights - pin 5
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_DIMMER
    #define DUMMY_RELAY_COUNT   1
    #define LIGHT_CHANNELS      1
    #define LIGHT_CH1_PIN       5
    #define LIGHT_CH1_INVERSE   0

    // A bit of HLW8012 - pins 6,7,8
    #ifndef HLW8012_SUPPORT
    #define HLW8012_SUPPORT     1
    #endif
    #define HLW8012_SEL_PIN     6
    #define HLW8012_CF1_PIN     7
    #define HLW8012_CF_PIN      8

    // A bit of Dallas - pin 9
    #ifndef DALLAS_SUPPORT
    #define DALLAS_SUPPORT      1
    #endif
    #define DALLAS_PIN          9

    // A bit of ECH1560 - pins 10,11, 12
    #ifndef ECH1560_SUPPORT
    #define ECH1560_SUPPORT     1
    #endif
    #define ECH1560_CLK_PIN     10
    #define ECH1560_MISO_PIN    11
    #define ECH1560_INVERTED    12

#elif defined(TRAVIS02)

    // A bit of CSE7766 - pin 1
    #ifndef CSE7766_SUPPORT
    #define CSE7766_SUPPORT     1
    #endif
    #define CSE7766_PIN         1

    // Relay type dual  - pins 2,3
    #define RELAY_PROVIDER      RELAY_PROVIDER_DUAL
    #define RELAY1_PIN          2
    #define RELAY2_PIN          3
    #define RELAY1_TYPE         RELAY_TYPE_NORMAL
    #define RELAY2_TYPE         RELAY_TYPE_NORMAL

    // IR - pin 4
    #define IR_SUPPORT          1
    #define IR_RECEIVER_PIN     4
    #define IR_BUTTON_SET       1

    // A bit of DHT - pin 5
    #ifndef DHT_SUPPORT
    #define DHT_SUPPORT         1
    #endif
    #define DHT_PIN             5

    // A bit of TMP3X (analog)
    #define TMP3X_SUPPORT       1

    // A bit of EVENTS - pin 10
    #define EVENTS_SUPPORT      1
    #define EVENTS_PIN          6

    // Sonar
    #define SONAR_SUPPORT       1
    #define SONAR_TRIGGER       7
    #define SONAR_ECHO          8

    // MHZ19
    #define MHZ19_SUPPORT       1
    #define MHZ19_RX_PIN        9
    #define MHZ19_TX_PIN        10

    // PZEM004T
    #define PZEM004T_SUPPORT    1
    #define PZEM004T_RX_PIN     11
    #define PZEM004T_TX_PIN     12

    // V9261F
    #define V9261F_SUPPORT      1
    #define V9261F_PIN          13

    // GUVAS12SD
    #define GUVAS12SD_SUPPORT   1
    #define GUVAS12SD_PIN       14

    // Test non-default modules
    #define MDNS_CLIENT_SUPPORT 1
    #define NOFUSS_SUPPORT      1
    #define UART_MQTT_SUPPORT   1
    #define INFLUXDB_SUPPORT    1
    #define IR_SUPPORT          1

#elif defined(TRAVIS03)

    // MY9231 Light - pins 1,2
    #define RELAY_PROVIDER      RELAY_PROVIDER_LIGHT
    #define LIGHT_PROVIDER      LIGHT_PROVIDER_MY92XX
    #define DUMMY_RELAY_COUNT   1
    #define LIGHT_CHANNELS      5
    #define MY92XX_MODEL        MY92XX_MODEL_MY9231
    #define MY92XX_CHIPS        2
    #define MY92XX_DI_PIN       1
    #define MY92XX_DCKI_PIN     2
    #define MY92XX_COMMAND      MY92XX_COMMAND_DEFAULT
    #define MY92XX_MAPPING      4, 3, 5, 0, 1

    // A bit of Analog EMON (analog)
    #ifndef EMON_ANALOG_SUPPORT
    #define EMON_ANALOG_SUPPORT 1
    #endif

    // Test non-default modules
    #define LLMNR_SUPPORT       1
    #define NETBIOS_SUPPORT     1
    #define SSDP_SUPPORT        1

// -----------------------------------------------------------------------------
// ESPurna Core
// -----------------------------------------------------------------------------

#else

    // This is a special device targeted to generate a light-weight binary image
    // meant to be able to do two-step-updates:
    // https://github.com/xoseperez/espurna/wiki/TwoStepUpdates

    #define ESPURNA_IMAGE           ESPURNA_CORE

    // Disable non-core modules
    #define ALEXA_SUPPORT           0
    #define BROKER_SUPPORT          0
    #define BUTTON_SUPPORT          0
    #define DOMOTICZ_SUPPORT        0
    #define HOMEASSISTANT_SUPPORT   0
    #define I2C_SUPPORT             0
    #define MDNS_SERVER_SUPPORT     0
    #define MQTT_SUPPORT            0
    #define NTP_SUPPORT             0
    #define SCHEDULER_SUPPORT       0
    #define SENSOR_SUPPORT          0
    #define THINGSPEAK_SUPPORT      0
    #define WEB_SUPPORT             0

    // Extra light-weight image
    //#define DEBUG_SERIAL_SUPPORT    0
    //#define DEBUG_TELNET_SUPPORT    0
    //#define DEBUG_WEB_SUPPORT       0
    //#define LED_SUPPORT             0
    //#define TELNET_SUPPORT          0
    //#define TERMINAL_SUPPORT        0

#endif
