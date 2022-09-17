/*

BUILD INFO

*/

#pragma once

#include <Arduino.h>

#include "types.h"

namespace espurna {
namespace build {

struct Sdk {
    StringView base;
    StringView version;
    StringView revision;
};

struct Hardware {
    StringView manufacturer;
    StringView device;
};

struct App {
    StringView name;
    StringView version;
    StringView build_time;
    StringView author;
    StringView website;
};

struct Info {
    Sdk sdk;
    Hardware hardware;
    App app;
};

} // namespace build
} // namespace espruna

time_t buildTime();

espurna::build::Info buildInfo();

espurna::build::Sdk buildSdk();
espurna::build::Hardware buildHardware();
espurna::build::App buildApp();

espurna::StringView buildModules();
