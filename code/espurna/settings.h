/*

SETTINGS MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#pragma once

#include <functional>
#include <utility>
#include <vector>
#include <ArduinoJson.h>

#include "libs/EmbedisWrap.h"
#include "settings_internal.h"

// --------------------------------------------------------------------------

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

        String toString() const;

        explicit operator String () const {
            return toString();
        }

    private:
        const String value;
        int index;
};

using settings_move_key_t = std::pair<settings_key_t, settings_key_t>;
using settings_filter_t = std::function<String(String& value)>;

// --------------------------------------------------------------------------

struct settings_cfg_t {
    String& setting;
    const char* key;
    const char* default_value;
};

using settings_cfg_list_t = std::initializer_list<settings_cfg_t>;

// --------------------------------------------------------------------------

void moveSetting(const String& from, const String& to);
void moveSetting(const String& from, const String& to, unsigned int index);
void moveSettings(const String& from, const String& to);

template<typename R, settings::internal::convert_t<R> Rfunc = settings::internal::convert>
R getSetting(const settings_key_t& key, R defaultValue);

String getSetting(const settings_key_t& key);

template<typename T>
bool setSetting(const settings_key_t& key, const T& value);

bool delSetting(const settings_key_t& key);
bool hasSetting(const settings_key_t& key);

void saveSettings();
void resetSettings();

void settingsGetJson(JsonObject& data);
bool settingsRestoreJson(JsonObject& data);

void settingsProcessConfig(const settings_cfg_list_t& config, settings_filter_t filter = nullptr);

// --------------------------------------------------------------------------

template <typename T>
String getSetting(const String& key, unsigned char index, T defaultValue)
__attribute__((deprecated("getSetting({key, index}, default) should be used instead")));

template<typename T>
bool setSetting(const String& key, unsigned char index, T value)
__attribute__((deprecated("setSetting({key, index}, value) should be used instead")));

template<typename T>
bool hasSetting(const String& key, unsigned char index)
__attribute__((deprecated("hasSetting({key, index}) should be used instead")));

template<typename T>
bool delSetting(const String& key, unsigned char index)
__attribute__((deprecated("delSetting({key, index}) should be used instead")));
