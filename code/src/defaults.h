// Managed from platformio.ini
//#define DEBUG
//#define ESPURNA
//#define SONOFF
//#define SLAMPHER
//#define S20
//#define NODEMCUV2

//#define ENABLE_NOFUSS           1
//#define ENABLE_EMON             1
//#define ENABLE_RF               1
//#define ENABLE_DHT              1

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
    #define LED_PIN             16
#endif

#define AP_PASS                 "fibonacci"

#define OTA_PASS                "fibonacci"
#define OTA_PORT                8266

#define BUFFER_SIZE             1024
#define STATUS_UPDATE_INTERVAL  30000
#define HEARTBEAT_INTERVAL      60000
#define FS_VERSION_FILE         "/fsversion"

#define WIFI_RECONNECT_INTERVAL 300000

#define RF_PIN                  14

#define DHT_PIN                 14
#define DHT_UPDATE_INTERVAL     300000
#define DHT_TYPE                DHT22
#define DHT_TIMING              11

#define MQTT_RECONNECT_DELAY    10000
#define MQTT_RETAIN             true
#define MQTT_STATUS_TOPIC       ""
#define MQTT_IP_TOPIC           "/ip"
#define MQTT_VERSION_TOPIC      "/version"
#define MQTT_FSVERSION_TOPIC    "/fsversion"
#define MQTT_HEARTBEAT_TOPIC    "/heartbeat"
#define MQTT_POWER_TOPIC        "/power"
#define MQTT_TEMPERATURE_TOPIC  "/temperature"
#define MQTT_HUMIDITY_TOPIC     "/humidity"

#define EMON_CURRENT_PIN        0
#define EMON_SAMPLES            1000
#define EMON_INTERVAL           10000
#define EMON_MEASUREMENTS       6
#define EMON_ADC_BITS           10
#define EMON_REFERENCE_VOLTAGE  1.0
#define EMON_CURRENT_PRECISION  1
#define EMON_CURRENT_OFFSET     0.25
