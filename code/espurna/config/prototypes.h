#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include <algorithm>
#include <vector>
#include <memory>

extern "C" {
    #include "user_interface.h"
    extern struct rst_info resetInfo;
}

#define UNUSED(x) (void)(x)

// -----------------------------------------------------------------------------
// System
// -----------------------------------------------------------------------------

#define LWIP_INTERNAL
#include <ESP8266WiFi.h>
#undef LWIP_INTERNAL

extern "C" {
  #include <lwip/opt.h>
  #include <lwip/ip.h>
  #include <lwip/tcp.h>
  #include <lwip/inet.h> // ip_addr_t
  #include <lwip/err.h> // ERR_x
  #include <lwip/dns.h> // dns_gethostbyname
  #include <lwip/ip_addr.h> // ip4/ip6 helpers
  #include <lwip/init.h> // LWIP_VERSION_MAJOR
}

// ref: https://github.com/me-no-dev/ESPAsyncTCP/pull/115/files#diff-e2e636049095cc1ff920c1bfabf6dcacR8
// This is missing with Core 2.3.0 and is sometimes missing from the build flags. Assume HIGH_BANDWIDTH version.
#ifndef TCP_MSS
#define TCP_MSS (1460)
#endif

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

using api_get_callback_f = std::function<void(char * buffer, size_t size)>;
using api_put_callback_f = std::function<void(const char * payload)> ;

#if WEB_SUPPORT
    void apiRegister(const char * key, api_get_callback_f getFn, api_put_callback_f putFn = NULL);
#endif

// -----------------------------------------------------------------------------
// Debug
// -----------------------------------------------------------------------------

#include "../libs/DebugSend.h"

void debugSendImpl(const char*);
extern "C" {
     void custom_crash_callback(struct rst_info*, uint32_t, uint32_t);
}

class PrintRaw;
class PrintHex;

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
// MQTT
// -----------------------------------------------------------------------------

#if MQTT_LIBRARY == MQTT_LIBRARY_ASYNCMQTTCLIENT
    #include <ESPAsyncTCP.h>
    #include <AsyncMqttClient.h>
#elif MQTT_LIBRARY == MQTT_LIBRARY_ARDUINOMQTT
    #include <MQTTClient.h>
#elif MQTT_LIBRARY == MQTT_LIBRARY_PUBSUBCLIENT
    #include <PubSubClient.h>
#endif

using mqtt_callback_f = std::function<void(unsigned int type, const char * topic, char * payload)>;
using mqtt_msg_t = std::pair<String, String>; // topic, payload

void mqttRegister(mqtt_callback_f callback);

String mqttTopic(const char * magnitude, bool is_set);
String mqttTopic(const char * magnitude, unsigned int index, bool is_set);

String mqttMagnitude(char * topic);

bool mqttSendRaw(const char * topic, const char * message, bool retain);
bool mqttSendRaw(const char * topic, const char * message);

void mqttSend(const char * topic, const char * message, bool force, bool retain);
void mqttSend(const char * topic, const char * message, bool force);
void mqttSend(const char * topic, const char * message);

void mqttSend(const char * topic, unsigned int index, const char * message, bool force);
void mqttSend(const char * topic, unsigned int index, const char * message);

const String& mqttPayloadOnline();
const String& mqttPayloadOffline();
const char* mqttPayloadStatus(bool status);

void mqttSendStatus();

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
#endif // SECURE_CLIENT != SECURE_CLIENT_NONE

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
// Settings
// -----------------------------------------------------------------------------
#include <Embedis.h>

template<typename T> bool setSetting(const String& key, T value);
template<typename T> bool setSetting(const String& key, unsigned int index, T value);
template<typename T> String getSetting(const String& key, T defaultValue);
template<typename T> String getSetting(const String& key, unsigned int index, T defaultValue);
void settingsGetJson(JsonObject& data);
bool settingsRestoreJson(JsonObject& data);

struct settings_cfg_t {
    String& setting;
    const char* key;
    const char* default_value;
};

using settings_filter_t = std::function<String(String& value)>;
using settings_cfg_list_t = std::initializer_list<settings_cfg_t>;

void settingsProcessConfig(const settings_cfg_list_t& config, settings_filter_t filter = nullptr);

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

using web_body_callback_f = std::function<bool(AsyncWebServerRequest*, uint8_t* data, size_t len, size_t index, size_t total)>;
using web_request_callback_f = std::function<bool(AsyncWebServerRequest*)>;
void webBodyRegister(web_body_callback_f);
void webRequestRegister(web_request_callback_f);

// -----------------------------------------------------------------------------
// WIFI
// -----------------------------------------------------------------------------
#include <JustWifi.h>
struct wifi_scan_info_t;
using wifi_scan_f = std::function<void(wifi_scan_info_t& info)>;
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
using thermostat_callback_f = std::function<void(bool state)>;
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
