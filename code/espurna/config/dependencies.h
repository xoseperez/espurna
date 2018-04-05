#ifndef DEPENDENCIES_H
#define DEPENDENCIES_H

//------------------------------------------------------------------------------
// Do not change this file unless you know what you are doing
// Configuration settings are in the settings.h file
//------------------------------------------------------------------------------


#if DEBUG_TELNET_SUPPORT
#undef TELNET_SUPPORT
#define TELNET_SUPPORT          1
#endif

#ifndef DEBUG_WEB_SUPPORT
#define DEBUG_WEB_SUPPORT       WEB_SUPPORT  // Enable web debug log if web is enabled too
#endif

#if DEBUG_WEB_SUPPORT
#undef WEB_SUPPORT
#define WEB_SUPPORT             1           // Chicken and egg :)
#endif

#if WEB_SUPPORT == 0
#undef SSDP_SUPPORT
#define SSDP_SUPPORT            0           // SSDP support requires web support
#endif

#if UART_MQTT_SUPPORT
#define MQTT_SUPPORT            1
#undef TERMINAL_SUPPORT
#define TERMINAL_SUPPORT        0
#undef DEBUG_SERIAL_SUPPORT
#define DEBUG_SERIAL_SUPPORT    0
#endif

#if DOMOTICZ_SUPPORT
#undef MQTT_SUPPORT
#define MQTT_SUPPORT            1               // If Domoticz enabled enable MQTT
#endif

#if HOMEASSISTANT_SUPPORT
#undef MQTT_SUPPORT
#define MQTT_SUPPORT            1               // If Home Assistant enabled enable MQTT
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



//------------------------------------------------------------------------------
#endif // DEPENDENCIES_H
