/*

SETTINGS MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>

#include <functional>
#include <utility>
#include <vector>
#include <type_traits>

#include <ArduinoJson.h>

#include "storage_eeprom.h"

#include "settings_convert.h"
#include "settings_helpers.h"
#include "settings_embedis.h"
#include "terminal.h"

// --------------------------------------------------------------------------

void resetSettings();
void saveSettings();
void autosaveSettings();

namespace espurna {
namespace settings {

// TODO: multi-byte access
// {blob} read(size_t)
// void write(size_t, {blob})

class EepromStorage {
public:
    uint8_t read(size_t pos) const {
        return eepromRead(pos);
    }

    void write(size_t pos, uint8_t value) const {
        eepromWrite(pos, value);
    }

    void commit() const {
        autosaveSettings();
    }
};

using kvs_type = embedis::KeyValueStore<EepromStorage>;

namespace traits {

template<typename T>
using is_arduino_string = std::is_same<String, typename std::decay<T>::type>;

template<typename T>
using enable_if_arduino_string = std::enable_if<is_arduino_string<T>::value>;

template<typename T>
using enable_if_not_arduino_string = std::enable_if<!is_arduino_string<T>::value>;

} // namespace types

// TODO: allow StringView as key and value
// does not work right now because embedis api only works on things with char access
// (won't work on our flash strings, since those can only be accessed via aligned reads)

ValueResult get(const String& key);
bool set(const String& key, const String& value);
bool del(const String& key);
bool has(const String& key);

using Keys = std::vector<String>;
Keys keys();

size_t available();
size_t size();

using KeyValueResultCallback = std::function<void(settings::kvs_type::KeyValueResult&&)>;
void foreach(KeyValueResultCallback&&);

using PrefixResultCallback = std::function<void(StringView prefix, String key, const kvs_type::ReadResult& value)>;
void foreach_prefix(PrefixResultCallback&&, settings::query::StringViewIterator);

// --------------------------------------------------------------------------

namespace query {

using Check = bool(*)(StringView key);
using Get = String(*)(StringView key);

struct Handler {
    Check check;
    Get get;
};

} // namespace query
} // namespace settings
} // namespace espurna

void settingsRegisterQueryHandler(espurna::settings::query::Handler);
String settingsQuery(espurna::StringView key);

// --------------------------------------------------------------------------

void moveSetting(const String& from, const String& to);
void moveSetting(const String& from, const String& to, size_t index);
void moveSettings(const String& from, const String& to);

String getSetting(const char* key);
String getSetting(const String& key);
String getSetting(const __FlashStringHelper* key);

String getSetting(const espurna::settings::Key& key);
String getSetting(const espurna::settings::Key& key, const char* defaultValue);
String getSetting(const espurna::settings::Key& key, const __FlashStringHelper* defaultValue);
String getSetting(const espurna::settings::Key& key, const String& defaultValue);
String getSetting(const espurna::settings::Key& key, String&& defaultValue);
String getSetting(const espurna::settings::Key& key, espurna::StringView defaultValue);

template <typename T, typename = typename espurna::settings::traits::enable_if_not_arduino_string<T>::type>
T getSetting(const espurna::settings::Key& key, T defaultValue) {
    auto result = espurna::settings::get(key.value());
    if (result) {
        return espurna::settings::internal::convert<T>(result.ref());
    }
    return defaultValue;
}

template <typename T, typename = typename espurna::settings::traits::enable_if_arduino_string<T>::type>
bool setSetting(const espurna::settings::Key& key, T&& value) {
    return espurna::settings::set(key.value(), value);
}

template <typename T, typename = typename espurna::settings::traits::enable_if_not_arduino_string<T>::type>
bool setSetting(const espurna::settings::Key& key, T value) {
    return setSetting(key, String(value));
}

bool delSetting(const char* key);
bool delSetting(const String& key);
bool delSetting(const __FlashStringHelper* key);
bool delSetting(const espurna::settings::Key& key);

void delSettingPrefix(espurna::settings::query::StringViewIterator);

bool hasSetting(const char* key);
bool hasSetting(const String& key);
bool hasSetting(const __FlashStringHelper* key);
bool hasSetting(const espurna::settings::Key& key);

void settingsDump(const espurna::terminal::CommandContext&,
    const espurna::settings::query::Setting* begin,
    const espurna::settings::query::Setting* end);

template <typename T>
void settingsDump(const espurna::terminal::CommandContext& ctx, const T& settings) {
    settingsDump(ctx, std::begin(settings), std::end(settings));
}

void settingsDump(const espurna::terminal::CommandContext&,
    const espurna::settings::query::IndexedSetting* begin,
    const espurna::settings::query::IndexedSetting* end, size_t index);

template <typename T>
void settingsDump(const espurna::terminal::CommandContext& ctx, const T& settings, size_t index) {
    settingsDump(ctx, std::begin(settings), std::end(settings), index);
}

void settingsGetJson(JsonObject& data);
bool settingsRestoreJson(char* json_string, size_t json_buffer_size = 1024);
bool settingsRestoreJson(JsonObject& data);

size_t settingsKeyCount();
espurna::settings::Keys settingsKeys();

size_t settingsSize();

void settingsSetup();

// -----------------------------------------------------------------------------
// Configuration updates
// -----------------------------------------------------------------------------

using MigrateVersionCallback = void(*)(int);

void migrateVersion(MigrateVersionCallback);
int migrateVersion();
void migrate();

// -----------------------------------------------------------------------------
// Deprecated implementation
// -----------------------------------------------------------------------------

template<typename T>
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

// --------------------------------------------------------------------------

template<typename T>
String getSetting(const String& key, unsigned char index, T defaultValue) {
    return getSetting({key, index}, defaultValue);
}

template<typename T>
bool setSetting(const String& key, unsigned char index, T value) {
    return setSetting({key, index}, value);
}

template<typename T>
bool hasSetting(const String& key, unsigned char index) {
    return hasSetting({key, index});
}

template<typename T>
bool delSetting(const String& key, unsigned char index) {
    return delSetting({key, index});
}
