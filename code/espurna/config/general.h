//------------------------------------------------------------------------------
// Do not change this file unless you know what you are doing
// To override user configuration, please see custom.h
//------------------------------------------------------------------------------

#pragma once

//------------------------------------------------------------------------------
// GENERAL
//------------------------------------------------------------------------------

#ifndef DEVICE_NAME
#define DEVICE_NAME             MANUFACTURER "_" DEVICE     // Concatenate both to get a unique device name
#endif

// When defined, ADMIN_PASS must be 8..63 printable ASCII characters. See:
// https://en.wikipedia.org/wiki/Wi-Fi_Protected_Access#Target_users_(authentication_key_distribution)
// https://github.com/xoseperez/espurna/issues/1151
#ifndef ADMIN_PASS
#define ADMIN_PASS              "fibonacci"     // Default password (WEB, OTA, WIFI SoftAP)
#endif

#ifndef USE_PASSWORD
#define USE_PASSWORD            1               // Insecurity caution! Disabling this will disable password querying completely.
#endif

#ifndef LOOP_DELAY_TIME
#define LOOP_DELAY_TIME         10              // Delay for the main loop, in millis [0-250]
                                                // Recommended minimum is 10, see:
                                                // https://github.com/xoseperez/espurna/issues/1541
                                                // https://github.com/xoseperez/espurna/issues/1631
                                                // https://github.com/esp8266/Arduino/issues/5825
#endif

//------------------------------------------------------------------------------
// DEBUG
//------------------------------------------------------------------------------

#ifndef DEBUG_LOG_MODE
#define DEBUG_LOG_MODE          DebugLogMode::Enabled   // Set global logger mode. One of:
                                                        // ::Enabled, ::Disabled or ::SkipBoot
#endif

// Serial debug log

#ifndef DEBUG_SERIAL_SUPPORT
#define DEBUG_SERIAL_SUPPORT    1               // Enable serial debug log
#endif

#ifndef DEBUG_PORT
#define DEBUG_PORT              Serial          // Default debugging port
#endif

#ifndef SERIAL_BAUDRATE
#define SERIAL_BAUDRATE         115200          // Default baudrate
#endif

#ifndef DEBUG_ADD_TIMESTAMP
#define DEBUG_ADD_TIMESTAMP     1               // Add timestamp to debug messages
                                                // (in millis overflowing every 1000 seconds)
#endif

// Second serial port (used for RX)

#ifndef SERIAL_RX_ENABLED
#define SERIAL_RX_ENABLED       0               // Secondary serial port for RX
#endif

#ifndef SERIAL_RX_PORT
#define SERIAL_RX_PORT          Serial          // This setting is usually defined
                                                // in the hardware.h file for those
                                                // boards that require it
#endif

#ifndef SERIAL_RX_BAUDRATE
#define SERIAL_RX_BAUDRATE      115200          // Default baudrate
#endif

//------------------------------------------------------------------------------

// UDP debug log
// To receive the message son the destination computer use nc:
// nc -ul 8113

#ifndef DEBUG_UDP_SUPPORT
#define DEBUG_UDP_SUPPORT       0               // Enable UDP debug log
#endif

#ifndef DEBUG_UDP_IP
#define DEBUG_UDP_IP            IPAddress(192, 168, 1, 100)
#endif

#ifndef DEBUG_UDP_PORT
#define DEBUG_UDP_PORT          514
#endif

// If DEBUG_UDP_PORT is set to 514 syslog format is assumed
// (https://tools.ietf.org/html/rfc3164)
// DEBUG_UDP_FAC_PRI is the facility+priority
#define DEBUG_UDP_FAC_PRI       (SYSLOG_LOCAL0 | SYSLOG_DEBUG)

//------------------------------------------------------------------------------

#ifndef DEBUG_TELNET_SUPPORT
#define DEBUG_TELNET_SUPPORT    1               // Enable telnet debug log (will only work if TELNET_SUPPORT is also 1)
#endif

//------------------------------------------------------------------------------

#ifndef DEBUG_WEB_SUPPORT
#define DEBUG_WEB_SUPPORT       1               // Enable web debug log (will only work if WEB_SUPPORT is also 1)
#endif

//------------------------------------------------------------------------------

#ifndef DEBUG_LOG_BUFFER_SUPPORT
#define DEBUG_LOG_BUFFER_SUPPORT       1        // Support boot log buffer (1.2Kb)
                                                // Will only work if DEBUG_LOG_BUFFER_ENABLED or runtime setting is also 1
#endif

#ifndef DEBUG_LOG_BUFFER_ENABLED
#define DEBUG_LOG_BUFFER_ENABLED       0        // Disable boot log buffer by default
#endif

#ifndef DEBUG_LOG_BUFFER_SIZE
#define DEBUG_LOG_BUFFER_SIZE          4096     // Store 4 Kb of log strings
                                                // WARNING! Memory is only reclaimed after `debug.buffer` prints the buffer contents
#endif

//------------------------------------------------------------------------------
// TELNET
//------------------------------------------------------------------------------

#ifndef TELNET_SUPPORT
#define TELNET_SUPPORT          1               // Enable telnet support by default (3.34Kb)
#endif

#ifndef TELNET_STA
#define TELNET_STA              0               // By default, disallow connections via STA interface
#endif

#ifndef TELNET_AUTHENTICATION
#define TELNET_AUTHENTICATION   1               // Request password to start telnet session by default
#endif

#ifndef TELNET_PORT
#define TELNET_PORT             23              // Port to listen to telnet clients
#endif

#ifndef TELNET_MAX_CLIENTS
#define TELNET_MAX_CLIENTS      1               // Max number of concurrent telnet clients
#endif

#ifndef TELNET_SERVER
#define TELNET_SERVER           TELNET_SERVER_ASYNC // Can be either TELNET_SERVER_ASYNC (using ESPAsyncTCP) or TELNET_SERVER_WIFISERVER (using WiFiServer)
#endif

#ifndef TELNET_SERVER_ASYNC_BUFFERED
#define TELNET_SERVER_ASYNC_BUFFERED         0  // Enable buffered output for telnet server (+1Kb)
                                                // Helps to avoid lost data with lwip2 TCP_MSS=536 option
#endif

// Enable this flag to add support for reverse telnet (+800 bytes)
// This is useful to telnet to a device behind a NAT or firewall
// To use this feature, start a listen server on a publicly reachable host with e.g. "ncat -vlp <port>" and use the MQTT reverse telnet command to connect
#ifndef TELNET_REVERSE_SUPPORT
#define TELNET_REVERSE_SUPPORT  0
#endif

//------------------------------------------------------------------------------
// TERMINAL
//------------------------------------------------------------------------------

#ifndef TERMINAL_SUPPORT
#define TERMINAL_SUPPORT         1              // Enable terminal commands (0.97Kb)
#endif

#define TERMINAL_BUFFER_SIZE     128            // Max size for commands commands

//------------------------------------------------------------------------------
// SYSTEM CHECK
//------------------------------------------------------------------------------

#ifndef SYSTEM_CHECK_ENABLED
#define SYSTEM_CHECK_ENABLED    1               // Enable crash check by default
#endif

#ifndef SYSTEM_CHECK_TIME
#define SYSTEM_CHECK_TIME       60000           // The system is considered stable after these many millis
#endif

#ifndef SYSTEM_CHECK_MAX
#define SYSTEM_CHECK_MAX        5               // After this many crashes on boot
                                                // the system is flagged as unstable
#endif

//------------------------------------------------------------------------------
// EEPROM
//------------------------------------------------------------------------------

#define EEPROM_SIZE             SPI_FLASH_SEC_SIZE  // EEPROM size in bytes (1 sector = 4096 bytes)

//#define EEPROM_RORATE_SECTORS   2             // Number of sectors to use for EEPROM rotation
                                                // If not defined the firmware will use a number based
                                                // on the number of available sectors

#define EEPROM_RELAY_STATUS     0               // Address for the relay status (1 byte)
#define EEPROM_ENERGY_COUNT     1               // Address for the energy counter (4 bytes)
#define EEPROM_CUSTOM_RESET     5               // Address for the reset reason (1 byte)
#define EEPROM_CRASH_COUNTER    6               // Address for the crash counter (1 byte)
#define EEPROM_MESSAGE_ID       7               // Address for the MQTT message id (4 bytes)
#define EEPROM_ROTATE_DATA      11              // Reserved for the EEPROM_ROTATE library (3 bytes)
#define EEPROM_DATA_END         14              // End of custom EEPROM data block


#ifndef SAVE_CRASH_ENABLED
#define SAVE_CRASH_ENABLED          1           // Save stack trace to EEPROM by default
                                                // Depends on DEBUG_SUPPORT == 1
#endif

#ifndef SAVE_CRASH_STACK_TRACE_MAX
#define SAVE_CRASH_STACK_TRACE_MAX  0x80        // limit at 128 bytes (increment/decrement by 16)
#endif

//------------------------------------------------------------------------------
// THERMOSTAT
//------------------------------------------------------------------------------

#ifndef THERMOSTAT_SUPPORT
#define THERMOSTAT_SUPPORT          0
#endif

#ifndef THERMOSTAT_DISPLAY_SUPPORT
#define THERMOSTAT_DISPLAY_SUPPORT  0
#endif

#ifndef THERMOSTAT_DISPLAY_OFF_INTERVAL         // Interval in seconds after which display will be switched off
#define THERMOSTAT_DISPLAY_OFF_INTERVAL  0      // This will prevent it from burnout
#endif                                          // 0 - newer switch display off

#define THERMOSTAT_SERVER_LOST_INTERVAL  120000 //server means lost after 2 min from last response
#define THERMOSTAT_REMOTE_TEMP_MAX_WAIT     120 // 2 min

#ifndef THERMOSTAT_REMOTE_SENSOR_NAME
#define THERMOSTAT_REMOTE_SENSOR_NAME        "" // Get remote temp(hum) from mqtt topic of this device
#endif

//------------------------------------------------------------------------------
// HEARTBEAT
//------------------------------------------------------------------------------

#define HEARTBEAT_NONE              0           // Never send heartbeat
#define HEARTBEAT_ONCE              1           // Send it only once upon MQTT connection
#define HEARTBEAT_REPEAT            2           // Send it upon MQTT connection and every HEARTBEAT_INTERVAL
#define HEARTBEAT_REPEAT_STATUS     3           // Send it upon MQTT connection and every HEARTBEAT_INTERVAL only STATUS report

// Backwards compatibility check
#if defined(HEARTBEAT_ENABLED) && (HEARTBEAT_ENABLED == 0)
#define HEARTBEAT_MODE              HEARTBEAT_NONE
#endif

#ifndef HEARTBEAT_MODE
#define HEARTBEAT_MODE              HEARTBEAT_REPEAT
#endif

#ifndef HEARTBEAT_INTERVAL
#define HEARTBEAT_INTERVAL          300UL         // Interval between heartbeat messages (in sec)
#endif

#define UPTIME_OVERFLOW             4294967295UL  // Uptime overflow value

// Values that will be reported in heartbeat
#ifndef HEARTBEAT_REPORT_STATUS
#define HEARTBEAT_REPORT_STATUS     1
#endif

#ifndef HEARTBEAT_REPORT_SSID
#define HEARTBEAT_REPORT_SSID       1
#endif

#ifndef HEARTBEAT_REPORT_IP
#define HEARTBEAT_REPORT_IP         1
#endif

#ifndef HEARTBEAT_REPORT_MAC
#define HEARTBEAT_REPORT_MAC        1
#endif

#ifndef HEARTBEAT_REPORT_RSSI
#define HEARTBEAT_REPORT_RSSI       1
#endif

#ifndef HEARTBEAT_REPORT_UPTIME
#define HEARTBEAT_REPORT_UPTIME     1
#endif

#ifndef HEARTBEAT_REPORT_DATETIME
#define HEARTBEAT_REPORT_DATETIME   1
#endif

#ifndef HEARTBEAT_REPORT_FREEHEAP
#define HEARTBEAT_REPORT_FREEHEAP   1
#endif

#ifndef HEARTBEAT_REPORT_VCC
#define HEARTBEAT_REPORT_VCC        1
#endif

#ifndef HEARTBEAT_REPORT_RELAY
#define HEARTBEAT_REPORT_RELAY      1
#endif

#ifndef HEARTBEAT_REPORT_LIGHT
#define HEARTBEAT_REPORT_LIGHT      1
#endif

#ifndef HEARTBEAT_REPORT_HOSTNAME
#define HEARTBEAT_REPORT_HOSTNAME   1
#endif

#ifndef HEARTBEAT_REPORT_DESCRIPTION
#define HEARTBEAT_REPORT_DESCRIPTION 1
#endif

#ifndef HEARTBEAT_REPORT_APP
#define HEARTBEAT_REPORT_APP        1
#endif

#ifndef HEARTBEAT_REPORT_VERSION
#define HEARTBEAT_REPORT_VERSION    1
#endif

#ifndef HEARTBEAT_REPORT_BOARD
#define HEARTBEAT_REPORT_BOARD      1
#endif

#ifndef HEARTBEAT_REPORT_LOADAVG
#define HEARTBEAT_REPORT_LOADAVG    1
#endif

#ifndef HEARTBEAT_REPORT_INTERVAL
#define HEARTBEAT_REPORT_INTERVAL   0
#endif

#ifndef HEARTBEAT_REPORT_RANGE
#define HEARTBEAT_REPORT_RANGE         THERMOSTAT_SUPPORT
#endif

#ifndef HEARTBEAT_REPORT_REMOTE_TEMP
#define HEARTBEAT_REPORT_REMOTE_TEMP   THERMOSTAT_SUPPORT
#endif

#ifndef HEARTBEAT_REPORT_BSSID
#define HEARTBEAT_REPORT_BSSID       0
#endif

//------------------------------------------------------------------------------
// Load average
//------------------------------------------------------------------------------

#ifndef LOADAVG_INTERVAL
#define LOADAVG_INTERVAL        30000           // Interval between calculating load average (in ms)
#endif

//------------------------------------------------------------------------------
// BUTTON
//------------------------------------------------------------------------------

#ifndef BUTTON_SUPPORT
#define BUTTON_SUPPORT              1
#endif

#ifndef BUTTON_DEBOUNCE_DELAY
#define BUTTON_DEBOUNCE_DELAY       50          // Debounce delay (ms)
#endif

#ifndef BUTTON_REPEAT_DELAY
#define BUTTON_REPEAT_DELAY         500         // Time in ms to wait for a second (or third...) click
#endif

#ifndef BUTTON_LNGCLICK_DELAY
#define BUTTON_LNGCLICK_DELAY       1000        // Time in ms holding the button down to get a long click
#endif

#ifndef BUTTON_LNGLNGCLICK_DELAY
#define BUTTON_LNGLNGCLICK_DELAY    10000       // Time in ms holding the button down to get a long-long click
#endif

#ifndef BUTTON_MQTT_SEND_ALL_EVENTS
#define BUTTON_MQTT_SEND_ALL_EVENTS     0           // 0 - to send only events the are bound to actions
                                                   // 1 - to send all button events to MQTT
#endif

#ifndef BUTTON_MQTT_RETAIN
#define BUTTON_MQTT_RETAIN              0
#endif

#ifndef BUTTON_EVENTS_SOURCE
#define BUTTON_EVENTS_SOURCE            BUTTON_EVENTS_SOURCE_GENERIC   // Type of button event source. One of:
                                                                       // BUTTON_EVENTS_SOURCE_GENERIC - GPIOs (virtual or real)
                                                                       // BUTTON_EVENTS_SOURCE_SONOFF_DUAL - hardware specific, drive buttons through serial connection
                                                                       // BUTTON_EVENTS_SOURCE_FOXEL_LIGHTFOX_DUAL - similar to Itead Sonoff Dual, hardware specific
#endif

//------------------------------------------------------------------------------
// ENCODER
//------------------------------------------------------------------------------

#ifndef ENCODER_SUPPORT
#define ENCODER_SUPPORT             0
#endif

#ifndef ENCODER_MINIMUM_DELTA
#define ENCODER_MINIMUM_DELTA       1
#endif

//------------------------------------------------------------------------------
// LED
//------------------------------------------------------------------------------

#ifndef LED_SUPPORT
#define LED_SUPPORT                 1
#endif

//------------------------------------------------------------------------------
// RELAY
//------------------------------------------------------------------------------

#ifndef RELAY_SUPPORT
#define RELAY_SUPPORT               1
#endif

// Default boot mode: 0 means OFF, 1 ON and 2 whatever was before
#ifndef RELAY_BOOT_MODE
#define RELAY_BOOT_MODE             RELAY_BOOT_OFF
#endif

// 0 means ANY, 1 zero or one and 2 one and only one
#ifndef RELAY_SYNC
#define RELAY_SYNC                  RELAY_SYNC_ANY
#endif

// Default pulse mode: 0 means no pulses, 1 means normally off, 2 normally on
#ifndef RELAY_PULSE_MODE
#define RELAY_PULSE_MODE            RELAY_PULSE_NONE
#endif

// Default pulse time in seconds
#ifndef RELAY_PULSE_TIME
#define RELAY_PULSE_TIME            1.0
#endif

// Relay requests flood protection window - in seconds
#ifndef RELAY_FLOOD_WINDOW
#define RELAY_FLOOD_WINDOW          3.0
#endif

// Allowed actual relay changes inside requests flood protection window
#ifndef RELAY_FLOOD_CHANGES
#define RELAY_FLOOD_CHANGES         5
#endif

// Pulse with in milliseconds for a latched relay
#ifndef RELAY_LATCHING_PULSE
#define RELAY_LATCHING_PULSE        10
#endif

// Do not save relay state after these many milliseconds
#ifndef RELAY_SAVE_DELAY
#define RELAY_SAVE_DELAY            1000
#endif

#ifndef RELAY_REPORT_STATUS
#define RELAY_REPORT_STATUS         1
#endif

// Configure the MQTT payload for ON, OFF and TOGGLE
#ifndef RELAY_MQTT_OFF
#define RELAY_MQTT_OFF              "0"
#endif

#ifndef RELAY_MQTT_ON
#define RELAY_MQTT_ON               "1"
#endif

#ifndef RELAY_MQTT_TOGGLE
#define RELAY_MQTT_TOGGLE           "2"
#endif

// -----------------------------------------------------------------------------
// WIFI
// -----------------------------------------------------------------------------

#ifndef WIFI_CONNECT_TIMEOUT
#define WIFI_CONNECT_TIMEOUT        60000               // Connecting timeout for WIFI in ms
#endif

#ifndef WIFI_RECONNECT_INTERVAL
#define WIFI_RECONNECT_INTERVAL     180000              // If could not connect to WIFI, retry after this time in ms
#endif

#ifndef WIFI_MAX_NETWORKS
#define WIFI_MAX_NETWORKS           5                   // Max number of WIFI connection configurations
#endif

#ifndef WIFI_AP_CAPTIVE
#define WIFI_AP_CAPTIVE             1                   // Captive portal enabled when in AP mode
#endif

#ifndef WIFI_FALLBACK_APMODE
#define WIFI_FALLBACK_APMODE        1                   // Fallback to AP mode if no STA connection
#endif

#ifndef WIFI_SLEEP_MODE
#define WIFI_SLEEP_MODE             WIFI_NONE_SLEEP     // WIFI_NONE_SLEEP, WIFI_LIGHT_SLEEP or WIFI_MODEM_SLEEP
#endif

#ifndef WIFI_SCAN_NETWORKS
#define WIFI_SCAN_NETWORKS          1                   // Perform a network scan before connecting
#endif

// Optional hardcoded configuration (up to 5 networks, depending on WIFI_MAX_NETWORKS and espurna/wifi_config.h)
#ifndef WIFI1_SSID
#define WIFI1_SSID                  ""
#endif

#ifndef WIFI1_PASS
#define WIFI1_PASS                  ""
#endif

#ifndef WIFI1_IP
#define WIFI1_IP                    ""
#endif

#ifndef WIFI1_GW
#define WIFI1_GW                    ""
#endif

#ifndef WIFI1_MASK
#define WIFI1_MASK                  ""
#endif

#ifndef WIFI1_DNS
#define WIFI1_DNS                   ""
#endif

#ifndef WIFI2_SSID
#define WIFI2_SSID                  ""
#endif

#ifndef WIFI2_PASS
#define WIFI2_PASS                  ""
#endif

#ifndef WIFI2_IP
#define WIFI2_IP                    ""
#endif

#ifndef WIFI2_GW
#define WIFI2_GW                    ""
#endif

#ifndef WIFI2_MASK
#define WIFI2_MASK                  ""
#endif

#ifndef WIFI2_DNS
#define WIFI2_DNS                   ""
#endif

#ifndef WIFI3_SSID
#define WIFI3_SSID                  ""
#endif

#ifndef WIFI3_PASS
#define WIFI3_PASS                  ""
#endif

#ifndef WIFI3_IP
#define WIFI3_IP                    ""
#endif

#ifndef WIFI3_GW
#define WIFI3_GW                    ""
#endif

#ifndef WIFI3_MASK
#define WIFI3_MASK                  ""
#endif

#ifndef WIFI3_DNS
#define WIFI3_DNS                   ""
#endif

#ifndef WIFI4_SSID
#define WIFI4_SSID                  ""
#endif

#ifndef WIFI4_PASS
#define WIFI4_PASS                  ""
#endif

#ifndef WIFI4_IP
#define WIFI4_IP                    ""
#endif

#ifndef WIFI4_GW
#define WIFI4_GW                    ""
#endif

#ifndef WIFI4_MASK
#define WIFI4_MASK                  ""
#endif

#ifndef WIFI4_DNS
#define WIFI4_DNS                   ""
#endif

#ifndef WIFI5_SSID
#define WIFI5_SSID                  ""
#endif

#ifndef WIFI5_PASS
#define WIFI5_PASS                  ""
#endif

#ifndef WIFI5_IP
#define WIFI5_IP                    ""
#endif

#ifndef WIFI5_GW
#define WIFI5_GW                    ""
#endif

#ifndef WIFI5_MASK
#define WIFI5_MASK                  ""
#endif

#ifndef WIFI5_DNS
#define WIFI5_DNS                   ""
#endif

#ifndef WIFI_RSSI_1M
#define WIFI_RSSI_1M                -30         // Calibrate it with your router reading the RSSI at 1m
#endif

#ifndef WIFI_PROPAGATION_CONST
#define WIFI_PROPAGATION_CONST      4           // This is typically something between 2.7 to 4.3 (free space is 2)
#endif

// ref: https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/kconfig.html#config-lwip-esp-gratuitous-arp
// ref: https://github.com/xoseperez/espurna/pull/1877#issuecomment-525612546 
//
// Broadcast gratuitous ARP periodically to update ARP tables on the AP and all devices on the same network.
// Helps to solve compatibility issues when ESP fails to timely reply to ARP requests, causing the device's ARP table entry to expire.

#ifndef WIFI_GRATUITOUS_ARP_SUPPORT
#define WIFI_GRATUITOUS_ARP_SUPPORT              1
#endif

// Interval is randomized on each boot in range from ..._MIN to ..._MAX (ms)
#ifndef WIFI_GRATUITOUS_ARP_INTERVAL_MIN
#define WIFI_GRATUITOUS_ARP_INTERVAL_MIN         15000
#endif

#ifndef WIFI_GRATUITOUS_ARP_INTERVAL_MAX
#define WIFI_GRATUITOUS_ARP_INTERVAL_MAX         30000
#endif

// ref: https://github.com/esp8266/Arduino/issues/6471
// ref: https://github.com/esp8266/Arduino/issues/6366
//
// Issue #6366 turned out to be high tx power causing weird behavior. Lowering tx power achieved stability.
#ifndef WIFI_OUTPUT_POWER_DBM
#define WIFI_OUTPUT_POWER_DBM                    20.0f
#endif


// -----------------------------------------------------------------------------
// WEB
// -----------------------------------------------------------------------------

#ifndef WEB_SUPPORT
#define WEB_SUPPORT                 1           // Enable web support (http, api, 121.65Kb)
#endif

#ifndef WEB_EMBEDDED
#define WEB_EMBEDDED                1           // Build the firmware with the web interface embedded in
#endif

// Requires ESPAsyncTCP to be built with ASYNC_TCP_SSL_ENABLED=1 and Arduino Core version >= 2.4.0
// XXX: This is not working at the moment!! Pending https://github.com/me-no-dev/ESPAsyncTCP/issues/95
#ifndef WEB_SSL_ENABLED
#define WEB_SSL_ENABLED             0           // Use HTTPS web interface
#endif

#ifndef WEB_USERNAME
#define WEB_USERNAME                "admin"     // HTTP username
#endif

#ifndef WEB_FORCE_PASS_CHANGE
#define WEB_FORCE_PASS_CHANGE       1           // Force the user to change the password if default one
#endif

#ifndef WEB_PORT
#define WEB_PORT                    80          // HTTP port
#endif

// Defining a WEB_REMOTE_DOMAIN will enable Cross-Origin Resource Sharing (CORS)
// so you will be able to login to this device from another domain. This will allow
// you to manage all ESPurna devices in your local network from a unique installation
// of the web UI. This installation could be in a local server (a Raspberry Pi, for instance)
// or in the Internet. Since the WebUI is just one compressed file with HTML, CSS and JS
// there are no special requirements. Any static web server will do (NGinx, Apache, Lighttpd,...).
// The only requirement is that the resource must be available under this domain.
#ifndef WEB_REMOTE_DOMAIN
#define WEB_REMOTE_DOMAIN           "http://espurna.io"
#endif

// -----------------------------------------------------------------------------
// WEBSOCKETS
// -----------------------------------------------------------------------------

// This will only be enabled if WEB_SUPPORT is 1 (this is the default value)
#ifndef WS_AUTHENTICATION
#define WS_AUTHENTICATION           1           // WS authentication ON by default (see #507)
#endif

#ifndef WS_BUFFER_SIZE
#define WS_BUFFER_SIZE              5           // Max number of secured websocket connections
#endif

#ifndef WS_TIMEOUT
#define WS_TIMEOUT                  1800000     // Timeout for secured websocket
#endif

#ifndef WS_UPDATE_INTERVAL
#define WS_UPDATE_INTERVAL          30000       // Update clients every 30 seconds
#endif

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

#ifndef API_SUPPORT
#define API_SUPPORT                 1           // API (REST & RPC) support built in
#endif

// This will only be enabled if WEB_SUPPORT is 1 (this is the default value)
#ifndef API_ENABLED
#define API_ENABLED                 0           // Do not enable API by default
#endif

#ifndef API_KEY
#define API_KEY                     ""          // Do not enable API by default. WebUI will automatically generate the key
#endif

#ifndef API_RESTFUL
#define API_RESTFUL                 1           // A restful API requires changes to be issued as PUT requests
                                                // Setting this to 0 will allow using GET to change relays, for instance
#endif

#ifndef API_BUFFER_SIZE
#define API_BUFFER_SIZE             64          // Size of the buffer for HTTP GET API responses
#endif

#ifndef API_REAL_TIME_VALUES
#define API_REAL_TIME_VALUES        0           // Show filtered/median values by default (0 => median, 1 => real time)
#endif


// -----------------------------------------------------------------------------
// MDNS / LLMNR / NETBIOS / SSDP
// -----------------------------------------------------------------------------

#ifndef MDNS_SERVER_SUPPORT
#define MDNS_SERVER_SUPPORT         1           // Publish services using mDNS by default (1.48Kb)
#endif

#ifndef MDNS_CLIENT_SUPPORT
#define MDNS_CLIENT_SUPPORT         0           // Resolve mDNS names (3.44Kb)
#endif

#ifndef LLMNR_SUPPORT
#define LLMNR_SUPPORT               0           // Publish device using LLMNR protocol by default (1.95Kb) - requires Core version >= 2.4.0
#endif

#ifndef NETBIOS_SUPPORT
#define NETBIOS_SUPPORT             0           // Publish device using NetBIOS protocol by default (1.26Kb) - requires Core version >= 2.4.0
#endif

#ifndef SSDP_SUPPORT
#define SSDP_SUPPORT                0           // Publish device using SSDP protocol by default (4.59Kb)
                                                // Not compatible with ALEXA_SUPPORT at the moment
#endif

#ifndef SSDP_DEVICE_TYPE
#define SSDP_DEVICE_TYPE            "upnp:rootdevice"
//#define SSDP_DEVICE_TYPE            "urn:schemas-upnp-org:device:BinaryLight:1"
#endif


// -----------------------------------------------------------------------------
// SPIFFS
// -----------------------------------------------------------------------------

#ifndef SPIFFS_SUPPORT
#define SPIFFS_SUPPORT              0           // Do not add support for SPIFFS by default
#endif

// -----------------------------------------------------------------------------
// SSL Client                                                 ** EXPERIMENTAL **
// -----------------------------------------------------------------------------

#ifndef SECURE_CLIENT
#define SECURE_CLIENT                          SECURE_CLIENT_NONE     // What variant of WiFiClient to use
                                                                      // SECURE_CLIENT_NONE    - No secure client support (default)
                                                                      // SECURE_CLIENT_AXTLS   - axTLS client secure support (All Core versions, ONLY TLS 1.1)
                                                                      // SECURE_CLIENT_BEARSSL - BearSSL client secure support (starting with 2.5.0, TLS 1.2)
                                                                      //
                                                                      // axTLS marked for derecation since Arduino Core 2.4.2 and **will** be removed in the future
#endif

// Security check that is performed when the connection is established:
// SECURE_CLIENT_CHECK_CA           - Use Trust Anchor / Root Certificate
//                                    Supported only by the SECURE_CLIENT_BEARSSL
//                                    (See respective ..._SECURE_CLIENT_INCLUDE_CA options per-module)
// SECURE_CLIENT_CHECK_FINGERPRINT  - Check certificate fingerprint
// SECURE_CLIENT_CHECK_NONE         - Allow insecure connections

#ifndef SECURE_CLIENT_CHECK

#if SECURE_CLIENT == SECURE_CLIENT_BEARSSL
#define SECURE_CLIENT_CHECK                    SECURE_CLIENT_CHECK_CA

#else
#define SECURE_CLIENT_CHECK                    SECURE_CLIENT_CHECK_FINGERPRINT

#endif


#endif // SECURE_CLIENT_CHECK

// Support Maximum Fragment Length Negotiation TLS extension
// "...negotiate a smaller maximum fragment length due to memory limitations or bandwidth limitations."
// - https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/bearssl-client-secure-class.html#mfln-or-maximum-fragment-length-negotiation-saving-ram
// - https://tools.ietf.org/html/rfc6066#section-4
#ifndef SECURE_CLIENT_MFLN
#define SECURE_CLIENT_MFLN                     0                      // The only possible values are: 512, 1024, 2048 and 4096
                                                                      // Set to 0 to disable (default)
#endif

// -----------------------------------------------------------------------------
// OTA
// -----------------------------------------------------------------------------

#ifndef OTA_PORT
#define OTA_PORT                    8266        // Port for ArduinoOTA
#endif

#ifndef OTA_MQTT_SUPPORT
#define OTA_MQTT_SUPPORT            0           // Listen for HTTP(s) URLs at '<root topic>/ota'. Depends on OTA_CLIENT
#endif

#ifndef OTA_ARDUINOOTA_SUPPORT
#define OTA_ARDUINOOTA_SUPPORT      1           // Support ArduinoOTA by default (4.2Kb)
                                                // Implicitly depends on ESP8266mDNS library, thus increasing firmware size
#endif

#ifndef OTA_CLIENT
#define OTA_CLIENT                  OTA_CLIENT_ASYNCTCP     // Terminal / MQTT OTA support
                                                            // OTA_CLIENT_ASYNCTCP   (ESPAsyncTCP library)
                                                            // OTA_CLIENT_HTTPUPDATE (Arduino Core library)j
                                                            // OTA_CLIENT_NONE to disable
#endif

#ifndef OTA_WEB_SUPPORT
#define OTA_WEB_SUPPORT          1              // Support `/upgrade` endpoint and WebUI OTA handler
#endif

#define OTA_GITHUB_FP               "CA:06:F5:6B:25:8B:7A:0D:4F:2B:05:47:09:39:47:86:51:15:19:84"

#ifndef OTA_FINGERPRINT
#define OTA_FINGERPRINT             OTA_GITHUB_FP
#endif

#ifndef OTA_SECURE_CLIENT_CHECK
#define OTA_SECURE_CLIENT_CHECK                SECURE_CLIENT_CHECK
#endif

#ifndef OTA_SECURE_CLIENT_MFLN
#define OTA_SECURE_CLIENT_MFLN                 SECURE_CLIENT_MFLN
#endif

#ifndef OTA_SECURE_CLIENT_INCLUDE_CA
#define OTA_SECURE_CLIENT_INCLUDE_CA        0            // Use user-provided CA. Only PROGMEM PEM option is supported.
                                                         // TODO: eventually should be replaced with pre-parsed structs, read directly from flash
                                                         // (ref: https://github.com/earlephilhower/bearssl-esp8266/pull/14)
                                                         //
                                                         // When enabled, current implementation includes "static/ota_client_trusted_root_ca.h" with
                                                         // const char _ota_client_trusted_root_ca[] PROGMEM = "...PEM data...";
                                                         // By default, using DigiCert root in "static/digicert_evroot_pem.h" (for https://github.com)
#endif


// -----------------------------------------------------------------------------
// NOFUSS
// -----------------------------------------------------------------------------

#ifndef NOFUSS_SUPPORT
#define NOFUSS_SUPPORT              0          // Do not enable support for NoFuss by default (12.65Kb)
#endif

#ifndef NOFUSS_ENABLED
#define NOFUSS_ENABLED              0           // Do not perform NoFUSS updates by default
#endif

#ifndef NOFUSS_SERVER
#define NOFUSS_SERVER               ""          // Default NoFuss Server
#endif

#ifndef NOFUSS_INTERVAL
#define NOFUSS_INTERVAL             3600000     // Check for updates every hour
#endif

// -----------------------------------------------------------------------------
// UART <-> MQTT
// -----------------------------------------------------------------------------

#ifndef UART_MQTT_SUPPORT
#define UART_MQTT_SUPPORT           0           // No support by default
#endif

#ifndef UART_MQTT_USE_SOFT
#define UART_MQTT_USE_SOFT          0           // Use SoftwareSerial
#endif

#ifndef UART_MQTT_HW_PORT
#define UART_MQTT_HW_PORT           Serial      // Hardware serial port (if UART_MQTT_USE_SOFT == 0)
#endif

#ifndef UART_MQTT_RX_PIN
#define UART_MQTT_RX_PIN            4           // RX PIN (if UART_MQTT_USE_SOFT == 1)
#endif

#ifndef UART_MQTT_TX_PIN
#define UART_MQTT_TX_PIN            5           // TX PIN (if UART_MQTT_USE_SOFT == 1)
#endif

#ifndef UART_MQTT_BAUDRATE
#define UART_MQTT_BAUDRATE          115200      // Serial speed
#endif

#ifndef UART_MQTT_TERMINATION
#define UART_MQTT_TERMINATION      '\n'         // Termination character
#endif

#define UART_MQTT_BUFFER_SIZE       100         // UART buffer size

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

#ifndef MQTT_SUPPORT
#define MQTT_SUPPORT                1           // MQTT support (22.38Kb async, 12.48Kb sync)
#endif


#ifndef MQTT_LIBRARY
#define MQTT_LIBRARY                MQTT_LIBRARY_ASYNCMQTTCLIENT       // MQTT_LIBRARY_ASYNCMQTTCLIENT (default, https://github.com/marvinroger/async-mqtt-client)
                                                                       // MQTT_LIBRARY_PUBSUBCLIENT (https://github.com/knolleary/pubsubclient)
                                                                       // MQTT_LIBRARY_ARDUINOMQTT (https://github.com/256dpi/arduino-mqtt)
#endif

// -----------------------------------------------------------------------------
// MQTT OVER SSL
// -----------------------------------------------------------------------------
//
// Requires SECURE_CLIENT set to SECURE_CLIENT_AXTLS or SECURE_CLIENT_BEARSSL
// It is recommended to use MQTT_LIBRARY_ARDUINOMQTT or MQTT_LIBRARY_PUBSUBCLIENT
// It is recommended to use SECURE_CLIENT_BEARSSL
// It is recommended to use ESP8266 Arduino Core >= 2.5.2 with SECURE_CLIENT_BEARSSL
//
// Current version of MQTT_LIBRARY_ASYNCMQTTCLIENT only supports SECURE_CLIENT_AXTLS
//
// It is recommended to use WEB_SUPPORT=0 with either SECURE_CLIENT option, as there are miscellaneous problems when using them simultaneously
// (although, things might've improved, and I'd encourage to check whether this is true or not)
//
// When using MQTT_LIBRARY_PUBSUBCLIENT or MQTT_LIBRARY_ARDUINOMQTT, you will have to disable every module that uses ESPAsyncTCP:
// ALEXA_SUPPORT=0, INFLUXDB_SUPPORT=0, TELNET_SUPPORT=0, THINGSPEAK_SUPPORT=0, DEBUG_TELNET_SUPPORT=0 and WEB_SUPPORT=0
// Or, use "sync" versions instead (note that not every module has this option):
// THINGSPEAK_USE_ASYNC=0, TELNET_SERVER=TELNET_SERVER_WIFISERVER
//
// See SECURE_CLIENT_CHECK for all possible connection verification options.
//
// The simpliest way to verify SSL connection is to use fingerprinting.
// For example, to get Google's MQTT server certificate fingerprint, run the following command:
// $ echo -n | openssl s_client -connect mqtt.googleapis.com:8883 2>&1 | openssl x509 -noout -fingerprint -sha1 | cut -d\= -f2
// Note that fingerprint will change when certificate changes e.g. LetsEncrypt renewals or when the CSR updates

#ifndef MQTT_SSL_ENABLED
#define MQTT_SSL_ENABLED            0               // By default MQTT over SSL will not be enabled
#endif

#ifndef MQTT_SSL_FINGERPRINT
#define MQTT_SSL_FINGERPRINT        ""              // SSL fingerprint of the server
#endif

#ifndef MQTT_SECURE_CLIENT_CHECK
#define MQTT_SECURE_CLIENT_CHECK    SECURE_CLIENT_CHECK // Use global verification setting by default
#endif

#ifndef MQTT_SECURE_CLIENT_MFLN
#define MQTT_SECURE_CLIENT_MFLN     SECURE_CLIENT_MFLN  // Use global MFLN setting by default 
#endif

#ifndef MQTT_SECURE_CLIENT_INCLUDE_CA
#define MQTT_SECURE_CLIENT_INCLUDE_CA        0           // Use user-provided CA. Only PROGMEM PEM option is supported.
                                                         // When enabled, current implementation includes "static/mqtt_client_trusted_root_ca.h" with
                                                         // const char _mqtt_client_trusted_root_ca[] PROGMEM = "...PEM data...";
                                                         // By default, using LetsEncrypt X3 root in "static/letsencrypt_isrgroot_pem.h"
#endif

#ifndef MQTT_ENABLED
#define MQTT_ENABLED                0               // Do not enable MQTT connection by default
#endif

#ifndef MQTT_AUTOCONNECT
#define MQTT_AUTOCONNECT            1               // If enabled and MDNS_SERVER_SUPPORT=1 will perform an autodiscover and
                                                    // autoconnect to the first MQTT broker found if none defined
#endif

#ifndef MQTT_SERVER
#define MQTT_SERVER                 ""              // Default MQTT broker address
#endif

#ifndef MQTT_USER
#define MQTT_USER                   ""              // Default MQTT broker usename
#endif

#ifndef MQTT_PASS
#define MQTT_PASS                   ""              // Default MQTT broker password
#endif

#ifndef MQTT_PORT
#define MQTT_PORT                   1883            // MQTT broker port
#endif

#ifndef MQTT_TOPIC
#define MQTT_TOPIC                  "{hostname}"    // Default MQTT base topic
#endif

#ifndef MQTT_RETAIN
#define MQTT_RETAIN                 true            // MQTT retain flag
#endif

#ifndef MQTT_QOS
#define MQTT_QOS                    0               // MQTT QoS value for all messages
#endif

#ifndef MQTT_KEEPALIVE
#define MQTT_KEEPALIVE              120             // MQTT keepalive value
#endif


#ifndef MQTT_RECONNECT_DELAY_MIN
#define MQTT_RECONNECT_DELAY_MIN    5000            // Try to reconnect in 5 seconds upon disconnection
#endif

#ifndef MQTT_RECONNECT_DELAY_STEP
#define MQTT_RECONNECT_DELAY_STEP   5000            // Increase the reconnect delay in 5 seconds after each failed attempt
#endif

#ifndef MQTT_RECONNECT_DELAY_MAX
#define MQTT_RECONNECT_DELAY_MAX    120000          // Set reconnect time to 2 minutes at most
#endif


#ifndef MQTT_SKIP_RETAINED
#define MQTT_SKIP_RETAINED          1               // Skip retained messages on connection
#endif

#ifndef MQTT_SKIP_TIME
#define MQTT_SKIP_TIME              1000            // Skip messages for 1 second anter connection
#endif

#ifndef MQTT_USE_JSON
#define MQTT_USE_JSON               0               // Don't group messages in a JSON body by default
#endif

#ifndef MQTT_USE_JSON_DELAY
#define MQTT_USE_JSON_DELAY         100             // Wait this many ms before grouping messages
#endif

#ifndef MQTT_QUEUE_MAX_SIZE
#define MQTT_QUEUE_MAX_SIZE         20              // Size of the MQTT queue when MQTT_USE_JSON is enabled
#endif

#ifndef MQTT_BUFFER_MAX_SIZE
#define MQTT_BUFFER_MAX_SIZE        1024            // Size of the MQTT payload buffer for MQTT_MESSAGE_EVENT. Large messages will only be available via MQTT_MESSAGE_RAW_EVENT.
                                                    // Note: When using MQTT_LIBRARY_PUBSUBCLIENT, MQTT_MAX_PACKET_SIZE should not be more than this value.
#endif

// These are the properties that will be sent when useJson is true
#ifndef MQTT_ENQUEUE_IP
#define MQTT_ENQUEUE_IP             1
#endif

#ifndef MQTT_ENQUEUE_MAC
#define MQTT_ENQUEUE_MAC            1
#endif

#ifndef MQTT_ENQUEUE_HOSTNAME
#define MQTT_ENQUEUE_HOSTNAME       1
#endif

#ifndef MQTT_ENQUEUE_DATETIME
#define MQTT_ENQUEUE_DATETIME       1
#endif

#ifndef MQTT_ENQUEUE_MESSAGE_ID
#define MQTT_ENQUEUE_MESSAGE_ID     1
#endif

// These particles will be concatenated to the MQTT_TOPIC base to form the actual topic
#define MQTT_TOPIC_JSON             "data"
#define MQTT_TOPIC_ACTION           "action"
#define MQTT_TOPIC_RELAY            "relay"
#define MQTT_TOPIC_LED              "led"
#define MQTT_TOPIC_BUTTON           "button"
#define MQTT_TOPIC_IP               "ip"
#define MQTT_TOPIC_SSID             "ssid"
#define MQTT_TOPIC_BSSID            "bssid"
#define MQTT_TOPIC_VERSION          "version"
#define MQTT_TOPIC_UPTIME           "uptime"
#define MQTT_TOPIC_DATETIME         "datetime"
#define MQTT_TOPIC_TIMESTAMP        "timestamp"
#define MQTT_TOPIC_FREEHEAP         "freeheap"
#define MQTT_TOPIC_VCC              "vcc"
#ifndef MQTT_TOPIC_STATUS
#define MQTT_TOPIC_STATUS           "status"
#endif
#define MQTT_TOPIC_MAC              "mac"
#define MQTT_TOPIC_RSSI             "rssi"
#define MQTT_TOPIC_MESSAGE_ID       "id"
#define MQTT_TOPIC_APP              "app"
#define MQTT_TOPIC_INTERVAL         "interval"
#define MQTT_TOPIC_HOSTNAME         "host"
#define MQTT_TOPIC_DESCRIPTION      "desc"
#define MQTT_TOPIC_TIME             "time"
#define MQTT_TOPIC_RFOUT            "rfout"
#define MQTT_TOPIC_RFIN             "rfin"
#define MQTT_TOPIC_RFLEARN          "rflearn"
#define MQTT_TOPIC_RFRAW            "rfraw"
#define MQTT_TOPIC_UARTIN           "uartin"
#define MQTT_TOPIC_UARTOUT          "uartout"
#define MQTT_TOPIC_LOADAVG          "loadavg"
#define MQTT_TOPIC_BOARD            "board"
#define MQTT_TOPIC_PULSE            "pulse"
#define MQTT_TOPIC_SPEED            "speed"
#define MQTT_TOPIC_IRIN             "irin"
#define MQTT_TOPIC_IROUT            "irout"
#define MQTT_TOPIC_OTA              "ota"
#define MQTT_TOPIC_TELNET_REVERSE   "telnet_reverse"
#define MQTT_TOPIC_CURTAIN          "curtain"

// Light module
#define MQTT_TOPIC_CHANNEL          "channel"
#define MQTT_TOPIC_COLOR_RGB        "rgb"
#define MQTT_TOPIC_COLOR_HSV        "hsv"
#define MQTT_TOPIC_ANIM_MODE        "anim_mode"
#define MQTT_TOPIC_ANIM_SPEED       "anim_speed"
#define MQTT_TOPIC_BRIGHTNESS       "brightness"
#define MQTT_TOPIC_MIRED            "mired"
#define MQTT_TOPIC_KELVIN           "kelvin"
#define MQTT_TOPIC_TRANSITION       "transition"

// Thermostat module
#define MQTT_TOPIC_HOLD_TEMP        "hold_temp"
#define MQTT_TOPIC_HOLD_TEMP_MIN    "min"
#define MQTT_TOPIC_HOLD_TEMP_MAX    "max"
#define MQTT_TOPIC_REMOTE_TEMP      "remote_temp"
#define MQTT_TOPIC_ASK_TEMP_RANGE   "ask_temp_range"
#define MQTT_TOPIC_NOTIFY_TEMP_RANGE_MIN "notify_temp_range_min"
#define MQTT_TOPIC_NOTIFY_TEMP_RANGE_MAX "notify_temp_range_max"


#ifndef MQTT_STATUS_ONLINE
#define MQTT_STATUS_ONLINE          "1"         // Value for the device ON message
#endif

#ifndef MQTT_STATUS_OFFLINE
#define MQTT_STATUS_OFFLINE         "0"         // Value for the device OFF message (will)
#endif

#define MQTT_ACTION_RESET           "reboot"    // RESET MQTT topic particle

// Custom get and set postfixes
// Use something like "/status" or "/set", with leading slash
// Since 1.9.0 the default value is "" for getter and "/set" for setter
#ifndef MQTT_GETTER
#define MQTT_GETTER                 ""
#endif

#ifndef MQTT_SETTER
#define MQTT_SETTER                 "/set"
#endif

// -----------------------------------------------------------------------------
// BROKER
// -----------------------------------------------------------------------------

#ifndef BROKER_SUPPORT
#define BROKER_SUPPORT          1           // The broker is a poor-man's pubsub manager
#endif

// -----------------------------------------------------------------------------
// SETTINGS
// -----------------------------------------------------------------------------

#ifndef SETTINGS_AUTOSAVE
#define SETTINGS_AUTOSAVE       1           // Autosave settings or force manual commit
#endif

#define SETTINGS_MAX_LIST_COUNT 16          // Maximum index for settings lists

// -----------------------------------------------------------------------------
// LIGHT
// -----------------------------------------------------------------------------

// LIGHT_PROVIDER_DIMMER can have from 1 to 5 different channels.
// They have to be defined for each device in the hardware.h file.
// If 3 or more channels first 3 will be considered RGB.
// Usual configurations are:
// 1 channels => W
// 2 channels => WW
// 3 channels => RGB
// 4 channels => RGBW
// 5 channels => RGBWW

#ifndef LIGHT_SAVE_ENABLED
#define LIGHT_SAVE_ENABLED      1           // Light channel values saved by default after each change
#endif

#ifndef LIGHT_COMMS_DELAY
#define LIGHT_COMMS_DELAY       100         // Delay communication after light update (in ms)
#endif

#ifndef LIGHT_SAVE_DELAY
#define LIGHT_SAVE_DELAY        5           // Persist color after 5 seconds to avoid wearing out
#endif

#ifndef LIGHT_MIN_PWM
#define LIGHT_MIN_PWM           0
#endif

#ifndef LIGHT_MAX_PWM

#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
#define LIGHT_MAX_PWM           255
#elif LIGHT_PROVIDER == LIGHT_PROVIDER_DIMMER
#define LIGHT_MAX_PWM           10000        // 10000 * 200ns => 2 kHz
#else
#define LIGHT_MAX_PWM           0
#endif

#endif // LIGHT_MAX_PWM

#ifndef LIGHT_LIMIT_PWM
#define LIGHT_LIMIT_PWM         LIGHT_MAX_PWM   // Limit PWM to this value (prevent 100% power)
#endif

#ifndef LIGHT_MIN_VALUE
#define LIGHT_MIN_VALUE         0           // Minimum light value
#endif

#ifndef LIGHT_MAX_VALUE
#define LIGHT_MAX_VALUE         255         // Maximum light value
#endif

#ifndef LIGHT_MIN_BRIGHTNESS
#define LIGHT_MIN_BRIGHTNESS    0           // Minimum brightness value
#endif

#ifndef LIGHT_MAX_BRIGHTNESS
#define LIGHT_MAX_BRIGHTNESS    255         // Maximum brightness value
#endif

// Default mireds & kelvin to the Philips Hue limits
// https://developers.meethue.com/documentation/core-concepts
//
// Home Assistant also uses these, see Light::min_mireds, Light::max_mireds
// https://github.com/home-assistant/home-assistant/blob/dev/homeassistant/components/light/__init__.py

// Used when LIGHT_USE_WHITE AND LIGHT_USE_CCT is 1 - (1000000/Kelvin = MiReds)
// Warning! Don't change this yet, NOT FULLY IMPLEMENTED!
#ifndef LIGHT_COLDWHITE_MIRED
#define LIGHT_COLDWHITE_MIRED   153         // Coldwhite Strip, Value must be __BELOW__ W2!! (Default: 6535 Kelvin/153 MiRed)
#endif

#ifndef LIGHT_WARMWHITE_MIRED
#define LIGHT_WARMWHITE_MIRED   500         // Warmwhite Strip, Value must be __ABOVE__ W1!! (Default: 2000 Kelvin/500 MiRed)
#endif

#ifndef LIGHT_STEP
#define LIGHT_STEP              32          // Step size
#endif

#ifndef LIGHT_USE_COLOR
#define LIGHT_USE_COLOR         1           // Use 3 first channels as RGB
#endif

#ifndef LIGHT_USE_WHITE
#define LIGHT_USE_WHITE         0           // Use the 4th channel as (Warm-)White LEDs
#endif

#ifndef LIGHT_USE_CCT
#define LIGHT_USE_CCT           0           // Use the 5th channel as Coldwhite LEDs, LIGHT_USE_WHITE must be 1.
#endif

#ifndef LIGHT_USE_GAMMA
#define LIGHT_USE_GAMMA         0           // Use gamma correction for color channels
#endif

#ifndef LIGHT_USE_CSS
#define LIGHT_USE_CSS           1           // Use CSS style to report colors (1=> "#FF0000", 0=> "255,0,0")
#endif

#ifndef LIGHT_USE_RGB
#define LIGHT_USE_RGB           0           // Use RGB color selector (1=> RGB, 0=> HSV)
#endif

#ifndef LIGHT_WHITE_FACTOR
#define LIGHT_WHITE_FACTOR      1           // When using LIGHT_USE_WHITE with uneven brightness LEDs,
                                            // this factor is used to scale the white channel to match brightness
#endif


#ifndef LIGHT_USE_TRANSITIONS
#define LIGHT_USE_TRANSITIONS   1           // Transitions between colors
#endif

#ifndef LIGHT_TRANSITION_STEP
#define LIGHT_TRANSITION_STEP   10          // Time in millis between each transtion step
#endif

#ifndef LIGHT_TRANSITION_TIME
#define LIGHT_TRANSITION_TIME   500         // Time in millis from color to color
#endif


// -----------------------------------------------------------------------------
// DOMOTICZ
// -----------------------------------------------------------------------------

#ifndef DOMOTICZ_SUPPORT
#define DOMOTICZ_SUPPORT        MQTT_SUPPORT    // Build with domoticz (if MQTT) support (1.72Kb)
#endif

#ifndef DOMOTICZ_ENABLED
#define DOMOTICZ_ENABLED        0               // Disable domoticz by default
#endif

#ifndef DOMOTICZ_IN_TOPIC
#define DOMOTICZ_IN_TOPIC       "domoticz/in"   // Default subscription topic
#endif

#ifndef DOMOTICZ_OUT_TOPIC
#define DOMOTICZ_OUT_TOPIC      "domoticz/out"  // Default publication topic
#endif

// -----------------------------------------------------------------------------
// HOME ASSISTANT
// -----------------------------------------------------------------------------

#ifndef HOMEASSISTANT_SUPPORT
#define HOMEASSISTANT_SUPPORT   MQTT_SUPPORT    // Build with home assistant support (if MQTT, 1.64Kb)
#endif

#ifndef HOMEASSISTANT_ENABLED
#define HOMEASSISTANT_ENABLED   0               // Integration not enabled by default
#endif

#ifndef HOMEASSISTANT_PREFIX
#define HOMEASSISTANT_PREFIX    "homeassistant" // Default MQTT prefix
#endif

// -----------------------------------------------------------------------------
// INFLUXDB
// -----------------------------------------------------------------------------

#ifndef INFLUXDB_SUPPORT
#define INFLUXDB_SUPPORT        0               // Disable InfluxDB support by default (4.38Kb)
#endif

#ifndef INFLUXDB_ENABLED
#define INFLUXDB_ENABLED        0               // InfluxDB disabled by default
#endif

#ifndef INFLUXDB_HOST
#define INFLUXDB_HOST           ""              // Default server
#endif

#ifndef INFLUXDB_PORT
#define INFLUXDB_PORT           8086            // Default InfluxDB port
#endif

#ifndef INFLUXDB_DATABASE
#define INFLUXDB_DATABASE       ""              // Default database
#endif

#ifndef INFLUXDB_USERNAME
#define INFLUXDB_USERNAME       ""              // Default username
#endif

#ifndef INFLUXDB_PASSWORD
#define INFLUXDB_PASSWORD       ""              // Default password
#endif


// -----------------------------------------------------------------------------
// THINGSPEAK
// -----------------------------------------------------------------------------

#ifndef THINGSPEAK_SUPPORT
#define THINGSPEAK_SUPPORT          1               // Enable Thingspeak support by default (2.56Kb)
#endif

#ifndef THINGSPEAK_ENABLED
#define THINGSPEAK_ENABLED          0               // Thingspeak disabled by default
#endif

#ifndef THINGSPEAK_APIKEY
#define THINGSPEAK_APIKEY           ""              // Default API KEY
#endif

#ifndef THINGSPEAK_CLEAR_CACHE
#define THINGSPEAK_CLEAR_CACHE      1               // Clear cache after sending values
                                                    // Not clearing it will result in latest values for each field being sent every time
#endif

#ifndef THINGSPEAK_USE_ASYNC
#define THINGSPEAK_USE_ASYNC        1               // Use AsyncClient instead of WiFiClientSecure
#endif

// THINGSPEAK OVER SSL
// Using THINGSPEAK over SSL works well but generates problems with the web interface,
// so you should compile it with WEB_SUPPORT to 0.
// When THINGSPEAK_USE_ASYNC is 1, requires EspAsyncTCP to be built with ASYNC_TCP_SSL_ENABLED=1 and ESP8266 Arduino Core >= 2.4.0.
// When THINGSPEAK_USE_ASYNC is 0, requires Arduino Core >= 2.6.0 and SECURE_CLIENT_BEARSSL
//
// WARNING: Thingspeak servers do not support MFLN right now, connection requires at least 30KB of free RAM.
//          Also see MQTT comments above.

#ifndef THINGSPEAK_USE_SSL
#define THINGSPEAK_USE_SSL          0               // Use secure connection
#endif

#ifndef THINGSPEAK_SECURE_CLIENT_CHECK
#define THINGSPEAK_SECURE_CLIENT_CHECK    SECURE_CLIENT_CHECK
#endif

#ifndef THINGSPEAK_SECURE_CLIENT_MFLN
#define THINGSPEAK_SECURE_CLIENT_MFLN     SECURE_CLIENT_MFLN
#endif

#ifndef THINGSPEAK_FINGERPRINT
#define THINGSPEAK_FINGERPRINT      "78 60 18 44 81 35 BF DF 77 84 D4 0A 22 0D 9B 4E 6C DC 57 2C"
#endif

#ifndef THINGSPEAK_ADDRESS
#if THINGSPEAK_USE_SSL
#define THINGSPEAK_ADDRESS          "https://api.thingspeak.com/update"
#else
#define THINGSPEAK_ADDRESS          "http://api.thingspeak.com/update"
#endif
#endif // ifndef THINGSPEAK_ADDRESS

#ifndef THINGSPEAK_TRIES
#define THINGSPEAK_TRIES            3               // Number of tries when sending data (minimum 1)
#endif

#define THINGSPEAK_MIN_INTERVAL     15000           // Minimum interval between POSTs (in millis)
#define THINGSPEAK_FIELDS           8               // Number of fields

// -----------------------------------------------------------------------------
// SCHEDULER
// -----------------------------------------------------------------------------

#ifndef SCHEDULER_SUPPORT
#define SCHEDULER_SUPPORT           1               // Enable scheduler (2.45Kb)
#endif

#ifndef SCHEDULER_MAX_SCHEDULES
#define SCHEDULER_MAX_SCHEDULES     10              // Max schedules alowed
#endif

#ifndef SCHEDULER_RESTORE_LAST_SCHEDULE
#define SCHEDULER_RESTORE_LAST_SCHEDULE      0      // Restore the last schedule state on the device boot
#endif

#ifndef SCHEDULER_WEEKDAYS
#define SCHEDULER_WEEKDAYS          "1,2,3,4,5,6,7" // (Default - Run the schedules every day)
#endif

// -----------------------------------------------------------------------------
// RPN RULES
// -----------------------------------------------------------------------------

#ifndef RPN_RULES_SUPPORT
#define RPN_RULES_SUPPORT           0               // Enable RPN Rules (8.6Kb)
#endif

#ifndef RPN_DELAY
#define RPN_DELAY                   100             // Execute rules after 100ms without messages
#endif

#ifndef RPN_STICKY
#define RPN_STICKY                  1               // Keeps variable after rule execution
#endif

// -----------------------------------------------------------------------------
// NTP
// -----------------------------------------------------------------------------

#ifndef NTP_SUPPORT
#define NTP_SUPPORT                 1               // Build with NTP support by default (depends on Core version)
#endif

#ifndef NTP_SERVER
#define NTP_SERVER                  "pool.ntp.org"  // Default NTP server
#endif

#ifndef NTP_TIMEZONE
#define NTP_TIMEZONE                TZ_Etc_UTC      // POSIX TZ variable. Default to UTC from TZ.h (which is PSTR("UTC0"))
                                                    // For the format documentation, see:
                                                    // - https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
                                                    // ESP8266 Core provides human-readable aliases for POSIX format, see:
                                                    // - Latest: https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
                                                    // - PlatformIO: ~/.platformio/packages/framework-arduinoespressif8266/cores/esp8266/TZ.h
                                                    //   (or, possibly, c:\.platformio\... on Windows)
                                                    // - Arduino IDE: depends on platform, see `/dist/arduino_ide/README.md`
#endif

#ifndef NTP_UPDATE_INTERVAL
#define NTP_UPDATE_INTERVAL         1800            // NTP check every 30 minutes
#endif

#ifndef NTP_START_DELAY
#define NTP_START_DELAY             3               // Delay NTP start for 3 seconds
#endif

#ifndef NTP_WAIT_FOR_SYNC
#define NTP_WAIT_FOR_SYNC           1               // Do not report any datetime until NTP sync'ed
#endif

// WARNING: legacy NTP settings. can be ignored with Core 2.6.2+

#ifndef NTP_TIMEOUT
#define NTP_TIMEOUT                 1000            // Set NTP request timeout to 2 seconds (issue #452)
#endif

#ifndef NTP_TIME_OFFSET
#define NTP_TIME_OFFSET             60              // Default timezone offset (GMT+1)
#endif

#ifndef NTP_DAY_LIGHT
#define NTP_DAY_LIGHT               1               // Enable daylight time saving by default
#endif

#ifndef NTP_SYNC_INTERVAL
#define NTP_SYNC_INTERVAL           60              // NTP initial check every minute
#endif

#ifndef NTP_DST_REGION
#define NTP_DST_REGION              0               // 0 for Europe, 1 for USA (defined in NtpClientLib)
#endif

// -----------------------------------------------------------------------------
// ALEXA
// -----------------------------------------------------------------------------

// This setting defines whether Alexa support should be built into the firmware
#ifndef ALEXA_SUPPORT
#define ALEXA_SUPPORT               1               // Enable Alexa support by default (10.84Kb)
#endif

// This is default value for the alexaEnabled setting that defines whether
// this device should be discoberable and respond to Alexa commands.
// Both ALEXA_SUPPORT and alexaEnabled should be 1 for Alexa support to work.
#ifndef ALEXA_ENABLED
#define ALEXA_ENABLED               1
#endif

#ifndef ALEXA_HOSTNAME
#define ALEXA_HOSTNAME              ""
#endif


// -----------------------------------------------------------------------------
// MQTT RF BRIDGE
// -----------------------------------------------------------------------------

#ifndef RF_SUPPORT
#define RF_SUPPORT                  0
#endif

#ifndef RF_DEBOUNCE
#define RF_DEBOUNCE                 500
#endif

#ifndef RF_LEARN_TIMEOUT
#define RF_LEARN_TIMEOUT            60000
#endif

#ifndef RF_SEND_TIMES
#define RF_SEND_TIMES               4               // How many times to send the message
#endif

#ifndef RF_SEND_DELAY
#define RF_SEND_DELAY               500             // Interval between sendings in ms
#endif

#ifndef RF_RECEIVE_DELAY
#define RF_RECEIVE_DELAY            500             // Interval between recieving in ms (avoid debouncing)
#endif

// Enable RCSwitch support
// Originally implemented for SONOFF BASIC
// https://tinkerman.cat/adding-rf-to-a-non-rf-itead-sonoff/
// Also possible to use with SONOFF RF BRIDGE, thanks to @wildwiz
// https://github.com/xoseperez/espurna/wiki/Hardware-Itead-Sonoff-RF-Bridge---Direct-Hack
#ifndef RFB_DIRECT
#define RFB_DIRECT                  0
#endif

#ifndef RFB_RX_PIN
#define RFB_RX_PIN                  GPIO_NONE
#endif

#ifndef RFB_TX_PIN
#define RFB_TX_PIN                  GPIO_NONE
#endif


// -----------------------------------------------------------------------------
// IR Bridge
// -----------------------------------------------------------------------------

#ifndef IR_SUPPORT
#define IR_SUPPORT                  0               // Do not build with IR support by default (10.25Kb)
#endif

//#define IR_RX_PIN                   5               // GPIO the receiver is connected to
//#define IR_TX_PIN                   4               // GPIO the transmitter is connected to

#ifndef IR_USE_RAW
#define IR_USE_RAW                  0               // Use raw codes
#endif

#ifndef IR_BUFFER_SIZE
#define IR_BUFFER_SIZE              1024
#endif

#ifndef IR_TIMEOUT
#define IR_TIMEOUT                  15U
#endif

#ifndef IR_REPEAT
#define IR_REPEAT                   1
#endif

#ifndef IR_DELAY
#define IR_DELAY                    100
#endif

#ifndef IR_DEBOUNCE
#define IR_DEBOUNCE                 500             // IR debounce time in milliseconds
#endif

#ifndef IR_BUTTON_SET
#define IR_BUTTON_SET               0               // IR button set to use (see ../ir_button.h)
#endif

//--------------------------------------------------------------------------------
// Custom RFM69 to MQTT bridge
// Check http://tinkerman.cat/rfm69-wifi-gateway/
// Enable support by passing RFM69_SUPPORT=1 build flag
//--------------------------------------------------------------------------------

#ifndef RFM69_SUPPORT
#define RFM69_SUPPORT               0
#endif

#ifndef RFM69_MAX_TOPICS
#define RFM69_MAX_TOPICS            50
#endif

#ifndef RFM69_MAX_NODES
#define RFM69_MAX_NODES             255
#endif

#ifndef RFM69_DEFAULT_TOPIC
#define RFM69_DEFAULT_TOPIC         "/rfm69gw/{node}/{key}"
#endif

#ifndef RFM69_NODE_ID
#define RFM69_NODE_ID               1
#endif

#ifndef RFM69_GATEWAY_ID
#define RFM69_GATEWAY_ID            1
#endif

#ifndef RFM69_NETWORK_ID
#define RFM69_NETWORK_ID            164
#endif

#ifndef RFM69_PROMISCUOUS
#define RFM69_PROMISCUOUS           0
#endif

#ifndef RFM69_PROMISCUOUS_SENDS
#define RFM69_PROMISCUOUS_SENDS     0
#endif

#ifndef RFM69_FREQUENCY
#define RFM69_FREQUENCY             RF69_868MHZ
#endif

#ifndef RFM69_ENCRYPTKEY
#define RFM69_ENCRYPTKEY            "fibonacci0123456"
#endif

#ifndef RFM69_CS_PIN
#define RFM69_CS_PIN                SS
#endif

#ifndef RFM69_IRQ_PIN
#define RFM69_IRQ_PIN               5
#endif

#ifndef RFM69_RESET_PIN
#define RFM69_RESET_PIN             7
#endif

#ifndef RFM69_IS_RFM69HW
#define RFM69_IS_RFM69HW            0
#endif

//--------------------------------------------------------------------------------
// TUYA switch & dimmer support
//--------------------------------------------------------------------------------
#ifndef TUYA_SUPPORT
#define TUYA_SUPPORT                0
#endif

#ifndef TUYA_SERIAL
#define TUYA_SERIAL                 Serial
#endif

// =============================================================================
// Configuration helpers
// =============================================================================

//------------------------------------------------------------------------------
// Provide generic way to detect debugging support
//------------------------------------------------------------------------------
#ifndef DEBUG_SUPPORT
#define DEBUG_SUPPORT ( \
    DEBUG_SERIAL_SUPPORT || \
    DEBUG_UDP_SUPPORT || \
    DEBUG_TELNET_SUPPORT || \
    DEBUG_WEB_SUPPORT \
)
#endif

