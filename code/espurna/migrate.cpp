/*

Part of the SETTINGS MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "settings.h"
#include "system.h"

#include "config/version.h"

#include <vector>
#include <utility>

namespace espurna {
namespace settings {
namespace {

namespace schema {

// Configuration version for the internal key-value storage
// Represented as a 32bit int, updates every time things change
constexpr static int Version PROGMEM { CFG_VERSION };
PROGMEM_STRING(Key, "cfg");

int version() {
    return getSetting(Key, Version);
}

} // namespace schema

namespace migrate {

void deletePrefixes(query::StringViewIterator prefixes) {
    std::vector<String> to_purge;

    foreach_prefix([&](StringView, String key, const kvs_type::ReadResult&) {
        to_purge.push_back(std::move(key));
    }, prefixes);

    for (const auto& key : to_purge) {
        delSetting(key);
    }
}

int currentVersion() {
    const static auto current = schema::version();
    if (current != schema::Version) {
        return current;
    }

    return 0;
}

void run(MigrateVersionCallback callback) {
    static const auto current = currentVersion();
    if (current) {
        callback(current);
    }
}

void run() {
    setSetting(FPSTR(schema::Key), schema::Version);

    if (currentVersion() < 4) {
        delSetting(F("board"));
    }

    if (currentVersion() < 12) {
        const auto hostname = systemHostname();
        if (systemIdentifier() == hostname) {
            delSetting(F("hostname"));
        }

        delSetting(F("boardName"));
    }

    saveSettings();
}

} // namespace migrate

} // namespace
} // namespace settings
} // namespace espurna

void delSettingPrefix(espurna::settings::query::StringViewIterator prefixes) {
    espurna::settings::migrate::deletePrefixes(std::move(prefixes));
}

int migrateVersion() {
    return espurna::settings::migrate::currentVersion();
}

void migrateVersion(MigrateVersionCallback callback) {
    return espurna::settings::migrate::run(callback);
}

void migrate() {
    espurna::settings::migrate::run();
}
