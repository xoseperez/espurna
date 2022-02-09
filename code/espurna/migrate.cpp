/*

Part of the SETTINGS MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "settings.h"
#include "config/version.h"

#include <vector>
#include <utility>

namespace espurna {
namespace settings {
namespace schema {
namespace {

// Configuration version for the internal key-value storage
// Represented as a 32bit int, updates every time things change
constexpr static int Version PROGMEM { CFG_VERSION };
alignas(4) static constexpr char Key[] PROGMEM = "cfg";

int version() {
    return getSetting(Key, Version);
}

} // namespace
} // namespace schema

namespace migrate {
namespace {

void deletePrefixes(::settings::query::StringViewIterator prefixes) {
    std::vector<String> to_purge;

    ::settings::internal::foreach_prefix([&](::settings::StringView, String key, const ::settings::kvs_type::ReadResult&) {
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

} // namespace
} // namespace migrate
} // namespace settings
} // namespace espurna

void delSettingPrefix(::settings::query::StringViewIterator prefixes) {
    espurna::settings::migrate::deletePrefixes(std::move(prefixes));
}

int migrateVersion() {
    return espurna::settings::migrate::currentVersion();
}

void migrateVersion(MigrateVersionCallback callback) {
    static const auto current = espurna::settings::migrate::currentVersion();
    if (current) {
        callback(current);
    }
}

void migrate() {
    using namespace espurna::settings::schema;
    setSetting(FPSTR(Key), Version);

    using namespace espurna::settings::migrate;
    switch (currentVersion()) {
    case 2:
    case 3:
    case 4:
        delSetting(F("board"));
        saveSettings();
        break;
    }
}
