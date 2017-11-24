#include <Arduino.h>
#include <NtpClientLib.h>
#include <ESPAsyncWebServer.h>
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include <functional>
#include <FastLED.h>

AsyncWebServer * webServer();

typedef std::function<void(char *, size_t)> api_get_callback_f;
typedef std::function<void(const char *)> api_put_callback_f;
void apiRegister(const char * url, const char * key, api_get_callback_f getFn, api_put_callback_f putFn = NULL);

typedef std::function<void(unsigned int, const char *, const char *)> mqtt_callback_f;
void mqttRegister(mqtt_callback_f callback);
String mqttSubtopic(char * topic);

typedef std::function<void(JsonObject&)> ws_callback_f;
void wsRegister(ws_callback_f sender, ws_callback_f receiver = NULL);
void wsSend(ws_callback_f sender);

template<typename T> bool setSetting(const String& key, T value);
template<typename T> bool setSetting(const String& key, unsigned int index, T value);
template<typename T> String getSetting(const String& key, T defaultValue);
template<typename T> String getSetting(const String& key, unsigned int index, T defaultValue);

template<typename T> void domoticzSend(const char * key, T value);
template<typename T> void domoticzSend(const char * key, T nvalue, const char * svalue);

template<typename T> bool idbSend(const char * topic, T payload);

char * ltrim(char * s);
