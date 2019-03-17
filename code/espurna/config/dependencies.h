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

#ifndef ASYNC_TCP_SSL_ENABLED
#if THINGSPEAK_USE_SSL && THINGSPEAK_USE_ASYNC
#undef THINGSPEAK_SUPPORT                       
#define THINGSPEAK_SUPPORT          0               // Thingspeak in ASYNC mode requires ASYNC_TCP_SSL_ENABLED
#endif
#endif

#if THINKSPEAK_SUPPORT
#undef BROKER_SUPPORT
#define BROKER_SUPPORT              1               // If Thingspeak enabled enable BROKER
#endif

#if SCHEDULER_SUPPORT
#undef NTP_SUPPORT
#define NTP_SUPPORT                 1           // Scheduler needs NTP
#endif
