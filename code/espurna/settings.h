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

#include "settings_helpers.h"
#include "settings_embedis.h"
#include "terminal.h"

// --------------------------------------------------------------------------

void resetSettings();
void saveSettings();
void autosaveSettings();

namespace settings {

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

namespace internal {

template <typename T>
using is_arduino_string = std::is_same<String, typename std::decay<T>::type>;

template <typename T>
using enable_if_arduino_string = std::enable_if<is_arduino_string<T>::value>;

template <typename T>
using enable_if_not_arduino_string = std::enable_if<!is_arduino_string<T>::value>;

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

using PrefixResultCallback = std::function<void(settings::StringView prefix, String key, const kvs_type::ReadResult& value)>;
void foreach_prefix(PrefixResultCallback&&, settings::query::StringViewIterator);

// --------------------------------------------------------------------------

template <typename T>
T convert(const String& value);

template <>
float convert(const String& value);

template <>
double convert(const String& value);

template <>
signed char convert(const String& value);

template <>
short convert(const String& value);

template <>
int convert(const String& value);

template <>
long convert(const String& value);

template <>
bool convert(const String& value);

template <>
unsigned long convert(const String& value);

template <>
unsigned int convert(const String& value);

template <>
unsigned short convert(const String& value);

template <>
unsigned char convert(const String& value);

inline String serialize(uint8_t value, int base = 10) {
    return String(value, base);
}

inline String serialize(uint16_t value, int base = 10) {
    return String(value, base);
}

String serialize(uint32_t value, int base = 10);

inline String serialize(unsigned long value, int base = 10) {
    return serialize(static_cast<uint32_t>(value), base);
}

inline String serialize(int16_t value, int base = 10) {
    return String(value, base);
}

inline String serialize(int32_t value, int base = 10) {
    return String(value, base);
}

inline String serialize(int8_t value, int base = 10) {
    return serialize(static_cast<int32_t>(value), base);
}

inline String serialize(long value, int base = 10) {
    return String(value, base);
}

inline String serialize(bool value) {
    return value ? PSTR("true") : PSTR("false");
}

inline String serialize(float value) {
    return String(value, 3);
}

inline String serialize(double value) {
    return String(value, 3);
}

template <typename Container, typename T>
T convert(const Container& options, const String& value, T defaultValue) {
    if (value.length()) {
        using ::settings::options::Enumeration;
        using UnderlyingType = typename Enumeration<T>::UnderlyingType;
        typename Enumeration<T>::Numeric numeric;
        numeric.check(value, convert<UnderlyingType>);

        for (auto it = std::begin(options); it != std::end(options); ++it) {
            if (numeric && ((*it).numeric() == numeric.value())) {
                return static_cast<T>(numeric.value());
            } else if (!numeric && ((*it) == value)) {
                return (*it).value();
            }
        }
    }

    return defaultValue;
}

template <typename Container, typename T>
String serialize(const Container& options, T value) {
    String out;

    for (auto it = std::begin(options); it != std::end(options); ++it) {
        if ((*it).value() == value) {
            out = FPSTR((*it).string());
            break;
        }
    }

    return out;
}

} // namespace internal
} // namespace settings

// --------------------------------------------------------------------------

namespace settings {
namespace query {

using Check = bool(*)(StringView key);
using Get = String(*)(StringView key);

struct Handler {
    Check check;
    Get get;
};

} // namespace query
} // namespace settings

void settingsRegisterQueryHandler(settings::query::Handler);
String settingsQuery(::settings::StringView key);

// --------------------------------------------------------------------------

void moveSetting(const String& from, const String& to);
void moveSetting(const String& from, const String& to, size_t index);
void moveSettings(const String& from, const String& to);

template <typename T, typename = typename settings::internal::enable_if_not_arduino_string<T>::type>
T getSetting(const SettingsKey& key, T defaultValue) {
    auto result = settings::internal::get(key.value());
    if (result) {
        return settings::internal::convert<T>(result.ref());
    }
    return defaultValue;
}

String getSetting(const char* key);
String getSetting(const String& key);
String getSetting(const __FlashStringHelper* key);

String getSetting(const SettingsKey& key);
String getSetting(const SettingsKey& key, const char* defaultValue);
String getSetting(const SettingsKey& key, const __FlashStringHelper* defaultValue);
String getSetting(const SettingsKey& key, const String& defaultValue);
String getSetting(const SettingsKey& key, const String& defaultValue);
String getSetting(const SettingsKey& key, String&& defaultValue);

template<typename T, typename = typename settings::internal::enable_if_arduino_string<T>::type>
bool setSetting(const SettingsKey& key, T&& value) {
    return settings::internal::set(key.value(), value);
}

template<typename T, typename = typename settings::internal::enable_if_not_arduino_string<T>::type>
bool setSetting(const SettingsKey& key, T value) {
    return setSetting(key, String(value));
}

bool delSetting(const char* key);
bool delSetting(const String& key);
bool delSetting(const __FlashStringHelper* key);
bool delSetting(const SettingsKey& key);

void delSettingPrefix(settings::query::StringViewIterator);

bool hasSetting(const char* key);
bool hasSetting(const String& key);
bool hasSetting(const __FlashStringHelper* key);
bool hasSetting(const SettingsKey& key);

void settingsDump(const ::terminal::CommandContext&, const ::settings::query::Setting* begin, const ::settings::query::Setting* end);
template <typename T>
void settingsDump(const ::terminal::CommandContext& ctx, const T& settings) {
    settingsDump(ctx, std::begin(settings), std::end(settings));
}

void settingsDump(const ::terminal::CommandContext&, const ::settings::query::IndexedSetting* begin, const ::settings::query::IndexedSetting* end, size_t index);
template <typename T>
void settingsDump(const ::terminal::CommandContext& ctx, const T& settings, size_t index) {
    settingsDump(ctx, std::begin(settings), std::end(settings), index);
}

void settingsGetJson(JsonObject& data);
bool settingsRestoreJson(char* json_string, size_t json_buffer_size = 1024);
bool settingsRestoreJson(JsonObject& data);

size_t settingsKeyCount();
std::vector<String> settingsKeys();

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
