/*

SETTINGS MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "espurna.h"

#include "crash.h"
#include "terminal.h"
#include "storage_eeprom.h"

#include <algorithm>
#include <vector>
#include <cstdlib>

#include <ArduinoJson.h>

BrokerBind(ConfigBroker);

// -----------------------------------------------------------------------------

namespace settings {

// Depending on features enabled, we may end up with different left boundary
// Settings are written right-to-left, so we only have issues when there are a lot of key-values
// XXX: slightly hacky, because we EEPROMr.length() is 0 before we enter setup() code
kvs_type kv_store(
    EepromStorage{},
#if DEBUG_SUPPORT
    EepromReservedSize + CrashReservedSize,
#else
    EepromReservedSize,
#endif
    EepromSize
);

} // namespace settings

size_t settingsSize() {
    return settings::kv_store.size() - settings::kv_store.available();
}

// --------------------------------------------------------------------------

namespace settings {
namespace internal {

uint32_t u32fromString(const String& string, int base) {

    const char *ptr = string.c_str();
    char *value_endptr = nullptr;

    // invalidate the whole string when invalid chars are detected
    const auto value = strtoul(ptr, &value_endptr, base);
    if (value_endptr == ptr || value_endptr[0] != '\0') {
        return 0;
    }

    return value;

}

// --------------------------------------------------------------------------

template <>
float convert(const String& value) {
    return atof(value.c_str());
}

template <>
double convert(const String& value) {
    return atof(value.c_str());
}

template <>
int convert(const String& value) {
    return value.toInt();
}

template <>
long convert(const String& value) {
    return value.toInt();
}

template <>
bool convert(const String& value) {
    if (value.length()) {
        if ((value == "0")
            || (value == "n")
            || (value == "no")
            || (value == "false")
            || (value == "off")) {
            return false;
        }

        return (value == "1")
            || (value == "y")
            || (value == "yes")
            || (value == "true")
            || (value == "on");
    }

    return false;
}

template <>
unsigned long convert(const String& value) {
    if (!value.length()) {
        return 0;
    }

    int base = 10;
    if (value.length() > 2) {
        if (value.startsWith("0b")) {
            base = 2;
        } else if (value.startsWith("0o")) {
            base = 8;
        } else if (value.startsWith("0x")) {
            base = 16;
        }
    }

    return u32fromString((base == 10) ? value : value.substring(2), base);
}

template <>
unsigned int convert(const String& value) {
    return convert<unsigned long>(value);
}

template <>
unsigned short convert(const String& value) {
    return convert<unsigned long>(value);
}

template <>
unsigned char convert(const String& value) {
    return convert<unsigned long>(value);
}

} // namespace settings::internal
} // namespace settings

// -----------------------------------------------------------------------------

/*
struct SettingsKeys {

    struct iterator {
        iterator(size_t total) :
            total(total)
        {}

        iterator& operator++() {
            if (total && (current_index < (total - 1))) {
                ++current_index
                current_value = settingsKeyName(current_index);
                return *this;
            }
            return end();
        }

        iterator operator++(int) {
            iterator val = *this;
            ++(*this);
            return val;
        }

        operator String() {
            return (current_index < total) ? current_value : empty_value;
        }

        bool operator ==(iterator& const other) const {
            return (total == other.total) && (current_index == other.current_index);
        }

        bool operator !=(iterator& const other) const {
            return !(*this == other);
        }

        using difference_type = size_t;
        using value_type = size_t;
        using pointer = const size_t*;
        using reference = const size_t&;
        using iterator_category = std::forward_iterator_tag;

        const size_t total;

        String empty_value;
        String current_value;
        size_t current_index = 0;
    };

    iterator begin() {
        return iterator {total};
    }

    iterator end() {
        return iterator {0};
    }

};
*/

// Note: we prefer things sorted via this function, not kv_store.keys() directly
std::vector<String> settingsKeys() {
    auto keys = settings::kv_store.keys();
    std::sort(keys.begin(), keys.end(), [](const String& rhs, const String& lhs) -> bool {
        return lhs.compareTo(rhs) > 0;
    });

    return keys;
}


static std::vector<settings_key_match_t> _settings_matchers;

void settingsRegisterDefaults(const settings_key_match_t& matcher) {
    _settings_matchers.push_back(matcher);
}

String settingsQueryDefaults(const String& key) {
    for (auto& matcher : _settings_matchers) {
        if (matcher.match(key.c_str())) {
            return matcher.key(key);
        }
    }
    return String();
}

// -----------------------------------------------------------------------------
// Key-value API
// -----------------------------------------------------------------------------

settings_move_key_t _moveKeys(const String& from, const String& to, unsigned char index) {
    return settings_move_key_t {{from, index}, {to, index}};
}

void moveSetting(const String& from, const String& to) {
    const auto value = getSetting(from);
    if (value.length() > 0) setSetting(to, value);
    delSetting(from);
}

void moveSetting(const String& from, const String& to, unsigned char index) {
    const auto keys = _moveKeys(from, to, index);
    const auto value = getSetting(keys.first);
    if (value.length() > 0) setSetting(keys.second, value);

    delSetting(keys.first);
}

void moveSettings(const String& from, const String& to) {
    unsigned char index = 0;
    while (index < 100) {
        const auto keys = _moveKeys(from, to, index);
        const auto value = getSetting(keys.first);
        if (value.length() == 0) break;
        setSetting(keys.second, value);
        delSetting(keys.first);
        ++index;
    }
}

template
bool getSetting(const SettingsKey& key, bool defaultValue);

template
int getSetting(const SettingsKey& key, int defaultValue);

template
long getSetting(const SettingsKey& key, long defaultValue);

template
unsigned char getSetting(const SettingsKey& key, unsigned char defaultValue);

template
unsigned short getSetting(const SettingsKey& key, unsigned short defaultValue);

template
unsigned int getSetting(const SettingsKey& key, unsigned int defaultValue);

template
unsigned long getSetting(const SettingsKey& key, unsigned long defaultValue);

template
float getSetting(const SettingsKey& key, float defaultValue);

template
double getSetting(const SettingsKey& key, double defaultValue);

String getSetting(const String& key) {
    return settings::kv_store.get(key).value;
}

String getSetting(const __FlashStringHelper* key) {
    return getSetting(String(key));
}

String getSetting(const char* key) {
    return getSetting(String(key));
}

String getSetting(const SettingsKey& key) {
    static const String defaultValue("");
    return getSetting(key, defaultValue);
}

String getSetting(const SettingsKey& key, const char* defaultValue) {
    return getSetting(key, std::move(String(defaultValue)));
}

String getSetting(const SettingsKey& key, const __FlashStringHelper* defaultValue) {
    return getSetting(key, std::move(String(defaultValue)));
}

String getSetting(const SettingsKey& key, const String& defaultValue) {
    auto result = settings::kv_store.get(key.toString());
    if (!result) {
        result.value = defaultValue;
    }

    return result.value;
}

String getSetting(const SettingsKey& key, String&& defaultValue) {
    auto result = settings::kv_store.get(key.toString());
    if (!result) {
        result.value = std::move(defaultValue);
    }

    return result.value;
}

bool delSetting(const String& key) {
    return settings::kv_store.del(key);
}

bool delSetting(const SettingsKey& key) {
    return delSetting(key.toString());
}

bool delSetting(const char* key) {
    return delSetting(String(key));
}

bool delSetting(const __FlashStringHelper* key) {
    return delSetting(String(key));
}

bool hasSetting(const String& key) {
    return settings::kv_store.has(key);
}

bool hasSetting(const SettingsKey& key) {
    return hasSetting(key.toString());
}

bool hasSetting(const char* key) {
    return hasSetting(String(key));
}

bool hasSetting(const __FlashStringHelper* key) {
    return hasSetting(String(key));
}

void saveSettings() {
#if not SETTINGS_AUTOSAVE
    eepromCommit();
#endif
}

void autosaveSettings() {
#if SETTINGS_AUTOSAVE
    eepromCommit();
#endif
}

void resetSettings() {
    eepromClear();
}

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

bool settingsRestoreJson(JsonObject& data) {

    // Note: we try to match what /config generates, expect {"app":"ESPURNA",...}
    const char* app = data["app"];
    if (!app || strcmp(app, APP_NAME) != 0) {
        DEBUG_MSG_P(PSTR("[SETTING] Wrong or missing 'app' key\n"));
        return false;
    }

    // .../config will add this key, but it is optional
    if (data["backup"].as<bool>()) {
        resetSettings();
    }

    // These three are just metadata, no need to actually store them
    for (auto element : data) {
        if (strcmp(element.key, "app") == 0) continue;
        if (strcmp(element.key, "version") == 0) continue;
        if (strcmp(element.key, "backup") == 0) continue;
        setSetting(element.key, element.value.as<char*>());
    }

    saveSettings();

    DEBUG_MSG_P(PSTR("[SETTINGS] Settings restored successfully\n"));
    return true;

}

bool settingsRestoreJson(char* json_string, size_t json_buffer_size) {

     // XXX: as of right now, arduinojson cannot trigger callbacks for each key individually
    // Manually separating kv pairs can allow to parse only a small chunk, since we know that there is only string type used (even with bools / ints). Can be problematic when parsing data that was not generated by us.
    // Current parsing method is limited only by keys (~sizeof(uintptr_t) bytes per key, data is not copied when string is non-const)
    DynamicJsonBuffer jsonBuffer(json_buffer_size);
    JsonObject& root = jsonBuffer.parseObject((char *) json_string);

    if (!root.success()) {
        DEBUG_MSG_P(PSTR("[SETTINGS] JSON parsing error\n"));
        return false;
    }

    return settingsRestoreJson(root);

 }

void settingsGetJson(JsonObject& root) {

    // Get sorted list of keys
    auto keys = settingsKeys();

    // Add the key-values to the json object
    for (unsigned int i=0; i<keys.size(); i++) {
        String value = getSetting(keys[i]);
        root[keys[i]] = value;
    }

}

void settingsProcessConfig(const settings_cfg_list_t& config, settings_filter_t filter) {
    for (auto& entry : config) {
        String value = getSetting(entry.key, entry.default_value);
        if (filter) {
            value = filter(value);
        }
        if (value.equals(entry.setting)) continue;
        entry.setting = std::move(value);
    }
}

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------

void settingsSetup() {

    terminalRegisterCommand(F("CONFIG"), [](const terminal::CommandContext& ctx) {
        // TODO: enough of a buffer?
        DynamicJsonBuffer jsonBuffer(1024);
        JsonObject& root = jsonBuffer.createObject();
        settingsGetJson(root);
        root.prettyPrintTo(ctx.output);
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("KEYS"), [](const terminal::CommandContext& ctx) {
        auto keys = settingsKeys();

        ctx.output.println(F("Current settings:"));
        for (unsigned int i=0; i<keys.size(); i++) {
            const auto value = getSetting(keys[i]);
            ctx.output.printf("> %s => \"%s\"\n", (keys[i]).c_str(), value.c_str());
        }

        auto available [[gnu::unused]] = settings::kv_store.available();
        ctx.output.printf("Number of keys: %u\n", keys.size());
        ctx.output.printf("Available: %u bytes (%u%%)\n", available, (100 * available) / settings::kv_store.size());

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("DEL"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc != 2) {
            terminalError(ctx, F("del <key> [<key>...]"));
            return;
        }

        int result = 0;
        for (auto it = (ctx.argv.begin() + 1); it != ctx.argv.end(); ++it) {
            result += settings::kv_store.del(*it);
        }

        if (result) {
            terminalOK(ctx);
        } else {
            terminalError(ctx, F("no keys were removed"));
        }
    });

    terminalRegisterCommand(F("SET"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc != 3) {
            terminalError(ctx, F("set <key> <value>"));
            return;
        }

        if (settings::kv_store.set(ctx.argv[1], ctx.argv[2])) {
            terminalOK(ctx);
            return;
        }

        terminalError(ctx, F("could not set the key"));
    });

    terminalRegisterCommand(F("GET"), [](const terminal::CommandContext& ctx) {
        if (ctx.argc < 2) {
            terminalError(ctx, F("Wrong arguments"));
            return;
        }

        for (auto it = (ctx.argv.begin() + 1); it != ctx.argv.end(); ++it) {
            const String& key = *it;
            auto result = settings::kv_store.get(key);
            if (!result) {
                const auto maybeDefault = settingsQueryDefaults(key);
                if (maybeDefault.length()) {
                    ctx.output.printf("> %s => %s (default)\n", key.c_str(), maybeDefault.c_str());
                } else {
                    ctx.output.printf("> %s =>\n", key.c_str());
                }
                continue;
            }

            ctx.output.printf("> %s => \"%s\"\n", key.c_str(), result.value.c_str());
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("RELOAD"), [](const terminal::CommandContext& ctx) {
        espurnaReload();
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("FACTORY.RESET"), [](const terminal::CommandContext& ctx) {
        factoryReset();
        terminalOK(ctx);
    });

#if not SETTINGS_AUTOSAVE
    terminalRegisterCommand(F("SAVE"), [](const terminal::CommandContext& ctx) {
        eepromCommit();
        terminalOK(ctx);
    });
#endif

}
