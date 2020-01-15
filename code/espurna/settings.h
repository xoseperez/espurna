/*

SETTINGS MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <utility>
#include <vector>
#include <ArduinoJson.h>

#include "libs/EmbedisWrap.h"

void moveSetting(const String& from, const String& to);
void moveSetting(const String& from, const String& to, unsigned int index);
void moveSettings(const String& from, const String& to);

static const String _settingsDefaultValue("");

class settings_key_t {

    public:
        settings_key_t(const char* value, unsigned char index) :
            value(value), index(index)
        {}
        settings_key_t(const String& value, unsigned char index) :
            value(value), index(index)
        {}
        settings_key_t(const char* value) :
            value(value), index(-1)
        {}
        settings_key_t(const String& value) :
            value(value), index(-1)
        {}
        settings_key_t(const __FlashStringHelper* value) :
            value(value), index(-1)
        {}

        String toString() const {
            if (index < 0) {
                return value;
            } else {
                return value + index;
            }
        }

        explicit operator String () const {
            return toString();
        }

    private:
        const String value;
        int index;
};

using settings_move_key_t = std::pair<settings_key_t, settings_key_t>;

template <typename T>
T _settingsConvert(const String& value);

template <>
float _settingsConvert(const String& value) {
    return value.toFloat();
}

template <>
double _settingsConvert(const String& value) {
    return _settingsConvert<float>(value);
}

template <>
int _settingsConvert(const String& value) {
    return value.toInt();
}

template <>
long _settingsConvert(const String& value) {
    return value.toInt();
}

template <>
bool _settingsConvert(const String& value) {
    return _settingsConvert<int>(value) == 1;
}

template <>
unsigned long _settingsConvert(const String& value) {
    return strtoul(value.c_str(), nullptr, 10);
}

template <>
unsigned int _settingsConvert(const String& value) {
    return _settingsConvert<unsigned long>(value);
}

template <>
unsigned short _settingsConvert(const String& value) {
    return _settingsConvert<unsigned long>(value);
}

template <>
unsigned char _settingsConvert(const String& value) {
    return _settingsConvert<unsigned long>(value);
}

template <>
String _settingsConvert(const String& value) {
    return value;
}

template <typename T>
T getSetting(const settings_key_t& key, T defaultValue);
String getSetting(const settings_key_t& key);

template<typename T>
bool setSetting(const settings_key_t& key, const T& value);

bool delSetting(const settings_key_t& key);
bool hasSetting(const settings_key_t& key);

void saveSettings();
void resetSettings();

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

