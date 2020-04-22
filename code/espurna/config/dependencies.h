//------------------------------------------------------------------------------
// Do not change this file unless you know what you are doing
// Configuration settings are in the general.h file
//------------------------------------------------------------------------------

#pragma once

//------------------------------------------------------------------------------
// Various modules configuration

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
#undef MQTT_SUPPORT
#define MQTT_SUPPORT                1           // UART<->MQTT requires MQTT and no serial debug
#undef DEBUG_SERIAL_SUPPORT
#define DEBUG_SERIAL_SUPPORT        0           // TODO: compare UART_MQTT_PORT with DEBUG_PORT? (as strings)
#endif

#if ALEXA_SUPPORT
#undef BROKER_SUPPORT
#define BROKER_SUPPORT              1               // If Alexa enabled enable BROKER
#undef RELAY_SUPPORT
#define RELAY_SUPPORT               1               // and switches
#endif

#if RPN_RULES_SUPPORT
#undef BROKER_SUPPORT
#define BROKER_SUPPORT              1               // If RPN Rules enabled enable BROKER
#undef MQTT_SUPPORT
#define MQTT_SUPPORT                1
#endif

#if RF_SUPPORT
#undef RELAY_SUPPORT
#define RELAY_SUPPORT               1
#endif

#if LED_SUPPORT
#undef BROKER_SUPPORT
#define BROKER_SUPPORT              1               // If LED is enabled enable BROKER to supply status changes
#endif

#if INFLUXDB_SUPPORT
#undef BROKER_SUPPORT
#define BROKER_SUPPORT              1               // If InfluxDB enabled enable BROKER
#endif

#if DOMOTICZ_SUPPORT
#undef BROKER_SUPPORT
#define BROKER_SUPPORT              1               // If Domoticz enabled enable BROKER
#undef MQTT_SUPPORT
#define MQTT_SUPPORT                1               // If Domoticz enabled enable MQTT
#endif

#if HOMEASSISTANT_SUPPORT
#undef MQTT_SUPPORT
#define MQTT_SUPPORT                1               // If Home Assistant enabled enable MQTT
#endif

#if THINGSPEAK_SUPPORT
#undef BROKER_SUPPORT
#define BROKER_SUPPORT              1               // If Thingspeak enabled enable BROKER
#endif

#if THERMOSTAT_SUPPORT
#undef MQTT_USE_JSON
#define MQTT_USE_JSON               1           // Thermostat depends on group messages in a JSON body
#undef RELAY_SUPPORT
#define RELAY_SUPPORT               1           // Thermostat depends on switches
#endif

#if SCHEDULER_SUPPORT
#undef NTP_SUPPORT
#define NTP_SUPPORT                 1           // Scheduler needs NTP to work
#undef BROKER_SUPPORT
#define BROKER_SUPPORT              1           // Scheduler needs Broker to trigger every minute
#undef RELAY_SUPPORT
#define RELAY_SUPPORT               1           // Scheduler needs relays
#endif

#if LWIP_VERSION_MAJOR != 1
#undef MDNS_CLIENT_SUPPORT
#define MDNS_CLIENT_SUPPORT         0          // default resolver already handles this
#endif

#if not defined(ARDUINO_ESP8266_RELEASE_2_3_0)
#undef TELNET_SERVER_ASYNC_BUFFERED
#define TELNET_SERVER_ASYNC_BUFFERED 1         // enable buffered telnet by default on latest Cores
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
#undef RELAY_SUPPORT
#define RELAY_SUPPORT               1           // Most of the time we require it
#endif

//------------------------------------------------------------------------------
// Hint about ESPAsyncTCP options and our internal one
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

#if !DEBUG_SUPPORT
#undef DEBUG_LOG_BUFFER_SUPPORT
#define DEBUG_LOG_BUFFER_SUPPORT  0              // Can't buffer if there is no debugging enabled.
                                                 // Helps to avoid checking twice for both DEBUG_SUPPORT and BUFFER_LOG_SUPPORT
#endif

//------------------------------------------------------------------------------
// These depend on newest Core libraries

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

//------------------------------------------------------------------------------
// Change ntp module depending on Core version

#if NTP_SUPPORT && defined(ARDUINO_ESP8266_RELEASE_2_3_0)
#define NTP_LEGACY_SUPPORT 1
#else
#define NTP_LEGACY_SUPPORT 0
#endif

//------------------------------------------------------------------------------
// When using Dual / Lightfox Dual, notify that Serial should be used

#if (BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_ITEAD_SONOFF_DUAL) || \
    (BUTTON_EVENTS_SOURCE == BUTTON_EVENTS_SOURCE_FOXEL_LIGHTFOX_DUAL)
#if DEBUG_SERIAL_SUPPORT
#warning "DEBUG_SERIAL_SUPPORT conflicts with the current BUTTON_EVENTS_SOURCE"
#undef DEBUG_SERIAL_SUPPORT
#define DEBUG_SERIAL_SUPPORT 0
#endif
#endif

//------------------------------------------------------------------------------
// It looks more natural that one click will enable display
// and long click will switch relay

#if THERMOSTAT_DISPLAY_SUPPORT
#undef BUTTON1_CLICK
#define BUTTON1_CLICK           BUTTON_ACTION_DISPLAY_ON
#undef BUTTON1_LNGCLICK
#define BUTTON1_LNGCLICK        BUTTON_ACTION_TOGGLE
#endif

//------------------------------------------------------------------------------
// We should always set MQTT_MAX_PACKET_SIZE
//

#if MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT
#if not defined(MQTT_MAX_PACKET_SIZE)
#warning "MQTT_MAX_PACKET_SIZE should be set in `build_flags = ...` of the environment! Default value is used instead."
#endif
#endif
