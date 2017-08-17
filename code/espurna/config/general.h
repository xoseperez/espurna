//------------------------------------------------------------------------------
// Do not change this file unless you know what you are doing
// Configuration settings are in the settings.h file
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// GENERAL
//------------------------------------------------------------------------------

#define SERIAL_BAUDRATE         115200          // Debugging console boud rate
#define HOSTNAME                DEVICE          // Hostname
#define UPTIME_OVERFLOW         4294967295      // Uptime overflow value

//--------------------------------------------------------------------------------
// DEBUG
//--------------------------------------------------------------------------------

#ifndef DEBUG_PORT
#define DEBUG_PORT              Serial          // Default debugging port
#endif

// Uncomment and configure these lines to enable remote debug via udpDebug
// To receive the message son the destination computer use nc:
// nc -ul 8111

#define DEBUG_UDP_IP            IPAddress(192, 168, 1, 100)
#define DEBUG_UDP_PORT          8113

//--------------------------------------------------------------------------------
// EEPROM
//--------------------------------------------------------------------------------

#define EEPROM_RELAY_STATUS     0               // Address for the relay status (1 byte)
#define EEPROM_ENERGY_COUNT     1               // Address for the energy counter (4 bytes)
#define EEPROM_CUSTOM_RESET     5               // Address for the reset reason (1 byte)
#define EEPROM_DATA_END         6               // End of custom EEPROM data block

//--------------------------------------------------------------------------------
// HEARTBEAT
//--------------------------------------------------------------------------------

#define HEARTBEAT_INTERVAL          300000      // Interval between heartbeat messages (in ms)

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

//--------------------------------------------------------------------------------
// RESET
//--------------------------------------------------------------------------------

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

//--------------------------------------------------------------------------------
// BUTTON
//--------------------------------------------------------------------------------

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

//--------------------------------------------------------------------------------
// RELAY
//--------------------------------------------------------------------------------

#define RELAY_MODE_OFF          0
#define RELAY_MODE_ON           1
#define RELAY_MODE_SAME         2
#define RELAY_MODE_TOOGLE       3

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

//--------------------------------------------------------------------------------
// I18N
//--------------------------------------------------------------------------------

#define TMP_CELSIUS             0
#define TMP_FAHRENHEIT          1
#define TMP_UNITS               TMP_CELSIUS // Temperature units (TMP_CELSIUS | TMP_FAHRENHEIT)

//--------------------------------------------------------------------------------
// LED
//--------------------------------------------------------------------------------

// All defined LEDs in the board can be managed through MQTT
// except the first one when LED_AUTO is set to 1.
// If LED_AUTO is set to 1 the board will use first defined LED to show wifi status.
#define LED_AUTO                1

// -----------------------------------------------------------------------------
// WIFI & WEB
// -----------------------------------------------------------------------------

#define WIFI_CONNECT_TIMEOUT    30000       // Connecting timeout for WIFI in ms
#define WIFI_RECONNECT_INTERVAL 120000      // If could not connect to WIFI, retry after this time in ms
#define WIFI_MAX_NETWORKS       5           // Max number of WIFI connection configurations
#define HTTP_USERNAME           "admin"     // HTTP username
#define ADMIN_PASS              "fibonacci" // Default password
#define FORCE_CHANGE_PASS       1           // Force the user to change the password if default one
#define WS_BUFFER_SIZE          5           // Max number of secured websocket connections
#define WS_TIMEOUT              1800000     // Timeout for secured websocket
#define WEBSERVER_PORT          80          // HTTP port
#define DNS_PORT                53          // MDNS port
#define ENABLE_MDNS             1           // Enabled MDNS
#define API_BUFFER_SIZE         10          // Size of the buffer for HTTP GET API responses

#define WEB_MODE_NORMAL         0
#define WEB_MODE_PASSWORD       1

#define AP_MODE                 AP_MODE_ALONE

// This option builds the firmware with the web interface embedded.
#ifndef EMBEDDED_WEB
#define EMBEDDED_WEB            1
#endif

// -----------------------------------------------------------------------------
// OTA & NOFUSS
// -----------------------------------------------------------------------------

#define OTA_PORT                8266        // OTA port
#define NOFUSS_SERVER           ""          // Default NoFuss Server
#define NOFUSS_INTERVAL         3600000     // Check for updates every hour

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

#ifndef MQTT_USE_ASYNC
#define MQTT_USE_ASYNC          1           // Use AysncMQTTClient (1) or PubSubClient (0)
#endif

#define MQTT_SERVER             ""          // Default MQTT broker address
#define MQTT_PORT               1883        // MQTT broker port
#define MQTT_TOPIC              "/test/switch/{identifier}"     // Default MQTT base topic
#define MQTT_RETAIN             true        // MQTT retain flag
#define MQTT_QOS                0           // MQTT QoS value for all messages
#define MQTT_KEEPALIVE          30          // MQTT keepalive value
#define MQTT_RECONNECT_DELAY    10000       // Try to reconnect after 10s
#define MQTT_TRY_INTERVAL       30000       // Timeframe for disconnect retries
#define MQTT_MAX_TRIES          12          // After these many retries during the previous MQTT_TRY_INTERVAL the board will reset
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

// Lights
#define MQTT_TOPIC_CHANNEL      "channel"
#define MQTT_TOPIC_COLOR        "color"
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
#define MQTT_USE_GETTER         ""
#define MQTT_USE_SETTER         "/set"

// -----------------------------------------------------------------------------
// I2C
// -----------------------------------------------------------------------------

#define ENABLE_I2C              0           // I2C enabled
#define I2C_SDA_PIN             4           // SDA GPIO
#define I2C_SCL_PIN             14          // SCL GPIO
#define I2C_CLOCK_STRETCH_TIME  200         // BRZO clock stretch time
#define I2C_SCL_FREQUENCY       1000        // BRZO SCL frequency

// -----------------------------------------------------------------------------
// LIGHT
// -----------------------------------------------------------------------------

#define LIGHT_PROVIDER_NONE     0
#define LIGHT_PROVIDER_MY9192   1
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

#define LIGHT_DEFAULT_COLOR     "#000080"   // Default start color
#define LIGHT_SAVE_DELAY        5           // Persist color after 5 seconds to avoid wearing out
#define LIGHT_PWM_FREQUENCY     1000        // PWM frequency
#define LIGHT_MAX_PWM           4095        // Maximum PWM value
#define LIGHT_MAX_VALUE         255         // Maximum light value
#define LIGHT_MAX_BRIGHTNESS    255         // Maximun brightness value
#define LIGHT_USE_WHITE         0           // Use white channel whenever RGB have the same value
#define LIGHT_ENABLE_GAMMA      0           // Enable gamma correction

// -----------------------------------------------------------------------------
// DOMOTICZ
// -----------------------------------------------------------------------------

#ifndef ENABLE_DOMOTICZ
#define ENABLE_DOMOTICZ         1               // Enable Domoticz support by default
#endif
#define DOMOTICZ_IN_TOPIC       "domoticz/in"   // Default subscription topic
#define DOMOTICZ_OUT_TOPIC      "domoticz/out"  // Default publication topic

// -----------------------------------------------------------------------------
// INFLUXDB
// -----------------------------------------------------------------------------

#ifndef ENABLE_INFLUXDB
#define ENABLE_INFLUXDB         1               // Enable InfluxDB support by default
#endif
#define INFLUXDB_PORT           8086            // Default InfluxDB port

// -----------------------------------------------------------------------------
// NTP
// -----------------------------------------------------------------------------

#define NTP_SERVER              "pool.ntp.org"  // Default NTP server
#define NTP_TIME_OFFSET         1               // Default timezone offset (GMT+1)
#define NTP_DAY_LIGHT           true            // Enable daylight time saving by default
#define NTP_UPDATE_INTERVAL     1800            // NTP check every 30 minutes

// -----------------------------------------------------------------------------
// FAUXMO
// -----------------------------------------------------------------------------

// This setting defines whether Alexa support should be built into the firmware
#ifndef ENABLE_FAUXMO
#define ENABLE_FAUXMO       1
#endif

// This is default value for the fauxmoEnabled setting that defines whether
// this device should be discoberable and respond to Alexa commands.
// Both ENABLE_FAUXMO and fauxmoEnabled should be 1 for Alexa support to work.
#define FAUXMO_ENABLED          1
