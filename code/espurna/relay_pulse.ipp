/*

Part of the RELAY MODULE

Copyright (C) 2016-2019 by Xose Pérez <xose dot perez at gmail dot com>
Copyright (C) 2019-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include <cstdint>

namespace espurna {
namespace relay {
namespace pulse {
namespace {

Result parse(StringView value) {
    return internal::parse(value.begin(), value.end());
}

#if 0
void test() {
    auto report = [](const String& value) {
        const auto result = parse(value);
        DEBUG_MSG_P(PSTR(":\"%s\" is #%c -> %u (ms)\n"),
            value.c_str(),
            static_cast<bool>(result) ? 't' : 'f',
            result.count());
    };

    report("5h");
    report("7h6h");
    report("15m");
    report("19m1h");
    report("12345");
    report("1.5");
}
#endif

} // namespace
} // namespace pulse
} // namespace relay
} // namespace espurna
