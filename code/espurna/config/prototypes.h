#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include <pgmspace.h>

extern "C" {
    #include "user_interface.h"
}

// -----------------------------------------------------------------------------
// WebServer
// -----------------------------------------------------------------------------
#include <ESPAsyncWebServer.h>
AsyncWebServer * webServer();

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------
typedef std::function<void(char *, size_t)> api_get_callback_f;
typedef std::function<void(const char *)> api_put_callback_f;
void apiRegister(const char * key, api_get_callback_f getFn, api_put_callback_f putFn = NULL);

// -----------------------------------------------------------------------------
// WebSockets
// -----------------------------------------------------------------------------
typedef std::function<void(JsonObject&)> ws_on_send_callback_f;
void wsOnSendRegister(ws_on_send_callback_f callback);
void wsSend(ws_on_send_callback_f sender);

typedef std::function<void(uint32_t, const char *, JsonObject&)> ws_on_action_callback_f;
void wsOnActionRegister(ws_on_action_callback_f callback);

typedef std::function<void(void)> ws_on_after_parse_callback_f;
void wsOnAfterParseRegister(ws_on_after_parse_callback_f callback);

// -----------------------------------------------------------------------------
// WIFI
// -----------------------------------------------------------------------------
#include "JustWifi.h"
typedef std::function<void(justwifi_messages_t code, char * parameter)> wifi_callback_f;
void wifiRegister(wifi_callback_f callback);

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------
typedef std::function<void(unsigned int, const char *, const char *)> mqtt_callback_f;
void mqttRegister(mqtt_callback_f callback);
String mqttMagnitude(char * topic);

// -----------------------------------------------------------------------------
// Broker
// -----------------------------------------------------------------------------
void brokerRegister(void (*)(const char *, unsigned char, const char *));

// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------
#include <Embedis.h>
template<typename T> bool setSetting(const String& key, T value);
template<typename T> bool setSetting(const String& key, unsigned int index, T value);
template<typename T> String getSetting(const String& key, T defaultValue);
template<typename T> String getSetting(const String& key, unsigned int index, T defaultValue);
bool settingsGetJson(JsonObject& data);
bool settingsRestoreJson(JsonObject& data);
void settingsRegisterCommand(const String& name, void (*call)(Embedis*));
void settingsInject(void *data, size_t len);

// -----------------------------------------------------------------------------
// I2C
// -----------------------------------------------------------------------------
void i2cScan();
void i2cClearBus();
bool i2cGetLock(unsigned char address);
bool i2cReleaseLock(unsigned char address);
unsigned char i2cFindAndLock(size_t size, unsigned char * addresses);

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
// GPIO
// -----------------------------------------------------------------------------
bool gpioValid(unsigned char gpio);
bool gpioGetLock(unsigned char gpio);
bool gpioReleaseLock(unsigned char gpio);

// -----------------------------------------------------------------------------
// Debug
// -----------------------------------------------------------------------------
void debugSend(const char * format, ...);
void debugSend_P(PGM_P format, ...);

// -----------------------------------------------------------------------------
// Domoticz
// -----------------------------------------------------------------------------
template<typename T> void domoticzSend(const char * key, T value);
template<typename T> void domoticzSend(const char * key, T nvalue, const char * svalue);

// -----------------------------------------------------------------------------
// Utils
// -----------------------------------------------------------------------------
char * ltrim(char * s);
void nice_delay(unsigned long ms);
