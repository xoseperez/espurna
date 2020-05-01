//------------------------------------------------------------------------------
// Type definitions
// Do not touch this definitions
//------------------------------------------------------------------------------

#pragma once

// -----------------------------------------------------------------------------
// WIFI
// -----------------------------------------------------------------------------

#define WIFI_STATE_AP               1
#define WIFI_STATE_STA              2
#define WIFI_STATE_AP_STA           3
#define WIFI_STATE_WPS              4
#define WIFI_STATE_SMARTCONFIG      8

#define WIFI_AP_ALLWAYS             1
#define WIFI_AP_FALLBACK            2

//------------------------------------------------------------------------------
// BUTTONS
//------------------------------------------------------------------------------

// button actions, limited to 4-bit number (0b1111 / 0xf / 15)
#define BUTTON_ACTION_NONE            0u
#define BUTTON_ACTION_TOGGLE          1u
#define BUTTON_ACTION_ON              2u
#define BUTTON_ACTION_OFF             3u
#define BUTTON_ACTION_AP              4u
#define BUTTON_ACTION_RESET           5u
#define BUTTON_ACTION_PULSE           6u
#define BUTTON_ACTION_FACTORY         7u
#define BUTTON_ACTION_WPS             8u
#define BUTTON_ACTION_SMART_CONFIG    9u
#define BUTTON_ACTION_DIM_UP          10u
#define BUTTON_ACTION_DIM_DOWN        11u
#define BUTTON_ACTION_DISPLAY_ON      12u
#define BUTTON_ACTION_MAX             15u

// Deprecated: legacy mapping, changed to action from above
#define BUTTON_MODE_NONE              BUTTON_ACTION_NONE
#define BUTTON_MODE_TOGGLE            BUTTON_ACTION_TOGGLE
#define BUTTON_MODE_ON                BUTTON_ACTION_ON
#define BUTTON_MODE_OFF               BUTTON_ACTION_OFF
#define BUTTON_MODE_AP                BUTTON_ACTION_AP
#define BUTTON_MODE_RESET             BUTTON_ACTION_RESET
#define BUTTON_MODE_PULSE             BUTTON_ACTION_PULSE
#define BUTTON_MODE_FACTORY           BUTTON_ACTION_FACTORY
#define BUTTON_MODE_WPS               BUTTON_ACTION_WPS
#define BUTTON_MODE_SMART_CONFIG      BUTTON_ACTION_SMART_CONFIG
#define BUTTON_MODE_DIM_UP            BUTTON_ACTION_DIM_UP
#define BUTTON_MODE_DIM_DOWN          BUTTON_ACTION_DIM_DOWN
#define BUTTON_MODE_DISPLAY_ON        BUTTON_ACTION_DISPLAY_ON

// compat definitions for DebounceEvent
#define BUTTON_PUSHBUTTON           ButtonMask::Pushbutton
#define BUTTON_SWITCH               ButtonMask::Switch
#define BUTTON_DEFAULT_HIGH         ButtonMask::DefaultHigh
#define BUTTON_SET_PULLUP           ButtonMask::SetPullup
#define BUTTON_SET_PULLDOWN         ButtonMask::SetPulldown

// configure which type of event emitter is used
#define BUTTON_EVENTS_SOURCE_GENERIC               0
#define BUTTON_EVENTS_SOURCE_ITEAD_SONOFF_DUAL     1
#define BUTTON_EVENTS_SOURCE_FOXEL_LIGHTFOX_DUAL   2

//------------------------------------------------------------------------------
// ENCODER
//------------------------------------------------------------------------------

#define ENCODER_MODE_CHANNEL        0
#define ENCODER_MODE_RATIO          1

//------------------------------------------------------------------------------
// RELAY
//------------------------------------------------------------------------------

#define RELAY_BOOT_OFF              0
#define RELAY_BOOT_ON               1
#define RELAY_BOOT_SAME             2
#define RELAY_BOOT_TOGGLE           3
#define RELAY_BOOT_LOCKED_OFF       4
#define RELAY_BOOT_LOCKED_ON        5

#define RELAY_TYPE_NORMAL           0
#define RELAY_TYPE_INVERSE          1
#define RELAY_TYPE_LATCHED          2
#define RELAY_TYPE_LATCHED_INVERSE  3

#define RELAY_SYNC_ANY              0
#define RELAY_SYNC_NONE_OR_ONE      1
#define RELAY_SYNC_ONE              2
#define RELAY_SYNC_SAME             3
#define RELAY_SYNC_FIRST            4

#define RELAY_PULSE_NONE            0
#define RELAY_PULSE_OFF             1
#define RELAY_PULSE_ON              2

#define RELAY_PROVIDER_RELAY        0
#define RELAY_PROVIDER_DUAL         1
#define RELAY_PROVIDER_LIGHT        2
#define RELAY_PROVIDER_RFBRIDGE     3
#define RELAY_PROVIDER_STM          4

#define RELAY_GROUP_SYNC_NORMAL      0
#define RELAY_GROUP_SYNC_INVERSE     1
#define RELAY_GROUP_SYNC_RECEIVEONLY 2

#define RELAY_LOCK_DISABLED          0
#define RELAY_LOCK_OFF               1
#define RELAY_LOCK_ON                2

//------------------------------------------------------------------------------
// UDP SYSLOG
//------------------------------------------------------------------------------

// Priority codes:
#define SYSLOG_EMERG       0 /* system is unusable */
#define SYSLOG_ALERT       1 /* action must be taken immediately */
#define SYSLOG_CRIT        2 /* critical conditions */
#define SYSLOG_ERR         3 /* error conditions */
#define SYSLOG_WARNING     4 /* warning conditions */
#define SYSLOG_NOTICE      5 /* normal but significant condition */
#define SYSLOG_INFO        6 /* informational */
#define SYSLOG_DEBUG       7 /* debug-level messages */

// Facility codes:
#define SYSLOG_KERN        (0<<3)  /* kernel messages */
#define SYSLOG_USER        (1<<3)  /* random user-level messages */
#define SYSLOG_MAIL        (2<<3)  /* mail system */
#define SYSLOG_DAEMON      (3<<3)  /* system daemons */
#define SYSLOG_AUTH        (4<<3)  /* security/authorization messages */
#define SYSLOG_SYSLOG      (5<<3)  /* messages generated internally by syslogd */
#define SYSLOG_LPR         (6<<3)  /* line printer subsystem */
#define SYSLOG_NEWS        (7<<3)  /* network news subsystem */
#define SYSLOG_UUCP        (8<<3)  /* UUCP subsystem */
#define SYSLOG_CRON        (9<<3)  /* clock daemon */
#define SYSLOG_AUTHPRIV    (10<<3) /* security/authorization messages (private) */
#define SYSLOG_FTP         (11<<3) /* ftp daemon */
#define SYSLOG_LOCAL0      (16<<3) /* reserved for local use */
#define SYSLOG_LOCAL1      (17<<3) /* reserved for local use */
#define SYSLOG_LOCAL2      (18<<3) /* reserved for local use */
#define SYSLOG_LOCAL3      (19<<3) /* reserved for local use */
#define SYSLOG_LOCAL4      (20<<3) /* reserved for local use */
#define SYSLOG_LOCAL5      (21<<3) /* reserved for local use */
#define SYSLOG_LOCAL6      (22<<3) /* reserved for local use */
#define SYSLOG_LOCAL7      (23<<3) /* reserved for local use */

//------------------------------------------------------------------------------
// MQTT
//------------------------------------------------------------------------------

// Internal MQTT events
#define MQTT_CONNECT_EVENT          0
#define MQTT_DISCONNECT_EVENT       1
#define MQTT_MESSAGE_EVENT          2

// MQTT_LIBRARY
#define MQTT_LIBRARY_ASYNCMQTTCLIENT        0
#define MQTT_LIBRARY_ARDUINOMQTT            1
#define MQTT_LIBRARY_PUBSUBCLIENT           2


//------------------------------------------------------------------------------
// LED
//------------------------------------------------------------------------------

#define LED_MODE_MANUAL             0       // LED will be managed manually (OFF by default)
#define LED_MODE_WIFI               1       // LED will blink according to the WIFI status
#define LED_MODE_FOLLOW             2       // LED will follow state of linked LED#_RELAY relay ID
#define LED_MODE_FOLLOW_INVERSE     3       // LED will follow the opposite state of linked LED#_RELAY relay ID
#define LED_MODE_FINDME             4       // LED will be ON if all relays are OFF
#define LED_MODE_FINDME_WIFI        5       // A mixture between WIFI and FINDME
#define LED_MODE_ON                 6       // LED always ON
#define LED_MODE_OFF                7       // LED always OFF
#define LED_MODE_RELAY              8       // If any relay is ON, LED will be ON, otherwise OFF
#define LED_MODE_RELAY_WIFI         9       // A mixture between WIFI and RELAY, the reverse of MIXED

// -----------------------------------------------------------------------------
// UI
// -----------------------------------------------------------------------------

#define UI_TAG_INPUT                0
#define UI_TAG_CHECKBOX             1
#define UI_TAG_SELECT               2

#define WEB_MODE_NORMAL             0
#define WEB_MODE_PASSWORD           1

// -----------------------------------------------------------------------------
// LIGHT
// -----------------------------------------------------------------------------

// Available light providers
#define LIGHT_PROVIDER_NONE         0
#define LIGHT_PROVIDER_MY92XX       1       // works with MY9291 and MY9231
#define LIGHT_PROVIDER_DIMMER       2
#define LIGHT_PROVIDER_TUYA         3

// -----------------------------------------------------------------------------
// SCHEDULER
// -----------------------------------------------------------------------------

#define SCHEDULER_TYPE_SWITCH       1
#define SCHEDULER_TYPE_DIM          2

// -----------------------------------------------------------------------------
// IR
// -----------------------------------------------------------------------------

#define IR_BUTTON_ACTION_NONE         0
#define IR_BUTTON_ACTION_RGB          1
#define IR_BUTTON_ACTION_HSV          2
#define IR_BUTTON_ACTION_BRIGHTER     3
#define IR_BUTTON_ACTION_STATE        4
#define IR_BUTTON_ACTION_EFFECT       5
#define IR_BUTTON_ACTION_TOGGLE       6

#define IR_BUTTON_MODE_NONE         IR_BUTTON_ACTION_NONE
#define IR_BUTTON_MODE_RGB          IR_BUTTON_ACTION_RGB
#define IR_BUTTON_MODE_HSV          IR_BUTTON_ACTION_HSV
#define IR_BUTTON_MODE_BRIGHTER     IR_BUTTON_ACTION_BRIGHTER
#define IR_BUTTON_MODE_STATE        IR_BUTTON_ACTION_STATE
#define IR_BUTTON_MODE_EFFECT       IR_BUTTON_ACTION_EFFECT
#define IR_BUTTON_MODE_TOGGLE       IR_BUTTON_ACTION_TOGGLE

#define LIGHT_EFFECT_SOLID          0
#define LIGHT_EFFECT_FLASH          1
#define LIGHT_EFFECT_STROBE         2
#define LIGHT_EFFECT_FADE           3
#define LIGHT_EFFECT_SMOOTH         4

//------------------------------------------------------------------------------
// RESET
//------------------------------------------------------------------------------

#define CUSTOM_RESET_HARDWARE       1       // Reset from hardware button
#define CUSTOM_RESET_WEB            2       // Reset from web interface
#define CUSTOM_RESET_TERMINAL       3       // Reset from terminal
#define CUSTOM_RESET_MQTT           4       // Reset via MQTT
#define CUSTOM_RESET_RPC            5       // Reset via RPC (HTTP)
#define CUSTOM_RESET_OTA            6       // Reset after successful OTA update
#define CUSTOM_RESET_HTTP           7       // Reset via HTTP GET
#define CUSTOM_RESET_NOFUSS         8       // Reset after successful NOFUSS update
#define CUSTOM_RESET_UPGRADE        9       // Reset after update from web interface
#define CUSTOM_RESET_FACTORY        10      // Factory reset from terminal

#define CUSTOM_RESET_MAX            10

//------------------------------------------------------------------------------
// ENVIRONMENTAL
//------------------------------------------------------------------------------

// American Society of Heating, Refrigerating and Air-Conditioning Engineers suggests a range of 45% - 55% humidity to manage health effects and illnesses.
// Comfortable: 30% - 60%
// Recommended: 45% - 55%
// High       : 55% - 80%

#define HUMIDITY_NORMAL             0       // > %30
#define HUMIDITY_COMFORTABLE        1       // > %45
#define HUMIDITY_DRY                2       // < %30
#define HUMIDITY_WET                3       // > %70

// United States Environmental Protection Agency - UV Index Scale
// One UV Index unit is equivalent to 25 milliWatts per square meter.
#define UV_INDEX_LOW                0       // 0 to 2 means low danger from the sun's UV rays for the average person.
#define UV_INDEX_MODERATE           1       // 3 to 5 means moderate risk of harm from unprotected sun exposure.
#define UV_INDEX_HIGH               2       // 6 to 7 means high risk of harm from unprotected sun exposure. Protection against skin and eye damage is needed.
#define UV_INDEX_VERY_HIGH          3       // 8 to 10 means very high risk of harm from unprotected sun exposure.
                                            // Take extra precautions because unprotected skin and eyes will be damaged and can burn quickly.
#define UV_INDEX_EXTREME            4       // 11 or more means extreme risk of harm from unprotected sun exposure.
                                            // Take all precautions because unprotected skin and eyes can burn in minutes.

//------------------------------------------------------------------------------
// UNITS
//------------------------------------------------------------------------------

#define POWER_WATTS                 sensor::Unit::Watt
#define POWER_KILOWATTS             sensor::Unit::Kilowatt

#define ENERGY_JOULES               sensor::Unit::Joule
#define ENERGY_KWH                  sensor::Unit::KilowattHour

#define TMP_CELSIUS                 sensor::Unit::Celcius
#define TMP_FAHRENHEIT              sensor::Unit::Farenheit
#define TMP_KELVIN                  sensor::Unit::Kelvin

//--------------------------------------------------------------------------------
// Sensor ID
// These should remain over time, do not modify them, only add new ones at the end
//--------------------------------------------------------------------------------

#define SENSOR_DHTXX_ID             1
#define SENSOR_DALLAS_ID            2
#define SENSOR_EMON_ANALOG_ID       3
#define SENSOR_EMON_ADC121_ID       4
#define SENSOR_EMON_ADS1X15_ID      5
#define SENSOR_HLW8012_ID           6
#define SENSOR_V9261F_ID            7
#define SENSOR_ECH1560_ID           8
#define SENSOR_ANALOG_ID            9
#define SENSOR_DIGITAL_ID           10
#define SENSOR_EVENTS_ID            11
#define SENSOR_PMSX003_ID           12
#define SENSOR_BMX280_ID            13
#define SENSOR_MHZ19_ID             14
#define SENSOR_SI7021_ID            15
#define SENSOR_SHT3X_I2C_ID         16
#define SENSOR_BH1750_ID            17
#define SENSOR_PZEM004T_ID          18
#define SENSOR_AM2320_ID            19
#define SENSOR_GUVAS12SD_ID         20
#define SENSOR_CSE7766_ID           21
#define SENSOR_TMP3X_ID             22
#define SENSOR_SONAR_ID             23
#define SENSOR_SENSEAIR_ID          24
#define SENSOR_GEIGER_ID            25
#define SENSOR_NTC_ID               26
#define SENSOR_SDS011_ID            27
#define SENSOR_MICS2710_ID          28
#define SENSOR_MICS5525_ID          29
#define SENSOR_PULSEMETER_ID        30
#define SENSOR_VEML6075_ID          31
#define SENSOR_VL53L1X_ID           32
#define SENSOR_EZOPH_ID             33
#define SENSOR_BMP180_ID            34
#define SENSOR_MAX6675_ID           35
#define SENSOR_LDR_ID               36
#define SENSOR_ADE7953_ID           37
#define SENSOR_T6613_ID             38
#define SENSOR_SI1145_ID            39
#define SENSOR_HDC1080_ID           40

//--------------------------------------------------------------------------------
// Magnitudes
// These should remain over time, do not modify their values, only add new ones at the end
//--------------------------------------------------------------------------------

#define MAGNITUDE_NONE              0
#define MAGNITUDE_TEMPERATURE       1
#define MAGNITUDE_HUMIDITY          2
#define MAGNITUDE_PRESSURE          3
#define MAGNITUDE_CURRENT           4
#define MAGNITUDE_VOLTAGE           5
#define MAGNITUDE_POWER_ACTIVE      6
#define MAGNITUDE_POWER_APPARENT    7
#define MAGNITUDE_POWER_REACTIVE    8
#define MAGNITUDE_POWER_FACTOR      9
#define MAGNITUDE_ENERGY            10
#define MAGNITUDE_ENERGY_DELTA      11
#define MAGNITUDE_ANALOG            12
#define MAGNITUDE_DIGITAL           13
#define MAGNITUDE_EVENT             14
#define MAGNITUDE_PM1dot0           15
#define MAGNITUDE_PM2dot5           16
#define MAGNITUDE_PM10              17
#define MAGNITUDE_CO2               18
#define MAGNITUDE_LUX               19
#define MAGNITUDE_UVA               20
#define MAGNITUDE_UVB               21
#define MAGNITUDE_UVI               22
#define MAGNITUDE_DISTANCE          23
#define MAGNITUDE_HCHO              24
#define MAGNITUDE_GEIGER_CPM        25
#define MAGNITUDE_GEIGER_SIEVERT    26
#define MAGNITUDE_COUNT             27
#define MAGNITUDE_NO2               28
#define MAGNITUDE_CO                29
#define MAGNITUDE_RESISTANCE        30
#define MAGNITUDE_PH                31

#define MAGNITUDE_MAX               32

#define SENSOR_ERROR_OK             0       // No error
#define SENSOR_ERROR_OUT_OF_RANGE   1       // Result out of sensor range
#define SENSOR_ERROR_WARM_UP        2       // Sensor is warming-up
#define SENSOR_ERROR_TIMEOUT        3       // Response from sensor timed out
#define SENSOR_ERROR_UNKNOWN_ID     4       // Sensor did not report a known ID
#define SENSOR_ERROR_CRC            5       // Sensor data corrupted
#define SENSOR_ERROR_I2C            6       // Wrong or locked I2C address
#define SENSOR_ERROR_GPIO_USED      7       // The GPIO is already in use
#define SENSOR_ERROR_CALIBRATION    8       // Calibration error or Not calibrated
#define SENSOR_ERROR_OTHER          99      // Any other error
//------------------------------------------------------------------------------
// Telnet server
//------------------------------------------------------------------------------

#define TELNET_SERVER_ASYNC        0
#define TELNET_SERVER_WIFISERVER   1

//------------------------------------------------------------------------------
// OTA Client (not related to the Web OTA support)
//------------------------------------------------------------------------------

#define OTA_CLIENT_NONE             0
#define OTA_CLIENT_ASYNCTCP         1
#define OTA_CLIENT_HTTPUPDATE       2

//------------------------------------------------------------------------------
// Secure Client
//------------------------------------------------------------------------------

#define SECURE_CLIENT_NONE                0
#define SECURE_CLIENT_AXTLS               1
#define SECURE_CLIENT_BEARSSL             2

#define SECURE_CLIENT_CHECK_NONE          0 // !!! INSECURE CONNECTION !!!
#define SECURE_CLIENT_CHECK_FINGERPRINT   1 // legacy fingerprint validation
#define SECURE_CLIENT_CHECK_CA            2 // set trust anchor from PROGMEM CA certificate

// -----------------------------------------------------------------------------
// Hardware default values
// -----------------------------------------------------------------------------

#define GPIO_NONE           0x99
#define RELAY_NONE          0x99
