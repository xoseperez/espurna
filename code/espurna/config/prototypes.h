#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include <core_version.h>

extern "C" {
    #include "user_interface.h"
}

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
#if WEB_SUPPORT
    typedef std::function<void(char *, size_t)> api_get_callback_f;
    typedef std::function<void(const char *)> api_put_callback_f;
    void apiRegister(const char * key, api_get_callback_f getFn, api_put_callback_f putFn = NULL);
#else
    #define api_get_callback_f void *
    #define api_put_callback_f void *
#endif

// -----------------------------------------------------------------------------
// Broker
// -----------------------------------------------------------------------------
#if BROKER_SUPPORT
    void brokerRegister(void (*)(const char *, unsigned char, const char *));
#endif

// -----------------------------------------------------------------------------
// Debug
// -----------------------------------------------------------------------------
void debugSend(const char * format, ...);
void debugSend_P(PGM_P format, ...);
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
#if MQTT_SUPPORT
    typedef std::function<void(unsigned int, const char *, const char *)> mqtt_callback_f;
    void mqttRegister(mqtt_callback_f callback);
    String mqttMagnitude(char * topic);
#else
    #define mqtt_callback_f void *
#endif

// -----------------------------------------------------------------------------
// OTA
// -----------------------------------------------------------------------------
#include "ESPAsyncTCP.h"

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
void settingsRegisterCommand(const String& name, void (*call)(Embedis*));
void settingsInject(void *data, size_t len);
Stream & settingsSerial();

// -----------------------------------------------------------------------------
// Utils
// -----------------------------------------------------------------------------
char * ltrim(char * s);
void nice_delay(unsigned long ms);

#define ARRAYINIT(type, name, ...) type name[] = {__VA_ARGS__};

// -----------------------------------------------------------------------------
// WebServer
// -----------------------------------------------------------------------------
#if WEB_SUPPORT
    #include <ESPAsyncWebServer.h>
    AsyncWebServer * webServer();
#else
    #define AsyncWebServerRequest void
    #define ArRequestHandlerFunction void
    #define AsyncWebSocketClient void
    #define AsyncWebSocket void
    #define AwsEventType void *
#endif
typedef std::function<bool(AsyncWebServerRequest *request)> web_request_callback_f;
void webRequestRegister(web_request_callback_f callback);

// -----------------------------------------------------------------------------
// WebSockets
// -----------------------------------------------------------------------------
#if WEB_SUPPORT
    typedef std::function<void(JsonObject&)> ws_on_send_callback_f;
    void wsOnSendRegister(ws_on_send_callback_f callback);
    void wsSend(ws_on_send_callback_f sender);

    typedef std::function<void(uint32_t, const char *, JsonObject&)> ws_on_action_callback_f;
    void wsOnActionRegister(ws_on_action_callback_f callback);

    typedef std::function<bool(const char *, JsonVariant&)> ws_on_receive_callback_f;
    void wsOnReceiveRegister(ws_on_receive_callback_f callback);
#else
    #define ws_on_send_callback_f void *
    #define ws_on_action_callback_f void *
    #define ws_on_receive_callback_f void *
#endif

// -----------------------------------------------------------------------------
// WIFI
// -----------------------------------------------------------------------------
#include "JustWifi.h"
typedef std::function<void(justwifi_messages_t code, char * parameter)> wifi_callback_f;
void wifiRegister(wifi_callback_f callback);
bool wifiConnected();
