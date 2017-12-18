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
void apiRegister(const char * url, const char * key, api_get_callback_f getFn, api_put_callback_f putFn = NULL);

// -----------------------------------------------------------------------------
// WebSockets
// -----------------------------------------------------------------------------
typedef std::function<void(JsonObject&)> ws_on_send_callback_f;
void wsOnSendRegister(ws_on_send_callback_f callback);
void wsSend(ws_on_send_callback_f sender);

typedef std::function<void(const char *, JsonObject&)> ws_on_action_callback_f;
void wsOnActionRegister(ws_on_action_callback_f callback);

typedef std::function<void(void)> ws_on_after_parse_callback_f;
void wsOnAfterParseRegister(ws_on_after_parse_callback_f callback);

// -----------------------------------------------------------------------------
// MQTT
// -----------------------------------------------------------------------------
typedef std::function<void(unsigned int, const char *, const char *)> mqtt_callback_f;
void mqttRegister(mqtt_callback_f callback);
String mqttSubtopic(char * topic);

// -----------------------------------------------------------------------------
// Settings
// -----------------------------------------------------------------------------
template<typename T> bool setSetting(const String& key, T value);
template<typename T> bool setSetting(const String& key, unsigned int index, T value);
template<typename T> String getSetting(const String& key, T defaultValue);
template<typename T> String getSetting(const String& key, unsigned int index, T defaultValue);

// -----------------------------------------------------------------------------
// Domoticz
// -----------------------------------------------------------------------------
#if DOMOTICZ_SUPPORT
template<typename T> void domoticzSend(const char * key, T value);
template<typename T> void domoticzSend(const char * key, T nvalue, const char * svalue);
#endif

// -----------------------------------------------------------------------------
// InfluxDB
// -----------------------------------------------------------------------------
#if INFLUXDB_SUPPORT
template<typename T> bool idbSend(const char * topic, T payload);
template<typename T> bool idbSend(const char * topic, unsigned char id, T payload);
#endif

// -----------------------------------------------------------------------------
// Light
// -----------------------------------------------------------------------------
#if LIGHT_PROVIDER == LIGHT_PROVIDER_MY92XX
#include <my92xx.h>
#endif

// -----------------------------------------------------------------------------
// Sensors
// -----------------------------------------------------------------------------
#include "sensors/BaseSensor.h"
#if DS18B20_SUPPORT
#include <OneWire.h>
#endif
#if PMSX003_SUPPORT
#include <SoftwareSerial.h>
#include <PMS.h>
#endif
#if BME280_SUPPORT
#include <SparkFunBME280.h>
#endif

// -----------------------------------------------------------------------------
// Utils
// -----------------------------------------------------------------------------
char * ltrim(char * s);
