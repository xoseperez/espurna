/*

Part of the TERMINAL MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2020 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <Arduino.h>
#include <vector>

namespace terminal {
namespace parsing {

// Generic command line parser
// - split each arg from the input line and put them into the argv array
// - argc is expected to be equal to the argv.size()
struct CommandLine {
    using Argv = std::vector<String>;
    Argv argv;
};

CommandLine parse_commandline(const char* line);

} // namespace parsing
} // namespace terminal
