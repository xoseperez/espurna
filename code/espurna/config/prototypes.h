#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include <vector>
#include <memory>
#include <core_version.h>

extern "C" {
    #include "user_interface.h"
    extern struct rst_info resetInfo;
}

#define UNUSED(x) (void)(x)

// -----------------------------------------------------------------------------
// System
// -----------------------------------------------------------------------------

uint32_t systemResetReason();
uint8_t systemStabilityCounter();
void systemStabilityCounter(uint8_t);

// -----------------------------------------------------------------------------
// PROGMEM
// -----------------------------------------------------------------------------

#include <pgmspace.h>

// ref: https://github.com/esp8266/Arduino/blob/master/tools/sdk/libc/xtensa-lx106-elf/include/sys/pgmspace.h
// __STRINGIZE && __STRINGIZE_NX && PROGMEM definitions port

// Do not replace macros unless running version older than 2.5.0
#if defined(ARDUINO_ESP8266_RELEASE_2_3_0) \
    || defined(ARDUINO_ESP8266_RELEASE_2_4_0) \
    || defined(ARDUINO_ESP8266_RELEASE_2_4_1) \
    || defined(ARDUINO_ESP8266_RELEASE_2_4_2)

// Quoting esp8266/Arduino comments:
// "Since __section__ is supposed to be only use for global variables,
// there could be conflicts when a static/inlined function has them in the
// same file as a non-static PROGMEM object.
// Ref: https://gcc.gnu.org/onlinedocs/gcc-3.2/gcc/Variable-Attributes.html
// Place each progmem object into its own named section, avoiding conflicts"

#define __TO_STR_(A) #A
#define __TO_STR(A) __TO_STR_(A)

#undef PROGMEM
#define PROGMEM __attribute__((section( "\".irom.text." __FILE__ "." __TO_STR(__LINE__) "."  __TO_STR(__COUNTER__) "\"")))

// "PSTR() macro modified to start on a 32-bit boundary.  This adds on average
// 1.5 bytes/string, but in return memcpy_P and strcpy_P will work 4~8x faster"
#undef PSTR
#define PSTR(s) (__extension__({static const char __c[] __attribute__((__aligned__(4))) PROGMEM = (s); &__c[0];}))

#endif

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

using api_get_callback_f = std::function<void(char *, size_t)>;
using api_put_callback_f = std::function<void(const char *)> ;

#if WEB_SUPPORT
    void apiRegister(const char * key, api_get_callback_f getFn, api_put_callback_f putFn = NULL);
#endif

// -----------------------------------------------------------------------------
// Broker
// -----------------------------------------------------------------------------
#if BROKER_SUPPORT
    void brokerRegister(void (*)(const unsigned char, const char *, unsigned char, const char *));
#endif

// -----------------------------------------------------------------------------
// Debug
// -----------------------------------------------------------------------------

#include "../libs/DebugSend.h"

void debugSendImpl(const char*);
extern "C" {
     void custom_crash_callback(struct rst_info*, uint32_t, uint32_t);
}

// Core version 2.4.2 and higher changed the cont_t structure to a pointer:
// https://github.com/esp8266/Arduino/commit/5d5ea92a4d004ab009d5f642629946a0cb8893dd#diff-3fa12668b289ccb95b7ab334833a4ba8L35
// Core version 2.5.0 introduced EspClass helper method:
// https://github.com/esp8266/Arduino/commit/0e0e34c614fe8a47544c9998201b1d9b3c24eb18
extern "C" {
    #include <cont.h>
#if defined(ARDUINO_ESP8266_RELEASE_2_3_0) \
    || defined(ARDUINO_ESP8266_RELEASE_2_4_0) \
    || defined(ARDUINO_ESP8266_RELEASE_2_4_1)
    extern cont_t g_cont;
    #define getFreeStack() cont_get_free_stack(&g_cont)
#elif defined(ARDUINO_ESP8266_RELEASE_2_4_2)
    extern cont_t* g_pcont;
    #define getFreeStack() cont_get_free_stack(g_pcont)
#else
    #define getFreeStack() ESP.getFreeContStack()
#endif
}

void infoMemory(const char* , unsigned int, unsigned int);
unsigned int getFreeHeap();
unsigned int getInitialFreeHeap();

// -----------------------------------------------------------------------------
// Domoticz
// -----------------------------------------------------------------------------
#if DOMOTICZ_SUPPORT
    template<typename T> void domoticzSend(const char * key, T value);
    template<typename T> void domoticzSend(const char * key, T nvalue, const char * svalue);
#endif

// -----------------------------------------------------------------------------
// EEPROM_ROTATE
// -----------------------------------------------------------------------------
#include <EEPROM_Rotate.h>
EEPROM_Rotate EEPROMr;

void eepromSectorsDebug();

// -----------------------------------------------------------------------------
// GPIO
// -----------------------------------------------------------------------------
bool gpioValid(unsigned char gpio);
bool gpioGetLock(unsigned char gpio);
bool gpioReleaseLock(unsigned char gpio);

// -----------------------------------------------------------------------------
// Homeassistant
// -----------------------------------------------------------------------------
struct ha_config_t;
void haSetup();

// -----------------------------------------------------------------------------
// I2C
// -----------------------------------------------------------------------------
void i2cScan();
void i2cClearBus();
bool i2cGetLock(unsigned char address);
bool i2cReleaseLock(unsigned char address);
unsigned char i2cFindAndLock(size_t size, unsigned char * addresses);

void i2c_wakeup(uint8_t address);
uint8_t i2c_write_buffer(uint8_t address, uint8_t * buffer, size_t len);
uint8_t i2c_write_uint8(uint8_t address, uint8_t value);
uint8_t i2c_write_uint8(uint8_t address, uint8_t reg, uint8_t value);
uint8_t i2c_write_uint8(uint8_t address, uint8_t reg, uint8_t value1, uint8_t value2);
uint8_t i2c_write_uint16(uint8_t address, uint16_t value);
uint8_t i2c_write_uint16(uint8_t address, uint8_t reg, uint16_t value);
uint8_t i2c_read_uint8(uint8_t address);
uint8_t i2c_read_uint8(uint8_t address, uint8_t reg);
uint16_t i2c_read_uint16(uint8_t address);
uint16_t i2c_read_uint16(uint8_t address, uint8_t reg);
uint16_t i2c_read_uint16_le(uint8_t address, uint8_t reg);
int16_t i2c_read_int16(uint8_t address, uint8_t reg);
int16_t i2c_read_int16_le(uint8_t address, uint8_t reg);
void i2c_read_buffer(uint8_t address, uint8_t * buffer, size_t len);

// -----------------------------------------------------------------------------
// Lights
// -----------------------------------------------------------------------------

unsigned char lightChannels();

void lightState(unsigned char i, bool state);
bool lightState(unsigned char i);

void lightState(bool state);
bool lightState();

void lightBrightness(unsigned int brightness);
unsigned int lightBrightness();

unsigned int lightChannel(unsigned char id);
void lightChannel(unsigned char id, unsigned char value);

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------

using mqtt_callback_f = std::function<void(unsigned int, const char *, char *)>;

#if MQTT_SUPPORT
    void mqttRegister(mqtt_callback_f callback);
    String mqttMagnitude(char * topic);
#endif

#if MQTT_SECURE_CLIENT_INCLUDE_CA
#include "../static/mqtt_secure_client_ca.h" // Assumes this header file defines a _mqtt_client_ca[] PROGMEM = "...PEM data..."
#else
#include "../static/letsencrypt_isrgroot_pem.h" // Default to LetsEncrypt X3 certificate
#define _mqtt_client_ca _ssl_letsencrypt_isrg_x3_ca
#endif // MQTT_SECURE_CLIENT_INCLUDE_CA

// -----------------------------------------------------------------------------
// OTA
// -----------------------------------------------------------------------------

#include <ArduinoOTA.h>

#if OTA_CLIENT == OTA_CLIENT_ASYNCTCP
    #include <ESPAsyncTCP.h>
#endif

#if OTA_CLIENT == OTA_CLIENT_HTTPUPDATE
    #include <ESP8266HTTPClient.h>
    #include <ESP8266httpUpdate.h>
#endif

#if SECURE_CLIENT != SECURE_CLIENT_NONE

    #include <WiFiClientSecure.h>

    #if OTA_SECURE_CLIENT_INCLUDE_CA
    #include "../static/ota_secure_client_ca.h"
    #else
    #include "../static/digicert_evroot_pem.h"
    #define _ota_client_http_update_ca _ssl_digicert_ev_root_ca
    #endif

#endif // SECURE_CLIENT_SUPPORT

// -----------------------------------------------------------------------------
// RFM69
// -----------------------------------------------------------------------------
typedef struct {
    unsigned long messageID;
    unsigned char packetID;
    unsigned char senderID;
    unsigned char targetID;
    char * key;
    char * value;
    int16_t rssi;
} packet_t;

// -----------------------------------------------------------------------------
// Relay
// -----------------------------------------------------------------------------
#include <bitset>

// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------
#include <Embedis.h>
template<typename T> bool setSetting(const String& key, T value);
template<typename T> bool setSetting(const String& key, unsigned int index, T value);
template<typename T> String getSetting(const String& key, T defaultValue);
template<typename T> String getSetting(const String& key, unsigned int index, T defaultValue);
void settingsGetJson(JsonObject& data);
bool settingsRestoreJson(JsonObject& data);

// -----------------------------------------------------------------------------
// Terminal
// -----------------------------------------------------------------------------
#if TERMINAL_SUPPORT
    void terminalRegisterCommand(const String& name, void (*call)(Embedis*));
    void terminalInject(void *data, size_t len);
    Stream & terminalSerial();
#endif

// -----------------------------------------------------------------------------
// Utils
// -----------------------------------------------------------------------------
char * ltrim(char * s);
void nice_delay(unsigned long ms);
bool inline eraseSDKConfig();

#define ARRAYINIT(type, name, ...) type name[] = {__VA_ARGS__};

size_t strnlen(const char*, size_t);
char* strnstr(const char*, const char*, size_t);

// -----------------------------------------------------------------------------
// WebServer
// -----------------------------------------------------------------------------

class AsyncClient;
class AsyncWebServer;

#if WEB_SUPPORT
    #include <ESPAsyncWebServer.h>
    AsyncWebServer * webServer();
#else
    class AsyncWebServerRequest;
    class ArRequestHandlerFunction;
    class AsyncWebSocketClient;
    class AsyncWebSocket;
    class AwsEventType;
#endif

using web_body_callback_f = std::function<bool(AsyncWebServerRequest*, uint8_t*, size_t, size_t, size_t)>;
using web_request_callback_f = std::function<bool(AsyncWebServerRequest*)>;
void webBodyRegister(web_body_callback_f);
void webRequestRegister(web_request_callback_f);

// -----------------------------------------------------------------------------
// WebSockets
// -----------------------------------------------------------------------------
#include <queue>

// TODO: pending configuration headers refactoring... here for now
struct ws_counter_t;
struct ws_data_t;
struct ws_debug_t;
struct ws_callbacks_t;

using ws_on_send_callback_f = std::function<void(JsonObject&)>;
using ws_on_action_callback_f = std::function<void(uint32_t, const char *, JsonObject&)>;
using ws_on_keycheck_callback_f = std::function<bool(const char *, JsonVariant&)>;

using ws_on_send_callback_list_t = std::vector<ws_on_send_callback_f>;
using ws_on_action_callback_list_t = std::vector<ws_on_action_callback_f>;
using ws_on_keycheck_callback_list_t = std::vector<ws_on_keycheck_callback_f>;

#if WEB_SUPPORT
    struct ws_callbacks_t {
        ws_on_send_callback_list_t on_visible;
        ws_on_send_callback_list_t on_connected;
        ws_on_send_callback_list_t on_data;

        ws_on_action_callback_list_t on_action;
        ws_on_keycheck_callback_list_t on_keycheck;

        ws_callbacks_t& onVisible(ws_on_send_callback_f);
        ws_callbacks_t& onConnected(ws_on_send_callback_f);
        ws_callbacks_t& onData(ws_on_send_callback_f);
        ws_callbacks_t& onAction(ws_on_action_callback_f);
        ws_callbacks_t& onKeyCheck(ws_on_keycheck_callback_f);
    };

    ws_callbacks_t& wsRegister();

    void wsSetup();
    void wsSend(uint32_t, const char*);
    void wsSend(uint32_t, JsonObject&);
    void wsSend(JsonObject&);
    void wsSend(ws_on_send_callback_f);

    void wsSend_P(PGM_P);
    void wsSend_P(uint32_t, PGM_P);

    void wsPost(const ws_on_send_callback_f&);
    void wsPost(const ws_on_send_callback_list_t&);
    void wsPost(uint32_t, const ws_on_send_callback_list_t&);

    void wsPostAll(uint32_t, const ws_on_send_callback_list_t&);
    void wsPostAll(const ws_on_send_callback_list_t&);

    void wsPostSequence(uint32_t, const ws_on_send_callback_list_t&);
    void wsPostSequence(const ws_on_send_callback_list_t&);

    bool wsConnected();
    bool wsConnected(uint32_t);
    bool wsDebugSend(const char*, const char*);
#endif

// -----------------------------------------------------------------------------
// WIFI
// -----------------------------------------------------------------------------
#include <JustWifi.h>
using wifi_callback_f = std::function<void(justwifi_messages_t code, char * parameter)>;
void wifiRegister(wifi_callback_f callback);
bool wifiConnected();

#if LWIP_VERSION_MAJOR == 1
#include <netif/etharp.h>
#else // LWIP_VERSION_MAJOR >= 2
#include <lwip/etharp.h>
#endif

// -----------------------------------------------------------------------------
// THERMOSTAT
// -----------------------------------------------------------------------------
using thermostat_callback_f = std::function<void(bool)>;
#if THERMOSTAT_SUPPORT
    void thermostatRegister(thermostat_callback_f callback);
#endif

// -----------------------------------------------------------------------------
// RTC MEMORY
// -----------------------------------------------------------------------------
#include "rtcmem.h"

// -----------------------------------------------------------------------------
// Warn about broken Arduino functions
// -----------------------------------------------------------------------------

// Division by zero bug
// https://github.com/esp8266/Arduino/pull/2397
// https://github.com/esp8266/Arduino/pull/2408
#if defined(ARDUINO_ESP8266_RELEASE_2_3_0)
long  __attribute__((deprecated("Please avoid using map() with Core 2.3.0"))) map(long x, long in_min, long in_max, long out_min, long out_max);
#endif

// -----------------------------------------------------------------------------
// std::make_unique backport for C++11
// -----------------------------------------------------------------------------
#if 201103L >= __cplusplus
namespace std {
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}
#endif
