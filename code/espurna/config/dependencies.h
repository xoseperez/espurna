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
#undef RELAY_SUPPORT
#define RELAY_SUPPORT               1               // and switches
#endif

#if RPN_RULES_SUPPORT
#undef MQTT_SUPPORT
#define MQTT_SUPPORT                1
#endif

#if DOMOTICZ_SUPPORT
#undef MQTT_SUPPORT
#define MQTT_SUPPORT                1               // If Domoticz enabled enable MQTT
#endif

#if HOMEASSISTANT_SUPPORT
#undef MQTT_SUPPORT
#define MQTT_SUPPORT                1               // If Home Assistant enabled enable MQTT
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
#undef RELAY_SUPPORT
#define RELAY_SUPPORT               1           // Scheduler needs relays
#endif

#if TUYA_SUPPORT
#undef LIGHT_TRANSITION_TIME
#define LIGHT_TRANSITION_TIME       1600       // longer transition than the default
#undef LIGHT_TRANSITION_STEP
#define LIGHT_TRANSITION_STEP       200        // step can't be 10ms since most tuya serial connections are not fast
#undef LIGHT_USE_TRANSITIONS
#define LIGHT_USE_TRANSITIONS       0          // also, disable transitions unless set at runtime
#endif

#if TUYA_SUPPORT
#undef RELAY_SUPPORT
#define RELAY_SUPPORT               1           // Most of the time we require it
#endif

#if TERMINAL_WEB_API_SUPPORT
#undef TERMINAL_SUPPORT
#define TERMINAL_SUPPORT            1           // Need terminal command line parser and commands
#undef WEB_SUPPORT
#define WEB_SUPPORT                 1           // Registered as web server request handler
#endif

#if TERMINAL_MQTT_SUPPORT
#undef TERMINAL_SUPPORT
#define TERMINAL_SUPPORT            1           // Need terminal command line parser and commands
#undef MQTT_SUPPORT
#define MQTT_SUPPORT                1           // Subscribe and publish things
#endif

#if IFAN_SUPPORT
#undef RELAY_SUPPORT
#define RELAY_SUPPORT               1            // Need relays to manage general state
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

#if MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT
#if not defined(MQTT_MAX_PACKET_SIZE)
#warning "MQTT_MAX_PACKET_SIZE should be set in `build_flags = ...` of the environment! Default value is used instead."
#endif
#endif

//------------------------------------------------------------------------------
// Disable BME680 support if using Core version 2.3.0 due to memory constraints.

#if BME680_SUPPORT && defined(ARDUINO_ESP8266_RELEASE_2_3_0)
#warning "BME680_SUPPORT is not available when using Arduino Core 2.3.0 due to memory constraints. Please use Arduino Core 2.6.3+ instead (or set `platform = ${common.platform_latest}` for the latest version)."
#undef BME680_SUPPORT
#define BME680_SUPPORT 0
#endif

//------------------------------------------------------------------------------
// Prometheus needs web server + request handler API

#if PROMETHEUS_SUPPORT
#undef WEB_SUPPORT
#define WEB_SUPPORT 1
#endif

//------------------------------------------------------------------------------
// Analog pin needs ADC_TOUT mode set up at compile time

#if BUTTON_PROVIDER_ANALOG_SUPPORT
#undef ADC_MODE_VALUE
#define ADC_MODE_VALUE ADC_TOUT
#endif
