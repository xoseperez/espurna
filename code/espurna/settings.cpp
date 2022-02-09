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

// -----------------------------------------------------------------------------

namespace settings {
namespace {

// Depending on features enabled, we may end up with different left boundary
// Settings are written right-to-left, so we only have issues when there are a lot of key-values
// XXX: slightly hacky, because we EEPROMr.length() is 0 before we enter setup() code
static kvs_type kv_store(
    EepromStorage{},
#if DEBUG_SUPPORT
    EepromReservedSize + crashReservedSize(),
#else
    EepromReservedSize,
#endif
    EepromSize
);

} // namespace
} // namespace settings

// --------------------------------------------------------------------------

namespace settings {
namespace query {

String Setting::findValueFrom(const Setting* begin, const Setting* end, StringView key) {
    String out;

    for (auto it = begin; it != end; ++it) {
        if ((*it) == key) {
            out = (*it).value();
            break;
        }
    }

    return out;
}

bool IndexedSetting::findSamePrefix(const IndexedSetting* begin, const IndexedSetting* end, StringView key) {
    for (auto it = begin; it != end; ++it) {
        if (samePrefix(key, (*it).prefix())) {
            return true;
        }
    }

    return false;
}

String IndexedSetting::findValueFrom(Iota iota, const IndexedSetting* begin, const IndexedSetting* end, StringView key) {
    String out;

    while (iota) {
        for (auto it = begin; it != end; ++it) {
            const auto expected = SettingsKey(
                (*it).prefix().toString(), *iota);
            if (key == expected.value()) {
                out = (*it).value(*iota);
                goto output;
            }
        }
        ++iota;
    }

output:
    return out;
}

namespace internal {
namespace {

std::forward_list<Handler> handlers;

} // namespace
} // namespace internal
} // namespace query

namespace options {

bool EnumerationNumericHelper::check(const String& value) {
    if (value.length()) {
        if ((value.length() > 1) && (*value.begin() == '0')) {
            return false;
        }

        for (auto it = value.begin(); it != value.end(); ++it) {
            switch (*it) {
            case '0'...'9':
                break;
            default:
                return false;
            }
        }

        return true;
    }

    return false;
}

} // namespace options

namespace internal {

ValueResult get(const String& key) {
    return kv_store.get(key);
}

bool set(const String& key, const String& value) {
    return kv_store.set(key, value);
}

bool del(const String& key) {
    return kv_store.del(key);
}

bool has(const String& key) {
    return kv_store.has(key);
}

Keys keys() {
    Keys out;
    kv_store.foreach([&](kvs_type::KeyValueResult&& kv) {
        out.push_back(kv.key.read());
    });

    return out;
}

size_t available() {
    return kv_store.available();
}

size_t size() {
    return kv_store.size();
}

void foreach(KeyValueResultCallback&& callback) {
    kv_store.foreach(callback);
}

void foreach_prefix(PrefixResultCallback&& callback, query::StringViewIterator prefixes) {
    kv_store.foreach([&](kvs_type::KeyValueResult&& kv) {
        auto key = kv.key.read();
        for (auto it = prefixes.begin(); it != prefixes.end(); ++it) {
            if (::settings::query::samePrefix(::settings::StringView{key}, (*it))) {
                callback((*it), std::move(key), kv.value);
            }
        }
    });
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
signed char convert(const String& value) {
    return value.toInt();
}

template <>
short convert(const String& value) {
    return value.toInt();
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
uint32_t convert(const String& value) {
    return parseUnsigned(value);
}

String serialize(uint32_t value, int base) {
    return formatUnsigned(value, base);
}

template <>
unsigned long convert(const String& value) {
    return convert<unsigned int>(value);
}

template <>
unsigned short convert(const String& value) {
    return convert<unsigned long>(value);
}

template <>
unsigned char convert(const String& value) {
    return convert<unsigned long>(value);
}

} // namespace internal
} // namespace settings

// -----------------------------------------------------------------------------
// Key-value API
// -----------------------------------------------------------------------------

size_t settingsSize() {
    return settings::internal::size() - settings::internal::available();
}

// TODO: UI needs this to avoid showing keys in storage order
std::vector<String> settingsKeys() {
    auto keys = settings::internal::keys();
    std::sort(keys.begin(), keys.end(), [](const String& rhs, const String& lhs) -> bool {
        return lhs.compareTo(rhs) > 0;
    });

    return keys;
}

void settingsRegisterQueryHandler(settings::query::Handler handler) {
    settings::query::internal::handlers.push_front(handler);
}

String settingsQuery(::settings::StringView key) {
    String out;

    for (const auto& handler : settings::query::internal::handlers) {
        if (handler.check(key)) {
            out = handler.get(key);
            break;
        }
    }

    return out;
}

void moveSetting(const String& from, const String& to) {
    auto result = settings::internal::get(from);
    if (result) {
        setSetting(to, result.ref());
    }
    delSetting(from);
}

using SettingsKeyPair = std::pair<SettingsKey, SettingsKey>;

void moveSetting(const String& from, const String& to, size_t index) {
    const SettingsKeyPair keys = {{from, index}, {to, index}};

    auto result = settings::internal::get(keys.first.value());
    if (result) {
        setSetting(keys.second, result.ref());
    }

    delSetting(keys.first);
}

void moveSettings(const String& from, const String& to) {
    for (size_t index = 0; index < 100; ++index) {
        const SettingsKeyPair keys = {{from, index}, {to, index}};
        auto result = settings::internal::get(keys.first.value());
        if (!result) {
            break;
        }

        setSetting(keys.second, result.ref());
        delSetting(keys.first);
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
    return std::move(settings::internal::get(key)).get();
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
    return getSetting(key, String(defaultValue));
}

String getSetting(const SettingsKey& key, const __FlashStringHelper* defaultValue) {
    return getSetting(key, String(defaultValue));
}

String getSetting(const SettingsKey& key, const String& defaultValue) {
    auto result = settings::internal::get(key.value());
    if (result) {
        return std::move(result).get();
    }

    return defaultValue;
}

String getSetting(const SettingsKey& key, String&& defaultValue) {
    auto result = settings::internal::get(key.value());
    if (result) {
        return std::move(result).get();
    }

    return std::move(defaultValue);
}

bool delSetting(const String& key) {
    return settings::internal::del(key);
}

bool delSetting(const SettingsKey& key) {
    return delSetting(key.value());
}

bool delSetting(const char* key) {
    return delSetting(String(key));
}

bool delSetting(const __FlashStringHelper* key) {
    return delSetting(String(key));
}

bool hasSetting(const String& key) {
    return settings::internal::has(key);
}

bool hasSetting(const SettingsKey& key) {
    return hasSetting(key.value());
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
    if (!app || strcmp(app, getAppName()) != 0) {
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

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------

#if TERMINAL_SUPPORT

void settingsDump(const ::terminal::CommandContext& ctx, const ::settings::query::Setting* begin, const ::settings::query::Setting* end) {
    for (auto it = begin; it != end; ++it) {
        ctx.output.printf_P(PSTR("> %s => %s\n"),
            (*it).key().c_str(), (*it).value().c_str());
    }
}

void settingsDump(const ::terminal::CommandContext& ctx, const ::settings::query::IndexedSetting* begin, const ::settings::query::IndexedSetting* end, size_t index) {
    for (auto it = begin; it != end; ++it) {
        ctx.output.printf_P(PSTR("> %s%u => %s\n"),
            (*it).prefix().c_str(), index,
            (*it).value(index).c_str());
    }
}

namespace {

void _settingsInitCommands() {
    terminalRegisterCommand(F("CONFIG"), [](::terminal::CommandContext&& ctx) {
        // TODO: enough of a buffer?
        DynamicJsonBuffer jsonBuffer(1024);
        JsonObject& root = jsonBuffer.createObject();
        settingsGetJson(root);
        root.prettyPrintTo(ctx.output);
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("KEYS"), [](::terminal::CommandContext&& ctx) {
        auto keys = settingsKeys();

        String value;
        for (unsigned int i=0; i<keys.size(); i++) {
            value = getSetting(keys[i]);
            ctx.output.printf_P(PSTR("> %s => \"%s\"\n"), (keys[i]).c_str(), value.c_str());
        }

        auto available [[gnu::unused]] = settings::internal::available();
        ctx.output.printf_P(PSTR("Number of keys: %u\n"), keys.size());
        ctx.output.printf_P(PSTR("Available: %u bytes (%u%%)\n"),
                available, (100 * available) / settings::internal::size());

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("DEL"), [](::terminal::CommandContext&& ctx) {
        if (ctx.argv.size() < 2) {
            terminalError(ctx, F("del <key> [<key>...]"));
            return;
        }

        int result = 0;
        for (auto it = (ctx.argv.begin() + 1); it != ctx.argv.end(); ++it) {
            result += settings::internal::del(*it);
        }

        if (result) {
            terminalOK(ctx);
        } else {
            terminalError(ctx, F("no keys were removed"));
        }
    });

    terminalRegisterCommand(F("SET"), [](::terminal::CommandContext&& ctx) {
        if (ctx.argv.size() != 3) {
            terminalError(ctx, F("set <key> <value>"));
            return;
        }

        if (settings::internal::set(ctx.argv[1], ctx.argv[2])) {
            terminalOK(ctx);
            return;
        }

        terminalError(ctx, F("could not set the key"));
    });

    terminalRegisterCommand(F("GET"), [](::terminal::CommandContext&& ctx) {
        if (ctx.argv.size() < 2) {
            terminalError(ctx, F("get <key> [<key>...]"));
            return;
        }

        for (auto it = (ctx.argv.cbegin() + 1); it != ctx.argv.cend(); ++it) {
            auto key = ::settings::StringView { *it };

            auto result = settings::internal::get(key.toString());
            if (!result) {
                const auto maybeValue = settingsQuery(key);
                if (maybeValue.length()) {
                    ctx.output.printf_P(PSTR("> %s => %s (default)\n"), key.c_str(), maybeValue.c_str());
                } else {
                    ctx.output.printf_P(PSTR("> %s =>\n"), key.c_str());
                }
                continue;
            }

            ctx.output.printf_P(PSTR("> %s => \"%s\"\n"), key.c_str(), result.c_str());
        }

        terminalOK(ctx);
    });

    terminalRegisterCommand(F("RELOAD"), [](::terminal::CommandContext&& ctx) {
        espurnaReload();
        terminalOK(ctx);
    });

    terminalRegisterCommand(F("FACTORY.RESET"), [](::terminal::CommandContext&& ctx) {
        factoryReset();
        terminalOK(ctx);
    });

#if not SETTINGS_AUTOSAVE
    terminalRegisterCommand(F("SAVE"), [](::terminal::CommandContext&& ctx) {
        eepromCommit();
        terminalOK(ctx);
    });
#endif
}

} // namespace
#endif

void settingsSetup() {
#if TERMINAL_SUPPORT
    _settingsInitCommands();
#endif
}
