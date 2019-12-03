#pragma once

//------------------------------------------------------------------------------
// Do not change this file unless you know what you are doing
// Configuration settings are in the general.h file
//------------------------------------------------------------------------------

#if DEBUG_TELNET_SUPPORT
#undef TELNET_SUPPORT
#define TELNET_SUPPORT              1
#endif

#if not WEB_SUPPORT
#undef DEBUG_WEB_SUPPORT
#define DEBUG_WEB_SUPPORT           0
#endif

#if not WEB_SUPPORT
#undef API_SUPPORT
#define API_SUPPORT                 0           // API support requires web support
#endif

#if not WEB_SUPPORT
#undef SSDP_SUPPORT
#define SSDP_SUPPORT                0           // SSDP support requires web support
#endif

#if UART_MQTT_SUPPORT
#define MQTT_SUPPORT                1
#undef TERMINAL_SUPPORT
#define TERMINAL_SUPPORT            0
#undef DEBUG_SERIAL_SUPPORT
#define DEBUG_SERIAL_SUPPORT        0
#endif

#if ALEXA_SUPPORT
#undef BROKER_SUPPORT
#define BROKER_SUPPORT              1               // If Alexa enabled enable BROKER
#endif

#if RPN_RULES_SUPPORT
#undef BROKER_SUPPORT
#define BROKER_SUPPORT              1               // If RPN Rules enabled enable BROKER
#undef WEB_SUPPORT
#define WEB_SUPPORT                 1
#undef MQTT_SUPPORT
#define MQTT_SUPPORT                1
#endif


#if INFLUXDB_SUPPORT
#undef BROKER_SUPPORT
#define BROKER_SUPPORT              1               // If InfluxDB enabled enable BROKER
#endif

#if DOMOTICZ_SUPPORT
#undef MQTT_SUPPORT
#define MQTT_SUPPORT                1               // If Domoticz enabled enable MQTT
#undef BROKER_SUPPORT
#define BROKER_SUPPORT              1               // If Domoticz enabled enable BROKER
#endif

#if HOMEASSISTANT_SUPPORT
#undef MQTT_SUPPORT
#define MQTT_SUPPORT                1               // If Home Assistant enabled enable MQTT
#endif

#if THINGSPEAK_SUPPORT
#undef BROKER_SUPPORT
#define BROKER_SUPPORT              1               // If Thingspeak enabled enable BROKER
#endif

#if SCHEDULER_SUPPORT
#undef NTP_SUPPORT
#define NTP_SUPPORT                 1           // Scheduler needs NTP
#endif

#if LWIP_VERSION_MAJOR != 1
#undef MDNS_CLIENT_SUPPORT
#define MDNS_CLIENT_SUPPORT         0          // default resolver already handles this
#endif

#if not defined(ARDUINO_ESP8266_RELEASE_2_3_0)
#undef TELNET_SERVER_ASYNC_BUFFERED
#define TELNET_SERVER_ASYNC_BUFFERED 1         // enable buffered telnet by default on latest Cores
#endif

#if LLMNR_SUPPORT && defined(ARDUINO_ESP8266_RELEASE_2_3_0)
#undef LLMNR_SUPPORT
#define LLMNR_SUPPORT 0
#endif

#if NETBIOS_SUPPORT && defined(ARDUINO_ESP8266_RELEASE_2_3_0)
#undef NETBIOS_SUPPORT
#define NETBIOS_SUPPORT 0
#endif

#if SSDP_SUPPORT && defined(ARDUINO_ESP8266_RELEASE_2_3_0)
#undef SSDP_SUPPORT
#define SSDP_SUPPORT 0
#endif

#if LIGHT_PROVIDER == LIGHT_PROVIDER_TUYA
#undef TUYA_SUPPORT
#define TUYA_SUPPORT                1           // Need base Tuya module for this to work
#undef LIGHT_USE_TRANSITIONS
#define LIGHT_USE_TRANSITIONS       0           // TODO: temporary, maybe slower step instead?
#endif

#if TUYA_SUPPORT
#undef BROKER_SUPPORT
#define BROKER_SUPPORT              1           // Broker is required to process relay & lights events
#endif

// TODO: clean-up SSL_ENABLED and USE_SSL settings for 1.15.0
#if ASYNC_TCP_SSL_ENABLED && SECURE_CLIENT == SECURE_CLIENT_NONE
#undef SECURE_CLIENT
#define SECURE_CLIENT               SECURE_CLIENT_AXTLS
#endif

#if THINGSPEAK_USE_SSL && THINGSPEAK_USE_ASYNC && (!ASYNC_TCP_SSL_ENABLED)
#warning "Thingspeak in ASYNC mode requires a globally defined ASYNC_TCP_SSL_ENABLED=1"
#undef THINGSPEAK_SUPPORT
#define THINGSPEAK_SUPPORT          0               // Thingspeak in ASYNC mode requires ASYNC_TCP_SSL_ENABLED
#endif

#if WEB_SUPPORT && WEB_SSL_ENABLED && (!ASYNC_TCP_SSL_ENABLED)
#warning "WEB_SUPPORT with SSL requires a globally defined ASYNC_TCP_SSL_ENABLED=1"
#undef WEB_SSL_ENABLED
#define WEB_SSL_ENABLED          0               // WEB_SUPPORT mode th SSL requires ASYNC_TCP_SSL_ENABLED
#endif
