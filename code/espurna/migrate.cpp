/*

MIGRATE MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>

*/

#include "settings.h"

#include <vector>
#include <utility>

namespace {

void delPrefixes(const char** prefixes, size_t size) {
    std::vector<String> to_purge;

    using namespace settings;
    kv_store.foreach([&](kvs_type::KeyValueResult&& kv) {
        auto key = kv.key.read();
        for (size_t index = 0; index < size; ++index) {
            if (key.startsWith(prefixes[index])) {
                to_purge.push_back(std::move(key));
                return;
            }
        }
    });

    for (auto& key : to_purge) {
        delSetting(key);
    }
}

template <size_t Size>
void delPrefixes(const char* (&&prefixes)[Size]) {
    delPrefixes(prefixes, Size);
}

} // namespace

// Configuration versions
//
// 1: based on Embedis, no board definitions
// 2: based on Embedis, with board definitions 1-based
// 3: based on Embedis, with board definitions 0-based
// 4: based on Embedis, no board definitions
// 5: based on Embedis, updated rfb codes format

int migrateVersion() {
    const static auto version = getSetting("cfg", CFG_VERSION);
    if (version == CFG_VERSION) {
        return 0;
    }

    return version;
}

void migrate() {

    // We either get 0, when version did not change
    // Or, the version we migrate from
    const auto version = migrateVersion();
    setSetting("cfg", CFG_VERSION);

    if (!version) {
        return;
    }

    // get rid of old keys that were never used until now
    // and some very old keys that were forced via migrate.ino
    switch (version) {
    case 2:
        delPrefixes({
            "btnGPIO",
            "ledGPIO",
            "ledLogic",
            "relayGPIO",
            "relayType"
        });
        // fall through
    case 3:
    case 4:
        delPrefixes({
            "board",
            "chGPIO",
            "chLogic",
            "ledGPIO",
            "ledLogic",
            "lightProvider",
            "myChips",
            "myDCKGPIO",
            "myDIGPIO",
            "relayGPIO",
            "relayProvider",
            "relayType",
            "relays"
        });
        break;
    }

    saveSettings();

}
