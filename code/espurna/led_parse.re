/*

LED MODULE

Copyright (C) 2020-2021 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#pragma once

#include "utils.h"
#include "led_internal.h"
#include "led_pattern.ipp"

namespace espurna {
namespace led {
namespace {

using ParseDuration = espurna::duration::Milliseconds;

using DurationPair = espurna::duration::Pair;
using ParseResult = espurna::duration::PairResult;

Duration native_duration(DurationPair pair) {
    using namespace espurna::duration;
    return to_chrono<Duration>(pair);
}

Duration native_duration(ParseResult result) {
    return result.ok
        ? native_duration(result.value)
        : Duration::min();
}

ParseResult parse_time(StringView view) {
    using namespace espurna::duration;
    return parse(view, ParseDuration::period{});
}

static constexpr auto RepeatsMax = size_t{ 255 };

// Scans input string with format
// '<on1>,<off1>,<repeats1> <on2>,<off2>,<repeats2> ...'
// And returns a list of Delay objects for the pattern

Pattern parse(StringView value) {
    Pattern out;

    StringView tmp;

    const char* p1;
    const char* p2;
    const char* p3;

    const char* YYCURSOR { value.begin() };
    const char* YYLIMIT { value.end() };
    const char* YYMARKER;

loop:
/*!stags:re2c format = 'const char *@@;'; */
/*!re2c
        re2c:define:YYCTYPE = char;
        re2c:flags:tags = 1;
        re2c:yyfill:enable   = 0;
        re2c:yych:conversion = 1;
        re2c:indent:top      = 1;
        re2c:eof = 0;

        end = "\x00";
        wsp = [ \t\v\r\n]+;

        num = [0-9]+;
        spec = num ([a-zA-z]{1,2})?;

        $ { goto return_out; }
        * { goto return_out; }

        wsp { goto loop; }

        @p1 spec [,] @p2 spec [,] (@p3 num)? {
            tmp = StringView(p1, p2 - p1 - 1);
            const auto on = parse_time(tmp);
            if (!on.ok) {
                goto return_out;
            }
            
            tmp = StringView(p2, p3 - p2 - 1);
            const auto off = parse_time(tmp);
            if (!off.ok) {
                goto return_out;
            }

            size_t repeats_value;
            if (p3) {
                tmp = StringView(p3, YYCURSOR);
                const auto repeats = parseUnsigned(tmp, 10);
                if (!repeats.ok) {
                    goto return_out;
                }
                repeats_value = repeats.value;
            } else {
                repeats_value = 0;
            }

            repeats_value = std::min(repeats_value, RepeatsMax);

            out.add(
                native_duration(on),
                native_duration(off),
                repeats_value);
            if (repeats_value) {
                goto loop;
            }
        }
*/

return_out:
    return out;
}

} // namespace
} // namespace led
} // namespace espurna
