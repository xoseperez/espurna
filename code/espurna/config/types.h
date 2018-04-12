//------------------------------------------------------------------------------
// Type definitions
// Do not touch this definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// BUTTONS
//------------------------------------------------------------------------------

#define BUTTON_EVENT_NONE           0
#define BUTTON_EVENT_PRESSED        1
#define BUTTON_EVENT_RELEASED       2
#define BUTTON_EVENT_CLICK          2
#define BUTTON_EVENT_DBLCLICK       3
#define BUTTON_EVENT_LNGCLICK       4
#define BUTTON_EVENT_LNGLNGCLICK    5

#define BUTTON_MODE_NONE            0
#define BUTTON_MODE_TOGGLE          1
#define BUTTON_MODE_ON              2
#define BUTTON_MODE_OFF             3
#define BUTTON_MODE_AP              4
#define BUTTON_MODE_RESET           5
#define BUTTON_MODE_PULSE           6
#define BUTTON_MODE_FACTORY         7

// Needed for ESP8285 boards under Windows using PlatformIO (?)
#ifndef BUTTON_PUSHBUTTON
#define BUTTON_PUSHBUTTON           0
#define BUTTON_SWITCH               1
#define BUTTON_DEFAULT_HIGH         2
#define BUTTON_SET_PULLUP           4
#endif

//------------------------------------------------------------------------------
// RELAY
//------------------------------------------------------------------------------

#define RELAY_BOOT_OFF              0
#define RELAY_BOOT_ON               1
#define RELAY_BOOT_SAME             2
#define RELAY_BOOT_TOGGLE           3

#define RELAY_TYPE_NORMAL           0
#define RELAY_TYPE_INVERSE          1
#define RELAY_TYPE_LATCHED          2
#define RELAY_TYPE_LATCHED_INVERSE  3

#define RELAY_SYNC_ANY              0
#define RELAY_SYNC_NONE_OR_ONE      1
#define RELAY_SYNC_ONE              2
#define RELAY_SYNC_SAME             3

#define RELAY_PULSE_NONE            0
#define RELAY_PULSE_OFF             1
#define RELAY_PULSE_ON              2

#define RELAY_PROVIDER_RELAY        0
#define RELAY_PROVIDER_DUAL         1
#define RELAY_PROVIDER_LIGHT        2
#define RELAY_PROVIDER_RFBRIDGE     3
#define RELAY_PROVIDER_STM          4

//------------------------------------------------------------------------------
// MQTT
//------------------------------------------------------------------------------

// Internal MQTT events
#define MQTT_CONNECT_EVENT          0
#define MQTT_DISCONNECT_EVENT       1
#define MQTT_MESSAGE_EVENT          2

//------------------------------------------------------------------------------
// LED
//------------------------------------------------------------------------------

#define LED_MODE_MQTT               0       // LED will be managed from MQTT (OFF by default)
#define LED_MODE_WIFI               1       // LED will blink according to the WIFI status
#define LED_MODE_FOLLOW             2       // LED will follow state of linked relay (check RELAY#_LED)
#define LED_MODE_FOLLOW_INVERSE     3       // LED will follow the opposite state of linked relay (check RELAY#_LED)
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

// -----------------------------------------------------------------------------
// SCHEDULER
// -----------------------------------------------------------------------------

#define SCHEDULER_TYPE_SWITCH       1
#define SCHEDULER_TYPE_DIM          2

// -----------------------------------------------------------------------------
// IR
// -----------------------------------------------------------------------------

// IR Button modes
#define IR_BUTTON_MODE_NONE         0
#define IR_BUTTON_MODE_RGB          1
#define IR_BUTTON_MODE_HSV          2
#define IR_BUTTON_MODE_BRIGHTER     3
#define IR_BUTTON_MODE_STATE        4
#define IR_BUTTON_MODE_EFFECT       5

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

#define POWER_WATTS                 0
#define POWER_KILOWATTS             1

#define ENERGY_JOULES               0
#define ENERGY_KWH                  1

#define TMP_CELSIUS                 0
#define TMP_FAHRENHEIT              1
#define TMP_KELVIN                  2

//--------------------------------------------------------------------------------
// Sensor ID
// These should remain over time, do not modify them, only add new ones at the end
//--------------------------------------------------------------------------------

#define SENSOR_DHTXX_ID             0x01
#define SENSOR_DALLAS_ID            0x02
#define SENSOR_EMON_ANALOG_ID       0x03
#define SENSOR_EMON_ADC121_ID       0x04
#define SENSOR_EMON_ADS1X15_ID      0x05
#define SENSOR_HLW8012_ID           0x06
#define SENSOR_V9261F_ID            0x07
#define SENSOR_ECH1560_ID           0x08
#define SENSOR_ANALOG_ID            0x09
#define SENSOR_DIGITAL_ID           0x10
#define SENSOR_EVENTS_ID            0x11
#define SENSOR_PMSX003_ID           0x12
#define SENSOR_BMX280_ID            0x13
#define SENSOR_MHZ19_ID             0x14
#define SENSOR_SI7021_ID            0x15
#define SENSOR_SHT3X_I2C_ID         0x16
#define SENSOR_BH1750_ID            0x17
#define SENSOR_PZEM004T_ID          0x18
#define SENSOR_AM2320_ID            0x19
#define SENSOR_GUVAS12SD_ID         0x20
#define SENSOR_CSE7766_ID           0x21
#define SENSOR_TMP3X_ID             0x22
#define SENSOR_HCSR04_ID            0x23

//--------------------------------------------------------------------------------
// Magnitudes
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
#define MAGNITUDE_EVENTS            14
#define MAGNITUDE_PM1dot0           15
#define MAGNITUDE_PM2dot5           16
#define MAGNITUDE_PM10              17
#define MAGNITUDE_CO2               18
#define MAGNITUDE_LUX               19
#define MAGNITUDE_UV                20
#define MAGNITUDE_DISTANCE          21

#define MAGNITUDE_MAX               22
