//------------------------------------------------------------------------------
// GENERAL
//------------------------------------------------------------------------------

#define SERIAL_BAUDRATE         115200
#define HOSTNAME                DEVICE
#define BUFFER_SIZE             1024
#define HEARTBEAT_INTERVAL      300000

//--------------------------------------------------------------------------------
// EEPROM
//--------------------------------------------------------------------------------

#define EEPROM_RELAY_STATUS     0
#define EEPROM_ENERGY_COUNT     1

//--------------------------------------------------------------------------------
// BUTTON
//--------------------------------------------------------------------------------

#define BUTTON_EVENT_NONE       0
#define BUTTON_EVENT_PRESSED    1
#define BUTTON_EVENT_CLICK      2
#define BUTTON_EVENT_DBLCLICK   3
#define BUTTON_EVENT_LNGCLICK   4

#define BUTTON_MODE_NONE        0
#define BUTTON_MODE_TOGGLE      1
#define BUTTON_MODE_AP          2
#define BUTTON_MODE_RESET       3
#define BUTTON_MODE_PULSE       4

#define BUTTON_DEFAULT_MODE     BUTTON_MODE_TOGGLE

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

// Pulse time in seconds
#define RELAY_PULSE_TIME        1

// 0 means OFF, 1 ON and 2 whatever was before
#define RELAY_MODE         		RELAY_MODE_OFF

// 0 means ANY, 1 zero or one and 2 one and only one
#define RELAY_SYNC         		RELAY_SYNC_ANY

// 0 means no pulses, 1 means normally off, 2 normally on
#define RELAY_PULSE_MODE     	RELAY_PULSE_NONE

//--------------------------------------------------------------------------------
// I18N
//--------------------------------------------------------------------------------

#define TMP_CELSIUS             0
#define TMP_FAHRENHEIT          1
#define TMP_UNITS               TMP_CELSIUS

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

#define WIFI_RECONNECT_INTERVAL 300000
#define WIFI_MAX_NETWORKS       5
#define ADMIN_PASS              "fibonacci"
#define FORCE_CHANGE_PASS       1
#define HTTP_USERNAME           "admin"
#define WS_BUFFER_SIZE          5
#define WS_TIMEOUT              1800000
#define WEBSERVER_PORT          80
#define DNS_PORT                53

#define WEB_MODE_NORMAL         0
#define WEB_MODE_PASSWORD       1

#define AP_MODE                 AP_MODE_ALONE
#define AP_MODE_IP              "192.168.4.1"
#define AP_MODE_GW              "192.168.4.1"
#define AP_MODE_MASK            "255.255.255.0"

#define EMBED_WEB_IN_FIRMWARE   0

// -----------------------------------------------------------------------------
// OTA & NOFUSS
// -----------------------------------------------------------------------------

#define OTA_PORT                8266
#define NOFUSS_SERVER           ""
#define NOFUSS_INTERVAL         3600000

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

#define MQTT_SERVER             ""
#define MQTT_PORT               1883
#define MQTT_TOPIC              "/test/switch/{identifier}"
#define MQTT_RETAIN             true
#define MQTT_QOS                0
#define MQTT_KEEPALIVE          30
#define MQTT_RECONNECT_DELAY    10000
#define MQTT_TRY_INTERVAL       30000
#define MQTT_MAX_TRIES          12
#define MQTT_SKIP_RETAINED      1
#define MQTT_SKIP_TIME          1000
#define MQTT_ACTION_TOPIC       "/action"
#define MQTT_RELAY_TOPIC        "/relay"
#define MQTT_LED_TOPIC          "/led"
#define MQTT_BUTTON_TOPIC       "/button"
#define MQTT_IP_TOPIC           "/ip"
#define MQTT_VERSION_TOPIC      "/version"
#define MQTT_HEARTBEAT_TOPIC    "/status"

#define MQTT_ACTION_RESET       "reset"

#define MQTT_CONNECT_EVENT      0
#define MQTT_DISCONNECT_EVENT   1
#define MQTT_MESSAGE_EVENT      2

// Custom get and set postfixes
// Use something like "/status" or "/set", with leading slash
#define MQTT_USE_GETTER         ""
#define MQTT_USE_SETTER         ""

// -----------------------------------------------------------------------------
// I2C
// -----------------------------------------------------------------------------

#define ENABLE_I2C              0
#define I2C_SDA_PIN             4
#define I2C_SCL_PIN             14
#define I2C_CLOCK_STRETCH_TIME  200
#define I2C_SCL_FREQUENCY       1000

// -----------------------------------------------------------------------------
// DOMOTICZ
// -----------------------------------------------------------------------------

#ifndef ENABLE_DOMOTICZ
    #define ENABLE_DOMOTICZ     1
#endif
#define DOMOTICZ_IN_TOPIC       "domoticz/in"
#define DOMOTICZ_OUT_TOPIC      "domoticz/out"

// -----------------------------------------------------------------------------
// NTP
// -----------------------------------------------------------------------------

#define NTP_SERVER              "pool.ntp.org"
#define NTP_TIME_OFFSET         1
#define NTP_DAY_LIGHT           true
#define NTP_UPDATE_INTERVAL     1800

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
