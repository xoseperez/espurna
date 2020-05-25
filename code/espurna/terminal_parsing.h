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
// - argc is expected to be equal to the argv
struct CommandLine {
    std::vector<String> argv;
    size_t argc;
};

CommandLine parse_commandline(const char *line);

// Fowler–Noll–Vo hash function to hash command strings that treats input as lowercase
// ref: https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
//
// TODO: afaik, unordered_map should handle collisions (however rare they are in our case)
//       if not, we can always roll static commands allocation and just match strings
//       with LowercaseEquals (which is not that much slower)
template <typename T>
struct LowercaseFnv1Hash {
    size_t operator()(const T& str) const;
};

template <typename T>
struct LowercaseEquals {
    bool operator()(const T& lhs, const T& rhs) const;
};

}
}
