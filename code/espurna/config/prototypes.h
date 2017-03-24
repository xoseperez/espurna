#include <Arduino.h>
#include <functional>
#include <NtpClientLib.h>
#include <ESPAsyncWebServer.h>
#include <AsyncMqttClient.h>
#include <functional>

typedef std::function<void(char *, size_t)> apiGetCallbackFunction;
typedef std::function<void(const char *)> apiPutCallbackFunction;
void apiRegister(const char * url, const char * key, apiGetCallbackFunction getFn, apiPutCallbackFunction putFn = NULL);
void mqttRegister(void (*callback)(unsigned int, const char *, const char *));
String mqttSubtopic(char * topic);
template<typename T> bool setSetting(const String& key, T value);
template<typename T> bool setSetting(const String& key, unsigned int index, T value);
template<typename T> String getSetting(const String& key, T defaultValue);
template<typename T> String getSetting(const String& key, unsigned int index, T defaultValue);
template<typename T> void domoticzSend(const char * key, T value);
template<typename T> void domoticzSend(const char * key, T nvalue, const char * svalue);
