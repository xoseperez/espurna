//------------------------------------------------------------------------------
// SET BY PLATFORMIO
//------------------------------------------------------------------------------

//#define DEBUG_PORT              Serial

//#define ESPURNA
//#define SONOFF
//#define SLAMPHER
//#define S20
//#define NODEMCUV2

//#define ENABLE_NOFUSS           1
//#define ENABLE_EMON             1
//#define ENABLE_DHT              1
//#define ENABLE_RF               1
//#define ENABLE_POW              1

// -----------------------------------------------------------------------------
// HARDWARE
// -----------------------------------------------------------------------------

#define SERIAL_BAUDRATE         115200
#define BUTTON_PIN              0
#define RELAY_PIN               12

#ifdef ESPURNA
    #define MANUFACTURER        "TINKERMAN"
    #define DEVICE              "ESPURNA"
    #define LED_PIN             13
#endif

#ifdef SONOFF
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF"
    #define LED_PIN             13
#endif

#ifdef SONOFF_POW
    #define ENABLE_POW          1
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SONOFF_POW"
    #define LED_PIN             13
#endif

#ifdef SLAMPHER
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "SLAMPHER"
    #define LED_PIN             13
#endif

#ifdef S20
    #define MANUFACTURER        "ITEAD"
    #define DEVICE              "S20"
    #define LED_PIN             13
#endif

#ifdef NODEMCUV2
    #define MANUFACTURER        "NODEMCU"
    #define DEVICE              "LOLIN"
    #define LED_PIN             2
#endif

#define HOSTNAME                DEVICE
#define BUFFER_SIZE             1024
#define HEARTBEAT_INTERVAL      300000
#define FS_VERSION_FILE         "/fsversion"


// -----------------------------------------------------------------------------
// WIFI & WEB
// -----------------------------------------------------------------------------

#define WIFI_RECONNECT_INTERVAL 300000
#define WIFI_MAX_NETWORKS       3
#define ADMIN_PASS              "fibonacci"
#define HTTP_USERNAME           "admin"
#define CSRF_BUFFER_SIZE        5

// -----------------------------------------------------------------------------
// OTA & NOFUSS
// -----------------------------------------------------------------------------

#define OTA_PORT                8266
#define NOFUSS_SERVER           "http://192.168.1.100"
#define NOFUSS_INTERVAL         3600000

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

#define MQTT_SERVER             "192.168.1.100"
#define MQTT_PORT               1883
#define MQTT_TOPIC              "/test/switch/{identifier}"
#define MQTT_RETAIN             true
#define MQTT_QOS                0
#define MQTT_KEEPALIVE          30
#define MQTT_RECONNECT_DELAY    10000
#define MQTT_STATUS_TOPIC       ""
#define MQTT_IP_TOPIC           "/ip"
#define MQTT_VERSION_TOPIC      "/version"
#define MQTT_FSVERSION_TOPIC    "/fsversion"
#define MQTT_HEARTBEAT_TOPIC    "/heartbeat"

// -----------------------------------------------------------------------------
// NTP
// -----------------------------------------------------------------------------

#define NTP_SERVER              "pool.ntp.org"
#define NTP_TIME_OFFSET         1
#define NTP_DAY_LIGHT           true
#define NTP_UPDATE_INTERVAL     1800

//--------------------------------------------------------------------------------
// DRIVERS
//--------------------------------------------------------------------------------

// 0 means OFF, 1 ON and 2 whatever was before
#define RELAY_MODE         		1

#define RF_PIN                  14
#define RF_CHANNEL              31
#define RF_DEVICE               1

#define DHT_PIN                 14
#define DHT_UPDATE_INTERVAL     300000
#define DHT_TYPE                DHT22
#define DHT_TIMING              11
#define DHT_TEMPERATURE_TOPIC   "/temperature"
#define DHT_HUMIDITY_TOPIC      "/humidity"

#define EMON_CURRENT_PIN        0
#define EMON_SAMPLES            1000
#define EMON_INTERVAL           10000
#define EMON_MEASUREMENTS       6
#define EMON_ADC_BITS           10
#define EMON_REFERENCE_VOLTAGE  1.0
#define EMON_CURRENT_PRECISION  1
#define EMON_CURRENT_OFFSET     0.25
#define EMON_MAINS_VOLTAGE      230
#define EMON_CURRENT_RATIO      180
#define EMON_POWER_TOPIC        "/power"

#define POW_SEL_PIN             5
#define POW_CF1_PIN             13
#define POW_CF_PIN              14
#define POW_SEL_CURRENT         HIGH
#define POW_CURRENT_R           0.001
#define POW_VOLTAGE_R_UP        ( 5 * 470000 ) // Real: 2280k
#define POW_VOLTAGE_R_DOWN      ( 1000 ) // Real 1.009k
#define POW_POWER_TOPIC         "/power"
#define POW_UPDATE_INTERVAL     10000
#define POW_REPORT_EVERY        6

#define FAUXMO_ENABLED          1
