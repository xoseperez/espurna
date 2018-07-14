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

#if DOMOTICZ_SUPPORT
#undef MQTT_SUPPORT
#define MQTT_SUPPORT                1               // If Domoticz enabled enable MQTT
#endif

#if HOMEASSISTANT_SUPPORT
#undef MQTT_SUPPORT
#define MQTT_SUPPORT                1               // If Home Assistant enabled enable MQTT
#endif

#ifndef ASYNC_TCP_SSL_ENABLED
#if THINGSPEAK_USE_SSL && THINGSPEAK_USE_ASYNC
#undef THINGSPEAK_SUPPORT                       // Thingspeak in ASYNC mode requires ASYNC_TCP_SSL_ENABLED
#endif
#endif

#if SCHEDULER_SUPPORT
#undef NTP_SUPPORT
#define NTP_SUPPORT                 1           // Scheduler needs NTP
#endif

// -----------------------------------------------------------------------------
// WEB UI IMAGE
// -----------------------------------------------------------------------------

#define WEBUI_IMAGE_SMALL      0
#define WEBUI_IMAGE_LIGHT      1
#define WEBUI_IMAGE_SENSOR     2
#define WEBUI_IMAGE_RFBRIDGE   4
#define WEBUI_IMAGE_RFM69      8
#define WEBUI_IMAGE_FULL       15

#if LIGHT_PROVIDER != LIGHT_PROVIDER_NONE
    #ifdef WEBUI_IMAGE
        #undef WEBUI_IMAGE
        #define WEBUI_IMAGE    WEBUI_IMAGE_FULL
    #else
        #define WEBUI_IMAGE    WEBUI_IMAGE_LIGHT
    #endif
#endif

#if SENSOR_SUPPORT == 1
    #ifndef WEBUI_IMAGE
        #define WEBUI_IMAGE    WEBUI_IMAGE_SENSOR
    #else
        #undef WEBUI_IMAGE
        #define WEBUI_IMAGE    WEBUI_IMAGE_FULL
    #endif
#endif

#if defined(ITEAD_SONOFF_RFBRIDGE)
    #ifndef WEBUI_IMAGE
        #define WEBUI_IMAGE    WEBUI_IMAGE_RFBRIDGE
    #else
        #undef WEBUI_IMAGE
        #define WEBUI_IMAGE    WEBUI_IMAGE_FULL
    #endif
#endif

#if RFM69_SUPPORT == 1
    #ifndef WEBUI_IMAGE
        #define WEBUI_IMAGE    WEBUI_IMAGE_RFM69
    #else
        #undef WEBUI_IMAGE
        #define WEBUI_IMAGE    WEBUI_IMAGE_FULL
    #endif
#endif

#ifndef WEBUI_IMAGE
    #define WEBUI_IMAGE        WEBUI_IMAGE_SMALL
#endif
