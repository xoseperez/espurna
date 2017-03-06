//--------------------------------------------------------------------------------
// These settings are normally provided by PlatformIO
// Uncomment the appropiate line(s) to build from the Arduino IDE
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
// General
//--------------------------------------------------------------------------------

#ifndef DEBUG_PORT
#define DEBUG_PORT              Serial
#endif

// Uncomment and configure these lines to enable remote debug via udpDebug
// To receive the message son the destination computer use nc:
// nc -ul 8111

//#define DEBUG_UDP_IP            IPAddress(192, 168, 1, 100)
//#define DEBUG_UDP_PORT          8111

//--------------------------------------------------------------------------------
// Hardware
//--------------------------------------------------------------------------------

//#define D1_RELAYSHIELD
//#define NODEMCUV2
//#define SONOFF
//#define SONOFF_TH
//#define SLAMPHER
//#define S20
//#define SONOFF_TOUCH
//#define SONOFF_SV
//#define SONOFF_POW
//#define SONOFF_DUAL
//#define SONOFF_4CH
//#define ESP_RELAY_BOARD
//#define ECOPLUG
//#define WIFI_RELAY_NC
//#define WIFI_RELAY_NO
//#define MQTT_RELAY
//#define WIFI_RELAYS_BOARD_KIT

//--------------------------------------------------------------------------------
// Features (values below are non-default values)
//--------------------------------------------------------------------------------

//#define ENABLE_DHT            1
//#define ENABLE_DS18B20        1
//#define ENABLE_EMON           1
//#define ENABLE_HLW8018        1
//#define ENABLE_RF             1
//#define ENABLE_FAUXMO         0
//#define ENABLE_NOFUSS         1
//#define ENABLE_DOMOTICZ       0
