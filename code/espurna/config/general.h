//------------------------------------------------------------------------------
// Do not change this file unless you know what you are doing
// Configuration settings are in the settings.h file
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// GENERAL
//------------------------------------------------------------------------------

#define ADMIN_PASS              "fibonacci" // Default password (WEB, OTA, WIFI)
#define DEVICE_NAME             MANUFACTURER "_" DEVICE // Concatenate both to get a unique device name

//------------------------------------------------------------------------------
// TELNET
//------------------------------------------------------------------------------

#ifndef TELNET_SUPPORT
#define TELNET_SUPPORT          1               // Enable telnet support by default
#endif

#ifndef TELNET_STA
#define TELNET_STA              0               // By default, disallow connections via STA interface
#endif

#define TELNET_PORT             23              // Port to listen to telnet clients
#define TELNET_MAX_CLIENTS      1               // Max number of concurrent telnet clients

//------------------------------------------------------------------------------
// DEBUG
//------------------------------------------------------------------------------

// Serial debug log

#ifndef DEBUG_SERIAL_SUPPORT
#define DEBUG_SERIAL_SUPPORT    1               // Enable serial debug log
#endif
#ifndef DEBUG_PORT
#define DEBUG_PORT              Serial          // Default debugging port
#endif

//------------------------------------------------------------------------------

// UDP debug log
// To receive the message son the destination computer use nc:
// nc -ul 8113

#ifndef DEBUG_UDP_SUPPORT
#define DEBUG_UDP_SUPPORT       0               // Enable UDP debug log
#endif
#define DEBUG_UDP_IP            IPAddress(192, 168, 1, 100)
#define DEBUG_UDP_PORT          8113

//------------------------------------------------------------------------------

#ifndef DEBUG_TELNET_SUPPORT
#define DEBUG_TELNET_SUPPORT    TELNET_SUPPORT  // Enable telnet debug log if telnet is enabled too
#endif

//------------------------------------------------------------------------------

// General debug options and macros
#define DEBUG_MESSAGE_MAX_LENGTH    80
#define DEBUG_SUPPORT           DEBUG_SERIAL_SUPPORT || DEBUG_UDP_SUPPORT || DEBUG_TELNET_SUPPORT


#if DEBUG_SUPPORT
    #define DEBUG_MSG(...) debugSend(__VA_ARGS__)
    #define DEBUG_MSG_P(...) debugSend_P(__VA_ARGS__)
#endif

#ifndef DEBUG_MSG
    #define DEBUG_MSG(...)
    #define DEBUG_MSG_P(...)
#endif

//------------------------------------------------------------------------------
// TERMINAL
//------------------------------------------------------------------------------

#ifndef TERMINAL_SUPPORT
#define TERMINAL_SUPPORT         1               // Enable terminal commands
#endif

//------------------------------------------------------------------------------
// CRASH
//------------------------------------------------------------------------------

#define CRASH_SAFE_TIME         60000           // The system is considered stable after these many millis
#define CRASH_COUNT_MAX         5               // After this many crashes on boot
                                                // the system is flagged as unstable

//------------------------------------------------------------------------------
// EEPROM
//------------------------------------------------------------------------------

#define EEPROM_SIZE             4096            // EEPROM size in bytes
#define EEPROM_RELAY_STATUS     0               // Address for the relay status (1 byte)
#define EEPROM_ENERGY_COUNT     1               // Address for the energy counter (4 bytes)
#define EEPROM_CUSTOM_RESET     5               // Address for the reset reason (1 byte)
#define EEPROM_CRASH_COUNTER    6               // Address for the crash counter (1 byte)
#define EEPROM_DATA_END         7               // End of custom EEPROM data block

//------------------------------------------------------------------------------
// HEARTBEAT
//------------------------------------------------------------------------------

#define HEARTBEAT_INTERVAL          300000      // Interval between heartbeat messages (in ms)
#define UPTIME_OVERFLOW             4294967295  // Uptime overflow value

// Topics that will be reported in heartbeat
#define HEARTBEAT_REPORT_STATUS     1
#define HEARTBEAT_REPORT_IP         1
#define HEARTBEAT_REPORT_MAC        1
#define HEARTBEAT_REPORT_RSSI       1
#define HEARTBEAT_REPORT_UPTIME     1
#define HEARTBEAT_REPORT_FREEHEAP   1
#define HEARTBEAT_REPORT_VCC        1
#define HEARTBEAT_REPORT_RELAY      1
#define HEARTBEAT_REPORT_LIGHT      1
#define HEARTBEAT_REPORT_HOSTNAME   1
#define HEARTBEAT_REPORT_APP        1
#define HEARTBEAT_REPORT_VERSION    1
#define HEARTBEAT_REPORT_INTERVAL   0

//------------------------------------------------------------------------------
// RESET
//------------------------------------------------------------------------------

#define CUSTOM_RESET_HARDWARE   1               // Reset from hardware button
#define CUSTOM_RESET_WEB        2               // Reset from web interface
#define CUSTOM_RESET_TERMINAL   3               // Reset from terminal
#define CUSTOM_RESET_MQTT       4               // Reset via MQTT
#define CUSTOM_RESET_RPC        5               // Reset via RPC (HTTP)
#define CUSTOM_RESET_OTA        6               // Reset after successful OTA update
#define CUSTOM_RESET_NOFUSS     8               // Reset after successful NOFUSS update
#define CUSTOM_RESET_UPGRADE    9               // Reset after update from web interface
#define CUSTOM_RESET_FACTORY    10              // Factory reset from terminal

#define CUSTOM_RESET_MAX        10

#include <pgmspace.h>

PROGMEM const char custom_reset_hardware[] = "Hardware button";
PROGMEM const char custom_reset_web[] = "Reset from web interface";
PROGMEM const char custom_reset_terminal[] = "Reset from terminal";
PROGMEM const char custom_reset_mqtt[] = "Reset from MQTT";
PROGMEM const char custom_reset_rpc[] = "Reset from RPC";
PROGMEM const char custom_reset_ota[] = "Reset after successful OTA update";
PROGMEM const char custom_reset_nofuss[] = "Reset after successful NoFUSS update";
PROGMEM const char custom_reset_upgrade[] = "Reset after successful web update";
PROGMEM const char custom_reset_factory[] = "Factory reset";
PROGMEM const char* const custom_reset_string[] = {
    custom_reset_hardware, custom_reset_web, custom_reset_terminal,
    custom_reset_mqtt, custom_reset_rpc, custom_reset_ota,
    custom_reset_nofuss, custom_reset_upgrade, custom_reset_factory
};

//------------------------------------------------------------------------------
// BUTTON
//------------------------------------------------------------------------------

#define BUTTON_DEBOUNCE_DELAY       50          // Debounce delay (ms)
#define BUTTON_DBLCLICK_DELAY       500         // Time in ms to wait for a second (or third...) click
#define BUTTON_LNGCLICK_DELAY       1000        // Time in ms holding the button down to get a long click
#define BUTTON_LNGLNGCLICK_DELAY    10000       // Time in ms holding the button down to get a long-long click

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

//------------------------------------------------------------------------------
// RELAY
//------------------------------------------------------------------------------

#define RELAY_MODE_OFF          0
#define RELAY_MODE_ON           1
#define RELAY_MODE_SAME         2
#define RELAY_MODE_TOOGLE       3

#define RELAY_TYPE_NORMAL       0
#define RELAY_TYPE_INVERSE      1
#define RELAY_TYPE_LATCHED      2

#define RELAY_SYNC_ANY          0
#define RELAY_SYNC_NONE_OR_ONE  1
#define RELAY_SYNC_ONE          2
#define RELAY_SYNC_SAME         3

#define RELAY_PULSE_NONE        0
#define RELAY_PULSE_OFF         1
#define RELAY_PULSE_ON          2

#define RELAY_PROVIDER_RELAY    0
#define RELAY_PROVIDER_DUAL     1
#define RELAY_PROVIDER_LIGHT    2
#define RELAY_PROVIDER_RFBRIDGE 3

// Pulse time in milliseconds
#define RELAY_PULSE_TIME        1.0

// 0 means OFF, 1 ON and 2 whatever was before
#define RELAY_MODE         		RELAY_MODE_OFF

// 0 means ANY, 1 zero or one and 2 one and only one
#define RELAY_SYNC         		RELAY_SYNC_ANY

// 0 means no pulses, 1 means normally off, 2 normally on
#define RELAY_PULSE_MODE     	RELAY_PULSE_NONE

// Relay requests flood protection window - in seconds
#define RELAY_FLOOD_WINDOW      3

// Allowed actual relay changes inside requests flood protection window
#define RELAY_FLOOD_CHANGES     5

// Pulse with in milliseconds for a latched relay
#define RELAY_LATCHING_PULSE    10

// Do not save relay state after these many milliseconds
#define RELAY_SAVE_DELAY        1000

//------------------------------------------------------------------------------
// I18N
//------------------------------------------------------------------------------

#define TMP_CELSIUS             0
#define TMP_FAHRENHEIT          1
#define TMP_UNITS               TMP_CELSIUS // Temperature units (TMP_CELSIUS | TMP_FAHRENHEIT)

//------------------------------------------------------------------------------
// LED
//------------------------------------------------------------------------------

// All defined LEDs in the board can be managed through MQTT
// except the first one when LED_AUTO is set to 1.
// If LED_AUTO is set to 1 the board will a defined LED to show wifi status.
#define LED_AUTO                1

// LED # to use as WIFI status indicator
#ifndef LED_WIFI
#define LED_WIFI                1
#endif

// -----------------------------------------------------------------------------
// WIFI
// -----------------------------------------------------------------------------

#define WIFI_CONNECT_TIMEOUT    30000       // Connecting timeout for WIFI in ms
#define WIFI_RECONNECT_INTERVAL 120000      // If could not connect to WIFI, retry after this time in ms
#define WIFI_MAX_NETWORKS       5           // Max number of WIFI connection configurations
#define WIFI_AP_MODE            AP_MODE_ALONE

// Optional hardcoded configuration (up to 2 different networks)
//#define WIFI1_SSID              "..."
//#define WIFI1_PASS              "..."
//#define WIFI1_IP                "192.168.1.201"
//#define WIFI1_GW                "192.168.1.1"
//#define WIFI1_MASK              "255.255.255.0"
//#define WIFI1_DNS               "8.8.8.8"
//#define WIFI2_SSID              "..."
//#define WIFI2_PASS              "..."

// -----------------------------------------------------------------------------
// WEB
// -----------------------------------------------------------------------------

#ifndef WEB_SUPPORT
#define WEB_SUPPORT             1           // Enable web support (http, api)
#endif

#ifndef WEB_EMBEDDED
#define WEB_EMBEDDED            1           // Build the firmware with the web interface embedded in
#endif

// This is not working at the moment!!
// Requires ASYNC_TCP_SSL_ENABLED to 1 and ESP8266 Arduino Core staging version.
#define WEB_SSL_ENABLED         0           // Use HTTPS web interface

#define WEB_MODE_NORMAL         0
#define WEB_MODE_PASSWORD       1

#define WEB_USERNAME            "admin"     // HTTP username
#define WEB_FORCE_PASS_CHANGE   1           // Force the user to change the password if default one
#define WEB_PORT                80          // HTTP port

// -----------------------------------------------------------------------------
// WEBSOCKETS
// -----------------------------------------------------------------------------

// This will only be enabled if WEB_SUPPORT is 1 (this is the default value)

#define WS_BUFFER_SIZE          5           // Max number of secured websocket connections
#define WS_TIMEOUT              1800000     // Timeout for secured websocket

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

// This will only be enabled if WEB_SUPPORT is 1 (this is the default value)

#define API_ENABLED              0          // Do not enable API by default
#define API_BUFFER_SIZE         10          // Size of the buffer for HTTP GET API responses

// -----------------------------------------------------------------------------
// MDNS
// -----------------------------------------------------------------------------

#ifndef MDNS_SUPPORT
#define MDNS_SUPPORT            1           // Publish services using mDNS by default
#endif

// -----------------------------------------------------------------------------
// SPIFFS
// -----------------------------------------------------------------------------

#ifndef SPIFFS_SUPPORT
#define SPIFFS_SUPPORT           0          // Do not add support for SPIFFS by default
#endif

// -----------------------------------------------------------------------------
// OTA
// -----------------------------------------------------------------------------

#define OTA_PORT                8266        // OTA port

// -----------------------------------------------------------------------------
// NOFUSS
// -----------------------------------------------------------------------------

#ifndef NOFUSS_SUPPORT
#define NOFUSS_SUPPORT          0          // Do not enable support for NoFuss by default
#endif

#define NOFUSS_ENABLED          0           // Do not perform NoFUSS updates by default
#define NOFUSS_SERVER           ""          // Default NoFuss Server
#define NOFUSS_INTERVAL         3600000     // Check for updates every hour

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

#ifndef MQTT_USE_ASYNC
#define MQTT_USE_ASYNC          1           // Use AysncMQTTClient (1) or PubSubClient (0)
#endif

// MQTT OVER SSL
// Using MQTT over SSL works pretty well but generates problems with the web interface.
// It could be a good idea to use it in conjuntion with WEB_SUPPORT=0.
// Requires ASYNC_TCP_SSL_ENABLED to 1 and ESP8266 Arduino Core staging version.
//
// You can use it with MQTT_USE_ASYNC=1 (AsyncMqttClient library)
// but you might experience hiccups on the web interface, so my recommendation is:
// WEB_SUPPORT=0
//
// If you use it with MQTT_USE_ASYNC=0 (PubSubClient library)
// you will have to disable all the modules that use ESPAsyncTCP, that is:
// ALEXA_SUPPORT=0, INFLUXDB_SUPPORT=0, TELNET_SUPPORT=0 and WEB_SUPPORT=0
//
// You will need the fingerprint for your MQTT server, example for CloudMQTT:
// $ echo -n | openssl s_client -connect m11.cloudmqtt.com:24055 > cloudmqtt.pem
// $ openssl x509 -noout -in cloudmqtt.pem -fingerprint -sha1

#define MQTT_SSL_ENABLED        0           // By default MQTT over SSL will not be enabled
#define MQTT_SSL_FINGERPRINT    ""          // SSL fingerprint of the server

#define MQTT_ENABLED            0           // Do not enable MQTT connection by default
#define MQTT_AUTOCONNECT        1           // If enabled and MDNS_SUPPORT=1 will perform an autodiscover and
                                            // autoconnect to the first MQTT broker found if none defined
#define MQTT_SERVER             ""          // Default MQTT broker address
#define MQTT_USER               ""          // Default MQTT broker usename
#define MQTT_PASS               ""          // Default MQTT broker password
#define MQTT_PORT               1883        // MQTT broker port
#define MQTT_TOPIC              "{identifier}"     // Default MQTT base topic
#define MQTT_RETAIN             true        // MQTT retain flag
#define MQTT_QOS                0           // MQTT QoS value for all messages
#define MQTT_KEEPALIVE          30          // MQTT keepalive value

#define MQTT_RECONNECT_DELAY_MIN    5000    // Try to reconnect in 5 seconds upon disconnection
#define MQTT_RECONNECT_DELAY_STEP   5000    // Increase the reconnect delay in 5 seconds after each failed attempt
#define MQTT_RECONNECT_DELAY_MAX    120000  // Set reconnect time to 2 minutes at most

#define MQTT_SKIP_RETAINED      1           // Skip retained messages on connection
#define MQTT_SKIP_TIME          1000        // Skip messages for 1 second anter connection

#define MQTT_USE_JSON           0           // Group messages in a JSON body
#define MQTT_USE_JSON_DELAY     100         // Wait this many ms before grouping messages

// These particles will be concatenated to the MQTT_TOPIC base to form the actual topic
#define MQTT_TOPIC_JSON         "data"
#define MQTT_TOPIC_ACTION       "action"
#define MQTT_TOPIC_RELAY        "relay"
#define MQTT_TOPIC_LED          "led"
#define MQTT_TOPIC_BUTTON       "button"
#define MQTT_TOPIC_IP           "ip"
#define MQTT_TOPIC_VERSION      "version"
#define MQTT_TOPIC_UPTIME       "uptime"
#define MQTT_TOPIC_FREEHEAP     "freeheap"
#define MQTT_TOPIC_VCC          "vcc"
#define MQTT_TOPIC_STATUS       "status"
#define MQTT_TOPIC_MAC          "mac"
#define MQTT_TOPIC_RSSI         "rssi"
#define MQTT_TOPIC_APP          "app"
#define MQTT_TOPIC_INTERVAL     "interval"
#define MQTT_TOPIC_HOSTNAME     "host"
#define MQTT_TOPIC_TIME         "time"
#define MQTT_TOPIC_ANALOG       "analog"
#define MQTT_TOPIC_COUNTER      "counter"
#define MQTT_TOPIC_RFOUT        "rfout"
#define MQTT_TOPIC_RFIN         "rfin"
#define MQTT_TOPIC_RFLEARN      "rflearn"

// Power module
#define MQTT_TOPIC_POWER_ACTIVE     "power"
#define MQTT_TOPIC_CURRENT          "current"
#define MQTT_TOPIC_VOLTAGE          "voltage"
#define MQTT_TOPIC_POWER_APPARENT   "apparent"
#define MQTT_TOPIC_POWER_REACTIVE   "reactive"
#define MQTT_TOPIC_POWER_FACTOR     "factor"
#define MQTT_TOPIC_ENERGY_DELTA     "energy_delta"
#define MQTT_TOPIC_ENERGY_TOTAL     "energy_total"

// Light module
#define MQTT_TOPIC_CHANNEL      "channel"
#define MQTT_TOPIC_COLOR        "color"
#define MQTT_TOPIC_COLOR_RGB    "color_rgb"
#define MQTT_TOPIC_COLOR_HSV    "color_hsv"
#define MQTT_TOPIC_ANIM_MODE    "anim_mode"
#define MQTT_TOPIC_ANIM_SPEED   "anim_speed"
#define MQTT_TOPIC_BRIGHTNESS   "brightness"
#define MQTT_TOPIC_MIRED        "mired"
#define MQTT_TOPIC_KELVIN       "kelvin"

#define MQTT_STATUS_ONLINE      "1"         // Value for the device ON message
#define MQTT_STATUS_OFFLINE     "0"         // Value for the device OFF message (will)

#define MQTT_ACTION_RESET       "reset"     // RESET MQTT topic particle

// Internal MQTT events (do not change)
#define MQTT_CONNECT_EVENT      0
#define MQTT_DISCONNECT_EVENT   1
#define MQTT_MESSAGE_EVENT      2

// Custom get and set postfixes
// Use something like "/status" or "/set", with leading slash
// Since 1.9.0 the default value is "" for getter and "/set" for setter
#define MQTT_USE_GETTER         ""
#define MQTT_USE_SETTER         "/set"

// -----------------------------------------------------------------------------
// SETTINGS
// -----------------------------------------------------------------------------

#ifndef SETTINGS_AUTOSAVE
#define SETTINGS_AUTOSAVE       1           // Autosave settings o force manual commit
#endif

// -----------------------------------------------------------------------------
// LIGHT
// -----------------------------------------------------------------------------

// Available light providers (do not change)
#define LIGHT_PROVIDER_NONE     0
#define LIGHT_PROVIDER_MY9192   1 // works with MY9231 also (Sonoff B1)
#define LIGHT_PROVIDER_DIMMER   2

// LIGHT_PROVIDER_DIMMER can have from 1 to 5 different channels.
// They have to be defined for each device in the hardware.h file.
// If 3 or more channels first 3 will be considered RGB.
// Usual configurations are:
// 1 channels => W
// 2 channels => WW
// 3 channels => RGB
// 4 channels => RGBW
// 5 channels => RGBWW

#ifndef LIGHT_SAVE_ENABLED
#define LIGHT_SAVE_ENABLED      1           // Light channel values saved by default after each change
#endif

#define LIGHT_SAVE_DELAY        5           // Persist color after 5 seconds to avoid wearing out
#define LIGHT_PWM_FREQUENCY     1000        // PWM frequency
#define LIGHT_MAX_PWM           4095        // Maximum PWM value
#define LIGHT_MAX_VALUE         255         // Maximum light value
#define LIGHT_MAX_BRIGHTNESS    255         // Maximun brightness value
#define LIGHT_USE_COLOR         1           // Use 3 first channels as RGB
#define LIGHT_USE_WHITE         0           // Use white channel whenever RGB have the same value
#define LIGHT_USE_GAMMA         0           // Use gamma correction for color channels

// -----------------------------------------------------------------------------
// POWER METERING
// -----------------------------------------------------------------------------

// Available power-metering providers (do not change this)
#define POWER_PROVIDER_NONE             0x00
#define POWER_PROVIDER_EMON_ANALOG      0x10
#define POWER_PROVIDER_EMON_ADC121      0x11
#define POWER_PROVIDER_HLW8012          0x20
#define POWER_PROVIDER_V9261F           0x30
#define POWER_PROVIDER_ECH1560          0x40

// Available magnitudes (do not change this)
#define POWER_MAGNITUDE_CURRENT         1
#define POWER_MAGNITUDE_VOLTAGE         2
#define POWER_MAGNITUDE_ACTIVE          4
#define POWER_MAGNITUDE_APPARENT        8
#define POWER_MAGNITUDE_REACTIVE        16
#define POWER_MAGNITUDE_POWER_FACTOR    32
#define POWER_MAGNITUDE_ALL             63

// No power provider defined (do not change this)
#ifndef POWER_PROVIDER
#define POWER_PROVIDER                  POWER_PROVIDER_NONE
#endif

// Identify available magnitudes (do not change this)
#if (POWER_PROVIDER == POWER_PROVIDER_HLW8012) || (POWER_PROVIDER == POWER_PROVIDER_V9261F)
#define POWER_HAS_ACTIVE                1
#else
#define POWER_HAS_ACTIVE                0
#endif

#if (POWER_PROVIDER == POWER_PROVIDER_HLW8012)
#define POWER_HAS_ENERGY                1
#else
#define POWER_HAS_ENERGY                0
#endif

#define POWER_VOLTAGE                   230     // Default voltage
#define POWER_READ_INTERVAL             6000    // Default reading interval (6 seconds)
#define POWER_REPORT_INTERVAL           60000   // Default report interval (1 minute)
#define POWER_REPORT_BUFFER             12      // Default buffer size ()
#define POWER_CURRENT_DECIMALS          2       // Decimals for current values
#define POWER_VOLTAGE_DECIMALS          0       // Decimals for voltage values
#define POWER_POWER_DECIMALS            0       // Decimals for power values
#define POWER_ENERGY_DECIMALS           3       // Decimals for energy values
#define POWER_ENERGY_DECIMALS_KWH       6       // Decimals for energy values
#define POWER_ENERGY_FACTOR             1       // Watt * seconds == Joule (Ws == J)
#define POWER_ENERGY_FACTOR_KWH         (1. / 1000. / 3600.) // kWh

#if POWER_PROVIDER == POWER_PROVIDER_EMON_ANALOG
    #define EMON_CURRENT_RATIO          30      // Current ratio in the clamp (30V/1A)
    #define EMON_SAMPLES                1000    // Number of samples to get for each reading
	#define EMON_ADC_BITS               10      // ADC depth
	#define EMON_REFERENCE_VOLTAGE      1.0     // Reference voltage of the ADC
    #define EMON_CURRENT_OFFSET         0.25    // Current offset (error)
    #undef ADC_VCC_ENABLED
    #define ADC_VCC_ENABLED             0       // Disable internal battery measurement
#endif

#if POWER_PROVIDER == POWER_PROVIDER_EMON_ADC121
    #define EMON_CURRENT_RATIO          30      // Current ratio in the clamp (30V/1A)
    #define EMON_SAMPLES                1000    // Number of samples to get for each reading
    #define EMON_ADC_BITS               12      // ADC depth
	#define EMON_REFERENCE_VOLTAGE      3.3     // Reference voltage of the ADC
    #define EMON_CURRENT_OFFSET         0.10    // Current offset (error)
    #define ADC121_I2C_ADDRESS          0x50    // I2C address of the ADC121
    #undef I2C_SUPPORT
    #define I2C_SUPPORT                 1       // Enabled I2C support
#endif

#if POWER_PROVIDER == POWER_PROVIDER_HLW8012
    #define HLW8012_USE_INTERRUPTS      1       // Use interrupts to trap HLW8012 signals
    #define HLW8012_SEL_CURRENT         HIGH    // SEL pin to HIGH to measure current
    #define HLW8012_CURRENT_R           0.001   // Current resistor
    #define HLW8012_VOLTAGE_R_UP        ( 5 * 470000 )  // Upstream voltage resistor
    #define HLW8012_VOLTAGE_R_DOWN      ( 1000 )        // Downstream voltage resistor
#endif

#if POWER_PROVIDER == POWER_PROVIDER_V9261F

    #undef POWER_REPORT_BUFFER
    #define POWER_REPORT_BUFFER         60      // Override median buffer size

    #ifndef V9261F_PIN
    #define V9261F_PIN                  2       // TX pin from the V9261F
    #endif
    #ifndef V9261F_PIN_INVERSE
    #define V9261F_PIN_INVERSE          1       // Signal is inverted
    #endif

    #define V9261F_SYNC_INTERVAL        600     // Sync signal length (ms)
    #define V9261F_BAUDRATE             4800    // UART baudrate

    // Default ratios
    #define V9261F_CURRENT_FACTOR       79371434.0
    #define V9261F_VOLTAGE_FACTOR       4160651.0
    #define V9261F_POWER_FACTOR         153699.0
    #define V9261F_RPOWER_FACTOR        V9261F_CURRENT_FACTOR

#endif

#if POWER_PROVIDER == POWER_PROVIDER_ECH1560

    #undef POWER_REPORT_BUFFER
    #define POWER_REPORT_BUFFER         60      // Override median buffer size

    #ifndef ECH1560_CLK_PIN
    #define ECH1560_CLK_PIN             4       // Default CLK pin
    #endif
    #ifndef ECH1560_MISO_PIN
    #define ECH1560_MISO_PIN            5       // Default MISO pin
    #endif
    #ifndef ECH1560_INVERTED
    #define ECH1560_INVERTED            0       // Power signal is inverted
    #endif

#endif

// -----------------------------------------------------------------------------
// I2C
// -----------------------------------------------------------------------------

#ifndef I2C_SUPPORT
#define I2C_SUPPORT             0           // I2C enabled
#endif

#define I2C_SDA_PIN             4           // SDA GPIO
#define I2C_SCL_PIN             14          // SCL GPIO
#define I2C_CLOCK_STRETCH_TIME  200         // BRZO clock stretch time
#define I2C_SCL_FREQUENCY       1000        // BRZO SCL frequency

// -----------------------------------------------------------------------------
// DOMOTICZ
// -----------------------------------------------------------------------------

#ifndef DOMOTICZ_SUPPORT
#define DOMOTICZ_SUPPORT        1               // Build with domoticz support
#endif

#define DOMOTICZ_ENABLED        0               // Disable domoticz by default
#define DOMOTICZ_IN_TOPIC       "domoticz/in"   // Default subscription topic
#define DOMOTICZ_OUT_TOPIC      "domoticz/out"  // Default publication topic

// -----------------------------------------------------------------------------
// HOME ASSISTANT
// -----------------------------------------------------------------------------

#ifndef HOMEASSISTANT_SUPPORT
#define HOMEASSISTANT_SUPPORT   1               // Build with home assistant support
#endif

#define HOMEASSISTANT_PREFIX    "homeassistant" // Default MQTT prefix

// -----------------------------------------------------------------------------
// INFLUXDB
// -----------------------------------------------------------------------------

#ifndef INFLUXDB_SUPPORT
#define INFLUXDB_SUPPORT         1               // Enable InfluxDB support by default
#endif

#define INFLUXDB_PORT           8086            // Default InfluxDB port

// -----------------------------------------------------------------------------
// NTP
// -----------------------------------------------------------------------------

#ifndef NTP_SUPPORT
#define NTP_SUPPORT             1               // Build with NTP support by default
#endif

#define NTP_SERVER              "pool.ntp.org"  // Default NTP server
#define NTP_TIME_OFFSET         1               // Default timezone offset (GMT+1)
#define NTP_DAY_LIGHT           true            // Enable daylight time saving by default
#define NTP_UPDATE_INTERVAL     1800            // NTP check every 30 minutes

// -----------------------------------------------------------------------------
// FAUXMO
// -----------------------------------------------------------------------------

// This setting defines whether Alexa support should be built into the firmware
#ifndef ALEXA_SUPPORT
#define ALEXA_SUPPORT           1
#endif

// This is default value for the alexaEnabled setting that defines whether
// this device should be discoberable and respond to Alexa commands.
// Both ALEXA_SUPPORT and alexaEnabled should be 1 for Alexa support to work.
#define ALEXA_ENABLED           1


// -----------------------------------------------------------------------------
// RFBRIDGE
// -----------------------------------------------------------------------------

#define RF_SEND_TIMES           4               // How many times to send the message
#define RF_SEND_DELAY           250             // Interval between sendings in ms
